#include "Input.hpp"

void Input::update() {
}

void Input::handleKeyPressed(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::Left:  moveLeft_ = true; break;
        case sf::Keyboard::Key::Right: moveRight_ = true; break;
        case sf::Keyboard::Key::Up:    moveUp_ = true; break;
        case sf::Keyboard::Key::Down:  moveDown_ = true; break;
        case sf::Keyboard::Key::R:     restart_ = true; break;
        case sf::Keyboard::Key::Escape: quit_ = true; break;
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

bool Input::moveLeft() const { return moveLeft_; }
bool Input::moveRight() const { return moveRight_; }
bool Input::moveUp() const { return moveUp_; }
bool Input::moveDown() const { return moveDown_; }
bool Input::restart() const { return restart_; }
bool Input::quit() const { return quit_; }
