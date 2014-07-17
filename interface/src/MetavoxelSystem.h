//
//  MetavoxelSystem.h
//  interface/src
//
//  Created by Andrzej Kapolka on 12/10/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_MetavoxelSystem_h
#define hifi_MetavoxelSystem_h

#include <QList>
#include <QOpenGLBuffer>
#include <QVector>

#include <glm/glm.hpp>

#include <MetavoxelClientManager.h>

#include "renderer/ProgramObject.h"

class Model;

/// Renders a metavoxel tree.
class MetavoxelSystem : public MetavoxelClientManager {
    Q_OBJECT

public:

    MetavoxelSystem();

    virtual void init();

    virtual MetavoxelLOD getLOD() const;
    
    void simulate(float deltaTime);
    void render();

protected:

    virtual MetavoxelClient* createClient(const SharedNodePointer& node);
    virtual void updateClient(MetavoxelClient* client); 

private:
    
    class Point {
    public:
        glm::vec4 vertex;
        quint8 color[4];
        quint8 normal[3];
    };
    
    class SimulateVisitor : public SpannerVisitor {
    public:
        SimulateVisitor(QVector<Point>& points);
        void setDeltaTime(float deltaTime) { _deltaTime = deltaTime; }
        void setOrder(const glm::vec3& direction) { _order = encodeOrder(direction); }
        virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);
        virtual int visit(MetavoxelInfo& info);
    
    private:
        QVector<Point>& _points;
        float _deltaTime;
        int _order;
    };
    
    class RenderVisitor : public SpannerVisitor {
    public:
        RenderVisitor();
        virtual bool visit(Spanner* spanner, const glm::vec3& clipMinimum, float clipSize);
    };
    
    static ProgramObject _program;
    static int _pointScaleLocation;
    
    QVector<Point> _points;
    SimulateVisitor _simulateVisitor;
    RenderVisitor _renderVisitor;
    QOpenGLBuffer _buffer;
    AttributePointer _pointBufferAttribute;
};

/// A client session associated with a single server.
class MetavoxelSystemClient : public MetavoxelClient {
    Q_OBJECT    
    
public:
    
    MetavoxelSystemClient(const SharedNodePointer& node, MetavoxelSystem* system);
    
    virtual int parseData(const QByteArray& packet);

protected:
    
    virtual void dataChanged(const MetavoxelData& oldData);
    virtual void sendDatagram(const QByteArray& data);
};

/// Describes contents of a point in a point buffer.
class BufferPoint {
public:
    glm::vec4 vertex;
    quint8 color[3];
    quint8 normal[3];
};

/// Contains the information necessary to render a group of points at variable detail levels.
class PointBuffer {
public:

    PointBuffer(const QOpenGLBuffer& buffer, const QVector<int>& offsets, int lastLeafCount);

    void render(int level);

private:
    
    QOpenGLBuffer _buffer;
    QVector<int> _offsets;
    int _lastLeafCount;
};

typedef QSharedPointer<PointBuffer> PointBufferPointer;

/// A client-side attribute that stores point buffers.
class PointBufferAttribute : public SharedPointerAttribute<PointBuffer> {
    Q_OBJECT
    
public:
    
    Q_INVOKABLE PointBufferAttribute();
    
    virtual MetavoxelNode* createMetavoxelNode(const AttributeValue& value, const MetavoxelNode* original) const;
    virtual bool merge(void*& parent, void* children[], bool postRead = false) const;
    virtual AttributeValue inherit(const AttributeValue& parentValue) const;
};

/// Base class for spanner renderers; provides clipping.
class ClippedRenderer : public SpannerRenderer {
    Q_OBJECT

public:
    
    virtual void render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(float alpha, Mode mode) = 0;
};

/// Renders spheres.
class SphereRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE SphereRenderer();
    
    virtual void render(float alpha, Mode mode, const glm::vec3& clipMinimum, float clipSize);
    
protected:

    virtual void renderUnclipped(float alpha, Mode mode);
};

/// Renders static models.
class StaticModelRenderer : public ClippedRenderer {
    Q_OBJECT

public:
    
    Q_INVOKABLE StaticModelRenderer();
    
    virtual void init(Spanner* spanner);
    virtual void simulate(float deltaTime);
    virtual bool findRayIntersection(const glm::vec3& origin, const glm::vec3& direction,
        const glm::vec3& clipMinimum, float clipSize, float& distance) const;

protected:

    virtual void renderUnclipped(float alpha, Mode mode);

private slots:

    void applyTranslation(const glm::vec3& translation);
    void applyRotation(const glm::quat& rotation);
    void applyScale(float scale);
    void applyURL(const QUrl& url);

private:
    
    Model* _model;
};

#endif // hifi_MetavoxelSystem_h
