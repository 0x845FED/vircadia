//
//  PolyLineEntityItem.h
//  libraries/entities/src
//
//  Created by Eric Levin on 8/3/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_PolyLineEntityItem_h
#define hifi_PolyLineEntityItem_h

#include "EntityItem.h"

class PolyLineEntityItem : public EntityItem {
 public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    PolyLineEntityItem(const EntityItemID& entityItemID);

    ALLOW_INSTANTIATION // This class can be instantiated

    // methods for getting/setting all properties of an entity
    virtual EntityItemProperties getProperties(EntityPropertyFlags desiredProperties = EntityPropertyFlags()) const override;
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer modelTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                 bool& somethingChanged) override;

    ScriptVec3UChar getColor() const;
    void setColor(const ScriptVec3UChar& value);

    void setLineWidth(float lineWidth){ _lineWidth = lineWidth; }
    float getLineWidth() const{ return _lineWidth; }

    bool setLinePoints(const QVector<ScriptVec3Float>& points);
    bool appendPoint(const ScriptVec3Float& point);
    QVector<ScriptVec3Float> getLinePoints() const;

    bool setNormals(const QVector<ScriptVec3Float>& normals);
    QVector<ScriptVec3Float> getNormals() const;

    bool setStrokeColors(const QVector<ScriptVec3Float>& strokeColors);
    QVector<ScriptVec3Float> getStrokeColors() const;

    bool setStrokeWidths(const QVector<float>& strokeWidths);
    QVector<float> getStrokeWidths() const;

    void setIsUVModeStretch(bool isUVModeStretch){ _isUVModeStretch = isUVModeStretch; }
    bool getIsUVModeStretch() const{ return _isUVModeStretch; }

    QString getTextures() const;
    void setTextures(const QString& textures);

    virtual ShapeType getShapeType() const override { return SHAPE_TYPE_NONE; }

    bool pointsChanged() const { return _pointsChanged; } 
    bool normalsChanged() const { return _normalsChanged; }
    bool strokeColorsChanged() const { return _strokeColorsChanged; }
    bool strokeWidthsChanged() const { return _strokeWidthsChanged; }
    bool texturesChanged() const { return _texturesChangedFlag; }
    void resetTexturesChanged() { _texturesChangedFlag = false; }
    void resetPolyLineChanged() { _strokeColorsChanged = _strokeWidthsChanged = _normalsChanged = _pointsChanged = false; }


    // never have a ray intersection pick a PolyLineEntityItem.
    virtual bool supportsDetailedIntersection() const override { return true; }
    virtual bool findDetailedRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
                                             OctreeElementPointer& element, float& distance,
                                             BoxFace& face, glm::vec3& surfaceNormal,
                                             QVariantMap& extraInfo, bool precisionPicking) const override { return false; }
    virtual bool findDetailedParabolaIntersection(const glm::vec3& origin, const glm::vec3& velocity,
                                                  const glm::vec3& acceleration, OctreeElementPointer& element, float& parabolicDistance,
                                                  BoxFace& face, glm::vec3& surfaceNormal,
                                                  QVariantMap& extraInfo, bool precisionPicking) const override { return false; }

    // disable these external interfaces as PolyLineEntities caculate their own dimensions based on the points they contain
    virtual void setRegistrationPoint(const glm::vec3& value) override {}; // FIXME: this is suspicious! 

    virtual void debugDump() const override;
    static const float DEFAULT_LINE_WIDTH;
    static const int MAX_POINTS_PER_LINE;
private:
    void calculateScaleAndRegistrationPoint();
    
 protected:
    ScriptVec3UChar _color;
    float _lineWidth { DEFAULT_LINE_WIDTH };
    bool _pointsChanged { true };
    bool _normalsChanged { true };
    bool _strokeColorsChanged { true };
    bool _strokeWidthsChanged { true };
    QVector<ScriptVec3Float> _points;
    QVector<ScriptVec3Float> _normals;
    QVector<ScriptVec3Float> _strokeColors;
    QVector<float> _strokeWidths;
    QString _textures;
    bool _isUVModeStretch;
    bool _texturesChangedFlag { false };
    mutable QReadWriteLock _quadReadWriteLock;
};

#endif // hifi_PolyLineEntityItem_h
