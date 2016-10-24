//
//  MyCharacterController.h
//  interface/src/avatar
//
//  Created by AndrewMeadows 2015.10.21
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "MyCharacterController.h"

#include <BulletUtil.h>

#include "MyAvatar.h"

// DONE TODO: improve walking up steps
// DONE TODO: make avatars able to walk up and down steps/slopes
// TODO: make avatars stand on steep slope
// TODO: make avatars not snag on low ceilings


void MyCharacterController::RayShotgunResult::reset() {
    //hitNormal = glm::vec3(0.0f);
    hitFraction = 1.0f;
    //numHits = 0;
    walkable = true;
}

MyCharacterController::MyCharacterController(MyAvatar* avatar) {

    assert(avatar);
    _avatar = avatar;
    updateShapeIfNecessary();
}

MyCharacterController::~MyCharacterController() {
}

void MyCharacterController::setDynamicsWorld(btDynamicsWorld* world) {
    CharacterController::setDynamicsWorld(world);
    if (world) {
        initRayShotgun(world);
    }
}

void MyCharacterController::updateShapeIfNecessary() {
    if (_pendingFlags & PENDING_FLAG_UPDATE_SHAPE) {
        _pendingFlags &= ~PENDING_FLAG_UPDATE_SHAPE;

        if (_radius > 0.0f) {
            // create RigidBody if it doesn't exist
            if (!_rigidBody) {
                btCollisionShape* shape = computeShape();

                // HACK: use some simple mass property defaults for now
                const btScalar DEFAULT_AVATAR_MASS = 100.0f;
                const btVector3 DEFAULT_AVATAR_INERTIA_TENSOR(30.0f, 8.0f, 30.0f);

                _rigidBody = new btRigidBody(DEFAULT_AVATAR_MASS, nullptr, shape, DEFAULT_AVATAR_INERTIA_TENSOR);
            } else {
                btCollisionShape* shape = _rigidBody->getCollisionShape();
                if (shape) {
                    delete shape;
                }
                shape = computeShape();
                _rigidBody->setCollisionShape(shape);
            }

            _rigidBody->setSleepingThresholds(0.0f, 0.0f);
            _rigidBody->setAngularFactor(0.0f);
            _rigidBody->setWorldTransform(btTransform(glmToBullet(_avatar->getOrientation()),
                                                      glmToBullet(_avatar->getPosition())));
            _rigidBody->setDamping(0.0f, 0.0f);
            if (_state == State::Hover) {
                _rigidBody->setGravity(btVector3(0.0f, 0.0f, 0.0f));
            } else {
                _rigidBody->setGravity(_gravity * _currentUp);
            }
            // KINEMATIC_CONTROLLER_HACK
            if (_moveKinematically) {
                _rigidBody->setCollisionFlags(btCollisionObject::CF_KINEMATIC_OBJECT);
            } else {
                _rigidBody->setCollisionFlags(_rigidBody->getCollisionFlags() &
                        ~(btCollisionObject::CF_KINEMATIC_OBJECT | btCollisionObject::CF_STATIC_OBJECT));
            }
        } else {
            // TODO: handle this failure case
        }
    }
}

bool MyCharacterController::testRayShotgun(const glm::vec3& position, const glm::vec3& step, RayShotgunResult& result) {
    btVector3 rayDirection = glmToBullet(step);
    btScalar stepLength = rayDirection.length();
    if (stepLength < FLT_EPSILON) {
        return false;
    }
    rayDirection /= stepLength;

    // get _ghost ready for ray traces
    btTransform transform = _rigidBody->getWorldTransform();
    btVector3 newPosition = glmToBullet(position);
    transform.setOrigin(newPosition);
    _ghost.setWorldTransform(transform);
    btMatrix3x3 rotation = transform.getBasis();
    _ghost.refreshOverlappingPairCache();

    CharacterRayResult rayResult(&_ghost);
    CharacterRayResult closestRayResult(&_ghost);
    btVector3 rayStart;
    btVector3 rayEnd;

    // compute rotation that will orient local ray start points to face step direction
    btVector3 forward = rotation * btVector3(0.0f, 0.0f, -1.0f);
    btVector3 adjustedDirection = rayDirection - rayDirection.dot(_currentUp) * _currentUp;
    btVector3 axis = forward.cross(adjustedDirection);
    btScalar lengthAxis = axis.length();
    if (lengthAxis > FLT_EPSILON) {
        // we're walking sideways
        btScalar angle = acosf(lengthAxis / adjustedDirection.length());
        if (rayDirection.dot(forward) < 0.0f) {
            angle = PI - angle;
        }
        axis /= lengthAxis;
        rotation = btMatrix3x3(btQuaternion(axis, angle)) * rotation;
    } else if (rayDirection.dot(forward) < 0.0f) {
        // we're walking backwards
        rotation = btMatrix3x3(btQuaternion(_currentUp, PI)) * rotation;
    }

    // scan the top
    // NOTE: if we scan an extra distance forward we can detect flat surfaces that are too steep to walk on.
    // The approximate extra distance can be derived with trigonometry.
    //
    //   minimumForward = [ (maxStepHeight + radius / cosTheta - radius) * (cosTheta / sinTheta) - radius ]
    //
    // where: theta = max angle between floor normal and vertical
    //
    // if stepLength is not long enough we can add the difference.
    //
    btScalar cosTheta = _minFloorNormalDotUp;
    btScalar sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const btScalar MIN_FORWARD_SLOP = 0.12f; // HACK: not sure why this is necessary to detect steepest walkable slope
    btScalar forwardSlop = (_maxStepHeight + _radius / cosTheta - _radius) * (cosTheta / sinTheta) - (_radius + stepLength) + MIN_FORWARD_SLOP;
    if (forwardSlop < 0.0f) {
        // BIG step, no slop necessary
        forwardSlop = 0.0f;
    }

    const btScalar backSlop = 0.04f;
    for (int32_t i = 0; i < _topPoints.size(); ++i) {
        rayStart = newPosition + rotation * _topPoints[i] - backSlop * rayDirection;
        rayEnd = rayStart + (backSlop + stepLength + forwardSlop) * rayDirection;
        if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
            if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                closestRayResult = rayResult;
            }
            if (result.walkable) {
                if (rayResult.m_hitNormalWorld.dot(_currentUp) < _minFloorNormalDotUp) {
                    result.walkable = false;
                    break;
                }
            }
        }
    }
    if (!result.walkable) {
        // the top scan wasn't walkable so don't bother scanning the bottom
        // remove both forwardSlop and backSlop
        result.hitFraction = glm::min(1.0f, (closestRayResult.m_closestHitFraction * (backSlop + stepLength + forwardSlop) - backSlop) / stepLength);
    } else {
        // remove forwardSlop
        result.hitFraction = (closestRayResult.m_closestHitFraction * (backSlop + stepLength + forwardSlop)) / (backSlop + stepLength);

        // scan the bottom
        for (int32_t i = 0; i < _bottomPoints.size(); ++i) {
            rayStart = newPosition + rotation * _bottomPoints[i] - backSlop * rayDirection;
            rayEnd = rayStart + (backSlop + stepLength) * rayDirection;
            if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
                if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                    closestRayResult = rayResult;
                }
            }
        }
        // remove backSlop
        // NOTE: backSlop removal can produce a NEGATIVE hitFraction!
        // which means the shape is actually in interpenetration
        result.hitFraction = ((closestRayResult.m_closestHitFraction * (backSlop + stepLength)) - backSlop) / stepLength;
    }
    return result.hitFraction < 1.0f;
}

glm::vec3 MyCharacterController::computeHMDStep(const glm::vec3& position, const glm::vec3& step) {
    btVector3 stepDirection = glmToBullet(step);
    btScalar stepLength = stepDirection.length();
    if (stepLength < FLT_EPSILON) {
        return glm::vec3(0.0f);
    }
    stepDirection /= stepLength;

    // get _ghost ready for ray traces
    btTransform transform = _rigidBody->getWorldTransform();
    btVector3 newPosition = glmToBullet(position);
    transform.setOrigin(newPosition);
    btMatrix3x3 rotation = transform.getBasis();
    _ghost.setWorldTransform(transform);
    _ghost.refreshOverlappingPairCache();

    // compute rotation that will orient local ray start points to face stepDirection
    btVector3 forward = rotation * btVector3(0.0f, 0.0f, -1.0f);
    btVector3 horizontalDirection = stepDirection - stepDirection.dot(_currentUp) * _currentUp;
    btVector3 axis = forward.cross(horizontalDirection);
    btScalar lengthAxis = axis.length();
    if (lengthAxis > FLT_EPSILON) {
        // non-zero sideways component
        btScalar angle = acosf(lengthAxis / horizontalDirection.length());
        if (stepDirection.dot(forward) < 0.0f) {
            angle = PI - angle;
        }
        axis /= lengthAxis;
        rotation = btMatrix3x3(btQuaternion(axis, angle)) * rotation;
    } else if (stepDirection.dot(forward) < 0.0f) {
        // backwards
        rotation = btMatrix3x3(btQuaternion(_currentUp, PI)) * rotation;
    }

    // scan the top
    // NOTE: if we scan an extra distance forward we can detect flat surfaces that are too steep to walk on.
    // The approximate extra distance can be derived with trigonometry.
    //
    //   minimumForward = [ (maxStepHeight + radius / cosTheta - radius) * (cosTheta / sinTheta) - radius ]
    //
    // where: theta = max angle between floor normal and vertical
    //
    // if stepLength is not long enough we can add the difference.
    //
    btScalar cosTheta = _minFloorNormalDotUp;
    btScalar sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const btScalar MIN_FORWARD_SLOP = 0.12f; // HACK: not sure why this is necessary to detect steepest walkable slope
    btScalar forwardSlop = (_maxStepHeight + _radius / cosTheta - _radius) * (cosTheta / sinTheta) - (_radius + stepLength) + MIN_FORWARD_SLOP;
    if (forwardSlop < 0.0f) {
        // BIG step, no slop necessary
        forwardSlop = 0.0f;
    }

    // we push the step forward by stepMargin to help reduce accidental overlap
    btScalar stepMargin = 0.04f;
    btScalar expandedStepLength = stepLength + forwardSlop + stepMargin;

    // loop
    CharacterRayResult rayResult(&_ghost);
    CharacterRayResult closestRayResult(&_ghost);
    btVector3 rayStart;
    btVector3 rayEnd;
    btVector3 overlap = btVector3(0.0f, 0.0f, 0.0f);
    int32_t numOverlaps = 0;
    bool walkable = true;
    for (int32_t i = 0; i < _topPoints.size(); ++i) {
        rayStart = newPosition + rotation * _topPoints[i];
        rayEnd = rayStart + expandedStepLength * stepDirection;
        rayResult.m_closestHitFraction = 1.0f; // reset rayResult for next test
        if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
            // track closest hit
            if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                closestRayResult = rayResult;
            }
            // check if walkable
            if (walkable) {
                if (rayResult.m_hitNormalWorld.dot(_currentUp) < _minFloorNormalDotUp) {
                    walkable = false;
                }
            }
            // sum any overlap
            btScalar distanceToPlane = -rayResult.m_closestHitFraction * stepLength * stepDirection.dot(rayResult.m_hitNormalWorld);
            if (distanceToPlane < stepMargin) {
                overlap += (stepMargin - distanceToPlane) * rayResult.m_hitNormalWorld;
                ++numOverlaps;
            }
        }
    }
    // remove expansion from the closestHitFraction
    btScalar closestHitFraction = glm::min(1.0f, (closestRayResult.m_closestHitFraction * expandedStepLength - stepMargin) / stepLength);

    /*
    // scan the bottom
    closestRayResult.m_closestHitFraction = 1.0f; // reset closestRayResult for next barrage
    btScalar stepHeight = _minStepHeight;
    for (int32_t i = 0; i < _bottomPoints.size(); ++i) {
        rayStart = newPosition + rotation * _bottomPoints[i];
        rayEnd = rayStart + (stepLength + stepMargin) * stepDirection;
        rayResult.m_closestHitFraction = 1.0f; // reset rayResult for next test
        if (_ghost.rayTest(rayStart, rayEnd, rayResult)) {
            // track closest hit
            if (rayResult.m_closestHitFraction < closestRayResult.m_closestHitFraction) {
                closestRayResult = rayResult;
            }
            // sum any overlap
            btScalar distanceToPlane = -rayResult.m_closestHitFraction * stepDirection * stepDirection.dot(rayResult.m_hitNormalWorld);
            if (distanceToPlane < stepMargin) {
                overlap += (stepMargin - distanceToPlane) * rayResult.m_hitNormalWorld;
                ++numOverlaps;
            }
        }
    }
    btScalar adjustedHitFraction = (closestRayResult.m_closestHitFraction * (stepLength - stepMargin) - stepMargin) / stepLength;
    if (adjustedHitFraction < closestHitFraction) {
        closestHitFraction = adjustedHitFraction;
    }
    */

    // compute the final step
    btVector3 finalStep = (closestHitFraction * stepLength) * stepDirection;
    if (numOverlaps > 0) {
        // we have two independent measures of displacement: finalStep and overlap
        if (numOverlaps > 1) {
            overlap /= (btScalar)numOverlaps;
        }

        // reconcile distinct displacements as follows:
        // add components that point in opposite directions, otherwise take the component with largest absolute value
        btVector3 product = finalStep;
        product *= overlap; // component-wise multiplication --> negative components point in opposite directions
        if (product.getX() < 0.0f) {
            finalStep.setX(finalStep.getX() + overlap.getX());
        } else if (fabsf(overlap.getX()) > fabsf(finalStep.getX())) {
            finalStep.setX(overlap.getX());
        }
        if (product.getY() < 0.0f) {
            finalStep.setY(finalStep.getY() + overlap.getY());
        } else if (fabsf(overlap.getY()) > fabsf(finalStep.getY())) {
            finalStep.setY(overlap.getY());
        }
        if (product.getZ() < 0.0f) {
            finalStep.setZ(finalStep.getZ() + overlap.getZ());
        } else if (fabsf(overlap.getZ()) > fabsf(finalStep.getZ())) {
            finalStep.setZ(overlap.getZ());
        }
    }

    return bulletToGLM(finalStep);
} // foo

btConvexHullShape* MyCharacterController::computeShape() const {
    // HACK: the avatar collides using convex hull with a collision margin equal to
    // the old capsule radius.  Two points define a capsule and additional points are
    // spread out at chest level to produce a slight taper toward the feet.  This
    // makes the avatar more likely to collide with vertical walls at a higher point
    // and thus less likely to produce a single-point collision manifold below the
    // _maxStepHeight when walking into against vertical surfaces --> fixes a bug
    // where the "walk up steps" feature would allow the avatar to walk up vertical
    // walls.
    const int32_t NUM_POINTS = 6;
    btVector3 points[NUM_POINTS];
    btVector3 xAxis = btVector3(1.0f, 0.0f, 0.0f);
    btVector3 yAxis = btVector3(0.0f, 1.0f, 0.0f);
    btVector3 zAxis = btVector3(0.0f, 0.0f, 1.0f);
    points[0] = _halfHeight * yAxis;
    points[1] = -_halfHeight * yAxis;
    points[2] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * zAxis;
    points[3] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * zAxis;
    points[4] = (0.75f * _halfHeight) * yAxis - (0.1f * _radius) * xAxis;
    points[5] = (0.75f * _halfHeight) * yAxis + (0.1f * _radius) * xAxis;
    btConvexHullShape* shape = new btConvexHullShape(reinterpret_cast<btScalar*>(points), NUM_POINTS);
    shape->setMargin(_radius);
    return shape;
}

void MyCharacterController::initRayShotgun(const btCollisionWorld* world) {
    // In order to trace rays out from the avatar's shape surface we need to know where the start points are in
    // the local-frame.  Since the avatar shape is somewhat irregular computing these points by hand is a hassle
    // so instead we ray-trace backwards to the avatar to find them.
    //
    // We trace back a regular grid (see below) of points against the shape and keep any that hit.
    //        ___
    //     + / + \ +
    //      |+   +|
    //     +|  +  | +
    //      |+   +|
    //     +|  +  | +
    //      |+   +|
    //     + \ + /  +
    //        ---
    // The shotgun will send rays out from these same points to see if the avatar's shape can proceed through space.

    // helper class for simple ray-traces against character
    class MeOnlyResultCallback : public btCollisionWorld::ClosestRayResultCallback {
    public:
        MeOnlyResultCallback (btRigidBody* me) : btCollisionWorld::ClosestRayResultCallback(btVector3(0.0f, 0.0f, 0.0f), btVector3(0.0f, 0.0f, 0.0f)) {
            _me = me;
            m_collisionFilterGroup = BULLET_COLLISION_GROUP_DYNAMIC;
            m_collisionFilterMask = BULLET_COLLISION_MASK_DYNAMIC;
        }
        virtual btScalar addSingleResult(btCollisionWorld::LocalRayResult& rayResult,bool normalInWorldSpace) override {
            if (rayResult.m_collisionObject != _me) {
                return 1.0f;
            }
            return ClosestRayResultCallback::addSingleResult(rayResult, normalInWorldSpace);
        }
        btRigidBody* _me;
    };

    const btScalar fullHalfHeight = _radius + _halfHeight;
    const btScalar divisionLine = -fullHalfHeight + _maxStepHeight; // line between top and bottom
    const btScalar topHeight = fullHalfHeight - divisionLine;
    const btScalar slop = 0.02f;

    const int32_t numRows = 3; // must be odd number > 1
    const int32_t numColumns = 3; // must be odd number > 1
    btVector3 reach = (2.0f * _radius) * btVector3(0.0f, 0.0f, 1.0f);

    { // top points
        _topPoints.clear();
        _topPoints.reserve(numRows * numColumns);
        btScalar stepY = (topHeight - slop) / (btScalar)(numRows - 1);
        btScalar stepX = 2.0f * (_radius - slop) / (btScalar)(numColumns - 1);

        btTransform transform = _rigidBody->getWorldTransform();
        btVector3 position = transform.getOrigin();
        btMatrix3x3 rotation = transform.getBasis();

        for (int32_t i = 0; i < numRows; ++i) {
            int32_t maxJ = numColumns;
            btScalar offsetX = -(btScalar)((numColumns - 1) / 2) * stepX;
            if (i%2 == 1) {
                // odd rows have one less point and start a halfStep closer
                maxJ -= 1;
                offsetX += 0.5f * stepX;
            }
            for (int32_t j = 0; j < maxJ; ++j) {
                btVector3 localRayEnd(offsetX + (btScalar)(j) * stepX, divisionLine + (btScalar)(i) * stepY, 0.0f);
                btVector3 localRayStart = localRayEnd - reach;
                MeOnlyResultCallback result(_rigidBody);
                world->rayTest(position + rotation * localRayStart, position + rotation * localRayEnd, result);
                if (result.m_closestHitFraction < 1.0f) {
                    _topPoints.push_back(localRayStart + result.m_closestHitFraction * reach);
                }
            }
        }
    }

    { // bottom points
        _bottomPoints.clear();
        _bottomPoints.reserve(numRows * numColumns);

        btScalar stepY = (_maxStepHeight - 2.0f * slop) / (btScalar)(numRows - 1);
        btScalar stepX = 2.0f * (_radius - slop) / (btScalar)(numColumns - 1);

        btTransform transform = _rigidBody->getWorldTransform();
        btVector3 position = transform.getOrigin();
        btMatrix3x3 rotation = transform.getBasis();

        for (int32_t i = 0; i < numRows; ++i) {
            int32_t maxJ = numColumns;
            btScalar offsetX = -(btScalar)((numColumns - 1) / 2) * stepX;
            if (i%2 == 1) {
                // odd rows have one less point and start a halfStep closer
                maxJ -= 1;
                offsetX += 0.5f * stepX;
            }
            for (int32_t j = 0; j < maxJ; ++j) {
                btVector3 localRayEnd(offsetX + (btScalar)(j) * stepX, (divisionLine - slop) - (btScalar)(i) * stepY, 0.0f);
                btVector3 localRayStart = localRayEnd - reach;
                MeOnlyResultCallback result(_rigidBody);
                world->rayTest(position + rotation * localRayStart, position + rotation * localRayEnd, result);
                if (result.m_closestHitFraction < 1.0f) {
                    _bottomPoints.push_back(localRayStart + result.m_closestHitFraction * reach);
                }
            }
        }
    }
}
