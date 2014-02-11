//
//  Vec3.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//
//  Scriptable Vec3 class library.
//
//

#include "Vec3.h"

glm::vec3 Vec3::multiply(const glm::vec3& v1, const glm::vec3& v2) {
    return v1 * v2;
}

glm::vec3 Vec3::multiply(const glm::vec3& v1, float f) {
    return v1 * f;
}

glm::vec3 Vec3::sum(const glm::vec3& v1, const glm::vec3& v2) {
    return v1 + v2;
}

glm::vec3 Vec3::subtract(const glm::vec3& v1, const glm::vec3& v2) {
        return v1 - v2;
}
