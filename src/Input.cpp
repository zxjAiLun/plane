#include "Input.hpp"
#include "Config.hpp"

void Input::update() {
    primaryFire_ = false;
    dash_ = false;
    nova_ = false;
    secondarySkill_ = false;
    pickup_ = false;
    talentDamage_ = false;
    talentAttackSpeed_ = false;
    talentMoveSpeed_ = false;
    talentMaxHp_ = false;
    talentPickupRange_ = false;
    nextMap_ = false;
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
        case sf::Keyboard::Key::Z:     talentDamage_ = true; break;
        case sf::Keyboard::Key::X:     talentAttackSpeed_ = true; break;
        case sf::Keyboard::Key::C:     talentMoveSpeed_ = true; break;
        case sf::Keyboard::Key::V:     talentMaxHp_ = true; break;
        case sf::Keyboard::Key::B:     talentPickupRange_ = true; break;
        case sf::Keyboard::Key::E:     nextMap_ = true; break;
        case sf::Keyboard::Key::R:     restart_ = true; break;
        case sf::Keyboard::Key::Escape: quit_ = true; break;
        case sf::Keyboard::Key::Num1: rewardChoice_ = 1; inventoryChoice_ = 1; break;
        case sf::Keyboard::Key::Num2: rewardChoice_ = 2; inventoryChoice_ = 2; break;
        case sf::Keyboard::Key::Num3: rewardChoice_ = 3; inventoryChoice_ = 3; break;
        case sf::Keyboard::Key::Num4: inventoryChoice_ = 4; break;
        case sf::Keyboard::Key::Num5: inventoryChoice_ = 5; break;
        case sf::Keyboard::Key::Num6: inventoryChoice_ = 6; break;
        case sf::Keyboard::Key::Num7: inventoryChoice_ = 7; break;
        case sf::Keyboard::Key::Num8: inventoryChoice_ = 8; break;
        case sf::Keyboard::Key::Num9: inventoryChoice_ = 9; break;
        case sf::Keyboard::Key::Numpad1: rewardChoice_ = 1; inventoryChoice_ = 1; break;
        case sf::Keyboard::Key::Numpad2: rewardChoice_ = 2; inventoryChoice_ = 2; break;
        case sf::Keyboard::Key::Numpad3: rewardChoice_ = 3; inventoryChoice_ = 3; break;
        case sf::Keyboard::Key::Numpad4: inventoryChoice_ = 4; break;
        case sf::Keyboard::Key::Numpad5: inventoryChoice_ = 5; break;
        case sf::Keyboard::Key::Numpad6: inventoryChoice_ = 6; break;
        case sf::Keyboard::Key::Numpad7: inventoryChoice_ = 7; break;
        case sf::Keyboard::Key::Numpad8: inventoryChoice_ = 8; break;
        case sf::Keyboard::Key::Numpad9: inventoryChoice_ = 9; break;
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
bool Input::talentDamage() const { return talentDamage_; }
bool Input::talentAttackSpeed() const { return talentAttackSpeed_; }
bool Input::talentMoveSpeed() const { return talentMoveSpeed_; }
bool Input::talentMaxHp() const { return talentMaxHp_; }
bool Input::talentPickupRange() const { return talentPickupRange_; }
bool Input::nextMap() const { return nextMap_; }
bool Input::restart() const { return restart_; }
bool Input::quit() const { return quit_; }
int Input::rewardChoice() const { return rewardChoice_; }
int Input::inventoryChoice() const { return inventoryChoice_; }
sf::Vector2i Input::mousePosition() const { return mousePosition_; }
