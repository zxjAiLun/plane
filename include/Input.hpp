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
    bool leftMousePressed() const;
    bool primaryFireHeld() const;
    bool dash() const;
    bool nova() const;
    bool useLifeFlask() const;
    bool secondarySkill() const;
    bool pickup() const;
    bool passiveTreeToggle() const;
    bool skillPanelToggle() const;
    bool nextMap() const;
    bool restart() const;
    bool quit() const;
    int numberChoice() const;
    int functionChoice() const;
    bool inventorySelectNext() const;
    bool inventoryDropSelected() const;
    bool inventorySalvageSelected() const;
    bool craftingToggle() const;
    bool stashStoreSelected() const;
    bool stashWithdrawSelected() const;
    bool saveRun() const;
    bool loadRun() const;
    bool cancel() const;
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
    bool useLifeFlask_ = false;
    bool secondarySkill_ = false;
    bool pickup_ = false;
    bool passiveTreeToggle_ = false;
    bool skillPanelToggle_ = false;
    bool nextMap_ = false;
    bool restart_ = false;
    bool quit_ = false;
    int numberChoice_ = 0;
    int functionChoice_ = 0;
    bool inventorySelectNext_ = false;
    bool inventoryDropSelected_ = false;
    bool inventorySalvageSelected_ = false;
    bool craftingToggle_ = false;
    bool stashStoreSelected_ = false;
    bool stashWithdrawSelected_ = false;
    bool saveRun_ = false;
    bool loadRun_ = false;
    bool cancel_ = false;
    sf::Vector2i mousePosition_{0, 0};
};
