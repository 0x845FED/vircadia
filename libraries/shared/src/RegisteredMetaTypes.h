//
//  RegisteredMetaTypes.h
//  libraries/shared/src
//
//  Created by Stephen Birarda on 10/3/13.
//  Copyright 2013 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RegisteredMetaTypes_h
#define hifi_RegisteredMetaTypes_h

#include <QtScript/QScriptEngine>
#include <QtCore/QUuid>
#include <QtCore/QUrl>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "AACube.h"
#include "SharedUtil.h"
#include "shared/Bilateral.h"

#include <QJsonDocument>
#include <QJsonObject>
#include "shared/JSONHelpers.h"

class QColor;
class QUrl;

Q_DECLARE_METATYPE(glm::vec4)
Q_DECLARE_METATYPE(glm::quat)
Q_DECLARE_METATYPE(glm::mat4)
Q_DECLARE_METATYPE(QVector<float>)
Q_DECLARE_METATYPE(AACube)
Q_DECLARE_METATYPE(std::function<void()>);
Q_DECLARE_METATYPE(std::function<QVariant()>);

void registerMetaTypes(QScriptEngine* engine);

// Mat4
QScriptValue mat4toScriptValue(QScriptEngine* engine, const glm::mat4& mat4);
void mat4FromScriptValue(const QScriptValue& object, glm::mat4& mat4);

/**jsdoc
* A 2-dimensional vector.
*
* @typedef {object} Vec2
* @property {number} x - X-coordinate of the vector. Synonyms: <code>u</code> and <code>width</code>.
* @property {number} y - Y-coordinate of the vector. Synonyms: <code>v</code> and <code>height</code>.
*/
class ScriptVec2Float : public QObject {
    Q_OBJECT
    Q_PROPERTY(float x MEMBER x)
    Q_PROPERTY(float y MEMBER y)
    Q_PROPERTY(float u MEMBER x)
    Q_PROPERTY(float v MEMBER y)
    Q_PROPERTY(float width MEMBER x)
    Q_PROPERTY(float height MEMBER y)
public:
    ScriptVec2Float() {}
    ScriptVec2Float(float xy) : x(xy), y(xy) {}
    ScriptVec2Float(float x, float y) : x(x), y(y) {}
    ScriptVec2Float(const ScriptVec2Float& other) : x(other.x), y(other.y) {}
    ScriptVec2Float(const glm::vec2& other) : x(other.x), y(other.y) {}
    void operator=(const ScriptVec2Float& other) { x = other.x; y = other.y; }
    inline bool operator==(const ScriptVec2Float& other) const { return (x == other.x && y == other.y); }
    inline bool operator!=(const ScriptVec2Float& other) const { return !(*this == other); }
    inline bool operator==(const glm::vec2& other) const { return (x == other.x && y == other.y); }
    inline bool operator!=(const glm::vec2& other) const { return !(*this == other); }

    glm::vec2 toGlm() const { return glm::vec2(x, y); }
    Q_INVOKABLE QVariantMap toJSON() { return toJsonValue(*this, { "x", "y" }).toObject().toVariantMap(); }

    float x { 0.0f };
    float y { 0.0f };
private:
    friend QDebug operator<<(QDebug debug, const ScriptVec2Float& vec2);
    friend bool operator==(glm::vec2 glmVec2, const ScriptVec2Float& vec2);
    friend bool operator!=(glm::vec2 glmVec2, const ScriptVec2Float& vec2);
};
inline QDebug operator<<(QDebug debug, const ScriptVec2Float& vec2) {
    debug << "(" << vec2.x << "," << vec2.y << ")";
    return debug;
}
inline bool operator==(glm::vec2 glmVec2, const ScriptVec2Float& vec2) { return (glmVec2.x == vec2.x && glmVec2.y == vec2.y); }
inline bool operator!=(glm::vec2 glmVec2, const ScriptVec2Float& vec2) { return !(glmVec2 == vec2); }
Q_DECLARE_METATYPE(ScriptVec2Float)
QScriptValue vec2FloatToScriptValue(QScriptEngine* engine, const ScriptVec2Float& vec2);
void vec2FloatFromScriptValue(const QScriptValue& object, ScriptVec2Float& vec2);

Q_DECLARE_METATYPE(glm::vec2)
QScriptValue vec2ToScriptValue(QScriptEngine* engine, const glm::vec2& vec2);
void vec2FromScriptValue(const QScriptValue& object, glm::vec2& vec2);

QVariant vec2ToVariant(const glm::vec2& vec2);
glm::vec2 vec2FromVariant(const QVariant& object, bool& valid);
glm::vec2 vec2FromVariant(const QVariant& object);

/**jsdoc
* A 3-dimensional vector. See also the {@link Vec3(0)|Vec3} object.
*
* @typedef {object} Vec3
* @property {number} x - X-coordinate of the vector. Synonyms: <code>r</code> and <code>red</code>.
* @property {number} y - Y-coordinate of the vector. Synonyms: <code>g</code> and <code>green</code>.
* @property {number} z - Z-coordinate of the vector. Synonyms: <code>b</code> and <code>blue</code>.
*/
class ScriptVec3Float : public QObject {
    Q_OBJECT
    Q_PROPERTY(float x MEMBER x)
    Q_PROPERTY(float y MEMBER y)
    Q_PROPERTY(float z MEMBER z)
    Q_PROPERTY(float r MEMBER x)
    Q_PROPERTY(float g MEMBER y)
    Q_PROPERTY(float b MEMBER z)
    Q_PROPERTY(float red MEMBER x)
    Q_PROPERTY(float green MEMBER y)
    Q_PROPERTY(float blue MEMBER z)
public:
    ScriptVec3Float() {}
    ScriptVec3Float(float xyz) : x(xyz), y(xyz), z(xyz) {}
    ScriptVec3Float(float x, float y, float z) : x(x), y(y), z(z) {}
    ScriptVec3Float(const ScriptVec3Float& other) : x(other.x), y(other.y), z(other.z) {}
    ScriptVec3Float(const glm::vec3& other) : x(other.x), y(other.y), z(other.z) {}
    void operator=(const ScriptVec3Float& other) { x = other.x; y = other.y; z = other.z; }
    inline bool operator==(const ScriptVec3Float& other) const { return (x == other.x && y == other.y && z == other.z); }
    inline bool operator!=(const ScriptVec3Float& other) const { return !(*this == other); }
    inline bool operator==(const glm::vec3& other) const { return (x == other.x && y == other.y && z == other.z); }
    inline bool operator!=(const glm::vec3& other) const { return !(*this == other); }

    glm::vec3 toGlm() const { return glm::vec3(x, y, z); }
    Q_INVOKABLE QVariantMap toJSON() { return toJsonValue(*this, { "x", "y", "z" }).toObject().toVariantMap(); }

    float x { 0.0f };
    float y { 0.0f };
    float z { 0.0f };
private:
    friend QDebug operator<<(QDebug debug, const ScriptVec3Float& vec3);
    friend bool operator==(glm::vec3 glmVec3, const ScriptVec3Float& vec3);
    friend bool operator!=(glm::vec3 glmVec3, const ScriptVec3Float& vec3);
};
inline QDebug operator<<(QDebug debug, const ScriptVec3Float& vec3) {
    debug << "(" << vec3.x << "," << vec3.y << "," << vec3.z << ")";
    return debug;
}
inline bool operator==(glm::vec3 glmVec3, const ScriptVec3Float& vec3) { return (glmVec3.x == vec3.x && glmVec3.y == vec3.y && glmVec3.z == vec3.z); }
inline bool operator!=(glm::vec3 glmVec3, const ScriptVec3Float& vec3) { return !(glmVec3 == vec3); }
Q_DECLARE_METATYPE(ScriptVec3Float)
QScriptValue vec3FloatToScriptValue(QScriptEngine* engine, const ScriptVec3Float& vec3);
void vec3FloatFromScriptValue(const QScriptValue& object, ScriptVec3Float& vec3);

/**jsdoc
* A color vector. See also the {@link Vec3(0)|Vec3} object.
*
* @typedef {object} Vec3Color
* @property {number} x - Red component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>r</code> and <code>red</code>.
* @property {number} y - Green component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>g</code> and <code>green</code>.
* @property {number} z - Blue component value. Integer in the range <code>0</code> - <code>255</code>.  Synonyms: <code>b</code> and <code>blue</code>.
*/
class ScriptVec3UChar : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned int x MEMBER x)
    Q_PROPERTY(unsigned int y MEMBER y)
    Q_PROPERTY(unsigned int z MEMBER z)
    Q_PROPERTY(unsigned int r MEMBER x)
    Q_PROPERTY(unsigned int g MEMBER y)
    Q_PROPERTY(unsigned int b MEMBER z)
    Q_PROPERTY(unsigned int red MEMBER x)
    Q_PROPERTY(unsigned int green MEMBER y)
    Q_PROPERTY(unsigned int blue MEMBER z)
public:
    ScriptVec3UChar() {}
    ScriptVec3UChar(unsigned int xyz) : x(xyz), y(xyz), z(xyz) {}
    ScriptVec3UChar(unsigned int x, unsigned int y, unsigned int z) : x(x), y(y), z(z) {}
    ScriptVec3UChar(const ScriptVec3UChar& other) : x(other.x), y(other.y), z(other.z) {}
    ScriptVec3UChar(const glm::vec3& other) : x(other.x), y(other.y), z(other.z) {}
    void operator=(const ScriptVec3UChar& other) { x = other.x; y = other.y; z = other.z; }
    inline bool operator==(const ScriptVec3UChar& other) const { return (x == other.x && y == other.y && z == other.z); }
    inline bool operator!=(const ScriptVec3UChar& other) const { return !(*this == other); }

    glm::vec3 toGlm() const { return glm::vec3(x, y, z); }
    Q_INVOKABLE QVariantMap toJSON() { return toJsonValue(*this, { "x", "y", "z" }).toObject().toVariantMap(); }

    unsigned char x { 0 };
    unsigned char y { 0 };
    unsigned char z { 0 };
private:
    friend QDebug operator<<(QDebug debug, const ScriptVec3UChar& vec3);
    friend bool operator==(glm::vec3 glmVec3, const ScriptVec3UChar& vec3);
    friend bool operator!=(glm::vec3 glmVec3, const ScriptVec3UChar& vec3);
};
inline QDebug operator<<(QDebug debug, const ScriptVec3UChar& vec3) {
    debug << "(" << vec3.x << "," << vec3.y << "," << vec3.z << ")";
    return debug;
}
inline bool operator==(glm::vec3 glmVec3, const ScriptVec3UChar& vec3) { return (glmVec3.x == vec3.x && glmVec3.y == vec3.y && glmVec3.z == vec3.z); }
inline bool operator!=(glm::vec3 glmVec3, const ScriptVec3UChar& vec3) { return !(glmVec3 == vec3); }
Q_DECLARE_METATYPE(ScriptVec3UChar)
QScriptValue vec3UCharToScriptValue(QScriptEngine* engine, const ScriptVec3UChar& vec3);
void vec3UCharFromScriptValue(const QScriptValue& object, ScriptVec3UChar& vec3);

Q_DECLARE_METATYPE(glm::vec3)
QScriptValue vec3ToScriptValue(QScriptEngine* engine, const glm::vec3 &vec3);
void vec3FromScriptValue(const QScriptValue &object, glm::vec3 &vec3);

QVariant vec3ToVariant(const glm::vec3& vec3);
glm::vec3 vec3FromVariant(const QVariant &object, bool& valid);
glm::vec3 vec3FromVariant(const QVariant &object);

/**jsdoc
 * A 4-dimensional vector.
 *
 * @typedef {object} Vec4
 * @property {number} x - X-coordinate of the vector.
 * @property {number} y - Y-coordinate of the vector.
 * @property {number} z - Z-coordinate of the vector.
 * @property {number} w - W-coordinate of the vector.
 */
QScriptValue vec4toScriptValue(QScriptEngine* engine, const glm::vec4& vec4);
void vec4FromScriptValue(const QScriptValue& object, glm::vec4& vec4);
QVariant vec4toVariant(const glm::vec4& vec4);
glm::vec4 vec4FromVariant(const QVariant &object, bool& valid);
glm::vec4 vec4FromVariant(const QVariant &object);

// Quaternions
QScriptValue quatToScriptValue(QScriptEngine* engine, const glm::quat& quat);
void quatFromScriptValue(const QScriptValue &object, glm::quat& quat);

QVariant quatToVariant(const glm::quat& quat);
glm::quat quatFromVariant(const QVariant &object, bool& isValid);
glm::quat quatFromVariant(const QVariant &object);

// Rect
QScriptValue qRectToScriptValue(QScriptEngine* engine, const QRect& rect);
void qRectFromScriptValue(const QScriptValue& object, QRect& rect);
QRect qRectFromVariant(const QVariant& object, bool& isValid);
QRect qRectFromVariant(const QVariant& object);
QVariant qRectToVariant(const QRect& rect);

// QColor
QScriptValue qColorToScriptValue(QScriptEngine* engine, const QColor& color);
void qColorFromScriptValue(const QScriptValue& object, QColor& color);

QScriptValue qURLToScriptValue(QScriptEngine* engine, const QUrl& url);
void qURLFromScriptValue(const QScriptValue& object, QUrl& url);

// vector<vec3>
Q_DECLARE_METATYPE(QVector<ScriptVec3Float>)
QScriptValue qVectorVec3ToScriptValue(QScriptEngine* engine, const QVector<ScriptVec3Float>& vector);
void qVectorVec3FromScriptValue(const QScriptValue& array, QVector<ScriptVec3Float>& vector);
QVector<ScriptVec3Float> qVectorVec3FromScriptValue(const QScriptValue& array);

// vector<quat>
QScriptValue qVectorQuatToScriptValue(QScriptEngine* engine, const QVector<glm::quat>& vector);
void qVectorQuatFromScriptValue(const QScriptValue& array, QVector<glm::quat>& vector);
QVector<glm::quat> qVectorQuatFromScriptValue(const QScriptValue& array);

// vector<bool>
QScriptValue qVectorBoolToScriptValue(QScriptEngine* engine, const QVector<bool>& vector);
void qVectorBoolFromScriptValue(const QScriptValue& array, QVector<bool>& vector);
QVector<bool> qVectorBoolFromScriptValue(const QScriptValue& array);

// vector<float>
QScriptValue qVectorFloatToScriptValue(QScriptEngine* engine, const QVector<float>& vector);
void qVectorFloatFromScriptValue(const QScriptValue& array, QVector<float>& vector);
QVector<float> qVectorFloatFromScriptValue(const QScriptValue& array);

// vector<uint32_t>
QScriptValue qVectorIntToScriptValue(QScriptEngine* engine, const QVector<uint32_t>& vector);
void qVectorIntFromScriptValue(const QScriptValue& array, QVector<uint32_t>& vector);

QVector<QUuid> qVectorQUuidFromScriptValue(const QScriptValue& array);

QScriptValue aaCubeToScriptValue(QScriptEngine* engine, const AACube& aaCube);
void aaCubeFromScriptValue(const QScriptValue &object, AACube& aaCube);

// MathPicks also have to overide operator== for their type
class MathPick {
public:
    virtual ~MathPick() {}
    virtual operator bool() const = 0;
    virtual QVariantMap toVariantMap() const = 0;
};

/**jsdoc
 * A PickRay defines a vector with a starting point. It is used, for example, when finding entities or overlays that lie under a
 * mouse click or intersect a laser beam.
 *
 * @typedef {object} PickRay
 * @property {Vec3} origin - The starting position of the PickRay.
 * @property {Vec3} direction - The direction that the PickRay travels.
 */
class PickRay : public MathPick {
public:
    PickRay() : origin(NAN), direction(NAN)  { }
    PickRay(const QVariantMap& pickVariant) : origin(vec3FromVariant(pickVariant["origin"])), direction(vec3FromVariant(pickVariant["direction"])) {}
    PickRay(const glm::vec3& origin, const glm::vec3 direction) : origin(origin), direction(direction) {}
    glm::vec3 origin;
    glm::vec3 direction;

    operator bool() const override {
        return !(glm::any(glm::isnan(origin)) || glm::any(glm::isnan(direction)));
    }
    bool operator==(const PickRay& other) const {
        return (origin == other.origin && direction == other.direction);
    }
    QVariantMap toVariantMap() const override {
        QVariantMap pickRay;
        pickRay["origin"] = vec3ToVariant(origin);
        pickRay["direction"] = vec3ToVariant(direction);
        return pickRay;
    }
};
Q_DECLARE_METATYPE(PickRay)
QScriptValue pickRayToScriptValue(QScriptEngine* engine, const PickRay& pickRay);
void pickRayFromScriptValue(const QScriptValue& object, PickRay& pickRay);

/**jsdoc
 * A StylusTip defines the tip of a stylus.
 *
 * @typedef {object} StylusTip
 * @property {number} side - The hand the tip is attached to: <code>0</code> for left, <code>1</code> for right.
 * @property {Vec3} position - The position of the stylus tip.
 * @property {Quat} orientation - The orientation of the stylus tip.
 * @property {Vec3} velocity - The velocity of the stylus tip.
 */
class StylusTip : public MathPick {
public:
    StylusTip() : position(NAN), velocity(NAN) {}
    StylusTip(const QVariantMap& pickVariant) : side(bilateral::Side(pickVariant["side"].toInt())), position(vec3FromVariant(pickVariant["position"])),
        orientation(quatFromVariant(pickVariant["orientation"])), velocity(vec3FromVariant(pickVariant["velocity"])) {}

    bilateral::Side side { bilateral::Side::Invalid };
    glm::vec3 position;
    glm::quat orientation;
    glm::vec3 velocity;

    operator bool() const override { return side != bilateral::Side::Invalid; }

    bool operator==(const StylusTip& other) const {
        return (side == other.side && position == other.position && orientation == other.orientation && velocity == other.velocity);
    }

    QVariantMap toVariantMap() const override {
        QVariantMap stylusTip;
        stylusTip["side"] = (int)side;
        stylusTip["position"] = vec3ToVariant(position);
        stylusTip["orientation"] = quatToVariant(orientation);
        stylusTip["velocity"] = vec3ToVariant(velocity);
        return stylusTip;
    }
};


namespace std {
    inline void hash_combine(std::size_t& seed) { }

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
        hash_combine(seed, rest...);
    }

    template <>
    struct hash<bilateral::Side> {
        size_t operator()(const bilateral::Side& a) const {
            return std::hash<int>()((int)a);
        }
    };

    template <>
    struct hash<glm::vec3> {
        size_t operator()(const glm::vec3& a) const {
            size_t result = 0;
            hash_combine(result, a.x, a.y, a.z);
            return result;
        }
    };

    template <>
    struct hash<glm::quat> {
        size_t operator()(const glm::quat& a) const {
            size_t result = 0;
            hash_combine(result, a.x, a.y, a.z, a.w);
            return result;
        }
    };

    template <>
    struct hash<PickRay> {
        size_t operator()(const PickRay& a) const {
            size_t result = 0;
            hash_combine(result, a.origin, a.direction);
            return result;
        }
    };

    template <>
    struct hash<StylusTip> {
        size_t operator()(const StylusTip& a) const {
            size_t result = 0;
            hash_combine(result, a.side, a.position, a.orientation, a.velocity);
            return result;
        }
    };

    template <>
    struct hash<QString> {
        size_t operator()(const QString& a) const {
            return qHash(a);
        }
    };
}

/**jsdoc
 * <p>The type of a collision contact event.
 * <table>
 *   <thead>
 *     <tr><th>Value</th><th>Description</th></tr>
 *   </thead>
 *   <tbody>
 *     <tr><td><code>0</code></td><td>Start of the collision.</td></tr>
 *     <tr><td><code>1</code></td><td>Continuation of the collision.</td></tr>
 *     <tr><td><code>2</code></td><td>End of the collision.</td></tr>
 *   </tbody>
 * </table>
 * @typedef {number} ContactEventType
 */
enum ContactEventType {
    CONTACT_EVENT_TYPE_START,
    CONTACT_EVENT_TYPE_CONTINUE,
    CONTACT_EVENT_TYPE_END
};

class Collision {
public:
    Collision() : type(CONTACT_EVENT_TYPE_START), idA(), idB(), contactPoint(0.0f), penetration(0.0f), velocityChange(0.0f) { }
    Collision(ContactEventType cType, const QUuid& cIdA, const QUuid& cIdB, const glm::vec3& cPoint,
              const glm::vec3& cPenetration, const glm::vec3& velocityChange)
    :   type(cType), idA(cIdA), idB(cIdB), contactPoint(cPoint), penetration(cPenetration), velocityChange(velocityChange) { }

    void invert(); // swap A and B

    ContactEventType type;
    QUuid idA;
    QUuid idB;
    glm::vec3 contactPoint; // on B in world-frame
    glm::vec3 penetration; // from B towards A in world-frame
    glm::vec3 velocityChange;
};
Q_DECLARE_METATYPE(Collision)
QScriptValue collisionToScriptValue(QScriptEngine* engine, const Collision& collision);
void collisionFromScriptValue(const QScriptValue &object, Collision& collision);

//Q_DECLARE_METATYPE(QUuid) // don't need to do this for QUuid since it's already a meta type
QScriptValue quuidToScriptValue(QScriptEngine* engine, const QUuid& uuid);
void quuidFromScriptValue(const QScriptValue& object, QUuid& uuid);

//Q_DECLARE_METATYPE(QSizeF) // Don't need to to this becase it's arleady a meta type
QScriptValue qSizeFToScriptValue(QScriptEngine* engine, const QSizeF& qSizeF);
void qSizeFFromScriptValue(const QScriptValue& object, QSizeF& qSizeF);

class AnimationDetails {
public:
    AnimationDetails();
    AnimationDetails(QString role, QUrl url, float fps, float priority, bool loop,
        bool hold, bool startAutomatically, float firstFrame, float lastFrame, bool running, float currentFrame, bool allowTranslation);

    QString role;
    QUrl url;
    float fps;
    float priority;
    bool loop;
    bool hold;
    bool startAutomatically;
    float firstFrame;
    float lastFrame;
    bool running;
    float currentFrame;
    bool allowTranslation;
};
Q_DECLARE_METATYPE(AnimationDetails);
QScriptValue animationDetailsToScriptValue(QScriptEngine* engine, const AnimationDetails& event);
void animationDetailsFromScriptValue(const QScriptValue& object, AnimationDetails& event);

namespace graphics {
    class Mesh;
}

using MeshPointer = std::shared_ptr<graphics::Mesh>;

/**jsdoc
 * A handle for a mesh in an entity, such as returned by {@link Entities.getMeshes}.
 * @class MeshProxy
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-server-entity
 * @hifi-assignment-client
 *
 * @deprecated Use the {@link Graphics} API instead.
 */
class MeshProxy : public QObject {
    Q_OBJECT

public:
    virtual MeshPointer getMeshPointer() const = 0;
    
    /**jsdoc
     * Get the number of vertices in the mesh.
     * @function MeshProxy#getNumVertices
     * @returns {number} Integer number of vertices in the mesh.
     * @deprecated Use the {@link Graphics} API instead.
     */
    Q_INVOKABLE virtual int getNumVertices() const = 0;

    /**jsdoc
     * Get the position of a vertex in the mesh.
     * @function MeshProxy#getPos
     * @param {number} index - Integer index of the mesh vertex.
     * @returns {Vec3} Local position of the vertex relative to the mesh.
     * @deprecated Use the {@link Graphics} API instead.
     */
    Q_INVOKABLE virtual glm::vec3 getPos(int index) const = 0;
    Q_INVOKABLE virtual glm::vec3 getPos3(int index) const { return getPos(index); } // deprecated
};

Q_DECLARE_METATYPE(MeshProxy*);

class MeshProxyList : public QList<MeshProxy*> {}; // typedef and using fight with the Qt macros/templates, do this instead
Q_DECLARE_METATYPE(MeshProxyList);


QScriptValue meshToScriptValue(QScriptEngine* engine, MeshProxy* const &in);
void meshFromScriptValue(const QScriptValue& value, MeshProxy* &out);

QScriptValue meshesToScriptValue(QScriptEngine* engine, const MeshProxyList &in);
void meshesFromScriptValue(const QScriptValue& value, MeshProxyList &out);

class MeshFace {

public:
    MeshFace() {}
    ~MeshFace() {}

    QVector<uint32_t> vertexIndices;
    // TODO -- material...
};

Q_DECLARE_METATYPE(MeshFace)
Q_DECLARE_METATYPE(QVector<MeshFace>)

QScriptValue meshFaceToScriptValue(QScriptEngine* engine, const MeshFace &meshFace);
void meshFaceFromScriptValue(const QScriptValue &object, MeshFace& meshFaceResult);
QScriptValue qVectorMeshFaceToScriptValue(QScriptEngine* engine, const QVector<MeshFace>& vector);
void qVectorMeshFaceFromScriptValue(const QScriptValue& array, QVector<MeshFace>& result);


#endif // hifi_RegisteredMetaTypes_h
