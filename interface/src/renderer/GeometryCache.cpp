//
//  GeometryCache.cpp
//  interface
//
//  Created by Andrzej Kapolka on 6/21/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.

#include <cmath>

#include <QNetworkReply>
#include <QTimer>

#include "Application.h"
#include "GeometryCache.h"
#include "world.h"

GeometryCache::~GeometryCache() {
    foreach (const VerticesIndices& vbo, _hemisphereVBOs) {
        glDeleteBuffers(1, &vbo.first);
        glDeleteBuffers(1, &vbo.second);
    }
}

void GeometryCache::renderHemisphere(int slices, int stacks) {
    VerticesIndices& vbo = _hemisphereVBOs[IntPair(slices, stacks)];
    int vertices = slices * (stacks - 1) + 1;
    int indices = slices * 2 * 3 * (stacks - 2) + slices * 3;
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i < stacks - 1; i++) {
            float phi = PIf * 0.5f * i / (stacks - 1);
            float z = sinf(phi), radius = cosf(phi);
            
            for (int j = 0; j < slices; j++) {
                float theta = PIf * 2.0f * j / slices;

                *(vertex++) = sinf(theta) * radius;
                *(vertex++) = cosf(theta) * radius;
                *(vertex++) = z;
            }
        }
        *(vertex++) = 0.0f;
        *(vertex++) = 0.0f;
        *(vertex++) = 1.0f;
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 2; i++) {
            GLushort bottom = i * slices;
            GLushort top = bottom + slices;
            for (int j = 0; j < slices; j++) {
                int next = (j + 1) % slices;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        GLushort bottom = (stacks - 2) * slices;
        GLushort top = bottom + slices;
        for (int i = 0; i < slices; i++) {    
            *(index++) = bottom + i;
            *(index++) = bottom + (i + 1) % slices;
            *(index++) = top;
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);
 
    glVertexPointer(3, GL_FLOAT, 0, 0);
    glNormalPointer(GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderSquare(int xDivisions, int yDivisions) {
    VerticesIndices& vbo = _squareVBOs[IntPair(xDivisions, yDivisions)];
    int xVertices = xDivisions + 1;
    int yVertices = yDivisions + 1;
    int vertices = xVertices * yVertices;
    int indices = 2 * 3 * xDivisions * yDivisions;
    if (vbo.first == 0) {
        GLfloat* vertexData = new GLfloat[vertices * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            for (int j = 0; j <= xDivisions; j++) {
                *(vertex++) = (float)j / xDivisions;
                *(vertex++) = y;
                *(vertex++) = 0.0f;
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < yDivisions; i++) {
            GLushort bottom = i * xVertices;
            GLushort top = bottom + xVertices;
            for (int j = 0; j < xDivisions; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
        
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
 
    // all vertices have the same normal
    glNormal3f(0.0f, 0.0f, 1.0f);
    
    glVertexPointer(3, GL_FLOAT, 0, 0);
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderHalfCylinder(int slices, int stacks) {
    VerticesIndices& vbo = _halfCylinderVBOs[IntPair(slices, stacks)];
    int vertices = (slices + 1) * stacks;
    int indices = 2 * 3 * slices * (stacks - 1);
    if (vbo.first == 0) {    
        GLfloat* vertexData = new GLfloat[vertices * 2 * 3];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= (stacks - 1); i++) {
            float y = (float)i / (stacks - 1);
            
            for (int j = 0; j <= slices; j++) {
                float theta = 3 * PIf / 2 + PIf * j / slices;

                //normals
                *(vertex++) = sinf(theta);
                *(vertex++) = 0;
                *(vertex++) = cosf(theta);

                // vertices
                *(vertex++) = sinf(theta);
                *(vertex++) = y;
                *(vertex++) = cosf(theta);
            }
        }
        
        glGenBuffers(1, &vbo.first);
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        const int BYTES_PER_VERTEX = 3 * sizeof(GLfloat);
        glBufferData(GL_ARRAY_BUFFER, 2 * vertices * BYTES_PER_VERTEX, vertexData, GL_STATIC_DRAW);
        delete[] vertexData;
        
        GLushort* indexData = new GLushort[indices];
        GLushort* index = indexData;
        for (int i = 0; i < stacks - 1; i++) {
            GLushort bottom = i * (slices + 1);
            GLushort top = bottom + slices + 1;
            for (int j = 0; j < slices; j++) {
                int next = j + 1;
                
                *(index++) = bottom + j;
                *(index++) = top + next;
                *(index++) = top + j;
                
                *(index++) = bottom + j;
                *(index++) = bottom + next;
                *(index++) = top + next;
            }
        }
        
        glGenBuffers(1, &vbo.second);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
        const int BYTES_PER_INDEX = sizeof(GLushort);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices * BYTES_PER_INDEX, indexData, GL_STATIC_DRAW);
        delete[] indexData;
    
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, vbo.first);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo.second);
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_NORMAL_ARRAY);

    glNormalPointer(GL_FLOAT, 6 * sizeof(float), 0);
    glVertexPointer(3, GL_FLOAT, (6 * sizeof(float)), (const void *)(3 * sizeof(float)));
        
    glDrawRangeElementsEXT(GL_TRIANGLES, 0, vertices - 1, indices, GL_UNSIGNED_SHORT, 0);
        
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_NORMAL_ARRAY);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GeometryCache::renderGrid(int xDivisions, int yDivisions) {
    QOpenGLBuffer& buffer = _gridBuffers[IntPair(xDivisions, yDivisions)];
    int vertices = (xDivisions + 1 + yDivisions + 1) * 2;
    if (!buffer.isCreated()) {
        GLfloat* vertexData = new GLfloat[vertices * 2];
        GLfloat* vertex = vertexData;
        for (int i = 0; i <= xDivisions; i++) {
            float x = (float)i / xDivisions;
        
            *(vertex++) = x;
            *(vertex++) = 0.0f;
            
            *(vertex++) = x;
            *(vertex++) = 1.0f;
        }
        for (int i = 0; i <= yDivisions; i++) {
            float y = (float)i / yDivisions;
            
            *(vertex++) = 0.0f;
            *(vertex++) = y;
            
            *(vertex++) = 1.0f;
            *(vertex++) = y;
        }
        buffer.create();
        buffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        buffer.bind();
        buffer.allocate(vertexData, vertices * 2 * sizeof(GLfloat));
        delete[] vertexData;
        
    } else {
        buffer.bind();
    }
    glEnableClientState(GL_VERTEX_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, 0, 0);
    
    glDrawArrays(GL_LINES, 0, vertices);
    
    glDisableClientState(GL_VERTEX_ARRAY);
    
    buffer.release();
}

QSharedPointer<NetworkGeometry> GeometryCache::getGeometry(const QUrl& url, const QUrl& fallback) {
    if (!url.isValid() && fallback.isValid()) {
        return getGeometry(fallback);
    }
    QSharedPointer<NetworkGeometry> geometry = _networkGeometry.value(url);
    if (geometry.isNull()) {
        geometry = QSharedPointer<NetworkGeometry>(new NetworkGeometry(url, fallback.isValid() ?
            getGeometry(fallback) : QSharedPointer<NetworkGeometry>()));
        geometry->setLODParent(geometry);
        _networkGeometry.insert(url, geometry);
    }
    return geometry;
}

NetworkGeometry::NetworkGeometry(const QUrl& url, const QSharedPointer<NetworkGeometry>& fallback,
        const QVariantHash& mapping, const QUrl& textureBase) :
    _request(url),
    _reply(NULL),
    _mapping(mapping),
    _textureBase(textureBase.isValid() ? textureBase : url),
    _fallback(fallback),
    _attempts(0),
    _startedLoading(false),
    _failedToLoad(false) {
    
    if (!url.isValid()) {
        return;
    }
    _request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::PreferCache);
    
    // if we already have a mapping (because we're an LOD), hold off on loading until we're requested
    if (mapping.isEmpty()) {    
        makeRequest();
    }
}

NetworkGeometry::~NetworkGeometry() {
    if (_reply != NULL) {
        delete _reply;
    }
}

QSharedPointer<NetworkGeometry> NetworkGeometry::getLODOrFallback(float distance) const {
    if (_lodParent.data() != this) {
        return _lodParent.data()->getLODOrFallback(distance);
    }
    if (_failedToLoad && _fallback) {
        return _fallback;
    }
    QMap<float, QSharedPointer<NetworkGeometry> >::const_iterator it = _lods.upperBound(distance);
    QSharedPointer<NetworkGeometry> lod = (it == _lods.constBegin()) ? _lodParent.toStrongRef() : *(it - 1);
    if (lod->isLoaded()) {
        return lod;
    }
    // if the ideal LOD isn't loaded, we need to make sure it's started to load, and possibly return the closest loaded one
    if (!lod->_startedLoading) {
        lod->makeRequest();
    }
    float closestDistance = FLT_MAX;
    if (isLoaded()) {
        lod = _lodParent;
        closestDistance = distance;
    }
    for (it = _lods.constBegin(); it != _lods.constEnd(); it++) {
        float distanceToLOD = glm::abs(distance - it.key());
        if (it.value()->isLoaded() && distanceToLOD < closestDistance) {
            lod = it.value();
            closestDistance = distanceToLOD;
        }    
    }
    return lod;
}

glm::vec4 NetworkGeometry::computeAverageColor() const {
    glm::vec4 totalColor;
    int totalTriangles = 0;
    for (int i = 0; i < _meshes.size(); i++) {
        const FBXMesh& mesh = _geometry.meshes.at(i);
        if (mesh.isEye) {
            continue; // skip eyes
        }
        const NetworkMesh& networkMesh = _meshes.at(i);
        for (int j = 0; j < mesh.parts.size(); j++) {
            const FBXMeshPart& part = mesh.parts.at(j);
            const NetworkMeshPart& networkPart = networkMesh.parts.at(j);
            glm::vec4 color = glm::vec4(part.diffuseColor, 1.0f);
            if (networkPart.diffuseTexture) {
                color *= networkPart.diffuseTexture->getAverageColor();
            }
            int triangles = part.quadIndices.size() * 2 + part.triangleIndices.size();
            totalColor += color * triangles;
            totalTriangles += triangles;
        }
    }
    return (totalTriangles == 0) ? glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) : totalColor / totalTriangles;
}

void NetworkGeometry::makeRequest() {
    _startedLoading = true;
    _reply = Application::getInstance()->getNetworkAccessManager()->get(_request);
    
    connect(_reply, SIGNAL(downloadProgress(qint64,qint64)), SLOT(handleDownloadProgress(qint64,qint64)));
    connect(_reply, SIGNAL(error(QNetworkReply::NetworkError)), SLOT(handleReplyError()));
}

void NetworkGeometry::handleDownloadProgress(qint64 bytesReceived, qint64 bytesTotal) {
    if (!_reply->isFinished()) {
        return;
    }
    
    QUrl url = _reply->url();
    QByteArray data = _reply->readAll();
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    
    if (url.path().toLower().endsWith(".fst")) {
        // it's a mapping file; parse it and get the mesh filename
        _mapping = readMapping(data);
        QString filename = _mapping.value("filename").toString();
        if (filename.isNull()) {
            qDebug() << "Mapping file " << url << " has no filename.";
            _failedToLoad = true;
            
        } else {
            QString texdir = _mapping.value("texdir").toString();
            if (!texdir.isNull()) {
                if (!texdir.endsWith('/')) {
                    texdir += '/';
                }
                _textureBase = url.resolved(texdir);
            }
            QVariantHash lods = _mapping.value("lod").toHash();
            for (QVariantHash::const_iterator it = lods.begin(); it != lods.end(); it++) {
                QSharedPointer<NetworkGeometry> geometry(new NetworkGeometry(url.resolved(it.key()),
                    QSharedPointer<NetworkGeometry>(), _mapping, _textureBase));    
                geometry->setLODParent(_lodParent);
                _lods.insert(it.value().toFloat(), geometry);
            }     
            _request.setUrl(url.resolved(filename));
            
            // make the request immediately only if we have no LODs to switch between
            _startedLoading = false;
            if (_lods.isEmpty()) {
                makeRequest();
            }
        }
        return;
    }
    
    try {
        _geometry = url.path().toLower().endsWith(".svo") ? readSVO(data) : readFBX(data, _mapping);
        
    } catch (const QString& error) {
        qDebug() << "Error reading " << url << ": " << error;
        _failedToLoad = true;
        return;
    }
    
    foreach (const FBXMesh& mesh, _geometry.meshes) {
        NetworkMesh networkMesh = { QOpenGLBuffer(QOpenGLBuffer::IndexBuffer), QOpenGLBuffer(QOpenGLBuffer::VertexBuffer) };
        
        int totalIndices = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            NetworkMeshPart networkPart;
            if (!part.diffuseFilename.isEmpty()) {
                networkPart.diffuseTexture = Application::getInstance()->getTextureCache()->getTexture(
                    _textureBase.resolved(QUrl(part.diffuseFilename)), false, mesh.isEye);
            }
            if (!part.normalFilename.isEmpty()) {
                networkPart.normalTexture = Application::getInstance()->getTextureCache()->getTexture(
                    _textureBase.resolved(QUrl(part.normalFilename)), true);
            }
            networkMesh.parts.append(networkPart);
                        
            totalIndices += (part.quadIndices.size() + part.triangleIndices.size());
        }
        
        networkMesh.indexBuffer.create();
        networkMesh.indexBuffer.bind();
        networkMesh.indexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        networkMesh.indexBuffer.allocate(totalIndices * sizeof(int));
        int offset = 0;
        foreach (const FBXMeshPart& part, mesh.parts) {
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.quadIndices.size() * sizeof(int),
                part.quadIndices.constData());
            offset += part.quadIndices.size() * sizeof(int);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, offset, part.triangleIndices.size() * sizeof(int),
                part.triangleIndices.constData());
            offset += part.triangleIndices.size() * sizeof(int);
        }
        networkMesh.indexBuffer.release();
        
        networkMesh.vertexBuffer.create();
        networkMesh.vertexBuffer.bind();
        networkMesh.vertexBuffer.setUsagePattern(QOpenGLBuffer::StaticDraw);
        
        // if we don't need to do any blending or springing, then the positions/normals can be static
        if (mesh.blendshapes.isEmpty() && mesh.springiness == 0.0f) {
            int normalsOffset = mesh.vertices.size() * sizeof(glm::vec3);
            int tangentsOffset = normalsOffset + mesh.normals.size() * sizeof(glm::vec3);
            int colorsOffset = tangentsOffset + mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            
            networkMesh.vertexBuffer.allocate(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(0, mesh.vertices.constData(), mesh.vertices.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(normalsOffset, mesh.normals.constData(), mesh.normals.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(tangentsOffset, mesh.tangents.constData(),
                mesh.tangents.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(colorsOffset, mesh.colors.constData(), mesh.colors.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(texCoordsOffset, mesh.texCoords.constData(),
                mesh.texCoords.size() * sizeof(glm::vec2));
            networkMesh.vertexBuffer.write(clusterIndicesOffset, mesh.clusterIndices.constData(),
                mesh.clusterIndices.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(clusterWeightsOffset, mesh.clusterWeights.constData(),
                mesh.clusterWeights.size() * sizeof(glm::vec4));
        
        // if there's no springiness, then the cluster indices/weights can be static
        } else if (mesh.springiness == 0.0f) {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            int clusterIndicesOffset = texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2);
            int clusterWeightsOffset = clusterIndicesOffset + mesh.clusterIndices.size() * sizeof(glm::vec4);
            networkMesh.vertexBuffer.allocate(clusterWeightsOffset + mesh.clusterWeights.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(0, mesh.tangents.constData(), mesh.tangents.size() * sizeof(glm::vec3));        
            networkMesh.vertexBuffer.write(colorsOffset, mesh.colors.constData(), mesh.colors.size() * sizeof(glm::vec3));    
            networkMesh.vertexBuffer.write(texCoordsOffset, mesh.texCoords.constData(),
                mesh.texCoords.size() * sizeof(glm::vec2));
            networkMesh.vertexBuffer.write(clusterIndicesOffset, mesh.clusterIndices.constData(),
                mesh.clusterIndices.size() * sizeof(glm::vec4));
            networkMesh.vertexBuffer.write(clusterWeightsOffset, mesh.clusterWeights.constData(),
                mesh.clusterWeights.size() * sizeof(glm::vec4));
            
        } else {
            int colorsOffset = mesh.tangents.size() * sizeof(glm::vec3);
            int texCoordsOffset = colorsOffset + mesh.colors.size() * sizeof(glm::vec3);
            networkMesh.vertexBuffer.allocate(texCoordsOffset + mesh.texCoords.size() * sizeof(glm::vec2));
            networkMesh.vertexBuffer.write(0, mesh.tangents.constData(), mesh.tangents.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(colorsOffset, mesh.colors.constData(), mesh.colors.size() * sizeof(glm::vec3));
            networkMesh.vertexBuffer.write(texCoordsOffset, mesh.texCoords.constData(),
                mesh.texCoords.size() * sizeof(glm::vec2));
        }
        
        networkMesh.vertexBuffer.release();
        
        _meshes.append(networkMesh);
    }
}

void NetworkGeometry::handleReplyError() {
    QDebug debug = qDebug() << _reply->errorString();
    
    QNetworkReply::NetworkError error = _reply->error();
    _reply->disconnect(this);
    _reply->deleteLater();
    _reply = NULL;
    
    // retry for certain types of failures
    switch (error) {
        case QNetworkReply::RemoteHostClosedError:
        case QNetworkReply::TimeoutError:
        case QNetworkReply::TemporaryNetworkFailureError:
        case QNetworkReply::ProxyConnectionClosedError:
        case QNetworkReply::ProxyTimeoutError:
        case QNetworkReply::UnknownNetworkError:
        case QNetworkReply::UnknownProxyError:
        case QNetworkReply::UnknownContentError:
        case QNetworkReply::ProtocolFailure: {        
            // retry with increasing delays
            const int MAX_ATTEMPTS = 8;
            const int BASE_DELAY_MS = 1000;
            if (++_attempts < MAX_ATTEMPTS) {
                QTimer::singleShot(BASE_DELAY_MS * (int)pow(2.0, _attempts), this, SLOT(makeRequest()));
                debug << " -- retrying...";
                return;
            }
            // fall through to final failure
        }    
        default:
            _failedToLoad = true;
            break;
    }
    
}

bool NetworkMeshPart::isTranslucent() const {
    return diffuseTexture && diffuseTexture->isTranslucent();
}

int NetworkMesh::getTranslucentPartCount() const {
    int count = 0;
    foreach (const NetworkMeshPart& part, parts) {
        if (part.isTranslucent()) {
            count++;
        }
    }
    return count;
}
