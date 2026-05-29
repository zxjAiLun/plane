#pragma once

#include <SFML/Graphics.hpp>
#include "GameWorld.hpp"

class Renderer {
public:
    explicit Renderer(sf::RenderWindow& window);

    void render(const GameWorld& world);

private:
    void drawPlayer(const GameWorld& world);
    void drawBullets(const GameWorld& world);
    void drawEnemies(const GameWorld& world);
    void drawGameOver(const GameWorld& world);

private:
    sf::RenderWindow& window_;
};
