//
//  AudioInjector.cpp
//  hifi
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QDataStream>

#include <NodeList.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <UUID.h>

#include "AbstractAudioInterface.h"
#include "AudioRingBuffer.h"

#include "AudioInjector.h"

int abstractAudioPointerMeta = qRegisterMetaType<AbstractAudioInterface*>("AbstractAudioInterface*");

AudioInjector::AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions) :
    _sound(sound),
    _options(injectorOptions)
{
    
}

const uchar MAX_INJECTOR_VOLUME = 0xFF;

void AudioInjector::injectAudio() {
    
    QByteArray soundByteArray = _sound->getByteArray();
    
    // make sure we actually have samples downloaded to inject
    if (soundByteArray.size()) {
        // give our sample byte array to the local audio interface, if we have it, so it can be handled locally
        if (_options.getLoopbackAudioInterface()) {
            // assume that localAudioInterface could be on a separate thread, use Qt::AutoConnection to handle properly
            QMetaObject::invokeMethod(_options.getLoopbackAudioInterface(), "handleAudioByteArray",
                                      Qt::AutoConnection,
                                      Q_ARG(QByteArray, soundByteArray));
            
        }
        
        NodeList* nodeList = NodeList::getInstance();
        
        // setup the packet for injected audio
        QByteArray injectAudioPacket = byteArrayWithPopluatedHeader(PacketTypeInjectAudio);
        QDataStream packetStream(&injectAudioPacket, QIODevice::Append);
        
        packetStream << QUuid::createUuid();
        
        // pack the flag for loopback
        uchar loopbackFlag = (uchar) (_options.getLoopbackAudioInterface() == NULL);
        packetStream << loopbackFlag;
        
        // pack the position for injected audio
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getPosition()), sizeof(_options.getPosition()));
        
        // pack our orientation for injected audio
        packetStream.writeRawData(reinterpret_cast<const char*>(&_options.getOrientation()), sizeof(_options.getOrientation()));
        
        // pack zero for radius
        float radius = 0;
        packetStream << radius;
        
        // pack 255 for attenuation byte
        quint8 volume = MAX_INJECTOR_VOLUME * _options.getVolume();
        packetStream << volume;
        
        timeval startTime = {};
        gettimeofday(&startTime, NULL);
        int nextFrame = 0;
        
        int currentSendPosition = 0;
        
        int numPreAudioDataBytes = injectAudioPacket.size();
        
        // loop to send off our audio in NETWORK_BUFFER_LENGTH_SAMPLES_PER_CHANNEL byte chunks
        while (currentSendPosition < soundByteArray.size()) {
            
            int bytesToCopy = std::min(NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL,
                                       soundByteArray.size() - currentSendPosition);
            
            // resize the QByteArray to the right size
            injectAudioPacket.resize(numPreAudioDataBytes + bytesToCopy);
            
            // copy the next NETWORK_BUFFER_LENGTH_BYTES_PER_CHANNEL bytes to the packet
            memcpy(injectAudioPacket.data() + numPreAudioDataBytes, soundByteArray.data() + currentSendPosition, bytesToCopy);
            
            // grab our audio mixer from the NodeList, if it exists
            SharedNodePointer audioMixer = nodeList->soloNodeOfType(NodeType::AudioMixer);
            
            // send off this audio packet
            nodeList->writeDatagram(injectAudioPacket, audioMixer);
            
            currentSendPosition += bytesToCopy;
            
            // send two packets before the first sleep so the mixer can start playback right away
            
            if (currentSendPosition != bytesToCopy && currentSendPosition < soundByteArray.size()) {
                // not the first packet and not done
                // sleep for the appropriate time
                int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();
                
                if (usecToSleep > 0) {
                    usleep(usecToSleep);
                }
            }
        }
    }
    
    emit finished();
}
