//
//  Vec3.cpp
//  libraries/script-engine/src
//
//  Created by Brad Hefta-Gaub on 1/29/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDebug>

#include "Vec3.h"

glm::vec3 Vec3::cross(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::cross(v1,v2);
}

glm::vec3 Vec3::multiply(const glm::vec3& v1, float f) {
    return v1 * f;
}

glm::vec3 Vec3::multiplyQbyV(const glm::quat& q, const glm::vec3& v) {
    return q * v;
}

glm::vec3 Vec3::sum(const glm::vec3& v1, const glm::vec3& v2) { 
    return v1 + v2;
}
glm::vec3 Vec3::subtract(const glm::vec3& v1, const glm::vec3& v2) {
        return v1 - v2;
}

float Vec3::length(const glm::vec3& v) {
    return glm::length(v);
}

float Vec3::distance(const glm::vec3& v1, const glm::vec3& v2) {
    return glm::distance(v1, v2);
}

glm::vec3 Vec3::normalize(const glm::vec3& v) {
    return glm::normalize(v);
}

void Vec3::print(const QString& lable, const glm::vec3& v) {
    qDebug() << qPrintable(lable) << v.x << "," << v.y << "," << v.z;
}
