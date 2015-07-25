//
//  Created by Bradley Austin Davis on 2014/04/13.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "Oculus_0_5_DisplayPlugin.h"

#if (OVR_MAJOR_VERSION == 5)

#include <memory>

#include <QMainWindow>
#include <QGLWidget>
#include <GLMHelpers.h>
#include <GlWindow.h>
#include <QEvent>
#include <QResizeEvent>
#include <QOpenGLContext>

#include <OVR_CAPI_GL.h>

#include <PerfStat.h>
#include <OglplusHelpers.h>

#include "plugins/PluginContainer.h"
#include "OculusHelpers.h"

using namespace oglplus;

const QString Oculus_0_5_DisplayPlugin::NAME("Oculus Rift (0.5)");

const QString & Oculus_0_5_DisplayPlugin::getName() const {
    return NAME;
}

bool Oculus_0_5_DisplayPlugin::isSupported() const {
    if (!ovr_Initialize(nullptr)) {
        return false;
    }
    bool result = false;
    if (ovrHmd_Detect() > 0) {
        result = true;
    }
    ovr_Shutdown();
    return result;
}

void Oculus_0_5_DisplayPlugin::activate(PluginContainer * container) {
    if (!OVR_SUCCESS(ovr_Initialize(nullptr))) {
        Q_ASSERT(false);
        qFatal("Failed to Initialize SDK");
    }
    _hmd = ovrHmd_Create(0);
    if (!_hmd) {
        qFatal("Failed to acquire HMD");
    }
    
    OculusBaseDisplayPlugin::activate(container);

    _window->makeCurrent();
    _hmdWindow = new GlWindow(QOpenGLContext::currentContext());

    QSurfaceFormat format;
    initSurfaceFormat(format);
    _hmdWindow->setFlags(Qt::FramelessWindowHint);
    _hmdWindow->setFormat(format);
    _hmdWindow->create();
    _hmdWindow->setGeometry(_hmd->WindowsPos.x, _hmd->WindowsPos.y, _hmd->Resolution.w, _hmd->Resolution.h);
    //_hmdWindow->setGeometry(0, -1080, _hmd->Resolution.w, _hmd->Resolution.h);
    _hmdWindow->show();
    _hmdWindow->installEventFilter(this);
    bool makeCurrentResult = _hmdWindow->makeCurrent();
    qDebug() << makeCurrentResult;
    ovrGLConfig config; memset(&config, 0, sizeof(ovrRenderAPIConfig));
    auto& header = config.Config.Header;
    header.API = ovrRenderAPI_OpenGL;
    header.BackBufferSize = _hmd->Resolution;
    header.Multisample = 1;
    int distortionCaps = 0
        | ovrDistortionCap_TimeWarp
        ;

    memset(_eyeTextures, 0, sizeof(ovrTexture) * 2);
    ovr_for_each_eye([&](ovrEyeType eye) {
        auto& header = _eyeTextures[eye].Header;
        header.API = ovrRenderAPI_OpenGL;
        header.TextureSize = { (int)_desiredFramebufferSize.x, (int)_desiredFramebufferSize.y };
        header.RenderViewport.Size = header.TextureSize;
        header.RenderViewport.Size.w /= 2;
        if (eye == ovrEye_Right) {
            header.RenderViewport.Pos.x = header.RenderViewport.Size.w;
        }
    });

    ovrEyeRenderDesc _eyeRenderDescs[ovrEye_Count];
    ovrBool result = ovrHmd_ConfigureRendering(_hmd, &config.Config, distortionCaps, _eyeFovs, _eyeRenderDescs);
    Q_ASSERT(result);
}

void Oculus_0_5_DisplayPlugin::deactivate(PluginContainer* container) {
    _hmdWindow->deleteLater();
    _hmdWindow = nullptr;

    OculusBaseDisplayPlugin::deactivate(container);
    
    ovrHmd_Destroy(_hmd);
    _hmd = nullptr;
    ovr_Shutdown();
}

void Oculus_0_5_DisplayPlugin::preRender() {
    OculusBaseDisplayPlugin::preRender();
    ovrHmd_BeginFrame(_hmd, _frameIndex);
}

void Oculus_0_5_DisplayPlugin::preDisplay() {
    _hmdWindow->makeCurrent();
}

void Oculus_0_5_DisplayPlugin::display(GLuint finalTexture, const glm::uvec2& sceneSize) {
    ++_frameIndex;
    ovr_for_each_eye([&](ovrEyeType eye) {
        reinterpret_cast<ovrGLTexture&>(_eyeTextures[eye]).OGL.TexId = finalTexture;
    });
    ovrHmd_EndFrame(_hmd, _eyePoses, _eyeTextures);
}

bool _hswDismissed{false};
// Pass input events on to the application
bool Oculus_0_5_DisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    
    switch (event->type()) {
        case QEvent::KeyPress:
            if (!_hswDismissed) {
                static ovrHSWDisplayState hswState;
                ovrHmd_GetHSWDisplayState(_hmd, &hswState);
                if (hswState.Displayed) {
                    ovrHmd_DismissHSWDisplay(_hmd);
                } else {
                    _hswDismissed = true;
                }
            }
    }
    return OculusBaseDisplayPlugin::eventFilter(receiver, event);
}

// FIXME mirroring tot he main window is diffucult on OSX because it requires that we
// trigger a swap, which causes the client to wait for the v-sync of the main screen running
// at 60 Hz.  This would introduce judder.  Perhaps we can push mirroring to a separate
// thread
void Oculus_0_5_DisplayPlugin::finishFrame() {
    _hmdWindow->doneCurrent();
};

#endif
