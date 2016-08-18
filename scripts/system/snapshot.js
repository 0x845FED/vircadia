//
// snapshot.js
//
// Created by David Kelly on 1 August 2016
// Copyright 2016 High Fidelity, Inc
//
// Distributed under the Apache License, Version 2.0
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var SNAPSHOT_DELAY = 500; // 500ms
var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");
var resetOverlays, recticleVisible;
var button = toolBar.addButton({
    objectName: "snapshot",
    imageURL: Script.resolvePath("assets/images/tools/snap.svg"),
    visible: true,
    buttonState: 1,
    defaultState: 1,
    hoverState: 2,
    alpha: 0.9,
});

function shouldOpenFeedAfterShare() {
    var persisted = Settings.getValue('openFeedAfterShare', true); // might answer true, false, "true", or "false"
    return persisted && (persisted !== 'false');
}
function showFeedWindow() {
    DialogsManager.showFeed();
}

var outstanding;
function confirmShare(data) {
    var dialog = new OverlayWebWindow('Snapshot Review', Script.resolvePath("html/ShareSnapshot.html"), 800, 470);
    function onMessage(message) {
        // Receives message from the html dialog via the qwebchannel EventBridge. This is complicated by the following:
        // 1. Although we can send POJOs, we cannot receive a toplevel object. (Arrays of POJOs are fine, though.)
        // 2. Although we currently use a single image, we would like to take snapshot, a selfie, a 360 etc. all at the
        //    same time, show the user all of them, and have the user deselect any that they do not want to share.
        //    So we'll ultimately be receiving a set of objects, perhaps with different post processing for each.
        var isLoggedIn, needsLogin = false;
        switch (message) {
        case 'ready':
            dialog.emitScriptEvent(data); // Send it.
            outstanding = 0;
            break;
        case 'openSettings':
            Desktop.show("hifi/dialogs/GeneralPreferencesDialog.qml", "GeneralPreferencesDialog");
            break;
        case 'setOpenFeedFalse':
            Settings.setValue('openFeedAfterShare', false)
            break;
        case 'setOpenFeedTrue':
            Settings.setValue('openFeedAfterShare', true)
            break;
        default:
            dialog.webEventReceived.disconnect(onMessage); // I'm not certain that this is necessary. If it is, what do we do on normal close?
            dialog.close();
            dialog.deleteLater();
            isLoggedIn = Account.isLoggedIn();
            message.forEach(function (submessage) {
                if (submessage.share && !isLoggedIn) {
                    needsLogin = true;
                    submessage.share = false;
                }
                if (submessage.share) {
                    print('sharing', submessage.localPath);
                    outstanding++;
                    Window.shareSnapshot(submessage.localPath);
                } else {
                    print('not sharing', submessage.localPath);
                }
            });
            if (!outstanding && shouldOpenFeedAfterShare()) {
                showFeedWindow();
            }
            if (needsLogin) { // after the possible feed, so that the login is on top
                Account.checkAndSignalForAccessToken();
            }
        }
    }
    dialog.webEventReceived.connect(onMessage);
    dialog.raise();
}

function snapshotShared(success) {
    if (success) {
        // for now just print status
        print('snapshot uploaded and shared');
    } else {
        // for now just print an error.
        print('snapshot upload/share failed');
    }
    if ((--outstanding <= 0) && shouldOpenFeedAfterShare()) {
        showFeedWindow();
    }
}

function onClicked() {
    // update button states
    resetOverlays = Menu.isOptionChecked("Overlays");
    reticleVisible = Reticle.visible;
    Reticle.visible = false;
    Window.snapshotTaken.connect(resetButtons);
    
    button.writeProperty("buttonState", 0);
    button.writeProperty("defaultState", 0);
    button.writeProperty("hoverState", 2);
     
    // hide overlays if they are on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", false);
    }
    
    // hide hud
    toolBar.writeProperty("visible", false);

    // take snapshot (with no notification)
    Script.setTimeout(function () {
        Window.takeSnapshot(false);
    }, SNAPSHOT_DELAY);
}

function resetButtons(path, notify) {
    // show overlays if they were on
    if (resetOverlays) {
        Menu.setIsOptionChecked("Overlays", true); 
    }
    // show hud
    toolBar.writeProperty("visible", true);
    Reticle.visible = reticleVisible;
    
    // update button states
    button.writeProperty("buttonState", 1);
    button.writeProperty("defaultState", 1);
    button.writeProperty("hoverState", 3);
    Window.snapshotTaken.disconnect(resetButtons);

    // last element in data array tells dialog whether we can share or not
    confirmShare([ 
        { localPath: path },
        {
            canShare: Boolean(Window.location.placename),
            isLoggedIn: true, // Just have the dialog act as though we are. To be removed at both ends later.
            openFeedAfterShare: shouldOpenFeedAfterShare()
        }
    ]);
 }

button.clicked.connect(onClicked);
Window.snapshotShared.connect(snapshotShared);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("snapshot");
    button.clicked.disconnect(onClicked);
    Window.snapshotShared.disconnect(snapshotShared);
});
