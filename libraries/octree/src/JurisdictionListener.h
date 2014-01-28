//
//  JurisdictionListener.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Voxel Packet Sender
//

#ifndef __shared__JurisdictionListener__
#define __shared__JurisdictionListener__

#include <NodeList.h>
#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>


#include "JurisdictionMap.h"

/// Sends out PacketType_JURISDICTION_REQUEST packets to all voxel servers and then listens for and processes
/// the PacketType_JURISDICTION packets it receives in order to maintain an accurate state of all jurisidictions
/// within the domain. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionListener : public ReceivedPacketProcessor {
    Q_OBJECT
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;
    static const int NO_SERVER_CHECK_RATE = 60; // if no servers yet detected, keep checking at 60fps

    JurisdictionListener(NODE_TYPE type = NODE_TYPE_VOXEL_SERVER);
    
    virtual bool process();

    NodeToJurisdictionMap* getJurisdictions() { return &_jurisdictions; };


    NODE_TYPE getNodeType() const { return _nodeType; }
    void setNodeType(NODE_TYPE type) { _nodeType = type; }

public slots:
    /// Called by NodeList to inform us that a node has been killed.
    void nodeKilled(SharedNodePointer node);
    
protected:
    /// Callback for processing of received packets. Will process any queued PacketType_JURISDICTION and update the
    /// jurisdiction map member variable
    /// \param sockaddr& senderAddress the address of the sender
    /// \param packetData pointer to received data
    /// \param ssize_t packetLength size of received data
    /// \thread "this" individual processing thread
    virtual void processPacket(const HifiSockAddr& senderAddress, const QByteArray& packet);

private:
    NodeToJurisdictionMap _jurisdictions;
    NODE_TYPE _nodeType;

    bool queueJurisdictionRequest();

    PacketSender _packetSender;
};
#endif // __shared__JurisdictionListener__
