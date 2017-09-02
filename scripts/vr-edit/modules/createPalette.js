//
//  createPalette.js
//
//  Created by David Rowe on 28 Jul 2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global App, CreatePalette */

CreatePalette = function (side, leftInputs, rightInputs, uiCommandCallback) {
    // Tool menu displayed on top of forearm.

    "use strict";

    var paletteOriginOverlay,
        paletteHeaderHeadingOverlay,
        paletteHeaderBarOverlay,
        paletteTitleOverlay,
        palettePanelOverlay,
        paletteItemOverlays = [],
        paletteItemPositions = [],
        paletteItemHoverOverlays = [],
        iconOverlays = [],
        staticOverlays = [],

        LEFT_HAND = 0,

        controlJointName,

        PALETTE_ORIGIN_POSITION = {
            x: 0,
            y: UIT.dimensions.handOffset,
            z: UIT.dimensions.canvasSeparation + UIT.dimensions.canvas.x / 2
        },
        PALETTE_ORIGIN_ROTATION = Quat.ZERO,
        paletteLateralOffset,

        PALETTE_ORIGIN_PROPERTIES = {
            dimensions: { x: 0.005, y: 0.005, z: 0.005 },
            localPosition: PALETTE_ORIGIN_POSITION,
            localRotation: PALETTE_ORIGIN_ROTATION,
            color: { red: 255, blue: 0, green: 0 },
            alpha: 1.0,
            parentID: Uuid.SELF,
            ignoreRayIntersection: true,
            visible: false
        },

        PALETTE_HEADER_HEADING_PROPERTIES = {
            dimensions: UIT.dimensions.headerHeading,
            localPosition: {
                x: 0,
                y: UIT.dimensions.canvas.y / 2 - UIT.dimensions.headerHeading.y / 2,
                z: UIT.dimensions.headerHeading.z / 2
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        PALETTE_HEADER_BAR_PROPERTIES = {
            dimensions: UIT.dimensions.headerBar,
            localPosition: {
                x: 0,
                y: UIT.dimensions.canvas.y / 2 - UIT.dimensions.headerHeading.y - UIT.dimensions.headerBar.y / 2,
                z: UIT.dimensions.headerBar.z / 2
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.blueHighlight,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        PALETTE_TITLE_PROPERTIES = {
            url: "../assets/create/create-heading.svg",
            scale: 0.0363,
            localPosition: {
                x: 0,
                y: 0,
                z: PALETTE_HEADER_HEADING_PROPERTIES.dimensions.z / 2 + UIT.dimensions.imageOverlayOffset
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.white,
            alpha: 1.0,
            emissive: true,
            ignoreRayIntersection: true,
            isFacingAvatar: false,
            visible: true
        },

        PALETTE_PANEL_PROPERTIES = {
            dimensions: UIT.dimensions.panel,
            localPosition: {
                x: 0,
                y: UIT.dimensions.panel.y / 2 - UIT.dimensions.canvas.y / 2,
                z: UIT.dimensions.panel.z / 2
            },
            localRotation: Quat.ZERO,
            color: UIT.colors.baseGray,
            alpha: 1.0,
            solid: true,
            ignoreRayIntersection: false,
            visible: true
        },

        ENTITY_CREATION_DIMENSIONS = { x: 0.2, y: 0.2, z: 0.2 },
        ENTITY_CREATION_COLOR = { red: 192, green: 192, blue: 192 },

        PALETTE_ITEM = {
            overlay: "cube",  // Invisible cube for hit area.
            properties: {
                dimensions: UIT.dimensions.itemCollisionZone,
                localRotation: Quat.ZERO,
                alpha: 0.0,  // Invisible.
                solid: true,
                ignoreRayIntersection: false,
                visible: true  // So that laser intersects.
            },
            hoverButton: {
                // Relative to root overlay.
                overlay: "cube",
                properties: {
                    dimensions: UIT.dimensions.paletteItemButtonDimensions,
                    localPosition: UIT.dimensions.paletteItemButtonOffset,
                    localRotation: Quat.ZERO,
                    color: UIT.colors.blueHighlight,
                    alpha: 1.0,
                    emissive: true,  // TODO: This has no effect.
                    solid: true,
                    ignoreRayIntersection: true,
                    visible: false
                }
            },
            icon: {
                // Relative to hoverButton.
                overlay: "model",
                properties: {
                    dimensions: UIT.dimensions.paletteItemIconDimensions,
                    localPosition: UIT.dimensions.paletteItemIconOffset,
                    localRotation: Quat.ZERO,
                    emissive: true,  // TODO: This has no effect.
                    ignoreRayIntersection: true
                }
            },
            entity: {
                dimensions: ENTITY_CREATION_DIMENSIONS
            }
        },

        PALETTE_ITEMS = [
            {
                icon: {
                    properties: {
                        url: "../assets/create/cube.fbx"
                    }
                },
                entity: {
                    type: "Box",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                icon: {
                    properties: {
                        url: "../assets/create/sphere.fbx"
                    }
                },
                entity: {
                    type: "Sphere",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                icon: {
                    properties: {
                        url: "../assets/create/tetrahedron.fbx"
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Tetrahedron",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                icon: {
                    properties: {
                        url: "../assets/create/octahedron.fbx"
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Octahedron",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                icon: {
                    properties: {
                        url: "../assets/create/icosahedron.fbx"
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Icosahedron",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            // TODO: "Dodecahedron" shape type per edit.js.
            // TODO: "Hexagon" shape type per edit.js.
            {
                icon: {
                    properties: {
                        url: "../assets/create/prism.fbx",
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 })
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Triangle",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            // TODO: "Octagon" shape type per edit.js.
            {
                icon: {
                    properties: {
                        url: "../assets/create/cylinder.fbx",
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 })
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cylinder",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            },
            {
                icon: {
                    properties: {
                        url: "../assets/create/cone.fbx",
                        localRotation: Quat.fromVec3Degrees({ x: 90, y: 0, z: 0 })
                    }
                },
                entity: {
                    type: "Shape",
                    shape: "Cone",
                    dimensions: ENTITY_CREATION_DIMENSIONS,
                    color: ENTITY_CREATION_COLOR
                }
            }
            // TODO: "Circle" shape type per edit.js.
        ],

        isDisplaying = false,

        NONE = -1,
        highlightedItem = NONE,
        wasTriggerClicked = false,

        // References.
        controlHand;

    if (!this instanceof CreatePalette) {
        return new CreatePalette();
    }


    function setHand(hand) {
        // Assumes UI is not displaying.
        side = hand;
        controlHand = side === LEFT_HAND ? rightInputs.hand() : leftInputs.hand();
        controlJointName = side === LEFT_HAND ? "LeftHand" : "RightHand";
        paletteLateralOffset = side === LEFT_HAND ? -UIT.dimensions.handLateralOffset : UIT.dimensions.handLateralOffset;
    }

    setHand(side);

    function otherHand(side) {
        return (side + 1) % 2;
    }

    function getEntityIDs() {
        return [palettePanelOverlay, paletteHeaderHeadingOverlay, paletteHeaderBarOverlay].concat(paletteItemOverlays);
    }

    function setVisible(visible) {
        var i,
            length;

        for (i = 0, length = staticOverlays.length; i < length; i += 1) {
            Overlays.editOverlay(staticOverlays[i], { visible: visible });
        }

        if (!visible) {
            for (i = 0, length = paletteItemHoverOverlays.length; i < length; i += 1) {
                Overlays.editOverlay(paletteItemHoverOverlays[i], { visible: false });
            }
        }
    }

    function update(intersectionOverlayID) {
        var itemIndex,
            isTriggerClicked,
            properties,
            CREATE_OFFSET = { x: 0, y: 0.05, z: -0.02 },
            INVERSE_HAND_BASIS_ROTATION = Quat.fromVec3Degrees({ x: 0, y: 0, z: -90 });

        itemIndex = paletteItemOverlays.indexOf(intersectionOverlayID);

        // Unhighlight and lower old item.
        if (highlightedItem !== NONE && (itemIndex === NONE || itemIndex !== highlightedItem)) {
            Overlays.editOverlay(paletteItemHoverOverlays[highlightedItem], {
                localPosition: UIT.dimensions.paletteItemButtonOffset,
                visible: false
            });
            highlightedItem = NONE;
        }

        // Highlight and raise new item.
        if (itemIndex !== NONE && highlightedItem !== itemIndex) {
            Feedback.play(side, Feedback.HOVER_BUTTON);
            Overlays.editOverlay(paletteItemHoverOverlays[itemIndex], {
                localPosition: UIT.dimensions.paletteItemButtonHoveredOffset,
                visible: true
            });
            highlightedItem = itemIndex;
        }

        // Press item and create new entity.
        isTriggerClicked = controlHand.triggerClicked();
        if (highlightedItem !== NONE && isTriggerClicked && !wasTriggerClicked) {
            // Create entity.
            Feedback.play(otherHand(side), Feedback.CREATE_ENTITY);
            properties = Object.clone(PALETTE_ITEMS[itemIndex].entity);
            properties.position = Vec3.sum(controlHand.palmPosition(),
                Vec3.multiplyQbyV(controlHand.orientation(),
                    Vec3.sum({ x: 0, y: properties.dimensions.z / 2, z: 0 }, CREATE_OFFSET)));
            properties.rotation = Quat.multiply(controlHand.orientation(), INVERSE_HAND_BASIS_ROTATION);
            Entities.addEntity(properties);

            // Lower and unhighlight item.
            Overlays.editOverlay(paletteItemHoverOverlays[itemIndex], {
                localPosition: UIT.dimensions.paletteItemButtonOffset,
                visible: false
            });

            uiCommandCallback("autoGrab");
        }

        wasTriggerClicked = isTriggerClicked;
    }

    function itemPosition(index) {
        // Position relative to palette panel.
        var ITEMS_PER_ROW = 4,
            ROW_ZERO_Y_OFFSET = 0.0580,
            ROW_SPACING = 0.0560,
            COLUMN_ZERO_OFFSET = -0.08415,
            COLUMN_SPACING = 0.0561,
            row,
            column;

        row = Math.floor(index / ITEMS_PER_ROW);
        column = index % ITEMS_PER_ROW;

        return {
            x: COLUMN_ZERO_OFFSET + column * COLUMN_SPACING,
            y: ROW_ZERO_Y_OFFSET - row * ROW_SPACING,
            z: UIT.dimensions.panel.z / 2 + UIT.dimensions.itemCollisionZone.z / 2
        };
    }

    function display() {
        // Creates and shows menu entities.
        var handJointIndex,
            properties,
            i,
            length;

        if (isDisplaying) {
            return;
        }

        // Joint index.
        handJointIndex = MyAvatar.getJointIndex(controlJointName);
        if (handJointIndex === NONE) {
            // Don't display if joint isn't available (yet) to attach to.
            // User can clear this condition by toggling the app off and back on once avatar finishes loading.
            App.log(side, "ERROR: CreatePalette: Hand joint index isn't available!");
            return;
        }

        // Calculate position to put palette.
        properties = Object.clone(PALETTE_ORIGIN_PROPERTIES);
        properties.parentJointIndex = handJointIndex;
        properties.localPosition = Vec3.sum(PALETTE_ORIGIN_POSITION, { x: paletteLateralOffset, y: 0, z: 0 });
        paletteOriginOverlay = Overlays.addOverlay("sphere", properties);

        // Header.
        properties = Object.clone(PALETTE_HEADER_HEADING_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        paletteHeaderHeadingOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(PALETTE_HEADER_BAR_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        paletteHeaderBarOverlay = Overlays.addOverlay("cube", properties);
        properties = Object.clone(PALETTE_TITLE_PROPERTIES);
        properties.parentID = paletteHeaderHeadingOverlay;
        properties.url = Script.resolvePath(properties.url);
        paletteTitleOverlay = Overlays.addOverlay("image3d", properties);

        // Palette background.
        properties = Object.clone(PALETTE_PANEL_PROPERTIES);
        properties.parentID = paletteOriginOverlay;
        palettePanelOverlay = Overlays.addOverlay("cube", properties);

        // Palette items.
        for (i = 0, length = PALETTE_ITEMS.length; i < length; i += 1) {
            // Collision overlay.
            properties = Object.clone(PALETTE_ITEM.properties);
            properties.parentID = palettePanelOverlay;
            properties.localPosition = itemPosition(i);

            paletteItemOverlays[i] = Overlays.addOverlay(PALETTE_ITEM.overlay, properties);
            paletteItemPositions[i] = properties.localPosition;

            // Highlight overlay.
            properties = Object.clone(PALETTE_ITEM.hoverButton.properties);
            properties.parentID = paletteItemOverlays[i];
            paletteItemHoverOverlays[i] = Overlays.addOverlay(PALETTE_ITEM.hoverButton.overlay, properties);

            // Icon overlay.
            properties = Object.clone(PALETTE_ITEM.icon.properties);
            properties = Object.merge(properties, PALETTE_ITEMS[i].icon.properties);
            properties.parentID = paletteItemHoverOverlays[i];
            properties.url = Script.resolvePath(properties.url);
            iconOverlays[i] = Overlays.addOverlay(PALETTE_ITEM.icon.overlay, properties);
        }

        // Always-visible overlays.
        staticOverlays = [].concat(paletteHeaderHeadingOverlay, paletteHeaderBarOverlay, paletteTitleOverlay,
            palettePanelOverlay, paletteItemOverlays, iconOverlays);

        isDisplaying = true;
    }

    function clear() {
        // Deletes menu entities.
        if (!isDisplaying) {
            return;
        }
        Overlays.deleteOverlay(paletteOriginOverlay);  // Automatically deletes all other overlays because they're children.
        paletteItemOverlays = [];
        paletteItemHoverOverlays = [];
        iconOverlays = [];
        staticOverlays = [];
        isDisplaying = false;
    }

    function destroy() {
        clear();
    }

    return {
        setHand: setHand,
        entityIDs: getEntityIDs,  // TODO: Rename to overlayIDs.
        setVisible: setVisible,
        update: update,
        display: display,
        clear: clear,
        destroy: destroy
    };
};

CreatePalette.prototype = {};
