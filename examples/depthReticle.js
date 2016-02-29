//  depthReticle.js
//  examples
//
//  Created by Brad Hefta-Gaub on 2/23/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  When used in HMD, this script will make the reticle depth track to any clickable item in view.
//  This script also handles auto-hiding the reticle after inactivity, as well as having the reticle
//  seek the look at position upon waking up.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var APPARENT_2D_OVERLAY_DEPTH = 1.0;
var APPARENT_MAXIMUM_DEPTH = 100.0; // this is a depth at which things all seem sufficiently distant
var lastDepthCheckTime = Date.now();
var desiredDepth = APPARENT_2D_OVERLAY_DEPTH;
var TIME_BETWEEN_DEPTH_CHECKS = 100;
var MINIMUM_DEPTH_ADJUST = 0.01;
var NON_LINEAR_DIVISOR = 2;
var MINIMUM_SEEK_DISTANCE = 0.01;

var lastMouseMove = Date.now();
var HIDE_STATIC_MOUSE_AFTER = 3000; // 3 seconds
var shouldSeekToLookAt = false;

Controller.mouseMoveEvent.connect(function(mouseEvent) {
    lastMouseMove = Date.now();

    // if the reticle is hidden, show it...
    if (!Reticle.visible) {
        Reticle.visible = true;
        if (HMD.active) {
            shouldSeekToLookAt = true;
        }
    }
});

function seekToLookAt() {
    // if we're currently seeking the lookAt move the mouse toward the lookat
    if (shouldSeekToLookAt) {
        var lookAt2D = HMD.getHUDLookAtPosition2D();
        var currentReticlePosition = Reticle.position;
        var distanceBetweenX = lookAt2D.x - Reticle.position.x;
        var distanceBetweenY = lookAt2D.y - Reticle.position.y;
        var moveX = distanceBetweenX / NON_LINEAR_DIVISOR;
        var moveY = distanceBetweenY / NON_LINEAR_DIVISOR;
        var newPosition = { x: Reticle.position.x + moveX, y: Reticle.position.y + moveY };
        var closeEnoughX = false;
        var closeEnoughY = false;
        if (moveX < MINIMUM_SEEK_DISTANCE) {
            newPosition.x = lookAt2D.x;
            closeEnoughX = true;
        }
        if (moveY < MINIMUM_SEEK_DISTANCE) {
            newPosition.y = lookAt2D.y;
            closeEnoughY = true;
        }
        if (closeEnoughX && closeEnoughY) {
            shouldSeekToLookAt = false;
        }
        Reticle.position = newPosition;
    }
}

function autoHideReticle() {
    // if we haven't moved in a long period of time, hide the reticle
    if (Reticle.visible) {
        var now = Date.now();
        var timeSinceLastMouseMove = now - lastMouseMove;
        if (timeSinceLastMouseMove > HIDE_STATIC_MOUSE_AFTER) {
            Reticle.visible = false;
        }
    }
}

function checkReticleDepth() {
    var now = Date.now();
    var timeSinceLastDepthCheck = now - lastDepthCheckTime;
    if (timeSinceLastDepthCheck > TIME_BETWEEN_DEPTH_CHECKS) {
        var newDesiredDepth = desiredDepth;
        lastDepthCheckTime = now;
        var reticlePosition = Reticle.position;

        // first check the 2D Overlays
        if (Reticle.pointingAtSystemOverlay || Overlays.getOverlayAtPoint(reticlePosition)) {
            newDesiredDepth = APPARENT_2D_OVERLAY_DEPTH;
        } else {
            var pickRay = Camera.computePickRay(reticlePosition.x, reticlePosition.y);

            // Then check the 3D overlays
            var result = Overlays.findRayIntersection(pickRay);

            if (!result.intersects) {
                // finally check the entities
                result = Entities.findRayIntersection(pickRay, true);
            }

            // If either the overlays or entities intersect, then set the reticle depth to 
            // the distance of intersection
            if (result.intersects) {
                newDesiredDepth = result.distance;
            } else {
                // if nothing intersects... set the depth to some sufficiently large depth
                newDesiredDepth = APPARENT_MAXIMUM_DEPTH;
            }
        }

        // If the desired depth has changed, reset our fade start time
        if (desiredDepth != newDesiredDepth) {
            desiredDepth = newDesiredDepth;
        }
    }

}

function moveToDesiredDepth() {
    // move the reticle toward the desired depth
    if (desiredDepth != Reticle.depth) {

        // cut distance between desiredDepth and current depth in half until we're close enough
        var distanceToAdjustThisCycle = (desiredDepth - Reticle.depth) / NON_LINEAR_DIVISOR;
        if (Math.abs(distanceToAdjustThisCycle) < MINIMUM_DEPTH_ADJUST) {
            newDepth = desiredDepth;
        } else {
            newDepth = Reticle.depth + distanceToAdjustThisCycle;
        }

        Reticle.setDepth(newDepth);
    }
}

Script.update.connect(function(deltaTime) {
    autoHideReticle(); // auto hide reticle for desktop or HMD mode
    if (HMD.active) {
        seekToLookAt(); // handle moving the reticle toward the look at
        checkReticleDepth(); // make sure reticle is at correct depth
        moveToDesiredDepth(); // move the fade the reticle to the desired depth
    }
});
