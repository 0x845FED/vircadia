//
//  controlPanel.js
//  examples
//
//  Created by Zander Otavka on 7/15/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Shows a few common controls in a FloatingUIPanel on right click.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    "libraries/globals.js",
    "libraries/overlayManager.js",
]);

var BG_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/card-bg.svg";
var CLOSE_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/tools/close.svg";
var MIC_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/tools/mic-toggle.svg";
var FACE_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/tools/face-toggle.svg";
var ADDRESS_BAR_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/tools/address-bar-toggle.svg";

var panel = new FloatingUIPanel({
    anchorPosition: {
        bind: "myAvatar"
    },
    offsetPosition: { x: 0, y: 0.4, z: 1 }
});

var background = new BillboardOverlay({
    url: BG_IMAGE_URL,
    dimensions: {
        x: 0.5,
        y: 0.5,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false
});
panel.addChild(background);

var closeButton = new BillboardOverlay({
    url: CLOSE_IMAGE_URL,
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.1,
        y: 0.1,
        z: -0.001
    }
});
closeButton.onClick = function(event) {
    panel.visible = false;
};
panel.addChild(closeButton);

var micMuteButton = new BillboardOverlay({
    url: MIC_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.1,
        y: 0.1,
        z: -0.001
    }
});
micMuteButton.onClick = function(event) {
    AudioDevice.toggleMute();
};
panel.addChild(micMuteButton);

var faceMuteButton = new BillboardOverlay({
    url: FACE_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.1,
        y: -0.1,
        z: -0.001
    }
});
faceMuteButton.onClick = function(event) {
    FaceTracker.toggleMute();
};
panel.addChild(faceMuteButton);

var addressBarButton = new BillboardOverlay({
    url: ADDRESS_BAR_IMAGE_URL,
    subImage: {
        x: 0,
        y: 0,
        width: 45,
        height: 45
    },
    dimensions: {
        x: 0.15,
        y: 0.15,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.1,
        y: -0.1,
        z: -0.001
    }
});
addressBarButton.onClick = function(event) {
    DialogsManager.toggleAddressBar();
};
panel.addChild(addressBarButton);


function onMicMuteToggled() {
    var offset;
    if (AudioDevice.getMuted()) {
        offset = 45;
    } else {
        offset = 0;
    }
    micMuteButton.subImage = {
        x: offset,
        y: 0,
        width: 45,
        height: 45
    };
}
onMicMuteToggled();

function onFaceMuteToggled() {
    var offset;
    if (FaceTracker.getMuted()) {
        offset = 45;
    } else {
        offset = 0;
    }
    faceMuteButton.subImage = {
        x: offset,
        y: 0,
        width: 45,
        height: 45
    };
}
onFaceMuteToggled();

var isLeftClick = false,
    isRightClick = false;

function onMouseDown(event) {
    isLeftClick = event.isLeftButton;
    isRightClick = event.isRightButton;
}

function onMouseMove(event) {
    isLeftClick = isRightClick = false;
}

function onMouseUp(event) {
    if (isLeftClick && event.isLeftButton) {
        var overlay = OverlayManager.findAtPoint({ x: event.x, y: event.y });
        if (overlay && overlay.onClick) {
            overlay.onClick(event);
        }
    }
    if (isRightClick && event.isRightButton) {
        panel.setProperties({
            visible: !panel.visible,
            offsetRotation: {
                bind: "quat",
                value: Quat.multiply(MyAvatar.orientation, { x: 0, y: 1, z: 0, w: 0 })
            }
        });
    }
    isLeftClick = isRightClick = false;
}

function onScriptEnd(event) {
    panel.destroy();
}

Controller.mousePressEvent.connect(onMouseDown);
Controller.mouseMoveEvent.connect(onMouseMove);
Controller.mouseReleaseEvent.connect(onMouseUp);
AudioDevice.muteToggled.connect(onMicMuteToggled);
FaceTracker.muteToggled.connect(onFaceMuteToggled);
Script.scriptEnding.connect(onScriptEnd);
