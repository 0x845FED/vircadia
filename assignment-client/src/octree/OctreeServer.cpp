//
//  OctreeServer.cpp
//  hifi
//
//  Created by Brad Hefta-Gaub on 9/16/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//

#include <QtCore/QTimer>
#include <QtCore/QUuid>
#include <QtNetwork/QNetworkAccessManager>

#include <time.h>
#include <HTTPConnection.h>
#include <Logging.h>
#include <UUID.h>

#include "OctreeServer.h"
#include "OctreeServerConsts.h"

OctreeServer* OctreeServer::_instance = NULL;

void OctreeServer::attachQueryNodeToNode(Node* newNode) {
    if (newNode->getLinkedData() == NULL) {
        OctreeQueryNode* newQueryNodeData = _instance->createOctreeQueryNode();
        newQueryNodeData->resetOctreePacket(true); // don't bump sequence
        newNode->setLinkedData(newQueryNodeData);
    }
}

OctreeServer::OctreeServer(const QByteArray& packet) :
    ThreadedAssignment(packet),
    _argc(0),
    _argv(NULL),
    _parsedArgV(NULL),
    _httpManager(NULL),
    _packetsPerClientPerInterval(10),
    _tree(NULL),
    _wantPersist(true),
    _debugSending(false),
    _debugReceiving(false),
    _verboseDebug(false),
    _jurisdiction(NULL),
    _jurisdictionSender(NULL),
    _octreeInboundPacketProcessor(NULL),
    _persistThread(NULL),
    _started(time(0)),
    _startedUSecs(usecTimestampNow())
{
    _instance = this;
}

OctreeServer::~OctreeServer() {
    if (_parsedArgV) {
        for (int i = 0; i < _argc; i++) {
            delete[] _parsedArgV[i];
        }
        delete[] _parsedArgV;
    }

    if (_jurisdictionSender) {
        _jurisdictionSender->terminate();
        _jurisdictionSender->deleteLater();
    }

    if (_octreeInboundPacketProcessor) {
        _octreeInboundPacketProcessor->terminate();
        _octreeInboundPacketProcessor->deleteLater();
    }

    if (_persistThread) {
        _persistThread->terminate();
        _persistThread->deleteLater();
    }

    delete _jurisdiction;
    _jurisdiction = NULL;

    qDebug() << "OctreeServer::run()... DONE";
}

void OctreeServer::initHTTPManager(int port) {
    // setup the embedded web server

    QString documentRoot = QString("%1/resources/web").arg(QCoreApplication::applicationDirPath());

    // setup an httpManager with us as the request handler and the parent
    _httpManager = new HTTPManager(port, documentRoot, this, this);
}

bool OctreeServer::handleHTTPRequest(HTTPConnection* connection, const QString& path) {

#ifdef FORCE_CRASH
    if (connection->requestOperation() == QNetworkAccessManager::GetOperation
        && path == "/force_crash") {

        qDebug() << "About to force a crash!";

        int foo;
        int* forceCrash = &foo;

        QString responseString("forcing a crash...");
        connection->respond(HTTPConnection::StatusCode200, qPrintable(responseString));

        delete[] forceCrash;

        return true;
    }
#endif

    bool showStats = false;

    if (connection->requestOperation() == QNetworkAccessManager::GetOperation) {
        if (path == "/") {
            showStats = true;
        } else if (path == "/resetStats") {
            _octreeInboundPacketProcessor->resetStats();
            showStats = true;
        }
    }

    if (showStats) {
        quint64 checkSum;
        // return a 200
        QString statsString("<html><doc>\r\n<pre>\r\n");
        statsString += QString("<b>Your %1 Server is running... <a href='/'>[RELOAD]</a></b>\r\n").arg(getMyServerName());

        tm* localtm = localtime(&_started);
        const int MAX_TIME_LENGTH = 128;
        char buffer[MAX_TIME_LENGTH];
        strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
        statsString += QString("Running since: %1").arg(buffer);

        // Convert now to tm struct for UTC
        tm* gmtm = gmtime(&_started);
        if (gmtm) {
            strftime(buffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", gmtm);
            statsString += (QString(" [%1 UTM] ").arg(buffer));
        }

        statsString += "\r\n";

        quint64 now  = usecTimestampNow();
        const int USECS_PER_MSEC = 1000;
        quint64 msecsElapsed = (now - _startedUSecs) / USECS_PER_MSEC;
        const int MSECS_PER_SEC = 1000;
        const int SECS_PER_MIN = 60;
        const int MIN_PER_HOUR = 60;
        const int MSECS_PER_MIN = MSECS_PER_SEC * SECS_PER_MIN;

        float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
        int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
        int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

        statsString += "Uptime: ";

        if (hours > 0) {
            statsString += QString("%1 hour%2").arg(hours).arg((hours > 1) ? "s" : "");
        }
        if (minutes > 0) {
            statsString += QString("%1 minute%s").arg(minutes).arg((minutes > 1) ? "s" : "");
        }
        if (seconds > 0) {
            statsString += QString().sprintf("%.3f seconds", seconds);
        }
        statsString += "\r\n\r\n";

        // display voxel file load time
        if (isInitialLoadComplete()) {
            if (isPersistEnabled()) {
                statsString += QString("%1 File Persist Enabled...\r\n").arg(getMyServerName());
            } else {
                statsString += QString("%1 File Persist Disabled...\r\n").arg(getMyServerName());
            }

            statsString += "\r\n";

            quint64 msecsElapsed = getLoadElapsedTime() / USECS_PER_MSEC;;
            float seconds = (msecsElapsed % MSECS_PER_MIN)/(float)MSECS_PER_SEC;
            int minutes = (msecsElapsed/(MSECS_PER_MIN)) % MIN_PER_HOUR;
            int hours = (msecsElapsed/(MSECS_PER_MIN * MIN_PER_HOUR));

            statsString += QString("%1 File Load Took").arg(getMyServerName());
            if (hours > 0) {
                statsString += QString("%1 hour%2").arg(hours).arg((hours > 1) ? "s" : "");
            }
            if (minutes > 0) {
                statsString += QString("%1 minute%2").arg(minutes).arg((minutes > 1) ? "s" : "");
            }
            if (seconds >= 0) {
                statsString += QString().sprintf("%.3f seconds", seconds);
            }
            statsString += "\r\n";

        } else {
            statsString += "Voxels not yet loaded...\r\n";
        }

        statsString += "\r\n\r\n";
        statsString += "<b>Configuration:</b>\r\n";

        for (int i = 1; i < _argc; i++) {
            statsString += _argv[i];
        }
        statsString += "\r\n"; //one to end the config line
        statsString += "\r\n\r\n"; // two more for spacing

        // display scene stats
        unsigned long nodeCount = OctreeElement::getNodeCount();
        unsigned long internalNodeCount = OctreeElement::getInternalNodeCount();
        unsigned long leafNodeCount = OctreeElement::getLeafNodeCount();

        QLocale locale(QLocale::English);
        const float AS_PERCENT = 100.0;
        statsString += "<b>Current Nodes in scene:</b>\r\n";
        statsString += QString("       Total Nodes: %1 nodes\r\n").arg(locale.toString((uint)nodeCount).rightJustified(16, ' '));
        statsString += QString().sprintf("    Internal Nodes: %s nodes (%5.2f%%)\r\n",
                                         locale.toString((uint)internalNodeCount).rightJustified(16,
                                                                                                 ' ').toLocal8Bit().constData(),
                                         ((float)internalNodeCount / (float)nodeCount) * AS_PERCENT);
        statsString += QString().sprintf("        Leaf Nodes: %s nodes (%5.2f%%)\r\n",
                                         locale.toString((uint)leafNodeCount).rightJustified(16, ' ').toLocal8Bit().constData(),
                                         ((float)leafNodeCount / (float)nodeCount) * AS_PERCENT);
        statsString += "\r\n";
        statsString += "\r\n";

        // display outbound packet stats
        statsString += QString("<b>%1 Outbound Packet Statistics...</b>\r\n").arg(getMyServerName());
        quint64 totalOutboundPackets = OctreeSendThread::_totalPackets;
        quint64 totalOutboundBytes = OctreeSendThread::_totalBytes;
        quint64 totalWastedBytes = OctreeSendThread::_totalWastedBytes;
        quint64 totalBytesOfOctalCodes = OctreePacketData::getTotalBytesOfOctalCodes();
        quint64 totalBytesOfBitMasks = OctreePacketData::getTotalBytesOfBitMasks();
        quint64 totalBytesOfColor = OctreePacketData::getTotalBytesOfColor();

        const int COLUMN_WIDTH = 10;
        statsString += QString("           Total Outbound Packets: %1 packets\r\n")
            .arg(locale.toString((uint)totalOutboundPackets).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("             Total Outbound Bytes: %1 bytes\r\n")
            .arg(locale.toString((uint)totalOutboundBytes).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("               Total Wasted Bytes: %1 bytes\r\n")
            .arg(locale.toString((uint)totalWastedBytes).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString().sprintf("            Total OctalCode Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfOctalCodes).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfOctalCodes / (float)totalOutboundBytes) * AS_PERCENT);
        statsString += QString().sprintf("             Total BitMasks Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfBitMasks).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfBitMasks / (float)totalOutboundBytes) * AS_PERCENT);
        statsString += QString().sprintf("                Total Color Bytes: %s bytes (%5.2f%%)\r\n",
            locale.toString((uint)totalBytesOfColor).rightJustified(COLUMN_WIDTH, ' ').toLocal8Bit().constData(),
            ((float)totalBytesOfColor / (float)totalOutboundBytes) * AS_PERCENT);

        statsString += "\r\n";
        statsString += "\r\n";

        // display inbound packet stats
        statsString += QString().sprintf("<b>%s Edit Statistics... <a href='/resetStats'>[RESET]</a></b>\r\n",
                                         getMyServerName());
        quint64 averageTransitTimePerPacket = _octreeInboundPacketProcessor->getAverageTransitTimePerPacket();
        quint64 averageProcessTimePerPacket = _octreeInboundPacketProcessor->getAverageProcessTimePerPacket();
        quint64 averageLockWaitTimePerPacket = _octreeInboundPacketProcessor->getAverageLockWaitTimePerPacket();
        quint64 averageProcessTimePerElement = _octreeInboundPacketProcessor->getAverageProcessTimePerElement();
        quint64 averageLockWaitTimePerElement = _octreeInboundPacketProcessor->getAverageLockWaitTimePerElement();
        quint64 totalElementsProcessed = _octreeInboundPacketProcessor->getTotalElementsProcessed();
        quint64 totalPacketsProcessed = _octreeInboundPacketProcessor->getTotalPacketsProcessed();

        float averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

        statsString += QString("           Total Inbound Packets: %1 packets\r\n")
            .arg(locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("          Total Inbound Elements: %1 elements\r\n")
            .arg(locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString().sprintf(" Average Inbound Elements/Packet: %f elements/packet\r\n", averageElementsPerPacket);
        statsString += QString("     Average Transit Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("     Average Process Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("   Average Wait Lock Time/Packet: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("    Average Process Time/Element: %1 usecs\r\n")
            .arg(locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' '));
        statsString += QString("  Average Wait Lock Time/Element: %1 usecs\r\n")
            .arg(locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' '));


        int senderNumber = 0;
        NodeToSenderStatsMap& allSenderStats = _octreeInboundPacketProcessor->getSingleSenderStats();
        for (NodeToSenderStatsMapIterator i = allSenderStats.begin(); i != allSenderStats.end(); i++) {
            senderNumber++;
            QUuid senderID = i->first;
            SingleSenderStats& senderStats = i->second;

            statsString += QString("\r\n             Stats for sender %1 uuid: %2\r\n")
                .arg(senderNumber).arg(senderID.toString());

            averageTransitTimePerPacket = senderStats.getAverageTransitTimePerPacket();
            averageProcessTimePerPacket = senderStats.getAverageProcessTimePerPacket();
            averageLockWaitTimePerPacket = senderStats.getAverageLockWaitTimePerPacket();
            averageProcessTimePerElement = senderStats.getAverageProcessTimePerElement();
            averageLockWaitTimePerElement = senderStats.getAverageLockWaitTimePerElement();
            totalElementsProcessed = senderStats.getTotalElementsProcessed();
            totalPacketsProcessed = senderStats.getTotalPacketsProcessed();

            averageElementsPerPacket = totalPacketsProcessed == 0 ? 0 : totalElementsProcessed / totalPacketsProcessed;

            statsString += QString("               Total Inbound Packets: %1 packets\r\n")
                .arg(locale.toString((uint)totalPacketsProcessed).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("              Total Inbound Elements: %1 elements\r\n")
                .arg(locale.toString((uint)totalElementsProcessed).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString().sprintf("     Average Inbound Elements/Packet: %f elements/packet\r\n",
                                             averageElementsPerPacket);
            statsString += QString("         Average Transit Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageTransitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("        Average Process Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageProcessTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("       Average Wait Lock Time/Packet: %1 usecs\r\n")
                .arg(locale.toString((uint)averageLockWaitTimePerPacket).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("        Average Process Time/Element: %1 usecs\r\n")
                .arg(locale.toString((uint)averageProcessTimePerElement).rightJustified(COLUMN_WIDTH, ' '));
            statsString += QString("      Average Wait Lock Time/Element: %1 usecs\r\n")
                .arg(locale.toString((uint)averageLockWaitTimePerElement).rightJustified(COLUMN_WIDTH, ' '));

        }

        statsString += "\r\n\r\n";

        // display memory usage stats
        statsString += "<b>Current Memory Usage Statistics</b>\r\n";
        statsString += QString().sprintf("\r\nOctreeElement size... %ld bytes\r\n", sizeof(OctreeElement));
        statsString += "\r\n";

        const char* memoryScaleLabel;
        const float MEGABYTES = 1000000.f;
        const float GIGABYTES = 1000000000.f;
        float memoryScale;
        if (OctreeElement::getTotalMemoryUsage() / MEGABYTES < 1000.0f) {
            memoryScaleLabel = "MB";
            memoryScale = MEGABYTES;
        } else {
            memoryScaleLabel = "GB";
            memoryScale = GIGABYTES;
        }

        statsString += QString().sprintf("Element Node Memory Usage:       %8.2f %s\r\n",
                                         OctreeElement::getVoxelMemoryUsage() / memoryScale, memoryScaleLabel);
        statsString += QString().sprintf("Octcode Memory Usage:            %8.2f %s\r\n",
                                         OctreeElement::getOctcodeMemoryUsage() / memoryScale, memoryScaleLabel);
        statsString += QString().sprintf("External Children Memory Usage:  %8.2f %s\r\n",
                                         OctreeElement::getExternalChildrenMemoryUsage() / memoryScale, memoryScaleLabel);
        statsString += "                                 -----------\r\n";
        statsString += QString().sprintf("                         Total:  %8.2f %s\r\n",
                                         OctreeElement::getTotalMemoryUsage() / memoryScale, memoryScaleLabel);
        statsString += "\r\n";

        statsString += "OctreeElement Children Population Statistics...\r\n";
        checkSum = 0;
        for (int i=0; i <= NUMBER_OF_CHILDREN; i++) {
            checkSum += OctreeElement::getChildrenCount(i);
            statsString += QString().sprintf("    Nodes with %d children:      %s nodes (%5.2f%%)\r\n", i,
                      locale.toString((uint)OctreeElement::getChildrenCount(i)).rightJustified(16, ' ').toLocal8Bit().constData(),
                      ((float)OctreeElement::getChildrenCount(i) / (float)nodeCount) * AS_PERCENT);
        }
        statsString += "                                ----------------------\r\n";
        statsString += QString("                    Total:      %1 nodes\r\n")
            .arg(locale.toString((uint)checkSum).rightJustified(16, ' '));

#ifdef BLENDED_UNION_CHILDREN
        statsString += "\r\n";
        statsString += "OctreeElement Children Encoding Statistics...\r\n";

        statsString += QString().sprintf("    Single or No Children:      %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getSingleChildrenCount(),
                                         ((float)OctreeElement::getSingleChildrenCount() / (float)nodeCount) * AS_PERCENT));
        statsString += QString().sprintf("    Two Children as Offset:     %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getTwoChildrenOffsetCount(),
                                         ((float)OctreeElement::getTwoChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT));
        statsString += QString().sprintf("    Two Children as External:   %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getTwoChildrenExternalCount(),
                                         ((float)OctreeElement::getTwoChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        statsString += QString().sprintf("    Three Children as Offset:   %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getThreeChildrenOffsetCount(),
                                         ((float)OctreeElement::getThreeChildrenOffsetCount() / (float)nodeCount) * AS_PERCENT);
        statsString += QString().sprintf("    Three Children as External: %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getThreeChildrenExternalCount(),
                                         ((float)OctreeElement::getThreeChildrenExternalCount() / (float)nodeCount) * AS_PERCENT);
        statsString += QString().sprintf("    Children as External Array: %10.llu nodes (%5.2f%%)\r\n",
                                         OctreeElement::getExternalChildrenCount(),
                                         ((float)OctreeElement::getExternalChildrenCount() / (float)nodeCount) * AS_PERCENT);

        checkSum = OctreeElement::getSingleChildrenCount() +
        OctreeElement::getTwoChildrenOffsetCount() + OctreeElement::getTwoChildrenExternalCount() +
        OctreeElement::getThreeChildrenOffsetCount() + OctreeElement::getThreeChildrenExternalCount() +
        OctreeElement::getExternalChildrenCount();

        statsString += "                                ----------------\r\n";
        statsString += QString().sprintf("                         Total: %10.llu nodes\r\n", checkSum);
        statsString += QString().sprintf("                      Expected: %10.lu nodes\r\n", nodeCount);

        statsString += "\r\n";
        statsString += "In other news....\r\n";

        statsString += QString().sprintf("could store 4 children internally:     %10.llu nodes\r\n",
                                         OctreeElement::getCouldStoreFourChildrenInternally());
        statsString += QString().sprintf("could NOT store 4 children internally: %10.llu nodes\r\n",
                                         OctreeElement::getCouldNotStoreFourChildrenInternally());
#endif

        statsString += "\r\n\r\n";
        statsString += "</pre>\r\n";
        statsString += "</doc></html>";

        connection->respond(HTTPConnection::StatusCode200, qPrintable(statsString), "text/html");

        return true;
    } else {
        // have HTTPManager attempt to process this request from the document_root
        return false;
    }

}

void OctreeServer::setArguments(int argc, char** argv) {
    _argc = argc;
    _argv = const_cast<const char**>(argv);

    qDebug("OctreeServer::setArguments()");
    for (int i = 0; i < _argc; i++) {
        qDebug("_argv[%d]=%s", i, _argv[i]);
    }

}

void OctreeServer::parsePayload() {

    if (getPayload().size() > 0) {
        QString config(_payload);

        // Now, parse the config
        QStringList configList = config.split(" ");

        int argCount = configList.size() + 1;

        qDebug("OctreeServer::parsePayload()... argCount=%d",argCount);

        _parsedArgV = new char*[argCount];
        const char* dummy = "config-from-payload";
        _parsedArgV[0] = new char[strlen(dummy) + sizeof(char)];
        strcpy(_parsedArgV[0], dummy);

        for (int i = 1; i < argCount; i++) {
            QString configItem = configList.at(i-1);
            _parsedArgV[i] = new char[configItem.length() + sizeof(char)];
            strcpy(_parsedArgV[i], configItem.toLocal8Bit().constData());
            qDebug("OctreeServer::parsePayload()... _parsedArgV[%d]=%s", i, _parsedArgV[i]);
        }

        setArguments(argCount, _parsedArgV);
    }
}

void OctreeServer::processDatagram(const QByteArray& dataByteArray, const HifiSockAddr& senderSockAddr) {
    NodeList* nodeList = NodeList::getInstance();

    PacketType packetType = packetTypeForPacket(dataByteArray);
    
    SharedNodePointer matchingNode = nodeList->sendingNodeForPacket(dataByteArray);

    if (packetType == getMyQueryMessageType()) {
        bool debug = false;
        if (debug) {
            qDebug() << "Got PacketType_VOXEL_QUERY at" << usecTimestampNow();
        }
       
        // If we got a PacketType_VOXEL_QUERY, then we're talking to an NodeType_t_AVATAR, and we
        // need to make sure we have it in our nodeList.


        if (matchingNode) {
            nodeList->updateNodeWithData(matchingNode.data(), senderSockAddr, dataByteArray);
            if (!matchingNode->getActiveSocket()) {
                // we don't have an active socket for this node, but they're talking to us
                // this means they've heard from us and can reply, let's assume public is active
                matchingNode->activatePublicSocket();
            }
            OctreeQueryNode* nodeData = (OctreeQueryNode*) matchingNode->getLinkedData();
            if (nodeData && !nodeData->isOctreeSendThreadInitalized()) {
                nodeData->initializeOctreeSendThread(this, matchingNode->getUUID());
            }
        }
    } else if (packetType == PacketTypeJurisdictionRequest) {
        _jurisdictionSender->queueReceivedPacket(matchingNode, dataByteArray);
    } else if (_octreeInboundPacketProcessor && getOctree()->handlesEditPacketType(packetType)) {
       _octreeInboundPacketProcessor->queueReceivedPacket(matchingNode, dataByteArray);
   } else {
       // let processNodeData handle it.
       NodeList::getInstance()->processNodeData(senderSockAddr, dataByteArray);
    }
}

void OctreeServer::run() {
    // Before we do anything else, create our tree...
    _tree = createTree();

    // change the logging target name while this is running
    Logging::setTargetName(getMyLoggingServerTargetName());

    // Now would be a good time to parse our arguments, if we got them as assignment
    if (getPayload().size() > 0) {
        parsePayload();
    }

    beforeRun(); // after payload has been processed

    qInstallMessageHandler(Logging::verboseMessageHandler);

    const char* STATUS_PORT = "--statusPort";
    const char* statusPort = getCmdOption(_argc, _argv, STATUS_PORT);
    if (statusPort) {
        int statusPortNumber = atoi(statusPort);
        initHTTPManager(statusPortNumber);
    }


    const char* JURISDICTION_FILE = "--jurisdictionFile";
    const char* jurisdictionFile = getCmdOption(_argc, _argv, JURISDICTION_FILE);
    if (jurisdictionFile) {
        qDebug("jurisdictionFile=%s", jurisdictionFile);

        qDebug("about to readFromFile().... jurisdictionFile=%s", jurisdictionFile);
        _jurisdiction = new JurisdictionMap(jurisdictionFile);
        qDebug("after readFromFile().... jurisdictionFile=%s", jurisdictionFile);
    } else {
        const char* JURISDICTION_ROOT = "--jurisdictionRoot";
        const char* jurisdictionRoot = getCmdOption(_argc, _argv, JURISDICTION_ROOT);
        if (jurisdictionRoot) {
            qDebug("jurisdictionRoot=%s", jurisdictionRoot);
        }

        const char* JURISDICTION_ENDNODES = "--jurisdictionEndNodes";
        const char* jurisdictionEndNodes = getCmdOption(_argc, _argv, JURISDICTION_ENDNODES);
        if (jurisdictionEndNodes) {
            qDebug("jurisdictionEndNodes=%s", jurisdictionEndNodes);
        }

        if (jurisdictionRoot || jurisdictionEndNodes) {
            _jurisdiction = new JurisdictionMap(jurisdictionRoot, jurisdictionEndNodes);
        }
    }

    NodeList* nodeList = NodeList::getInstance();
    nodeList->setOwnerType(getMyNodeType());

    // we need to ask the DS about agents so we can ping/reply with them
    nodeList->addNodeTypeToInterestSet(NodeType::Agent);

    setvbuf(stdout, NULL, _IOLBF, 0);

    nodeList->linkedDataCreateCallback = &OctreeServer::attachQueryNodeToNode;

    srand((unsigned)time(0));

    const char* VERBOSE_DEBUG = "--verboseDebug";
    _verboseDebug =  cmdOptionExists(_argc, _argv, VERBOSE_DEBUG);
    qDebug("verboseDebug=%s", debug::valueOf(_verboseDebug));

    const char* DEBUG_SENDING = "--debugSending";
    _debugSending =  cmdOptionExists(_argc, _argv, DEBUG_SENDING);
    qDebug("debugSending=%s", debug::valueOf(_debugSending));

    const char* DEBUG_RECEIVING = "--debugReceiving";
    _debugReceiving =  cmdOptionExists(_argc, _argv, DEBUG_RECEIVING);
    qDebug("debugReceiving=%s", debug::valueOf(_debugReceiving));

    // By default we will persist, if you want to disable this, then pass in this parameter
    const char* NO_PERSIST = "--NoPersist";
    if (cmdOptionExists(_argc, _argv, NO_PERSIST)) {
        _wantPersist = false;
    }
    qDebug("wantPersist=%s", debug::valueOf(_wantPersist));

    // if we want Persistence, set up the local file and persist thread
    if (_wantPersist) {

        // Check to see if the user passed in a command line option for setting packet send rate
        const char* PERSIST_FILENAME = "--persistFilename";
        const char* persistFilenameParameter = getCmdOption(_argc, _argv, PERSIST_FILENAME);
        if (persistFilenameParameter) {
            strcpy(_persistFilename, persistFilenameParameter);
        } else {
            strcpy(_persistFilename, getMyDefaultPersistFilename());
        }

        qDebug("persistFilename=%s", _persistFilename);

        // now set up PersistThread
        _persistThread = new OctreePersistThread(_tree, _persistFilename);
        if (_persistThread) {
            _persistThread->initialize(true);
        }
    }

    // Debug option to demonstrate that the server's local time does not
    // need to be in sync with any other network node. This forces clock
    // skew for the individual server node
    const char* CLOCK_SKEW = "--clockSkew";
    const char* clockSkewOption = getCmdOption(_argc, _argv, CLOCK_SKEW);
    if (clockSkewOption) {
        int clockSkew = atoi(clockSkewOption);
        usecTimestampNowForceClockSkew(clockSkew);
        qDebug("clockSkewOption=%s clockSkew=%d", clockSkewOption, clockSkew);
    }

    // Check to see if the user passed in a command line option for setting packet send rate
    const char* PACKETS_PER_SECOND = "--packetsPerSecond";
    const char* packetsPerSecond = getCmdOption(_argc, _argv, PACKETS_PER_SECOND);
    if (packetsPerSecond) {
        _packetsPerClientPerInterval = atoi(packetsPerSecond) / INTERVALS_PER_SECOND;
        if (_packetsPerClientPerInterval < 1) {
            _packetsPerClientPerInterval = 1;
        }
        qDebug("packetsPerSecond=%s PACKETS_PER_CLIENT_PER_INTERVAL=%d", packetsPerSecond, _packetsPerClientPerInterval);
    }

    HifiSockAddr senderSockAddr;

    // set up our jurisdiction broadcaster...
    if (_jurisdiction) {
        _jurisdiction->setNodeType(getMyNodeType());
    }
    _jurisdictionSender = new JurisdictionSender(_jurisdiction, getMyNodeType());
    _jurisdictionSender->initialize(true);

    // set up our OctreeServerPacketProcessor
    _octreeInboundPacketProcessor = new OctreeInboundPacketProcessor(this);
    _octreeInboundPacketProcessor->initialize(true);

    // Convert now to tm struct for local timezone
    tm* localtm = localtime(&_started);
    const int MAX_TIME_LENGTH = 128;
    char localBuffer[MAX_TIME_LENGTH] = { 0 };
    char utcBuffer[MAX_TIME_LENGTH] = { 0 };
    strftime(localBuffer, MAX_TIME_LENGTH, "%m/%d/%Y %X", localtm);
    // Convert now to tm struct for UTC
    tm* gmtm = gmtime(&_started);
    if (gmtm != NULL) {
        strftime(utcBuffer, MAX_TIME_LENGTH, " [%m/%d/%Y %X UTC]", gmtm);
    }
    qDebug() << "Now running... started at: " << localBuffer << utcBuffer;

    QTimer* domainServerTimer = new QTimer(this);
    connect(domainServerTimer, SIGNAL(timeout()), this, SLOT(checkInWithDomainServerOrExit()));
    domainServerTimer->start(DOMAIN_SERVER_CHECK_IN_USECS / 1000);

    QTimer* silentNodeTimer = new QTimer(this);
    connect(silentNodeTimer, SIGNAL(timeout()), nodeList, SLOT(removeSilentNodes()));
    silentNodeTimer->start(NODE_SILENCE_THRESHOLD_USECS / 1000);

    QTimer* pingNodesTimer = new QTimer(this);
    connect(pingNodesTimer, SIGNAL(timeout()), nodeList, SLOT(pingInactiveNodes()));
    pingNodesTimer->start(PING_INACTIVE_NODE_INTERVAL_USECS / 1000);
}
