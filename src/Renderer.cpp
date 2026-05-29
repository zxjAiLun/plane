#include "Renderer.hpp"
#include "Config.hpp"

Renderer::Renderer(sf::RenderWindow& window)
    : window_(window) {
}

void Renderer::render(const GameWorld& world) {
    window_.clear(sf::Color::Black);

    drawPlayer(world);
    drawProjectiles(world);
    drawEnemies(world);
    drawOrbs(world);

    switch (world.state()) {
        case GameState::GameOver:
            drawGameOver(world);
            break;
        case GameState::LevelUp:
            drawLevelUp(world);
            break;
        case GameState::Victory:
            drawVictory(world);
            break;
        case GameState::Playing:
            break;
    }

    window_.display();
}

void Renderer::drawPlayer(const GameWorld& world) {
    if (world.state() != GameState::Playing) {
        return;
    }

    const auto& player = world.player();
    sf::CircleShape shape(player.radius());
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition({player.position().x, player.position().y});
    window_.draw(shape);
}

void Renderer::drawProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.projectiles()) {
        sf::CircleShape shape(projectile.radius());
        shape.setFillColor(sf::Color::Yellow);
        shape.setOrigin({projectile.radius(), projectile.radius()});
        shape.setPosition({projectile.position().x, projectile.position().y});
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

void Renderer::drawOrbs(const GameWorld& world) {
    for (const auto& orb : world.orbs()) {
        sf::CircleShape shape(orb.radius());
        shape.setFillColor(sf::Color::Cyan);
        shape.setOrigin({orb.radius(), orb.radius()});
        shape.setPosition({orb.position().x, orb.position().y});
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

    sf::RectangleShape box({300.0f, 100.0f});
    box.setFillColor(sf::Color::Red);
    box.setOrigin({150.0f, 50.0f});
    box.setPosition({center.x, center.y - 50.0f});
    window_.draw(box);

    sf::RectangleShape restartBox({200.0f, 40.0f});
    restartBox.setFillColor(sf::Color::White);
    restartBox.setOrigin({100.0f, 20.0f});
    restartBox.setPosition({center.x, center.y + 50.0f});
    window_.draw(restartBox);
}

void Renderer::drawLevelUp(const GameWorld& /*world*/) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 100, 180));
    window_.draw(overlay);

    sf::RectangleShape box({300.0f, 150.0f});
    box.setFillColor(sf::Color::Blue);
    box.setOrigin({150.0f, 75.0f});
    box.setPosition({center.x, center.y});
    window_.draw(box);

    sf::RectangleShape confirmBox({200.0f, 40.0f});
    confirmBox.setFillColor(sf::Color::White);
    confirmBox.setOrigin({100.0f, 20.0f});
    confirmBox.setPosition({center.x, center.y + 80.0f});
    window_.draw(confirmBox);
}

void Renderer::drawVictory(const GameWorld& /*world*/) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 100, 0, 180));
    window_.draw(overlay);

    sf::RectangleShape box({300.0f, 100.0f});
    box.setFillColor(sf::Color::Green);
    box.setOrigin({150.0f, 50.0f});
    box.setPosition({center.x, center.y - 50.0f});
    window_.draw(box);

    sf::RectangleShape restartBox({200.0f, 40.0f});
    restartBox.setFillColor(sf::Color::White);
    restartBox.setOrigin({100.0f, 20.0f});
    restartBox.setPosition({center.x, center.y + 50.0f});
    window_.draw(restartBox);
}
