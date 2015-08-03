//
//  AssetClient.cpp
//
//  Created by Ryan Huffman on 2015/07/21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetClient.h"

#include <QThread>

#include "AssetRequest.h"
#include "NodeList.h"
#include "PacketReceiver.h"

MessageID AssetClient::_currentID = 0;


AssetClient::AssetClient() {
    auto& packetReceiver = DependencyManager::get<NodeList>()->getPacketReceiver();
    packetReceiver.registerListener(PacketType::AssetGetInfoReply, this, "handleAssetGetInfoReply");
    packetReceiver.registerListener(PacketType::AssetGetReply, this, "handleAssetGetReply");
    packetReceiver.registerListener(PacketType::AssetUploadReply, this, "handleAssetUploadReply");
}

AssetRequest* AssetClient::create(QString hash) {
    if (QThread::currentThread() != thread()) {
        AssetRequest* req;
        QMetaObject::invokeMethod(this, "create",
            Qt::BlockingQueuedConnection, 
            Q_RETURN_ARG(AssetRequest*, req),
            Q_ARG(QString, hash));
        return req;
    }

    if (hash.length() != 32) {
        qDebug() << "Invalid hash size";
        return nullptr;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto assetClient = DependencyManager::get<AssetClient>();
        auto request = new AssetRequest(assetClient.data(), hash);

        return request;
    }

    return nullptr;
}

bool AssetClient::getAsset(QString hash, DataOffset start, DataOffset end, ReceivedAssetCallback callback) {
    if (hash.length() != HASH_HEX_LENGTH) {
        qDebug() << "Invalid hash size";
        return false;
    }

    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetGet);

        auto messageID = ++_currentID;
        packet->writePrimitive(messageID);
        packet->write(hash.toLatin1().constData(), 32);
        packet->writePrimitive(start);
        packet->writePrimitive(end);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingRequests[messageID] = callback;

        return true;
    }

    return false;
}

bool AssetClient::getAssetInfo(QString hash, GetInfoCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);

    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetGetInfo);

        auto messageID = ++_currentID;
        packet->writePrimitive(messageID);
        packet->write(hash.toLatin1().constData(), 32);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingInfoRequests[messageID] = callback;

        return true;
    }

    return false;
}

void AssetClient::handleAssetGetInfoReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    MessageID messageID;
    packet->readPrimitive(&messageID);
    auto assetHash = QString(packet->read(HASH_HEX_LENGTH));
    
    AssetServerError error;
    packet->readPrimitive(&error);

    AssetInfo info;

    if (!error) {
        packet->readPrimitive(&info.size);
    }

    if (_pendingInfoRequests.contains(messageID)) {
        auto callback = _pendingInfoRequests.take(messageID);
        callback(error != NO_ERROR, info);
    }
}

void AssetClient::handleAssetGetReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    auto assetHash = packet->read(HASH_HEX_LENGTH);
    qDebug() << "Got reply for asset: " << assetHash;

    MessageID messageID;
    packet->readPrimitive(&messageID);

    AssetServerError error;
    packet->readPrimitive(&error);
    QByteArray data;

    if (!error) {
        DataOffset length;
        packet->readPrimitive(&length);
        data = packet->read(length);
        qDebug() << "Got data: " << length << ", " << data.toHex();
    } else {
        qDebug() << "Failure getting asset: " << error;
    }

    if (_pendingRequests.contains(messageID)) {
        auto callback = _pendingRequests.take(messageID);
        callback(!error, data);
    }
}

bool AssetClient::uploadAsset(QByteArray data, QString extension, UploadResultCallback callback) {
    auto nodeList = DependencyManager::get<NodeList>();
    SharedNodePointer assetServer = nodeList->soloNodeOfType(NodeType::AssetServer);
    if (assetServer) {
        auto packet = NLPacket::create(PacketType::AssetUpload);

        auto messageID = _currentID++;
        packet->writePrimitive(messageID);

        packet->writePrimitive(static_cast<char>(extension.length()));
        packet->write(extension.toLatin1().constData(), extension.length());

        qDebug() << "Extension length: " << extension.length();
        qDebug() << "Extension: " << extension;

        int size = data.length();
        packet->writePrimitive(size);
        packet->write(data.constData(), size);

        nodeList->sendPacket(std::move(packet), *assetServer);

        _pendingUploads[messageID] = callback;

        return true;
    }
    return false;
}

void AssetClient::handleAssetUploadReply(QSharedPointer<NLPacket> packet, SharedNodePointer senderNode) {
    qDebug() << "Got asset upload reply";
    MessageID messageID;
    packet->readPrimitive(&messageID);

    bool success;
    packet->readPrimitive(&success);

    QString hashString { "" };

    if (success) {
        auto hashData = packet->read(HASH_HEX_LENGTH);

        hashString = QString(hashData);

        qDebug() << "Hash: " << hashString;
    } else {
        qDebug() << "Error uploading file";
    }

    if (_pendingUploads.contains(messageID)) {
        auto callback = _pendingUploads.take(messageID);
        callback(success, hashString);
    }
}
