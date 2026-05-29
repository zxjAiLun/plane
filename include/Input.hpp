#pragma once

#include <SFML/Window/Keyboard.hpp>

class Input {
public:
    void update();

    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);

    bool moveLeft() const;
    bool moveRight() const;
    bool moveUp() const;
    bool moveDown() const;
    bool shoot() const;
    bool restart() const;
    bool quit() const;

private:
    bool moveLeft_ = false;
    bool moveRight_ = false;
    bool moveUp_ = false;
    bool moveDown_ = false;
    bool shoot_ = false;
    bool restart_ = false;
    bool quit_ = false;
};
