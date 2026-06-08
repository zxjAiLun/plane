#pragma once

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Vector2.hpp>

class Input {
public:
    void update();

    void handleKeyPressed(sf::Keyboard::Key key);
    void handleKeyReleased(sf::Keyboard::Key key);
    void handleMouseMoved(sf::Vector2i position);
    void handleMousePressed(sf::Mouse::Button button, sf::Vector2i position);
    void handleMouseReleased(sf::Mouse::Button button, sf::Vector2i position);

    bool moveLeft() const;
    bool moveRight() const;
    bool moveUp() const;
    bool moveDown() const;
    bool primaryFire() const;
    bool primaryFireHeld() const;
    bool dash() const;
    bool nova() const;
    bool restart() const;
    bool quit() const;
    int upgradeChoice() const;
    sf::Vector2i mousePosition() const;

private:
    bool moveLeft_ = false;
    bool moveRight_ = false;
    bool moveUp_ = false;
    bool moveDown_ = false;
    bool primaryFire_ = false;
    bool primaryFireHeld_ = false;
    bool dash_ = false;
    bool nova_ = false;
    bool restart_ = false;
    bool quit_ = false;
    int upgradeChoice_ = 0;
    sf::Vector2i mousePosition_{0, 0};
};
