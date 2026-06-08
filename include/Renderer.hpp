#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "GameWorld.hpp"

class Renderer {
public:
    explicit Renderer(sf::RenderWindow& window);

    void render(const GameWorld& world);

private:
    void drawPlayer(const GameWorld& world);
    void drawAimIndicator(const GameWorld& world);
    void drawProjectiles(const GameWorld& world);
    void drawEnemies(const GameWorld& world);
    void drawOrbs(const GameWorld& world);
    void drawGameOver(const GameWorld& world);
    void drawLevelUp(const GameWorld& world);
    void drawVictory(const GameWorld& world);

    void drawBox(const sf::Vector2f& center, const sf::Vector2f& size, const sf::Color& color);
    void drawText(const std::string& text, const sf::Vector2f& position, unsigned int size, const sf::Color& color);
    void drawCenteredText(const std::string& text, const sf::Vector2f& center, unsigned int size, const sf::Color& color);

private:
    sf::RenderWindow& window_;
    sf::Font font_;
    bool fontLoaded_;
};
