//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#pragma once

#include "plugins/Plugin.h"

#include <QSize>
#include <QPoint>
#include <functional>

#include "gpu/GPUConfig.h"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <RegisteredMetaTypes.h>

enum Eye {
    Left,
    Right,
    Mono
};

/*
 * Helper method to iterate over each eye
 */
template <typename F>
void for_each_eye(F f) {
    f(Left);
    f(Right);
}

/*
 * Helper method to iterate over each eye, with an additional lambda to take action between the eyes
 */
template <typename F, typename FF>
void for_each_eye(F f, FF ff) {
    f(Eye::Left);
    ff();
    f(Eye::Right);
}

class QWindow;

class DisplayPlugin : public Plugin {
    Q_OBJECT
public:
    virtual bool isHmd() const { return false; }
    /// By default, all HMDs are stereo
    virtual bool isStereo() const { return isHmd(); }
    virtual bool isThrottled() const { return false; }

    // Rendering support

    /**
     *  Called by the application before the frame rendering.  Can be used for
     *  render timing related calls (for instance, the Oculus begin frame timing
     *  call)
     */
    virtual void preRender() = 0;
    /**
     *  Called by the application immediately before calling the display function.
     *  For OpenGL based plugins, this is the best place to put activate the output
     *  OpenGL context
     */
    virtual void preDisplay() = 0;
    /**
     *  Sends the scene texture and the overlay texture to the display plugin.
     *  The plugin is responsible for compositing these and adding rendering of
     *  additional elements like mouse and hydra pointers as required
     */
    virtual void display(GLuint sceneTexture, const glm::uvec2& sceneSize,
                         GLuint overlayTexture, const glm::uvec2& overlaySize) = 0;
    /**
     *  Called by the application immeidately after display.  For OpenGL based
     *  displays, this is the best place to put the buffer swap
     */
    virtual void finishFrame() = 0;

    // Does the rendering surface have current focus?
    virtual bool hasFocus() const = 0;
    // The size of the rendering surface
    virtual QSize getDeviceSize() const = 0;
    // The size of the rendering target (may be larger than the device size due to distortion)
    virtual QSize getRecommendedFramebufferSize() const { return getDeviceSize(); }

    // The size of the window (differs from the framebuffers size in instances like Retina macs)
    virtual glm::ivec2 getCanvasSize() const = 0;

    // The window for the surface, used for event interception.  May be null.
    virtual QWindow* getWindow() const = 0;

    virtual void installEventFilter(QObject* filter) {}
    virtual void removeEventFilter(QObject* filter) {}

    // The mouse position relative to the window (or proxy window) surface
    virtual glm::ivec2 getTrueMousePosition() const = 0;

    // The mouse position relative to the UI elements
    virtual glm::ivec2 getUiMousePosition() const {
        return trueMouseToUiMouse(getTrueMousePosition());
    }

    virtual std::function<QPointF(const QPointF&)> getMouseTranslator() { return [](const QPointF& p) { return p; }; };

    // Convert from screen mouse coordinates to UI mouse coordinates
    virtual glm::ivec2 trueMouseToUiMouse(const glm::ivec2 & position) const { return position; };

    virtual PickRay computePickRay(const glm::vec2 & pos) const {
        // FIXME make pure virtual
        return PickRay();
    };
    virtual bool isMouseOnScreen() const;


    // Stereo specific methods
    virtual glm::mat4 getProjection(Eye eye, const glm::mat4& baseProjection) const {
        return baseProjection;
    }

    virtual glm::mat4 getModelview(Eye eye, const glm::mat4& baseModelview) const {
        return glm::inverse(getEyePose(eye)) * baseModelview;
    }

    // HMD specific methods
    // TODO move these into another class
    virtual glm::mat4 getEyePose(Eye eye) const {
        static const glm::mat4 pose; return pose;
    }

    virtual glm::mat4 getHeadPose() const {
        static const glm::mat4 pose; return pose;
    }

    virtual void abandonCalibration() {}
    virtual void resetSensors() {}
    virtual float devicePixelRatio() { return 1.0;  }
    
signals:
    void recommendedFramebufferSizeChanged(const QSize & size);
    void requestRender();
};

