//
//  CollisionInfo.cpp
//  hifi
//
//  Created by Andrew Meadows on 2014.02.14
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include "CollisionInfo.h"

void CollisionInfo::rotateThenTranslate(const glm::quat& rotation, const glm::vec3& translation) {
    _contactPoint = translation + rotation * _contactPoint;
    _penetration = rotation * _penetration;
    _addedVelocity = rotation * _addedVelocity;
}

void CollisionInfo::translateThenRotate(const glm::vec3& translation, const glm::quat& rotation) {
    _contactPoint = rotation * (_contactPoint + translation);
    _penetration = rotation * _penetration;
    _addedVelocity = rotation * _addedVelocity;
}

CollisionList::CollisionList(int maxSize) :
    _maxSize(maxSize),
    _size(0) {
    _collisions.resize(_maxSize);
}

CollisionInfo* CollisionList::getNewCollision() {
    // return pointer to existing CollisionInfo, or NULL of list is full
    return (_size < _maxSize) ? &(_collisions[++_size]) : NULL;
}

CollisionInfo* CollisionList::getCollision(int index) {
    return (index > -1 && index < _size) ? &(_collisions[index]) : NULL;
}

void CollisionList::clear() {
    for (int i = 0; i < _size; ++i) {
        // we only clear the important stuff
        CollisionInfo& collision = _collisions[i];
        collision._type = BASE_COLLISION;
        collision._data = NULL; // CollisionInfo does not own whatever this points to.
        collision._flags = 0;
        // we rely on the consumer to properly overwrite these fields when the collision is "created"
        //collision._damping;
        //collision._elasticity;
        //collision._contactPoint;
        //collision._penetration;
        //collision._addedVelocity;
    }
    _size = 0;
}

