#include "Input.hpp"
#include "Config.hpp"

void Input::update() {
    primaryFire_ = false;
    dash_ = false;
    nova_ = false;
    useLifeFlask_ = false;
    useManaFlask_ = false;
    secondarySkill_ = false;
    pickup_ = false;
    passiveTreeToggle_ = false;
    skillPanelToggle_ = false;
    skillPanelAlternateChoice_ = false;
    nextMap_ = false;
    inventorySelectNext_ = false;
    inventoryDropSelected_ = false;
    inventorySalvageSelected_ = false;
    craftingToggle_ = false;
    stashStoreSelected_ = false;
    stashWithdrawSelected_ = false;
    mapDeviceToggle_ = false;
    atlasPanelToggle_ = false;
    saveRun_ = false;
    loadRun_ = false;
    cancel_ = false;
    quit_ = false;
    escapePressed_ = false;
    numberChoice_ = 0;
    functionChoice_ = 0;
}

void Input::handleKeyPressed(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::Left:  moveLeft_ = true; break;
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Right: moveRight_ = true; break;
        case sf::Keyboard::Key::W:
        case sf::Keyboard::Key::Up:    moveUp_ = true; break;
        case sf::Keyboard::Key::S:
        case sf::Keyboard::Key::Down:  moveDown_ = true; break;
        case sf::Keyboard::Key::Space: dash_ = true; break;
        case sf::Keyboard::Key::Q:     nova_ = true; quit_ = true; break;
        case sf::Keyboard::Key::G:     useLifeFlask_ = true; break;
        case sf::Keyboard::Key::H:     useManaFlask_ = true; break;
        case sf::Keyboard::Key::F:     pickup_ = true; break;
        case sf::Keyboard::Key::P:     passiveTreeToggle_ = true; break;
        case sf::Keyboard::Key::K:     skillPanelToggle_ = true; break;
        case sf::Keyboard::Key::Y:     skillPanelAlternateChoice_ = true; break;
        case sf::Keyboard::Key::E:     nextMap_ = true; break;
        case sf::Keyboard::Key::R:     restart_ = true; break;
        case sf::Keyboard::Key::Escape: escapePressed_ = true; cancel_ = true; break;
        case sf::Keyboard::Key::Tab: inventorySelectNext_ = true; break;
        case sf::Keyboard::Key::Delete: inventoryDropSelected_ = true; break;
        case sf::Keyboard::Key::C: inventorySalvageSelected_ = true; break;
        case sf::Keyboard::Key::V: craftingToggle_ = true; break;
        case sf::Keyboard::Key::I: stashStoreSelected_ = true; break;
        case sf::Keyboard::Key::O: stashWithdrawSelected_ = true; break;
        case sf::Keyboard::Key::M: mapDeviceToggle_ = true; break;
        case sf::Keyboard::Key::T: atlasPanelToggle_ = true; break;
        case sf::Keyboard::Key::Num1: numberChoice_ = 1; break;
        case sf::Keyboard::Key::Num2: numberChoice_ = 2; break;
        case sf::Keyboard::Key::Num3: numberChoice_ = 3; break;
        case sf::Keyboard::Key::Num4: numberChoice_ = 4; break;
        case sf::Keyboard::Key::Num5: numberChoice_ = 5; break;
        case sf::Keyboard::Key::Num6: numberChoice_ = 6; break;
        case sf::Keyboard::Key::Num7: numberChoice_ = 7; break;
        case sf::Keyboard::Key::Num8: numberChoice_ = 8; break;
        case sf::Keyboard::Key::Num9: numberChoice_ = 9; break;
        case sf::Keyboard::Key::Num0: numberChoice_ = 10; break;
        case sf::Keyboard::Key::F1: functionChoice_ = 1; break;
        case sf::Keyboard::Key::F2: functionChoice_ = 2; break;
        case sf::Keyboard::Key::F3: functionChoice_ = 3; break;
        case sf::Keyboard::Key::F4: functionChoice_ = 4; break;
        case sf::Keyboard::Key::F5: functionChoice_ = 5; saveRun_ = true; break;
        case sf::Keyboard::Key::F6: functionChoice_ = 6; break;
        case sf::Keyboard::Key::F7: functionChoice_ = 7; break;
        case sf::Keyboard::Key::F8: functionChoice_ = 8; break;
        case sf::Keyboard::Key::F9: functionChoice_ = 9; loadRun_ = true; break;
        case sf::Keyboard::Key::F10: functionChoice_ = 10; break;
        case sf::Keyboard::Key::F11: functionChoice_ = 11; break;
        case sf::Keyboard::Key::F12: functionChoice_ = 12; break;
        case sf::Keyboard::Key::F13: functionChoice_ = 13; break;
        case sf::Keyboard::Key::F14: functionChoice_ = 14; break;
        case sf::Keyboard::Key::F15: functionChoice_ = 15; break;
        case sf::Keyboard::Key::Numpad1: numberChoice_ = 1; break;
        case sf::Keyboard::Key::Numpad2: numberChoice_ = 2; break;
        case sf::Keyboard::Key::Numpad3: numberChoice_ = 3; break;
        case sf::Keyboard::Key::Numpad4: numberChoice_ = 4; break;
        case sf::Keyboard::Key::Numpad5: numberChoice_ = 5; break;
        case sf::Keyboard::Key::Numpad6: numberChoice_ = 6; break;
        case sf::Keyboard::Key::Numpad7: numberChoice_ = 7; break;
        case sf::Keyboard::Key::Numpad8: numberChoice_ = 8; break;
        case sf::Keyboard::Key::Numpad9: numberChoice_ = 9; break;
        case sf::Keyboard::Key::Numpad0: numberChoice_ = 10; break;
        default: break;
    }
}

void Input::handleKeyReleased(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::A:
        case sf::Keyboard::Key::Left:  moveLeft_ = false; break;
        case sf::Keyboard::Key::D:
        case sf::Keyboard::Key::Right: moveRight_ = false; break;
        case sf::Keyboard::Key::W:
        case sf::Keyboard::Key::Up:    moveUp_ = false; break;
        case sf::Keyboard::Key::S:
        case sf::Keyboard::Key::Down:  moveDown_ = false; break;
        case sf::Keyboard::Key::R:     restart_ = false; break;
        default: break;
    }
}

void Input::handleMouseMoved(sf::Vector2i position) {
    mousePosition_ = position;
}

void Input::handleMousePressed(sf::Mouse::Button button, sf::Vector2i position) {
    mousePosition_ = position;

    if (button == sf::Mouse::Button::Right) {
        secondarySkill_ = true;
        return;
    }

    if (button != sf::Mouse::Button::Left) {
        return;
    }

    primaryFire_ = true;
    primaryFireHeld_ = true;
}

void Input::handleMouseReleased(sf::Mouse::Button button, sf::Vector2i position) {
    mousePosition_ = position;

    if (button == sf::Mouse::Button::Left) {
        primaryFireHeld_ = false;
    }
}

bool Input::moveLeft() const { return moveLeft_; }
bool Input::moveRight() const { return moveRight_; }
bool Input::moveUp() const { return moveUp_; }
bool Input::moveDown() const { return moveDown_; }
bool Input::primaryFire() const { return primaryFire_; }
bool Input::leftMousePressed() const { return primaryFire_; }
bool Input::primaryFireHeld() const { return primaryFireHeld_; }
bool Input::dash() const { return dash_; }
bool Input::nova() const { return nova_; }
bool Input::useLifeFlask() const { return useLifeFlask_; }
bool Input::useManaFlask() const { return useManaFlask_; }
bool Input::secondarySkill() const { return secondarySkill_; }
bool Input::pickup() const { return pickup_; }
bool Input::passiveTreeToggle() const { return passiveTreeToggle_; }
bool Input::skillPanelToggle() const { return skillPanelToggle_; }
bool Input::skillPanelAlternateChoice() const { return skillPanelAlternateChoice_; }
bool Input::nextMap() const { return nextMap_; }
bool Input::restart() const { return restart_; }
bool Input::quit() const { return quit_; }
bool Input::escapePressed() const { return escapePressed_; }
int Input::numberChoice() const { return numberChoice_; }
int Input::functionChoice() const { return functionChoice_; }
bool Input::inventorySelectNext() const { return inventorySelectNext_; }
bool Input::inventoryDropSelected() const { return inventoryDropSelected_; }
bool Input::inventorySalvageSelected() const { return inventorySalvageSelected_; }
bool Input::craftingToggle() const { return craftingToggle_; }
bool Input::stashStoreSelected() const { return stashStoreSelected_; }
bool Input::stashWithdrawSelected() const { return stashWithdrawSelected_; }
bool Input::mapDeviceToggle() const { return mapDeviceToggle_; }
bool Input::atlasPanelToggle() const { return atlasPanelToggle_; }
bool Input::saveRun() const { return saveRun_; }
bool Input::loadRun() const { return loadRun_; }
bool Input::cancel() const { return cancel_; }
sf::Vector2i Input::mousePosition() const { return mousePosition_; }
