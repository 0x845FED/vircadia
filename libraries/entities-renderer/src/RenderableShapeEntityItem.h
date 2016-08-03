//
//  Created by Bradley Austin Davis on 2016/05/09
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableShapeEntityItem_h
#define hifi_RenderableShapeEntityItem_h

#include <ShapeEntityItem.h>
#include <procedural/Procedural.h>
#include <Interpolate.h>

#include "RenderableEntityItem.h"

class RenderableShapeEntityItem : public ShapeEntityItem {
    using Pointer = std::shared_ptr<RenderableShapeEntityItem>;
    static Pointer baseFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer boxFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    static EntityItemPointer sphereFactory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableShapeEntityItem(const EntityItemID& entityItemID) : ShapeEntityItem(entityItemID), _fadeStartTime(usecTimestampNow()) {}

    void render(RenderArgs* args) override;
    void setUserData(const QString& value) override;

    bool isTransparent() override { return Interpolate::calculateFadeRatio(_fadeStartTime) < 1.0f; }

    SIMPLE_RENDERABLE();

private:
    QSharedPointer<Procedural> _procedural;
    quint64 _fadeStartTime { 0 };
};


#endif // hifi_RenderableShapeEntityItem_h
