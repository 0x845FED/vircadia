//
//  EntityItemProperties.h
//  libraries/entities/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityItemProperties_h
#define hifi_EntityItemProperties_h

#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/gtx/extented_min_max.hpp>

#include <QtScript/QScriptEngine>
#include <QtCore/QObject>
#include <QVector>
#include <QString>

#include <AACube.h>
#include <FBXReader.h> // for SittingPoint
#include <PropertyFlags.h>
#include <OctreeConstants.h>
#include <ShapeInfo.h>

#include "EntityItemID.h"
#include "EntityItemPropertiesMacros.h"
#include "EntityTypes.h"

enum EntityPropertyList {
    PROP_PAGED_PROPERTY,
    PROP_CUSTOM_PROPERTIES_INCLUDED,
    
    // these properties are supported by the EntityItem base class
    PROP_VISIBLE,
    PROP_POSITION,
    PROP_RADIUS, // NOTE: PROP_RADIUS is obsolete and only included in old format streams
    PROP_DIMENSIONS = PROP_RADIUS,
    PROP_ROTATION,
    PROP_DENSITY,
    PROP_VELOCITY,
    PROP_GRAVITY,
    PROP_DAMPING,
    PROP_LIFETIME,
    PROP_SCRIPT,

    // these properties are supported by some derived classes
    PROP_COLOR,
    PROP_MODEL_URL,
    PROP_ANIMATION_URL,
    PROP_ANIMATION_FPS,
    PROP_ANIMATION_FRAME_INDEX,
    PROP_ANIMATION_PLAYING,

    // these properties are supported by the EntityItem base class
    PROP_REGISTRATION_POINT,
    PROP_ANGULAR_VELOCITY,
    PROP_ANGULAR_DAMPING,
    PROP_IGNORE_FOR_COLLISIONS,
    PROP_COLLISIONS_WILL_MOVE,

    // property used by Light entity
    PROP_IS_SPOTLIGHT,
    PROP_DIFFUSE_COLOR_UNUSED,
    PROP_AMBIENT_COLOR_UNUSED,
    PROP_SPECULAR_COLOR_UNUSED,
    PROP_INTENSITY, // Previously PROP_CONSTANT_ATTENUATION
    PROP_LINEAR_ATTENUATION_UNUSED,
    PROP_QUADRATIC_ATTENUATION_UNUSED,
    PROP_EXPONENT,
    PROP_CUTOFF,

    // available to all entities
    PROP_LOCKED,
    
    // used by Model entities
    PROP_TEXTURES,
    PROP_ANIMATION_SETTINGS,
    PROP_USER_DATA,
    PROP_SHAPE_TYPE,

    // used by ParticleEffect entities
    PROP_MAX_PARTICLES,
    PROP_LIFESPAN,
    PROP_EMIT_RATE,
    PROP_EMIT_DIRECTION,
    PROP_EMIT_STRENGTH,
    PROP_LOCAL_GRAVITY,
    PROP_PARTICLE_RADIUS,
    
    PROP_COLLISION_MODEL_URL,
    PROP_MARKETPLACE_ID,
    PROP_ACCELERATION,
    PROP_SIMULATOR_ID,

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // ATTENTION: add new properties ABOVE this line
    PROP_AFTER_LAST_ITEM,
    ////////////////////////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////////////////////////
    // WARNING! Do not add props here unless you intentionally mean to reuse PROP_ indexes
    //
    // These properties of TextEntity piggy back off of properties of ModelEntities, the type doesn't matter
    // since the derived class knows how to interpret it's own properties and knows the types it expects
    PROP_TEXT_COLOR = PROP_COLOR,
    PROP_TEXT = PROP_MODEL_URL,
    PROP_LINE_HEIGHT = PROP_ANIMATION_URL,
    PROP_BACKGROUND_COLOR = PROP_ANIMATION_FPS,
    PROP_COLLISION_MODEL_URL_OLD_VERSION = PROP_ANIMATION_FPS + 1,

    // Aliases/Piggyback properties for Zones. These properties intentionally reuse the enum values for
    // other properties which will never overlap with each other. We do this so that we don't have to expand
    // the size of the properties bitflags mask
    PROP_KEYLIGHT_COLOR = PROP_COLOR,
    PROP_KEYLIGHT_INTENSITY = PROP_INTENSITY,
    PROP_KEYLIGHT_AMBIENT_INTENSITY = PROP_CUTOFF,
    PROP_KEYLIGHT_DIRECTION = PROP_EXPONENT,
    PROP_STAGE_SUN_MODEL_ENABLED = PROP_IS_SPOTLIGHT,
    PROP_STAGE_LATITUDE = PROP_DIFFUSE_COLOR_UNUSED,
    PROP_STAGE_LONGITUDE = PROP_AMBIENT_COLOR_UNUSED,
    PROP_STAGE_ALTITUDE = PROP_SPECULAR_COLOR_UNUSED,
    PROP_STAGE_DAY = PROP_LINEAR_ATTENUATION_UNUSED,
    PROP_STAGE_HOUR = PROP_QUADRATIC_ATTENUATION_UNUSED,

    // WARNING!!! DO NOT ADD PROPS_xxx here unless you really really meant to.... Add them UP above
};

typedef PropertyFlags<EntityPropertyList> EntityPropertyFlags;

// this is set at the top of EntityItemProperties.cpp to PROP_AFTER_LAST_ITEM - 1.  PROP_AFTER_LAST_ITEM is always
// one greater than the last item property due to the enum's auto-incrementing.
extern EntityPropertyList PROP_LAST_ITEM;

const quint64 UNKNOWN_CREATED_TIME = 0;

/// A collection of properties of an entity item used in the scripting API. Translates between the actual properties of an
/// entity and a JavaScript style hash/QScriptValue storing a set of properties. Used in scripting to set/get the complete
/// set of entity item properties via JavaScript hashes/QScriptValues
/// all units for position, dimensions, etc are in meter units
class EntityItemProperties {
    friend class EntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ModelEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class BoxEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class SphereEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class LightEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class TextEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ParticleEffectEntityItem; // TODO: consider removing this friend relationship and use public methods
    friend class ZoneEntityItem; // TODO: consider removing this friend relationship and use public methods
public:
    EntityItemProperties();
    virtual ~EntityItemProperties();

    EntityTypes::EntityType getType() const { return _type; }
    void setType(EntityTypes::EntityType type) { _type = type; }

    virtual QScriptValue copyToScriptValue(QScriptEngine* engine) const;
    virtual void copyFromScriptValue(const QScriptValue& object);

    // editing related features supported by all entities
    quint64 getLastEdited() const { return _lastEdited; }
    float getEditedAgo() const /// Elapsed seconds since this entity was last edited
        { return (float)(usecTimestampNow() - getLastEdited()) / (float)USECS_PER_SECOND; }
    EntityPropertyFlags getChangedProperties() const;

    /// used by EntityScriptingInterface to return EntityItemProperties for unknown models
    void setIsUnknownID() { _id = UNKNOWN_ENTITY_ID; _idSet = true; }
    
    AACube getMaximumAACube() const;
    AABox getAABox() const;

    void debugDump() const;
    void setLastEdited(quint64 usecTime);

    // Note:  DEFINE_PROPERTY(PROP_FOO, Foo, foo, type) creates the following methods and variables:
    // type getFoo() const;
    // void setFoo(type);
    // bool fooChanged() const;
    // type _foo;
    // bool _fooChanged;

    DEFINE_PROPERTY(PROP_VISIBLE, Visible, visible, bool);
    DEFINE_PROPERTY_REF_WITH_SETTER(PROP_POSITION, Position, position, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_DIMENSIONS, Dimensions, dimensions, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ROTATION, Rotation, rotation, glm::quat);
    DEFINE_PROPERTY(PROP_DENSITY, Density, density, float);
    DEFINE_PROPERTY_REF(PROP_VELOCITY, Velocity, velocity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_GRAVITY, Gravity, gravity, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ACCELERATION, Acceleration, acceleration, glm::vec3);
    DEFINE_PROPERTY(PROP_DAMPING, Damping, damping, float);
    DEFINE_PROPERTY(PROP_LIFETIME, Lifetime, lifetime, float);
    DEFINE_PROPERTY_REF(PROP_SCRIPT, Script, script, QString);
    DEFINE_PROPERTY_REF(PROP_COLOR, Color, color, xColor);
    DEFINE_PROPERTY_REF(PROP_MODEL_URL, ModelURL, modelURL, QString);
    DEFINE_PROPERTY_REF(PROP_COLLISION_MODEL_URL, CollisionModelURL, collisionModelURL, QString);
    DEFINE_PROPERTY_REF(PROP_ANIMATION_URL, AnimationURL, animationURL, QString);
    DEFINE_PROPERTY(PROP_ANIMATION_FPS, AnimationFPS, animationFPS, float);
    DEFINE_PROPERTY(PROP_ANIMATION_FRAME_INDEX, AnimationFrameIndex, animationFrameIndex, float);
    DEFINE_PROPERTY(PROP_ANIMATION_PLAYING, AnimationIsPlaying, animationIsPlaying, bool);
    DEFINE_PROPERTY_REF(PROP_REGISTRATION_POINT, RegistrationPoint, registrationPoint, glm::vec3);
    DEFINE_PROPERTY_REF(PROP_ANGULAR_VELOCITY, AngularVelocity, angularVelocity, glm::vec3);
    DEFINE_PROPERTY(PROP_ANGULAR_DAMPING, AngularDamping, angularDamping, float);
    DEFINE_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, IgnoreForCollisions, ignoreForCollisions, bool);
    DEFINE_PROPERTY(PROP_COLLISIONS_WILL_MOVE, CollisionsWillMove, collisionsWillMove, bool);
    DEFINE_PROPERTY(PROP_IS_SPOTLIGHT, IsSpotlight, isSpotlight, bool);
    DEFINE_PROPERTY(PROP_INTENSITY, Intensity, intensity, float);
    DEFINE_PROPERTY(PROP_EXPONENT, Exponent, exponent, float);
    DEFINE_PROPERTY(PROP_CUTOFF, Cutoff, cutoff, float);
    DEFINE_PROPERTY(PROP_LOCKED, Locked, locked, bool);
    DEFINE_PROPERTY_REF(PROP_TEXTURES, Textures, textures, QString);
    DEFINE_PROPERTY_REF_WITH_SETTER_AND_GETTER(PROP_ANIMATION_SETTINGS, AnimationSettings, animationSettings, QString);
    DEFINE_PROPERTY_REF(PROP_USER_DATA, UserData, userData, QString);
    DEFINE_PROPERTY_REF(PROP_SIMULATOR_ID, SimulatorID, simulatorID, QUuid);
    DEFINE_PROPERTY_REF(PROP_TEXT, Text, text, QString);
    DEFINE_PROPERTY(PROP_LINE_HEIGHT, LineHeight, lineHeight, float);
    DEFINE_PROPERTY_REF(PROP_TEXT_COLOR, TextColor, textColor, xColor);
    DEFINE_PROPERTY_REF(PROP_BACKGROUND_COLOR, BackgroundColor, backgroundColor, xColor);
    DEFINE_PROPERTY_REF_ENUM(PROP_SHAPE_TYPE, ShapeType, shapeType, ShapeType);
    DEFINE_PROPERTY(PROP_MAX_PARTICLES, MaxParticles, maxParticles, quint32);
    DEFINE_PROPERTY(PROP_LIFESPAN, Lifespan, lifespan, float);
    DEFINE_PROPERTY(PROP_EMIT_RATE, EmitRate, emitRate, float);
    DEFINE_PROPERTY_REF(PROP_EMIT_DIRECTION, EmitDirection, emitDirection, glm::vec3);
    DEFINE_PROPERTY(PROP_EMIT_STRENGTH, EmitStrength, emitStrength, float);
    DEFINE_PROPERTY(PROP_LOCAL_GRAVITY, LocalGravity, localGravity, float);
    DEFINE_PROPERTY(PROP_PARTICLE_RADIUS, ParticleRadius, particleRadius, float);
    DEFINE_PROPERTY_REF(PROP_MARKETPLACE_ID, MarketplaceID, marketplaceID, QString);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_COLOR, KeyLightColor, keyLightColor, xColor);
    DEFINE_PROPERTY(PROP_KEYLIGHT_INTENSITY, KeyLightIntensity, keyLightIntensity, float);
    DEFINE_PROPERTY(PROP_KEYLIGHT_AMBIENT_INTENSITY, KeyLightAmbientIntensity, keyLightAmbientIntensity, float);
    DEFINE_PROPERTY_REF(PROP_KEYLIGHT_DIRECTION, KeyLightDirection, keyLightDirection, glm::vec3);
    DEFINE_PROPERTY(PROP_STAGE_SUN_MODEL_ENABLED, StageSunModelEnabled, stageSunModelEnabled, bool);
    DEFINE_PROPERTY(PROP_STAGE_LATITUDE, StageLatitude, stageLatitude, float);
    DEFINE_PROPERTY(PROP_STAGE_LONGITUDE, StageLongitude, stageLongitude, float);
    DEFINE_PROPERTY(PROP_STAGE_ALTITUDE, StageAltitude, stageAltitude, float);
    DEFINE_PROPERTY(PROP_STAGE_DAY, StageDay, stageDay, quint16);
    DEFINE_PROPERTY(PROP_STAGE_HOUR, StageHour, stageHour, float);


public:
    float getMaxDimension() const { return glm::max(_dimensions.x, _dimensions.y, _dimensions.z); }

    float getAge() const { return (float)(usecTimestampNow() - _created) / (float)USECS_PER_SECOND; }
    quint64 getCreated() const { return _created; }
    void setCreated(quint64 usecTime);
    bool hasCreatedTime() const { return (_created != UNKNOWN_CREATED_TIME); }

    bool containsBoundsProperties() const { return (_positionChanged || _dimensionsChanged); }
    bool containsPositionChange() const { return _positionChanged; }
    bool containsDimensionsChange() const { return _dimensionsChanged; }
    bool containsAnimationSettingsChange() const { return _animationSettingsChanged; }

    float getGlowLevel() const { return _glowLevel; }
    float getLocalRenderAlpha() const { return _localRenderAlpha; }
    void setGlowLevel(float value) { _glowLevel = value; _glowLevelChanged = true; }
    void setLocalRenderAlpha(float value) { _localRenderAlpha = value; _localRenderAlphaChanged = true; }

    static bool encodeEntityEditPacket(PacketType command, EntityItemID id, const EntityItemProperties& properties,
        unsigned char* bufferOut, int sizeIn, int& sizeOut);

    static bool encodeEraseEntityMessage(const EntityItemID& entityItemID, 
                                            unsigned char* outputBuffer, size_t maxLength, size_t& outputLength);


    static bool decodeEntityEditPacket(const unsigned char* data, int bytesToRead, int& processedBytes,
                        EntityItemID& entityID, EntityItemProperties& properties);

    bool glowLevelChanged() const { return _glowLevelChanged; }
    bool localRenderAlphaChanged() const { return _localRenderAlphaChanged; }

    void clearID() { _id = UNKNOWN_ENTITY_ID; _idSet = false; }
    void markAllChanged();

    void setSittingPoints(const QVector<SittingPoint>& sittingPoints);

    const glm::vec3& getNaturalDimensions() const { return _naturalDimensions; }
    void setNaturalDimensions(const glm::vec3& value) { _naturalDimensions = value; }

    const QStringList& getTextureNames() const { return _textureNames; }
    void setTextureNames(const QStringList& value) { _textureNames = value; }

    QString getSimulatorIDAsString() const { return _simulatorID.toString().mid(1,36).toUpper(); }

private:
    QUuid _id;
    bool _idSet;
    quint64 _lastEdited;
    quint64 _created;
    EntityTypes::EntityType _type;
    void setType(const QString& typeName) { _type = EntityTypes::getEntityTypeFromName(typeName); }

    float _glowLevel;
    float _localRenderAlpha;
    bool _glowLevelChanged;
    bool _localRenderAlphaChanged;
    bool _defaultSettings;

    // NOTE: The following are pseudo client only properties. They are only used in clients which can access
    // properties of model geometry. But these properties are not serialized like other properties.
    QVector<SittingPoint> _sittingPoints;
    QStringList _textureNames;
    glm::vec3 _naturalDimensions;
};
Q_DECLARE_METATYPE(EntityItemProperties);
QScriptValue EntityItemPropertiesToScriptValue(QScriptEngine* engine, const EntityItemProperties& properties);
void EntityItemPropertiesFromScriptValue(const QScriptValue &object, EntityItemProperties& properties);


// define these inline here so the macros work
inline void EntityItemProperties::setPosition(const glm::vec3& value) 
                    { _position = glm::clamp(value, 0.0f, (float)TREE_SCALE); _positionChanged = true; }


inline QDebug operator<<(QDebug debug, const EntityItemProperties& properties) {
    debug << "EntityItemProperties[" << "\n";
    
    debug << "    _type:" << properties.getType() << "\n";

    // TODO: figure out why position and animationSettings don't seem to like the macro approach
    if (properties.containsPositionChange()) {
        debug << "  position:" << properties.getPosition() << "in meters" << "\n";
    }
    if (properties.containsAnimationSettingsChange()) {
        debug << "  animationSettings:" << properties.getAnimationSettings() << "\n";
    }

    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Dimensions, dimensions, "in meters");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Velocity, velocity, "in meters");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Visible, visible, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Rotation, rotation, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Density, density, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Gravity, gravity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Acceleration, acceleration, "in meters per second");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Damping, damping, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Lifetime, lifetime, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Script, script, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Color, color, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ModelURL, modelURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CollisionModelURL, collisionModelURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationURL, animationURL, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationFPS, animationFPS, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationFrameIndex, animationFrameIndex, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AnimationIsPlaying, animationIsPlaying, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, RegistrationPoint, registrationPoint, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularVelocity, angularVelocity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, AngularDamping, angularDamping, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IgnoreForCollisions, ignoreForCollisions, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, CollisionsWillMove, collisionsWillMove, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, IsSpotlight, isSpotlight, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Intensity, intensity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Exponent, exponent, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Cutoff, cutoff, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Locked, locked, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Textures, textures, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, UserData, userData, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, SimulatorID, simulatorID, QUuid());
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Text, text, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, LineHeight, lineHeight, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, TextColor, textColor, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, BackgroundColor, backgroundColor, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ShapeType, shapeType, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, MaxParticles, maxParticles, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, Lifespan, lifespan, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitRate, emitRate, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitDirection, emitDirection, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, EmitStrength, emitStrength, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, LocalGravity, localGravity, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, ParticleRadius, particleRadius, "");
    DEBUG_PROPERTY_IF_CHANGED(debug, properties, MarketplaceID, marketplaceID, "");

    debug << "  last edited:" << properties.getLastEdited() << "\n";
    debug << "  edited ago:" << properties.getEditedAgo() << "\n";
    debug << "]";

    return debug;
}

#endif // hifi_EntityItemProperties_h
