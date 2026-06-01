#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Vector2.hpp>

class Input {
public:
    void update();

    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);
    void handleMousePressed(sf::Mouse::Button button, sf::Vector2i position);

    bool moveLeft() const;
    bool moveRight() const;
    bool moveUp() const;
    bool moveDown() const;
    bool restart() const;
    bool quit() const;
    int upgradeChoice() const;

private:
    bool moveLeft_ = false;
    bool moveRight_ = false;
    bool moveUp_ = false;
    bool moveDown_ = false;
    bool restart_ = false;
    bool quit_ = false;
    int upgradeChoice_ = 0;
};
