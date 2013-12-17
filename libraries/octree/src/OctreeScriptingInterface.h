//
//  OctreeScriptingInterface.h
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/6/13
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#ifndef __hifi__OctreeScriptingInterface__
#define __hifi__OctreeScriptingInterface__

#include <QtCore/QObject>

#include "JurisdictionListener.h"
#include "OctreeEditPacketSender.h"

/// handles scripting of Particle commands from JS passed to assigned clients
class OctreeScriptingInterface : public QObject {
    Q_OBJECT
public:
    OctreeScriptingInterface(OctreeEditPacketSender* packetSender = NULL, 
                JurisdictionListener* jurisdictionListener = NULL);

    ~OctreeScriptingInterface();

    OctreeEditPacketSender* getPacketSender() const { return _packetSender; }
    JurisdictionListener* getJurisdictionListener() const { return _jurisdictionListener; }
    void setPacketSender(OctreeEditPacketSender* packetSender);
    void setJurisdictionListener(JurisdictionListener* jurisdictionListener);
    void init();
    
    virtual NODE_TYPE getServerNodeType() const = 0;
    virtual OctreeEditPacketSender* createPacketSender() = 0;

public slots:
    /// Set the desired max packet size in bytes that should be created
    void setMaxPacketSize(int maxPacketSize) { return _packetSender->setMaxPacketSize(maxPacketSize); }

    /// returns the current desired max packet size in bytes that will be created
    int getMaxPacketSize() const { return _packetSender->getMaxPacketSize(); }

    /// set the max packets per second send rate
    void setPacketsPerSecond(int packetsPerSecond) { return _packetSender->setPacketsPerSecond(packetsPerSecond); }

    /// get the max packets per second send rate
    int getPacketsPerSecond() const  { return _packetSender->getPacketsPerSecond(); }

    /// does a particle server exist to send to
    bool serversExist() const { return _packetSender->serversExist(); }

    /// are there packets waiting in the send queue to be sent
    bool hasPacketsToSend() const { return _packetSender->hasPacketsToSend(); }

    /// how many packets are there in the send queue waiting to be sent
    int packetsToSendCount() const { return _packetSender->packetsToSendCount(); }

    /// returns the packets per second send rate of this object over its lifetime
    float getLifetimePPS() const { return _packetSender->getLifetimePPS(); }

    /// returns the bytes per second send rate of this object over its lifetime
    float getLifetimeBPS() const { return _packetSender->getLifetimeBPS(); }
    
    /// returns the packets per second queued rate of this object over its lifetime
    float getLifetimePPSQueued() const  { return _packetSender->getLifetimePPSQueued(); }

    /// returns the bytes per second queued rate of this object over its lifetime
    float getLifetimeBPSQueued() const { return _packetSender->getLifetimeBPSQueued(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    long long unsigned int getLifetimeInUsecs() const { return _packetSender->getLifetimeInUsecs(); }

    /// returns lifetime of this object from first packet sent to now in usecs
    float getLifetimeInSeconds() const { return _packetSender->getLifetimeInSeconds(); }

    /// returns the total packets sent by this object over its lifetime
    long long unsigned int getLifetimePacketsSent() const { return _packetSender->getLifetimePacketsSent(); }

    /// returns the total bytes sent by this object over its lifetime
    long long unsigned int getLifetimeBytesSent() const { return _packetSender->getLifetimeBytesSent(); }

    /// returns the total packets queued by this object over its lifetime
    long long unsigned int getLifetimePacketsQueued() const { return _packetSender->getLifetimePacketsQueued(); }

    /// returns the total bytes queued by this object over its lifetime
    long long unsigned int getLifetimeBytesQueued() const { return _packetSender->getLifetimeBytesQueued(); }

protected:
    /// attached OctreeEditPacketSender that handles queuing and sending of packets to VS
    OctreeEditPacketSender* _packetSender;
    JurisdictionListener* _jurisdictionListener;
    bool _managedPacketSender;
    bool _managedJurisdictionListener;
};

#endif /* defined(__hifi__OctreeScriptingInterface__) */
