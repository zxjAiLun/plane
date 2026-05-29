#pragma once

#include <SFML/Graphics.hpp>
#include "GameWorld.hpp"

class Renderer {
public:
    explicit Renderer(sf::RenderWindow& window);

    void render(const GameWorld& world);

private:
    void drawPlayer(const GameWorld& world);
    void drawProjectiles(const GameWorld& world);
    void drawEnemies(const GameWorld& world);
    void drawOrbs(const GameWorld& world);
    void drawGameOver(const GameWorld& world);
    void drawLevelUp(const GameWorld& world);
    void drawVictory(const GameWorld& world);

private:
    sf::RenderWindow& window_;
};
