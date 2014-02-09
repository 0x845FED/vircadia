//
//  EventTypes.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/28/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  Used to register meta-types with Qt for very various event types so that they can be exposed to our
//  scripting engine

#include <QDebug>
#include <RegisteredMetaTypes.h>
#include "EventTypes.h"


void registerEventTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, keyEventToScriptValue, keyEventFromScriptValue);
    qScriptRegisterMetaType(engine, mouseEventToScriptValue, mouseEventFromScriptValue);
    qScriptRegisterMetaType(engine, touchEventToScriptValue, touchEventFromScriptValue);
    qScriptRegisterMetaType(engine, wheelEventToScriptValue, wheelEventFromScriptValue);
}

KeyEvent::KeyEvent() :
    key(0),
    text(""),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false),
    isKeypad(false),
    isValid(false)
{
};


KeyEvent::KeyEvent(const QKeyEvent& event) {
    key = event.key();
    text = event.text();
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
    isKeypad = event.modifiers().testFlag(Qt::KeypadModifier);
    isValid = true;

    // handle special text for special characters...
    if (key == Qt::Key_F1) {
        text = "F1";
    } else if (key == Qt::Key_F2) {
        text = "F2";
    } else if (key == Qt::Key_F3) {
        text = "F3";
    } else if (key == Qt::Key_F4) {
        text = "F4";
    } else if (key == Qt::Key_F5) {
        text = "F5";
    } else if (key == Qt::Key_F6) {
        text = "F6";
    } else if (key == Qt::Key_F7) {
        text = "F7";
    } else if (key == Qt::Key_F8) {
        text = "F8";
    } else if (key == Qt::Key_F9) {
        text = "F9";
    } else if (key == Qt::Key_F10) {
        text = "F10";
    } else if (key == Qt::Key_F11) {
        text = "F11";
    } else if (key == Qt::Key_F12) {
        text = "F12";
    } else if (key == Qt::Key_Up) {
        text = "UP";
    } else if (key == Qt::Key_Down) {
        text = "DOWN";
    } else if (key == Qt::Key_Left) {
        text = "LEFT";
    } else if (key == Qt::Key_Right) {
        text = "RIGHT";
    } else if (key == Qt::Key_Escape) {
        text = "ESC";
    } else if (key == Qt::Key_Tab) {
        text = "TAB";
    } else if (key == Qt::Key_Delete) {
        text = "DELETE";
    } else if (key == Qt::Key_Backspace) {
        text = "BACKSPACE";
    } else if (key == Qt::Key_Shift) {
        text = "SHIFT";
    } else if (key == Qt::Key_Alt) {
        text = "ALT";
    } else if (key == Qt::Key_Control) {
        text = "CONTROL";
    } else if (key == Qt::Key_Meta) {
        text = "META";
    } else if (key == Qt::Key_PageDown) {
        text = "PAGE DOWN";
    } else if (key == Qt::Key_PageUp) {
        text = "PAGE UP";
    } else if (key == Qt::Key_Home) {
        text = "HOME";
    } else if (key == Qt::Key_End) {
        text = "END";
    } else if (key == Qt::Key_Help) {
        text = "HELP";
    }
}

bool KeyEvent::operator==(const KeyEvent& other) const { 
    return other.key == key 
        && other.isShifted == isShifted 
        && other.isControl == isControl
        && other.isMeta == isMeta
        && other.isAlt == isAlt
        && other.isKeypad == isKeypad; 
}

QScriptValue keyEventToScriptValue(QScriptEngine* engine, const KeyEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("key", event.key);
    obj.setProperty("text", event.text);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    obj.setProperty("isKeypad", event.isKeypad);
    return obj;
}

void keyEventFromScriptValue(const QScriptValue& object, KeyEvent& event) {

    event.isValid = false; // assume the worst
    event.isMeta = object.property("isMeta").toVariant().toBool();
    event.isControl = object.property("isControl").toVariant().toBool();
    event.isAlt = object.property("isAlt").toVariant().toBool();
    event.isKeypad = object.property("isKeypad").toVariant().toBool();

    QScriptValue key = object.property("key");
    if (key.isValid()) {
        event.key = key.toVariant().toInt();
        event.text = QString(QChar(event.key));
        event.isValid = true;
    } else {
        QScriptValue text = object.property("text");
        if (text.isValid()) {
            event.text = object.property("text").toVariant().toString();
            
            // if the text is a special command, then map it here...
            // TODO: come up with more elegant solution here, a map? is there a Qt function that gives nice names for keys?
            if (event.text.toUpper() == "F1") {
                event.key = Qt::Key_F1;
            } else if (event.text.toUpper() == "F2") {
                event.key = Qt::Key_F2;
            } else if (event.text.toUpper() == "F3") {
                event.key = Qt::Key_F3;
            } else if (event.text.toUpper() == "F4") {
                event.key = Qt::Key_F4;
            } else if (event.text.toUpper() == "F5") {
                event.key = Qt::Key_F5;
            } else if (event.text.toUpper() == "F6") {
                event.key = Qt::Key_F6;
            } else if (event.text.toUpper() == "F7") {
                event.key = Qt::Key_F7;
            } else if (event.text.toUpper() == "F8") {
                event.key = Qt::Key_F8;
            } else if (event.text.toUpper() == "F9") {
                event.key = Qt::Key_F9;
            } else if (event.text.toUpper() == "F10") {
                event.key = Qt::Key_F10;
            } else if (event.text.toUpper() == "F11") {
                event.key = Qt::Key_F11;
            } else if (event.text.toUpper() == "F12") {
                event.key = Qt::Key_F12;
            } else if (event.text.toUpper() == "UP") {
                event.key = Qt::Key_Up;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "DOWN") {
                event.key = Qt::Key_Down;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "LEFT") {
                event.key = Qt::Key_Left;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "RIGHT") {
                event.key = Qt::Key_Right;
                event.isKeypad = true;
            } else if (event.text.toUpper() == "ESC") {
                event.key = Qt::Key_Escape;
            } else if (event.text.toUpper() == "TAB") {
                event.key = Qt::Key_Tab;
            } else if (event.text.toUpper() == "DELETE") {
                event.key = Qt::Key_Delete;
            } else if (event.text.toUpper() == "BACKSPACE") {
                event.key = Qt::Key_Backspace;
            } else if (event.text.toUpper() == "SHIFT") {
                event.key = Qt::Key_Shift;
            } else if (event.text.toUpper() == "ALT") {
                event.key = Qt::Key_Alt;
            } else if (event.text.toUpper() == "CONTROL") {
                event.key = Qt::Key_Control;
            } else if (event.text.toUpper() == "META") {
                event.key = Qt::Key_Meta;
            } else if (event.text.toUpper() == "PAGE DOWN") {
                event.key = Qt::Key_PageDown;
            } else if (event.text.toUpper() == "PAGE UP") {
                event.key = Qt::Key_PageUp;
            } else if (event.text.toUpper() == "HOME") {
                event.key = Qt::Key_Home;
            } else if (event.text.toUpper() == "END") {
                event.key = Qt::Key_End;
            } else if (event.text.toUpper() == "HELP") {
                event.key = Qt::Key_Help;
            } else {
                event.key = event.text.at(0).unicode();
            }
            event.isValid = true;
        }
    }

    QScriptValue isShifted = object.property("isShifted");
    if (isShifted.isValid()) {
        event.isShifted = isShifted.toVariant().toBool();
    } else {
        // if no isShifted was included, get it from the text
        QChar character = event.text.at(0);
        if (character.isLetter() && character.isUpper()) {
            event.isShifted = true;
        } else {
            // if it's a symbol, then attempt to detect shifted-ness
            if (QString("~!@#$%^&*()_+{}|:\"<>?").contains(character)) {
                event.isShifted = true;
            }
        }
    }
    

    const bool wantDebug = false;
    if (wantDebug) {    
        qDebug() << "event.key=" << event.key
            << " event.text=" << event.text
            << " event.isShifted=" << event.isShifted
            << " event.isControl=" << event.isControl
            << " event.isMeta=" << event.isMeta
            << " event.isAlt=" << event.isAlt
            << " event.isKeypad=" << event.isKeypad;
    }
}

MouseEvent::MouseEvent() : 
    x(0), 
    y(0), 
    isLeftButton(false), 
    isRightButton(false), 
    isMiddleButton(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false)
{ 
}; 


MouseEvent::MouseEvent(const QMouseEvent& event) {
    x = event.x();
    y = event.y();
    
    // single button that caused the event
    switch (event.button()) {
        case Qt::LeftButton:
            button = "LEFT";
            isLeftButton = true;
            break;
        case Qt::RightButton:
            button = "RIGHT";
            isRightButton = true;
            break;
        case Qt::MiddleButton:
            button = "MIDDLE";
            isMiddleButton = true;
            break;
        default:
            button = "NONE";
            break;
    }
    // button pressed state
    isLeftButton = isLeftButton || (event.buttons().testFlag(Qt::LeftButton));
    isRightButton = isRightButton || (event.buttons().testFlag(Qt::RightButton));
    isMiddleButton = isMiddleButton || (event.buttons().testFlag(Qt::MiddleButton));

    // keyboard modifiers
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
}

QScriptValue mouseEventToScriptValue(QScriptEngine* engine, const MouseEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("button", event.button);
    obj.setProperty("isLeftButton", event.isLeftButton);
    obj.setProperty("isRightButton", event.isRightButton);
    obj.setProperty("isMiddleButton", event.isMiddleButton);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);

    return obj;
}

void mouseEventFromScriptValue(const QScriptValue& object, MouseEvent& event) {
    // nothing for now...
}

TouchEvent::TouchEvent() : 
    x(0), 
    y(0),
    isPressed(false),
    isMoved(false),
    isStationary(false),
    isReleased(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false),
    points(),
    radius(0),
    isPinching(false),
    isPinchOpening(false)
{
};

TouchEvent::TouchEvent(const QTouchEvent& event) {
    initWithQTouchEvent(event);
}

TouchEvent::TouchEvent(const QTouchEvent& event, const TouchEvent& other) {
    initWithQTouchEvent(event);
    calculateMetaAttributes(other);
}

void TouchEvent::initWithQTouchEvent(const QTouchEvent& event) {
    // convert the touch points into an average    
    const QList<QTouchEvent::TouchPoint>& tPoints = event.touchPoints();
    float touchAvgX = 0.0f;
    float touchAvgY = 0.0f;
    int numTouches = tPoints.count();
    if (numTouches > 1) {
        for (int i = 0; i < numTouches; ++i) {
            touchAvgX += tPoints[i].pos().x();
            touchAvgY += tPoints[i].pos().y();
            
            // add it to our points vector
            glm::vec2 thisPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
            points << thisPoint;
        }
        touchAvgX /= (float)(numTouches);
        touchAvgY /= (float)(numTouches);
    } else {
        // I'm not sure this should ever happen, why would Qt send us a touch event for only one point?
        // maybe this happens in the case of a multi-touch where all but the last finger is released?
        touchAvgX = tPoints[0].pos().x();
        touchAvgY = tPoints[0].pos().y();
    }
    x = touchAvgX;
    y = touchAvgY;
    
    // after calculating the center point (average touch point), determine the maximum radius
    float maxRadius = 0.0f;
    glm::vec2 center(x,y);
    for (int i = 0; i < numTouches; ++i) {
        glm::vec2 touchPoint(tPoints[i].pos().x(), tPoints[i].pos().y());
        float thisRadius = glm::distance(center,touchPoint);
        if (thisRadius > maxRadius) {
            maxRadius = thisRadius;
        }
    }
    radius = maxRadius;

    isPressed = event.touchPointStates().testFlag(Qt::TouchPointPressed);
    isMoved = event.touchPointStates().testFlag(Qt::TouchPointMoved);
    isStationary = event.touchPointStates().testFlag(Qt::TouchPointStationary);
    isReleased = event.touchPointStates().testFlag(Qt::TouchPointReleased);

    // keyboard modifiers
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
}

void TouchEvent::calculateMetaAttributes(const TouchEvent& other) {
    // calculate comparative event attributes...
    if (other.radius > radius) {
        isPinching = true;
        isPinchOpening = false;
    } else if (other.radius < radius) {
        isPinchOpening = true;
        isPinching = false;
    } else {
        isPinching = other.isPinching;
        isPinchOpening = other.isPinchOpening;
    }
}


QScriptValue touchEventToScriptValue(QScriptEngine* engine, const TouchEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("isPressed", event.isPressed);
    obj.setProperty("isMoved", event.isMoved);
    obj.setProperty("isStationary", event.isStationary);
    obj.setProperty("isReleased", event.isReleased);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    
    QScriptValue pointsObj = engine->newArray();
    int index = 0;
    foreach (glm::vec2 point, event.points) {
        QScriptValue thisPoint = vec2toScriptValue(engine, point);
        pointsObj.setProperty(index, thisPoint);
        index++;
    }    
    obj.setProperty("points", pointsObj);
    obj.setProperty("radius", event.radius);
    obj.setProperty("isPinching", event.isPinching);
    obj.setProperty("isPinchOpening", event.isPinchOpening);
    return obj;
}

void touchEventFromScriptValue(const QScriptValue& object, TouchEvent& event) {
    // nothing for now...
}

WheelEvent::WheelEvent() : 
    x(0), 
    y(0), 
    delta(0), 
    orientation("UNKNOwN"), 
    isLeftButton(false), 
    isRightButton(false), 
    isMiddleButton(false),
    isShifted(false),
    isControl(false),
    isMeta(false),
    isAlt(false)
{ 
}; 

WheelEvent::WheelEvent(const QWheelEvent& event) {
    x = event.x();
    y = event.y();

    delta = event.delta();
    if (event.orientation() == Qt::Horizontal) {
        orientation = "HORIZONTAL";
    } else {
        orientation = "VERTICAL";
    }

    // button pressed state
    isLeftButton = (event.buttons().testFlag(Qt::LeftButton));
    isRightButton = (event.buttons().testFlag(Qt::RightButton));
    isMiddleButton = (event.buttons().testFlag(Qt::MiddleButton));

    // keyboard modifiers
    isShifted = event.modifiers().testFlag(Qt::ShiftModifier);
    isMeta = event.modifiers().testFlag(Qt::MetaModifier);
    isControl = event.modifiers().testFlag(Qt::ControlModifier);
    isAlt = event.modifiers().testFlag(Qt::AltModifier);
}


QScriptValue wheelEventToScriptValue(QScriptEngine* engine, const WheelEvent& event) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", event.x);
    obj.setProperty("y", event.y);
    obj.setProperty("delta", event.delta);
    obj.setProperty("orientation", event.orientation);
    obj.setProperty("isLeftButton", event.isLeftButton);
    obj.setProperty("isRightButton", event.isRightButton);
    obj.setProperty("isMiddleButton", event.isMiddleButton);
    obj.setProperty("isShifted", event.isShifted);
    obj.setProperty("isMeta", event.isMeta);
    obj.setProperty("isControl", event.isControl);
    obj.setProperty("isAlt", event.isAlt);
    return obj;
}

void wheelEventFromScriptValue(const QScriptValue& object, WheelEvent& event) {
    // nothing for now...
}


