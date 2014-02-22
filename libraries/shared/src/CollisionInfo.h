//
//  CollisionInfo.h
//  hifi
//
//  Created by Andrew Meadows on 2014.01.13
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __hifi__CollisionInfo__
#define __hifi__CollisionInfo__

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <QVector>

enum CollisionType {
    BASE_COLLISION = 0,
    PADDLE_HAND_COLLISION,
    MODEL_COLLISION,
};

const quint32 COLLISION_GROUP_ENVIRONMENT = 1U << 0;
const quint32 COLLISION_GROUP_AVATARS     = 1U << 1;
const quint32 COLLISION_GROUP_VOXELS      = 1U << 2;
const quint32 COLLISION_GROUP_PARTICLES   = 1U << 3;

// CollisionInfo contains details about the collision between two things: BodyA and BodyB.
// The assumption is that the context that analyzes the collision knows about BodyA but
// does not necessarily know about BodyB.  Hence the data storred in the CollisionInfo
// is expected to be relative to BodyA (for example the penetration points from A into B).

class CollisionInfo {
public:
    CollisionInfo() 
        : _type(0),
        _data(NULL),
        _flags(0),
        _damping(0.f),
        _elasticity(1.f),
        _contactPoint(0.f), 
        _penetration(0.f), 
        _addedVelocity(0.f) {
    }

    CollisionInfo(qint32 type)
        : _type(type),
        _data(NULL),
        _flags(0),
        _damping(0.f),
        _elasticity(1.f),
        _contactPoint(0.f), 
        _penetration(0.f), 
        _addedVelocity(0.f) {
    }

    ~CollisionInfo() {}

    /// Rotate and translate the details.
    void rotateThenTranslate(const glm::quat& rotation, const glm::vec3& translation);

    /// Translate then rotate the details
    void translateThenRotate(const glm::vec3& translation, const glm::quat& rotation);

    qint32 _type;             // type of Collision (will determine what is supposed to be in _data and _flags)
    void* _data;              // pointer to user supplied data
    quint32 _flags;           // 32 bits for whatever

    float _damping;           // range [0,1] of friction coeficient
    float _elasticity;        // range [0,1] of energy conservation
    glm::vec3 _contactPoint;  // world-frame point on BodyA that is deepest into BodyB
    glm::vec3 _penetration;   // depth that BodyA penetrates into BodyB
    glm::vec3 _addedVelocity; // velocity of BodyB
};

// CollisionList is intended to be a recycled container.  Fill the CollisionInfo's,
// use them, and then clear them for the next frame or context.

class CollisionList {
public:
    CollisionList(int maxSize);

    /// \return pointer to next collision. NULL if list is full.
    CollisionInfo* getNewCollision();

    /// \return pointer to collision by index.  NULL if index out of bounds.
    CollisionInfo* getCollision(int index);

    /// \return number of valid collisions
    int size() const { return _size; }

    /// Clear valid collisions.
    void clear();

private:
    int _maxSize;
    int _size;
    QVector<CollisionInfo> _collisions;
};

#endif /* defined(__hifi__CollisionInfo__) */
