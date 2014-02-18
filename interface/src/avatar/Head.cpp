//
//  Head.cpp
//  interface
//
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <glm/gtx/quaternion.hpp>

#include <NodeList.h>

#include "Application.h"
#include "Avatar.h"
#include "Head.h"
#include "Menu.h"
#include "Util.h"

using namespace std;

Head::Head(Avatar* owningAvatar) :
    HeadData((AvatarData*)owningAvatar),
    _returnHeadToCenter(false),
    _position(0.0f, 0.0f, 0.0f),
    _rotation(0.0f, 0.0f, 0.0f),
    _leftEyePosition(0.0f, 0.0f, 0.0f),
    _rightEyePosition(0.0f, 0.0f, 0.0f),
    _eyePosition(0.0f, 0.0f, 0.0f),
    _scale(1.0f),
    _gravity(0.0f, -1.0f, 0.0f),
    _lastLoudness(0.0f),
    _audioAttack(0.0f),
    _bodyRotation(0.0f, 0.0f, 0.0f),
    _angularVelocity(0,0,0),
    _renderLookatVectors(false),
    _saccade(0.0f, 0.0f, 0.0f),
    _saccadeTarget(0.0f, 0.0f, 0.0f),
    _leftEyeBlinkVelocity(0.0f),
    _rightEyeBlinkVelocity(0.0f),
    _timeWithoutTalking(0.0f),
    _tweakedPitch(0.f),
    _tweakedYaw(0.f),
    _tweakedRoll(0.f),
    _isCameraMoving(false),
    _faceModel(this)
{
  
}

void Head::init() {
    _faceModel.init();
}

void Head::reset() {
    _yaw = _pitch = _roll = 0.0f;
    _leanForward = _leanSideways = 0.0f;
    _faceModel.reset();
}




void Head::simulate(float deltaTime, bool isMine) {
    
    //  Update audio trailing average for rendering facial animations
    Faceshift* faceshift = Application::getInstance()->getFaceshift();
    if (isMine) {
        _isFaceshiftConnected = faceshift->isActive();
    }
    
    if (isMine && faceshift->isActive()) {
        const float EYE_OPEN_SCALE = 0.5f;
        _leftEyeBlink = faceshift->getLeftBlink() - EYE_OPEN_SCALE * faceshift->getLeftEyeOpen();
        _rightEyeBlink = faceshift->getRightBlink() - EYE_OPEN_SCALE * faceshift->getRightEyeOpen();
        
        // set these values based on how they'll be used.  if we use faceshift in the long term, we'll want a complete
        // mapping between their blendshape coefficients and our avatar features
        const float MOUTH_SIZE_SCALE = 2500.0f;
        _averageLoudness = faceshift->getMouthSize() * faceshift->getMouthSize() * MOUTH_SIZE_SCALE;
        const float BROW_HEIGHT_SCALE = 0.005f;
        _browAudioLift = faceshift->getBrowUpCenter() * BROW_HEIGHT_SCALE;
        _blendshapeCoefficients = faceshift->getBlendshapeCoefficients();
        
    } else if (!_isFaceshiftConnected) {
        // Update eye saccades
        const float AVERAGE_MICROSACCADE_INTERVAL = 0.50f;
        const float AVERAGE_SACCADE_INTERVAL = 4.0f;
        const float MICROSACCADE_MAGNITUDE = 0.002f;
        const float SACCADE_MAGNITUDE = 0.04f;
        
        if (randFloat() < deltaTime / AVERAGE_MICROSACCADE_INTERVAL) {
            _saccadeTarget = MICROSACCADE_MAGNITUDE * randVector();
        } else if (randFloat() < deltaTime / AVERAGE_SACCADE_INTERVAL) {
            _saccadeTarget = SACCADE_MAGNITUDE * randVector();
        }
        _saccade += (_saccadeTarget - _saccade) * 0.50f;
    
        const float AUDIO_AVERAGING_SECS = 0.05f;
        _averageLoudness = (1.f - deltaTime / AUDIO_AVERAGING_SECS) * _averageLoudness +
                                 (deltaTime / AUDIO_AVERAGING_SECS) * _audioLoudness;
        
        //  Detect transition from talking to not; force blink after that and a delay
        bool forceBlink = false;
        const float TALKING_LOUDNESS = 100.0f;
        const float BLINK_AFTER_TALKING = 0.25f;
        if (_averageLoudness > TALKING_LOUDNESS) {
            _timeWithoutTalking = 0.0f;
        
        } else if (_timeWithoutTalking < BLINK_AFTER_TALKING && (_timeWithoutTalking += deltaTime) >= BLINK_AFTER_TALKING) {
            forceBlink = true;
        }
                                 
        //  Update audio attack data for facial animation (eyebrows and mouth)
        _audioAttack = 0.9f * _audioAttack + 0.1f * fabs(_audioLoudness - _lastLoudness);
        _lastLoudness = _audioLoudness;
        
        const float BROW_LIFT_THRESHOLD = 100.0f;
        if (_audioAttack > BROW_LIFT_THRESHOLD) {
            _browAudioLift += sqrtf(_audioAttack) * 0.00005f;
        }
        
        const float CLAMP = 0.01f;
        if (_browAudioLift > CLAMP) {
            _browAudioLift = CLAMP;
        }
        
        _browAudioLift *= 0.7f;      

        const float BLINK_SPEED = 10.0f;
        const float FULLY_OPEN = 0.0f;
        const float FULLY_CLOSED = 1.0f;
        if (_leftEyeBlinkVelocity == 0.0f && _rightEyeBlinkVelocity == 0.0f) {
            // no blinking when brows are raised; blink less with increasing loudness
            const float BASE_BLINK_RATE = 15.0f / 60.0f;
            const float ROOT_LOUDNESS_TO_BLINK_INTERVAL = 0.25f;
            if (forceBlink || (_browAudioLift < EPSILON && shouldDo(glm::max(1.0f, sqrt(_averageLoudness) *
                    ROOT_LOUDNESS_TO_BLINK_INTERVAL) / BASE_BLINK_RATE, deltaTime))) {
                _leftEyeBlinkVelocity = BLINK_SPEED;
                _rightEyeBlinkVelocity = BLINK_SPEED;
            }
        } else {
            _leftEyeBlink = glm::clamp(_leftEyeBlink + _leftEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
            _rightEyeBlink = glm::clamp(_rightEyeBlink + _rightEyeBlinkVelocity * deltaTime, FULLY_OPEN, FULLY_CLOSED);
            
            if (_leftEyeBlink == FULLY_CLOSED) {
                _leftEyeBlinkVelocity = -BLINK_SPEED;
            
            } else if (_leftEyeBlink == FULLY_OPEN) {
                _leftEyeBlinkVelocity = 0.0f;
            }
            if (_rightEyeBlink == FULLY_CLOSED) {
                _rightEyeBlinkVelocity = -BLINK_SPEED;
            
            } else if (_rightEyeBlink == FULLY_OPEN) {
                _rightEyeBlinkVelocity = 0.0f;
            }
        }
        
        // use data to update fake Faceshift blendshape coefficients
        const float BROW_LIFT_SCALE = 500.0f;
        const float JAW_OPEN_SCALE = 0.01f;
        const float JAW_OPEN_DEAD_ZONE = 0.75f;
        faceshift->updateFakeCoefficients(_leftEyeBlink, _rightEyeBlink, min(1.0f, _browAudioLift * BROW_LIFT_SCALE),
            glm::clamp(sqrt(_averageLoudness * JAW_OPEN_SCALE) - JAW_OPEN_DEAD_ZONE, 0.0f, 1.0f), _blendshapeCoefficients);
    }
    
    _faceModel.simulate(deltaTime);
    
    // the blend face may have custom eye meshes
    if (!_faceModel.getEyePositions(_leftEyePosition, _rightEyePosition)) {
        _leftEyePosition = _rightEyePosition = getPosition();
    }
    _eyePosition = calculateAverageEyePosition();
}

void Head::render(float alpha) {
    if (_faceModel.render(alpha) && _renderLookatVectors) {
        renderLookatVectors(_leftEyePosition, _rightEyePosition, _lookAtPosition);
    }
}

void Head::setScale (float scale) {
    if (_scale == scale) {
        return;
    }
    _scale = scale;
}

glm::quat Head::getOrientation() const {
    return glm::quat(glm::radians(_bodyRotation)) * glm::quat(glm::radians(glm::vec3(_pitch, _yaw, _roll)));
}

glm::quat Head::getTweakedOrientation() const {
    return glm::quat(glm::radians(_bodyRotation)) * glm::quat(glm::radians(glm::vec3(getTweakedPitch(), getTweakedYaw(), getTweakedRoll() )));
}

glm::quat Head::getCameraOrientation () const {
    Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
    return owningAvatar->getWorldAlignedOrientation()
            * glm::quat(glm::radians(glm::vec3(_pitch, 0.f, 0.0f)));
}

glm::quat Head::getEyeRotation(const glm::vec3& eyePosition) const {
    glm::quat orientation = getOrientation();
    return rotationBetween(orientation * IDENTITY_FRONT, _lookAtPosition + _saccade - eyePosition) * orientation;
}

glm::vec3 Head::getScalePivot() const {
    return _faceModel.isActive() ? _faceModel.getTranslation() : _position;
}

float Head::getTweakedYaw() const {
    return glm::clamp(_yaw + _tweakedYaw, MIN_HEAD_YAW, MAX_HEAD_YAW);
}

float Head::getTweakedPitch() const {
    return glm::clamp(_pitch + _tweakedPitch, MIN_HEAD_PITCH, MAX_HEAD_PITCH);
}

float Head::getTweakedRoll() const {
    return glm::clamp(_roll + _tweakedRoll, MIN_HEAD_ROLL, MAX_HEAD_ROLL);
}

void Head::applyCollision(CollisionInfo& collision) {
    // HACK: the collision proxies for the FaceModel are bad.  As a temporary workaround
    // we collide against a hard coded collision proxy.
    // TODO: get a better collision proxy here.
    const float HEAD_RADIUS = 0.15f;
    const glm::vec3 HEAD_CENTER = _position;

    // collide the contactPoint against the collision proxy to obtain a new penetration
    // NOTE: that penetration is in opposite direction (points the way out for the point, not the sphere)
    glm::vec3 penetration;
    if (findPointSpherePenetration(collision._contactPoint, HEAD_CENTER, HEAD_RADIUS, penetration)) {
        // compute lean angles
        Avatar* owningAvatar = static_cast<Avatar*>(_owningAvatar);
        glm::quat bodyRotation = owningAvatar->getOrientation();
        glm::vec3 neckPosition;
        if (owningAvatar->getSkeletonModel().getNeckPosition(neckPosition)) {
            glm::vec3 xAxis = bodyRotation * glm::vec3(1.f, 0.f, 0.f);
            glm::vec3 zAxis = bodyRotation * glm::vec3(0.f, 0.f, 1.f);
            float neckLength = glm::length(_position - neckPosition);
            if (neckLength > 0.f) {
                float forward = glm::dot(collision._penetration, zAxis) / neckLength;
                float sideways = - glm::dot(collision._penetration, xAxis) / neckLength;
                addLean(sideways, forward);
            }
        }
    } 
}

void Head::renderLookatVectors(glm::vec3 leftEyePosition, glm::vec3 rightEyePosition, glm::vec3 lookatPosition) {

    Application::getInstance()->getGlowEffect()->begin();
    
    glLineWidth(2.0);
    glBegin(GL_LINES);
    glColor4f(0.2f, 0.2f, 0.2f, 1.f);
    glVertex3f(leftEyePosition.x, leftEyePosition.y, leftEyePosition.z);
    glColor4f(1.0f, 1.0f, 1.0f, 0.f);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glColor4f(0.2f, 0.2f, 0.2f, 1.f);
    glVertex3f(rightEyePosition.x, rightEyePosition.y, rightEyePosition.z);
    glColor4f(1.0f, 1.0f, 1.0f, 0.f);
    glVertex3f(lookatPosition.x, lookatPosition.y, lookatPosition.z);
    glEnd();
    
    Application::getInstance()->getGlowEffect()->end();
}


