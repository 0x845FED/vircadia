//
//  ParticleCollisionSystem.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#include <algorithm>
#include <AbstractAudioInterface.h>
#include <VoxelTree.h>
#include <AvatarData.h>
#include <HeadData.h>
#include <HandData.h>

#include "Particle.h"
#include "ParticleCollisionSystem.h"
#include "ParticleEditPacketSender.h"
#include "ParticleTree.h"

ParticleCollisionSystem::ParticleCollisionSystem(ParticleEditPacketSender* packetSender,
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio,
    AvatarHashMap* avatars) {
    init(packetSender, particles, voxels, audio, avatars);
}

void ParticleCollisionSystem::init(ParticleEditPacketSender* packetSender,
    ParticleTree* particles, VoxelTree* voxels, AbstractAudioInterface* audio,
    AvatarHashMap* avatars) {
    _packetSender = packetSender;
    _particles = particles;
    _voxels = voxels;
    _audio = audio;
    _avatars = avatars;
}

ParticleCollisionSystem::~ParticleCollisionSystem() {
}

bool ParticleCollisionSystem::updateOperation(OctreeElement* element, void* extraData) {
    ParticleCollisionSystem* system = static_cast<ParticleCollisionSystem*>(extraData);
    ParticleTreeElement* particleTreeElement = static_cast<ParticleTreeElement*>(element);

    // iterate the particles...
    QList<Particle>& particles = particleTreeElement->getParticles();
    uint16_t numberOfParticles = particles.size();
    for (uint16_t i = 0; i < numberOfParticles; i++) {
        Particle* particle = &particles[i];
        system->checkParticle(particle);
    }

    return true;
}


void ParticleCollisionSystem::update() {
    // update all particles
    _particles->lockForWrite();
    _particles->recurseTreeWithOperation(updateOperation, this);
    _particles->unlock();
}


void ParticleCollisionSystem::checkParticle(Particle* particle) {
    updateCollisionWithVoxels(particle);
    updateCollisionWithParticles(particle);
    updateCollisionWithAvatars(particle);
}

void ParticleCollisionSystem::updateCollisionWithVoxels(Particle* particle) {
    glm::vec3 center = particle->getPosition() * (float)(TREE_SCALE);
    float radius = particle->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.4f;
    const float DAMPING = 0.05f;
    const float COLLISION_FREQUENCY = 0.5f;
    CollisionInfo collisionInfo;
    collisionInfo._damping = DAMPING;
    collisionInfo._elasticity = ELASTICITY;
    VoxelDetail* voxelDetails = NULL;
    if (_voxels->findSpherePenetration(center, radius, collisionInfo._penetration, (void**)&voxelDetails)) {

        // let the particles run their collision scripts if they have them
        particle->collisionWithVoxel(voxelDetails);

        updateCollisionSound(particle, collisionInfo._penetration, COLLISION_FREQUENCY);
        collisionInfo._penetration /= (float)(TREE_SCALE);
        particle->applyHardCollision(collisionInfo);
        queueParticlePropertiesUpdate(particle);

        delete voxelDetails; // cleanup returned details
    }
}

void ParticleCollisionSystem::updateCollisionWithParticles(Particle* particleA) {
    glm::vec3 center = particleA->getPosition() * (float)(TREE_SCALE);
    float radius = particleA->getRadius() * (float)(TREE_SCALE);
    //const float ELASTICITY = 0.4f;
    //const float DAMPING = 0.0f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;
    Particle* particleB;
    if (_particles->findSpherePenetration(center, radius, penetration, (void**)&particleB)) {
        // NOTE: 'penetration' is the depth that 'particleA' overlaps 'particleB'.
        // That is, it points from A into B.

        // Even if the particles overlap... when the particles are already moving appart
        // we don't want to count this as a collision.
        glm::vec3 relativeVelocity = particleA->getVelocity() - particleB->getVelocity();
        if (glm::dot(relativeVelocity, penetration) > 0.0f) {
            particleA->collisionWithParticle(particleB);
            particleB->collisionWithParticle(particleA);

            glm::vec3 axis = glm::normalize(penetration);
            glm::vec3 axialVelocity = glm::dot(relativeVelocity, axis) * axis;

            // particles that are in hand are assigned an ureasonably large mass for collisions
            // which effectively makes them immovable but allows the other ball to reflect correctly.
            const float MAX_MASS = 1.0e6f;
            float massA = (particleA->getInHand()) ? MAX_MASS : particleA->getMass();
            float massB = (particleB->getInHand()) ? MAX_MASS : particleB->getMass();
            float totalMass = massA + massB;

            // handle A particle
            particleA->setVelocity(particleA->getVelocity() - axialVelocity * (2.0f * massB / totalMass));
            particleA->setPosition(particleA->getPosition() - 0.5f * penetration);
            ParticleProperties propertiesA;
            ParticleID particleAid(particleA->getID());
            propertiesA.copyFromParticle(*particleA);
            propertiesA.setVelocity(particleA->getVelocity() * (float)TREE_SCALE);
            propertiesA.setPosition(particleA->getPosition() * (float)TREE_SCALE);
            _packetSender->queueParticleEditMessage(PacketTypeParticleAddOrEdit, particleAid, propertiesA);

            // handle B particle
            particleB->setVelocity(particleB->getVelocity() + axialVelocity * (2.0f * massA / totalMass));
            particleA->setPosition(particleB->getPosition() + 0.5f * penetration);
            ParticleProperties propertiesB;
            ParticleID particleBid(particleB->getID());
            propertiesB.copyFromParticle(*particleB);
            propertiesB.setVelocity(particleB->getVelocity() * (float)TREE_SCALE);
            propertiesB.setPosition(particleB->getPosition() * (float)TREE_SCALE);
            _packetSender->queueParticleEditMessage(PacketTypeParticleAddOrEdit, particleBid, propertiesB);

            _packetSender->releaseQueuedMessages();

            updateCollisionSound(particleA, penetration, COLLISION_FREQUENCY);
        }
    }
}

// MIN_VALID_SPEED is obtained by computing speed gained at one gravity after the shortest expected frame
const float MIN_EXPECTED_FRAME_PERIOD = 0.0167f;  // 1/60th of a second
const float HALTING_SPEED = 9.8 * MIN_EXPECTED_FRAME_PERIOD / (float)(TREE_SCALE);

void ParticleCollisionSystem::updateCollisionWithAvatars(Particle* particle) {
    // particles that are in hand, don't collide with avatars
    if (!_avatars || particle->getInHand()) {
        return;
    }

    glm::vec3 center = particle->getPosition() * (float)(TREE_SCALE);
    float radius = particle->getRadius() * (float)(TREE_SCALE);
    const float ELASTICITY = 0.9f;
    const float DAMPING = 0.1f;
    const float COLLISION_FREQUENCY = 0.5f;
    glm::vec3 penetration;

    foreach (const AvatarSharedPointer& avatarPointer, _avatars->getAvatarHash()) {
        AvatarData* avatar = avatarPointer.data();
        CollisionInfo collisionInfo;
        collisionInfo._damping = DAMPING;
        collisionInfo._elasticity = ELASTICITY;
        if (avatar->findSphereCollision(center, radius, collisionInfo)) {
            collisionInfo._addedVelocity /= (float)(TREE_SCALE);
            glm::vec3 relativeVelocity = collisionInfo._addedVelocity - particle->getVelocity();
            if (glm::dot(relativeVelocity, collisionInfo._penetration) < 0.f) {
                // only collide when particle and collision point are moving toward each other
                // (doing this prevents some "collision snagging" when particle penetrates the object)

                // HACK BEGIN: to allow paddle hands to "hold" particles we attenuate soft collisions against the avatar.
                // NOTE: the physics are wrong (particles cannot roll) but it IS possible to catch a slow moving particle.
                // TODO: make this less hacky when we have more per-collision details
                float elasticity = ELASTICITY;
                float attenuationFactor = glm::length(collisionInfo._addedVelocity) / HALTING_SPEED;
                float damping = DAMPING;
                if (attenuationFactor < 1.f) {
                    collisionInfo._addedVelocity *= attenuationFactor;
                    elasticity *= attenuationFactor;
                    // NOTE: the math below keeps the damping piecewise continuous,
                    // while ramping it up to 1.0 when attenuationFactor = 0
                    damping = DAMPING + (1.f - attenuationFactor) * (1.f - DAMPING);
                }
                // HACK END

                updateCollisionSound(particle, collisionInfo._penetration, COLLISION_FREQUENCY);
                collisionInfo._penetration /= (float)(TREE_SCALE);
                particle->applyHardCollision(collisionInfo);
                queueParticlePropertiesUpdate(particle);
            }
        }
    }
}

void ParticleCollisionSystem::queueParticlePropertiesUpdate(Particle* particle) {
    // queue the result for sending to the particle server
    ParticleProperties properties;
    ParticleID particleID(particle->getID());
    properties.copyFromParticle(*particle);

    properties.setPosition(particle->getPosition() * (float)TREE_SCALE);
    properties.setVelocity(particle->getVelocity() * (float)TREE_SCALE);
    _packetSender->queueParticleEditMessage(PacketTypeParticleAddOrEdit, particleID, properties);
}


void ParticleCollisionSystem::updateCollisionSound(Particle* particle, const glm::vec3 &penetration, float frequency) {

    //  consider whether to have the collision make a sound
    const float AUDIBLE_COLLISION_THRESHOLD = 0.3f;
    const float COLLISION_LOUDNESS = 1.f;
    const float DURATION_SCALING = 0.004f;
    const float NOISE_SCALING = 0.1f;
    glm::vec3 velocity = particle->getVelocity() * (float)(TREE_SCALE);

    /*
    // how do we want to handle this??
    //
     glm::vec3 gravity = particle->getGravity() * (float)(TREE_SCALE);

    if (glm::length(gravity) > EPSILON) {
        //  If gravity is on, remove the effect of gravity on velocity for this
        //  frame, so that we are not constantly colliding with the surface
        velocity -= _scale * glm::length(gravity) * GRAVITY_EARTH * deltaTime * glm::normalize(gravity);
    }
    */
    float normalSpeed = glm::dot(velocity, glm::normalize(penetration));
    // NOTE: it is possible for normalSpeed to be NaN at this point 
    // (sometimes the average penetration of a bunch of voxels is a zero length vector which cannot be normalized) 
    // however the check below will fail (NaN comparisons always fail) and everything will be fine.

    if (normalSpeed > AUDIBLE_COLLISION_THRESHOLD) {
        //  Volume is proportional to collision velocity
        //  Base frequency is modified upward by the angle of the collision
        //  Noise is a function of the angle of collision
        //  Duration of the sound is a function of both base frequency and velocity of impact
        float tangentialSpeed = glm::length(velocity) - normalSpeed;
        _audio->startCollisionSound( std::min(COLLISION_LOUDNESS * normalSpeed, 1.f),
            frequency * (1.f + tangentialSpeed / normalSpeed),
            std::min(tangentialSpeed / normalSpeed * NOISE_SCALING, 1.f),
            1.f - DURATION_SCALING * powf(frequency, 0.5f) / normalSpeed, false);
    }
}
