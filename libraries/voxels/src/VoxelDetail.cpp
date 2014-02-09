//
//  VoxelDetail.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 1/29/2014
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
#include "VoxelDetail.h"

void registerVoxelMetaTypes(QScriptEngine* engine) {
    qScriptRegisterMetaType(engine, voxelDetailToScriptValue, voxelDetailFromScriptValue);
    qScriptRegisterMetaType(engine, rayToVoxelIntersectionResultToScriptValue, rayToVoxelIntersectionResultFromScriptValue);
}

QScriptValue voxelDetailToScriptValue(QScriptEngine* engine, const VoxelDetail& voxelDetail) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("x", voxelDetail.x * (float)TREE_SCALE);
    obj.setProperty("y", voxelDetail.y * (float)TREE_SCALE);
    obj.setProperty("z", voxelDetail.z * (float)TREE_SCALE);
    obj.setProperty("s", voxelDetail.s * (float)TREE_SCALE);
    obj.setProperty("red", voxelDetail.red);
    obj.setProperty("green", voxelDetail.green);
    obj.setProperty("blue", voxelDetail.blue);
    return obj;
}

void voxelDetailFromScriptValue(const QScriptValue &object, VoxelDetail& voxelDetail) {
    voxelDetail.x = object.property("x").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.y = object.property("y").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.z = object.property("z").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.s = object.property("s").toVariant().toFloat() / (float)TREE_SCALE;
    voxelDetail.red = object.property("red").toVariant().toInt();
    voxelDetail.green = object.property("green").toVariant().toInt();
    voxelDetail.blue = object.property("blue").toVariant().toInt();
}

RayToVoxelIntersectionResult::RayToVoxelIntersectionResult() : 
    intersects(false), 
    voxel(),
    distance(0),
    face()
{ 
};

QScriptValue rayToVoxelIntersectionResultToScriptValue(QScriptEngine* engine, const RayToVoxelIntersectionResult& value) {
    QScriptValue obj = engine->newObject();
    obj.setProperty("intersects", value.intersects);
    QScriptValue voxelValue = voxelDetailToScriptValue(engine, value.voxel);
    obj.setProperty("voxel", voxelValue);
    obj.setProperty("distance", value.distance);

    QString faceName = "";    
    // handle BoxFace
    switch (value.face) {
        case MIN_X_FACE:
            faceName = "MIN_X_FACE";
            break;
        case MAX_X_FACE:
            faceName = "MAX_X_FACE";
            break;
        case MIN_Y_FACE:
            faceName = "MIN_Y_FACE";
            break;
        case MAX_Y_FACE:
            faceName = "MAX_Y_FACE";
            break;
        case MIN_Z_FACE:
            faceName = "MIN_Z_FACE";
            break;
        case MAX_Z_FACE:
            faceName = "MAX_Z_FACE";
            break;
    }
    obj.setProperty("face", faceName);
    return obj;
}

void rayToVoxelIntersectionResultFromScriptValue(const QScriptValue& object, RayToVoxelIntersectionResult& value) {
    value.intersects = object.property("intersects").toVariant().toBool();
    QScriptValue voxelValue = object.property("voxel");
    if (voxelValue.isValid()) {
        voxelDetailFromScriptValue(voxelValue, value.voxel);
    }
    value.distance = object.property("distance").toVariant().toFloat();

    QString faceName = object.property("face").toVariant().toString();
    if (faceName == "MIN_X_FACE") {
        value.face = MIN_X_FACE;
    } else if (faceName == "MAX_X_FACE") {
        value.face = MAX_X_FACE;
    } else if (faceName == "MIN_Y_FACE") {
        value.face = MIN_Y_FACE;
    } else if (faceName == "MAX_Y_FACE") {
        value.face = MAX_Y_FACE;
    } else if (faceName == "MIN_Z_FACE") {
        value.face = MIN_Z_FACE;
    } else {
        value.face = MAX_Z_FACE;
    };
}




