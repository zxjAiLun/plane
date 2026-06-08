#include "Input.hpp"
#include "Config.hpp"

void Input::update() {
    primaryFire_ = false;
    dash_ = false;
    upgradeChoice_ = 0;
}

void Input::handleKeyPressed(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Left:  moveLeft_ = true; break;
        case sf::Keyboard::Key::Right: moveRight_ = true; break;
        case sf::Keyboard::Key::Up:    moveUp_ = true; break;
        case sf::Keyboard::Key::Down:  moveDown_ = true; break;
        case sf::Keyboard::Key::Space: dash_ = true; break;
        case sf::Keyboard::Key::R:     restart_ = true; break;
        case sf::Keyboard::Key::Escape: quit_ = true; break;
        case sf::Keyboard::Key::Num1: upgradeChoice_ = 1; break;
        case sf::Keyboard::Key::Num2: upgradeChoice_ = 2; break;
        case sf::Keyboard::Key::Num3: upgradeChoice_ = 3; break;
        case sf::Keyboard::Key::Numpad1: upgradeChoice_ = 1; break;
        case sf::Keyboard::Key::Numpad2: upgradeChoice_ = 2; break;
        case sf::Keyboard::Key::Numpad3: upgradeChoice_ = 3; break;
        default: break;
    }
}

void Input::handleKeyReleased(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Left:  moveLeft_ = false; break;
        case sf::Keyboard::Key::Right: moveRight_ = false; break;
        case sf::Keyboard::Key::Up:    moveUp_ = false; break;
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

    if (button != sf::Mouse::Button::Left) {
        return;
    }

    primaryFire_ = true;
    primaryFireHeld_ = true;

    const float centerX = Config::WindowWidth / 2.0f;
    const float centerY = Config::WindowHeight / 2.0f;
    constexpr float cardWidth = 350.0f;
    constexpr float cardHeight = 60.0f;

    for (int i = 0; i < 3; ++i) {
        const float cardY = centerY - 100.0f + i * 80.0f;
        const bool insideX = position.x >= centerX - cardWidth / 2.0f
            && position.x <= centerX + cardWidth / 2.0f;
        const bool insideY = position.y >= cardY - cardHeight / 2.0f
            && position.y <= cardY + cardHeight / 2.0f;

        if (insideX && insideY) {
            upgradeChoice_ = i + 1;
            return;
        }
    }
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
bool Input::restart() const { return restart_; }
bool Input::quit() const { return quit_; }
int Input::upgradeChoice() const { return upgradeChoice_; }
sf::Vector2i Input::mousePosition() const { return mousePosition_; }
