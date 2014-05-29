//
//  VoxelServer.cpp
//  assignment-client/src/voxels
//
//  Created by Brad Hefta-Gaub on 9/16/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <VoxelTree.h>

#include "VoxelServer.h"
#include "VoxelServerConsts.h"
#include "VoxelNodeData.h"

const char* VOXEL_SERVER_NAME = "Voxel";
const char* VOXEL_SERVER_LOGGING_TARGET_NAME = "voxel-server";
const char* LOCAL_VOXELS_PERSIST_FILE = "resources/voxels.svo";

VoxelServer::VoxelServer(const QByteArray& packet) : OctreeServer(packet) {
    // nothing special to do here...
}

VoxelServer::~VoxelServer() {
    // nothing special to do here...
}

OctreeQueryNode* VoxelServer::createOctreeQueryNode() {
    return new VoxelNodeData();
}

Octree* VoxelServer::createTree() {
    return new VoxelTree(true);
}

bool VoxelServer::hasSpecialPacketToSend(const SharedNodePointer& node) {
    bool shouldSendEnvironments = _sendEnvironments && shouldDo(ENVIRONMENT_SEND_INTERVAL_USECS, OCTREE_SEND_INTERVAL_USECS);
    return shouldSendEnvironments;
}

int VoxelServer::sendSpecialPacket(OCTREE_PACKET_SEQUENCE& sequenceNumber, const SharedNodePointer& node) {

    unsigned char* copyAt = _tempOutputBuffer;    

    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(_tempOutputBuffer), PacketTypeEnvironmentData);
    copyAt += numBytesPacketHeader;
    int envPacketLength = numBytesPacketHeader;


    // pack in flags
    OCTREE_PACKET_FLAGS flags = 0;
    OCTREE_PACKET_FLAGS* flagsAt = (OCTREE_PACKET_FLAGS*)copyAt;
    *flagsAt = flags;
    copyAt += sizeof(OCTREE_PACKET_FLAGS);
    envPacketLength += sizeof(OCTREE_PACKET_FLAGS);

    // pack in sequence number
    OCTREE_PACKET_SEQUENCE* sequenceAt = (OCTREE_PACKET_SEQUENCE*)copyAt;
    *sequenceAt = sequenceNumber;
    copyAt += sizeof(OCTREE_PACKET_SEQUENCE);
    envPacketLength += sizeof(OCTREE_PACKET_SEQUENCE);

    // pack in timestamp
    OCTREE_PACKET_SENT_TIME now = usecTimestampNow();
    OCTREE_PACKET_SENT_TIME* timeAt = (OCTREE_PACKET_SENT_TIME*)copyAt;
    *timeAt = now;
    copyAt += sizeof(OCTREE_PACKET_SENT_TIME);
    envPacketLength += sizeof(OCTREE_PACKET_SENT_TIME);


    int environmentsToSend = getSendMinimalEnvironment() ? 1 : getEnvironmentDataCount();

    for (int i = 0; i < environmentsToSend; i++) {
        envPacketLength += getEnvironmentData(i)->getBroadcastData(_tempOutputBuffer + envPacketLength);
    }

    NodeList::getInstance()->writeDatagram((char*) _tempOutputBuffer, envPacketLength, SharedNodePointer(node));
    sequenceNumber++;

    return envPacketLength;
}


void VoxelServer::beforeRun() {
    // should we send environments? Default is yes, but this command line suppresses sending
    const char* SEND_ENVIRONMENTS = "--sendEnvironments";
    bool dontSendEnvironments =  !cmdOptionExists(_argc, _argv, SEND_ENVIRONMENTS);
    if (dontSendEnvironments) {
        qDebug("Sending environments suppressed...");
        _sendEnvironments = false;
    } else {
        _sendEnvironments = true;
        // should we send environments? Default is yes, but this command line suppresses sending
        const char* MINIMAL_ENVIRONMENT = "--minimalEnvironment";
        _sendMinimalEnvironment =  cmdOptionExists(_argc, _argv, MINIMAL_ENVIRONMENT);
        qDebug("Using Minimal Environment=%s", debug::valueOf(_sendMinimalEnvironment));
    }
    qDebug("Sending environments=%s", debug::valueOf(_sendEnvironments));
    
    NodeList::getInstance()->addNodeTypeToInterestSet(NodeType::AnimationServer);
}
