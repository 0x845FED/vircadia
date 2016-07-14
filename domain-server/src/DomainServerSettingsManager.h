//
//  DomainServerSettingsManager.h
//  domain-server/src
//
//  Created by Stephen Birarda on 2014-06-24.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_DomainServerSettingsManager_h
#define hifi_DomainServerSettingsManager_h

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>

#include <HifiConfigVariantMap.h>
#include <HTTPManager.h>

#include <ReceivedMessage.h>
#include "NodePermissions.h"

const QString SETTINGS_PATHS_KEY = "paths";

const QString SETTINGS_PATH = "/settings";
const QString SETTINGS_PATH_JSON = SETTINGS_PATH + ".json";
const QString AGENT_STANDARD_PERMISSIONS_KEYPATH = "security.standard_permissions";
const QString AGENT_PERMISSIONS_KEYPATH = "security.permissions";
const QString GROUP_PERMISSIONS_KEYPATH = "security.group_permissions";
const QString GROUP_FORBIDDENS_KEYPATH = "security.group_forbiddens";

using GroupByUUIDKey = QPair<QUuid, int>;

class DomainServerSettingsManager : public QObject {
    Q_OBJECT
public:
    DomainServerSettingsManager();
    bool handlePublicHTTPRequest(HTTPConnection* connection, const QUrl& url);
    bool handleAuthenticatedHTTPRequest(HTTPConnection* connection, const QUrl& url);

    void setupConfigMap(const QStringList& argumentList);
    QVariant valueOrDefaultValueForKeyPath(const QString& keyPath);

    QVariantMap& getUserSettingsMap() { return _configMap.getUserConfig(); }
    QVariantMap& getSettingsMap() { return _configMap.getMergedConfig(); }

    QVariantMap& getDescriptorsMap();

    // these give access to anonymous/localhost/logged-in settings from the domain-server settings page
    bool haveStandardPermissionsForName(const QString& name) const { return _standardAgentPermissions.contains(name, 0); }
    NodePermissions getStandardPermissionsForName(const NodePermissionsKey& name) const;

    // these give access to permissions for specific user-names from the domain-server settings page
    bool havePermissionsForName(const QString& name) const { return _agentPermissions.contains(name, 0); }
    NodePermissions getPermissionsForName(const QString& name) const;
    NodePermissions getPermissionsForName(const NodePermissionsKey& key) const { return getPermissionsForName(key.first); }
    QStringList getAllNames() const;

    // these give access to permissions for specific groups from the domain-server settings page
    bool havePermissionsForGroup(const QString& groupName, int rank) const {
        return _groupPermissions.contains(groupName, rank);
    }
    NodePermissions getPermissionsForGroup(const QString& groupName, int rank) const;
    NodePermissions getPermissionsForGroup(const QUuid& groupID, int rank) const;

    // these remove permissions from users in certain groups
    bool haveForbiddensForGroup(const QString& groupName, int rank) const { return _groupForbiddens.contains(groupName, rank); }
    NodePermissions getForbiddensForGroup(const QString& groupName, int rank) const;
    NodePermissions getForbiddensForGroup(const QUuid& groupID, int rank) const;

    QStringList getAllKnownGroupNames();
    bool setGroupID(const QString& groupName, const QUuid& groupID);

    QList<QUuid> getGroupIDs();
    QList<QUuid> getBlacklistGroupIDs();

    // these are used to locally cache the result of calling "api/v1/groups/.../is_member/..." on metaverse's api
    void clearGroupMemberships(const QString& name) { _groupMembership[name].clear(); }
    void recordGroupMembership(const QString& name, const QUuid groupID, int rank);
    int isGroupMember(const QString& name, const QUuid& groupID); // returns rank or -1 if not a member

    // calls http api to refresh group information
    void apiRefreshGroupInformation();


signals:
    void updateNodePermissions();

public slots:
    void apiGetGroupIDJSONCallback(QNetworkReply& requestReply);
    void apiGetGroupIDErrorCallback(QNetworkReply& requestReply);
    void apiGetGroupRanksJSONCallback(QNetworkReply& requestReply);
    void apiGetGroupRanksErrorCallback(QNetworkReply& requestReply);

private slots:
    void processSettingsRequestPacket(QSharedPointer<ReceivedMessage> message);

private:
    QStringList _argumentList;

    QJsonObject responseObjectForType(const QString& typeValue, bool isAuthenticated = false);
    bool recurseJSONObjectAndOverwriteSettings(const QJsonObject& postedObject);

    void updateSetting(const QString& key, const QJsonValue& newValue, QVariantMap& settingMap,
                       const QJsonObject& settingDescription);
    QJsonObject settingDescriptionFromGroup(const QJsonObject& groupObject, const QString& settingName);
    void sortPermissions();
    void persistToFile();

    double _descriptionVersion;
    QJsonArray _descriptionArray;
    HifiConfigVariantMap _configMap;

    friend class DomainServer;

    void validateDescriptorsMap();

    // these cause calls to metaverse's group api
    void apiGetGroupID(const QString& groupName);
    void apiGetGroupRanks(const QUuid& groupID);

    void packPermissionsForMap(QString mapName, NodePermissionsMap& agentPermissions, QString keyPath);
    void packPermissions();
    void unpackPermissions();
    bool ensurePermissionsForGroupRanks();

    NodePermissionsMap _standardAgentPermissions; // anonymous, logged-in, localhost, friend-of-domain-owner
    NodePermissionsMap _agentPermissions; // specific account-names

    NodePermissionsMap _groupPermissions; // permissions granted by membership to specific groups
    NodePermissionsMap _groupForbiddens; // permissions denied due to membership in a specific group
    // these are like _groupPermissions and _groupForbiddens but with uuids rather than group-names in the keys
    QHash<GroupByUUIDKey, NodePermissionsPointer> _groupPermissionsByUUID;
    QHash<GroupByUUIDKey, NodePermissionsPointer> _groupForbiddensByUUID;

    QHash<QString, QUuid> _groupIDs; // keep track of group-name to group-id mappings
    QHash<QUuid, QString> _groupNames; // keep track of group-id to group-name mappings

    // remember the responses to api/v1/groups/%1/ranks
    QHash<QUuid, QVector<QString>> _groupRanks; // QHash<group-id, QVector<rank-name>>
    QHash<QUuid, quint64> _groupRanksLastFetched; // when did we last update _groupRanks

    // keep track of answers to api queries about which users are in which groups
    QHash<QString, QHash<QUuid, int>> _groupMembership; // QHash<user-name, QHash<group-id, rank>>


    void debugDumpGroupsState();
};

#endif // hifi_DomainServerSettingsManager_h
