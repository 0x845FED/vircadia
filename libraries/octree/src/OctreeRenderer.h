//
//  OctreeRenderer.h
//  libraries/octree/src
//
//  Created by Brad Hefta-Gaub on 12/6/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_OctreeRenderer_h
#define hifi_OctreeRenderer_h

#include <glm/glm.hpp>
#include <stdint.h>

#include <QObject>

#include <PacketHeaders.h>
#include <SharedUtil.h>

#include "Octree.h"
#include "OctreePacketData.h"
#include "ViewFrustum.h"

class OctreeRenderer;

class RenderArgs {
public:
    int _renderedItems;
    OctreeRenderer* _renderer;
    ViewFrustum* _viewFrustum;
};


// Generic client side Octree renderer class.
class OctreeRenderer : public QObject {
    Q_OBJECT
public:
    OctreeRenderer();
    virtual ~OctreeRenderer();

    virtual Octree* createTree() = 0;
    virtual NodeType_t getMyNodeType() const = 0;
    virtual PacketType getMyQueryMessageType() const = 0;
    virtual PacketType getExpectedPacketType() const = 0;
    virtual void renderElement(OctreeElement* element, RenderArgs* args) = 0;

    virtual void setTree(Octree* newTree);
    
    /// process incoming data
    virtual void processDatagram(const QByteArray& dataByteArray, const SharedNodePointer& sourceNode);

    /// initialize and GPU/rendering related resources
    virtual void init();

    /// render the content of the octree
    virtual void render();

    ViewFrustum* getViewFrustum() const { return _viewFrustum; }
    void setViewFrustum(ViewFrustum* viewFrustum) { _viewFrustum = viewFrustum; }

    static bool renderOperation(OctreeElement* element, void* extraData);

    /// clears the tree
    void clear();
protected:
    Octree* _tree;
    bool _managedTree;
    ViewFrustum* _viewFrustum;
};

#endif // hifi_OctreeRenderer_h
