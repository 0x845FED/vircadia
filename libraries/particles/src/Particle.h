//
//  Particle.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//

#ifndef __hifi__Particle__
#define __hifi__Particle__

#include <glm/glm.hpp>
#include <stdint.h>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>

#include <SharedUtil.h>
#include <OctreePacketData.h>

class VoxelsScriptingInterface;
class ParticlesScriptingInterface;
class VoxelEditPacketSender;
class ParticleEditPacketSender;
class ParticleProperties;
class Particle;
class ParticleTree;

const uint32_t NEW_PARTICLE = 0xFFFFFFFF;
const uint32_t UNKNOWN_TOKEN = 0xFFFFFFFF;
const uint32_t UNKNOWN_PARTICLE_ID = 0xFFFFFFFF;

const uint16_t PACKET_CONTAINS_RADIUS = 1;
const uint16_t PACKET_CONTAINS_POSITION = 2;
const uint16_t PACKET_CONTAINS_COLOR = 4;
const uint16_t PACKET_CONTAINS_VELOCITY = 8;
const uint16_t PACKET_CONTAINS_GRAVITY = 16;
const uint16_t PACKET_CONTAINS_DAMPING = 32;
const uint16_t PACKET_CONTAINS_LIFETIME = 64;
const uint16_t PACKET_CONTAINS_INHAND = 128;
const uint16_t PACKET_CONTAINS_SCRIPT = 256;
const uint16_t PACKET_CONTAINS_SHOULDDIE = 512;

const float DEFAULT_LIFETIME = 10.0f; // particles live for 10 seconds by default
const float DEFAULT_DAMPING = 0.99f;
const float DEFAULT_RADIUS = 0.1f / TREE_SCALE;
const float MINIMUM_PARTICLE_ELEMENT_SIZE = (1.0f / 100000.0f) / TREE_SCALE; // smallest size container
const glm::vec3 DEFAULT_GRAVITY(0, (-9.8f / TREE_SCALE), 0);
const QString DEFAULT_SCRIPT("");
const bool IN_HAND = true; // it's in a hand
const bool NOT_IN_HAND = !IN_HAND; // it's not in a hand

/// A collection of properties of a particle used in the scripting API. Translates between the actual properties of a particle
/// and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete set of
/// particle properties via JavaScript hashes/QScriptValues
/// all units for position, velocity, gravity, radius, etc are in meter units
class ParticleProperties {
public:
    ParticleProperties();

    QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    void copyFromScriptValue(const QScriptValue& object);

    void copyToParticle(Particle& particle) const;
    void copyFromParticle(const Particle& particle);

    const glm::vec3& getPosition() const { return _position; }
    xColor getColor() const { return _color; }
    float getRadius() const { return _radius; }
    const glm::vec3& getVelocity() const { return _velocity; }
    const glm::vec3& getGravity() const { return _gravity; }
    float getDamping() const { return _damping; }
    float getLifetime() const { return _lifetime; }
    const QString& getScript() const { return _script; }
    bool getInHand() const { return _inHand; }
    bool getShouldDie() const { return _shouldDie; }

    uint64_t getLastEdited() const { return _lastEdited; }
    uint16_t getChangedBits() const;

    /// set position in meter units
    void setPosition(const glm::vec3& value) { _position = value; _positionChanged = true; }

    /// set velocity in meter units
    void setVelocity(const glm::vec3& value) { _velocity = value; _velocityChanged = true;  }
    void setColor(const xColor& value) { _color = value; _colorChanged = true; }
    void setRadius(float value) { _radius = value; _radiusChanged = true; }

    /// set gravity in meter units
    void setGravity(const glm::vec3& value) { _gravity = value; _gravityChanged = true;  }
    void setInHand(bool inHand) { _inHand = inHand; _inHandChanged = true; }
    void setDamping(float value) { _damping = value; _dampingChanged = true;  }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; _shouldDieChanged = true;  }
    void setLifetime(float value) { _lifetime = value; _lifetimeChanged = true;  }
    void setScript(QString updateScript) { _script = updateScript; _scriptChanged = true;  }

private:
    glm::vec3 _position;
    xColor _color;
    float _radius;
    glm::vec3 _velocity;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    bool _inHand;
    bool _shouldDie;

    uint64_t _lastEdited;
    bool _positionChanged;
    bool _colorChanged;
    bool _radiusChanged;
    bool _velocityChanged;
    bool _gravityChanged;
    bool _dampingChanged;
    bool _lifetimeChanged;
    bool _scriptChanged;
    bool _inHandChanged;
    bool _shouldDieChanged;
    bool _defaultSettings;
};
Q_DECLARE_METATYPE(ParticleProperties);
QScriptValue ParticlePropertiesToScriptValue(QScriptEngine* engine, const ParticleProperties& properties);
void ParticlePropertiesFromScriptValue(const QScriptValue &object, ParticleProperties& properties);


/// Abstract ID for editing particles. Used in Particle JS API - When particles are created in the JS api, they are given a
/// local creatorTokenID, the actual id for the particle is not known until the server responds to the creator with the
/// correct mapping. This class works with the scripting API an allows the developer to edit particles they created.
class ParticleID {
public:
    ParticleID() :
            id(NEW_PARTICLE), creatorTokenID(UNKNOWN_TOKEN), isKnownID(false) { };

    ParticleID(uint32_t id, uint32_t creatorTokenID, bool isKnownID) :
            id(id), creatorTokenID(creatorTokenID), isKnownID(isKnownID) { };

    ParticleID(uint32_t id) :
            id(id), creatorTokenID(UNKNOWN_TOKEN), isKnownID(true) { };

    uint32_t id;
    uint32_t creatorTokenID;
    bool isKnownID;
};

Q_DECLARE_METATYPE(ParticleID);
QScriptValue ParticleIDtoScriptValue(QScriptEngine* engine, const ParticleID& properties);
void ParticleIDfromScriptValue(const QScriptValue &object, ParticleID& properties);



/// Particle class - this is the actual particle class.
class Particle  {

public:
    Particle();
    
    /// all position, velocity, gravity, radius units are in domain units (0.0 to 1.0)
    Particle(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity,
            glm::vec3 gravity = DEFAULT_GRAVITY, float damping = DEFAULT_DAMPING, float lifetime = DEFAULT_LIFETIME,
            bool inHand = NOT_IN_HAND, QString updateScript = DEFAULT_SCRIPT, uint32_t id = NEW_PARTICLE);

    /// creates an NEW particle from an PACKET_TYPE_PARTICLE_ADD_OR_EDIT edit data buffer
    static Particle fromEditPacket(unsigned char* data, int length, int& processedBytes, ParticleTree* tree);

    virtual ~Particle();
    virtual void init(glm::vec3 position, float radius, rgbColor color, glm::vec3 velocity,
            glm::vec3 gravity = DEFAULT_GRAVITY, float damping = DEFAULT_DAMPING, float lifetime = DEFAULT_LIFETIME,
            bool inHand = NOT_IN_HAND, QString updateScript = DEFAULT_SCRIPT, uint32_t id = NEW_PARTICLE);

    /// get position in domain scale units (0.0 - 1.0)
    const glm::vec3& getPosition() const { return _position; }

    const rgbColor& getColor() const { return _color; }
    xColor getXColor() const { xColor color = { _color[RED_INDEX], _color[GREEN_INDEX], _color[BLUE_INDEX] }; return color; }

    /// get radius in domain scale units (0.0 - 1.0)
    float getRadius() const { return _radius; }
    float getMass() const { return _mass; }

    /// get velocity in domain scale units (0.0 - 1.0)
    const glm::vec3& getVelocity() const { return _velocity; }

    /// get gravity in domain scale units (0.0 - 1.0)
    const glm::vec3& getGravity() const { return _gravity; }

    bool getInHand() const { return _inHand; }
    float getDamping() const { return _damping; }
    float getLifetime() const { return _lifetime; }
    ParticleProperties getProperties() const;

    /// The last updated/simulated time of this particle from the time perspective of the authoritative server/source
    uint64_t getLastUpdated() const { return _lastUpdated; }

    /// The last edited time of this particle from the time perspective of the authoritative server/source
    uint64_t getLastEdited() const { return _lastEdited; }

    /// lifetime of the particle in seconds
    float getAge() const { return static_cast<float>(usecTimestampNow() - _created) / static_cast<float>(USECS_PER_SECOND); }
    float getEditedAgo() const { return static_cast<float>(usecTimestampNow() - _lastEdited) / static_cast<float>(USECS_PER_SECOND); }
    uint32_t getID() const { return _id; }
    bool getShouldDie() const { return _shouldDie; }
    QString getScript() const { return _script; }
    uint32_t getCreatorTokenID() const { return _creatorTokenID; }
    bool isNewlyCreated() const { return _newlyCreated; }

    /// set position in domain scale units (0.0 - 1.0)
    void setPosition(const glm::vec3& value) { _position = value; }

    /// set velocity in domain scale units (0.0 - 1.0)
    void setVelocity(const glm::vec3& value) { _velocity = value; }
    void setColor(const rgbColor& value) { memcpy(_color, value, sizeof(_color)); }
    void setColor(const xColor& value) {
            _color[RED_INDEX] = value.red;
            _color[GREEN_INDEX] = value.green;
            _color[BLUE_INDEX] = value.blue;
    }
    /// set radius in domain scale units (0.0 - 1.0)
    void setRadius(float value) { _radius = value; }
    void setMass(float value);

    /// set gravity in domain scale units (0.0 - 1.0)
    void setGravity(const glm::vec3& value) { _gravity = value; }
    void setInHand(bool inHand) { _inHand = inHand; }
    void setDamping(float value) { _damping = value; }
    void setShouldDie(bool shouldDie) { _shouldDie = shouldDie; }
    void setLifetime(float value) { _lifetime = value; }
    void setScript(QString updateScript) { _script = updateScript; }
    void setCreatorTokenID(uint32_t creatorTokenID) { _creatorTokenID = creatorTokenID; }
    void setProperties(const ParticleProperties& properties);

    bool appendParticleData(OctreePacketData* packetData) const;
    int readParticleDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args);
    static int expectedBytes();
    static int expectedEditMessageBytes();

    static bool encodeParticleEditMessageDetails(PACKET_TYPE command, ParticleID id, const ParticleProperties& details,
                        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static void adjustEditPacketForClockSkew(unsigned char* codeColorBuffer, ssize_t length, int clockSkew);

    void update(const uint64_t& now);
    void collisionWithParticle(Particle* other);
    void collisionWithVoxel(VoxelDetail* voxel);

    void debugDump() const;

    // similar to assignment/copy, but it handles keeping lifetime accurate
    void copyChangedProperties(const Particle& other);

    static VoxelEditPacketSender* getVoxelEditPacketSender() { return _voxelEditSender; }
    static ParticleEditPacketSender* getParticleEditPacketSender() { return _particleEditSender; }

    static void setVoxelEditPacketSender(VoxelEditPacketSender* senderInterface)
                    { _voxelEditSender = senderInterface; }

    static void setParticleEditPacketSender(ParticleEditPacketSender* senderInterface)
                    { _particleEditSender = senderInterface; }


    // these methods allow you to create particles, and later edit them.
    static uint32_t getIDfromCreatorTokenID(uint32_t creatorTokenID);
    static uint32_t getNextCreatorTokenID();
    static void handleAddParticleResponse(unsigned char* packetData , int packetLength);

protected:
    static VoxelEditPacketSender* _voxelEditSender;
    static ParticleEditPacketSender* _particleEditSender;

    void runUpdateScript();
    static QScriptValue vec3toScriptValue(QScriptEngine *engine, const glm::vec3 &vec3);
    static void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);
    static QScriptValue xColorToScriptValue(QScriptEngine *engine, const xColor& color);
    static void xColorFromScriptValue(const QScriptValue &object, xColor& color);

    void setAge(float age);

    glm::vec3 _position;
    rgbColor _color;
    float _radius;
    float _mass;
    glm::vec3 _velocity;
    uint32_t _id;
    static uint32_t _nextID;
    bool _shouldDie;
    glm::vec3 _gravity;
    float _damping;
    float _lifetime;
    QString _script;
    bool _inHand;

    uint32_t _creatorTokenID;
    bool _newlyCreated;

    uint64_t _lastUpdated;
    uint64_t _lastEdited;

    // this doesn't go on the wire, we send it as lifetime
    uint64_t _created;

    // used by the static interfaces for creator token ids
    static uint32_t _nextCreatorTokenID;
    static std::map<uint32_t,uint32_t> _tokenIDsToIDs;
};

/// Scriptable interface to a single Particle object. Used exclusively in the JavaScript API for interacting with single
/// Particles.
class ParticleScriptObject  : public QObject {
    Q_OBJECT
public:
    ParticleScriptObject(Particle* particle) { _particle = particle; }

    void emitUpdate() { emit update(); }
    void emitCollisionWithParticle(QObject* other) { emit collisionWithParticle(other); }
    void emitCollisionWithVoxel(QObject* voxel) { emit collisionWithVoxel(voxel); }

public slots:
    unsigned int getID() const { return _particle->getID(); }
    
    /// get position in meter units
    glm::vec3 getPosition() const { return _particle->getPosition() * (float)TREE_SCALE; }

    /// get velocity in meter units
    glm::vec3 getVelocity() const { return _particle->getVelocity() * (float)TREE_SCALE; }
    xColor getColor() const { return _particle->getXColor(); }

    /// get gravity in meter units
    glm::vec3 getGravity() const { return _particle->getGravity() * (float)TREE_SCALE; }

    float getDamping() const { return _particle->getDamping(); }

    /// get radius in meter units
    float getRadius() const { return _particle->getRadius() * (float)TREE_SCALE; }
    bool getShouldDie() { return _particle->getShouldDie(); }
    float getAge() const { return _particle->getAge(); }
    float getLifetime() const { return _particle->getLifetime(); }
    ParticleProperties getProperties() const { return _particle->getProperties(); }

    /// set position in meter units
    void setPosition(glm::vec3 value) { _particle->setPosition(value / (float)TREE_SCALE); }

    /// set velocity in meter units
    void setVelocity(glm::vec3 value) { _particle->setVelocity(value / (float)TREE_SCALE); }

    /// set gravity in meter units
    void setGravity(glm::vec3 value) { _particle->setGravity(value / (float)TREE_SCALE); }
    
    void setDamping(float value) { _particle->setDamping(value); }
    void setColor(xColor value) { _particle->setColor(value); }

    /// set radius in meter units
    void setRadius(float value) { _particle->setRadius(value / (float)TREE_SCALE); }
    void setShouldDie(bool value) { _particle->setShouldDie(value); }
    void setScript(const QString& script) { _particle->setScript(script); }
    void setLifetime(float value) const { return _particle->setLifetime(value); }
    void setProperties(const ParticleProperties& properties) { return _particle->setProperties(properties); }

signals:
    void update();
    void collisionWithVoxel(QObject* voxel);
    void collisionWithParticle(QObject* other);

private:
    Particle* _particle;
};



#endif /* defined(__hifi__Particle__) */
