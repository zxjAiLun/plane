#include "Renderer.hpp"
#include "Config.hpp"
#include "EnemyDefinition.hpp"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace {
sf::Color rarityColor(Rarity rarity) {
    switch (rarity) {
        case Rarity::Normal: return sf::Color(220, 220, 220);
        case Rarity::Magic: return sf::Color(90, 150, 255);
        case Rarity::Rare: return sf::Color(255, 210, 80);
    }
    return sf::Color::White;
}

sf::Color enemyColor(const EnemyColor& color) {
    return sf::Color(color.r, color.g, color.b);
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
    if (stats.projectileDamageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.projectileDamageMultiplier)) + "% PDMG ";
    }
    if (stats.areaDamageMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.areaDamageMultiplier)) + "% ADMG ";
    }
    if (stats.areaRadiusMultiplier > 1.0f) {
        summary += "+" + std::to_string(multiplierPercent(stats.areaRadiusMultiplier)) + "% AREA ";
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
        next.projectileDamageMultiplier / current.projectileDamageMultiplier,
        next.areaDamageMultiplier / current.areaDamageMultiplier,
        next.areaRadiusMultiplier / current.areaRadiusMultiplier,
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
    if (delta.projectileDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.projectileDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% PDMG ";
    }
    if (delta.areaDamageMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.areaDamageMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% ADMG ";
    }
    if (delta.areaRadiusMultiplier != 1.0f) {
        const int value = multiplierPercent(delta.areaRadiusMultiplier);
        summary += (value > 0 ? "+" : "") + std::to_string(value) + "% AREA ";
    }
    return summary.empty() ? "No stat change" : summary;
}

sf::Color deltaColor(const Stats& delta) {
    const bool positive = delta.maxHp > 0
        || delta.damageMultiplier > 1.0f
        || delta.attackSpeedMultiplier > 1.0f
        || delta.moveSpeedMultiplier > 1.0f
        || delta.pickupRangeMultiplier > 1.0f
        || delta.projectileDamageMultiplier > 1.0f
        || delta.areaDamageMultiplier > 1.0f
        || delta.areaRadiusMultiplier > 1.0f;
    const bool negative = delta.maxHp < 0
        || delta.damageMultiplier < 1.0f
        || delta.attackSpeedMultiplier < 1.0f
        || delta.moveSpeedMultiplier < 1.0f
        || delta.pickupRangeMultiplier < 1.0f
        || delta.projectileDamageMultiplier < 1.0f
        || delta.areaDamageMultiplier < 1.0f
        || delta.areaRadiusMultiplier < 1.0f;

    if (positive && !negative) {
        return sf::Color(120, 230, 140);
    }
    if (negative && !positive) {
        return sf::Color(240, 120, 120);
    }
    return sf::Color(230, 220, 150);
}

sf::Color passiveBranchColor(PassiveBranch branch) {
    switch (branch) {
        case PassiveBranch::Projectile: return sf::Color(110, 185, 255);
        case PassiveBranch::Area: return sf::Color(255, 150, 85);
        case PassiveBranch::Survival: return sf::Color(120, 235, 145);
        case PassiveBranch::Loot: return sf::Color(245, 215, 90);
    }
    return sf::Color::White;
}

std::string passiveBranchName(PassiveBranch branch) {
    switch (branch) {
        case PassiveBranch::Projectile: return "Projectile";
        case PassiveBranch::Area: return "Area";
        case PassiveBranch::Survival: return "Survival";
        case PassiveBranch::Loot: return "Loot";
    }
    return "Unknown";
}

std::string passiveKeyLabel(std::size_t index) {
    if (index < 9) {
        return std::to_string(index + 1);
    }
    if (index == 9) {
        return "0";
    }
    return "F" + std::to_string(index - 9);
}

std::string skillSlotName(SkillSlot slot) {
    switch (slot) {
        case SkillSlot::Primary: return "Primary";
        case SkillSlot::Secondary: return "Secondary";
        case SkillSlot::Utility: return "Utility";
        case SkillSlot::Movement: return "Movement";
        case SkillSlot::Count: break;
    }
    return "Unknown";
}

std::string skillCastTypeName(SkillCastType type) {
    switch (type) {
        case SkillCastType::Projectile: return "Projectile";
        case SkillCastType::SelfCenteredArea: return "Self Area";
        case SkillCastType::MouseTargetedArea: return "Mouse Area";
        case SkillCastType::Dash: return "Dash";
    }
    return "Unknown";
}

std::string formatFloat(float value, int precision = 1) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << value;
    return stream.str();
}

int effectiveSkillDamage(const SkillDefinition& skill, const Stats& stats) {
    if (skill.baseDamage <= 0) {
        return 0;
    }

    float damage = static_cast<float>(skill.baseDamage) * stats.damageMultiplier;
    switch (skill.castType) {
        case SkillCastType::Projectile:
            damage *= stats.projectileDamageMultiplier;
            break;
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            damage *= stats.areaDamageMultiplier;
            break;
        case SkillCastType::Dash:
            break;
    }

    return std::max(1, static_cast<int>(std::ceil(damage)));
}

float effectiveSkillRadius(const SkillDefinition& skill, const Stats& stats) {
    switch (skill.castType) {
        case SkillCastType::SelfCenteredArea:
        case SkillCastType::MouseTargetedArea:
            return skill.radius * stats.areaRadiusMultiplier;
        case SkillCastType::Projectile:
        case SkillCastType::Dash:
            return skill.radius;
    }

    return skill.radius;
}

float effectiveSkillCooldown(const SkillDefinition& skill, const Stats& stats) {
    if (skill.slot == SkillSlot::Primary) {
        return skill.cooldown / stats.attackSpeedMultiplier;
    }

    return skill.cooldown;
}

std::string skillEffectiveSummary(const SkillDefinition& skill, const Stats& stats) {
    return "Base " + std::to_string(skill.baseDamage)
        + "/" + std::to_string(static_cast<int>(skill.radius))
        + "/" + formatFloat(skill.cooldown, 2)
        + "  Actual " + std::to_string(effectiveSkillDamage(skill, stats))
        + "/" + std::to_string(static_cast<int>(effectiveSkillRadius(skill, stats)))
        + "/" + formatFloat(effectiveSkillCooldown(skill, stats), 2);
}

std::string rewardDetailSummary(const MapRewardDefinition& reward, const GameWorld& world) {
    if (reward.type != MapRewardType::UnlockSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    const auto& current = world.skillBar().definition(skill->slot);
    return skillSlotName(skill->slot) + " / " + skillCastTypeName(skill->castType)
        + "  |  Replaces " + current.name;
}

std::string rewardStatPreview(const MapRewardDefinition& reward, const GameWorld& world) {
    if (reward.type != MapRewardType::UnlockSkill) {
        return reward.description;
    }

    const auto* skill = SkillLibrary::find(reward.skillName);
    if (!skill) {
        return reward.description;
    }

    return skillEffectiveSummary(*skill, world.player().stats());
}

std::string mapOptionSummary(const MapOption& option) {
    const auto& modifier = option.modifier;
    return "HP +" + std::to_string(multiplierPercent(modifier.monsterHpMultiplier))
        + "%  DMG +" + std::to_string(modifier.monsterDamageBonus)
        + "  IQ +" + std::to_string(multiplierPercent(modifier.itemQuantityMultiplier))
        + "%  Boss +" + std::to_string(modifier.bossDropBonus);
}

sf::Color mapEventColor(MapEventType type, bool completed) {
    if (completed) {
        return sf::Color(130, 130, 130);
    }

    switch (type) {
        case MapEventType::LootCache: return sf::Color(255, 215, 70);
        case MapEventType::ElitePack: return sf::Color(190, 90, 255);
        case MapEventType::Shrine: return sf::Color(80, 230, 230);
    }

    return sf::Color::White;
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
    drawBossAoeEffect(world);
    drawPlayer(world);
    drawAimIndicator(world);
    drawProjectiles(world);
    drawBossProjectiles(world);
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
    drawText("MAP " + std::to_string(world.mapLevel()) + "  " + world.currentMapOption().modifier.name
        + "  AREA " + mapAreaName(world.currentMapArea())
        + "  ENEMIES " + std::to_string(world.enemiesRemainingInWave()),
        {16.0f, 108.0f}, 16, sf::Color(210, 220, 255));
    drawText(world.mapModifier().description + "  |  " + mapOptionSummary(world.currentMapOption()),
        {16.0f, 130.0f}, 14, sf::Color(255, 220, 150));
    const std::string bossLine = world.map().bossDefeated()
        ? "Boss defeated: " + world.bossDefinition().name
        : world.map().bossTriggered()
            ? "Boss active: " + world.bossDefinition().name
            : world.bossDefinition().name + " distance " + std::to_string(static_cast<int>(world.distanceToBoss()));
    drawText(bossLine,
        {16.0f, 174.0f}, 14, sf::Color(255, 190, 150));
    drawText("Objective: " + world.mapObjective(),
        {16.0f, 196.0f}, 14, sf::Color(210, 255, 210));
    if (!world.nearbyEventPrompt().empty()) {
        drawText(world.nearbyEventPrompt(), {16.0f, 218.0f}, 14, sf::Color(255, 235, 150));
    }
    if (world.shrineBuffTimeRemaining() > 0.0f) {
        drawText("Shrine +35% damage  "
            + std::to_string(static_cast<int>(world.shrineBuffTimeRemaining() + 0.99f)) + "s",
            {16.0f, 236.0f}, 14, sf::Color(100, 240, 240));
    }
    drawText("Build: " + world.passiveBuildSummary() + "  |  P Passive Tree  |  K Skills",
        {16.0f, 152.0f}, 14, sf::Color(210, 255, 210));
    const auto& stats = world.player().stats();
    drawText("DMG +" + std::to_string(multiplierPercent(stats.damageMultiplier))
        + "%  AS +" + std::to_string(multiplierPercent(stats.attackSpeedMultiplier))
        + "%  MS +" + std::to_string(multiplierPercent(stats.moveSpeedMultiplier))
        + "%  PDMG +" + std::to_string(multiplierPercent(stats.projectileDamageMultiplier))
        + "%  ADMG +" + std::to_string(multiplierPercent(stats.areaDamageMultiplier))
        + "%  AREA +" + std::to_string(multiplierPercent(stats.areaRadiusMultiplier)) + "%",
        {16.0f, 84.0f}, 14, sf::Color(210, 220, 255));
    drawSkillBar(world);
    drawEquipment(world);
    drawInventory(world);
    drawMinimap(world);
    drawBossHealth(world);
    drawPassiveTree(world);
    drawSkillPanel(world);

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
    const float baseRadius = world.novaEffectRadius();
    const float radius = baseRadius * (1.0f - progress * 0.25f);
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
    const float baseRadius = world.secondarySkillEffectRadius();
    const float radius = baseRadius * (1.0f - progress * 0.20f);
    const auto alpha = static_cast<std::uint8_t>(170.0f * progress);

    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 180, 80, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 210, 120, alpha));
    shape.setOutlineThickness(3.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, center));
    window_.draw(shape);
}

void Renderer::drawBossAoeEffect(const GameWorld& world) {
    const float telegraphProgress = world.bossAoeTelegraphProgress();
    if (telegraphProgress > 0.0f) {
        const float radius = world.bossAoeRadius();
        const auto alpha = static_cast<std::uint8_t>(70.0f + 120.0f * (1.0f - telegraphProgress));
        sf::CircleShape shape(radius);
        shape.setFillColor(sf::Color(180, 30, 20, alpha / 4));
        shape.setOutlineColor(sf::Color(255, 90, 60, alpha));
        shape.setOutlineThickness(4.0f);
        shape.setOrigin({radius, radius});
        shape.setPosition(worldToScreen(world, world.bossAoeCenter()));
        window_.draw(shape);
    }

    const float effectProgress = world.bossAoeEffectProgress();
    if (effectProgress <= 0.0f) {
        return;
    }

    const float radius = world.bossAoeRadius() * (1.0f - effectProgress * 0.15f);
    const auto alpha = static_cast<std::uint8_t>(190.0f * effectProgress);
    sf::CircleShape shape(radius);
    shape.setFillColor(sf::Color(255, 70, 35, alpha / 5));
    shape.setOutlineColor(sf::Color(255, 160, 70, alpha));
    shape.setOutlineThickness(5.0f);
    shape.setOrigin({radius, radius});
    shape.setPosition(worldToScreen(world, world.bossAoeCenter()));
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

void Renderer::drawBossProjectiles(const GameWorld& world) {
    for (const auto& projectile : world.bossProjectiles()) {
        sf::CircleShape shape(projectile.radius);
        shape.setFillColor(sf::Color(255, 90, 45));
        shape.setOutlineColor(sf::Color(255, 210, 120));
        shape.setOutlineThickness(2.0f);
        shape.setOrigin({projectile.radius, projectile.radius});
        shape.setPosition(worldToScreen(world, projectile.position));
        window_.draw(shape);
    }
}

void Renderer::drawEnemies(const GameWorld& world) {
    for (const auto& enemy : world.enemies()) {
        const auto& definition = EnemyLibrary::forType(enemy.type());
        sf::RectangleShape shape({enemy.radius() * 2, enemy.radius() * 2});
        shape.setFillColor(enemyColor(definition.fillColor));
        if (definition.outlineThickness > 0.0f) {
            shape.setOutlineColor(enemyColor(definition.outlineColor));
            shape.setOutlineThickness(definition.outlineThickness);
        }
        shape.setOrigin({enemy.radius(), enemy.radius()});
        const sf::Vector2f screenPosition = worldToScreen(world, enemy.position());
        shape.setPosition(screenPosition);
        window_.draw(shape);

        if (definition.outlineThickness > 0.0f) {
            const std::string label = enemy.isBoss() ? world.bossDefinition().name : definition.name;
            drawCenteredText(label, {screenPosition.x, screenPosition.y - enemy.radius() - 18.0f},
                11, enemyColor(definition.outlineColor));
        }
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

    drawBox({center.x, center.y}, {760.0f, 520.0f}, sf::Color(22, 28, 36));
    drawCenteredText("Passive Tree", {center.x, center.y - 238.0f}, 24, sf::Color::White);
    drawCenteredText("SP " + std::to_string(world.player().talentPoints())
        + "  |  Left click node  |  1-0/F1-F10 allocate  |  P close",
        {center.x, center.y - 210.0f}, 14, sf::Color(210, 230, 255));

    const auto& nodes = world.player().passiveTree().nodes();

    const auto nodeScreenPosition = [&](const PassiveNode& node) {
        return sf::Vector2f{
            center.x + node.treePosition.x,
            center.y + node.treePosition.y
        };
    };

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const sf::Vector2f from = node.prerequisite >= 0
            ? nodeScreenPosition(nodes[static_cast<std::size_t>(node.prerequisite)])
            : center;
        const sf::Vector2f to = nodeScreenPosition(node);
        sf::VertexArray line(sf::PrimitiveType::Lines, 2);
        const sf::Color branchColor = passiveBranchColor(node.branch);
        line[0].position = from;
        line[1].position = to;
        line[0].color = node.allocated ? branchColor : sf::Color(90, 100, 112);
        line[1].color = node.allocated ? branchColor : sf::Color(90, 100, 112);
        window_.draw(line);
    }

    sf::CircleShape origin(9.0f);
    origin.setOrigin({9.0f, 9.0f});
    origin.setPosition(center);
    origin.setFillColor(sf::Color(190, 200, 215));
    window_.draw(origin);

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        const auto& node = nodes[i];
        const bool prerequisiteMet = node.prerequisite < 0
            || nodes[static_cast<std::size_t>(node.prerequisite)].allocated;
        const bool available = !node.allocated && prerequisiteMet && world.player().talentPoints() > 0;
        const bool hovered = world.hoveredPassiveNode() == static_cast<int>(i);

        const sf::Color branchColor = passiveBranchColor(node.branch);
        sf::Color fill = sf::Color(54, 60, 70);
        sf::Color outline = sf::Color(120, 130, 145);
        if (node.allocated) {
            fill = branchColor;
            outline = sf::Color::White;
        } else if (available) {
            fill = sf::Color(branchColor.r / 3, branchColor.g / 3, branchColor.b / 3);
            outline = branchColor;
        } else if (prerequisiteMet) {
            outline = sf::Color(180, 180, 180);
        }

        if (hovered) {
            outline = sf::Color::White;
        }

        const float radius = node.size == PassiveNodeSize::Notable ? 16.0f : 12.0f;
        const sf::Vector2f position = nodeScreenPosition(node);
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        shape.setPosition(position);
        shape.setFillColor(fill);
        shape.setOutlineColor(outline);
        shape.setOutlineThickness(hovered ? 3.0f : 2.0f);
        window_.draw(shape);

        drawCenteredText(passiveKeyLabel(i), {position.x, position.y - 6.0f}, 10, sf::Color::White);
    }

    const int hoveredIndex = world.hoveredPassiveNode();
    if (hoveredIndex >= 0 && hoveredIndex < static_cast<int>(nodes.size())) {
        const auto& node = nodes[static_cast<std::size_t>(hoveredIndex)];
        const bool prerequisiteMet = node.prerequisite < 0
            || nodes[static_cast<std::size_t>(node.prerequisite)].allocated;
        std::string status = "Locked";
        sf::Color statusColor = sf::Color(170, 170, 170);
        if (node.allocated) {
            status = "Allocated";
            statusColor = sf::Color(130, 240, 150);
        } else if (prerequisiteMet && world.player().talentPoints() > 0) {
            status = "Available";
            statusColor = sf::Color(220, 240, 255);
        } else if (prerequisiteMet) {
            status = "Need SP";
            statusColor = sf::Color(230, 215, 150);
        }

        drawText(passiveBranchName(node.branch) + " / " + status,
            {center.x - 350.0f, center.y + 218.0f}, 13, statusColor);
        drawText(node.name + " - " + node.description,
            {center.x - 350.0f, center.y + 238.0f}, 14, passiveBranchColor(node.branch));
    } else {
        drawText("Hover a node to inspect it",
            {center.x - 350.0f, center.y + 232.0f}, 14, sf::Color(190, 200, 215));
    }
}

void Renderer::drawSkillPanel(const GameWorld& world) {
    if (!world.skillPanelOpen()) {
        return;
    }

    const float width = static_cast<float>(Config::WindowWidth);
    const float height = static_cast<float>(Config::WindowHeight);
    const sf::Vector2f center{width / 2.0f, height / 2.0f};

    sf::RectangleShape overlay({width, height});
    overlay.setFillColor(sf::Color(0, 0, 0, 145));
    window_.draw(overlay);

    drawBox({center.x, center.y}, {760.0f, 500.0f}, sf::Color(24, 30, 40));
    drawCenteredText("Skill Panel", {center.x, center.y - 228.0f}, 24, sf::Color::White);
    drawCenteredText("1-8 assign unlocked skill  |  K close",
        {center.x, center.y - 200.0f}, 14, sf::Color(210, 230, 255));

    const SkillSlot slots[] = {
        SkillSlot::Primary,
        SkillSlot::Secondary,
        SkillSlot::Utility,
        SkillSlot::Movement
    };

    float y = center.y - 164.0f;
    drawText("Equipped", {center.x - 350.0f, y}, 16, sf::Color::White);
    y += 22.0f;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto slot = slots[i];
        const auto& skill = world.skillBar().definition(slot);
        const float x = center.x - 350.0f + static_cast<float>(i % 2) * 370.0f;
        if (i == 2) {
            y += 20.0f;
        }
        drawText(skillSlotName(slot) + ": " + skill.name,
            {x, y}, 13, sf::Color(180, 230, 255));
    }

    y += 40.0f;
    drawText("Skills", {center.x - 350.0f, y}, 16, sf::Color::White);
    y += 24.0f;

    const auto& skills = SkillLibrary::all();
    for (std::size_t i = 0; i < skills.size(); ++i) {
        const auto& skill = skills[i];
        const bool equipped = world.skillBar().definition(skill.slot).name == skill.name;
        const bool unlocked = world.isSkillUnlocked(skill.name);
        const sf::Color color = !unlocked ? sf::Color(130, 135, 145)
            : equipped ? sf::Color(135, 245, 155)
            : sf::Color(220, 230, 240);
        const std::string state = equipped ? "Equipped" : unlocked ? "Available" : "Locked";
        const std::string marker = equipped ? "> " : "  ";
        const float columnX = center.x - 350.0f + static_cast<float>(i % 2) * 370.0f;
        const float rowY = y + static_cast<float>(i / 2) * 54.0f;
        drawText(marker + std::to_string(i + 1) + ". " + skill.name + " [" + state + "]",
            {columnX, rowY}, 14, color);
        drawText("     " + skillSlotName(skill.slot) + " / " + skillCastTypeName(skill.castType),
            {columnX, rowY + 17.0f}, 11,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
        drawText("     " + skillEffectiveSummary(skill, world.player().stats()),
            {columnX, rowY + 32.0f}, 10,
            unlocked ? sf::Color(190, 205, 220) : sf::Color(105, 112, 122));
    }
}

void Renderer::drawMinimap(const GameWorld& world) {
    const sf::Vector2f size{150.0f, 112.0f};
    const sf::Vector2f origin{
        static_cast<float>(Config::WindowWidth) - size.x - 18.0f,
        18.0f
    };
    const auto& map = world.map();
    const Vector2 mapSize = map.size();
    const float scaleX = size.x / mapSize.x;
    const float scaleY = size.y / mapSize.y;

    const auto toMinimap = [&](const Vector2& position) {
        return sf::Vector2f{
            origin.x + position.x * scaleX,
            origin.y + position.y * scaleY
        };
    };

    sf::RectangleShape background(size);
    background.setPosition(origin);
    background.setFillColor(sf::Color(10, 14, 18, 205));
    background.setOutlineColor(sf::Color(180, 190, 200));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

    auto drawMapCircle = [&](const Vector2& center, float worldRadius, sf::Color color, float outline = 1.0f) {
        const float radius = std::max(2.0f, worldRadius * std::min(scaleX, scaleY));
        sf::CircleShape shape(radius);
        shape.setOrigin({radius, radius});
        shape.setPosition(toMinimap(center));
        shape.setFillColor(sf::Color(color.r, color.g, color.b, 45));
        shape.setOutlineColor(color);
        shape.setOutlineThickness(outline);
        window_.draw(shape);
    };

    drawMapCircle(map.playerStart(), Config::StartSafeRadius, sf::Color(80, 210, 120));
    drawMapCircle(map.bossCenter(), Config::BossGateRadius, sf::Color(255, 190, 90));
    drawMapCircle(map.bossCenter(), Config::BossArenaRadius, sf::Color(255, 80, 60), 1.5f);

    for (const auto& event : map.events()) {
        sf::CircleShape eventDot(3.0f);
        eventDot.setOrigin({3.0f, 3.0f});
        eventDot.setPosition(toMinimap(event.position));
        eventDot.setFillColor(mapEventColor(event.type, event.completed));
        window_.draw(eventDot);
    }

    sf::CircleShape bossDot(3.5f);
    bossDot.setOrigin({3.5f, 3.5f});
    bossDot.setPosition(toMinimap(map.bossCenter()));
    bossDot.setFillColor(map.bossDefeated() ? sf::Color(120, 120, 120) : sf::Color(255, 80, 60));
    window_.draw(bossDot);

    sf::CircleShape playerDot(3.0f);
    playerDot.setOrigin({3.0f, 3.0f});
    playerDot.setPosition(toMinimap(world.player().position()));
    playerDot.setFillColor(sf::Color(90, 180, 255));
    window_.draw(playerDot);

    drawText("Map " + std::to_string(world.mapLevel())
        + "  " + std::to_string(static_cast<int>(map.progressToBoss(world.player().position()) * 100.0f)) + "%",
        {origin.x, origin.y + size.y + 5.0f}, 11, sf::Color(210, 220, 230));
}

void Renderer::drawBossHealth(const GameWorld& world) {
    const Enemy* boss = nullptr;
    for (const auto& enemy : world.enemies()) {
        if (enemy.isBoss() && !enemy.isDead()) {
            boss = &enemy;
            break;
        }
    }

    if (!boss || boss->maxHp() <= 0) {
        return;
    }

    const sf::Vector2f position{16.0f, 258.0f};
    const sf::Vector2f size{260.0f, 12.0f};
    const float ratio = std::clamp(
        static_cast<float>(std::max(0, boss->hp())) / static_cast<float>(boss->maxHp()),
        0.0f,
        1.0f
    );

    drawText(world.bossDefinition().name + "  "
        + std::to_string(std::max(0, boss->hp())) + "/" + std::to_string(boss->maxHp()),
        {position.x, position.y - 18.0f}, 13, sf::Color(255, 210, 160));

    sf::RectangleShape background(size);
    background.setPosition(position);
    background.setFillColor(sf::Color(60, 30, 28, 210));
    background.setOutlineColor(sf::Color(255, 210, 160));
    background.setOutlineThickness(1.0f);
    window_.draw(background);

    sf::RectangleShape fill({size.x * ratio, size.y});
    fill.setPosition(position);
    fill.setFillColor(sf::Color(220, 55, 45));
    window_.draw(fill);
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
    drawCenteredText("BOSS DEFEATED: " + world.bossDefinition().name,
        {center.x, center.y - 110.0f}, 22, sf::Color::White);
    drawCenteredText("Kills " + std::to_string(world.mapKills())
        + "  XP " + std::to_string(world.mapExperienceGained()),
        {center.x, center.y - 76.0f}, 16, sf::Color::White);
    drawCenteredText("Drops " + std::to_string(world.mapItemsDropped())
        + "  Picked " + std::to_string(world.mapItemsPickedUp()),
        {center.x, center.y - 52.0f}, 16, sf::Color::White);
    drawCenteredText("Events " + std::to_string(world.mapEventsCompleted())
        + "/" + std::to_string(world.mapEventsTotal()),
        {center.x, center.y - 30.0f}, 16, sf::Color::White);

    const auto& mapOptions = world.nextMapOptions();
    if (!world.mapRewardChosen()) {
        drawCenteredText("Choose Reward", {center.x, center.y - 2.0f}, 18, sf::Color::White);
        const auto& rewards = world.mapRewardOptions();
        float optionY = center.y + 26.0f;
        for (std::size_t i = 0; i < rewards.size(); ++i) {
            const auto& reward = rewards[i];
            drawText(std::to_string(i + 1) + ". " + reward.title,
                {center.x - 235.0f, optionY}, 15, sf::Color(220, 245, 255));
            drawText("     " + rewardDetailSummary(reward, world),
                {center.x - 235.0f, optionY + 19.0f}, 12, sf::Color(230, 220, 170));
            drawText("     " + rewardStatPreview(reward, world),
                {center.x - 235.0f, optionY + 34.0f}, 11, sf::Color(200, 220, 245));
            optionY += 58.0f;
        }
    } else {
        const auto selectedReward = static_cast<std::size_t>(world.selectedMapRewardOption());
        const auto& reward = world.mapRewardOptions()[selectedReward];
        drawCenteredText("Reward: " + reward.title,
            {center.x, center.y - 4.0f}, 16, sf::Color(150, 255, 175));
        drawCenteredText("Choose Next Map", {center.x, center.y + 22.0f}, 18, sf::Color::White);
        float optionY = center.y + 48.0f;
        for (std::size_t i = 0; i < mapOptions.size(); ++i) {
            const bool selected = world.selectedNextMapOption() == static_cast<int>(i);
            const auto& option = mapOptions[i];
            const sf::Color color = selected ? sf::Color(140, 255, 160) : sf::Color(220, 240, 255);
            const std::string marker = selected ? "> " : "  ";
            drawText(marker + std::to_string(i + 1) + ". " + option.modifier.name + " - " + option.recommendedLevel,
                {center.x - 235.0f, optionY}, 14, color);
            drawText("     " + option.modifier.description,
                {center.x - 235.0f, optionY + 17.0f}, 12, sf::Color(230, 220, 170));
            drawText("     " + mapOptionSummary(option) + "  |  " + option.rewardDescription,
                {center.x - 235.0f, optionY + 32.0f}, 12, sf::Color(200, 220, 245));
            optionY += 50.0f;
        }
    }

    drawBox({center.x, center.y + 198.0f}, {360.0f, 40.0f}, sf::Color::White);
    if (!world.mapRewardChosen()) {
        drawCenteredText("Pick 1 / 2 / 3 reward first",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
    } else if (world.nextMapOptionChosen()) {
        const auto& selected = mapOptions[static_cast<std::size_t>(world.selectedNextMapOption())];
        drawCenteredText("E Enter " + selected.modifier.name + " / R Restart",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
    } else {
        drawCenteredText("Pick 1 / 2 / 3 next map first",
            {center.x, center.y + 191.0f}, 16, sf::Color::Black);
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
