//
//  AddressBarDialog.qml
//
//  Created by Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "controls"
import "controls-uit" as HifiControls
import "styles"
import "windows"
import "hifi"

Window {
    id: root
    HifiConstants { id: hifi }

    objectName: "AddressBarDialog"
    frame: HiddenFrame {}
    hideBackground: true

    shown: false
    destroyOnHidden: false
    resizable: false
    scale: 1.25  // Make this dialog a little larger than normal

    width: addressBarDialog.implicitWidth
    height: addressBarDialog.implicitHeight

    onShownChanged: addressBarDialog.observeShownChanged(shown);
    Component.onCompleted: {
        root.parentChanged.connect(center);
        center();
    }
    Component.onDestruction: {
        root.parentChanged.disconnect(center);
    }

    function center() {
        // Explicitly center in order to avoid warnings at shutdown
        anchors.centerIn = parent;
    }

    function goCard(card) {
        addressLine.text = card.userStory.name;
        toggleOrGo();
    }
    property var suggestionChoices: null;

    AddressBarDialog {
        id: addressBarDialog
        implicitWidth: backgroundImage.width
        implicitHeight: backgroundImage.height

        Column {
            width: backgroundImage.width;
            anchors {
                left: backgroundImage.left;
                bottom: backgroundImage.top;
                leftMargin: parent.height + 2 * hifi.layout.spacing;
                bottomMargin: 2 * hifi.layout.spacing;
            }
            Text {
                id: suggestionsLabel;
                text: "Suggestions"
            }
            Row {
                Card {
                    id: s0;
                    width: 200;
                    height: 200;
                    goFunction: goCard
                }
                HifiControls.HorizontalSpacer { }
                Card {
                    id: s1;
                    width: 200;
                    height: 200;
                    goFunction: goCard
                }
                HifiControls.HorizontalSpacer { }
                Card {
                    id: s2;
                    width: 200;
                    height: 200;
                    goFunction: goCard
                }
            }
        }

        Image {
            id: backgroundImage
            source: "../images/address-bar.svg"
            width: 576 * root.scale
            height: 80 * root.scale
            property int inputAreaHeight: 56.0 * root.scale  // Height of the background's input area
            property int inputAreaStep: (height - inputAreaHeight) / 2

            Image {
                id: homeButton
                source: "../images/home-button.svg"
                width: 29
                height: 26
                anchors {
                    left: parent.left
                    leftMargin: parent.height + 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadHome()
                    }
                }
            }

            Image {
                id: backArrow
                source: addressBarDialog.backEnabled ? "../images/left-arrow.svg" : "../images/left-arrow-disabled.svg"
                width: 22
                height: 26
                anchors {
                    left: homeButton.right
                    leftMargin: 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadBack()
                    }
                }
            }

            Image {
                id: forwardArrow
                source: addressBarDialog.forwardEnabled ? "../images/right-arrow.svg" : "../images/right-arrow-disabled.svg"
                width: 22
                height: 26
                anchors {
                    left: backArrow.right
                    leftMargin: 2 * hifi.layout.spacing
                    verticalCenter: parent.verticalCenter
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        addressBarDialog.loadForward()
                    }
                }
            }

            // FIXME replace with TextField
            TextInput {
                id: addressLine
                focus: true
                anchors {
                    fill: parent
                    leftMargin: parent.height + parent.height + hifi.layout.spacing * 7
                    rightMargin: hifi.layout.spacing * 2
                    topMargin: parent.inputAreaStep + hifi.layout.spacing
                    bottomMargin: parent.inputAreaStep + hifi.layout.spacing
                }
                font.pixelSize: hifi.fonts.pixelSize * root.scale * 0.75
                helperText: "Go to: place, @user, /path, network address"
                onTextChanged: filterChoicesByText()
            }

        }
    }

    function getRequest(url, cb) { // cb(error, responseOfCorrectContentType) of url. General for 'get' text/html/json, but without redirects.
        // TODO: make available to other .qml.
        var request = new XMLHttpRequest();
        // QT bug: apparently doesn't handle onload. Workaround using readyState.
        request.onreadystatechange = function () {
            var READY_STATE_DONE = 4;
            var HTTP_OK = 200;
            if (request.readyState >= READY_STATE_DONE) {
                var error = (request.status !== HTTP_OK) && request.status.toString() + ':' + request.statusText,
                    response = !error && request.responseText,
                    contentType = !error && request.getResponseHeader('content-type');
                if (!error && contentType.indexOf('application/json') === 0) {
                    try {
                        response = JSON.parse(response);
                    } catch (e) {
                        error = e;
                    }
                }
                cb(error, response);
            }
        };
        request.open("GET", url, true);
        request.send();
    }
    // call iterator(element, icb) once for each element of array, and then cb(error) when icb(error) has been called by each iterator.
    // short-circuits if error. Note that iterator MUST be an asynchronous function. (Use setTimeout if necessary.)
    function asyncEach(array, iterator, cb) {
        var count = array.length;
        function icb(error) {
            if (!--count || error) {
                count = -1; // don't cb multiple times (e.g., if error)
                cb(error);
            }
        }
        if (!count) {
            return cb();
        }
        array.forEach(function (element) {
            iterator(element, icb);
        });
    }

    function addPictureToDomain(domainInfo, cb) { // asynchronously add thumbnail and lobby to domainInfo, if available, and cb(error)
        asyncEach([domainInfo.name].concat(domainInfo.names || null).filter(function (x) { return x; }), function (name, icb) {
            var url = "https://metaverse.highfidelity.com/api/v1/places/" + name;
            getRequest(url, function (error, json) {
                var previews = !error && json.data.place.previews;
                if (previews) {
                    if (!domainInfo.thumbnail) { // just grab tghe first one
                        domainInfo.thumbnail = previews.thumbnail;
                    }
                    if (!domainInfo.lobby) {
                        domainInfo.lobby = previews.lobby;
                    }
                }
                icb(error);
            });
        }, cb);
    }

    function getDomains(options, cb) { // cb(error, arrayOfData)
        if (!options.page) {
            options.page = 1;
        }
        // FIXME: really want places I'm allowed in, not just open ones
        var url = "https://metaverse.highfidelity.com/api/v1/domains/all?open&page=" + options.page + "&users=" + options.minUsers + "-" + options.maxUsers;
        getRequest(url, function (error, json) {
            if (!error && (json.status !== 'success')) {
                error = new Error("Bad response: " + JSON.stringify(json));
            }
            if (error) {
                error.message += ' for ' + url;
                return cb(error);
            }
            var domains = json.data.domains;
            asyncEach(domains, addPictureToDomain, function (error) {
                if (json.current_page < json.total_pages) {
                    options.page++;
                    return getDomains(options, function (error, others) {
                        cb(error, domains.concat(others));
                    });
                }
                cb(null, domains);
            });
        });
    }

    function filterChoicesByText() {
        function fill1(target, data) {
            if (!data) {
                target.visible = false;
                return;
            }
            console.log('suggestion:', JSON.stringify(data));
            target.userStory = data;
            target.image.source = data.lobby || ''; // should fail to load and thus use default
            target.placeText = data.name;
            target.usersText = data.online_users + ((data.online_users === 1) ? ' user' : ' users');
            target.visible = true;
        }
        if (!suggestionChoices) {
            return;
        }
        var words = addressLine.text.toUpperCase().split(/\s+/);
        var filtered = !words.length ? suggestionChoices : suggestionChoices.filter(function (domain) {
            var text = domain.names.concat(domain.tags).join(' ');
            if (domain.description) {
                text += domain.description;
            }
            text = text.toUpperCase();
            return words.every(function (word) {
                return text.indexOf(word) >= 0;
            });
        });
        fill1(s0, filtered[0]);
        fill1(s1, filtered[1]);
        fill1(s2, filtered[2]);
        suggestionsLabel.visible = filtered.length;
    }

    function fillDestinations() {
        suggestionChoices = null;
        getDomains({minUsers: 0, maxUsers: 20}, function (error, domains) {
            var here = addressBarDialog.getHost(); // don't show where we are now.
            var withLobby = !error && domains.filter(function (domain) { return domain.lobby && (domain.name !== here); });
            withLobby.sort(function (a, b) { return b.online_users - a.online_users; });
            suggestionChoices = withLobby;
            filterChoicesByText();
        });
    }

    onVisibleChanged: {
        if (visible) {
            addressLine.forceActiveFocus()
            fillDestinations();
        } else {
            addressLine.text = ""
        }
    }

    function toggleOrGo() {
        if (addressLine.text !== "") {
            addressBarDialog.loadAddress(addressLine.text)
        }
        root.shown = false;
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                root.shown = false
                event.accepted = true
                break
            case Qt.Key_Enter:
            case Qt.Key_Return:
                toggleOrGo()
                event.accepted = true
                break
        }
    }
}
