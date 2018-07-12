//
//  LineEntityItem.cpp
//  libraries/entities/src
//
//  Created by Seth Alves on 5/11/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "LineEntityItem.h"

#include <QDebug>

#include <ByteCountCoding.h>

#include "EntitiesLogging.h"
#include "EntityItemProperties.h"
#include "EntityTree.h"
#include "EntityTreeElement.h"
#include "OctreeConstants.h"

const float LineEntityItem::DEFAULT_LINE_WIDTH = 2.0f;
const int LineEntityItem::MAX_POINTS_PER_LINE = 70;


EntityItemPointer LineEntityItem::factory(const EntityItemID& entityID, const EntityItemProperties& properties) {
    EntityItemPointer entity(new LineEntityItem(entityID), [](EntityItem* ptr) { ptr->deleteLater(); });
    entity->setProperties(properties);
    return entity;
}

LineEntityItem::LineEntityItem(const EntityItemID& entityItemID) :
    EntityItem(entityItemID)
{
    _type = EntityTypes::Line;
}

EntityItemProperties LineEntityItem::getProperties(EntityPropertyFlags desiredProperties) const {
    
    EntityItemProperties properties = EntityItem::getProperties(desiredProperties); // get the properties from our base class

    COPY_ENTITY_PROPERTY_TO_PROPERTIES(color, getColor);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(lineWidth, getLineWidth);
    COPY_ENTITY_PROPERTY_TO_PROPERTIES(linePoints, getLinePoints);

    return properties;
}

bool LineEntityItem::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;
    somethingChanged = EntityItem::setProperties(properties); // set the properties in our base class

    SET_ENTITY_PROPERTY_FROM_PROPERTIES(color, setColor);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(lineWidth, setLineWidth);
    SET_ENTITY_PROPERTY_FROM_PROPERTIES(linePoints, setLinePoints);

    if (somethingChanged) {
        bool wantDebug = false;
        if (wantDebug) {
            uint64_t now = usecTimestampNow();
            int elapsed = now - getLastEdited();
            qCDebug(entities) << "LineEntityItem::setProperties() AFTER update... edited AGO=" << elapsed <<
                "now=" << now << " getLastEdited()=" << getLastEdited();
        }
        setLastEdited(properties._lastEdited);
    }
    return somethingChanged;
}

bool LineEntityItem::appendPoint(const ScriptVec3Float& point) {
    if (_points.size() > MAX_POINTS_PER_LINE - 1) {
        qCDebug(entities) << "MAX POINTS REACHED!";
        return false;
    }
    glm::vec3 halfBox = getScaledDimensions() * 0.5f;
    if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
        qCDebug(entities) << "Point is outside entity's bounding box";
        return false;
    }
    withWriteLock([&] {
        _points << point;
        _pointsChanged = true;
    });
    return true;
}

bool LineEntityItem::setLinePoints(const QVector<ScriptVec3Float>& points) {
    if (points.size() > MAX_POINTS_PER_LINE) {
        return false;
    }
    glm::vec3 halfBox = getScaledDimensions() * 0.5f;
    for (int i = 0; i < points.size(); i++) {
        ScriptVec3Float point = points.at(i);
        if ( (point.x < - halfBox.x || point.x > halfBox.x) || (point.y < -halfBox.y || point.y > halfBox.y) || (point.z < - halfBox.z || point.z > halfBox.z) ) {
            qCDebug(entities) << "Point is outside entity's bounding box";
            return false;
        }
    }

    withWriteLock([&] {
        _points = points;
        _pointsChanged = true;
    });
    return true;
}

int LineEntityItem::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                     ReadBitstreamToTreeParams& args,
                                                     EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                     bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_COLOR, ScriptVec3UChar, setColor);
    READ_ENTITY_PROPERTY(PROP_LINE_WIDTH, float, setLineWidth);
    READ_ENTITY_PROPERTY(PROP_LINE_POINTS, QVector<ScriptVec3Float>, setLinePoints);

    return bytesRead;
}


EntityPropertyFlags LineEntityItem::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties = EntityItem::getEntityProperties(params);
    requestedProperties += PROP_COLOR;
    requestedProperties += PROP_LINE_WIDTH;
    requestedProperties += PROP_LINE_POINTS;
    return requestedProperties;
}

void LineEntityItem::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params, 
                                        EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                        EntityPropertyFlags& requestedProperties,
                                        EntityPropertyFlags& propertyFlags,
                                        EntityPropertyFlags& propertiesDidntFit,
                                        int& propertyCount, 
                                        OctreeElement::AppendState& appendState) const { 

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_COLOR, getColor());
    APPEND_ENTITY_PROPERTY(PROP_LINE_WIDTH, getLineWidth());
    APPEND_ENTITY_PROPERTY(PROP_LINE_POINTS, getLinePoints());
}

void LineEntityItem::debugDump() const {
    quint64 now = usecTimestampNow();
    qCDebug(entities) << "   LINE EntityItem id:" << getEntityItemID() << "---------------------------------------------";
    qCDebug(entities) << "               color:" << _color;
    qCDebug(entities) << "            position:" << debugTreeVector(getWorldPosition());
    qCDebug(entities) << "          dimensions:" << debugTreeVector(getScaledDimensions());
    qCDebug(entities) << "       getLastEdited:" << debugTime(getLastEdited(), now);
}

const ScriptVec3UChar& LineEntityItem::getColor() const {
    return resultWithReadLock<ScriptVec3UChar>([&] {
        return _color;
    });
}

void LineEntityItem::setColor(const ScriptVec3UChar& value) {
    withWriteLock([&] {
        _color = value;
    });
}

void LineEntityItem::setLineWidth(float lineWidth) { 
    withWriteLock([&] {
        _lineWidth = lineWidth;
    });
}

float LineEntityItem::getLineWidth() const { 
    float result;
    withReadLock([&] {
        result = _lineWidth;
    });
    return result;
}

QVector<ScriptVec3Float> LineEntityItem::getLinePoints() const { 
    QVector<ScriptVec3Float> result;
    withReadLock([&] {
        result = _points;
    });
    return result;
}

void LineEntityItem::resetPointsChanged() { 
    withWriteLock([&] {
        _pointsChanged = false;
    });
}
