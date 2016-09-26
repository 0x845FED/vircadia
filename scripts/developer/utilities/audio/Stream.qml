//
//  Stream.qml
//  scripts/developer/utilities/audio
//
//  Created by Zach Pomerantz on 9/22/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3

ColumnLayout {
    property var stream 

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Ring Buffer"
        font.italic: true
    }
    MovingValue {
        label: "Desired"
        source: stream.framesDesired
        unit: "frames"
    }
    MovingValue {
        label: "Unplayed"
        source: stream.unplayedMsMax
    }
    Value {
        label: "Available (avg)"
        source: stream.framesAvailable + " (" + stream.framesAvailableAvg + ") frames"
    }

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Jitter"
        font.italic: true
    }
    Jitter {
        max: stream.timegapMsMax
        avg: stream.timegapMsAvg
        maxWindow: stream.timegapMsMaxWindow
        avgWindow: stream.timegapMsAvgWindow
    }

    Label {
        Layout.alignment: Qt.AlignCenter
        text: "Packet Loss"
        font.italic: true
    }
    Value {
        label: "Overall"
        source: stream.lossRate + "% (" + stream.lossCount + " lost)"
    }
    Value {
        label: "Window"
        source: stream.lossRateWindow + "% (" + stream.lossCountWindow + " lost)"
    }
}

