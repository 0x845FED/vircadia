//
//  AudioInjector.h
//  libraries/audio/src
//
//  Created by Stephen Birarda on 1/2/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AudioInjector_h
#define hifi_AudioInjector_h

#include <atomic>

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
#include <QtCore/QThread>

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>

#include "AudioInjectorLocalBuffer.h"
#include "AudioInjectorOptions.h"
#include "Sound.h"

class AbstractAudioInterface;
class AudioInjectorManager;
class NLPacket;

// In order to make scripting cleaner for the AudioInjector, the script now holds on to the AudioInjector object
// until it dies. 

class AudioInjector : public QObject {
    Q_OBJECT
    
public:
    enum class State : uint8_t {
        NotFinished,
        NotFinishedWithPendingDelete,
        Finished
    };
    
    AudioInjector(QObject* parent);
    AudioInjector(Sound* sound, const AudioInjectorOptions& injectorOptions);
    AudioInjector(const QByteArray& audioData, const AudioInjectorOptions& injectorOptions);
    
    bool isFinished() const { return _state == State::Finished; }
    
    int getCurrentSendOffset() const { return _currentSendOffset; }
    void setCurrentSendOffset(int currentSendOffset) { _currentSendOffset = currentSendOffset; }
    
    AudioInjectorLocalBuffer* getLocalBuffer() const { return _localBuffer; }
    bool isLocalOnly() const { return _options.localOnly; }
    
    void setLocalAudioInterface(AbstractAudioInterface* localAudioInterface) { _localAudioInterface = localAudioInterface; }

    static AudioInjector* playSoundAndDelete(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface);
    static AudioInjector* playSound(const QByteArray& buffer, const AudioInjectorOptions options, AbstractAudioInterface* localInterface);
    static AudioInjector* playSound(const QString& soundUrl, const float volume, const float stretchFactor, const glm::vec3 position);

public slots:
    void restart();
    
    void stop();
    void triggerDeleteAfterFinish();
    void stopAndDeleteLater();
    
    const AudioInjectorOptions& getOptions() const { return _options; }
    void setOptions(const AudioInjectorOptions& options) { _options = options;  }
    
    float getLoudness() const { return _loudness; }
    bool isPlaying() const { return _state == State::NotFinished || _state == State::NotFinishedWithPendingDelete; }
    
signals:
    void finished();
    void restartedWhileFinished();

private:
    void setupInjection();
    uint64_t injectNextFrame();
    void injectLocally();
    
    void finish();
    
    QByteArray _audioData;
    AudioInjectorOptions _options;
    std::atomic<State> _state { State::NotFinished };
    bool _hasStarted = false;
    bool _shouldStop = false;
    float _loudness = 0.0f;
    int _currentSendOffset = 0;
    std::unique_ptr<NLPacket> _currentPacket { nullptr };
    AbstractAudioInterface* _localAudioInterface { nullptr };
    AudioInjectorLocalBuffer* _localBuffer { nullptr };
    
    friend class AudioInjectorManager;
};


#endif // hifi_AudioInjector_h
