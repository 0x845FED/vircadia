//
//  ParticleEditPacketSender.cpp
//  libraries/particles/src
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <assert.h>
#include <PerfStat.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include "ParticleEditPacketSender.h"
#include "Particle.h"


void ParticleEditPacketSender::sendEditParticleMessage(PacketType type, ParticleID particleID, const ParticleProperties& properties) {
    // allows app to disable sending if for example voxels have been disabled
    if (!_shouldSend) {
        return; // bail early
    }

    static unsigned char bufferOut[MAX_PACKET_SIZE];
    int sizeOut = 0;

    // This encodes the voxel edit message into a buffer...
    if (Particle::encodeParticleEditMessageDetails(type, particleID, properties, &bufferOut[0], _maxPacketSize, sizeOut)){
        // If we don't have voxel jurisdictions, then we will simply queue up these packets and wait till we have
        // jurisdictions for processing
        if (!serversExist()) {
            queuePendingPacketToNodes(type, bufferOut, sizeOut);
        } else {
            queuePacketToNodes(bufferOut, sizeOut);
        }
    }
}

void ParticleEditPacketSender::adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew) {
    Particle::adjustEditPacketForClockSkew(codeColorBuffer, length, clockSkew);
}


void ParticleEditPacketSender::queueParticleEditMessage(PacketType type, ParticleID particleID, 
                                                                const ParticleProperties& properties) {
    if (!_shouldSend) {
        return; // bail early
    }

    // use MAX_PACKET_SIZE since it's static and guaranteed to be larger than _maxPacketSize
    static unsigned char bufferOut[MAX_PACKET_SIZE];
    int sizeOut = 0;

    if (Particle::encodeParticleEditMessageDetails(type, particleID, properties, &bufferOut[0], _maxPacketSize, sizeOut)) {
        queueOctreeEditMessage(type, bufferOut, sizeOut);
    }
}

