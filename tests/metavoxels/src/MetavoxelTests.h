//
//  MetavoxelTests.h
//  metavoxel-tests
//
//  Created by Andrzej Kapolka on 2/7/14.
//  Copyright (c) 2014 High Fidelity, Inc. All rights reserved.
//

#ifndef __interface__MetavoxelTests__
#define __interface__MetavoxelTests__

#include <QCoreApplication>
#include <QVariantList>

#include <DatagramSequencer.h>

class SequencedTestMessage;

/// Tests various aspects of the metavoxel library.
class MetavoxelTests : public QCoreApplication {
    Q_OBJECT
    
public:
    
    MetavoxelTests(int& argc, char** argv);
    
    /// Performs our various tests.
    /// \return true if any of the tests failed.
    bool run();
};

/// Represents a simulated endpoint.
class Endpoint : public QObject {
    Q_OBJECT

public:
    
    Endpoint(const QByteArray& datagramHeader);

    void setOther(Endpoint* other) { _other = other; }

    /// Perform a simulation step.
    /// \return true if failure was detected
    bool simulate(int iterationNumber);

private slots:

    void sendDatagram(const QByteArray& datagram);    
    void handleHighPriorityMessage(const QVariant& message);
    void readMessage(Bitstream& in);
    void readReliableChannel();
    void readLowPriorityReliableChannel();
    
private:
    
    DatagramSequencer* _sequencer;
    Endpoint* _other;
    QList<QPair<QByteArray, int> > _delayedDatagrams;
    float _highPriorityMessagesToSend;
    QVariantList _highPriorityMessagesSent;
    QList<SequencedTestMessage> _unreliableMessagesSent;
    CircularBuffer _dataStreamed;
    CircularBuffer _lowPriorityDataStreamed;
};

/// A simple test message.
class TestMessageA {
    STREAMABLE

public:
    
    STREAM bool foo;
    STREAM int bar;
    STREAM float baz;
};

DECLARE_STREAMABLE_METATYPE(TestMessageA)

// Another simple test message.
class TestMessageB {
    STREAMABLE

public:
    
    STREAM QByteArray foo;
};

DECLARE_STREAMABLE_METATYPE(TestMessageB)

// A test message that demonstrates inheritance and composition.
class TestMessageC : public TestMessageA {
    STREAMABLE

public:
    
    STREAM TestMessageB bong;
};

DECLARE_STREAMABLE_METATYPE(TestMessageC)

/// Combines a sequence number with a submessage; used for testing unreliable transport.
class SequencedTestMessage {
    STREAMABLE

public:
    
    STREAM int sequenceNumber;
    STREAM QVariant submessage;
};

DECLARE_STREAMABLE_METATYPE(SequencedTestMessage)

#endif /* defined(__interface__MetavoxelTests__) */
