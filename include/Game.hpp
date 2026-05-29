#pragma once

#include <SFML/Graphics.hpp>
#include "GameWorld.hpp"
#include "Renderer.hpp"
#include "Input.hpp"

class Game {
public:
    Game();
    void run();

private:
    void processInput();
    void update(float dt);
    void render();

private:
    sf::RenderWindow window_;
    GameWorld world_;
    Renderer renderer_;
    Input input_;
};
