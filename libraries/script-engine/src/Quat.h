//
//  Quat.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable Quaternion class library.
//
//

#ifndef __hifi__Quat__
#define __hifi__Quat__

#include <glm/gtc/quaternion.hpp>
#include <QtCore/QObject>

/// Scriptable interface a Quaternion helper class object. Used exclusively in the JavaScript API
class Quat : public QObject {
    Q_OBJECT

public slots:
    glm::quat multiply(const glm::quat& q1, const glm::quat& q2);
    glm::quat fromVec3(const glm::vec3& vec3);
};



#endif /* defined(__hifi__Quat__) */
