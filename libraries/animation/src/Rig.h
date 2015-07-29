//
//  Rig.h
//  libraries/animation/src/
//
//  Produces animation data and hip placement for the current timestamp.
//
//  Created by Howard Stearns, Seth Alves, Anthony Thibault, Andrew Meadows on 7/15/15.
//  Copyright (c) 2015 High Fidelity, Inc. All rights reserved.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
/*
 Things we want to be able to do, that I think we cannot do now:
 * Stop an animation at a given time so that it can be examined visually or in a test harness. (I think we can already stop animation and set frame to a computed float? But does that move the bones?)
 * Play two animations, blending between them. (Current structure just has one, under script control.)
 * Fade in an animation over another.
 * Apply IK, lean, head pointing or other overrides relative to previous position.
 All of this depends on coordinated state.

 TBD:
 - What are responsibilities of Animation/AnimationPointer/AnimationCache/AnimationDetails/AnimationObject/AnimationLoop?
    Is there common/copied code (e.g., ScriptableAvatar::update)?
 - How do attachments interact with the physics of the attached entity? E.g., do hand joints need to reflect held object
    physics?
 - Is there any current need (i.e., for initial campatability) to have multiple animations per role (e.g., idle) with the
    system choosing randomly?

 - Distribute some doc from here to the right files if it turns out to be correct:
    - AnimationDetails is a script-useable copy of animation state, analogous to EntityItemProperties, but without anything
       equivalent to editEntity.
      But what's the intended difference vs AnimationObjection? Maybe AnimationDetails is to Animation as AnimationObject
       is to AnimationPointer?
 */

#ifndef __hifi__Rig__
#define __hifi__Rig__

#include <QObject>

#include "JointState.h"  // We might want to change this (later) to something that doesn't depend on gpu, fbx and model. -HRS

class AnimationHandle;
typedef std::shared_ptr<AnimationHandle> AnimationHandlePointer;
// typedef QWeakPointer<AnimationHandle> WeakAnimationHandlePointer;

class Rig;
typedef std::shared_ptr<Rig> RigPointer;


class Rig : public QObject, public std::enable_shared_from_this<Rig> {

public:

    virtual ~Rig() {}

    RigPointer getRigPointer() { return shared_from_this(); }

    AnimationHandlePointer createAnimationHandle();
    void removeAnimationHandle(const AnimationHandlePointer& handle);
    bool removeRunningAnimation(AnimationHandlePointer animationHandle);
    void addRunningAnimation(AnimationHandlePointer animationHandle);
    bool isRunningAnimation(AnimationHandlePointer animationHandle);
    const QList<AnimationHandlePointer>& getRunningAnimations() const { return _runningAnimations; }
    void deleteAnimations();
    const QList<AnimationHandlePointer>& getAnimationHandles() const { return _animationHandles; }
    void startAnimation(const QString& url, float fps = 30.0f, float priority = 1.0f, bool loop = false,
                        bool hold = false, float firstFrame = 0.0f, float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    void stopAnimation(const QString& url);
    void startAnimationByRole(const QString& role, const QString& url = QString(), float fps = 30.0f,
                              float priority = 1.0f, bool loop = false, bool hold = false, float firstFrame = 0.0f,
                              float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList());
    void stopAnimationByRole(const QString& role);
    void addAnimationByRole(const QString& role, const QString& url = QString(), float fps = 30.0f,
                            float priority = 1.0f, bool loop = false, bool hold = false, float firstFrame = 0.0f,
                            float lastFrame = FLT_MAX, const QStringList& maskedJoints = QStringList(), bool startAutomatically = false);

    float initJointStates(QVector<JointState> states, glm::mat4 parentTransform);
    bool jointStatesEmpty() { return _jointStates.isEmpty(); };
    int getJointStateCount() const { return _jointStates.size(); }
    int indexOfJoint(const QString& jointName) ;

    void initJointTransforms(glm::mat4 parentTransform);
    void clearJointTransformTranslation(int jointIndex);
    void reset(const QVector<FBXJoint>& fbxJoints);
    bool getJointStateRotation(int index, glm::quat& rotation) const;
    void applyJointRotationDelta(int jointIndex, const glm::quat& delta, bool constrain, float priority);
    JointState getJointState(int jointIndex) const; // XXX
    bool getVisibleJointState(int index, glm::quat& rotation) const;
    void clearJointState(int index);
    void clearJointStates();
    void clearJointAnimationPriority(int index);
    float getJointAnimatinoPriority(int index);
    void setJointAnimatinoPriority(int index, float newPriority);
    void setJointState(int index, bool valid, const glm::quat& rotation, float priority);
    void restoreJointRotation(int index, float fraction, float priority);
    bool getJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                      glm::vec3 translation, glm::quat rotation) const;

    bool getJointPosition(int jointIndex, glm::vec3& position) const;
    bool getJointRotationInWorldFrame(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getJointRotation(int jointIndex, glm::quat& rotation) const;
    bool getJointCombinedRotation(int jointIndex, glm::quat& result, const glm::quat& rotation) const;
    bool getVisibleJointPositionInWorldFrame(int jointIndex, glm::vec3& position,
                                             glm::vec3 translation, glm::quat rotation) const;
    bool getVisibleJointRotationInWorldFrame(int jointIndex, glm::quat& result, glm::quat rotation) const;
    glm::mat4 getJointTransform(int jointIndex) const;
    void setJointTransform(int jointIndex, glm::mat4 newTransform);
    glm::mat4 getJointVisibleTransform(int jointIndex) const;
    void setJointVisibleTransform(int jointIndex, glm::mat4 newTransform);
    // Start or stop animations as needed.
    void computeMotionAnimationState(float deltaTime, const glm::vec3& worldPosition, const glm::vec3& worldVelocity, const glm::quat& worldRotation);
    // Regardless of who started the animations or how many, update the joints.
    void updateAnimations(float deltaTime, glm::mat4 parentTransform);
    bool setJointPosition(int jointIndex, const glm::vec3& position, const glm::quat& rotation, bool useRotation,
                          int lastFreeIndex, bool allIntermediatesFree, const glm::vec3& alignment, float priority,
                          const QVector<int>& freeLineage, glm::mat4 parentTransform);
    void inverseKinematics(int endIndex, glm::vec3 targetPosition, const glm::quat& targetRotation, float priority,
                           const QVector<int>& freeLineage, glm::mat4 parentTransform);
    bool restoreJointPosition(int jointIndex, float fraction, float priority, const QVector<int>& freeLineage);
    float getLimbLength(int jointIndex, const QVector<int>& freeLineage,
                        const glm::vec3 scale, const QVector<FBXJoint>& fbxJoints) const;

    glm::quat setJointRotationInBindFrame(int jointIndex, const glm::quat& rotation, float priority, bool constrain = false);
    glm::vec3 getJointDefaultTranslationInConstrainedFrame(int jointIndex);
    glm::quat setJointRotationInConstrainedFrame(int jointIndex, glm::quat targetRotation,
                                                 float priority, bool constrain = false);
    glm::quat getJointDefaultRotationInParentFrame(int jointIndex);
    void updateVisibleJointStates();

    virtual void updateJointState(int index, glm::mat4 parentTransform) = 0;
    virtual void updateFaceJointState(int index, glm::mat4 parentTransform) = 0;

    void setEnableRig(bool isEnabled) { _enableRig = isEnabled; }

 protected:
    QVector<JointState> _jointStates;

    QList<AnimationHandlePointer> _animationHandles;
    QList<AnimationHandlePointer> _runningAnimations;

    bool _enableRig;
    bool _isWalking;
    bool _isTurning;
    bool _isIdle;
    glm::vec3 _lastFront;
    glm::vec3 _lastPosition;
 };

#endif /* defined(__hifi__Rig__) */
