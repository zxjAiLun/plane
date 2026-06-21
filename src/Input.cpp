#include "Input.hpp"
#include "Config.hpp"

void Input::update() {
    primaryFire_ = false;
    dash_ = false;
    nova_ = false;
    secondarySkill_ = false;
    pickup_ = false;
    passiveTreeToggle_ = false;
    nextMap_ = false;
    passiveChoice_ = 0;
    rewardChoice_ = 0;
    inventoryChoice_ = 0;
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
        case sf::Keyboard::Key::Q:     nova_ = true; break;
        case sf::Keyboard::Key::F:     pickup_ = true; break;
        case sf::Keyboard::Key::P:     passiveTreeToggle_ = true; break;
        case sf::Keyboard::Key::E:     nextMap_ = true; break;
        case sf::Keyboard::Key::R:     restart_ = true; break;
        case sf::Keyboard::Key::Escape: quit_ = true; break;
        case sf::Keyboard::Key::Num1: passiveChoice_ = 1; rewardChoice_ = 1; inventoryChoice_ = 1; break;
        case sf::Keyboard::Key::Num2: passiveChoice_ = 2; rewardChoice_ = 2; inventoryChoice_ = 2; break;
        case sf::Keyboard::Key::Num3: passiveChoice_ = 3; rewardChoice_ = 3; inventoryChoice_ = 3; break;
        case sf::Keyboard::Key::Num4: passiveChoice_ = 4; inventoryChoice_ = 4; break;
        case sf::Keyboard::Key::Num5: passiveChoice_ = 5; inventoryChoice_ = 5; break;
        case sf::Keyboard::Key::Num6: passiveChoice_ = 6; inventoryChoice_ = 6; break;
        case sf::Keyboard::Key::Num7: passiveChoice_ = 7; inventoryChoice_ = 7; break;
        case sf::Keyboard::Key::Num8: passiveChoice_ = 8; inventoryChoice_ = 8; break;
        case sf::Keyboard::Key::Num9: passiveChoice_ = 9; inventoryChoice_ = 9; break;
        case sf::Keyboard::Key::Num0: passiveChoice_ = 10; inventoryChoice_ = 10; break;
        case sf::Keyboard::Key::F1: passiveChoice_ = 11; break;
        case sf::Keyboard::Key::F2: passiveChoice_ = 12; break;
        case sf::Keyboard::Key::F3: passiveChoice_ = 13; break;
        case sf::Keyboard::Key::F4: passiveChoice_ = 14; break;
        case sf::Keyboard::Key::F5: passiveChoice_ = 15; break;
        case sf::Keyboard::Key::F6: passiveChoice_ = 16; break;
        case sf::Keyboard::Key::F7: passiveChoice_ = 17; break;
        case sf::Keyboard::Key::F8: passiveChoice_ = 18; break;
        case sf::Keyboard::Key::F9: passiveChoice_ = 19; break;
        case sf::Keyboard::Key::F10: passiveChoice_ = 20; break;
        case sf::Keyboard::Key::Numpad1: passiveChoice_ = 1; rewardChoice_ = 1; inventoryChoice_ = 1; break;
        case sf::Keyboard::Key::Numpad2: passiveChoice_ = 2; rewardChoice_ = 2; inventoryChoice_ = 2; break;
        case sf::Keyboard::Key::Numpad3: passiveChoice_ = 3; rewardChoice_ = 3; inventoryChoice_ = 3; break;
        case sf::Keyboard::Key::Numpad4: passiveChoice_ = 4; inventoryChoice_ = 4; break;
        case sf::Keyboard::Key::Numpad5: passiveChoice_ = 5; inventoryChoice_ = 5; break;
        case sf::Keyboard::Key::Numpad6: passiveChoice_ = 6; inventoryChoice_ = 6; break;
        case sf::Keyboard::Key::Numpad7: passiveChoice_ = 7; inventoryChoice_ = 7; break;
        case sf::Keyboard::Key::Numpad8: passiveChoice_ = 8; inventoryChoice_ = 8; break;
        case sf::Keyboard::Key::Numpad9: passiveChoice_ = 9; inventoryChoice_ = 9; break;
        case sf::Keyboard::Key::Numpad0: passiveChoice_ = 10; inventoryChoice_ = 10; break;
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
        case sf::Keyboard::Key::Escape: quit_ = false; break;
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
bool Input::primaryFireHeld() const { return primaryFireHeld_; }
bool Input::dash() const { return dash_; }
bool Input::nova() const { return nova_; }
bool Input::secondarySkill() const { return secondarySkill_; }
bool Input::pickup() const { return pickup_; }
bool Input::passiveTreeToggle() const { return passiveTreeToggle_; }
bool Input::nextMap() const { return nextMap_; }
bool Input::restart() const { return restart_; }
bool Input::quit() const { return quit_; }
int Input::passiveChoice() const { return passiveChoice_; }
int Input::rewardChoice() const { return rewardChoice_; }
int Input::inventoryChoice() const { return inventoryChoice_; }
sf::Vector2i Input::mousePosition() const { return mousePosition_; }
