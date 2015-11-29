//
//  SpatiallyNestable.h
//  libraries/shared/src/
//
//  Created by Seth Alves on 2015-10-18
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_SpatiallyNestable_h
#define hifi_SpatiallyNestable_h

#include <QUuid>

#include "Transform.h"
#include "SpatialParentFinder.h"
#include "shared/ReadWriteLockable.h"


class SpatiallyNestable;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakConstPointer = std::weak_ptr<const SpatiallyNestable>;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableConstPointer = std::shared_ptr<const SpatiallyNestable>;

class NestableTypes {
public:
    using NestableType = enum NestableType_t {
        Entity,
        Avatar
    };
};

class SpatiallyNestable : public std::enable_shared_from_this<SpatiallyNestable> {
public:
    SpatiallyNestable(NestableTypes::NestableType nestableType, QUuid id);
    virtual ~SpatiallyNestable() { }

    virtual const QUuid& getID() const { return _id; }
    virtual void setID(const QUuid& id) { _id = id; }

    virtual const QUuid& getParentID() const { return _parentID; }
    virtual void setParentID(const QUuid& parentID);

    virtual quint16 getParentJointIndex() const { return _parentJointIndex; }
    virtual void setParentJointIndex(quint16 parentJointIndex) { _parentJointIndex = parentJointIndex; }

    glm::vec3 worldToLocal(const glm::vec3& position);
    glm::quat worldToLocal(const glm::quat& orientation);

    static glm::vec3 localToWorld(const glm::vec3& position, QUuid parentID, int parentJointIndex);
    static glm::quat localToWorld(const glm::quat& orientation, QUuid parentID, int parentJointIndex);

    // world frame
    virtual const Transform& getTransform() const;
    virtual void setTransform(const Transform& transform);

    virtual Transform getParentTransform() const;

    virtual const glm::vec3& getPosition() const;
    virtual void setPosition(const glm::vec3& position);

    virtual const glm::quat& getOrientation() const;
    virtual const glm::quat& getOrientation(int jointIndex) const;
    virtual void setOrientation(const glm::quat& orientation);

    virtual const glm::vec3& getScale() const;
    virtual void setScale(const glm::vec3& scale);

    // get world location of a specific joint
    virtual const Transform& getTransform(int jointIndex) const;
    virtual const glm::vec3& getPosition(int jointIndex) const;
    virtual const glm::vec3& getScale(int jointIndex) const;

    // object's parent's frame
    virtual const Transform& getLocalTransform() const;
    virtual void setLocalTransform(const Transform& transform);

    virtual const glm::vec3& getLocalPosition() const;
    virtual void setLocalPosition(const glm::vec3& position);

    virtual const glm::quat& getLocalOrientation() const;
    virtual void setLocalOrientation(const glm::quat& orientation);

    virtual const glm::vec3& getLocalScale() const;
    virtual void setLocalScale(const glm::vec3& scale);

    QList<SpatiallyNestablePointer> getChildren() const;
    NestableTypes::NestableType getNestableType() const { return _nestableType; }

    // this object's frame
    virtual const Transform& getJointTransformInObjectFrame(int jointIndex) const;
    virtual glm::quat getJointRotation(int index) const = 0;
    virtual glm::vec3 getJointTranslation(int index) const = 0;

protected:
    NestableTypes::NestableType _nestableType; // EntityItem or an AvatarData
    QUuid _id;
    QUuid _parentID; // what is this thing's transform relative to?
    quint16 _parentJointIndex; // which joint of the parent is this relative to?
    SpatiallyNestablePointer getParentPointer() const;
    mutable SpatiallyNestableWeakPointer _parent;

    virtual void beParentOfChild(SpatiallyNestablePointer newChild) const;
    virtual void forgetChild(SpatiallyNestablePointer newChild) const;

    mutable ReadWriteLockable _childrenLock;
    mutable QHash<QUuid, SpatiallyNestableWeakPointer> _children;

    virtual void parentChanged() {} // called when parent pointer is updated

private:
    Transform _transform; // this is to be combined with parent's world-transform to produce this' world-transform.

    // these are so we can return by reference
    mutable glm::vec3 _absolutePositionCache;
    mutable glm::quat _absoluteRotationCache;
    mutable Transform _worldTransformCache;
    mutable bool _parentKnowsMe = false;
    mutable QVector<Transform> _jointInObjectFrameCache;
    mutable QVector<Transform> _jointInWorldFrameCache;
};


#endif // hifi_SpatiallyNestable_h
