//
//  marketplace.js
//
//  Created by Eric Levin on 8 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var toolIconUrl = Script.resolvePath("assets/images/tools/");

var MARKETPLACE_URL = "https://metaverse.highfidelity.com/marketplace";
var CLARA_URL = "https://clara.io/library";
var marketplaceWindow = new OverlayWindow({
    title: "Marketplace",
    source: "about:blank",
    //source: MARKETPLACE_URL,
    //source: "https://s3.amazonaws.com/DreamingContent/qml/content.qml",
    //source: "C:\Users\elisa\Documents\GitHub\hifi\interface\resources\qml\controls-uit\WebView.qml",
    width: 900,
    height: 700,
    toolWindow: false,
    visible: false,
});

var toolHeight = 50;
var toolWidth = 50;
var TOOLBAR_MARGIN_Y = 0;


function showMarketplace(marketplaceID) {
    var url = MARKETPLACE_URL;
    if (marketplaceID) {
        url = url + "/items/" + marketplaceID;
    }
    marketplaceWindow.setVisible(true);
    //marketplaceWindow.webview.url = url;

    UserActivityLogger.openedMarketplace();
}

function hideMarketplace() {
    marketplaceWindow.setVisible(false);
    marketplaceWindow.webview.url = "about:blank";
}

function toggleMarketplace() {
    if (marketplaceWindow.visible) {
        hideMarketplace();
    } else {
        showMarketplace();
    }
}

var toolBar = Toolbars.getToolbar("com.highfidelity.interface.toolbar.system");

var browseExamplesButton = toolBar.addButton({
    imageURL: toolIconUrl + "market.svg",
    objectName: "marketplace",
    buttonState: 1,
    defaultState: 1,
    hoverState: 3,
    alpha: 0.9
});

function onExamplesWindowVisibilityChanged() {
    browseExamplesButton.writeProperty('buttonState', marketplaceWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('defaultState', marketplaceWindow.visible ? 0 : 1);
    browseExamplesButton.writeProperty('hoverState', marketplaceWindow.visible ? 2 : 3);
}
function onClick() {
    toggleMarketplace();
}
browseExamplesButton.clicked.connect(onClick);
marketplaceWindow.visibleChanged.connect(onExamplesWindowVisibilityChanged);

Script.scriptEnding.connect(function () {
    toolBar.removeButton("marketplace");
    browseExamplesButton.clicked.disconnect(onClick);
    marketplaceWindow.visibleChanged.disconnect(onExamplesWindowVisibilityChanged);
});
