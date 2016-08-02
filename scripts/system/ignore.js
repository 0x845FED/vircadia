//
//  ignore.js
//  scripts/system/
//
//  Created by Stephen Birarda on 07/11/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// grab the toolbar
var toolbar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

function buttonImageURL() {
    return Script.resolvePath("assets/images/tools/" + (Users.canKick ? 'kick.svg' : 'ignore.svg'));
}

// setup the ignore button and add it to the toolbar
var button = toolbar.addButton({
    objectName: 'ignore',
    imageURL: buttonImageURL(),
    visible: true,
    buttonState: 1,
    defaultState: 2,
    hoverState: 3,
    alpha: 0.9
});

// if this user's kick permissions change, change the state of the button in the HUD
Users.canKickChanged.connect(function(canKick){
    button.writeProperty('imageURL', buttonImageURL());
});

var isShowingOverlays = false;
var ignoreOverlays = {};

function removeOverlays() {
    // enumerate the overlays and remove them
    var ignoreOverlayKeys = Object.keys(ignoreOverlays);

    for (i = 0; i < ignoreOverlayKeys.length; ++i) {
        var avatarID = ignoreOverlayKeys[i];
        Overlays.deleteOverlay(ignoreOverlays[avatarID]);
    }

    ignoreOverlays = {};
}

// handle clicks on the toolbar button
function buttonClicked(){
    if (isShowingOverlays) {
        removeOverlays();
        isShowingOverlays = false;
    } else {
        isShowingOverlays = true;
    }

    button.writeProperty('buttonState', isShowingOverlays ? 0 : 1);
    button.writeProperty('defaultState', isShowingOverlays ? 0 : 1);
    button.writeProperty('hoverState', isShowingOverlays ? 2 : 3);
}

button.clicked.connect(buttonClicked);

function overlayURL() {
    return Script.resolvePath("assets/images/" + (Users.canKick ? "kick-target.png" : "ignore-target.png"));
}

function updateOverlays() {
    if (isShowingOverlays) {

        var identifiers = AvatarList.getAvatarIdentifiers();

        for (i = 0; i < identifiers.length; ++i) {
            var avatarID = identifiers[i];

            if (avatarID === null) {
                // this is our avatar, skip it
                continue;
            }

            // get the position for this avatar
            var avatar = AvatarList.getAvatar(avatarID);
            var avatarPosition = avatar && avatar.position;

            if (!avatarPosition) {
                // we don't have a valid position for this avatar, skip it
                continue;
            }

            // setup a position for the overlay that is just above this avatar's head
            var overlayPosition = avatar.getJointPosition("Head");
            overlayPosition.y += 0.45;

            if (avatarID in ignoreOverlays) {
                // keep the overlay above the current position of this avatar
                Overlays.editOverlay(ignoreOverlays[avatarID], {
                    position: overlayPosition,
                    url: overlayURL()
                });
            } else {
                // add the overlay above this avatar
                var newOverlay = Overlays.addOverlay("image3d", {
                    url: overlayURL(),
                    position: overlayPosition,
                    size: 1,
                    scale: 0.4,
                    color: { red: 255, green: 255, blue: 255},
                    alpha: 1,
                    solid: true,
                    isFacingAvatar: true,
                    drawInFront: true
                });

                // push this overlay to our array of overlays
                ignoreOverlays[avatarID] = newOverlay;
            }
        }
    }
}

Script.update.connect(updateOverlays);

AvatarList.avatarRemovedEvent.connect(function(avatarID){
    if (isShowingOverlays) {
        // we are currently showing overlays and an avatar just went away

        // first remove the rendered overlay
        Overlays.deleteOverlay(ignoreOverlays[avatarID]);

        // delete the saved ID of the overlay from our ignored overlays object
        delete ignoreOverlays[avatarID];
    }
});

function handleSelectedOverlay(clickedOverlay) {
    // see this is one of our ignore overlays

    var ignoreOverlayKeys = Object.keys(ignoreOverlays)
    for (i = 0; i < ignoreOverlayKeys.length; ++i) {
        var avatarID = ignoreOverlayKeys[i];
        var ignoreOverlay = ignoreOverlays[avatarID];

        if (clickedOverlay.overlayID == ignoreOverlay) {
            // matched to an overlay, ask for the matching avatar to be ignored
            if (Users.canKick) {
                Users.kick(avatarID);
            } else {
                Users.ignore(avatarID);
            }

            // cleanup of the overlay is handled by the connection to avatarRemovedEvent
        }
    }
}

Controller.mousePressEvent.connect(function(event){
    if (isShowingOverlays) {
        // handle click events so we can detect when our overlays are clicked

        if (!event.isLeftButton) {
            // if another mouse button than left is pressed ignore it
            return false;
        }

        // compute the pick ray from the event
        var pickRay = Camera.computePickRay(event.x, event.y);

        // grab the clicked overlay for the given pick ray
        var clickedOverlay = Overlays.findRayIntersection(pickRay);
        if (clickedOverlay.intersects) {
            handleSelectedOverlay(clickedOverlay);
        }
    }
});

// We get mouseMoveEvents from the handControllers, via handControllerPointer.
// But we dont' get mousePressEvents.
var triggerMapping = Controller.newMapping(Script.resolvePath('') + '-click');

function controllerComputePickRay(hand) {
    var controllerPose = Controller.getPoseValue(hand);
    if (controllerPose.valid && triggered) {
        var controllerPosition = Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, controllerPose.translation),
                                          MyAvatar.position);
        // This gets point direction right, but if you want general quaternion it would be more complicated:
        var controllerDirection = Quat.getUp(Quat.multiply(MyAvatar.orientation, controllerPose.rotation));
        return { origin: controllerPosition, direction: controllerDirection };
    }
}

function makeClickHandler(hand) {
    return function (value) {
        if (isShowingOverlays) {
            var pickRay = controllerComputePickRay(hand);
            if (pickRay) {
                var overlayIntersection = Overlays.findRayIntersection(pickRay);
                if (overlayIntersection.intersects) {
                    handleSelectedOverlay(overlayIntersection);
                }
            }
        }
    };
}
triggerMapping.from(Controller.Standard.RTClick).peek().to(makeClickHandler(Controller.Standard.RightHand));
triggerMapping.from(Controller.Standard.LTClick).peek().to(makeClickHandler(Controller.Standard.LeftHand));

triggerMapping.enable();

// cleanup the toolbar button and overlays when script is stopped
Script.scriptEnding.connect(function() {
    toolbar.removeButton('ignore');
    removeOverlays();
    triggerMapping.disable();
});
