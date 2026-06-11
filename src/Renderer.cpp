#include "Renderer.hpp"
#include "Config.hpp"

#include <algorithm>
#include <cstdint>

namespace {
sf::Color rarityColor(Rarity rarity) {
    switch (rarity) {
        case Rarity::Normal: return sf::Color(220, 220, 220);
        case Rarity::Magic: return sf::Color(90, 150, 255);
        case Rarity::Rare: return sf::Color(255, 210, 80);
    }
    return sf::Color::White;
}

int multiplierPercent(float multiplier) {
    return static_cast<int>((multiplier - 1.0f) * 100.0f + 0.5f);
}

std::string statsSummary(const Stats& stats) {
    std::string summary;
    if (stats.maxHp > 0) {
        summary += "+" + std::to_string(stats.maxHp) + " HP ";
    }
    if (stats.damageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.damageMultiplier)) + "% DMG ";
    }
    if (stats.attackSpeedMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier)) + "% AS ";
    }
    if (stats.moveSpeedMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier)) + "% MS ";
    }
    if (stats.pickupRangeMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.pickupRangeMultiplier)) + "% PICKUP ";
    }
    return summary;
}

std::string itemSummary(const Item& item) {
    return item.name + " " + statsSummary(item.stats);
}

Stats statsDelta(const Stats& next, const Stats& current) {
    return {
        next.maxHp - current.maxHp,
        next.moveSpeedMultiplier / current.moveSpeedMultiplier,
        next.damageMultiplier / current.damageMultiplier,
        next.attackSpeedMultiplier / current.attackSpeedMultiplier,
        next.pickupRangeMultiplier / current.pickupRangeMultiplier,
    };
}

std::string statsDeltaSummary(const Stats& delta) {
    std::string summary;
    if (delta.maxHp != 0) {
        summary += (delta.maxHp > 0 ? "+" : "") + std::to_string(delta.maxHp) + " HP ";
    }
    if (delta.damageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.damageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% DMG ";
    }
    if (delta.attackSpeedMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.attackSpeedMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% AS ";
    }
    if (delta.moveSpeedMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.moveSpeedMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% MS ";
    }
    if (delta.pickupRangeMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.pickupRangeMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% PICKUP ";
    }
    return summary.empty() ? "No stat change" : summary;
}

sf::Color deltaColor(const Stats& delta) {
    const bool positive = delta.maxHp > 0
        || delta.damageMultiplier > 1.0f
        || delta.attackSpeedMultiplier > 1.0f
        || delta.moveSpeedMultiplier > 1.0f
        || delta.pickupRangeMultiplier > 1.0f;
    const bool negative = delta.maxHp < 0
        || delta.damageMultiplier < 1.0f
        || delta.attackSpeedMultiplier < 1.0f
        || delta.moveSpeedMultiplier < 1.0f
        || delta.pickupRangeMultiplier < 1.0f;

    if (positive && !negative) {
        return sf::Color(120, 230, 140);
    }
    if (negative && !positive) {
        return sf::Color(240, 120, 120);
    }
    return sf::Color(230, 220, 150);
}
}

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

    drawMap(world);
    drawNovaEffect(world);
    drawSecondarySkillEffect(world);
    drawPlayer(world);
    drawAimIndicator(world);
    drawProjectiles(world);
    drawEnemies(world);
    drawDroppedItems(world);

    drawText("HP " + std::to_string(world.player().hp()) + "/" + std::to_string(world.player().maxHp()),
        {16.0f, 12.0f}, 18, sf::Color::White);
    drawText("LV " + std::to_string(world.player().level())
        + "  EXP " + std::to_string(world.player().exp()) + "/" + std::to_string(world.player().expToNextLevel())
        + "  SP " + std::to_string(world.player().talentPoints()),
        {16.0f, 36.0f}, 18, sf::Color::White);
    drawText("TIME " + std::to_string(static_cast<int>(world.survivalTime()))
        + "  SCORE " + std::to_string(world.score()),
        {16.0f, 60.0f}, 18, sf::Color::White);
    drawText("MAP " + std::to_string(world.mapLevel())
        + "  AREA " + mapAreaName(world.currentMapArea())
        + "  ENEMIES " + std::to_string(world.enemiesRemainingInWave()),
        {16.0f, 108.0f}, 16, sf::Color(210, 220, 255));
    drawText(world.mapModifier().description,
        {16.0f, 130.0f}, 14, sf::Color(255, 220, 150));
    const std::string bossLine = world.map().bossDefeated() ? "Boss defeated"
        : world.map().bossTriggered() ? "Boss active"
        : "Boss distance " + std::to_string(static_cast<int>(world.distanceToBoss()));
    drawText(bossLine,
        {16.0f, 174.0f}, 14, sf::Color(255, 190, 150));
    drawText("P Passive Tree  |  Spend SP on nodes",
        {16.0f, 152.0f}, 14, sf::Color(210, 255, 210));
    const auto& stats = world.player().stats();
    drawText("DMG +" + std::to_string(multiplierPercent(stats.damageMultiplier))
        + "%  AS +" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier))
        + "%  MS +" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier)) + "%",
        {16.0f, 84.0f}, 16, sf::Color(210, 220, 255));
    drawSkillBar(world);
    drawEquipment(world);
    drawInventory(world);
    drawPassiveTree(world);

    switch (world.state()) {
        case GameState::GameOver:
            drawGameOver(world);
            break;
        case GameState::MapComplete:
            drawMapComplete(world);
            break;
        case GameState::Playing:
            break;
    }

    window_.display();
}

void Renderer::drawMap(const GameWorld& world) {
    const Vector2 camera = world.cameraTopLeft();
    const auto& map = world.map();

    sf::RectangleShape floor({map.size().x, map.size().y});
    floor.setFillColor(sf::Color(24, 28, 30));
    floor.setOutlineColor(sf::Color(80, 90, 96));
    floor.setOutlineThickness(6.0f);
    floor.setPosition({-camera.x, -camera.y});
    window_.draw(floor);

    const sf::Vector2f bossCenter = worldToScreen(world, map.bossCenter());
    sf::CircleShape gate(Config::BossGateRadius);
    gate.setFillColor(sf::Color(120, 70, 40, 35));
    gate.setOutlineColor(sf::Color(210, 150, 90, 130));
    gate.setOutlineThickness(3.0f);
    gate.setOrigin({Config::BossGateRadius, Config::BossGateRadius});
    gate.setPosition(bossCenter);
    window_.draw(gate);

    sf::CircleShape arena(Config::BossArenaRadius);
    arena.setFillColor(sf::Color(120, 35, 35, 45));
    arena.setOutlineColor(sf::Color(240, 90, 80, 160));
    arena.setOutlineThickness(4.0f);
    arena.setOrigin({Config::BossArenaRadius, Config::BossArenaRadius});
    arena.setPosition(bossCenter);
    window_.draw(arena);

    const sf::Vector2f startCenter = worldToScreen(world, map.playerStart());
    sf::CircleShape start(Config::StartSafeRadius);
    start.setFillColor(sf::Color(40, 110, 70, 45));
    start.setOutlineColor(sf::Color(90, 210, 130, 120));
    start.setOutlineThickness(3.0f);
    start.setOrigin({Config::StartSafeRadius, Config::StartSafeRadius});
    start.setPosition(startCenter);
    window_.draw(start);
}

void Renderer::drawPlayer(const GameWorld& world) {
    const auto& player = world.player();
    sf::CircleShape shape(player.radius());
    shape.setFillColor(sf::Color::Green);
    shape.setOrigin({player.radius(), player.radius()});
    shape.setPosition(worldToScreen(world, player.position()));
    window_.draw(shape);
}

void Renderer::drawNovaEffect(const GameWorld& world) {
    const float progress = world.novaEffectProgress();
    if (progress <= 0.0f) {
        return;
    }

    const auto& player = world.player();
    const float radius = Config::NovaRadius * (1.0f - progress * 0.25f);
    const auto alpha = static_cast<std::uint8_t>(180.0f * progress);

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(80, 180, 255, alpha / 4));
    shape.setOutlineColor(sf::Color(120, 220, 255, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, player.position()));
    window_.draw(shape);
}

void Renderer::drawSecondarySkillEffect(const GameWorld& world) {
    const float progress = world.secondarySkillEffectProgress();
    if (progress <= 0.0f) {
        return;
    }

    const auto& center = world.secondarySkillEffectPosition();
    const float radius = Config::SecondarySkillRadius * (1.0f - progress * 0.20f);
    const auto alpha = static_cast<std::uint8_t>(170.0f * progress);

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 180, 80, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 210, 120, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, center));
    window_.draw(shape);
}

void Renderer::drawAimIndicator(const GameWorld& world) {
    const auto& player = world.player();
    const auto& aim = world.aimPosition();

    sf::VertexArray line(sf::PrimitiveType::Lines, 2);
    line[0].position = worldToScreen(world, player.position());
    line[0].color = sf::Color(120, 220, 255, 160);
    line[1].position = worldToScreen(world, aim);
    line[1].color = sf::Color(120, 220, 255, 80);
    window_.draw(line);

    constexpr float reticleSize = 8.0f;
    const sf::Vector2f aimScreen = worldToScreen(world, aim);
    sf::VertexArray reticle(sf::PrimitiveType::Lines, 4);
    reticle[0].position = {aimScreen.x - reticleSize, aimScreen.y};
    reticle[1].position = {aimScreen.x + reticleSize, aimScreen.y};
    reticle[2].position = {aimScreen.x, aimScreen.y - reticleSize};
    reticle[3].position = {aimScreen.x, aimScreen.y + reticleSize};

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
        shape.setPosition(worldToScreen(world, projectile.position()));
        window_.draw(shape);
    }
}

void Renderer::drawEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies()) {
        sf::RectangleShape shape({enemy.radius() * 2, enemy.radius() * 2});
        shape.setFillColor(enemy.isBoss() ? sf::Color(255, 80, 40)
            : enemy.isElite() ? sf::Color(180, 60, 255)
            : sf::Color::Red);
        if (enemy.isBoss() || enemy.isElite()) {
            shape.setOutlineColor(sf::Color(255, 220, 120));
            shape.setOutlineThickness(enemy.isBoss() ? 5.0f : 3.0f);
        }
        shape.setOrigin({enemy.radius(), enemy.radius()});
        shape.setPosition(worldToScreen(world, enemy.position()));
        window_.draw(shape);
    }
}

void Renderer::drawDroppedItems(const GameWorld& world) {
    const auto& player = world.player();
    const float pickupRange = (Config::ItemPickupRange + player.radius())
        * player.stats().pickupRangeMultiplier;

    for (const auto& droppedItem : world.droppedItems()) {
        const auto& item = droppedItem.item();
        const Vector2 diff = player.position() - droppedItem.position();
        const bool canPickup = diff.lengthSquared() <= pickupRange * pickupRange;

        sf::RectangleShape shape({droppedItem.radius() * 2.0f, droppedItem.radius() * 2.0f});
        shape.setFillColor(rarityColor(item.rarity));
        if (canPickup) {
            shape.setOutlineColor(sf::Color::White);
            shape.setOutlineThickness(2.0f);
        }
        shape.setOrigin({droppedItem.radius(), droppedItem.radius()});
        shape.setPosition(worldToScreen(world, droppedItem.position()));
        window_.draw(shape);

        const std::string label = std::string(canPickup ? "F " : "")
            + item.name + " [" + slotName(item.slot) + "]";
        drawCenteredText(label,
            worldToScreen(world, Vector2(droppedItem.position().x, droppedItem.position().y - 20.0f)),
            12,
            rarityColor(item.rarity));
    }
}

void Renderer::drawSkillBar(const GameWorld& world) {
    const SkillSlot slots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };
    const char* keys[] = {"LMB", "RMB", "Q", "Space"};

    float x = 16.0f;
    const float y = static_cast<float>(Config::WindowHeight) - 34.0f;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = slots[i];
        const auto& skill = world.skillBar().definition(slot);
        const float progress = world.skillBar().cooldownProgress(slot);
        const sf::Color color = progress >= 1.0f ? sf::Color(130, 230, 150) : sf::Color(230, 180, 80);
        drawText(std::string(keys[i]) + " " + skill.name + " "
            + std::to_string(static_cast<int>(progress * 100.0f)) + "%",
            {x, y}, 13, color);
        x += 150.0f;
    }
}

void Renderer::drawEquipment(const GameWorld& world) {
    const auto& equipment = world.player().equipment();
    const float x = static_cast<float>(Config::WindowWidth) - 260.0f;
    float y = 12.0f;

    drawText("Equipped", {x, y}, 16, sf::Color::White);
    y += 22.0f;

    const EquipmentSlot slots[] = {
        EquipmentSlot::Weapon,
        EquipmentSlot::Armor,
        EquipmentSlot::Ring,
        EquipmentSlot::Amulet
    };

    for (const auto slot : slots) {
        const auto& item = equipment.itemInSlot(slot);
        const std::string line = std::string(slotName(slot)) + ": "
            + (item ? itemSummary(*item) : "Empty");
        drawText(line, {x, y}, 12, item ? rarityColor(item->rarity) : sf::Color(150, 150, 150));
        y += 17.0f;
    }
}

void Renderer::drawInventory(const GameWorld& world) {
    const auto& items = world.inventory().items();
    const auto& equipment = world.player().equipment();
    const float x = static_cast<float>(Config::WindowWidth) - 260.0f;
    float y = 118.0f;

    drawText("Inventory", {x, y}, 16, sf::Color::White);
    y += 22.0f;

    const std::size_t visibleCount = std::min<std::size_t>(items.size(), 9);
    for (std::size_t i = 0; i < visibleCount; ++i) {
        const auto& item = items[i];
        const std::string line = std::to_string(i + 1) + ". "
            + item.name + " [" + slotName(item.slot) + "] " + statsSummary(item.stats);
        drawText(line, {x, y}, 13, rarityColor(item.rarity));
        y += 16.0f;

        const auto& current = equipment.itemInSlot(item.slot);
        if (current) {
            const Stats delta = statsDelta(item.stats, current->stats);
            drawText("   Delta: " + statsDeltaSummary(delta), {x, y}, 11, deltaColor(delta));
            y += 14.0f;
        }
    }
}

void Renderer::drawPassiveTree(const GameWorld& world) {
    if (!world.passiveTreeOpen()) {
        return;
    }

    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 145));
    window_.draw(overlay);

    drawBox({center.x, center.y}, {520.0f, 300.0f}, sf::Color(30, 38, 48));
    drawCenteredText("Passive Tree", {center.x, center.y - 126.0f}, 24, sf::Color::White);
    drawCenteredText("SP " + std::to_string(world.player().talentPoints()) + "  |  1-5 allocate  |  P close",
        {center.x, center.y - 98.0f}, 14, sf::Color(210, 230, 255));

    const auto& nodes = world.player().passiveTree().nodes();
    float y = center.y - 64.0f;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const bool prerequisiteMet = node.prerequisite < 0
            || nodes[static_cast<std::size_t>(node.prerequisite)].allocated;
        const bool available = !node.allocated && prerequisiteMet && world.player().talentPoints() > 0;

        sf::Color color = sf::Color(150, 150, 150);
        std::string status = "Locked";
        if (node.allocated) {
            color = sf::Color(120, 230, 140);
            status = "Allocated";
        } else if (available) {
            color = sf::Color(220, 240, 255);
            status = "Available";
        } else if (prerequisiteMet) {
            color = sf::Color(190, 190, 190);
            status = "No SP";
        }

        drawText(std::to_string(i + 1) + ". " + node.name + " - " + node.description,
            {center.x - 220.0f, y}, 15, color);
        drawText("[" + status + "]",
            {center.x + 150.0f, y}, 15, color);
        y += 34.0f;
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

void Renderer::drawMapComplete(const GameWorld& world) {
    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 100, 0, 180));
    window_.draw(overlay);

    drawBox({center.x, center.y - 84.0f}, {360.0f, 90.0f}, sf::Color::Green);
    drawCenteredText("BOSS DEFEATED", {center.x, center.y - 110.0f}, 28, sf::Color::White);
    drawCenteredText("Kills " + std::to_string(world.mapKills())
        + "  XP " + std::to_string(world.mapExperienceGained()),
        {center.x, center.y - 76.0f}, 16, sf::Color::White);
    drawCenteredText("Drops " + std::to_string(world.mapItemsDropped())
        + "  Picked " + std::to_string(world.mapItemsPickedUp()),
        {center.x, center.y - 52.0f}, 16, sf::Color::White);

    drawCenteredText("Choose Reward", {center.x, center.y - 12.0f}, 18, sf::Color::White);
    drawText("1. +20% Damage", {center.x - 150.0f, center.y + 14.0f}, 15, sf::Color(220, 240, 255));
    drawText("2. +1 Max HP", {center.x - 150.0f, center.y + 36.0f}, 15, sf::Color(220, 240, 255));
    drawText("3. +25% Future Drops", {center.x - 150.0f, center.y + 58.0f}, 15, sf::Color(220, 240, 255));

    drawBox({center.x, center.y + 104.0f}, {320.0f, 40.0f}, sf::Color::White);
    if (world.mapRewardChosen()) {
        drawCenteredText("E Enter Map " + std::to_string(world.mapLevel() + 1) + " / R Restart",
            {center.x, center.y + 97.0f}, 17, sf::Color::Black);
    } else {
        drawCenteredText("Pick 1 / 2 / 3 reward first",
            {center.x, center.y + 97.0f}, 17, sf::Color::Black);
    }
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

sf::Vector2f Renderer::worldToScreen(const GameWorld& world, const Vector2& position) const {
    const Vector2 camera = world.cameraTopLeft();
    return {position.x - camera.x, position.y - camera.y};
}
