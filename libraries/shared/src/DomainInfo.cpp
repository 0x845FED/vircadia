//
//  DomainInfo.cpp
//  hifi
//
//  Created by Stephen Birarda on 2/18/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QJsonObject>

#include "AccountManager.h"

#include "DomainInfo.h"

DomainInfo::DomainInfo() :
    _uuid(),
    _sockAddr(HifiSockAddr(QHostAddress::Null, DEFAULT_DOMAIN_SERVER_PORT)),
    _assignmentUUID(),
    _connectionSecret(),
    _registrationToken(),
    _rootAuthenticationURL(),
    _publicKey(),
    _isConnected(false)
{
    // clear appropriate variables after a domain-server logout
    connect(&AccountManager::getInstance(), &AccountManager::logoutComplete, this, &DomainInfo::logout);
}

void DomainInfo::reset() {
    _uuid = QUuid();
    _hostname = QString();
    _sockAddr.setAddress(QHostAddress::Null);
    _assignmentUUID = QUuid();
    _connectionSecret = QString();
    _registrationToken = QByteArray();
    _rootAuthenticationURL = QUrl();
    _publicKey = QString();
    _isConnected = false;
}

void DomainInfo::parseAuthInformationFromJsonObject(const QJsonObject& jsonObject) {
    QJsonObject dataObject = jsonObject["data"].toObject();
    _connectionSecret = QUuid(dataObject["connection_secret"].toString());
    _registrationToken = QByteArray::fromHex(dataObject["registration_token"].toString().toUtf8());
    _publicKey = dataObject["public_key"].toString();
}

void DomainInfo::setHostname(const QString& hostname) {
    
    if (hostname != _hostname) {
        // re-set the domain info so that auth information is reloaded
        reset();
        
        int colonIndex = hostname.indexOf(':');
        
        if (colonIndex > 0) {
            // the user has included a custom DS port with the hostname
            
            // the new hostname is everything up to the colon
            _hostname = hostname.left(colonIndex);
            
            // grab the port by reading the string after the colon
            _sockAddr.setPort(atoi(hostname.mid(colonIndex + 1, hostname.size()).toLocal8Bit().constData()));
            
            qDebug() << "Updated hostname to" << _hostname << "and port to" << _sockAddr.getPort();
            
        } else {
            // no port included with the hostname, simply set the member variable and reset the domain server port to default
            _hostname = hostname;
            _sockAddr.setPort(DEFAULT_DOMAIN_SERVER_PORT);
        }
        
        // re-set the sock addr to null and fire off a lookup of the IP address for this domain-server's hostname
        qDebug("Looking up DS hostname %s.", _hostname.toLocal8Bit().constData());
        QHostInfo::lookupHost(_hostname, this, SLOT(completedHostnameLookup(const QHostInfo&)));
        
        emit hostnameChanged(_hostname);
    }
}

void DomainInfo::completedHostnameLookup(const QHostInfo& hostInfo) {
    for (int i = 0; i < hostInfo.addresses().size(); i++) {
        if (hostInfo.addresses()[i].protocol() == QAbstractSocket::IPv4Protocol) {
            _sockAddr.setAddress(hostInfo.addresses()[i]);
            qDebug("DS at %s is at %s", _hostname.toLocal8Bit().constData(),
                   _sockAddr.getAddress().toString().toLocal8Bit().constData());
            return;
        }        
    }
    
    // if we got here then we failed to lookup the address
    qDebug("Failed domain server lookup");
}

void DomainInfo::setIsConnected(bool isConnected) {
    if (_isConnected != isConnected) {
        _isConnected = isConnected;
        
        if (_isConnected) {
            emit connectedToDomain(_hostname);
        }
    }
}

void DomainInfo::logout() {
    // clear any information related to auth for this domain, assuming it had requested auth
    if (!_rootAuthenticationURL.isEmpty()) {
        _rootAuthenticationURL = QUrl();
        _connectionSecret = QUuid();
        _registrationToken = QByteArray();
        _publicKey = QString();
        _isConnected = false;
    }
}
