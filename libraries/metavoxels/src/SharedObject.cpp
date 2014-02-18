//
//  SharedObject.cpp
//  metavoxels
//
//  Created by Andrzej Kapolka on 2/5/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#include <QComboBox>
#include <QFormLayout>
#include <QItemEditorFactory>
#include <QMetaProperty>
#include <QVBoxLayout>

#include "Bitstream.h"
#include "MetavoxelUtil.h"
#include "SharedObject.h"

REGISTER_META_OBJECT(SharedObject)

SharedObject::SharedObject() : _referenceCount(0) {
}

void SharedObject::incrementReferenceCount() {
    _referenceCount++;
}

void SharedObject::decrementReferenceCount() {
    if (--_referenceCount == 0) {
        delete this;
    
    } else if (_referenceCount == 1) {
        emit referenceCountDroppedToOne();
    }
}

SharedObject* SharedObject::clone() const {
    // default behavior is to make a copy using the no-arg constructor and copy the stored properties
    const QMetaObject* metaObject = this->metaObject();
    SharedObject* newObject = static_cast<SharedObject*>(metaObject->newInstance());
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored()) {
            property.write(newObject, property.read(this));
        }
    }
    foreach (const QByteArray& propertyName, dynamicPropertyNames()) {
        newObject->setProperty(propertyName, property(propertyName));
    }
    return newObject;
}

bool SharedObject::equals(const SharedObject* other) const {
    // default behavior is to compare the properties
    const QMetaObject* metaObject = this->metaObject();
    if (metaObject != other->metaObject()) {
        return false;
    }
    for (int i = 0; i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (property.isStored() && property.read(this) != property.read(other)) {
            return false;
        }
    }
    QList<QByteArray> dynamicPropertyNames = this->dynamicPropertyNames();
    if (dynamicPropertyNames.size() != other->dynamicPropertyNames().size()) {
        return false;
    }
    foreach (const QByteArray& propertyName, dynamicPropertyNames) {
        if (property(propertyName) != other->property(propertyName)) {
            return false;
        }
    }
    return true;
}

SharedObjectEditor::SharedObjectEditor(const QMetaObject* metaObject, bool nullable, QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout();
    layout->setAlignment(Qt::AlignTop);
    setLayout(layout);
    
    QFormLayout* form = new QFormLayout();    
    layout->addLayout(form);
    
    form->addRow("Type:", _type = new QComboBox());
    if (nullable) {
        _type->addItem("(none)");
    }
    foreach (const QMetaObject* metaObject, Bitstream::getMetaObjectSubClasses(metaObject)) {
        // add add constructable subclasses
        if (metaObject->constructorCount() > 0) {
            _type->addItem(metaObject->className(), QVariant::fromValue(metaObject));
        }
    }
    connect(_type, SIGNAL(currentIndexChanged(int)), SLOT(updateType()));
    updateType();
}

void SharedObjectEditor::setObject(const SharedObjectPointer& object) {
    _object = object;
    const QMetaObject* metaObject = object ? object->metaObject() : NULL;
    int index = _type->findData(QVariant::fromValue(metaObject));
    if (index != -1) {
        // ensure that we call updateType to obtain the values
        if (_type->currentIndex() == index) {
            updateType();
        } else {
            _type->setCurrentIndex(index);
        }
    }
}

const QMetaObject* getOwningAncestor(const QMetaObject* metaObject, int propertyIndex) {
    while (propertyIndex < metaObject->propertyOffset()) {
        metaObject = metaObject->superClass();
    }
    return metaObject;
}

void SharedObjectEditor::updateType() {
    // delete the existing rows
    if (layout()->count() > 1) {
        QFormLayout* form = static_cast<QFormLayout*>(layout()->takeAt(1));
        while (!form->isEmpty()) {
            QLayoutItem* item = form->takeAt(0);
            if (item->widget()) {
                delete item->widget();
            }
            delete item;
        }
        delete form;
    }
    const QMetaObject* metaObject = _type->itemData(_type->currentIndex()).value<const QMetaObject*>();
    if (metaObject == NULL) {
        _object.reset();
        return;
    }
    QObject* oldObject = static_cast<SharedObject*>(_object.data());
    const QMetaObject* oldMetaObject = oldObject ? oldObject->metaObject() : NULL;
    QObject* newObject = metaObject->newInstance();
    
    QFormLayout* form = new QFormLayout();
    static_cast<QVBoxLayout*>(layout())->addLayout(form);
    for (int i = QObject::staticMetaObject.propertyCount(); i < metaObject->propertyCount(); i++) {
        QMetaProperty property = metaObject->property(i);
        if (!property.isDesignable()) {
            continue;
        }
        if (oldMetaObject && i < oldMetaObject->propertyCount() &&
                getOwningAncestor(metaObject, i) == getOwningAncestor(oldMetaObject, i)) {
            // copy the state of the shared ancestry
            property.write(newObject, property.read(oldObject));
        }
        QWidget* widget = QItemEditorFactory::defaultFactory()->createEditor(property.userType(), NULL);
        if (widget) {
            widget->setProperty("propertyIndex", i);
            form->addRow(QByteArray(property.name()) + ':', widget);
            QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(property.userType());
            const QMetaObject* widgetMetaObject = widget->metaObject();
            QMetaProperty widgetProperty = widgetMetaObject->property(widgetMetaObject->indexOfProperty(valuePropertyName));
            widgetProperty.write(widget, property.read(newObject));
            if (widgetProperty.hasNotifySignal()) {
                connect(widget, signal(widgetProperty.notifySignal().methodSignature()), SLOT(propertyChanged()));
            }
        }
    }
    _object = static_cast<SharedObject*>(newObject);
}

void SharedObjectEditor::propertyChanged() {
    QFormLayout* form = static_cast<QFormLayout*>(layout()->itemAt(1));
    for (int i = 0; i < form->rowCount(); i++) {
        QWidget* widget = form->itemAt(i, QFormLayout::FieldRole)->widget();
        if (widget != sender()) {
            continue;
        }
        _object.detach();
        QObject* object = _object.data();
        QMetaProperty property = object->metaObject()->property(widget->property("propertyIndex").toInt());
        QByteArray valuePropertyName = QItemEditorFactory::defaultFactory()->valuePropertyName(property.userType());
        property.write(object, widget->property(valuePropertyName));
    }
}
