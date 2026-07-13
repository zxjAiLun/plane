#pragma once

#include <SFML/Graphics.hpp>
#include <optional>
#include <string>
#include "GameWorld.hpp"
#include "Item.hpp"

class Renderer {
public:
    explicit Renderer(sf::RenderWindow& window);

    void render(const GameWorld& world);

private:
    void drawMap(const GameWorld& world);
    void drawGroundHazards(const GameWorld& world);
    void drawPlayer(const GameWorld& world);
    void drawNovaEffect(const GameWorld& world);
    void drawSecondarySkillEffect(const GameWorld& world);
    void drawDashImpactEffect(const GameWorld& world);
    void drawBossAoeEffect(const GameWorld& world);
    void drawBossDashEffect(const GameWorld& world);
    void drawVolatileExplosionEffect(const GameWorld& world);
    void drawAimIndicator(const GameWorld& world);
    void drawProjectiles(const GameWorld& world);
    void drawBossProjectiles(const GameWorld& world);
    void drawEnemyProjectiles(const GameWorld& world);
    void drawEnemies(const GameWorld& world);
    void drawCombatFeedback(const GameWorld& world);
    void drawDroppedItems(const GameWorld& world);
    void drawSkillBar(const GameWorld& world);
    void drawEquipment(const GameWorld& world);
    void drawInventory(const GameWorld& world);
    void drawCraftingPanel(const GameWorld& world);
    void drawItemDetailPanel(const GameWorld& world,
        const sf::Vector2f& panelPos,
        const Item& item,
        const std::optional<Item>& current,
        const std::string& statusLabel,
        const std::string& actionHint,
        bool compact = false);
    void drawPassiveTree(const GameWorld& world);
    void drawSkillPanel(const GameWorld& world);
    void drawMinimap(const GameWorld& world);
    void drawBossHealth(const GameWorld& world);
    void drawGameOver(const GameWorld& world);
    void drawPause(const GameWorld& world);
    void drawMapComplete(const GameWorld& world);
    void drawMapCompleteInventoryPanel(const GameWorld& world);
    void drawMapCompleteStashPanel(const GameWorld& world);
    void drawMapCompleteLootDetail(const GameWorld& world);

    void drawBox(const sf::Vector2f& center, const sf::Vector2f& size, const sf::Color& color);
    void drawText(const std::string& text, const sf::Vector2f& position, unsigned int size, const sf::Color& color);
    void drawCenteredText(const std::string& text, const sf::Vector2f& center, unsigned int size, const sf::Color& color);
    sf::Vector2f worldToScreen(const GameWorld& world, const Vector2& position) const;

private:
    sf::RenderWindow& window_;
    sf::Font font_;
    bool fontLoaded_;
};
