#include "Renderer.hpp"
#include "Config.hpp"

Renderer::Renderer(sf::RenderWindow& window)
    : window_(window)
    , fontLoaded_(false) {
    const char* fontPaths[] = {
        "assets/font.ttf",
        "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/ubuntu/UbuntuSans[wdth,wght].ttf",
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf"
    };

    for (const char* path : fontPaths) {
        if (font_.openFromFile(path)) {
            fontLoaded_ = true;
            break;
        }
    }
}

void Renderer::render(const GameWorld& world) {
    window_.clear(sf::Color::Black);

    drawPlayer(world);
    drawAimIndicator(world);
    drawProjectiles(world);
    drawEnemies(world);
    drawOrbs(world);

    drawText("HP " + std::to_string(world.player().hp()) + "/" + std::to_string(world.player().maxHp()),
        {16.0f, 12.0f}, 18, sf::Color::White);
    drawText("LV " + std::to_string(world.player().level())
        + "  EXP " + std::to_string(world.player().exp()) + "/" + std::to_string(world.player().expToNextLevel()),
        {16.0f, 36.0f}, 18, sf::Color::White);
    drawText("TIME " + std::to_string(static_cast<int>(world.survivalTime()))
        + "  SCORE " + std::to_string(world.score()),
        {16.0f, 60.0f}, 18, sf::Color::White);

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
    const auto& player = world.player();
    sf::CircleShape shape(player.radius());
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition({player.position().x, player.position().y});
    window_.draw(shape);
}

void Renderer::drawAimIndicator(const GameWorld& world) {
    const auto& player = world.player();
    const auto& aim = world.aimPosition();

    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = {player.position().x, player.position().y};
    line[0].color = sf::Color(120, 220, 255, 160);
    line[1].position = {aim.x, aim.y};
    line[1].color = sf::Color(120, 220, 255, 80);
    window_.draw(line);

    constexpr float reticleSize = 8.0f;
    sf::VertexArray reticle(sf::PrimitiveType::Lines, 4);
    reticle[0].position = {aim.x - reticleSize, aim.y};
    reticle[1].position = {aim.x + reticleSize, aim.y};
    reticle[2].position = {aim.x, aim.y - reticleSize};
    reticle[3].position = {aim.x, aim.y + reticleSize};

    for (std::size_t i = 0; i < reticle.getVertexCount(); ++i) {
        reticle[i].color = sf::Color(120, 220, 255);
    }

    window_.draw(reticle);
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

    drawBox({center.x, center.y - 50.0f}, {300.0f, 100.0f}, sf::Color::Red);
    drawCenteredText("GAME OVER", {center.x, center.y - 58.0f}, 28, sf::Color::White);
    drawBox({center.x, center.y + 50.0f}, {200.0f, 40.0f}, sf::Color::White);
    drawCenteredText("Press R", {center.x, center.y + 43.0f}, 20, sf::Color::Black);
}

void Renderer::drawLevelUp(const GameWorld& world) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 100, 180));
    window_.draw(overlay);

    const auto& upgrades = world.currentUpgrades();
    drawCenteredText("LEVEL UP", {center.x, center.y - 175.0f}, 30, sf::Color::White);
    for (int i = 0; i < 3; ++i) {
        float y = center.y - 100.0f + i * 80.0f;
        sf::Color color = sf::Color(50, 50, 150);
        if (i == 0) color = sf::Color(80, 80, 200);
        if (i == 1) color = sf::Color(60, 60, 180);
        if (i == 2) color = sf::Color(40, 40, 160);

        drawBox({center.x, y}, {350.0f, 60.0f}, color);
        drawText(std::to_string(i + 1) + ". " + upgrades[i].name,
            {center.x - 155.0f, y - 22.0f}, 18, sf::Color::White);
        drawText(upgrades[i].description,
            {center.x - 155.0f, y + 2.0f}, 14, sf::Color(210, 220, 255));
    }
}

void Renderer::drawVictory(const GameWorld& /*world*/) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 100, 0, 180));
    window_.draw(overlay);

    drawBox({center.x, center.y - 50.0f}, {300.0f, 100.0f}, sf::Color::Green);
    drawCenteredText("VICTORY", {center.x, center.y - 58.0f}, 28, sf::Color::White);
    drawBox({center.x, center.y + 50.0f}, {200.0f, 40.0f}, sf::Color::White);
    drawCenteredText("Press R", {center.x, center.y + 43.0f}, 20, sf::Color::Black);
}

void Renderer::drawBox(const sf::Vector2f& center, const sf::Vector2f& size, const sf::Color& color) {
    sf::RectangleShape box(size);
    box.setFillColor(color);
    box.setOrigin({size.x / 2.0f, size.y / 2.0f});
    box.setPosition(center);
    window_.draw(box);
}

void Renderer::drawText(const std::string& text, const sf::Vector2f& position, unsigned int size, const sf::Color& color) {
    if (!fontLoaded_) {
        return;
    }

    sf::Text drawable(font_, text, size);
    drawable.setFillColor(color);
    drawable.setPosition(position);
    window_.draw(drawable);
}

void Renderer::drawCenteredText(const std::string& text, const sf::Vector2f& center, unsigned int size, const sf::Color& color) {
    if (!fontLoaded_) {
        return;
    }

    sf::Text drawable(font_, text, size);
    drawable.setFillColor(color);
    const auto bounds = drawable.getLocalBounds();
    drawable.setOrigin({bounds.position.x + bounds.size.x / 2.0f, bounds.position.y + bounds.size.y / 2.0f});
    drawable.setPosition(center);
    window_.draw(drawable);
}
