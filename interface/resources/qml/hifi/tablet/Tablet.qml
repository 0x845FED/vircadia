import QtQuick 2.0
import QtGraphicalEffects 1.0

FocusScope {
    id: tablet
    objectName: "tablet"
    focus: true
    enabled: true
    property double micLevel: 0.8
    property int index: 0
    property int count: flowMain.children.length
    width: parent.width
    height: parent.height

    //property alias currentItem: flowMain.currentItem

    // called by C++ code to keep audio bar updated
    function setMicLevel(newMicLevel) {
        tablet.micLevel = newMicLevel;
    }

    // used to look up a button by its uuid
    function findButtonIndex(uuid) {
        if (!uuid) {
            return -1;
        }

        for (var i in flowMain.children) {
            var child = flowMain.children[i];
            if (child.uuid === uuid) {
                return i;
            }
        }
        return -1;
    }

    // called by C++ code when a button should be added to the tablet
    function addButtonProxy(properties) {
        var component = Qt.createComponent("TabletButton.qml");
        var button = component.createObject(flowMain);

        // copy all properites to button
        var keys = Object.keys(properties).forEach(function (key) {
            button[key] = properties[key];
        });

        // pass a reference to the tabletRoot object to the button.
        button.tabletRoot = parent.parent;
        return button;
    }

    // called by C++ code when a button should be removed from the tablet
    function removeButtonProxy(properties) {
        var index = findButtonIndex(properties.uuid);
        if (index < 0) {
            console.log("Warning: Tablet.qml could not find button with uuid = " + properties.uuid);
        } else {
            flowMain.children[index].destroy();
        }
    }

    Rectangle {
        id: bgTopBar
        height: 90
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"
            }

            GradientStop {
                position: 1
                color: "#1e1e1e"
            }
        }
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.topMargin: 0
        anchors.top: parent.top

        Row {
            id: rowAudio1
            height: parent.height
            anchors.topMargin: 0
            anchors.top: parent.top
            anchors.leftMargin: 30
            anchors.left: parent.left
            anchors.rightMargin: 30
            anchors.right: parent.right
            spacing: 5

            Image {
                id: muteIcon
                width: 40
                height: 40
                source: "../../../icons/tablet-mute-icon.svg"
                anchors.verticalCenter: parent.verticalCenter
            }

            Item {
                id: item1
                width: 225
                height: 10
                anchors.verticalCenter: parent.verticalCenter
                Rectangle {
                    id: audioBarBase
                    color: "#333333"
                    radius: 5
                    anchors.fill: parent
                }
                Rectangle {
                    id: audioBarMask
                    width: parent.width * tablet.micLevel
                    color: "#333333"
                    radius: 5
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    anchors.top: parent.top
                    anchors.topMargin: 0
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                }
                LinearGradient {
                    anchors.fill: audioBarMask
                    source: audioBarMask
                    start: Qt.point(0, 0)
                    end: Qt.point(225, 0)
                    gradient: Gradient {
                        GradientStop {
                            position: 0
                            color: "#2c8e72"
                        }
                        GradientStop {
                            position: 0.9
                            color: "#1fc6a6"
                        }
                        GradientStop {
                            position: 0.91
                            color: "#ea4c5f"
                        }
                        GradientStop {
                            position: 1
                            color: "#ea4c5f"
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id: bgMain
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#2b2b2b"

            }

            GradientStop {
                position: 1
                color: "#0f212e"
            }
        }
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: bgTopBar.bottom
        anchors.topMargin: 0

        Flickable {
            id: flickable
            width: parent.width
            height: parent.height
            contentWidth: parent.width
            contentHeight: flowMain.childrenRect.height + flowMain.anchors.topMargin + flowMain.anchors.bottomMargin + flowMain.spacing
            clip: true
            Flow {
                id: flowMain
                focus: true
                spacing: 16
                anchors.right: parent.right
                anchors.rightMargin: 30
                anchors.left: parent.left
                anchors.leftMargin: 30
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 30
                anchors.top: parent.top
                anchors.topMargin: 30

                function setCurrentItemState(state) {
                    flowMain.children[index].state = state;
                }
                function nextItem() {
                    setCurrentItemState("base state");
                    index = (index + count + 1) % count;
                    setCurrentItemState("hover state");
                    console.log("next item at index: " + index);
                }
                
                function previousItem() {
                    setCurrentItemState("base state");
                    index = (index + count - 1 ) % count;
                    setCurrentItemState("hover state");
                    console.log("previous item at index: " + index);
                }
                
                function upItem() {
                    setCurrentItemState("base state");
                    index = (index + count - 3) % count;
                    setCurrentItemState("hover state");
                    console.log("up item at index: " + index);
                }
                
                function downItem() {
                    setCurrentItemState("base state");
                    index = (index + count + 3) % count;
                    setCurrentItemState("hover state");
                    console.log("down item at index :" + index);
                }

                function selectItem() {
                    flowMain.children[index].clicked();
                    if(tabletRoot) {
                        tabletRoot.playButtonClickSound();
                    }
                }

                Keys.onRightPressed: nextItem();
                Keys.onLeftPressed: previousItem();
                Keys.onDownPressed: downItem();
                Keys.onUpPressed: upItem();
                Keys.onReturnPressed: selectItem();
            }
        }
    }

    states: [
        State {
            name: "muted state"

            PropertyChanges {
                target: muteText
                text: "UNMUTE"
            }

            PropertyChanges {
                target: muteIcon
                source: "../../../icons/tablet-unmute-icon.svg"
            }

            PropertyChanges {
                target: tablet
                micLevel: 0
            }
        }
    ]
}

