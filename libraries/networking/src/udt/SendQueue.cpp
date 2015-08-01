//
//  SendQueue.cpp
//  libraries/networking/src/udt
//
//  Created by Clement on 7/21/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "SendQueue.h"

#include <algorithm>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>

#include <SharedUtil.h>

#include "ControlPacket.h"
#include "Packet.h"
#include "Socket.h"

using namespace udt;
using namespace std::chrono;

std::unique_ptr<SendQueue> SendQueue::create(Socket* socket, HifiSockAddr dest) {
    auto queue = std::unique_ptr<SendQueue>(new SendQueue(socket, dest));
    
    Q_ASSERT_X(socket, "SendQueue::create", "Must be called with a valid Socket*");
    
    // Setup queue private thread
    QThread* thread = new QThread();
    thread->setObjectName("Networking: SendQueue " + dest.objectName()); // Name thread for easier debug
    
    connect(thread, &QThread::started, queue.get(), &SendQueue::run);
    
    connect(queue.get(), &QObject::destroyed, thread, &QThread::quit); // Thread auto cleanup
    connect(thread, &QThread::finished, thread, &QThread::deleteLater); // Thread auto cleanup
    
    // Move queue to private thread and start it
    queue->moveToThread(thread);
    thread->start();
    
    return std::move(queue);
}
    
SendQueue::SendQueue(Socket* socket, HifiSockAddr dest) :
    _socket(socket),
    _destination(dest)
{

}

void SendQueue::queuePacket(std::unique_ptr<Packet> packet) {
    {
        QWriteLocker locker(&_packetsLock);
        _packets.push_back(std::move(packet));
    }
    if (!this->thread()->isRunning()) {
        this->thread()->start();
    }
}

void SendQueue::stop() {
    _isRunning = false;
}
    
void SendQueue::sendPacket(const Packet& packet) {
    _socket->writeDatagram(packet.getData(), packet.getDataSize(), _destination);
}
    
void SendQueue::ack(SequenceNumber ack) {
    if (_lastACKSequenceNumber == (uint32_t) ack) {
        return;
    }
    
    {   // remove any ACKed packets from the map of sent packets
        QWriteLocker locker(&_sentLock);
        for (auto seq = SequenceNumber { (uint32_t) _lastACKSequenceNumber }; seq <= ack; ++seq) {
            _sentPackets.erase(seq);
        }
    }
    
    {   // remove any sequence numbers equal to or lower than this ACK in the loss list
        QWriteLocker nakLocker(&_naksLock);
        
        if (_naks.getLength() > 0 && _naks.getFirstSequenceNumber() <= ack) {
            _naks.remove(_naks.getFirstSequenceNumber(), ack);
        }
    }
    
    _lastACKSequenceNumber = (uint32_t) ack;
}

void SendQueue::nak(SequenceNumber start, SequenceNumber end) {
    QWriteLocker locker(&_naksLock);
    _naks.insert(start, end);
}

void SendQueue::overrideNAKListFromPacket(ControlPacket& packet) {
    QWriteLocker locker(&_naksLock);
    _naks.clear();
    
    SequenceNumber first, second;
    while (packet.bytesLeftToRead() >= (qint64)(2 * sizeof(SequenceNumber))) {
        packet.readPrimitive(&first);
        packet.readPrimitive(&second);
        
        if (first == second) {
            _naks.append(first);
        } else {
            _naks.append(first, second);
        }
    }
}

SequenceNumber SendQueue::getNextSequenceNumber() {
    _atomicCurrentSequenceNumber = (SequenceNumber::Type)++_currentSequenceNumber;
    return _currentSequenceNumber;
}

void SendQueue::sendNewPacketAndAddToSentList(std::unique_ptr<Packet> newPacket, SequenceNumber sequenceNumber) {
    // write the sequence number and send the packet
    newPacket->writeSequenceNumber(sequenceNumber);
    sendPacket(*newPacket);
    
    // Save packet/payload size before we move it
    auto packetSize = newPacket->getDataSize();
    auto payloadSize = newPacket->getPayloadSize();
    
    {
        // Insert the packet we have just sent in the sent list
        QWriteLocker locker(&_sentLock);
        _sentPackets[newPacket->getSequenceNumber()].swap(newPacket);
        Q_ASSERT_X(!newPacket, "SendQueue::sendNewPacketAndAddToSentList()", "Overriden packet in sent list");
    }
    
    emit packetSent(packetSize, payloadSize);
}

void SendQueue::run() {
    _isRunning = true;
    
    while (_isRunning) {
        // Record timing
        _lastSendTimestamp = high_resolution_clock::now();
        
        bool resentPacket = false;
        
        while (!resentPacket) {
            // prioritize a loss retransmission
            _naksLock.lockForWrite();
            
            if (_naks.getLength() > 0) {
                
                // pull the sequence number we need to re-send
                SequenceNumber resendNumber = _naks.popFirstSequenceNumber();
                _naksLock.unlock();
                
                // pull the packet to re-send from the sent packets list
                _sentLock.lockForRead();
                
                // see if we can find the packet to re-send
                auto it = _sentPackets.find(resendNumber);
                
                if (it != _sentPackets.end()) {
                    // we found the packet - grab it
                    auto& resendPacket = *(it->second);
                    
                    // unlock the sent packets
                    _sentLock.unlock();
                    
                    // send it off
                    sendPacket(resendPacket);
                    emit packetRetransmitted();
                    
                    // mark that we did resend a packet
                    resentPacket = true;
                    
                    // break out of our while now that we have re-sent a packet
                    break;
                } else {
                    // we didn't find this packet in the sentPackets queue - assume this means it was ACKed
                    // we'll fire the loop again to see if there is another to re-send
                    
                    // unlock the sent packets
                    _sentLock.unlock();
                }
                
            } else {
                // unlock the loss list, it's empty
                _naksLock.unlock();
                
                // break from the while, we didn't resend a packet
                break;
            }
        }
        
        if (!resentPacket
            && seqlen(SequenceNumber { (uint32_t) _lastACKSequenceNumber }, _currentSequenceNumber) <= _flowWindowSize) {
            
            // we didn't re-send a packet, so time to send a new one
            _packetsLock.lockForWrite();
            
            if (_packets.size() > 0) {
                SequenceNumber nextNumber = getNextSequenceNumber();
                
                // grab the first packet we will send
                std::unique_ptr<Packet> firstPacket;
                firstPacket.swap(_packets.front());
                _packets.pop_front();
               
                std::unique_ptr<Packet> secondPacket;
                
                if (((uint32_t) nextNumber & 0xF) == 0) {
                    // the first packet is the first in a probe pair - every 16 (rightmost 16 bits = 0) packets
                    // pull off a second packet if we can before we unlock
                    if (_packets.size() > 0) {
                        secondPacket.swap(_packets.front());
                        _packets.pop_front();
                    }
                }
                
                // unlock the packets, we're done pulling
                _packetsLock.unlock();
                
                // definitely send the first packet
                sendNewPacketAndAddToSentList(move(firstPacket), nextNumber);
                
                // do we have a second in a pair to send as well?
                if (secondPacket) {
                    nextNumber = getNextSequenceNumber();
                    sendNewPacketAndAddToSentList(move(secondPacket), nextNumber);
                }
                
            } else {
                _packetsLock.unlock();
            }
        }
        
        // since we're a while loop, give the thread a chance to process events
        QCoreApplication::processEvents();
        
        // we just processed events so check now if we were just told to stop
        if (!_isRunning) {
            break;
        }
        
        // sleep as long as we need until next packet send, if we can
        auto now = high_resolution_clock::now();
        auto microsecondDuration = (_lastSendTimestamp + microseconds(_packetSendPeriod)) - now;
        
        if (microsecondDuration.count() > 0) {
            usleep(microsecondDuration.count());
        }
    }
}
