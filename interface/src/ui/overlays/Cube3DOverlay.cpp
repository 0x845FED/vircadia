//
//  Cube3DOverlay.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>

#include <DeferredLightingEffect.h>
#include <GlowEffect.h>
#include <SharedUtil.h>
#include <StreamUtils.h>

#include "Application.h"
#include "Cube3DOverlay.h"

Cube3DOverlay::Cube3DOverlay() : _borderSize(0) {
}

Cube3DOverlay::Cube3DOverlay(const Cube3DOverlay* cube3DOverlay) :
    Volume3DOverlay(cube3DOverlay)
{
}

Cube3DOverlay::~Cube3DOverlay() {
}

void Cube3DOverlay::render(RenderArgs* args) {
    if (!_visible) {
        return; // do nothing if we're not visible
    }


    float glowLevel = getGlowLevel();
    Glower* glower = NULL;
    if (glowLevel > 0.0f) {
        glower = new Glower(glowLevel);
    }

    float alpha = getAlpha();
    xColor color = getColor();
    const float MAX_COLOR = 255.0f;
    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);

    //glDisable(GL_LIGHTING);

    // TODO: handle registration point??    
    glm::vec3 position = getPosition();
    glm::vec3 center = getCenter();
    glm::vec3 dimensions = getDimensions();
    glm::quat rotation = getRotation();
    
    glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glm::vec3 axis = glm::axis(rotation);
        glRotatef(glm::degrees(glm::angle(rotation)), axis.x, axis.y, axis.z);
        glPushMatrix();
            glm::vec3 positionToCenter = center - position;
            glTranslatef(positionToCenter.x, positionToCenter.y, positionToCenter.z);
            if (_isSolid) {
                if (_borderSize > 0) {
                    // Draw a cube at a larger size behind the main cube, creating
                    // a border effect.
                    // Disable writing to the depth mask so that the "border" cube will not
                    // occlude the main cube.  This means the border could be covered by
                    // overlays that are further back and drawn later, but this is good
                    // enough for the use-case.
                    glDepthMask(GL_FALSE);
                    glPushMatrix();
                        glColor4f(1.0f, 1.0f, 1.0f, alpha);
                        glScalef(dimensions.x * _borderSize, dimensions.y * _borderSize, dimensions.z * _borderSize);

                        if (_drawOnApplicationOverlay) {
                            glutSolidCube(1.0f);
                        } else {
                            DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(1.0f);
                        }

                    glPopMatrix();
                    glDepthMask(GL_TRUE);
                }

                glPushMatrix();
                    glColor4f(color.red / MAX_COLOR, color.green / MAX_COLOR, color.blue / MAX_COLOR, alpha);
                    glScalef(dimensions.x, dimensions.y, dimensions.z);
                    if (_drawOnApplicationOverlay) {
                        DependencyManager::get<GeometryCache>()->renderSolidCube(1.0f);
                    } else {
                        DependencyManager::get<DeferredLightingEffect>()->renderSolidCube(1.0f);
                    }
                glPopMatrix();
            } else {
                glLineWidth(_lineWidth);

                if (getIsDashedLine()) {
                    glm::vec3 halfDimensions = dimensions / 2.0f;
                    glm::vec3 bottomLeftNear(-halfDimensions.x, -halfDimensions.y, -halfDimensions.z);
                    glm::vec3 bottomRightNear(halfDimensions.x, -halfDimensions.y, -halfDimensions.z);
                    glm::vec3 topLeftNear(-halfDimensions.x, halfDimensions.y, -halfDimensions.z);
                    glm::vec3 topRightNear(halfDimensions.x, halfDimensions.y, -halfDimensions.z);

                    glm::vec3 bottomLeftFar(-halfDimensions.x, -halfDimensions.y, halfDimensions.z);
                    glm::vec3 bottomRightFar(halfDimensions.x, -halfDimensions.y, halfDimensions.z);
                    glm::vec3 topLeftFar(-halfDimensions.x, halfDimensions.y, halfDimensions.z);
                    glm::vec3 topRightFar(halfDimensions.x, halfDimensions.y, halfDimensions.z);
                
                    drawDashedLine(bottomLeftNear, bottomRightNear);
                    drawDashedLine(bottomRightNear, bottomRightFar);
                    drawDashedLine(bottomRightFar, bottomLeftFar);
                    drawDashedLine(bottomLeftFar, bottomLeftNear);

                    drawDashedLine(topLeftNear, topRightNear);
                    drawDashedLine(topRightNear, topRightFar);
                    drawDashedLine(topRightFar, topLeftFar);
                    drawDashedLine(topLeftFar, topLeftNear);

                    drawDashedLine(bottomLeftNear, topLeftNear);
                    drawDashedLine(bottomRightNear, topRightNear);
                    drawDashedLine(bottomLeftFar, topLeftFar);
                    drawDashedLine(bottomRightFar, topRightFar);

                } else {
                    glScalef(dimensions.x, dimensions.y, dimensions.z);
                    DependencyManager::get<DeferredLightingEffect>()->renderWireCube(1.0f);
                }
            }
        glPopMatrix();
    glPopMatrix();

    if (glower) {
        delete glower;
    }
}

Cube3DOverlay* Cube3DOverlay::createClone() const {
    return new Cube3DOverlay(this);
}

void Cube3DOverlay::setProperties(const QScriptValue& properties) {
    Volume3DOverlay::setProperties(properties);

    QScriptValue borderSize = properties.property("borderSize");

    if (borderSize.isValid()) {
        float value = borderSize.toVariant().toFloat();
        setBorderSize(value);
    }
}

QScriptValue Cube3DOverlay::getProperty(const QString& property) {
    if (property == "borderSize") {
        return _borderSize;
    }

    return Volume3DOverlay::getProperty(property);
}
