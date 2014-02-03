//
//  ParticleCollisionSystem.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__ParticleCollisionSystem__
#define __hifi__ParticleCollisionSystem__

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <AvatarHashMap.h>
#include <CollisionInfo.h>
#include <SharedUtil.h>
#include <OctreePacketData.h>

#include "Particle.h"

class AbstractAudioInterface;
class AvatarData;
class ParticleEditPacketSender;
class ParticleTree;
class VoxelTree;

const glm::vec3 NO_ADDED_VELOCITY = glm::vec3(0);

class ParticleCollisionSystem : public QObject {
Q_OBJECT
public:
    ParticleCollisionSystem(ParticleEditPacketSender* packetSender = NULL, ParticleTree* particles = NULL, 
                                VoxelTree* voxels = NULL, AbstractAudioInterface* audio = NULL, 
                                AvatarHashMap* avatars = NULL);

    void init(ParticleEditPacketSender* packetSender, ParticleTree* particles, VoxelTree* voxels, 
                                AbstractAudioInterface* audio = NULL, AvatarHashMap* _avatars = NULL);
                                
    ~ParticleCollisionSystem();

    void update();

    void checkParticle(Particle* particle);
    void updateCollisionWithVoxels(Particle* particle);
    void updateCollisionWithParticles(Particle* particle);
    void updateCollisionWithAvatars(Particle* particle);
    void queueParticlePropertiesUpdate(Particle* particle);
    void updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency);

signals:
    void particleCollisionWithVoxel(const ParticleID& particleID, const VoxelDetail& voxel);
    void particleCollisionWithParticle(const ParticleID& idA, const ParticleID& idB);

private:
    static bool updateOperation(OctreeElement* element, void* extraData);
    void emitGlobalParticleCollisionWithVoxel(Particle* particle, VoxelDetail* voxelDetails);
    void emitGlobalParticleCollisionWithParticle(Particle* particleA, Particle* particleB);

    ParticleEditPacketSender* _packetSender;
    ParticleTree* _particles;
    VoxelTree* _voxels;
    AbstractAudioInterface* _audio;
    AvatarHashMap* _avatars;
};

#endif /* defined(__hifi__ParticleCollisionSystem__) */
