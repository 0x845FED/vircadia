//
//  AbstractAudioInterface.h
//  libraries/audio/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef __hifi__AbstractAudioInterface__
#define __hifi__AbstractAudioInterface__

#include <QtCore/QObject>

class AbstractAudioInterface : public QObject {
    Q_OBJECT
public:
    AbstractAudioInterface(QObject* parent = 0) : QObject(parent) {};
    
    virtual void startCollisionSound(float magnitude, float frequency, float noise, float duration, bool flashScreen) = 0;
    virtual void startDrumSound(float volume, float frequency, float duration, float decay) = 0;
public slots:
    virtual void handleAudioByteArray(const QByteArray& audioByteArray) = 0;
};

Q_DECLARE_METATYPE(AbstractAudioInterface*)

#endif /* defined(__hifi__AbstractAudioInterface__) */
