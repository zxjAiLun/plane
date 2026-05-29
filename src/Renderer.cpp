#include "Renderer.hpp"
#include "Config.hpp"

Renderer::Renderer(sf::RenderWindow& window)
    : window_(window) {
}

void Renderer::render(const GameWorld& world) {
    window_.clear(sf::Color::Black);

    drawPlayer(world);
    drawBullets(world);
    drawEnemies(world);

    if (world.isGameOver()) {
        drawGameOver(world);
    }

    window_.display();
}

void Renderer::drawPlayer(const GameWorld& world) {
    if (world.isGameOver()) {
        return;
    }

    const auto& player = world.player();
    sf::CircleShape shape(player.radius());
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition({player.position().x, player.position().y});
    window_.draw(shape);
}

void Renderer::drawBullets(const GameWorld& world) {
    for (const auto& bullet : world.bullets()) {
        sf::CircleShape shape(bullet.radius());
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({bullet.radius(), bullet.radius()});
        shape.setPosition({bullet.position().x, bullet.position().y});
        window_.draw(shape);
    }
}

void Renderer::drawEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies()) {
        sf::RectangleShape shape({enemy.radius() * 2, enemy.radius() * 2});
        shape.setFillColor(sf::Color::Red);
        shape.setOrigin({enemy.radius(), enemy.radius()});
        shape.setPosition({enemy.position().x, enemy.position().y});
        window_.draw(shape);
    }
}

void Renderer::drawGameOver(const GameWorld& /*world*/) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window_.draw(overlay);

    sf::RectangleShape gameOverBox({300.0f, 100.0f});
    gameOverBox.setFillColor(sf::Color::Red);
    gameOverBox.setOrigin({150.0f, 50.0f});
    gameOverBox.setPosition({center.x, center.y - 50.0f});
    window_.draw(gameOverBox);

    sf::RectangleShape restartBox({200.0f, 40.0f});
    restartBox.setFillColor(sf::Color::White);
    restartBox.setOrigin({100.0f, 20.0f});
    restartBox.setPosition({center.x, center.y + 50.0f});
    window_.draw(restartBox);
}
