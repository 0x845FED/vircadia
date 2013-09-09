//
//  AudioMixer.cpp
//  hifi
//
//  Created by Stephen Birarda on 8/22/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <limits>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "Syssocket.h"
#include "Systime.h"
#include <math.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <sys/socket.h>
#endif //_WIN32

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <Logging.h>
#include <NodeList.h>
#include <Node.h>
#include <NodeTypes.h>
#include <PacketHeaders.h>
#include <SharedUtil.h>
#include <StdDev.h>

#include "AudioRingBuffer.h"

#include "AvatarAudioRingBuffer.h"
#include "InjectedAudioRingBuffer.h"

#include "AudioMixer.h"

const unsigned short MIXER_LISTEN_PORT = 55443;

const short JITTER_BUFFER_MSECS = 12;
const short JITTER_BUFFER_SAMPLES = JITTER_BUFFER_MSECS * (SAMPLE_RATE / 1000.0);

const unsigned int BUFFER_SEND_INTERVAL_USECS = floorf((BUFFER_LENGTH_SAMPLES_PER_CHANNEL / SAMPLE_RATE) * 1000000);

const int MAX_SAMPLE_VALUE = std::numeric_limits<int16_t>::max();
const int MIN_SAMPLE_VALUE = std::numeric_limits<int16_t>::min();

const char AUDIO_MIXER_LOGGING_TARGET_NAME[] = "audio-mixer";

void attachNewBufferToNode(Node *newNode) {
    if (!newNode->getLinkedData()) {
        if (newNode->getType() == NODE_TYPE_AGENT) {
            newNode->setLinkedData(new AvatarAudioRingBuffer());
        } else {
            newNode->setLinkedData(new InjectedAudioRingBuffer());
        }
    }
}

void AudioMixer::run() {
    // change the logging target name while this is running
    Logging::setTargetName(AUDIO_MIXER_LOGGING_TARGET_NAME);
    
    NodeList *nodeList = NodeList::getInstance();
    nodeList->setOwnerType(NODE_TYPE_AUDIO_MIXER);
    
    ssize_t receivedBytes = 0;
    
    nodeList->linkedDataCreateCallback = attachNewBufferToNode;
    
    nodeList->startSilentNodeRemovalThread();
    
    unsigned char* packetData = new unsigned char[MAX_PACKET_SIZE];
    
    sockaddr* nodeAddress = new sockaddr;
    
    // make sure our node socket is non-blocking
    nodeList->getNodeSocket()->setBlocking(false);
    
    int nextFrame = 0;
    timeval startTime;
    
    int numBytesPacketHeader = numBytesForPacketHeader((unsigned char*) &PACKET_TYPE_MIXED_AUDIO);
    unsigned char clientPacket[BUFFER_LENGTH_BYTES_STEREO + numBytesPacketHeader];
    populateTypeAndVersion(clientPacket, PACKET_TYPE_MIXED_AUDIO);
    
    int16_t clientSamples[BUFFER_LENGTH_SAMPLES_PER_CHANNEL * 2] = {};
    
    gettimeofday(&startTime, NULL);
    
    timeval lastDomainServerCheckIn = {};
    
    timeval beginSendTime, endSendTime;
    float sumFrameTimePercentages = 0.0f;
    int numStatCollections = 0;
    
    stk::StkFrames stkFrameBuffer(BUFFER_LENGTH_SAMPLES_PER_CHANNEL, 1);
    
    // if we'll be sending stats, call the Logstash::socket() method to make it load the logstash IP outside the loop
    if (Logging::shouldSendStats()) {
        Logging::socket();
    }
    
    while (true) {
        if (NodeList::getInstance()->getNumNoReplyDomainCheckIns() == MAX_SILENT_DOMAIN_SERVER_CHECK_INS) {
            break;
        }
        
        if (Logging::shouldSendStats()) {
            gettimeofday(&beginSendTime, NULL);
        }
        
        // send a check in packet to the domain server if DOMAIN_SERVER_CHECK_IN_USECS has elapsed
        if (usecTimestampNow() - usecTimestamp(&lastDomainServerCheckIn) >= DOMAIN_SERVER_CHECK_IN_USECS) {
            gettimeofday(&lastDomainServerCheckIn, NULL);
            NodeList::getInstance()->sendDomainServerCheckIn();
            
            if (Logging::shouldSendStats() && numStatCollections > 0) {
                // if we should be sending stats to Logstash send the appropriate average now
                const char MIXER_LOGSTASH_METRIC_NAME[] = "audio-mixer-frame-time-usage";
                
                float averageFrameTimePercentage = sumFrameTimePercentages / numStatCollections;
                Logging::stashValue(STAT_TYPE_TIMER, MIXER_LOGSTASH_METRIC_NAME, averageFrameTimePercentage);
                
                sumFrameTimePercentages = 0.0f;
                numStatCollections = 0;
            }
        }
        
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            PositionalAudioRingBuffer* positionalRingBuffer = (PositionalAudioRingBuffer*) node->getLinkedData();
            if (positionalRingBuffer && positionalRingBuffer->shouldBeAddedToMix(JITTER_BUFFER_SAMPLES)) {
                // this is a ring buffer that is ready to go
                // set its flag so we know to push its buffer when all is said and done
                positionalRingBuffer->setWillBeAddedToMix(true);
            }
        }
        
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            
            const int PHASE_DELAY_AT_90 = 20;
            
            if (node->getType() == NODE_TYPE_AGENT) {
                AvatarAudioRingBuffer* nodeRingBuffer = (AvatarAudioRingBuffer*) node->getLinkedData();
                
                // zero out the client mix for this node
                memset(clientSamples, 0, sizeof(clientSamples));
                
                // loop through all other nodes that have sufficient audio to mix
                for (NodeList::iterator otherNode = nodeList->begin(); otherNode != nodeList->end(); otherNode++) {
                    if (((PositionalAudioRingBuffer*) otherNode->getLinkedData())->willBeAddedToMix()
                        && (otherNode != node || (otherNode == node && nodeRingBuffer->shouldLoopbackForNode()))) {
                        PositionalAudioRingBuffer* otherNodeBuffer = (PositionalAudioRingBuffer*) otherNode->getLinkedData();
                        // based on our listen mode we will do this mixing...
                        if (nodeRingBuffer->isListeningToNode(*otherNode)) {
                            float bearingRelativeAngleToSource = 0.0f;
                            float attenuationCoefficient = 1.0f;
                            int numSamplesDelay = 0;
                            float weakChannelAmplitudeRatio = 1.0f;
                            
                            stk::TwoPole* otherNodeTwoPole = NULL;
                            
                            // only do axis/distance attenuation when in normal mode
                            if (otherNode != node && nodeRingBuffer->getListeningMode() == AudioRingBuffer::NORMAL) {
                                
                                glm::vec3 listenerPosition = nodeRingBuffer->getPosition();
                                glm::vec3 relativePosition = otherNodeBuffer->getPosition() - nodeRingBuffer->getPosition();
                                glm::quat inverseOrientation = glm::inverse(nodeRingBuffer->getOrientation());
                                
                                float distanceSquareToSource = glm::dot(relativePosition, relativePosition);
                                float radius = 0.0f;
                                
                                if (otherNode->getType() == NODE_TYPE_AUDIO_INJECTOR) {
                                    InjectedAudioRingBuffer* injectedBuffer = (InjectedAudioRingBuffer*) otherNodeBuffer;
                                    radius = injectedBuffer->getRadius();
                                    attenuationCoefficient *= injectedBuffer->getAttenuationRatio();
                                }
                                
                                if (radius == 0 || (distanceSquareToSource > radius * radius)) {
                                    // this is either not a spherical source, or the listener is outside the sphere
                                    
                                    if (radius > 0) {
                                        // this is a spherical source - the distance used for the coefficient
                                        // needs to be the closest point on the boundary to the source
                                        
                                        // ovveride the distance to the node with the distance to the point on the
                                        // boundary of the sphere
                                        distanceSquareToSource -= (radius * radius);
                                        
                                    } else {
                                        // calculate the angle delivery for off-axis attenuation
                                        glm::vec3 rotatedListenerPosition = glm::inverse(otherNodeBuffer->getOrientation())
                                        * relativePosition;
                                        
                                        float angleOfDelivery = glm::angle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                                           glm::normalize(rotatedListenerPosition));
                                        
                                        const float MAX_OFF_AXIS_ATTENUATION = 0.2f;
                                        const float OFF_AXIS_ATTENUATION_FORMULA_STEP = (1 - MAX_OFF_AXIS_ATTENUATION) / 2.0f;
                                        
                                        float offAxisCoefficient = MAX_OFF_AXIS_ATTENUATION +
                                        (OFF_AXIS_ATTENUATION_FORMULA_STEP * (angleOfDelivery / 90.0f));
                                        
                                        // multiply the current attenuation coefficient by the calculated off axis coefficient
                                        attenuationCoefficient *= offAxisCoefficient;
                                    }
                                    
                                    glm::vec3 rotatedSourcePosition = inverseOrientation * relativePosition;
                                    
                                    const float DISTANCE_SCALE = 2.5f;
                                    const float GEOMETRIC_AMPLITUDE_SCALAR = 0.3f;
                                    const float DISTANCE_LOG_BASE = 2.5f;
                                    const float DISTANCE_SCALE_LOG = logf(DISTANCE_SCALE) / logf(DISTANCE_LOG_BASE);
                                    
                                    // calculate the distance coefficient using the distance to this node
                                    float distanceCoefficient = powf(GEOMETRIC_AMPLITUDE_SCALAR,
                                                                     DISTANCE_SCALE_LOG +
                                                                     (0.5f * logf(distanceSquareToSource) / logf(DISTANCE_LOG_BASE)) - 1);
                                    distanceCoefficient = std::min(1.0f, distanceCoefficient);
                                    
                                    // multiply the current attenuation coefficient by the distance coefficient
                                    attenuationCoefficient *= distanceCoefficient;
                                    
                                    // project the rotated source position vector onto the XZ plane
                                    rotatedSourcePosition.y = 0.0f;
                                    
                                    // produce an oriented angle about the y-axis
                                    bearingRelativeAngleToSource = glm::orientedAngle(glm::vec3(0.0f, 0.0f, -1.0f),
                                                                                      glm::normalize(rotatedSourcePosition),
                                                                                      glm::vec3(0.0f, 1.0f, 0.0f));
                                    
                                    const float PHASE_AMPLITUDE_RATIO_AT_90 = 0.5;
                                    
                                    // figure out the number of samples of delay and the ratio of the amplitude
                                    // in the weak channel for audio spatialization
                                    float sinRatio = fabsf(sinf(glm::radians(bearingRelativeAngleToSource)));
                                    numSamplesDelay = PHASE_DELAY_AT_90 * sinRatio;
                                    weakChannelAmplitudeRatio = 1 - (PHASE_AMPLITUDE_RATIO_AT_90 * sinRatio);
                                    
                                    // grab the TwoPole object for this source, add it if it doesn't exist
                                    TwoPoleNodeMap& nodeTwoPoles = nodeRingBuffer->getTwoPoles();
                                    TwoPoleNodeMap::iterator twoPoleIterator = nodeTwoPoles.find(otherNode->getNodeID());
                                    
                                    if (twoPoleIterator == nodeTwoPoles.end()) {
                                        // setup the freeVerb effect for this source for this client
                                        otherNodeTwoPole = nodeTwoPoles[otherNode->getNodeID()] = new stk::TwoPole;
                                    } else {
                                        otherNodeTwoPole = twoPoleIterator->second;
                                    }
                                    
                                    // calculate the reasonance for this TwoPole based on angle to source
                                    float TWO_POLE_CUT_OFF_FREQUENCY = 800.0f;
                                    float TWO_POLE_MAX_FILTER_STRENGTH = 0.4f;
                                    
                                    otherNodeTwoPole->setResonance(TWO_POLE_CUT_OFF_FREQUENCY,
                                                                   TWO_POLE_MAX_FILTER_STRENGTH
                                                                   * fabsf(bearingRelativeAngleToSource) / 180.0f,
                                                                   true);
                                }
                            }
                            
                            int16_t* sourceBuffer = otherNodeBuffer->getNextOutput();
                            
                            int16_t* goodChannel = (bearingRelativeAngleToSource > 0.0f)
                            ? clientSamples
                            : clientSamples + BUFFER_LENGTH_SAMPLES_PER_CHANNEL;
                            int16_t* delayedChannel = (bearingRelativeAngleToSource > 0.0f)
                            ? clientSamples + BUFFER_LENGTH_SAMPLES_PER_CHANNEL
                            : clientSamples;
                            
                            int16_t* delaySamplePointer = otherNodeBuffer->getNextOutput() == otherNodeBuffer->getBuffer()
                            ? otherNodeBuffer->getBuffer() + RING_BUFFER_LENGTH_SAMPLES - numSamplesDelay
                            : otherNodeBuffer->getNextOutput() - numSamplesDelay;
                            
                            for (int s = 0; s < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                                // load up the stkFrameBuffer with this source's samples
                                stkFrameBuffer[s] = (stk::StkFloat) sourceBuffer[s];
                            }
                            
                            // perform the TwoPole effect on the stkFrameBuffer
                            if (otherNodeTwoPole) {
                                otherNodeTwoPole->tick(stkFrameBuffer);
                            }
                            
                            for (int s = 0; s < BUFFER_LENGTH_SAMPLES_PER_CHANNEL; s++) {
                                if (s < numSamplesDelay) {
                                    // pull the earlier sample for the delayed channel
                                    int earlierSample = delaySamplePointer[s] * attenuationCoefficient * weakChannelAmplitudeRatio;
                                    
                                    delayedChannel[s] = glm::clamp(delayedChannel[s] + earlierSample,
                                                                   MIN_SAMPLE_VALUE,
                                                                   MAX_SAMPLE_VALUE);
                                }
                                
                                int16_t currentSample = stkFrameBuffer[s] * attenuationCoefficient;
                                
                                goodChannel[s] = glm::clamp(goodChannel[s] + currentSample,
                                                            MIN_SAMPLE_VALUE,
                                                            MAX_SAMPLE_VALUE);
                                
                                if (s + numSamplesDelay < BUFFER_LENGTH_SAMPLES_PER_CHANNEL) {
                                    int sumSample = delayedChannel[s + numSamplesDelay]
                                    + (currentSample * weakChannelAmplitudeRatio);
                                    delayedChannel[s + numSamplesDelay] = glm::clamp(sumSample,
                                                                                     MIN_SAMPLE_VALUE,
                                                                                     MAX_SAMPLE_VALUE);
                                }
                                
                                if (s >= BUFFER_LENGTH_SAMPLES_PER_CHANNEL - PHASE_DELAY_AT_90) {
                                    // this could be a delayed sample on the next pass
                                    // so store the affected back in the ARB
                                    otherNodeBuffer->getNextOutput()[s] = (int16_t) stkFrameBuffer[s];
                                }
                            }
                        }
                    }
                }
                
                memcpy(clientPacket + numBytesPacketHeader, clientSamples, sizeof(clientSamples));
                nodeList->getNodeSocket()->send(node->getPublicSocket(), clientPacket, sizeof(clientPacket));
            }
        }
        
        // push forward the next output pointers for any audio buffers we used
        for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
            PositionalAudioRingBuffer* nodeBuffer = (PositionalAudioRingBuffer*) node->getLinkedData();
            if (nodeBuffer && nodeBuffer->willBeAddedToMix()) {
                nodeBuffer->setNextOutput(nodeBuffer->getNextOutput() + BUFFER_LENGTH_SAMPLES_PER_CHANNEL);
                
                if (nodeBuffer->getNextOutput() >= nodeBuffer->getBuffer() + RING_BUFFER_LENGTH_SAMPLES) {
                    nodeBuffer->setNextOutput(nodeBuffer->getBuffer());
                }
                nodeBuffer->setWillBeAddedToMix(false);
            }
        }
        
        // pull any new audio data from nodes off of the network stack
        while (nodeList->getNodeSocket()->receive(nodeAddress, packetData, &receivedBytes) &&
               packetVersionMatch(packetData)) {
            if (packetData[0] == PACKET_TYPE_MICROPHONE_AUDIO_NO_ECHO ||
                packetData[0] == PACKET_TYPE_MICROPHONE_AUDIO_WITH_ECHO) {
                
                unsigned char* currentBuffer = packetData + numBytesForPacketHeader(packetData);
                uint16_t sourceID;
                memcpy(&sourceID, currentBuffer, sizeof(sourceID));
                
                Node* avatarNode = nodeList->addOrUpdateNode(nodeAddress,
                                                             nodeAddress,
                                                             NODE_TYPE_AGENT,
                                                             sourceID);
                
                nodeList->updateNodeWithData(nodeAddress, packetData, receivedBytes);
                
                if (std::isnan(((PositionalAudioRingBuffer *)avatarNode->getLinkedData())->getOrientation().x)) {
                    // kill off this node - temporary solution to mixer crash on mac sleep
                    avatarNode->setAlive(false);
                }
            } else if (packetData[0] == PACKET_TYPE_INJECT_AUDIO) {
                Node* matchingInjector = NULL;
                
                for (NodeList::iterator node = nodeList->begin(); node != nodeList->end(); node++) {
                    if (node->getLinkedData()) {
                        
                        InjectedAudioRingBuffer* ringBuffer = (InjectedAudioRingBuffer*) node->getLinkedData();
                        if (memcmp(ringBuffer->getStreamIdentifier(),
                                   packetData + numBytesForPacketHeader(packetData),
                                   STREAM_IDENTIFIER_NUM_BYTES) == 0) {
                            // this is the matching stream, assign to matchingInjector and stop looking
                            matchingInjector = &*node;
                            break;
                        }
                    }
                }
                
                if (!matchingInjector) {
                    matchingInjector = nodeList->addOrUpdateNode(NULL,
                                                                 NULL,
                                                                 NODE_TYPE_AUDIO_INJECTOR,
                                                                 nodeList->getLastNodeID());
                    nodeList->increaseNodeID();
                    
                }
                
                // give the new audio data to the matching injector node
                nodeList->updateNodeWithData(matchingInjector, packetData, receivedBytes);
            } else if (packetData[0] == PACKET_TYPE_PING || packetData[0] == PACKET_TYPE_DOMAIN) {
                
                // If the packet is a ping, let processNodeData handle it.
                nodeList->processNodeData(nodeAddress, packetData, receivedBytes);
            }
        }
        
        if (Logging::shouldSendStats()) {
            // send a packet to our logstash instance
            
            // calculate the percentage value for time elapsed for this send (of the max allowable time)
            gettimeofday(&endSendTime, NULL);
            
            float percentageOfMaxElapsed = ((float) (usecTimestamp(&endSendTime) - usecTimestamp(&beginSendTime))
                                            / BUFFER_SEND_INTERVAL_USECS) * 100.0f;
            
            sumFrameTimePercentages += percentageOfMaxElapsed;
            
            numStatCollections++;
        }
        
        int usecToSleep = usecTimestamp(&startTime) + (++nextFrame * BUFFER_SEND_INTERVAL_USECS) - usecTimestampNow();
        
        if (usecToSleep > 0) {
            usleep(usecToSleep);
        } else {
            std::cout << "Took too much time, not sleeping!\n";
        }
    }
}