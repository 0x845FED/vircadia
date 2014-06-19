//
//  Ragdoll.h
//  libraries/shared/src
//
//  Created by Andrew Meadows 2014.05.30
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_Ragdoll_h
#define hifi_Ragdoll_h

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include <QVector>

class Shape;

class VerletPoint {
public:
    VerletPoint() : _position(0.0f), _lastPosition(0.0f), _mass(0.0f) {}
    glm::vec3 _position;
    glm::vec3 _lastPosition;
    float _mass;
};

class Constraint {
public:
    Constraint() {}
    virtual ~Constraint() {}

    /// Enforce contraint by moving relevant points.
    /// \return max distance of point movement
    virtual float enforce() = 0;

protected:
    int _type;
};

class FixedConstraint : public Constraint {
public:
    FixedConstraint(glm::vec3* point, const glm::vec3& anchor);
    float enforce();
    void setPoint(glm::vec3* point);
    void setAnchor(const glm::vec3& anchor);
private:
    glm::vec3* _point;
    glm::vec3 _anchor;
};

class DistanceConstraint : public Constraint {
public:
    DistanceConstraint(VerletPoint* startPoint, VerletPoint* endPoint);
    DistanceConstraint(const DistanceConstraint& other);
    float enforce();
    void setDistance(float distance);
private:
    float _distance;
    VerletPoint* _points[2];
};

class Ragdoll {
public:

    Ragdoll();
    virtual ~Ragdoll();

    virtual void stepRagdollForward(float deltaTime) = 0;

    /// \return max distance of point movement
    float enforceRagdollConstraints();

    // both const and non-const getPoints()
    const QVector<VerletPoint>& getRagdollPoints() const { return _ragdollPoints; }
    QVector<VerletPoint>& getRagdollPoints() { return _ragdollPoints; }

protected:
    void clearRagdollConstraintsAndPoints();
    virtual void initRagdollPoints() = 0;
    virtual void buildRagdollConstraints() = 0;

    QVector<VerletPoint> _ragdollPoints;
    QVector<Constraint*> _ragdollConstraints;
};

#endif // hifi_Ragdoll_h
