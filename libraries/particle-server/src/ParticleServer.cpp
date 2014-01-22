//
//  ParticleServer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QTimer>
#include <ParticleTree.h>

#include "ParticleServer.h"
#include "ParticleServerConsts.h"
#include "ParticleNodeData.h"

const char* PARTICLE_SERVER_NAME = "Particle";
const char* PARTICLE_SERVER_LOGGING_TARGET_NAME = "particle-server";
const char* LOCAL_PARTICLES_PERSIST_FILE = "resources/particles.svo";

ParticleServer::ParticleServer(const unsigned char* dataBuffer, int numBytes) : OctreeServer(dataBuffer, numBytes) {
    // nothing special to do here...
}

ParticleServer::~ParticleServer() {
    ParticleTree* tree = (ParticleTree*)_tree;
    tree->removeNewlyCreatedHook(this);
}

OctreeQueryNode* ParticleServer::createOctreeQueryNode(Node* newNode) {
    return new ParticleNodeData(newNode);
}

Octree* ParticleServer::createTree() {
    ParticleTree* tree = new ParticleTree(true);
    tree->addNewlyCreatedHook(this);
    return tree;
}

void ParticleServer::beforeRun() {
    QTimer* pruneDeletedParticlesTimer = new QTimer(this);
    connect(pruneDeletedParticlesTimer, SIGNAL(timeout()), this, SLOT(pruneDeletedParticles()));
    const int PRUNE_DELETED_PARTICLES_INTERVAL_MSECS = 1 * 1000; // once every second
    pruneDeletedParticlesTimer->start(PRUNE_DELETED_PARTICLES_INTERVAL_MSECS);
}

void ParticleServer::particleCreated(const Particle& newParticle, Node* node) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    unsigned char* copyAt = outputBuffer;

    int numBytesPacketHeader = populateTypeAndVersion(outputBuffer, PACKET_TYPE_PARTICLE_ADD_RESPONSE);
    int packetLength = numBytesPacketHeader;
    copyAt += numBytesPacketHeader;

    // encode the creatorTokenID
    uint32_t creatorTokenID = newParticle.getCreatorTokenID();
    memcpy(copyAt, &creatorTokenID, sizeof(creatorTokenID));
    copyAt += sizeof(creatorTokenID);
    packetLength += sizeof(creatorTokenID);

    // encode the particle ID
    uint32_t particleID = newParticle.getID();
    memcpy(copyAt, &particleID, sizeof(particleID));
    copyAt += sizeof(particleID);
    packetLength += sizeof(particleID);

    NodeList::getInstance()->getNodeSocket().writeDatagram((char*) outputBuffer, packetLength,
                                                           node->getActiveSocket()->getAddress(),
                                                           node->getActiveSocket()->getPort());
}


// ParticleServer will use the "special packets" to send list of recently deleted particles
bool ParticleServer::hasSpecialPacketToSend(Node* node) {
    bool shouldSendDeletedParticles = false;

    // check to see if any new particles have been added since we last sent to this node...
    ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(node->getLinkedData());
    if (nodeData) {
        uint64_t deletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();

        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        shouldSendDeletedParticles = tree->hasParticlesDeletedSince(deletedParticlesSentAt);
    }

    return shouldSendDeletedParticles;
}

int ParticleServer::sendSpecialPacket(Node* node) {
    unsigned char outputBuffer[MAX_PACKET_SIZE];
    size_t packetLength = 0;

    ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(node->getLinkedData());
    if (nodeData) {
        uint64_t deletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();
        uint64_t deletePacketSentAt = usecTimestampNow();

        ParticleTree* tree = static_cast<ParticleTree*>(_tree);
        bool hasMoreToSend = true;

        // TODO: is it possible to send too many of these packets? what if you deleted 1,000,000 particles?
        while (hasMoreToSend) {
            hasMoreToSend = tree->encodeParticlesDeletedSince(deletedParticlesSentAt,
                                                outputBuffer, MAX_PACKET_SIZE, packetLength);

            //qDebug() << "sending PACKET_TYPE_PARTICLE_ERASE packetLength:" << packetLength;

            NodeList::getInstance()->getNodeSocket().writeDatagram((char*) outputBuffer, packetLength,
                                                                   node->getActiveSocket()->getAddress(),
                                                                   node->getActiveSocket()->getPort());
        }

        nodeData->setLastDeletedParticlesSentAt(deletePacketSentAt);
    }

    // TODO: caller is expecting a packetLength, what if we send more than one packet??
    return packetLength;
}

void ParticleServer::pruneDeletedParticles() {
    ParticleTree* tree = static_cast<ParticleTree*>(_tree);
    if (tree->hasAnyDeletedParticles()) {

        //qDebug() << "there are some deleted particles to consider...";
        uint64_t earliestLastDeletedParticlesSent = usecTimestampNow() + 1; // in the future
        foreach (const SharedNodePointer& otherNode, NodeList::getInstance()->getNodeHash()) {
            if (otherNode->getLinkedData()) {
                ParticleNodeData* nodeData = static_cast<ParticleNodeData*>(otherNode->getLinkedData());
                uint64_t nodeLastDeletedParticlesSentAt = nodeData->getLastDeletedParticlesSentAt();
                if (nodeLastDeletedParticlesSentAt < earliestLastDeletedParticlesSent) {
                    earliestLastDeletedParticlesSent = nodeLastDeletedParticlesSentAt;
                }
            }
        }
        //qDebug() << "earliestLastDeletedParticlesSent=" << earliestLastDeletedParticlesSent;
        tree->forgetParticlesDeletedBefore(earliestLastDeletedParticlesSent);
    }
}

