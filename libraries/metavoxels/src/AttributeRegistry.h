//
//  AttributeRegistry.h
//  metavoxels
//
//  Created by Andrzej Kapolka on 12/6/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__AttributeRegistry__
#define __interface__AttributeRegistry__

#include <QColor>
#include <QExplicitlySharedDataPointer>
#include <QHash>
#include <QObject>
#include <QSharedData>
#include <QSharedPointer>
#include <QString>
#include <QWidget>

#include "Bitstream.h"

class QComboBox;
class QFormLayout;
class QPushButton;
class QScriptContext;
class QScriptEngine;
class QScriptValue;

class Attribute;

typedef QSharedPointer<Attribute> AttributePointer;

/// Maintains information about metavoxel attribute types.
class AttributeRegistry {
public:
    
    /// Returns a pointer to the singleton registry instance.
    static AttributeRegistry* getInstance();
    
    AttributeRegistry();
    
    /// Configures the supplied script engine with the global AttributeRegistry property.
    void configureScriptEngine(QScriptEngine* engine);
    
    /// Registers an attribute with the system.  The registry assumes ownership of the object.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(Attribute* attribute) { return registerAttribute(AttributePointer(attribute)); }
    
    /// Registers an attribute with the system.
    /// \return either the pointer passed as an argument, if the attribute wasn't already registered, or the existing
    /// attribute
    AttributePointer registerAttribute(AttributePointer attribute);
    
    /// Retrieves an attribute by name.
    AttributePointer getAttribute(const QString& name) const { return _attributes.value(name); }
    
    /// Returns a reference to the attribute hash.
    const QHash<QString, AttributePointer>& getAttributes() const { return _attributes; }
    
    /// Returns a reference to the standard PolymorphicDataPointer "guide" attribute.
    const AttributePointer& getGuideAttribute() const { return _guideAttribute; }
    
    /// Returns a reference to the standard QRgb "color" attribute.
    const AttributePointer& getColorAttribute() const { return _colorAttribute; }
    
    /// Returns a reference to the standard QRgb "normal" attribute.
    const AttributePointer& getNormalAttribute() const { return _normalAttribute; }
    
private:

    static QScriptValue getAttribute(QScriptContext* context, QScriptEngine* engine);

    QHash<QString, AttributePointer> _attributes;
    AttributePointer _guideAttribute;
    AttributePointer _colorAttribute;
    AttributePointer _normalAttribute;
};

/// Converts a value to a void pointer.
template<class T> inline void* encodeInline(T value) {
    return *(void**)&value;
}

/// Extracts a value from a void pointer.
template<class T> inline T decodeInline(void* value) {
    return *(T*)&value;
}

/// Pairs an attribute value with its type.
class AttributeValue {
public:
    
    AttributeValue(const AttributePointer& attribute = AttributePointer());
    AttributeValue(const AttributePointer& attribute, void* value);
    
    AttributePointer getAttribute() const { return _attribute; }
    void* getValue() const { return _value; }
    
    template<class T> void setInlineValue(T value) { _value = encodeInline(value); }
    template<class T> T getInlineValue() const { return decodeInline<T>(_value); }
    
    template<class T> T* getPointerValue() const { return static_cast<T*>(_value); }
    
    void* copy() const;

    bool isDefault() const;

    bool operator==(const AttributeValue& other) const;
    bool operator==(void* other) const;
    
protected:
    
    AttributePointer _attribute;
    void* _value;
};

// Assumes ownership of an attribute value.
class OwnedAttributeValue : public AttributeValue {
public:
    
    /// Assumes ownership of the specified value.  It will be destroyed when this is destroyed or reassigned.
    OwnedAttributeValue(const AttributePointer& attribute, void* value);
    
    /// Creates an owned attribute with a copy of the specified attribute's default value.
    OwnedAttributeValue(const AttributePointer& attribute = AttributePointer());
    
    /// Creates an owned attribute with a copy of the specified other value.
    OwnedAttributeValue(const AttributeValue& other);
    
    /// Destroys the current value, if any.
    ~OwnedAttributeValue();
    
    /// Destroys the current value, if any, and copies the specified other value.
    OwnedAttributeValue& operator=(const AttributeValue& other);
};

/// Represents a registered attribute.
class Attribute : public QObject {
    Q_OBJECT
    
public:
    
    static const int MERGE_COUNT = 8;
    
    Attribute(const QString& name);
    virtual ~Attribute();

    Q_INVOKABLE QString getName() const { return objectName(); }

    void* create() const { return create(getDefaultValue()); }
    virtual void* create(void* copy) const = 0;
    virtual void destroy(void* value) const = 0;

    virtual void read(Bitstream& in, void*& value, bool isLeaf) const = 0;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const = 0;

    virtual void readDelta(Bitstream& in, void*& value, void* reference, bool isLeaf) const { read(in, value, isLeaf); }
    virtual void writeDelta(Bitstream& out, void* value, void* reference, bool isLeaf) const { write(out, value, isLeaf); }

    virtual bool equal(void* first, void* second) const = 0;

    /// Merges the value of a parent and its children.
    /// \return whether or not the children and parent values are all equal
    virtual bool merge(void*& parent, void* children[]) const = 0;

    virtual void* getDefaultValue() const = 0;

    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const { return create(); }
    
    virtual void* createFromVariant(const QVariant& value) const { return create(); }
    
    /// Creates a widget to use to edit values of this attribute, or returns NULL if the attribute isn't editable.
    /// The widget should have a single "user" property that will be used to get/set the value.
    virtual QWidget* createEditor(QWidget* parent = NULL) const { return NULL; }
};

/// A simple attribute class that stores its values inline.
template<class T, int bits = 32> class InlineAttribute : public Attribute {
public:
    
    InlineAttribute(const QString& name, const T& defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { void* value; new (&value) T(*(T*)&copy); return value; }
    virtual void destroy(void* value) const { ((T*)&value)->~T(); }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool equal(void* first, void* second) const { return decodeInline<T>(first) == decodeInline<T>(second); }

    virtual void* getDefaultValue() const { return encodeInline(_defaultValue); }

protected:
    
    T _defaultValue;
};

template<class T, int bits> inline void InlineAttribute<T, bits>::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        value = getDefaultValue();
        in.read(&value, bits);
    }
}

template<class T, int bits> inline void InlineAttribute<T, bits>::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        out.write(&value, bits);
    }
}

/// Provides merging using the =, ==, += and /= operators.
template<class T, int bits = 32> class SimpleInlineAttribute : public InlineAttribute<T, bits> {
public:
    
    SimpleInlineAttribute(const QString& name, T defaultValue = T()) : InlineAttribute<T, bits>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[]) const;
};

template<class T, int bits> inline bool SimpleInlineAttribute<T, bits>::merge(void*& parent, void* children[]) const {
    T& merged = *(T*)&parent;
    merged = decodeInline<T>(children[0]);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        merged += decodeInline<T>(children[i]);
        allChildrenEqual &= (decodeInline<T>(children[0]) == decodeInline<T>(children[i]));
    }
    merged /= Attribute::MERGE_COUNT;
    return allChildrenEqual;
}

/// Provides appropriate averaging for RGBA values.
class QRgbAttribute : public InlineAttribute<QRgb> {
    Q_OBJECT
    Q_PROPERTY(int defaultValue MEMBER _defaultValue)

public:
    
    Q_INVOKABLE QRgbAttribute(const QString& name = QString(), QRgb defaultValue = QRgb());
    
    virtual bool merge(void*& parent, void* children[]) const;
    
    virtual void* createFromScript(const QScriptValue& value, QScriptEngine* engine) const;
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual QWidget* createEditor(QWidget* parent = NULL) const;
};

/// Editor for RGBA values.
class QRgbEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color MEMBER _color WRITE setColor USER true)

public:
    
    QRgbEditor(QWidget* parent);

public slots:

    void setColor(const QColor& color);
        
private slots:

    void selectColor();    
    
private:
    
    QPushButton* _button;
    QColor _color;
};

/// An attribute class that stores pointers to its values.
template<class T> class PointerAttribute : public Attribute {
public:
    
    PointerAttribute(const QString& name, T defaultValue = T()) : Attribute(name), _defaultValue(defaultValue) { }
    
    virtual void* create(void* copy) const { new T(*static_cast<T*>(copy)); }
    virtual void destroy(void* value) const { delete static_cast<T*>(value); }
    
    virtual void read(Bitstream& in, void*& value, bool isLeaf) const;
    virtual void write(Bitstream& out, void* value, bool isLeaf) const;

    virtual bool equal(void* first, void* second) const { return *static_cast<T*>(first) == *static_cast<T*>(second); }

    virtual void* getDefaultValue() const { return const_cast<void*>((void*)&_defaultValue); }

private:
    
    T _defaultValue;
}; 

template<class T> inline void PointerAttribute<T>::read(Bitstream& in, void*& value, bool isLeaf) const {
    if (isLeaf) {
        in.read(value, sizeof(T) * 8);
    }
}

template<class T> inline void PointerAttribute<T>::write(Bitstream& out, void* value, bool isLeaf) const {
    if (isLeaf) {
        out.write(value, sizeof(T) * 8);
    }
}

/// Provides merging using the =, ==, += and /= operators.
template<class T> class SimplePointerAttribute : public PointerAttribute<T> {
public:
    
    SimplePointerAttribute(const QString& name, T defaultValue = T()) : PointerAttribute<T>(name, defaultValue) { }
    
    virtual bool merge(void*& parent, void* children[]) const;
};

template<class T> inline bool SimplePointerAttribute<T>::merge(void*& parent, void* children[]) const {
    T& merged = *static_cast<T*>(parent);
    merged = *static_cast<T*>(children[0]);
    bool allChildrenEqual = true;
    for (int i = 1; i < Attribute::MERGE_COUNT; i++) {
        merged += *static_cast<T*>(children[i]);
        allChildrenEqual &= (*static_cast<T*>(children[0]) == *static_cast<T*>(children[i]));
    }
    merged /= Attribute::MERGE_COUNT;
    return allChildrenEqual;
}

/// Base class for polymorphic attribute data.  
class PolymorphicData : public QSharedData {
public:
    
    virtual ~PolymorphicData();
    
    /// Creates a new clone of this object.
    virtual PolymorphicData* clone() const = 0;
    
    /// Tests this object for equality with another.
    virtual bool equals(const PolymorphicData* other) const = 0;
};

template<> PolymorphicData* QExplicitlySharedDataPointer<PolymorphicData>::clone();

typedef QExplicitlySharedDataPointer<PolymorphicData> PolymorphicDataPointer;

Q_DECLARE_METATYPE(PolymorphicDataPointer)

/// Provides polymorphic streaming and averaging.
class PolymorphicAttribute : public InlineAttribute<PolymorphicDataPointer> {
public:

    PolymorphicAttribute(const QString& name, const PolymorphicDataPointer& defaultValue = PolymorphicDataPointer());
    
    virtual void* createFromVariant(const QVariant& value) const;
    
    virtual bool equal(void* first, void* second) const;
    
    virtual bool merge(void*& parent, void* children[]) const;
};

class SharedObject : public QObject, public PolymorphicData {
    Q_OBJECT
    
public:

    virtual PolymorphicData* clone() const;
    
    virtual bool equals(const PolymorphicData* other) const;
};

/// An attribute that takes the form of QObjects of a given meta-type (a subclass of SharedObject).
class SharedObjectAttribute : public PolymorphicAttribute {
    Q_OBJECT
    Q_PROPERTY(const QMetaObject* metaObject MEMBER _metaObject)
    
public:
    
    Q_INVOKABLE SharedObjectAttribute(const QString& name = QString(), const QMetaObject* metaObject = NULL,
        const PolymorphicDataPointer& defaultValue = PolymorphicDataPointer());

    virtual QWidget* createEditor(QWidget* parent = NULL) const;
    
private:
    
    const QMetaObject* _metaObject;
};

Q_DECLARE_METATYPE(const QMetaObject*)

/// Allows editing shared object instances.
class SharedObjectEditor : public QWidget {
    Q_OBJECT
    Q_PROPERTY(PolymorphicDataPointer object MEMBER _object WRITE setObject USER true)

public:
    
    SharedObjectEditor(const QMetaObject* metaObject, QWidget* parent);

public slots:

    void setObject(const PolymorphicDataPointer& object);

private slots:

    void updateType();
    void propertyChanged();

private:
    
    QComboBox* _type;
    PolymorphicDataPointer _object;
};

#endif /* defined(__interface__AttributeRegistry__) */
