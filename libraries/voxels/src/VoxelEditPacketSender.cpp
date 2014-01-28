//
//  VoxelEditPacketSender.cpp
//  interface
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Threaded or non-threaded voxel packet Sender for the Application
//

#include <assert.h>
#include <PerfStat.h>
#include <OctalCode.h>
#include <PacketHeaders.h>
#include "VoxelEditPacketSender.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Function:    createVoxelEditMessage()
// Description: creates an "insert" or "remove" voxel message for a voxel code
//              corresponding to the closest voxel which encloses a cube with
//              lower corners at x,y,z, having side of length S.
//              The input values x,y,z range 0.0 <= v < 1.0
//              message should be either 'S' for SET or 'E' for ERASE
//
// IMPORTANT:   The buffer is returned to you a buffer which you MUST delete when you are
//              done with it.
//
// HACK ATTACK: Well, what if this is larger than the MTU? That's the caller's problem, we
//              just truncate the message
//
// Complaints:  Brad :)
#define GUESS_OF_VOXELCODE_SIZE 10
#define MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE 1500
#define SIZE_OF_COLOR_DATA sizeof(rgbColor)
bool createVoxelEditMessage(PacketType command, short int sequence,
                            int voxelCount, VoxelDetail* voxelDetails, unsigned char*& bufferOut, int& sizeOut) {
    
    bool success = true; // assume the best
    int messageSize = MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE; // just a guess for now
    unsigned char* messageBuffer = new unsigned char[messageSize];
    
    int numBytesPacketHeader = populatePacketHeader(reinterpret_cast<char*>(messageBuffer), command);
    unsigned short int* sequenceAt = (unsigned short int*) &messageBuffer[numBytesPacketHeader];
    *sequenceAt = sequence;
    
    // pack in timestamp
    quint64 now = usecTimestampNow();
    quint64* timeAt = (quint64*)&messageBuffer[numBytesPacketHeader + sizeof(sequence)];
    *timeAt = now;
    
    unsigned char* copyAt = &messageBuffer[numBytesPacketHeader + sizeof(sequence) + sizeof(now)];
    int actualMessageSize = numBytesPacketHeader + sizeof(sequence) + sizeof(now);
    
    for (int i = 0; i < voxelCount && success; i++) {
        // get the coded voxel
        unsigned char* voxelData = pointToVoxel(voxelDetails[i].x,voxelDetails[i].y,voxelDetails[i].z,
                                                voxelDetails[i].s,voxelDetails[i].red,voxelDetails[i].green,voxelDetails[i].blue);
        
        int lengthOfVoxelData = bytesRequiredForCodeLength(*voxelData)+SIZE_OF_COLOR_DATA;
        
        // make sure we have room to copy this voxel
        if (actualMessageSize + lengthOfVoxelData > MAXIMUM_EDIT_VOXEL_MESSAGE_SIZE) {
            success = false;
        } else {
            // add it to our message
            memcpy(copyAt, voxelData, lengthOfVoxelData);
            copyAt += lengthOfVoxelData;
            actualMessageSize += lengthOfVoxelData;
        }
        // cleanup
        delete[] voxelData;
    }
    
    if (success) {
        // finally, copy the result to the output
        bufferOut = new unsigned char[actualMessageSize];
        sizeOut = actualMessageSize;
        memcpy(bufferOut, messageBuffer, actualMessageSize);
    }
    
    delete[] messageBuffer; // clean up our temporary buffer
    return success;
}

/// encodes the voxel details portion of a voxel edit message
bool encodeVoxelEditMessageDetails(PacketType, int voxelCount, VoxelDetail* voxelDetails,
                                   unsigned char* bufferOut, int sizeIn, int& sizeOut) {
    
    bool success = true; // assume the best
    unsigned char* copyAt = bufferOut;
    sizeOut = 0;
    
    for (int i = 0; i < voxelCount && success; i++) {
        // get the coded voxel
        unsigned char* voxelData = pointToVoxel(voxelDetails[i].x,voxelDetails[i].y,voxelDetails[i].z,
                                                voxelDetails[i].s,voxelDetails[i].red,voxelDetails[i].green,voxelDetails[i].blue);
        
        int lengthOfVoxelData = bytesRequiredForCodeLength(*voxelData)+SIZE_OF_COLOR_DATA;
        
        // make sure we have room to copy this voxel
        if (sizeOut + lengthOfVoxelData > sizeIn) {
            success = false;
        } else {
            // add it to our message
            memcpy(copyAt, voxelData, lengthOfVoxelData);
            copyAt += lengthOfVoxelData;
            sizeOut += lengthOfVoxelData;
        }
        // cleanup
        delete[] voxelData;
    }
    
    return success;
}

void VoxelEditPacketSender::sendVoxelEditMessage(PacketType type, VoxelDetail& detail) {
    // allows app to disable sending if for example voxels have been disabled
    if (!_shouldSend) {
        return; // bail early
    }

    unsigned char* bufferOut;
    int sizeOut;

    // This encodes the voxel edit message into a buffer...
    if (createVoxelEditMessage(type, 0, 1, &detail, bufferOut, sizeOut)){
        // If we don't have voxel jurisdictions, then we will simply queue up these packets and wait till we have
        // jurisdictions for processing
        if (!voxelServersExist()) {
            queuePendingPacketToNodes(type, bufferOut, sizeOut);
        } else {
            queuePacketToNodes(bufferOut, sizeOut);
        }
        
        // either way, clean up the created buffer
        delete[] bufferOut;
    }
}

void VoxelEditPacketSender::queueVoxelEditMessages(PacketType type, int numberOfDetails, VoxelDetail* details) {
    if (!_shouldSend) {
        return; // bail early
    }

    for (int i = 0; i < numberOfDetails; i++) {
        // use MAX_PACKET_SIZE since it's static and guarenteed to be larger than _maxPacketSize
        static unsigned char bufferOut[MAX_PACKET_SIZE]; 
        int sizeOut = 0;
        
        if (encodeVoxelEditMessageDetails(type, 1, &details[i], &bufferOut[0], _maxPacketSize, sizeOut)) {
            queueOctreeEditMessage(type, bufferOut, sizeOut);
        }
    }    
}
