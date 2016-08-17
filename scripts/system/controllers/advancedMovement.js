var mappingName, advancedMapping;

function addAdvancedMovementItemToSettingsMenu() {
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: "Advanced Movement",
        isCheckable: true,
        isChecked: false
    });

}

function addTranslationToLeftStick() {
    Controller.enableMapping(mappingName);
}

function registerMappings() {
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    advancedMapping = Controller.newMapping(mappingName);
    advancedMapping.from(Controller.Vive.LY).when(Controller.Vive.LSY).invert().to(Controller.Standard.LY);
    advancedMapping.from(Controller.Vive.LX).when(Controller.Vive.LSX).to(Controller.Standard.LX);
    advancedMapping.from(Controller.Vive.RY).when(Controller.Vive.RSY).invert().to(Controller.Standard.RY);
}

function removeTranslationFromLeftStick() {
    Controller.disableMapping(mappingName);
}

function scriptEnding() {
    Menu.removeMenuItem("Settings", "Advanced Movement");
    removeTranslationFromLeftStick();
}

function menuItemEvent(menuItem) {
    if (menuItem == "Advanced Movement") {
        print("  checked=" + Menu.isOptionChecked("Advanced Movement"));
        var isChecked = Menu.isOptionChecked("Advanced Movement");
        if (isChecked === true) {
            addTranslationToLeftStick();
        } else if (isChecked === false) {
            removeTranslationFromLeftStick();
        }
    }

}

addAdvancedMovementItemToSettingsMenu();

// register our scriptEnding callback
Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);

registerMappings();