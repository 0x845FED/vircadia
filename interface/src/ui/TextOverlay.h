//
//  TextOverlay.h
//  interface
//
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__TextOverlay__
#define __interface__TextOverlay__

// include this before QGLWidget, which includes an earlier version of OpenGL
#include "InterfaceConfig.h"

#include <QGLWidget>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QRect>
#include <QScriptValue>
#include <QString>
#include <QUrl>

#include <SharedUtil.h>

#include "Overlay.h"

const int DEFAULT_MARGIN = 10;

class TextOverlay : public Overlay {
    Q_OBJECT
    
public:
    TextOverlay();
    ~TextOverlay();
    virtual void render();

    // getters
    const QString& getText() const { return _text; }
    int getLeftMargin() const { return _leftMargin; }
    int getTopMargin() const { return _topMargin; }

    // setters
    void setText(const QString& text) { _text = text; }
    void setLeftMargin(int margin) { _leftMargin = margin; }
    void setTopMargin(int margin) { _topMargin = margin; }

    virtual void setProperties(const QScriptValue& properties);

private:

    QString _text;
    int _leftMargin;
    int _topMargin;
    
};

 
#endif /* defined(__interface__TextOverlay__) */
