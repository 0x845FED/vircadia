//
//  JurisdictionSender.h
//  shared
//
//  Created by Brad Hefta-Gaub on 8/12/13.
//  Copyright (c) 2013 High Fidelity, Inc. All rights reserved.
//
//  Jurisdiction Sender
//

#ifndef __shared__JurisdictionSender__
#define __shared__JurisdictionSender__

#include <queue>
#include <QMutex>

#include <PacketSender.h>
#include <ReceivedPacketProcessor.h>
#include "JurisdictionMap.h"

/// Will process PacketType_JURISDICTION_REQUEST packets and send out PacketType_JURISDICTION packets
/// to requesting parties. As with other ReceivedPacketProcessor classes the user is responsible for reading inbound packets
/// and adding them to the processing queue by calling queueReceivedPacket()
class JurisdictionSender : public ReceivedPacketProcessor {
    Q_OBJECT
public:
    static const int DEFAULT_PACKETS_PER_SECOND = 1;

    JurisdictionSender(JurisdictionMap* map, NodeType_t type = NodeType::VoxelServer);
    ~JurisdictionSender();

    void setJurisdiction(JurisdictionMap* map) { _jurisdictionMap = map; }

    virtual bool process();

    NodeType_t getNodeType() const { return _nodeType; }
    void setNodeType(NodeType_t type) { _nodeType = type; }

protected:
    virtual void processPacket(const HifiSockAddr& senderAddress, const QByteArray& packet);

    /// Locks all the resources of the thread.
    void lockRequestingNodes() { _requestingNodeMutex.lock(); }

    /// Unlocks all the resources of the thread.
    void unlockRequestingNodes() { _requestingNodeMutex.unlock(); }


private:
    QMutex _requestingNodeMutex;
    JurisdictionMap* _jurisdictionMap;
    std::queue<QUuid> _nodesRequestingJurisdictions;
    NodeType_t _nodeType;
    
    PacketSender _packetSender;
};
#endif // __shared__JurisdictionSender__
