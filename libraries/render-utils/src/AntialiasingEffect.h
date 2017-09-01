//
//  AntialiasingEffect.h
//  libraries/render-utils/src/
//
//  Created by Raffi Bedikian on 8/30/15
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AntialiasingEffect_h
#define hifi_AntialiasingEffect_h

#include <DependencyManager.h>

#include "render/DrawTask.h"
#include "DeferredFrameTransform.h"
#include "VelocityBufferPass.h"


class JitterSampleConfig : public render::Job::Config {
    Q_OBJECT
        Q_PROPERTY(float scale MEMBER scale NOTIFY dirty)
        Q_PROPERTY(bool freeze MEMBER freeze NOTIFY dirty)
        Q_PROPERTY(int index READ getIndex NOTIFY dirty)
public:
    JitterSampleConfig() : render::Job::Config(true) {}

    float scale{ 0.5f };
    bool freeze{ false };

    void setIndex(int current);

public slots:
    int pause();
    int prev();
    int next();
    int play();

    int getIndex() const { return _index; }

signals:
    void dirty();

private:
    int _index{ 0 };

};


class JitterSample {
public:

    struct SampleSequence {
        SampleSequence();
        static const int SEQUENCE_LENGTH{ 16 };
        glm::vec2 offsets[SEQUENCE_LENGTH + 1];
        int sequenceLength{ SEQUENCE_LENGTH };
        int currentIndex{ 0 };

    };

    using JitterBuffer = gpu::StructBuffer<SampleSequence>;

    using Config = JitterSampleConfig;
    using JobModel = render::Job::ModelO<JitterSample, JitterBuffer, Config>;

    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, JitterBuffer& jitterBuffer);


private:

    JitterBuffer _jitterBuffer;
    float _scale{ 1.0 };
    bool _freeze{ false };
};


class AntialiasingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(float blend MEMBER blend NOTIFY dirty)
    Q_PROPERTY(float velocityScale MEMBER velocityScale NOTIFY dirty)
 
    Q_PROPERTY(bool unjitter MEMBER unjitter NOTIFY dirty)
    Q_PROPERTY(bool constrainColor MEMBER constrainColor NOTIFY dirty)

    Q_PROPERTY(bool debug MEMBER debug NOTIFY dirty)
    Q_PROPERTY(float debugX MEMBER debugX NOTIFY dirty)
    Q_PROPERTY(float debugFXAAX MEMBER debugFXAAX NOTIFY dirty)
    Q_PROPERTY(float debugShowVelocityThreshold MEMBER debugShowVelocityThreshold NOTIFY dirty)
    Q_PROPERTY(bool showCursorPixel MEMBER showCursorPixel NOTIFY dirty)
    Q_PROPERTY(glm::vec2 debugCursorTexcoord MEMBER debugCursorTexcoord NOTIFY dirty)
    Q_PROPERTY(float debugOrbZoom MEMBER debugOrbZoom NOTIFY dirty)

    Q_PROPERTY(bool showJitterSequence MEMBER showJitterSequence NOTIFY dirty)
    Q_PROPERTY(bool showClosestFragment MEMBER showClosestFragment NOTIFY dirty)

public:
    AntialiasingConfig() : render::Job::Config(true) {}

    float blend{ 0.1f };
    float velocityScale{ 1.0f };

    float debugX{ 0.0f };
    float debugFXAAX{ 1.0f };
    float debugShowVelocityThreshold{ 1.0f };
    glm::vec2 debugCursorTexcoord{ 0.5f, 0.5f };
    float debugOrbZoom{ 2.0f };

    bool unjitter{ true };
    bool constrainColor{ true };

    bool debug { false };
    bool showCursorPixel { false };
    bool showJitterSequence{ false };
    bool showClosestFragment{ false };

signals:
    void dirty();
};

#define SET_BIT(bitfield, bitIndex, value) bitfield = ((bitfield) & ~(1 << (bitIndex))) | ((value) << (bitIndex))
#define GET_BIT(bitfield, bitIndex) ((bitfield) & (1 << (bitIndex)))

struct TAAParams {
    float nope{ 0.0f };
    float blend{ 0.1f };
    float velocityScale{ 1.0f };
    float debugShowVelocityThreshold{ 1.0f };

    glm::ivec4 flags{ 0 };
    glm::vec4 pixelInfo{ 0.5f, 0.5f, 2.0f, 0.0f };
    glm::vec4 regionInfo{ 0.0f, 0.0f, 1.0f, 0.0f };

    void setUnjitter(bool enabled) { SET_BIT(flags.y, 0, enabled); }
    bool isUnjitter() const { return (bool)GET_BIT(flags.y, 0); }

    void setConstrainColor(bool enabled) { SET_BIT(flags.y, 1, enabled); }
    bool isConstrainColor() const { return (bool)GET_BIT(flags.y, 1); }

    void setDebug(bool enabled) { SET_BIT(flags.x, 0, enabled); }
    bool isDebug() const { return (bool) GET_BIT(flags.x, 0); }

    void setShowDebugCursor(bool enabled) { SET_BIT(flags.x, 1, enabled); }
    bool showDebugCursor() const { return (bool)GET_BIT(flags.x, 1); }

    void setDebugCursor(glm::vec2 debugCursor) { pixelInfo.x = debugCursor.x; pixelInfo.y = debugCursor.y; }
    glm::vec2 getDebugCursor() const { return glm::vec2(pixelInfo.x, pixelInfo.y); }
    
    void setDebugOrbZoom(float orbZoom) { pixelInfo.z = orbZoom; }
    float getDebugOrbZoom() const { return pixelInfo.z; }

    void setShowJitterSequence(bool enabled) { SET_BIT(flags.x, 2, enabled); }
    void setShowClosestFragment(bool enabled) { SET_BIT(flags.x, 3, enabled); }


};
using TAAParamsBuffer = gpu::StructBuffer<TAAParams>;

class Antialiasing {
public:
    using Inputs = render::VaryingSet5 < DeferredFrameTransformPointer, JitterSample::JitterBuffer, gpu::FramebufferPointer, LinearDepthFramebufferPointer, VelocityFramebufferPointer > ;
    using Config = AntialiasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, Inputs, Config>;

    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config);
    void run(const render::RenderContextPointer& renderContext, const Inputs& inputs);

    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();
    const gpu::PipelinePointer& getDebugBlendPipeline();

private:

    // Uniforms for AA
    gpu::int32 _texcoordOffsetLoc;

    gpu::FramebufferPointer _antialiasingBuffer[2];
    gpu::TexturePointer _antialiasingTexture[2];

    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;
    gpu::PipelinePointer _debugBlendPipeline;

    TAAParamsBuffer _params;
    int _currentFrame{ 0 };
};


/*
class AntiAliasingConfig : public render::Job::Config {
    Q_OBJECT
    Q_PROPERTY(bool enabled MEMBER enabled)
public:
    AntiAliasingConfig() : render::Job::Config(true) {}
};

class Antialiasing {
public:
    using Config = AntiAliasingConfig;
    using JobModel = render::Job::ModelI<Antialiasing, gpu::FramebufferPointer, Config>;
    
    Antialiasing();
    ~Antialiasing();
    void configure(const Config& config) {}
    void run(const render::RenderContextPointer& renderContext, const gpu::FramebufferPointer& sourceBuffer);
    
    const gpu::PipelinePointer& getAntialiasingPipeline();
    const gpu::PipelinePointer& getBlendPipeline();
    
private:
    
    // Uniforms for AA
    gpu::int32 _texcoordOffsetLoc;
    
    gpu::FramebufferPointer _antialiasingBuffer;
    
    gpu::TexturePointer _antialiasingTexture;
    
    gpu::PipelinePointer _antialiasingPipeline;
    gpu::PipelinePointer _blendPipeline;
    int _geometryId { 0 };
};
*/

#endif // hifi_AntialiasingEffect_h
