//
//  Snapshot.h
//  interface/src/ui
//
//  Created by Stojce Slavkovski on 1/26/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Snapshot_h
#define hifi_Snapshot_h

#include "InterfaceConfig.h"

#include <QImage>
#include <QGLWidget>
#include <QString>

#include "avatar/Avatar.h"

class SnapshotMetaData {
public:
    
    QString getDomain() { return _domain; }
    void setDomain(QString domain) { _domain = domain; }
    
    glm::vec3 getLocation() { return _location; }
    void setLocation(glm::vec3 location) { _location = location; }
    
    glm::quat getOrientation() { return _orientation; }
    void setOrientation(glm::quat orientation) { _orientation = orientation; }
    
private:
    QString _domain;
    glm::vec3 _location;
    glm::quat _orientation;;
};

class Snapshot {

public:
    static void saveSnapshot(QGLWidget* widget, Avatar* avatar);
    static SnapshotMetaData* parseSnapshotData(QString snapshotPath);
};

#endif // hifi_Snapshot_h
