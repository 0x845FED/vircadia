//
//  floatingUI.js
//  examples/example/ui
//
//  Created by Alexander Otavka
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include([
    "../../libraries/globals.js",
    "../../libraries/overlayManager.js",
]);

var BG_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/card-bg.svg";
var RED_DOT_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/red-dot.svg";
var BLUE_SQUARE_IMAGE_URL = HIFI_PUBLIC_BUCKET + "images/blue-square.svg";

var mainPanel = new FloatingUIPanel({
    offsetRotation: {
        bind: "quat",
        value: { w: 1, x: 0, y: 0, z: 0 }
    },
    offsetPosition: { x: 0, y: 0.4, z: 1 }
});

var bluePanel = mainPanel.addChild(new FloatingUIPanel ({
    offsetPosition: { x: 0.1, y: 0.1, z: -0.2 }
}));

var mainPanelBackground = new BillboardOverlay({
    url: BG_IMAGE_URL,
    dimensions: {
        x: 0.5,
        y: 0.5,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0,
        y: 0,
        z: 0.001
    }
});

var bluePanelBackground = mainPanelBackground.clone();
bluePanelBackground.dimensions = {
    x: 0.3,
    y: 0.3
};

mainPanel.addChild(mainPanelBackground);
bluePanel.addChild(bluePanelBackground);

var redDot = mainPanel.addChild(new BillboardOverlay({
    url: RED_DOT_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.15,
        y: -0.15,
        z: 0
    }
}));

var redDot2 = mainPanel.addChild(new BillboardOverlay({
    url: RED_DOT_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: -0.155,
        y: 0.005,
        z: 0
    }
}));

var blueSquare = bluePanel.addChild(new BillboardOverlay({
    url: BLUE_SQUARE_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.055,
        y: -0.055,
        z: 0
    }
}));

var blueSquare2 = bluePanel.addChild(new BillboardOverlay({
    url: BLUE_SQUARE_IMAGE_URL,
    dimensions: {
        x: 0.1,
        y: 0.1,
    },
    isFacingAvatar: false,
    alpha: 1.0,
    ignoreRayIntersection: false,
    offsetPosition: {
        x: 0.055,
        y: 0.055,
        z: 0
    }
}));

var blueSquare3 = blueSquare2.clone();
blueSquare3.offsetPosition = {
    x: -0.055,
    y: 0.055,
    z: 0
};


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
        print(overlay.attachedPanel);
        if (overlay.attachedPanel === bluePanel) {
            overlay.destroy();
        } else if (overlay) {
            var oldPos = overlay.offsetPosition;
            var newPos = {
                x: Number(oldPos.x),
                y: Number(oldPos.y),
                z: Number(oldPos.z) + 0.1
            };
            overlay.offsetPosition = newPos;
        }
    }
    if (isRightClick && event.isRightButton) {
        mainPanel.visible = !mainPanel.visible;
    }
    isLeftClick = isRightClick = false;
}

function onScriptEnd() {
    mainPanel.destroy();
}

Controller.mousePressEvent.connect(onMouseDown);
Controller.mouseMoveEvent.connect(onMouseMove);
Controller.mouseReleaseEvent.connect(onMouseUp);
Script.scriptEnding.connect(onScriptEnd);