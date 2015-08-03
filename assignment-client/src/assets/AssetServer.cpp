//
//  AssetServer.cpp
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#include "AssetServer.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>

#include <NodeType.h>

const QString ASSET_SERVER_LOGGING_TARGET_NAME = "asset-server";

AssetServer::AssetServer(NLPacket& packet) : ThreadedAssignment(packet) {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGet, this, "handleAssetGet");
    packetReceiver.registerListener(PacketType::AssetGetInfo, this, "handleAssetGetInfo");
    packetReceiver.registerListener(PacketType::AssetUpload, this, "handleAssetUpload");
}

AssetServer::~AssetServer() {
}

void AssetServer::run() {
    ThreadedAssignment::commonInit(ASSET_SERVER_LOGGING_TARGET_NAME, NodeType::AssetServer);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    _resourcesDirectory = QDir(QCoreApplication::applicationDirPath()).filePath("resources/assets");
    if (!_resourcesDirectory.exists()) {
        qDebug() << "Creating resources directory";
        _resourcesDirectory.mkpath(".");
    }
    qDebug() << "Serving files from: " << _resourcesDirectory.path();

    // Scan for new files
    qDebug() << "Looking for new files in asset directory";
    auto files = _resourcesDirectory.entryInfoList(QDir::Files);
    QRegExp filenameRegex { "^[a-f0-9]{32}$" };
    for (auto fileInfo : files) {
        auto filename = fileInfo.fileName();
        if (!filenameRegex.exactMatch(filename)) {
            qDebug() << "Found file: " << filename;
            if (!fileInfo.isReadable()) {
                qDebug() << "\tCan't open file for reading: " << filename;
                continue;
            }

            // Read file
            QFile file { fileInfo.absoluteFilePath() };
            file.open(QFile::ReadOnly);
            QByteArray data = file.readAll();

            auto hash = hashData(data);

            qDebug() << "\tMoving " << filename << " to " << hash;

            file.rename(_resourcesDirectory.absoluteFilePath(hash));
        }
    }

    while (!_isFinished) {
        // since we're a while loop we need to help Qt's event processing
        QCoreApplication::processEvents();
    }
}

void AssetServer::writeError(NLPacket* packet, AssetServerError error) {
    packet->writePrimitive(error);
}

void AssetServer::handleAssetGetInfo(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    if (packet->getPayloadSize() < HASH_HEX_LENGTH) {
        qDebug() << "ERROR bad file request";
        return;
    }

    QByteArray assetHash;
    MessageID messageID;
    packet->readPrimitive(&messageID);
    assetHash = packet->read(HASH_HEX_LENGTH);

    auto replyPacket = NLPacket::create(PacketType::AssetGetInfoReply);

    replyPacket->writePrimitive(messageID);
    replyPacket->write(assetHash, HASH_HEX_LENGTH);

    QFileInfo fileInfo { _resourcesDirectory.filePath(QString(assetHash)) };
    qDebug() << "Opening file: " << QString(QFileInfo(assetHash).fileName());

    if (fileInfo.exists() && fileInfo.isReadable()) {
        replyPacket->writePrimitive(AssetServerError::NO_ERROR);
        replyPacket->writePrimitive(fileInfo.size());
    } else {
        qDebug() << "Asset not found: " << assetHash;
        replyPacket->writePrimitive(AssetServerError::ASSET_NOT_FOUND);
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

void AssetServer::handleAssetGet(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    if (packet->getPayloadSize() < HASH_HEX_LENGTH) {
        qDebug() << "ERROR bad file request";
        return;
    }

    MessageID messageID;
    packet->readPrimitive(&messageID);

    QByteArray assetHash;
    assetHash = packet->read(HASH_HEX_LENGTH);

    DataOffset start;
    packet->readPrimitive(&start);

    DataOffset end;
    packet->readPrimitive(&end);

    qDebug() << "Received a request for the file: " << assetHash << " from " << start << " to " << end;

    // We need to reply...
    auto replyPacket = NLPacket::create(PacketType::AssetGetReply);

    replyPacket->write(assetHash, HASH_HEX_LENGTH);

    replyPacket->writePrimitive(messageID);

    const int64_t MAX_LENGTH = 1024;

    if (end <= start) {
        writeError(replyPacket.get(), AssetServerError::INVALID_BYTE_RANGE);
    } else if (end - start > MAX_LENGTH) {
        writeError(replyPacket.get(), AssetServerError::INVALID_BYTE_RANGE);
    } else {
        QFile file { _resourcesDirectory.filePath(QString(assetHash)) };
        qDebug() << "Opening file: " << QString(QFileInfo(assetHash).fileName());

        if (file.open(QIODevice::ReadOnly)) {
            if (file.size() < end) {
                writeError(replyPacket.get(), AssetServerError::INVALID_BYTE_RANGE);
            } else {
                auto size = end - start;
                file.seek(start);
                QByteArray data = file.read(size);
                replyPacket->writePrimitive(AssetServerError::NO_ERROR);
                replyPacket->writePrimitive(size);
                replyPacket->write(data, size);
            }
        } else {
            qDebug() << "Asset not found";
            writeError(replyPacket.get(), AssetServerError::ASSET_NOT_FOUND);
        }
    }

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

void AssetServer::handleAssetUpload(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    packet->readPrimitive(&messageID);

    char extensionLength;
    packet->readPrimitive(&extensionLength);

    char extension[extensionLength];
    packet->read(extension, extensionLength);

    qDebug() << "Got extension: " << extension;

    int fileSize;
    packet->readPrimitive(&fileSize);

    const int MAX_LENGTH = 1024;
    fileSize = std::min(MAX_LENGTH, fileSize);
    qDebug() << "Receiving a file of size " << fileSize;

    QByteArray data = packet->read(fileSize);

    QString hash = hashData(data);

    qDebug() << "Got data: (" << hash << ") " << data;

    QFile file { _resourcesDirectory.filePath(QString(hash)) };

    if (file.exists()) {
        qDebug() << "[WARNING] This file already exists";
    } else {
        file.open(QIODevice::WriteOnly);
        file.write(data);
    }

    auto replyPacket = NLPacket::create(PacketType::AssetUploadReply);

    replyPacket->writePrimitive(messageID);

    replyPacket->writePrimitive(true);
    replyPacket->write(hash.toLatin1().constData(), HASH_HEX_LENGTH);

    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->sendPacket(std::move(replyPacket), *senderNode);
}

QString AssetServer::hashData(const QByteArray& data) {
    return QString(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
}

