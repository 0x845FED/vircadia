import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    height: 48

    property string text: "NO STEP TEXT"
    property int uploaderState;
    property var uploader;

    state: root.uploader === undefined ? "" :
                (root.uploader.state > uploaderState ? "success"
                : (root.uploader.error !== 0 ? "fail" : (root.uploader.state === uploaderState ? "running" : "")))

    states: [
        State {
            name: "running"
            PropertyChanges { target: stepText; color: "white" }
            PropertyChanges { target: runningImage; visible: true; playing: true }
        },
        State {
            name: "fail"
            PropertyChanges { target: stepText; color: "#EA4C5F" }
            PropertyChanges { target: failGlyph; visible: true }
        },
        State {
            name: "success"
            PropertyChanges { target: stepText; color: "white" }
            PropertyChanges { target: successGlyph; visible: true }
        }
    ]

    Item {
        id: statusItem

        width: 48
        height: parent.height

        LoadingCircle {
            id: runningImage

            visible: false

            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            width: 32
            height: 32
        }
        Image {
            id: successGlyph

            visible: false

            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            width: 30
            height: 30

            source: "../../../icons/checkmark-stroke.svg"
        }
        HiFiGlyphs {
            id: failGlyph

            visible: false

            width: implicitWidth
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter

            size: 48
            text: "+"
            color: "#EA4C5F"
        }
    }
    RalewayRegular {
        id: stepText

        anchors.left: statusItem.right
        anchors.verticalCenter: parent.verticalCenter

        text: root.text
        size: 28
        color: "#777777"
    }
}
