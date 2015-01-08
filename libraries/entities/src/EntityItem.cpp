//
//  EntityItem.cpp
//  libraries/models/src
//
//  Created by Brad Hefta-Gaub on 12/4/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QObject>

#include <ByteCountCoding.h>
#include <GLMHelpers.h>
#include <Octree.h>
#include <RegisteredMetaTypes.h>
#include <SharedUtil.h> // usecTimestampNow()

#include "EntityScriptingInterface.h"
#include "EntityItem.h"
#include "EntityTree.h"

const float EntityItem::IMMORTAL = -1.0f; /// special lifetime which means the entity lives for ever. default lifetime
const float EntityItem::DEFAULT_GLOW_LEVEL = 0.0f;
const float EntityItem::DEFAULT_LOCAL_RENDER_ALPHA = 1.0f;
const float EntityItem::DEFAULT_MASS = 1.0f;
const float EntityItem::DEFAULT_LIFETIME = EntityItem::IMMORTAL;
const QString EntityItem::DEFAULT_USER_DATA = QString("");
const float EntityItem::DEFAULT_DAMPING = 0.39347f;  // approx timescale = 2.0 sec (see damping timescale formula in header)
const glm::vec3 EntityItem::NO_VELOCITY = glm::vec3(0, 0, 0);
const float EntityItem::EPSILON_VELOCITY_LENGTH = (1.0f / 1000.0f) / (float)TREE_SCALE; // really small: 1mm/second
const glm::vec3 EntityItem::DEFAULT_VELOCITY = EntityItem::NO_VELOCITY;
const glm::vec3 EntityItem::NO_GRAVITY = glm::vec3(0, 0, 0);
const glm::vec3 EntityItem::REGULAR_GRAVITY = glm::vec3(0, (-9.8f / TREE_SCALE), 0);
const glm::vec3 EntityItem::DEFAULT_GRAVITY = EntityItem::NO_GRAVITY;
const QString EntityItem::DEFAULT_SCRIPT = QString("");
const glm::quat EntityItem::DEFAULT_ROTATION;
const glm::vec3 EntityItem::DEFAULT_DIMENSIONS = glm::vec3(0.1f, 0.1f, 0.1f);
const glm::vec3 EntityItem::DEFAULT_REGISTRATION_POINT = glm::vec3(0.5f, 0.5f, 0.5f); // center
const glm::vec3 EntityItem::NO_ANGULAR_VELOCITY = glm::vec3(0.0f, 0.0f, 0.0f);
const glm::vec3 EntityItem::DEFAULT_ANGULAR_VELOCITY = NO_ANGULAR_VELOCITY;
const float EntityItem::DEFAULT_ANGULAR_DAMPING = 2.0f;
const bool EntityItem::DEFAULT_VISIBLE = true;
const bool EntityItem::DEFAULT_IGNORE_FOR_COLLISIONS = false;
const bool EntityItem::DEFAULT_COLLISIONS_WILL_MOVE = false;
const bool EntityItem::DEFAULT_LOCKED = false;

void EntityItem::initFromEntityItemID(const EntityItemID& entityItemID) {
    _id = entityItemID.id;
    _creatorTokenID = entityItemID.creatorTokenID;

    // init values with defaults before calling setProperties
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _created = UNKNOWN_CREATED_TIME;
    _changedOnServer = 0;

    _position = glm::vec3(0,0,0);
    _dimensions = DEFAULT_DIMENSIONS;
    _rotation = DEFAULT_ROTATION;
    _glowLevel = DEFAULT_GLOW_LEVEL;
    _localRenderAlpha = DEFAULT_LOCAL_RENDER_ALPHA;
    _mass = DEFAULT_MASS;
    _velocity = DEFAULT_VELOCITY;
    _gravity = DEFAULT_GRAVITY;
    _damping = DEFAULT_DAMPING;
    _lifetime = DEFAULT_LIFETIME;
    _script = DEFAULT_SCRIPT;
    _registrationPoint = DEFAULT_REGISTRATION_POINT;
    _angularVelocity = DEFAULT_ANGULAR_VELOCITY;
    _angularDamping = DEFAULT_ANGULAR_DAMPING;
    _visible = DEFAULT_VISIBLE;
    _ignoreForCollisions = DEFAULT_IGNORE_FOR_COLLISIONS;
    _collisionsWillMove = DEFAULT_COLLISIONS_WILL_MOVE;
    _locked = DEFAULT_LOCKED;
    _userData = DEFAULT_USER_DATA;
    
    recalculateCollisionShape();
}

EntityItem::EntityItem(const EntityItemID& entityItemID) {
    _type = EntityTypes::Unknown;
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _created = UNKNOWN_CREATED_TIME;
    _physicsInfo = NULL;
    _dirtyFlags = 0;
    _changedOnServer = 0;
    initFromEntityItemID(entityItemID);
}

EntityItem::EntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) {
    _type = EntityTypes::Unknown;
    quint64 now = usecTimestampNow();
    _lastSimulated = now;
    _lastUpdated = now;
    _lastEdited = 0;
    _lastEditedFromRemote = 0;
    _lastEditedFromRemoteInRemoteTime = 0;
    _created = UNKNOWN_CREATED_TIME;
    _physicsInfo = NULL;
    _dirtyFlags = 0;
    _changedOnServer = 0;
    initFromEntityItemID(entityItemID);
    setProperties(properties);
}

EntityItem::~EntityItem() {
    // be sure to clean up _physicsInfo before calling this dtor
    assert(_physicsInfo == NULL);
}

EntityPropertyFlags EntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_POSITION;
    requestedProperties += PROP_DIMENSIONS; // NOTE: PROP_RADIUS obsolete
    requestedProperties += PROP_ROTATION;
    requestedProperties += PROP_MASS;
    requestedProperties += PROP_VELOCITY;
    requestedProperties += PROP_GRAVITY;
    requestedProperties += PROP_DAMPING;
    requestedProperties += PROP_LIFETIME;
    requestedProperties += PROP_SCRIPT;
    requestedProperties += PROP_REGISTRATION_POINT;
    requestedProperties += PROP_ANGULAR_VELOCITY;
    requestedProperties += PROP_ANGULAR_DAMPING;
    requestedProperties += PROP_VISIBLE;
    requestedProperties += PROP_IGNORE_FOR_COLLISIONS;
    requestedProperties += PROP_COLLISIONS_WILL_MOVE;
    requestedProperties += PROP_LOCKED;
    requestedProperties += PROP_USER_DATA;
    
    return requestedProperties;
}

OctreeElement::AppendState EntityItem::appendEntityData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                            EntityTreeElementExtraEncodeData* entityTreeElementExtraEncodeData) const {
    // ALL this fits...
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    
    OctreeElement::AppendState appendState = OctreeElement::COMPLETED; // assume the best

    // encode our ID as a byte count coded byte stream
    QByteArray encodedID = getID().toRfc4122();

    // encode our type as a byte count coded byte stream
    ByteCountCoded<quint32> typeCoder = getType();
    QByteArray encodedType = typeCoder;

    quint64 updateDelta = getLastSimulated() <= getLastEdited() ? 0 : getLastSimulated() - getLastEdited();
    ByteCountCoded<quint64> updateDeltaCoder = updateDelta;
    QByteArray encodedUpdateDelta = updateDeltaCoder;
    EntityPropertyFlags propertyFlags(PROP_LAST_ITEM);
    EntityPropertyFlags requestedProperties = getEntityProperties(params);
    EntityPropertyFlags propertiesDidntFit = requestedProperties;

    // If we are being called for a subsequent pass at appendEntityData() that failed to completely encode this item,
    // then our entityTreeElementExtraEncodeData should include data about which properties we need to append.
    if (entityTreeElementExtraEncodeData && entityTreeElementExtraEncodeData->entities.contains(getEntityItemID())) {
        requestedProperties = entityTreeElementExtraEncodeData->entities.value(getEntityItemID());
    }

    LevelDetails entityLevel = packetData->startLevel();

    quint64 lastEdited = getLastEdited();

    const bool wantDebug = false;
    if (wantDebug) {
        float editedAgo = getEditedAgo();
        QString agoAsString = formatSecondsElapsed(editedAgo);
        qDebug() << "Writing entity " << getEntityItemID() << " to buffer, lastEdited =" << lastEdited 
                        << " ago=" << editedAgo << "seconds - " << agoAsString;
    }

    bool successIDFits = false;
    bool successTypeFits = false;
    bool successCreatedFits = false;
    bool successLastEditedFits = false;
    bool successLastUpdatedFits = false;
    bool successPropertyFlagsFits = false;
    int propertyFlagsOffset = 0;
    int oldPropertyFlagsLength = 0;
    QByteArray encodedPropertyFlags;
    int propertyCount = 0;

    successIDFits = packetData->appendValue(encodedID);
    if (successIDFits) {
        successTypeFits = packetData->appendValue(encodedType);
    }
    if (successTypeFits) {
        successCreatedFits = packetData->appendValue(_created);
    }
    if (successCreatedFits) {
        successLastEditedFits = packetData->appendValue(lastEdited);
    }
    if (successLastEditedFits) {
        successLastUpdatedFits = packetData->appendValue(encodedUpdateDelta);
    }
    
    if (successLastUpdatedFits) {
        propertyFlagsOffset = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        oldPropertyFlagsLength = encodedPropertyFlags.length();
        successPropertyFlagsFits = packetData->appendValue(encodedPropertyFlags);
    }

    bool headerFits = successIDFits && successTypeFits && successCreatedFits && successLastEditedFits 
                              && successLastUpdatedFits && successPropertyFlagsFits;

    int startOfEntityItemData = packetData->getUncompressedByteOffset();

    if (headerFits) {
        bool successPropertyFits;

        propertyFlags -= PROP_LAST_ITEM; // clear the last item for now, we may or may not set it as the actual item

        // These items would go here once supported....
        //      PROP_PAGED_PROPERTY,
        //      PROP_CUSTOM_PROPERTIES_INCLUDED,

        APPEND_ENTITY_PROPERTY(PROP_POSITION, appendPosition, getPosition());
        APPEND_ENTITY_PROPERTY(PROP_DIMENSIONS, appendValue, getDimensions()); // NOTE: PROP_RADIUS obsolete

        if (wantDebug) {
            qDebug() << "    APPEND_ENTITY_PROPERTY() PROP_DIMENSIONS:" << getDimensions();
        }

        APPEND_ENTITY_PROPERTY(PROP_ROTATION, appendValue, getRotation());
        APPEND_ENTITY_PROPERTY(PROP_MASS, appendValue, getMass());
        APPEND_ENTITY_PROPERTY(PROP_VELOCITY, appendValue, getVelocity());
        APPEND_ENTITY_PROPERTY(PROP_GRAVITY, appendValue, getGravity());
        APPEND_ENTITY_PROPERTY(PROP_DAMPING, appendValue, getDamping());
        APPEND_ENTITY_PROPERTY(PROP_LIFETIME, appendValue, getLifetime());
        APPEND_ENTITY_PROPERTY(PROP_SCRIPT, appendValue, getScript());
        APPEND_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, appendValue, getRegistrationPoint());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_VELOCITY, appendValue, getAngularVelocity());
        APPEND_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, appendValue, getAngularDamping());
        APPEND_ENTITY_PROPERTY(PROP_VISIBLE, appendValue, getVisible());
        APPEND_ENTITY_PROPERTY(PROP_IGNORE_FOR_COLLISIONS, appendValue, getIgnoreForCollisions());
        APPEND_ENTITY_PROPERTY(PROP_COLLISIONS_WILL_MOVE, appendValue, getCollisionsWillMove());
        APPEND_ENTITY_PROPERTY(PROP_LOCKED, appendValue, getLocked());
        APPEND_ENTITY_PROPERTY(PROP_USER_DATA, appendValue, getUserData());

        appendSubclassData(packetData, params, entityTreeElementExtraEncodeData,
                                requestedProperties,
                                propertyFlags,
                                propertiesDidntFit,
                                propertyCount,
                                appendState);
    }

    if (propertyCount > 0) {
        int endOfEntityItemData = packetData->getUncompressedByteOffset();
        encodedPropertyFlags = propertyFlags;
        int newPropertyFlagsLength = encodedPropertyFlags.length();
        packetData->updatePriorBytes(propertyFlagsOffset, 
                (const unsigned char*)encodedPropertyFlags.constData(), encodedPropertyFlags.length());
        
        // if the size of the PropertyFlags shrunk, we need to shift everything down to front of packet.
        if (newPropertyFlagsLength < oldPropertyFlagsLength) {
            int oldSize = packetData->getUncompressedSize();
            const unsigned char* modelItemData = packetData->getUncompressedData(propertyFlagsOffset + oldPropertyFlagsLength);
            int modelItemDataLength = endOfEntityItemData - startOfEntityItemData;
            int newEntityItemDataStart = propertyFlagsOffset + newPropertyFlagsLength;
            packetData->updatePriorBytes(newEntityItemDataStart, modelItemData, modelItemDataLength);
            int newSize = oldSize - (oldPropertyFlagsLength - newPropertyFlagsLength);
            packetData->setUncompressedSize(newSize);

        } else {
            assert(newPropertyFlagsLength == oldPropertyFlagsLength); // should not have grown
        }
       
        packetData->endLevel(entityLevel);
    } else {
        packetData->discardLevel(entityLevel);
        appendState = OctreeElement::NONE; // if we got here, then we didn't include the item
    }
    
    // If any part of the model items didn't fit, then the element is considered partial
    if (appendState != OctreeElement::COMPLETED) {
        // add this item into our list for the next appendElementData() pass
        entityTreeElementExtraEncodeData->entities.insert(getEntityItemID(), propertiesDidntFit);
    }

    return appendState;
}

// TODO: My goal is to get rid of this concept completely. The old code (and some of the current code) used this
// result to calculate if a packet being sent to it was potentially bad or corrupt. I've adjusted this to now
// only consider the minimum header bytes as being required. But it would be preferable to completely eliminate
// this logic from the callers.
int EntityItem::expectedBytes() {
    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;
    return MINIMUM_HEADER_BYTES;
}


int EntityItem::readEntityDataFromBuffer(const unsigned char* data, int bytesLeftToRead, ReadBitstreamToTreeParams& args) {
    bool wantDebug = false;

    if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU) {
    
        // NOTE: This shouldn't happen. The only versions of the bit stream that didn't support split mtu buffers should
        // be handled by the model subclass and shouldn't call this routine.
        qDebug() << "EntityItem::readEntityDataFromBuffer()... "
                        "ERROR CASE...args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_SPLIT_MTU";
        return 0;
    }

    // Header bytes
    //    object ID [16 bytes]
    //    ByteCountCoded(type code) [~1 byte]
    //    last edited [8 bytes]
    //    ByteCountCoded(last_edited to last_updated delta) [~1-8 bytes]
    //    PropertyFlags<>( everything ) [1-2 bytes]
    // ~27-35 bytes...
    const int MINIMUM_HEADER_BYTES = 27;

    int bytesRead = 0;
    if (bytesLeftToRead >= MINIMUM_HEADER_BYTES) {

        int originalLength = bytesLeftToRead;
        QByteArray originalDataBuffer((const char*)data, originalLength);

        int clockSkew = args.sourceNode ? args.sourceNode->getClockSkewUsec() : 0;

        const unsigned char* dataAt = data;

        // id
        QByteArray encodedID = originalDataBuffer.mid(bytesRead, NUM_BYTES_RFC4122_UUID); // maximum possible size
        _id = QUuid::fromRfc4122(encodedID);
        _creatorTokenID = UNKNOWN_ENTITY_TOKEN; // if we know the id, then we don't care about the creator token
        _newlyCreated = false;
        dataAt += encodedID.size();
        bytesRead += encodedID.size();
        
        // type
        QByteArray encodedType = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint32> typeCoder = encodedType;
        encodedType = typeCoder; // determine true length
        dataAt += encodedType.size();
        bytesRead += encodedType.size();
        quint32 type = typeCoder;
        _type = (EntityTypes::EntityType)type;

        bool overwriteLocalData = true; // assume the new content overwrites our local data

        // _created
        quint64 createdFromBuffer = 0;
        memcpy(&createdFromBuffer, dataAt, sizeof(createdFromBuffer));
        dataAt += sizeof(createdFromBuffer);
        bytesRead += sizeof(createdFromBuffer);

        quint64 now = usecTimestampNow();
        if (_created == UNKNOWN_CREATED_TIME) {
            // we don't yet have a _created timestamp, so we accept this one
            createdFromBuffer -= clockSkew;
            if (createdFromBuffer > now || createdFromBuffer == UNKNOWN_CREATED_TIME) {
                createdFromBuffer = now;
            }
            _created = createdFromBuffer;
        }

        if (wantDebug) {
            quint64 lastEdited = getLastEdited();
            float editedAgo = getEditedAgo();
            QString agoAsString = formatSecondsElapsed(editedAgo);
            QString ageAsString = formatSecondsElapsed(getAge());
            qDebug() << "Loading entity " << getEntityItemID() << " from buffer...";
            qDebug() << "    _created =" << _created;
            qDebug() << "    age=" << getAge() << "seconds - " << ageAsString;
            qDebug() << "    lastEdited =" << lastEdited;
            qDebug() << "    ago=" << editedAgo << "seconds - " << agoAsString;
        }
        
        quint64 lastEditedFromBuffer = 0;
        quint64 lastEditedFromBufferAdjusted = 0;

        // TODO: we could make this encoded as a delta from _created
        // _lastEdited
        memcpy(&lastEditedFromBuffer, dataAt, sizeof(lastEditedFromBuffer));
        dataAt += sizeof(lastEditedFromBuffer);
        bytesRead += sizeof(lastEditedFromBuffer);
        lastEditedFromBufferAdjusted = lastEditedFromBuffer - clockSkew;
        if (lastEditedFromBufferAdjusted > now) {
            lastEditedFromBufferAdjusted = now;
        }
        
        bool fromSameServerEdit = (lastEditedFromBuffer == _lastEditedFromRemoteInRemoteTime);

        if (wantDebug) {
            qDebug() << "data from server **************** ";
            qDebug() << "      entityItemID=" << getEntityItemID();
            qDebug() << "      now=" << now;
            qDebug() << "      getLastEdited()=" << getLastEdited();
            qDebug() << "      lastEditedFromBuffer=" << lastEditedFromBuffer << " (BEFORE clockskew adjust)";
            qDebug() << "      clockSkew=" << clockSkew;
            qDebug() << "      lastEditedFromBufferAdjusted=" << lastEditedFromBufferAdjusted << " (AFTER clockskew adjust)";
            qDebug() << "      _lastEditedFromRemote=" << _lastEditedFromRemote 
                                        << " (our local time the last server edit we accepted)";
            qDebug() << "      _lastEditedFromRemoteInRemoteTime=" << _lastEditedFromRemoteInRemoteTime 
                                        << " (remote time the last server edit we accepted)";
            qDebug() << "      fromSameServerEdit=" << fromSameServerEdit;
        }

        bool ignoreServerPacket = false; // assume we'll use this server packet
        
        // If this packet is from the same server edit as the last packet we accepted from the server
        // we probably want to use it.
        if (fromSameServerEdit) {
            // If this is from the same sever packet, then check against any local changes since we got
            // the most recent packet from this server time
            if (_lastEdited > _lastEditedFromRemote) {
                ignoreServerPacket = true;
            }
        } else {
            // If this isn't from the same sever packet, then honor our skew adjusted times...
            // If we've changed our local tree more recently than the new data from this packet
            // then we will not be changing our values, instead we just read and skip the data
            if (_lastEdited > lastEditedFromBufferAdjusted) {
                ignoreServerPacket = true;
            }
        }
        
        if (ignoreServerPacket) {
            overwriteLocalData = false;
            if (wantDebug) {
                qDebug() << "IGNORING old data from server!!! ****************";
            }
        } else {

            if (wantDebug) {
                qDebug() << "USING NEW data from server!!! ****************";
            }

            // don't allow _lastEdited to be in the future
            _lastEdited = lastEditedFromBufferAdjusted;
            _lastEditedFromRemote = now;
            _lastEditedFromRemoteInRemoteTime = lastEditedFromBuffer;
            
            // TODO: only send this notification if something ACTUALLY changed (hint, we haven't yet parsed 
            // the properties out of the bitstream (see below))
            somethingChangedNotification(); // notify derived classes that something has changed
        }

        // last updated is stored as ByteCountCoded delta from lastEdited
        QByteArray encodedUpdateDelta = originalDataBuffer.mid(bytesRead); // maximum possible size
        ByteCountCoded<quint64> updateDeltaCoder = encodedUpdateDelta;
        quint64 updateDelta = updateDeltaCoder;
        if (overwriteLocalData) {
            _lastUpdated = lastEditedFromBufferAdjusted + updateDelta; // don't adjust for clock skew since we already did that
            if (wantDebug) {
                qDebug() << "_lastUpdated =" << _lastUpdated;
                qDebug() << "_lastEdited=" << _lastEdited;
                qDebug() << "lastEditedFromBufferAdjusted=" << lastEditedFromBufferAdjusted;
            }
        }
        encodedUpdateDelta = updateDeltaCoder; // determine true length
        dataAt += encodedUpdateDelta.size();
        bytesRead += encodedUpdateDelta.size();

        // Property Flags
        QByteArray encodedPropertyFlags = originalDataBuffer.mid(bytesRead); // maximum possible size
        EntityPropertyFlags propertyFlags = encodedPropertyFlags;
        dataAt += propertyFlags.getEncodedLength();
        bytesRead += propertyFlags.getEncodedLength();
        
        READ_ENTITY_PROPERTY_SETTER(PROP_POSITION, glm::vec3, updatePosition);

        // Old bitstreams had PROP_RADIUS, new bitstreams have PROP_DIMENSIONS
        if (args.bitstreamVersion < VERSION_ENTITIES_SUPPORT_DIMENSIONS) {
            if (propertyFlags.getHasProperty(PROP_RADIUS)) {
                float fromBuffer;
                memcpy(&fromBuffer, dataAt, sizeof(fromBuffer));
                dataAt += sizeof(fromBuffer);
                bytesRead += sizeof(fromBuffer);
                if (overwriteLocalData) {
                    setRadius(fromBuffer);
                }

                if (wantDebug) {
                    qDebug() << "    readEntityDataFromBuffer() OLD FORMAT... found PROP_RADIUS";
                }

            }
        } else {
            READ_ENTITY_PROPERTY_SETTER(PROP_DIMENSIONS, glm::vec3, setDimensions);
            if (wantDebug) {
                qDebug() << "    readEntityDataFromBuffer() NEW FORMAT... look for PROP_DIMENSIONS";
            }
        }
        
        if (wantDebug) {
            qDebug() << "    readEntityDataFromBuffer() _dimensions:" << getDimensionsInMeters() << " in meters";
        }
                
        READ_ENTITY_PROPERTY_QUAT_SETTER(PROP_ROTATION, updateRotation);
        READ_ENTITY_PROPERTY_SETTER(PROP_MASS, float, updateMass);
        READ_ENTITY_PROPERTY_SETTER(PROP_VELOCITY, glm::vec3, updateVelocity);
        READ_ENTITY_PROPERTY_SETTER(PROP_GRAVITY, glm::vec3, updateGravity);
        READ_ENTITY_PROPERTY(PROP_DAMPING, float, _damping);
        READ_ENTITY_PROPERTY_SETTER(PROP_LIFETIME, float, updateLifetime);
        READ_ENTITY_PROPERTY_STRING(PROP_SCRIPT, setScript);
        READ_ENTITY_PROPERTY(PROP_REGISTRATION_POINT, glm::vec3, _registrationPoint);
        READ_ENTITY_PROPERTY_SETTER(PROP_ANGULAR_VELOCITY, glm::vec3, updateAngularVelocity);
        READ_ENTITY_PROPERTY(PROP_ANGULAR_DAMPING, float, _angularDamping);
        READ_ENTITY_PROPERTY(PROP_VISIBLE, bool, _visible);
        READ_ENTITY_PROPERTY_SETTER(PROP_IGNORE_FOR_COLLISIONS, bool, updateIgnoreForCollisions);
        READ_ENTITY_PROPERTY_SETTER(PROP_COLLISIONS_WILL_MOVE, bool, updateCollisionsWillMove);
        READ_ENTITY_PROPERTY(PROP_LOCKED, bool, _locked);
        READ_ENTITY_PROPERTY_STRING(PROP_USER_DATA,setUserData);

        if (wantDebug) {
            qDebug() << "    readEntityDataFromBuffer() _registrationPoint:" << _registrationPoint;
            qDebug() << "    readEntityDataFromBuffer() _visible:" << _visible;
            qDebug() << "    readEntityDataFromBuffer() _ignoreForCollisions:" << _ignoreForCollisions;
            qDebug() << "    readEntityDataFromBuffer() _collisionsWillMove:" << _collisionsWillMove;
        }

        bytesRead += readEntitySubclassDataFromBuffer(dataAt, (bytesLeftToRead - bytesRead), args, propertyFlags, overwriteLocalData);

        recalculateCollisionShape();
        if (overwriteLocalData && (getDirtyFlags() & EntityItem::DIRTY_POSITION)) {
            _lastSimulated = now;
        }
    }
    return bytesRead;
}

void EntityItem::debugDump() const {
    qDebug() << "EntityItem id:" << getEntityItemID();
    qDebug(" edited ago:%f", getEditedAgo());
    qDebug(" position:%f,%f,%f", _position.x, _position.y, _position.z);
    qDebug() << " dimensions:" << _dimensions;
}

// adjust any internal timestamps to fix clock skew for this server
void EntityItem::adjustEditPacketForClockSkew(unsigned char* editPacketBuffer, size_t length, int clockSkew) {
    unsigned char* dataAt = editPacketBuffer;
    int octets = numberOfThreeBitSectionsInCode(dataAt);
    int lengthOfOctcode = bytesRequiredForCodeLength(octets);
    dataAt += lengthOfOctcode;

    // lastEdited
    quint64 lastEditedInLocalTime;
    memcpy(&lastEditedInLocalTime, dataAt, sizeof(lastEditedInLocalTime));
    quint64 lastEditedInServerTime = lastEditedInLocalTime + clockSkew;
    memcpy(dataAt, &lastEditedInServerTime, sizeof(lastEditedInServerTime));
    const bool wantDebug = false;
    if (wantDebug) {
        qDebug("EntityItem::adjustEditPacketForClockSkew()...");
        qDebug() << "     lastEditedInLocalTime: " << lastEditedInLocalTime;
        qDebug() << "                 clockSkew: " << clockSkew;
        qDebug() << "    lastEditedInServerTime: " << lastEditedInServerTime;
    }
}

// TODO: we probably want to change this to make "down" be the direction of the entity's gravity vector
//       for now, this is always true DOWN even if entity has non-down gravity.
// TODO: the old code had "&& _velocity.y >= -EPSILON && _velocity.y <= EPSILON" --- what was I thinking?
bool EntityItem::isRestingOnSurface() const { 
    glm::vec3 downwardVelocity = glm::vec3(0.0f, _velocity.y, 0.0f);

    return _position.y <= getDistanceToBottomOfEntity()
            && (glm::length(downwardVelocity) <= EPSILON_VELOCITY_LENGTH)
            && _gravity.y < 0.0f;
}

void EntityItem::simulate(const quint64& now) {
    if (_physicsInfo) {
        // we rely on bullet for simulation, so bail
        return;
    }

    bool wantDebug = false;
    
    if (_lastSimulated == 0) {
        _lastSimulated = now;
    }

    float timeElapsed = (float)(now - _lastSimulated) / (float)(USECS_PER_SECOND);

    if (wantDebug) {
        qDebug() << "********** EntityItem::simulate()";
        qDebug() << "    entity ID=" << getEntityItemID();
        qDebug() << "    now=" << now;
        qDebug() << "    _lastSimulated=" << _lastSimulated;
        qDebug() << "    timeElapsed=" << timeElapsed;
        qDebug() << "    hasVelocity=" << hasVelocity();
        qDebug() << "    hasGravity=" << hasGravity();
        qDebug() << "    isRestingOnSurface=" << isRestingOnSurface();
        qDebug() << "    hasAngularVelocity=" << hasAngularVelocity();
        qDebug() << "    getAngularVelocity=" << getAngularVelocity();
        qDebug() << "    isMortal=" << isMortal();
        qDebug() << "    getAge()=" << getAge();
        qDebug() << "    getLifetime()=" << getLifetime();


        if (hasVelocity() || (hasGravity() && !isRestingOnSurface())) {
            qDebug() << "    MOVING...=";
            qDebug() << "        hasVelocity=" << hasVelocity();
            qDebug() << "        hasGravity=" << hasGravity();
            qDebug() << "        isRestingOnSurface=" << isRestingOnSurface();
            qDebug() << "        hasAngularVelocity=" << hasAngularVelocity();
            qDebug() << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (hasAngularVelocity()) {
            qDebug() << "    CHANGING...=";
            qDebug() << "        hasAngularVelocity=" << hasAngularVelocity();
            qDebug() << "        getAngularVelocity=" << getAngularVelocity();
        }
        if (isMortal()) {
            qDebug() << "    MORTAL...=";
            qDebug() << "        isMortal=" << isMortal();
            qDebug() << "        getAge()=" << getAge();
            qDebug() << "        getLifetime()=" << getLifetime();
        }
    }

    if (wantDebug) {
        qDebug() << "     ********** EntityItem::simulate() .... SETTING _lastSimulated=" << _lastSimulated;
    }

    if (hasAngularVelocity()) {
        glm::quat rotation = getRotation();

        // angular damping
        glm::vec3 angularVelocity = getAngularVelocity();
        if (_angularDamping > 0.0f) {
            angularVelocity *= powf(1.0f - _angularDamping, timeElapsed);
            if (wantDebug) {        
                qDebug() << "    angularDamping :" << _angularDamping;
                qDebug() << "    newAngularVelocity:" << angularVelocity;
            }
            setAngularVelocity(angularVelocity);
        }

        float angularSpeed = glm::length(_angularVelocity);
        
        const float EPSILON_ANGULAR_VELOCITY_LENGTH = 0.1f; // 
        if (angularSpeed < EPSILON_ANGULAR_VELOCITY_LENGTH) {
            setAngularVelocity(NO_ANGULAR_VELOCITY);
        } else {
            // NOTE: angularSpeed is currently in degrees/sec!!!
            // TODO: Andrew to convert to radians/sec
            float angle = timeElapsed * glm::radians(angularSpeed);
            glm::vec3 axis = _angularVelocity / angularSpeed;
            glm::quat  dQ = glm::angleAxis(angle, axis);
            rotation = glm::normalize(dQ * rotation);
            setRotation(rotation);
        }
    }

#ifdef USE_BULLET_PHYSICS
    // When Bullet is available we assume that "zero velocity" means "at rest"
    // because of collision conditions this simulation does not know about
    // so we don't fall in for the non-zero gravity case here.
    if (hasVelocity()) {
#else // !USE_BULLET_PHYSICS
    if (hasVelocity() || hasGravity()) {
#endif // USE_BULLET_PHYSICS

        // linear damping
        glm::vec3 velocity = getVelocity();
        if (_damping > 0.0f) {
            velocity *= powf(1.0f - _damping, timeElapsed);
            if (wantDebug) {        
                qDebug() << "    damping:" << _damping;
                qDebug() << "    velocity AFTER dampingResistance:" << velocity;
                qDebug() << "    glm::length(velocity):" << glm::length(velocity);
                qDebug() << "    EPSILON_VELOCITY_LENGTH:" << EPSILON_VELOCITY_LENGTH;
            }
        }

        // integrate position forward
        glm::vec3 position = getPosition();
        glm::vec3 newPosition = position + (velocity * timeElapsed);

        if (wantDebug) {        
            qDebug() << "  EntityItem::simulate()....";
            qDebug() << "    timeElapsed:" << timeElapsed;
            qDebug() << "    old AACube:" << getMaximumAACube();
            qDebug() << "    old position:" << position;
            qDebug() << "    old velocity:" << velocity;
            qDebug() << "    old getAABox:" << getAABox();
            qDebug() << "    getDistanceToBottomOfEntity():" << getDistanceToBottomOfEntity() * (float)TREE_SCALE << " in meters";
            qDebug() << "    newPosition:" << newPosition;
            qDebug() << "    glm::distance(newPosition, position):" << glm::distance(newPosition, position);
        }
        
        position = newPosition;

        // handle bounces off the ground... We bounce at the distance to the bottom of our entity
        if (position.y <= getDistanceToBottomOfEntity()) {
            velocity = velocity * glm::vec3(1,-1,1);

#ifndef USE_BULLET_PHYSICS
            // if we've slowed considerably, then just stop moving, but only if no BULLET
            if (glm::length(velocity) <= EPSILON_VELOCITY_LENGTH) {
                velocity = NO_VELOCITY;
            }
#endif // !USE_BULLET_PHYSICS
            
            position.y = getDistanceToBottomOfEntity();
        }

        // apply gravity
        if (hasGravity()) { 
            // handle resting on surface case, this is definitely a bit of a hack, and it only works on the
            // "ground" plane of the domain, but for now it's what we've got
            if (isRestingOnSurface()) {
                velocity.y = 0.0f;
                position.y = getDistanceToBottomOfEntity();
            } else {
                velocity += getGravity() * timeElapsed;
            }
        }
        
#ifdef USE_BULLET_PHYSICS
        // When Bullet is available we assume that it will tell us when velocities go to zero...
#else // !USE_BULLET_PHYSICS
        // ... otherwise we help things come to rest by clamping small velocities.
        if (glm::length(velocity) <= EPSILON_VELOCITY_LENGTH) {
            velocity = NO_VELOCITY;
        }
#endif // USE_BULLET_PHYSICS

        // NOTE: the simulation should NOT set any DirtyFlags on this entity
        setPosition(position); // this will automatically recalculate our collision shape
        setVelocity(velocity);
        
        if (wantDebug) {        
            qDebug() << "    new position:" << position;
            qDebug() << "    new velocity:" << velocity;
            qDebug() << "    new AACube:" << getMaximumAACube();
            qDebug() << "    old getAABox:" << getAABox();
        }
    }

    _lastSimulated = now;
}

bool EntityItem::isMoving() const {
#ifdef USE_BULLET_PHYSICS
    // When Bullet is available we assume that "zero velocity" means "at rest"
    // because of collision conditions this simulation does not know about.
    return hasVelocity() || hasAngularVelocity();
#else // !USE_BULLET_PHYSICS
    return hasVelocity() || (hasGravity() && !isRestingOnSurface()) || hasAngularVelocity();
#endif //USE_BULLET_PHYSICS
}

bool EntityItem::lifetimeHasExpired() const { 
    return isMortal() && (getAge() > getLifetime()); 
}

quint64 EntityItem::getExpiry() const {
    return _created + (quint64)(_lifetime * (float)USECS_PER_SECOND);
}

EntityItemProperties EntityItem::getProperties() const {
    EntityItemProperties properties;
    properties._id = getID();
    properties._idSet = true;
    properties._created = _created;

    properties._type = getType();
    
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(position, getPositionInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(dimensions, getDimensionsInMeters); // NOTE: radius is obsolete
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(rotation, getRotation);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(mass, getMass);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(velocity, getVelocityInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(gravity, getGravityInMeters);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(damping, getDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lifetime, getLifetime);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(script, getScript);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(registrationPoint, getRegistrationPoint);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularVelocity, getAngularVelocity);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(angularDamping, getAngularDamping);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(glowLevel, getGlowLevel);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(localRenderAlpha, getLocalRenderAlpha);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(visible, getVisible);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(ignoreForCollisions, getIgnoreForCollisions);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(collisionsWillMove, getCollisionsWillMove);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(locked, getLocked);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(userData, getUserData);

    properties._defaultSettings = false;
    
    return properties;
}

bool EntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(position, updatePositionInMeters); // this will call recalculate collision shape if needed
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(dimensions, updateDimensionsInMeters); // NOTE: radius is obsolete
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(rotation, updateRotation);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(mass, updateMass);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(velocity, updateVelocityInMeters);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(gravity, updateGravityInMeters);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(damping, updateDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lifetime, updateLifetime);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(script, setScript);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(registrationPoint, setRegistrationPoint);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularVelocity, updateAngularVelocity);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(angularDamping, updateAngularDamping);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(glowLevel, setGlowLevel);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(localRenderAlpha, setLocalRenderAlpha);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(visible, setVisible);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(ignoreForCollisions, updateIgnoreForCollisions);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(collisionsWillMove, updateCollisionsWillMove);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(locked, setLocked);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(userData, setUserData);

    if (somethingChanged) {
        somethingChangedNotification(); // notify derived classes that something has changed
        bool wantDebug = false;
        uint64_t now = usecTimestampNow();
        if (wantDebug) {
            int elapsed = now - getLastEdited();
            qDebug() << "EntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                    "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        if (_created != UNKNOWN_CREATED_TIME) {
            setLastEdited(now);
        }
        if (getDirtyFlags() & EntityItem::DIRTY_POSITION) {
            _lastSimulated = now;
        }
    }

    // timestamps
    quint64 timestamp = properties.getCreated();
    if (_created == UNKNOWN_CREATED_TIME && timestamp != UNKNOWN_CREATED_TIME) {
        quint64 now = usecTimestampNow();
        if (timestamp > now) {
            timestamp = now;
        }
        _created = timestamp;

        timestamp = properties.getLastEdited();
        if (timestamp > now) {
            timestamp = now;
        } else if (timestamp < _created) {
            timestamp = _created;
        }
        _lastEdited = timestamp;
    }

    return somethingChanged;
}

void EntityItem::recordCreationTime() {
    assert(_created == UNKNOWN_CREATED_TIME);
    _created = usecTimestampNow();
    _lastEdited = _created;
    _lastUpdated = _created;
    _lastSimulated = _created;
}


// TODO: is this really correct? how do we use size, does it need to handle rotation?
float EntityItem::getSize() const { 
    return glm::length(_dimensions);
}

float EntityItem::getDistanceToBottomOfEntity() const {
    glm::vec3 minimumPoint = getAABox().getMinimumPoint();
    return getPosition().y - minimumPoint.y;
}

// TODO: doesn't this need to handle rotation?
glm::vec3 EntityItem::getCenter() const {
    return _position + (_dimensions * (glm::vec3(0.5f,0.5f,0.5f) - _registrationPoint));
}

/// The maximum bounding cube for the entity, independent of it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
/// 
AACube EntityItem::getMaximumAACube() const { 
    // * we know that the position is the center of rotation
    glm::vec3 centerOfRotation = _position; // also where _registration point is

    // * we know that the registration point is the center of rotation
    // * we can calculate the length of the furthest extent from the registration point
    //   as the dimensions * max (registrationPoint, (1.0,1.0,1.0) - registrationPoint)
    glm::vec3 registrationPoint = (_dimensions * _registrationPoint);
    glm::vec3 registrationRemainder = (_dimensions * (glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint));
    glm::vec3 furthestExtentFromRegistration = glm::max(registrationPoint, registrationRemainder);
    
    // * we know that if you rotate in any direction you would create a sphere
    //   that has a radius of the length of furthest extent from registration point
    float radius = glm::length(furthestExtentFromRegistration);

    // * we know that the minimum bounding cube of this maximum possible sphere is
    //   (center - radius) to (center + radius)
    glm::vec3 minimumCorner = centerOfRotation - glm::vec3(radius, radius, radius);

    AACube boundingCube(minimumCorner, radius * 2.0f);
    return boundingCube;
}

/// The minimum bounding cube for the entity accounting for it's rotation.
/// This accounts for the registration point (upon which rotation occurs around).
/// 
AACube EntityItem::getMinimumAACube() const { 
    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;

    glm::vec3 unrotatedMinRelativeToEntity = glm::vec3(0.0f, 0.0f, 0.0f) - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());

    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);
    
    // the cube that best encompasses extents is...
    AABox box(rotatedExtentsRelativeToRegistrationPoint);
    glm::vec3 centerOfBox = box.calcCenter();
    float longestSide = box.getLargestDimension();
    float halfLongestSide = longestSide / 2.0f;
    glm::vec3 cornerOfCube = centerOfBox - glm::vec3(halfLongestSide, halfLongestSide, halfLongestSide);
    
    
    // old implementation... not correct!!!
    return AACube(cornerOfCube, longestSide);
}

AABox EntityItem::getAABox() const { 

    // _position represents the position of the registration point.
    glm::vec3 registrationRemainder = glm::vec3(1.0f, 1.0f, 1.0f) - _registrationPoint;
    
    glm::vec3 unrotatedMinRelativeToEntity = glm::vec3(0.0f, 0.0f, 0.0f) - (_dimensions * _registrationPoint);
    glm::vec3 unrotatedMaxRelativeToEntity = _dimensions * registrationRemainder;
    Extents unrotatedExtentsRelativeToRegistrationPoint = { unrotatedMinRelativeToEntity, unrotatedMaxRelativeToEntity };
    Extents rotatedExtentsRelativeToRegistrationPoint = unrotatedExtentsRelativeToRegistrationPoint.getRotated(getRotation());
    
    // shift the extents to be relative to the position/registration point
    rotatedExtentsRelativeToRegistrationPoint.shiftBy(_position);
    
    return AABox(rotatedExtentsRelativeToRegistrationPoint);
}


// NOTE: This should only be used in cases of old bitstreams which only contain radius data
//    0,0,0 --> maxDimension,maxDimension,maxDimension
//    ... has a corner to corner distance of glm::length(maxDimension,maxDimension,maxDimension)
//    ... radius = cornerToCornerLength / 2.0f
//    ... radius * 2.0f = cornerToCornerLength
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius * 2.0f = sqrt(3 x maxDimension ^ 2)
//    ... (radius * 2.0f) ^2 = 3 x maxDimension ^ 2
//    ... ((radius * 2.0f) ^2) / 3 = maxDimension ^ 2
//    ... sqrt(((radius * 2.0f) ^2) / 3) = maxDimension
//    ... sqrt((diameter ^2) / 3) = maxDimension
//    
void EntityItem::setRadius(float value) { 
    float diameter = value * 2.0f;
    float maxDimension = sqrt((diameter * diameter) / 3.0f);
    _dimensions = glm::vec3(maxDimension, maxDimension, maxDimension);

    bool wantDebug = false;    
    if (wantDebug) {
        qDebug() << "EntityItem::setRadius()...";
        qDebug() << "    radius:" << value;
        qDebug() << "    diameter:" << diameter;
        qDebug() << "    maxDimension:" << maxDimension;
        qDebug() << "    _dimensions:" << _dimensions;
    }
}

// TODO: get rid of all users of this function...
//    ... radius = cornerToCornerLength / 2.0f
//    ... cornerToCornerLength = sqrt(3 x maxDimension ^ 2)
//    ... radius = sqrt(3 x maxDimension ^ 2) / 2.0f;
float EntityItem::getRadius() const { 
    float length = glm::length(_dimensions);
    float radius = length / 2.0f;
    return radius;
}

void EntityItem::computeShapeInfo(ShapeInfo& info) const {
    info.clear();
}

void EntityItem::recalculateCollisionShape() {
    AACube entityAACube = getMinimumAACube();
    entityAACube.scale(TREE_SCALE); // scale to meters
    _collisionShape.setTranslation(entityAACube.calcCenter());
    _collisionShape.setScale(entityAACube.getScale());
    // TODO: use motionState to update physics object
}

const float MIN_POSITION_DELTA = 0.0001f;
const float MIN_DIMENSION_DELTA = 0.0001f;
const float MIN_ALIGNMENT_DOT = 0.9999f;
const float MIN_MASS_DELTA = 0.001f;
const float MIN_VELOCITY_DELTA = 0.025f;
const float MIN_DAMPING_DELTA = 0.001f;
const float MIN_GRAVITY_DELTA = 0.001f;
const float MIN_SPIN_DELTA = 0.0003f;

void EntityItem::updatePosition(const glm::vec3& value) { 
    if (glm::distance(_position, value) * (float)TREE_SCALE > MIN_POSITION_DELTA) {
        _position = value; 
        recalculateCollisionShape();
        _dirtyFlags |= EntityItem::DIRTY_POSITION;
    }
}

void EntityItem::updatePositionInMeters(const glm::vec3& value) { 
    glm::vec3 position = glm::clamp(value / (float) TREE_SCALE, 0.0f, 1.0f);
    if (glm::distance(_position, position) * (float)TREE_SCALE > MIN_POSITION_DELTA) {
        _position = position;
        recalculateCollisionShape();
        _dirtyFlags |= EntityItem::DIRTY_POSITION;
    }
}

void EntityItem::updateDimensions(const glm::vec3& value) { 
    if (glm::distance(_dimensions, value) * (float)TREE_SCALE > MIN_DIMENSION_DELTA) {
        _dimensions = value; 
        recalculateCollisionShape();
        _dirtyFlags |= (EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS);
    }
}

void EntityItem::updateDimensionsInMeters(const glm::vec3& value) { 
    glm::vec3 dimensions = value / (float) TREE_SCALE;
    if (glm::distance(_dimensions, dimensions) * (float)TREE_SCALE > MIN_DIMENSION_DELTA) {
        _dimensions = dimensions; 
        recalculateCollisionShape();
        _dirtyFlags |= (EntityItem::DIRTY_SHAPE | EntityItem::DIRTY_MASS);
    }
}

void EntityItem::updateRotation(const glm::quat& rotation) { 
    if (glm::dot(_rotation, rotation) < MIN_ALIGNMENT_DOT) {
        _rotation = rotation; 
        recalculateCollisionShape();
        _dirtyFlags |= EntityItem::DIRTY_POSITION;
    }
}

void EntityItem::updateMass(float value) {
    if (fabsf(_mass - value) > MIN_MASS_DELTA) {
        _mass = value;
        _dirtyFlags |= EntityItem::DIRTY_MASS;
    }
}

void EntityItem::updateVelocity(const glm::vec3& value) {
    if (glm::distance(_velocity, value) * (float)TREE_SCALE > MIN_VELOCITY_DELTA) {
        if (glm::length(value) * (float)TREE_SCALE < MIN_VELOCITY_DELTA) {
            _velocity = glm::vec3(0.0f);
        } else {
            _velocity = value;
        }
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateVelocityInMeters(const glm::vec3& value) { 
    glm::vec3 velocity = value / (float) TREE_SCALE; 
    if (glm::distance(_velocity, velocity) * (float)TREE_SCALE > MIN_VELOCITY_DELTA) {
        if (glm::length(value) < MIN_VELOCITY_DELTA) {
            _velocity = glm::vec3(0.0f);
        } else {
            _velocity = velocity;
        }
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateDamping(float value) { 
    if (fabsf(_damping - value) > MIN_DAMPING_DELTA) {
        _damping = glm::clamp(value, 0.0f, 1.0f);
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateGravity(const glm::vec3& value) { 
    if (glm::distance(_gravity, value) * (float)TREE_SCALE > MIN_GRAVITY_DELTA) {
        _gravity = value; 
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateGravityInMeters(const glm::vec3& value) { 
    glm::vec3 gravity = value / (float) TREE_SCALE;
    if ( glm::distance(_gravity, gravity) * (float)TREE_SCALE > MIN_GRAVITY_DELTA) {
        _gravity = gravity;
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateAngularVelocity(const glm::vec3& value) { 
    if (glm::distance(_angularVelocity, value) > MIN_SPIN_DELTA) {
        _angularVelocity = value; 
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateAngularDamping(float value) { 
    if (fabsf(_angularDamping - value) > MIN_DAMPING_DELTA) {
        _angularDamping = glm::clamp(value, 0.0f, 1.0f);
        _dirtyFlags |= EntityItem::DIRTY_VELOCITY;
    }
}

void EntityItem::updateIgnoreForCollisions(bool value) { 
    if (_ignoreForCollisions != value) {
        _ignoreForCollisions = value; 
        _dirtyFlags |= EntityItem::DIRTY_COLLISION_GROUP;
    }
}

void EntityItem::updateCollisionsWillMove(bool value) { 
    if (_collisionsWillMove != value) {
        _collisionsWillMove = value; 
        _dirtyFlags |= EntityItem::DIRTY_MOTION_TYPE;
    }
}

void EntityItem::updateLifetime(float value) {
    if (_lifetime != value) {
        _lifetime = value;
        _dirtyFlags |= EntityItem::DIRTY_LIFETIME;
    }
}

