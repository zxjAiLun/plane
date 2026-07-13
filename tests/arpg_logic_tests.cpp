// Unit tests for shipped pure ARPG logic (no SFML).
// Each assertion drives real headers/types used by the game binary.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <utility>

#include "BossDash.hpp"
#include "BossDefinition.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"
#include "Crafting.hpp"
#include "EliteModifier.hpp"
#include "Enemy.hpp"
#include "EnemyDefinition.hpp"
#include "Equipment.hpp"
#include "GroundHazard.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "LootGenerator.hpp"
#include "MapModifier.hpp"
#include "MapInstance.hpp"
#include "MapRewardLibrary.hpp"
#include "PassiveTree.hpp"
#include "Player.hpp"
#include "SkillBar.hpp"
#include "SkillLibrary.hpp"
#include "Stash.hpp"
#include "Stats.hpp"
#include "SupportLibrary.hpp"

namespace {

int g_failures = 0;
int g_passed = 0;

void expect(bool condition, const std::string& label) {
    if (condition) {
        ++g_passed;
        std::cout << "  PASS  " << label << '\n';
    } else {
        ++g_failures;
        std::cout << "  FAIL  " << label << '\n';
    }
}

bool statsEqual(const Stats& lhs, const Stats& rhs) {
    return lhs.maxHp == rhs.maxHp
        && std::abs(lhs.moveSpeedMultiplier - rhs.moveSpeedMultiplier) < 0.0001f
        && std::abs(lhs.damageMultiplier - rhs.damageMultiplier) < 0.0001f
        && std::abs(lhs.attackSpeedMultiplier - rhs.attackSpeedMultiplier) < 0.0001f
        && std::abs(lhs.pickupRangeMultiplier - rhs.pickupRangeMultiplier) < 0.0001f
        && std::abs(lhs.projectileDamageMultiplier - rhs.projectileDamageMultiplier) < 0.0001f
        && std::abs(lhs.areaDamageMultiplier - rhs.areaDamageMultiplier) < 0.0001f
        && std::abs(lhs.areaRadiusMultiplier - rhs.areaRadiusMultiplier) < 0.0001f
        && lhs.armor == rhs.armor
        && lhs.projectileCountBonus == rhs.projectileCountBonus
        && std::abs(lhs.lifeFlaskEffectMultiplier - rhs.lifeFlaskEffectMultiplier) < 0.0001f
        && std::abs(lhs.itemQuantityMultiplier - rhs.itemQuantityMultiplier) < 0.0001f
        && std::abs(lhs.incomingDamageMultiplier - rhs.incomingDamageMultiplier) < 0.0001f;
}

void section(const std::string& title) {
    std::cout << "\n== " << title << " ==\n";
}

// --- Passive tree ---
void testPassiveTreePrerequisitesAndStats() {
    section("PassiveTree allocate + combinedStats");

    PassiveTree tree;
    const Stats baseline = tree.combinedStats();
    expect(baseline.maxHp == 0, "empty tree: maxHp == 0");
    expect(std::abs(baseline.damageMultiplier - 1.0f) < 0.0001f, "empty tree: damageMultiplier == 1");

    // Node 1 (Rapid Fire) requires node 0 (Sharpened Bolt).
    expect(!tree.allocate(1), "reject allocate without prerequisite");
    expect(!tree.nodes()[1].allocated, "node 1 still unallocated after reject");

    expect(tree.allocate(0), "allocate root node 0 (Sharpened Bolt)");
    expect(tree.nodes()[0].allocated, "node 0 marked allocated");

    const Stats afterRoot = tree.combinedStats();
    expect(afterRoot.projectileDamageMultiplier > baseline.projectileDamageMultiplier,
        "combinedStats rises after allocating projectile damage node");
    expect(std::abs(afterRoot.projectileDamageMultiplier - 1.08f) < 0.0001f,
        "node 0 grants exactly 1.08 projectileDamageMultiplier");

    expect(tree.allocate(1), "allocate node 1 after prerequisite");
    expect(tree.nodes()[1].allocated, "node 1 allocated");
    expect(!tree.allocate(1), "reject re-allocate of same node");

    const Stats afterSecond = tree.combinedStats();
    expect(afterSecond.attackSpeedMultiplier > afterRoot.attackSpeedMultiplier,
        "attack speed rises after Rapid Fire");
}

// --- Passive keystones ---
void allocateBranchEndpoint(PassiveTree& tree, std::size_t rootIndex) {
    for (std::size_t index = rootIndex; index < rootIndex + 4; ++index) {
        expect(tree.allocate(index), "allocate Keystone prerequisite node " + std::to_string(index));
    }
}

void testPassiveKeystones() {
    section("Passive Tree Keystone tradeoffs");

    PassiveTree empty;
    const Stats emptyStats = empty.combinedStats();
    expect(!empty.hasKeystone(PassiveKeystone::VolleyDoctrine),
        "unallocated tree has no Projectile Keystone");
    expect(emptyStats.projectileCountBonus == 0
            && emptyStats.lifeFlaskEffectMultiplier == 1.0f
            && emptyStats.itemQuantityMultiplier == 1.0f
            && emptyStats.incomingDamageMultiplier == 1.0f,
        "unallocated tree keeps Keystone modifiers at neutral values");

    PassiveTree projectile;
    allocateBranchEndpoint(projectile, 0);
    expect(projectile.allocate(4), "allocate Volley Doctrine endpoint");
    expect(projectile.nodes()[4].keystone == PassiveKeystone::VolleyDoctrine,
        "Projectile endpoint uses Volley Doctrine data");
    expect(projectile.hasKeystone(PassiveKeystone::VolleyDoctrine),
        "allocated Projectile endpoint reports its Keystone");
    const Stats projectileStats = projectile.combinedStats();
    expect(projectileStats.projectileCountBonus == 2,
        "Volley Doctrine adds two projectiles");
    expect(projectileStats.projectileDamageMultiplier < 1.0f,
        "Volley Doctrine applies its projectile damage penalty");
    expect(!projectile.allocate(4), "allocated Projectile Keystone cannot be allocated twice");

    const auto spread = SkillLibrary::spreadShot();
    auto projectileDamageProbe = spread;
    projectileDamageProbe.baseDamage = 10;
    const auto volley = SupportLibrary::find("Volley");
    expect(skillProjectileCount(spread, nullptr, projectileStats) == spread.projectileCount + 2,
        "Volley Doctrine adds projectiles to the base skill");
    expect(skillProjectileCount(spread, volley, projectileStats)
            == spread.projectileCount + 2 + volley->extraProjectileCount,
        "Volley Doctrine stacks with Volley Support");
    expect(skillDamage(projectileDamageProbe, projectileStats, nullptr)
            < skillDamage(projectileDamageProbe, Stats{}, nullptr),
        "Volley Doctrine lowers projectile damage");
    expect(skillDamage(SkillLibrary::meteor(), projectileStats, nullptr)
            == skillDamage(SkillLibrary::meteor(), Stats{}, nullptr),
        "Volley Doctrine does not change area skill damage");

    PassiveTree area;
    allocateBranchEndpoint(area, 5);
    expect(area.allocate(9), "allocate Concentrated Impact endpoint");
    expect(area.nodes()[9].keystone == PassiveKeystone::ConcentratedImpact,
        "Area endpoint uses Concentrated Impact data");
    const Stats areaStats = area.combinedStats();
    expect(areaStats.areaDamageMultiplier > 1.0f,
        "Concentrated Impact increases area damage");
    expect(areaStats.areaRadiusMultiplier < 1.0f,
        "Concentrated Impact reduces area radius");
    expect(skillRadius(SkillLibrary::meteor(), areaStats, nullptr)
            < skillRadius(SkillLibrary::meteor(), Stats{}, nullptr),
        "Concentrated Impact reduces actual area radius");
    expect(skillDamage(SkillLibrary::meteor(), areaStats, nullptr)
            > skillDamage(SkillLibrary::meteor(), Stats{}, nullptr),
        "Concentrated Impact increases actual area damage");
    expect(skillDamage(spread, areaStats, nullptr) == skillDamage(spread, Stats{}, nullptr),
        "Concentrated Impact does not change projectile damage");

    PassiveTree survival;
    allocateBranchEndpoint(survival, 10);
    expect(survival.allocate(14), "allocate Second Wind endpoint");
    expect(survival.nodes()[14].keystone == PassiveKeystone::SecondWind,
        "Survival endpoint uses Second Wind data");
    const Stats survivalStats = survival.combinedStats();
    expect(std::abs(survivalStats.lifeFlaskEffectMultiplier - 1.5f) < 0.0001f,
        "Second Wind increases life flask healing by 50 percent");
    expect(lifeFlaskHealAmount(Config::LifeFlaskHealAmount, survivalStats)
            > lifeFlaskHealAmount(Config::LifeFlaskHealAmount, Stats{}),
        "Second Wind changes the actual life flask heal amount");

    PassiveTree loot;
    allocateBranchEndpoint(loot, 15);
    expect(loot.allocate(19), "allocate Loaded Dice endpoint");
    expect(loot.nodes()[19].keystone == PassiveKeystone::LoadedDice,
        "Loot endpoint uses Loaded Dice data");
    const Stats lootStats = loot.combinedStats();
    expect(std::abs(lootStats.itemQuantityMultiplier - 1.25f) < 0.0001f,
        "Loaded Dice increases item quantity by 25 percent");
    expect(std::abs(lootStats.incomingDamageMultiplier - 1.20f) < 0.0001f,
        "Loaded Dice increases incoming damage by 20 percent");
    expect(itemDropChancePercent(35, 1.0f, lootStats) > itemDropChancePercent(35, 1.0f, Stats{}),
        "Loaded Dice increases real item drop chance");
    expect(incomingDamage(10, lootStats) > incomingDamage(10, Stats{}),
        "Loaded Dice increases real incoming damage");
}

// --- Skill bar ---
void testSkillBarAssignSkillAndSupport() {
    section("SkillBar assign skill/support compatibility");

    SkillBar bar;

    expect(bar.assignSkill(SkillSlot::Primary, "Spread Shot"), "assign Spread Shot to Primary");
    expect(!bar.assignSkill(SkillSlot::Primary, "Meteor"), "reject Meteor on Primary (wrong slot)");
    expect(!bar.assignSkill(SkillSlot::Secondary, "NotARealSkill"), "reject unknown skill name");

    expect(bar.assignSkill(SkillSlot::Secondary, "Meteor"), "assign Meteor to Secondary");
    expect(bar.definition(SkillSlot::Secondary).name == "Meteor", "Secondary definition is Meteor");

    // Pierce only on projectiles.
    expect(bar.assignSupport(SkillSlot::Primary, "Pierce"), "Pierce attaches to projectile Primary");
    expect(bar.support(SkillSlot::Primary) != nullptr, "Primary has support after Pierce");
    expect(bar.support(SkillSlot::Primary)->name == "Pierce", "Primary support name is Pierce");

    expect(!bar.assignSupport(SkillSlot::Secondary, "Pierce"),
        "reject Pierce on area Secondary (incompatible)");
    expect(bar.support(SkillSlot::Secondary) == nullptr, "Secondary still has no support");

    expect(bar.assignSupport(SkillSlot::Secondary, "Amplify"), "Amplify attaches to area Secondary");
    expect(bar.support(SkillSlot::Secondary) != nullptr
            && bar.support(SkillSlot::Secondary)->name == "Amplify",
        "Secondary support is Amplify");

    expect(!bar.assignSupport(SkillSlot::Movement, "Quickcast"),
        "reject Quickcast on Movement slot");
    expect(bar.assignSupport(SkillSlot::Movement, "Trailblazer"),
        "Trailblazer attaches to Movement slot");
    expect(bar.support(SkillSlot::Movement) != nullptr
            && bar.support(SkillSlot::Movement)->name == "Trailblazer",
        "Movement support is Trailblazer");

    expect(bar.assignSkill(SkillSlot::Secondary, "Flare"), "assign Flare to Secondary");
    expect(bar.assignSupport(SkillSlot::Secondary, "Combustion"),
        "Combustion attaches to Ignite skill");
    expect(!bar.assignSupport(SkillSlot::Secondary, "Deep Chill"),
        "reject Deep Chill on Ignite skill");
    expect(bar.assignSkill(SkillSlot::Secondary, "Frost Bomb"), "assign Frost Bomb to Secondary");
    expect(bar.assignSupport(SkillSlot::Secondary, "Deep Chill"),
        "Deep Chill attaches to Chill skill");

    // Replacing projectile skill with another keeps Pierce only if still compatible.
    expect(bar.assignSkill(SkillSlot::Utility, "Pulse"), "assign Pulse to Utility");
    expect(bar.assignSupport(SkillSlot::Utility, "Quickcast"), "Quickcast on Utility");
    expect(bar.assignSkill(SkillSlot::Utility, "Nova"), "swap Utility to Nova");
    expect(bar.support(SkillSlot::Utility) != nullptr
            && bar.support(SkillSlot::Utility)->name == "Quickcast",
        "Quickcast remains (compatible with Nova)");
}

// --- Mana resource ---
void testManaResourceAndSkillCastGates() {
    section("Mana resource and skill cast gates");

    Player player;
    expect(std::abs(player.mana() - Config::PlayerMaxMana) < 0.0001f,
        "player starts with full Mana");
    expect(std::abs(player.maxMana() - Config::PlayerMaxMana) < 0.0001f,
        "player max Mana uses the configured value");
    expect(std::abs(player.manaRegenPerSecond() - Config::PlayerManaRegenPerSecond) < 0.0001f,
        "player Mana regeneration uses the configured value");
    expect(player.canSpendMana(0.0f), "zero-cost skill is always affordable");
    expect(player.spendMana(25.0f), "player can spend affordable Mana");
    expect(std::abs(player.mana() - 75.0f) < 0.0001f,
        "spending Mana subtracts exactly the requested amount");
    expect(!player.spendMana(100.0f), "insufficient Mana rejects the spend");
    expect(std::abs(player.mana() - 75.0f) < 0.0001f,
        "rejected Mana spend has no side effect");

    player.update(-1.0f);
    expect(std::abs(player.mana() - 75.0f) < 0.0001f,
        "negative dt does not regenerate Mana");
    player.update(0.0f);
    expect(std::abs(player.mana() - 75.0f) < 0.0001f,
        "zero dt does not regenerate Mana");
    player.update(1.0f);
    expect(std::abs(player.mana() - (75.0f + Config::PlayerManaRegenPerSecond)) < 0.0001f,
        "positive dt regenerates Mana at the configured rate");
    player.update(100.0f);
    expect(std::abs(player.mana() - player.maxMana()) < 0.0001f,
        "Mana regeneration clamps at max Mana");

    const auto& skills = SkillLibrary::all();
    expect(skills.size() == 8, "skill library exposes all eight Mana-aware skills");
    const auto& primary = SkillLibrary::spreadShot();
    const auto& secondary = SkillLibrary::meteor();
    const auto& utility = SkillLibrary::pulse();
    const auto& movement = SkillLibrary::dash();
    expect(primary.manaCost > 0.0f && primary.manaCost < secondary.manaCost,
        "Primary has a lower Mana cost than Meteor");
    expect(secondary.manaCost > 0.0f && utility.manaCost > 0.0f,
        "Secondary and Utility skills have positive Mana costs");
    expect(movement.manaCost == 0.0f, "Dash has zero Mana cost");
    expect(std::abs(SkillLibrary::flare().manaCost - Config::FlareManaCost) < 0.0001f,
        "Flare exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::meteor().manaCost - Config::MeteorManaCost) < 0.0001f,
        "Meteor exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::frostBomb().manaCost - Config::FrostBombManaCost) < 0.0001f,
        "Frost Bomb exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::nova().manaCost - Config::NovaManaCost) < 0.0001f,
        "Nova exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::pulse().manaCost - Config::PulseManaCost) < 0.0001f,
        "Pulse exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::bladestorm().manaCost - Config::BladestormManaCost) < 0.0001f,
        "Bladestorm exposes its configured Mana cost");
    for (const auto& skill : skills) {
        expect(skill.manaCost >= 0.0f,
            skill.name + " has an explicit non-negative Mana cost");
    }

    SkillBar bar;
    expect(bar.canCast(SkillSlot::Secondary),
        "ready skill can be checked without consuming cooldown");
    expect(player.spendMana(secondary.manaCost),
        "affordable skill can spend its Mana before cooldown consumption");
    bar.consumeCooldown(SkillSlot::Secondary);
    expect(!bar.canCast(SkillSlot::Secondary),
        "consumed skill cooldown becomes unavailable");
    player.spendMana(player.mana());
    expect(!player.spendMana(secondary.manaCost),
        "a second cast is rejected after Mana is exhausted");
    expect(!bar.canCast(SkillSlot::Secondary),
        "Mana rejection does not make an already cooling skill ready");

    SkillBar insufficientBar;
    Player emptyPlayer;
    emptyPlayer.spendMana(emptyPlayer.mana());
    expect(insufficientBar.canCast(SkillSlot::Primary),
        "insufficient-Mana test starts with a ready Primary cooldown");
    expect(!emptyPlayer.spendMana(primary.manaCost),
        "insufficient Mana rejects Primary before cooldown is consumed");
    expect(insufficientBar.canCast(SkillSlot::Primary),
        "insufficient Mana leaves the Primary cooldown ready");
}

// --- Combat math ---
void testCombatMathDamageRadiusPierce() {
    section("CombatMath skillDamage / skillRadius / skillPierce");

    const SkillDefinition projectile = SkillLibrary::spreadShot();
    const SkillDefinition area = SkillLibrary::meteor();
    const SupportDefinition* pierce = SupportLibrary::find("Pierce");
    const SupportDefinition* amplify = SupportLibrary::find("Amplify");
    const SupportDefinition* quickcast = SupportLibrary::find("Quickcast");
    const SupportDefinition* volley = SupportLibrary::find("Volley");
    const SupportDefinition* trailblazer = SupportLibrary::find("Trailblazer");
    const SupportDefinition* combustion = SupportLibrary::find("Combustion");
    const SupportDefinition* deepChill = SupportLibrary::find("Deep Chill");
    expect(pierce != nullptr && amplify != nullptr && quickcast != nullptr
            && volley != nullptr && trailblazer != nullptr
            && combustion != nullptr && deepChill != nullptr,
        "supports exist in library");

    Stats stats;
    stats.damageMultiplier = 1.0f;
    stats.projectileDamageMultiplier = 1.5f;
    stats.areaDamageMultiplier = 2.0f;
    stats.areaRadiusMultiplier = 1.25f;

    const int baseProj = skillDamage(projectile, Stats{}, nullptr);
    expect(baseProj == std::max(1, projectile.baseDamage),
        "projectile base damage matches skill baseDamage with default stats");

    const int buffedProj = skillDamage(projectile, stats, nullptr);
    expect(buffedProj > baseProj, "projectile damage scales with projectileDamageMultiplier");

    const int areaDmg = skillDamage(area, stats, nullptr);
    const int areaBase = skillDamage(area, Stats{}, nullptr);
    expect(areaDmg > areaBase, "area damage scales with areaDamageMultiplier");

    const int quickcastDmg = skillDamage(area, stats, quickcast);
    expect(quickcastDmg < areaDmg, "Quickcast support reduces damage via damageMultiplier");

    const float baseRadius = skillRadius(area, Stats{}, nullptr);
    expect(std::abs(baseRadius - area.radius) < 0.001f, "area radius matches skill.radius by default");

    const float scaledRadius = skillRadius(area, stats, nullptr);
    expect(scaledRadius > baseRadius, "area radius scales with areaRadiusMultiplier");

    const float ampRadius = skillRadius(area, stats, amplify);
    expect(ampRadius > scaledRadius, "Amplify support increases area radius");

    // Projectile radius is not scaled by area radius stats.
    const float projRadius = skillRadius(projectile, stats, amplify);
    expect(std::abs(projRadius - projectile.radius) < 0.001f,
        "projectile skillRadius ignores area radius multipliers");

    expect(skillPierceCount(nullptr) == 0, "no support => 0 pierce");
    expect(skillPierceCount(pierce) == 1, "Pierce support => 1 pierce");
    expect(skillPierceCount(amplify) == 0, "Amplify does not grant pierce");
    expect(skillProjectileCount(projectile, volley) == projectile.projectileCount + 2,
        "Volley adds two projectiles");
    expect(skillSpreadAngle(projectile, volley) > projectile.spreadAngle,
        "Volley widens projectile spread");
    expect(supportAreaDamage(*trailblazer, stats) > 0,
        "Trailblazer produces area damage with player stats");
    expect(supportAreaDamage(*trailblazer, stats, 1.35f) > supportAreaDamage(*trailblazer, stats),
        "Trailblazer damage stacks shrine multiplier");
    expect(supportAreaRadius(*trailblazer, stats) > trailblazer->dashRadius,
        "Trailblazer radius scales with area specialization");

    const AilmentDefinition baseIgnite = SkillLibrary::meteor().ailment;
    const AilmentDefinition combustionIgnite = skillAilment(SkillLibrary::meteor(), combustion);
    expect(combustionIgnite.damageMultiplier > baseIgnite.damageMultiplier,
        "Combustion increases Ignite damage multiplier");
    expect(combustionIgnite.duration > baseIgnite.duration,
        "Combustion increases Ignite duration");
    expect(skillDamage(SkillLibrary::meteor(), stats, combustion) < areaDmg,
        "Combustion applies its hit damage tradeoff");

    const AilmentDefinition baseChill = SkillLibrary::frostBomb().ailment;
    const AilmentDefinition specializedChill = skillAilment(SkillLibrary::frostBomb(), deepChill);
    expect(specializedChill.duration > baseChill.duration,
        "Deep Chill increases Chill duration");
    expect(specializedChill.speedMultiplier < baseChill.speedMultiplier,
        "Deep Chill increases Chill slow strength");

    expect(refilledFlaskCharges(0, 3, 1) == 1, "flask refill adds granted charge");
    expect(refilledFlaskCharges(2, 3, 3) == 3, "flask refill is capped at maximum charges");
    expect(refilledFlaskCharges(1, 3, -1) == 1, "negative flask refill does not remove charges");
}

// --- Ailments ---
void testSkillAilments() {
    section("Skill ailments and enemy lifecycle");

    const SkillDefinition flare = SkillLibrary::flare();
    const SkillDefinition meteor = SkillLibrary::meteor();
    const SkillDefinition frostBomb = SkillLibrary::frostBomb();
    expect(flare.ailment.type == AilmentType::Ignite, "Flare applies Ignite");
    expect(meteor.ailment.type == AilmentType::Ignite, "Meteor applies Ignite");
    expect(frostBomb.ailment.type == AilmentType::Chill, "Frost Bomb applies Chill");
    expect(ailmentTickDamage(meteor.ailment, 4) == 2,
        "Ignite tick damage derives from the scaled hit damage");
    expect(ailmentTickDamage(frostBomb.ailment, 4) == 0,
        "Chill does not create damage-over-time ticks");

    Enemy enemy({0.0f, 0.0f}, 10, 1);
    enemy.applyIgnite(2, 2.0f);
    expect(enemy.isIgnited(), "Ignite is active after application");
    enemy.updateAilments(Config::AilmentTickInterval);
    expect(enemy.hp() == 8, "Ignite deals its configured tick damage");

    enemy.applyChill(0.55f, 1.5f);
    expect(enemy.isChilled(), "Chill is active after application");
    expect(std::abs(enemy.movementSpeedMultiplier() - 0.55f) < 0.0001f,
        "Chill applies its movement speed multiplier");
    enemy.applyChill(0.70f, 2.0f);
    expect(std::abs(enemy.movementSpeedMultiplier() - 0.55f) < 0.0001f,
        "weaker Chill does not overwrite a stronger existing Chill");
    enemy.updateAilments(2.1f);
    expect(!enemy.isChilled(), "Chill expires after its duration");
    expect(std::abs(enemy.movementSpeedMultiplier() - 1.0f) < 0.0001f,
        "movement speed returns to normal after Chill expires");
}

// --- Ailment resistances and penetration ---
void testAilmentResistances() {
    section("Ailment resistances and penetration");

    const auto& normal = EnemyLibrary::forType(EnemyType::Normal);
    const auto& ranged = EnemyLibrary::forType(EnemyType::Ranged);
    const auto& elite = EnemyLibrary::forType(EnemyType::Elite);
    const auto& charger = EnemyLibrary::forType(EnemyType::Charger);
    expect(normal.igniteResistance == 0 && normal.chillResistance == 0,
        "Normal enemies have no ailment resistance");
    expect(ranged.igniteResistance == 10 && ranged.chillResistance == 10,
        "Ranged enemies use the low ailment resistance baseline");
    expect(elite.igniteResistance == 15 && elite.chillResistance == 15,
        "Elite enemies use the elevated ailment resistance baseline");
    expect(charger.igniteResistance == 15 && charger.chillResistance == 10,
        "Charger enemies use the data-driven ailment resistance baseline");

    const auto& bosses = BossLibrary::all();
    expect(bosses[0].igniteResistance == 35 && bosses[0].chillResistance == 20,
        "Brimstone exposes its Ignite-heavy resistance profile");
    expect(bosses[1].igniteResistance == 20 && bosses[1].chillResistance == 35,
        "Storm exposes its Chill-heavy resistance profile");
    expect(bosses[2].igniteResistance == 30 && bosses[2].chillResistance == 30,
        "Brood exposes its balanced resistance profile");
    for (const auto& boss : bosses) {
        expect(boss.igniteResistance >= 0 && boss.igniteResistance <= 100
                && boss.chillResistance >= 0 && boss.chillResistance <= 100,
            boss.name + " has clamped boss ailment resistance data");
    }

    expect(effectiveAilmentResistance(40, 20) == 20,
        "penetration reduces resistance before ailment scaling");
    expect(effectiveAilmentResistance(-10, 0) == 0,
        "negative resistance input clamps to zero");
    expect(effectiveAilmentResistance(150, -10) == 100,
        "resistance and penetration inputs are safely clamped");
    expect(effectiveAilmentResistance(40, 60) == 0,
        "penetration cannot create negative effective resistance");

    expect(ailmentTickDamageAfterResistance(10, 0, 0) == 10,
        "Ignite damage is unchanged with zero resistance");
    expect(ailmentTickDamageAfterResistance(10, 50, 0) == 5,
        "Ignite damage is reduced by resistance");
    expect(ailmentTickDamageAfterResistance(10, 50, 20) == 7,
        "Ignite penetration restores part of the resisted damage");
    expect(ailmentTickDamageAfterResistance(10, 100, 0) == 0,
        "full Ignite resistance prevents positive damage over time");

    expect(std::abs(chillSpeedMultiplierAfterResistance(0.55f, 0, 0) - 0.55f) < 0.0001f,
        "Chill is unchanged with zero resistance");
    expect(chillSpeedMultiplierAfterResistance(0.55f, 50, 0) > 0.55f,
        "Chill slow is reduced by resistance");
    expect(chillSpeedMultiplierAfterResistance(0.55f, 50, 20)
            < chillSpeedMultiplierAfterResistance(0.55f, 50, 0),
        "Chill penetration restores part of the slow effect");
    expect(std::abs(chillSpeedMultiplierAfterResistance(0.55f, 100, 0) - 1.0f) < 0.0001f,
        "full Chill resistance prevents the slow effect");
    expect(chillSpeedMultiplierAfterResistance(0.05f, 0, 0) >= 0.20f,
        "Chill resistance scaling preserves the minimum speed safety bound");

    const auto* combustion = SupportLibrary::find("Combustion");
    const auto* deepChill = SupportLibrary::find("Deep Chill");
    const AilmentDefinition igniteSnapshot = skillAilment(SkillLibrary::meteor(), combustion);
    const AilmentDefinition chillSnapshot = skillAilment(SkillLibrary::frostBomb(), deepChill);
    expect(igniteSnapshot.ignitePenetration == 0,
        "Combustion keeps its existing damage role without penetration");
    expect(chillSnapshot.chillPenetration == 20,
        "Deep Chill snapshots its data-driven Chill penetration");

    Enemy rangedEnemy({0.0f, 0.0f}, 20, 1, EnemyType::Ranged);
    rangedEnemy.applyIgnite(
        ailmentTickDamageAfterResistance(10, ranged.igniteResistance, 0),
        2.0f
    );
    rangedEnemy.updateAilments(Config::AilmentTickInterval);
    expect(rangedEnemy.hp() == 11,
        "Enemy ailment lifecycle consumes the resistance-adjusted Ignite tick");

    Enemy chargerEnemy({0.0f, 0.0f}, 10, 1, EnemyType::Charger);
    chargerEnemy.applyChill(
        chillSpeedMultiplierAfterResistance(0.55f, charger.chillResistance, 0),
        2.0f
    );
    expect(chargerEnemy.movementSpeedMultiplier() > 0.55f,
        "Enemy ailment lifecycle uses the resistance-adjusted Chill speed");
}

// --- Armor mitigation via Player (shipped path) ---
void testPlayerArmorMitigation() {
    section("Player armor damage mitigation");

    Player player;
    const int hpFull = player.hp();
    expect(hpFull == player.maxHp(), "fresh player at full HP");

    // No armor: 2 damage should remove 2 HP.
    expect(player.takeDamage(2) == 2, "takeDamage reports actual damage without armor");
    expect(player.hp() == hpFull - 2, "takeDamage without armor subtracts full amount");

    // Equip armor with +2 armor so mitigation uses shipped equip + recalculate path.
    Item armor;
    armor.name = "Test Plate";
    armor.slot = EquipmentSlot::Armor;
    armor.rarity = Rarity::Magic;
    armor.stats.armor = 2;
    armor.stats.maxHp = 0;
    player.equipItem(std::move(armor));
    expect(player.stats().armor == 2, "equipped armor contributes to combined stats.armor");

    const int hpBefore = player.hp();
    expect(player.takeDamage(3) == mitigatedDamage(3, 2),
        "takeDamage reports armor-mitigated damage");
    // mitigatedDamage(3, 2) == 1
    expect(player.hp() == hpBefore - mitigatedDamage(3, 2),
        "takeDamage uses mitigatedDamage(raw, armor) via shipped Player path");
    expect(mitigatedDamage(3, 2) == 1, "mitigatedDamage(3,2) == 1");
    expect(mitigatedDamage(1, 5) == 1, "mitigatedDamage never drops below 1");
}

// --- Equipment stats feed combat math ---
void testEquipmentChangesCombatStats() {
    section("Equipment changes combined stats used by combat math");

    Player player;
    const int dmgBefore = skillDamage(SkillLibrary::spreadShot(), player.stats(), nullptr);

    Item weapon;
    weapon.name = "Vicious Blade";
    weapon.slot = EquipmentSlot::Weapon;
    weapon.rarity = Rarity::Rare;
    weapon.stats.damageMultiplier = 1.20f;
    weapon.stats.projectileDamageMultiplier = 1.10f;
    player.equipItem(std::move(weapon));

    const int dmgAfter = skillDamage(SkillLibrary::spreadShot(), player.stats(), nullptr);
    expect(dmgAfter > dmgBefore, "weapon affixes increase skillDamage for projectiles");
    expect(player.stats().damageMultiplier > 1.0f, "player combined damageMultiplier > 1 after equip");
}

// --- Loot generation ---
void testLootGeneration() {
    section("LootGenerator rarity/slot/affixes");

    std::srand(42);
    LootGenerator gen;

    bool sawAffix = false;
    bool sawValidSlot = true;
    bool sawValidRarity = true;
    bool sawName = true;

    for (int i = 0; i < 24; ++i) {
        Item item = gen.generate(3);
        if (item.name.empty()) {
            sawName = false;
        }
        if (item.slot < EquipmentSlot::Weapon || item.slot >= EquipmentSlot::Count) {
            sawValidSlot = false;
        }
        if (item.rarity != Rarity::Normal && item.rarity != Rarity::Magic && item.rarity != Rarity::Rare) {
            sawValidRarity = false;
        }
        if (!item.affixes.empty()) {
            sawAffix = true;
        }
        // Affix count must match rarity rules in LootGenerator.
        const int expectedAffixes = item.rarity == Rarity::Normal ? 1
            : item.rarity == Rarity::Magic ? 2 : 3;
        expect(static_cast<int>(item.affixes.size()) == expectedAffixes,
            "item ilvl3 roll " + std::to_string(i) + " affix count matches rarity");
        expect(item.itemLevel == 3, "itemLevel equals monster level 3 for roll " + std::to_string(i));
        for (const auto& affix : item.affixes) {
            expect(affix.tier == 2, "item ilvl3 roll " + std::to_string(i) + " rolls T2 affixes");
        }
    }

    expect(sawName, "generated items have non-empty names");
    expect(sawValidSlot, "generated items use valid equipment slots");
    expect(sawValidRarity, "generated items use valid rarities");
    expect(sawAffix, "at least one generated item has affixes");

    Item boss = gen.generateBossReward(5, BossLootTheme::Brimstone);
    expect(boss.rarity == Rarity::Rare, "boss relic is Rare");
    expect(!boss.name.empty(), "boss relic has a name");
    expect(!boss.affixes.empty(), "boss relic has affixes");
    expect(boss.slot == EquipmentSlot::Weapon, "Brimstone boss relic is a Weapon");
    for (const auto& affix : boss.affixes) {
        expect(affix.tier == 3, "boss ilvl5 relic rolls T3 affixes");
    }

    expect(LootGenerator::rarityForRoll(1, 20) == Rarity::Magic,
        "low-level rarity roll 20 is Magic");
    expect(LootGenerator::rarityForRoll(5, 20) == Rarity::Rare,
        "same roll becomes Rare at higher map level");
}

// --- Item base types and implicit stats ---
void testItemBaseTypes() {
    section("Item base types and implicit stats");

    const auto& bases = ItemBaseLibrary::all();
    std::set<std::string> baseIds;
    for (const auto& base : bases) {
        expect(baseIds.insert(base.id).second, base.id + " has a unique base id");
        expect(!base.name.empty(), base.id + " has a display name");
        expect(base.slot >= EquipmentSlot::Weapon && base.slot < EquipmentSlot::Count,
            base.id + " uses a valid equipment slot");
    }

    for (int slotValue = static_cast<int>(EquipmentSlot::Weapon);
        slotValue < static_cast<int>(EquipmentSlot::Count); ++slotValue) {
        const auto slot = static_cast<EquipmentSlot>(slotValue);
        int normalBaseCount = 0;
        for (const auto& base : bases) {
            normalBaseCount += base.kind == ItemBaseKind::Normal && base.slot == slot ? 1 : 0;
        }
        expect(normalBaseCount >= 3,
            std::string(slotName(slot)) + " has at least three normal base types");
    }

    std::srand(17);
    LootGenerator generator;
    for (int roll = 0; roll < 16; ++roll) {
        const Item item = generator.generate(3);
        const auto* base = ItemBaseLibrary::find(item.baseId);
        expect(base != nullptr, "generated item resolves its base id");
        if (!base) {
            continue;
        }

        expect(base->kind == ItemBaseKind::Normal && base->slot == item.slot,
            "normal drop uses a normal base for its slot");
        expect(item.baseName == base->name && statsEqual(item.implicitStats, base->implicitStats),
            "generated item preserves base name and implicit stats");
        expect(item.name.find(item.baseName) != std::string::npos,
            "generated item name contains its base name");

        Stats expected = item.implicitStats;
        for (const auto& affix : item.affixes) {
            expected = combineStats(expected, affix.stats);
        }
        expect(statsEqual(item.stats, expected),
            "generated item stats equal implicit plus affix contributions");
    }

    const std::array<std::pair<BossLootTheme, std::string>, 3> bossThemes{{
        {BossLootTheme::Brimstone, "boss.brimstone-brand"},
        {BossLootTheme::Storm, "boss.storm-signet"},
        {BossLootTheme::Brood, "boss.brood-talisman"},
    }};
    for (const auto& [theme, expectedBaseId] : bossThemes) {
        const Item item = generator.generateBossReward(5, theme);
        expect(item.baseId == expectedBaseId, "Boss relic keeps its theme-specific base");
        const auto* base = ItemBaseLibrary::find(item.baseId);
        expect(base != nullptr && base->kind == ItemBaseKind::BossRelic,
            "Boss relic resolves to a special base definition");

        Stats expected = item.implicitStats;
        for (const auto& affix : item.affixes) {
            expected = combineStats(expected, affix.stats);
        }
        expect(statsEqual(item.stats, expected),
            "Boss relic stats equal implicit plus affix contributions");
    }

    const Item brimstone = generator.generateBossReward(5, BossLootTheme::Brimstone);
    const Item storm = generator.generateBossReward(5, BossLootTheme::Storm);
    const Item brood = generator.generateBossReward(5, BossLootTheme::Brood);
    expect(std::abs(brimstone.stats.damageMultiplier - 1.27f) < 0.0001f
            && std::abs(brimstone.stats.areaDamageMultiplier - 1.14f) < 0.0001f,
        "Brimstone relic preserves its level-scaled combat bonuses");
    expect(std::abs(storm.stats.attackSpeedMultiplier - 1.16f) < 0.0001f
            && std::abs(storm.stats.projectileDamageMultiplier - 1.16f) < 0.0001f,
        "Storm relic preserves its level-scaled combat bonuses");
    expect(std::abs(brood.stats.areaDamageMultiplier - 1.16f) < 0.0001f
            && std::abs(brood.stats.areaRadiusMultiplier - 1.14f) < 0.0001f,
        "Brood relic preserves its level-scaled combat bonuses");
}

// --- Affix tags, weights and themed selection ---
void testAffixTagsAndWeights() {
    section("Affix tags, weights and themed selection");

    const auto& affixes = LootGenerator::affixDefinitions();
    expect(!affixes.empty(), "affix library exposes data-driven definitions");
    std::set<std::string> affixIds;
    for (const auto& affix : affixes) {
        expect(!affix.tags.empty() && affix.weight > 0,
            affix.name + " has tags and a positive weight");
        expect(!affix.id.empty() && affixIds.insert(affix.id).second,
            affix.name + " has a unique stable id");

        const auto hasTag = [&](AffixTag tag) {
            return std::find(affix.tags.begin(), affix.tags.end(), tag) != affix.tags.end();
        };
        switch (affix.stat) {
            case AffixStat::MaxHp:
                expect(hasTag(AffixTag::Survival), affix.name + " maps MaxHp to Survival");
                break;
            case AffixStat::DamageMultiplier:
                expect(hasTag(AffixTag::Damage), affix.name + " maps damage to Damage");
                break;
            case AffixStat::AttackSpeedMultiplier:
                expect(hasTag(AffixTag::AttackSpeed), affix.name + " maps attack speed to AttackSpeed");
                break;
            case AffixStat::MoveSpeedMultiplier:
                expect(hasTag(AffixTag::MoveSpeed), affix.name + " maps move speed to MoveSpeed");
                break;
            case AffixStat::PickupRangeMultiplier:
                expect(hasTag(AffixTag::Pickup), affix.name + " maps pickup to Pickup");
                break;
            case AffixStat::ProjectileDamageMultiplier:
                expect(hasTag(AffixTag::Projectile) && hasTag(AffixTag::Damage),
                    affix.name + " maps projectile damage to Projectile and Damage");
                break;
            case AffixStat::AreaDamageMultiplier:
            case AffixStat::AreaRadiusMultiplier:
                expect(hasTag(AffixTag::Area), affix.name + " maps area scaling to Area");
                break;
            case AffixStat::Armor:
                expect(hasTag(AffixTag::Armor) && hasTag(AffixTag::Survival),
                    affix.name + " maps armor to Armor and Survival");
                break;
        }
    }

    expect(LootGenerator::weightedChoiceIndex({1, 3}, 0) == 0,
        "weighted choice starts in the first bucket");
    expect(LootGenerator::weightedChoiceIndex({1, 3}, 1) == 1,
        "weighted choice uses the larger second bucket");
    expect(LootGenerator::weightedChoiceIndex({1, 3}, 7) == 1,
        "weighted choice wraps deterministic rolls by total weight");
    expect(LootGenerator::weightedChoiceIndex({}, 10) == 0,
        "weighted choice handles an empty candidate list");

    const auto projectileIt = std::find_if(
        affixes.begin(), affixes.end(),
        [](const AffixDefinition& affix) {
            return affix.slot == EquipmentSlot::Weapon
                && affix.stat == AffixStat::ProjectileDamageMultiplier;
        }
    );
    const auto armorIt = std::find_if(
        affixes.begin(), affixes.end(),
        [](const AffixDefinition& affix) {
            return affix.slot == EquipmentSlot::Armor
                && affix.stat == AffixStat::Armor;
        }
    );
    expect(projectileIt != affixes.end() && armorIt != affixes.end(),
        "bias test finds projectile and armor affixes");
    if (projectileIt != affixes.end() && armorIt != affixes.end()) {
        const LootBias projectileBias{AffixTag::Projectile, 1.5f, AffixTag::None, 1.0f};
        const LootBias noBias{};
        expect(LootGenerator::weightFor(*projectileIt, projectileBias)
                > LootGenerator::weightFor(*projectileIt, noBias),
            "Projectile bias increases projectile affix weight");
        expect(LootGenerator::weightFor(*armorIt, projectileBias)
                == LootGenerator::weightFor(*armorIt, noBias),
            "Projectile bias does not alter unrelated armor weight");
    }

    const auto mapOptions = MapOptionLibrary::generateOptions(2);
    expect(mapOptions[0].modifier.lootBiasTag != AffixTag::None
            && mapOptions[1].modifier.lootBiasTag != AffixTag::None
            && mapOptions[2].modifier.lootBiasTag != AffixTag::None,
        "map options expose explicit loot bias tags");

    auto generateSignatures = [](unsigned int seed, const LootBias& bias) {
        std::srand(seed);
        LootGenerator generator;
        std::vector<std::string> signatures;
        for (int index = 0; index < 10; ++index) {
            const Item item = generator.generate(3, bias);
            std::string signature = item.baseId + "|" + item.name;
            for (const auto& affix : item.affixes) {
                signature += "|" + affix.name + ":" + std::to_string(affix.tier);
            }
            signatures.push_back(signature);
        }
        return signatures;
    };
    const LootBias areaBias{AffixTag::Area, 1.45f, AffixTag::None, 1.0f};
    expect(generateSignatures(91, areaBias) == generateSignatures(91, areaBias),
        "fixed seed reproduces weighted item selection");

    std::srand(123);
    LootGenerator generator;
    for (int index = 0; index < 18; ++index) {
        const Item item = generator.generate(5, areaBias);
        std::set<AffixStat> rolledStats;
        for (const auto& affix : item.affixes) {
            const auto definition = std::find_if(
                affixes.begin(), affixes.end(),
                [&](const AffixDefinition& candidate) {
                    return candidate.name == affix.name && candidate.slot == item.slot;
                }
            );
            if (definition != affixes.end()) {
                expect(rolledStats.insert(definition->stat).second,
                    "generated item avoids duplicate AffixStat");
            }
            expect(!affix.tags.empty(), "rolled affix preserves its tags on the Item");
        }
    }
}

// --- Crafting choice operations ---
void testCraftingChoiceOperations() {
    section("Crafting choice operations");

    std::srand(2468);
    LootGenerator generator;
    Item item;
    for (int roll = 0; roll < 64; ++roll) {
        item = generator.generate(3);
        if (item.affixes.size() >= 2) {
            break;
        }
    }

    expect(item.affixes.size() >= 2, "crafting fixture has at least two affixes");
    if (item.affixes.size() < 2) {
        return;
    }

    const std::string baseId = item.baseId;
    const std::string baseName = item.baseName;
    const Stats implicit = item.implicitStats;
    const ItemAffix originalTarget = item.affixes[0];
    const ItemAffix originalOther = item.affixes[1];

    item.affixes[0].tier = 1;
    item.affixes[0].stats = LootGenerator::contributionFor(item.affixes[0].id, item.itemLevel, 1);
    LootGenerator::rebuildStats(item);
    const Stats beforeImprove = item.affixes[0].stats;
    expect(LootGenerator::improveAffix(item, 0) == CraftingResult::Success,
        "ImproveAffix changes a craftable affix");
    expect(item.affixes[0].stats.damageMultiplier != beforeImprove.damageMultiplier
            || item.affixes[0].stats.attackSpeedMultiplier != beforeImprove.attackSpeedMultiplier
            || item.affixes[0].stats.moveSpeedMultiplier != beforeImprove.moveSpeedMultiplier
            || item.affixes[0].stats.maxHp != beforeImprove.maxHp
            || item.affixes[0].stats.armor != beforeImprove.armor
            || item.affixes[0].stats.projectileDamageMultiplier != beforeImprove.projectileDamageMultiplier
            || item.affixes[0].stats.areaDamageMultiplier != beforeImprove.areaDamageMultiplier
            || item.affixes[0].stats.areaRadiusMultiplier != beforeImprove.areaRadiusMultiplier
            || item.affixes[0].stats.pickupRangeMultiplier != beforeImprove.pickupRangeMultiplier,
        "ImproveAffix changes only the target contribution");
    expect(item.baseId == baseId && item.baseName == baseName
            && statsEqual(item.implicitStats, implicit)
            && item.affixes[1].id == originalOther.id,
        "ImproveAffix preserves Base, implicit and other affixes");

    Stats expected = item.implicitStats;
    for (const auto& affix : item.affixes) {
        expected = combineStats(expected, affix.stats);
    }
    expect(statsEqual(item.stats, expected),
        "ImproveAffix rebuilds final Item stats from implicit and contributions");

    item.affixes[0].tier = 1;
    item.affixes[0].stats = LootGenerator::contributionFor(item.affixes[0].id, item.itemLevel, 1);
    LootGenerator::rebuildStats(item);
    expect(LootGenerator::raiseAffixTier(item, 0) == CraftingResult::Success,
        "RaiseAffixTier raises a lower-tier affix");
    expect(item.affixes[0].tier == 2
            && statsEqual(item.affixes[0].stats,
                LootGenerator::contributionFor(item.affixes[0].id, item.itemLevel, 2)),
        "RaiseAffixTier recomputes the contribution for the new tier");
    expect(LootGenerator::raiseAffixTier(item, 0) == CraftingResult::AlreadyMaxTier,
        "RaiseAffixTier rejects the item-level maximum tier");

    const std::string targetIdBeforeReroll = item.affixes[0].id;
    const ItemAffix otherBeforeReroll = item.affixes[1];
    std::srand(97531);
    expect(LootGenerator::rerollAffix(item, 0) == CraftingResult::Success,
        "RerollAffix selects a legal replacement");
    expect(item.affixes[0].id != targetIdBeforeReroll
            && item.affixes[0].isPrefix == originalTarget.isPrefix
            && item.affixes[1].id == otherBeforeReroll.id,
        "RerollAffix changes only the selected affix and preserves its affix group");
    expect(item.baseId == baseId && statsEqual(item.implicitStats, implicit),
        "RerollAffix preserves Base and implicit stats");

    expected = item.implicitStats;
    for (const auto& affix : item.affixes) {
        expected = combineStats(expected, affix.stats);
    }
    expect(statsEqual(item.stats, expected),
        "RerollAffix rebuilds final Item stats");

    expect(LootGenerator::improveAffix(item, item.affixes.size()) == CraftingResult::InvalidTarget,
        "Crafting rejects an out-of-range affix index");
    Item relic = generator.generateBossReward(5, BossLootTheme::Storm);
    expect(LootGenerator::rerollAffix(relic, 0) == CraftingResult::InvalidTarget,
        "Boss relic marker affix is not rerollable");
}

// --- Elite modifiers ---
void testEliteModifierDefinitions() {
    section("Elite modifier definitions");

    const auto& none = EliteModifierLibrary::forModifier(EliteModifier::None);
    const auto& hardened = EliteModifierLibrary::forModifier(EliteModifier::Hardened);
    const auto& swift = EliteModifierLibrary::forModifier(EliteModifier::Swift);
    const auto& volatileModifier = EliteModifierLibrary::forModifier(EliteModifier::Volatile);

    expect(none.name.empty(), "None modifier has no display label");
    expect(std::abs(none.hpMultiplier - 1.0f) < 0.0001f
            && std::abs(none.speedMultiplier - 1.0f) < 0.0001f,
        "None modifier leaves elite base stats unchanged");
    expect(hardened.hpMultiplier > 1.0f && hardened.speedMultiplier == 1.0f,
        "Hardened increases life without increasing speed");
    expect(swift.speedMultiplier > 1.0f && swift.hpMultiplier == 1.0f,
        "Swift increases speed without increasing life");
    expect(volatileModifier.deathBurstRadius > 0.0f && volatileModifier.deathBurstDamage > 0,
        "Volatile defines a damaging death burst");
}

// --- Charger behavior ---
void testChargerStateMachine() {
    section("Charger windup and charge state");

    const auto& definition = EnemyLibrary::forType(EnemyType::Charger);
    expect(definition.attackStyle == EnemyAttackStyle::Charge,
        "Ravager uses the charge attack style");
    expect(definition.chargeSpeedMultiplier > 1.0f && definition.chargeDuration > 0.0f,
        "Ravager definition has a fast finite charge");

    MapInstance map;
    Enemy charger({400.0f, 400.0f}, 10, 2, EnemyType::Charger);
    const Vector2 target{650.0f, 400.0f};
    charger.update(0.1f, target, map);
    expect(charger.isAttackWindingUp(), "charger enters windup inside charge range");

    charger.update(0.6f, target, map);
    expect(charger.isCharging(), "charger enters charge state after windup");
    const float beforeChargeX = charger.position().x;
    charger.update(0.1f, target, map);
    expect(charger.position().x > beforeChargeX, "charger moves along its telegraphed direction");
    expect(charger.consumeChargeHit(), "charger exposes one impact during a charge");
    expect(!charger.consumeChargeHit(), "charger cannot apply multiple impacts per charge");

    for (const auto& mapTemplate : MapTemplateLibrary::all()) {
        const auto& encounter = mapTemplate.encounter;
        expect(encounter.chargerWeight > 0,
            mapTemplate.name + " includes Charger encounters");
        expect(encounter.normalWeight + encounter.rangedWeight
                + encounter.eliteWeight + encounter.chargerWeight == 100,
            mapTemplate.name + " encounter weights total 100");
    }
}

// --- Flask reward definitions ---
void testFlaskChargeRewards() {
    section("Enemy flask charge rewards");

    const auto& normal = EnemyLibrary::forType(EnemyType::Normal);
    const auto& ranged = EnemyLibrary::forType(EnemyType::Ranged);
    const auto& elite = EnemyLibrary::forType(EnemyType::Elite);
    const auto& boss = EnemyLibrary::forType(EnemyType::Boss);

    expect(normal.flaskChargeChancePercent > 0 && normal.flaskChargeChancePercent < 100,
        "normal enemies restore flask charges only occasionally");
    expect(ranged.flaskChargeChancePercent > 0 && ranged.flaskChargeChancePercent < 100,
        "ranged enemies restore flask charges only occasionally");
    expect(elite.flaskChargeChancePercent == 100 && elite.flaskChargeAmount == 1,
        "elites always restore one flask charge");
    expect(boss.flaskChargeChancePercent == 100
            && boss.flaskChargeAmount >= Config::LifeFlaskMaxCharges,
        "boss kill refills the life flask");
}

// --- Boss summon skills ---
void testBossSummonDefinitions() {
    section("Boss summon definitions and population cap");

    const auto& bosses = BossLibrary::all();
    const auto broodIt = std::find_if(
        bosses.begin(), bosses.end(),
        [](const BossDefinition& boss) { return boss.name == "Brood Matriarch"; }
    );
    expect(broodIt != bosses.end(), "Brood Matriarch definition exists");
    if (broodIt == bosses.end()) {
        return;
    }

    bool hasNormalSummon = false;
    bool hasRangedSummon = false;
    for (const auto& skill : broodIt->skills) {
        if (skill.type != BossSkillType::SummonAdds) {
            continue;
        }

        expect(skill.summonCount > 0, skill.name + " summons at least one add");
        expect(skill.radius > 0.0f, skill.name + " has a spawn radius");
        expect(skill.telegraphDuration > 0.0f, skill.name + " has a warning window");
        hasNormalSummon = hasNormalSummon || skill.summonType == EnemyType::Normal;
        hasRangedSummon = hasRangedSummon || skill.summonType == EnemyType::Ranged;
    }
    expect(hasNormalSummon, "Brood normal phase can summon melee broodlings");
    expect(hasRangedSummon, "Brood enrage phase can summon ranged broodlings");

    bool normalOrderSummons = false;
    for (std::size_t i = 0; i < broodIt->normalSkillOrder.size(); ++i) {
        normalOrderSummons = normalOrderSummons
            || broodIt->skillForCast(i, false).type == BossSkillType::SummonAdds;
    }
    expect(normalOrderSummons, "Brood normal skill order schedules a summon");

    bool enragedOrderSummonsRanged = false;
    for (std::size_t i = 0; i < broodIt->enragedSkillOrder.size(); ++i) {
        const auto& skill = broodIt->skillForCast(i, true);
        enragedOrderSummonsRanged = enragedOrderSummonsRanged
            || (skill.type == BossSkillType::SummonAdds
                && skill.summonType == EnemyType::Ranged);
    }
    expect(enragedOrderSummonsRanged, "Brood enrage order schedules ranged broodlings");

    expect(availableBossSummonCount(4, 0, 10) == 4,
        "summon count is unchanged below the population cap");
    expect(availableBossSummonCount(4, 8, 10) == 2,
        "summon count is reduced to remaining population space");
    expect(availableBossSummonCount(4, 10, 10) == 0,
        "summon count is zero at the population cap");
    expect(availableBossSummonCount(-2, -1, 10) == 0,
        "summon count rejects invalid negative inputs");
}

// --- Persistent ground hazards ---
void testGroundHazardLifecycle() {
    section("Ground hazard data and tick lifecycle");

    GroundHazardDefinition definition{"Test Fire", 80.0f, 2.0f, 0.5f, 2};
    GroundHazard hazard({40.0f, 60.0f}, definition);
    expect(hazard.isActive(), "valid ground hazard starts active");
    expect(hazard.position().x == 40.0f && hazard.position().y == 60.0f,
        "ground hazard preserves its world position");
    expect(hazard.update(0.25f) == 0, "ground hazard waits for its first tick interval");
    expect(hazard.update(0.25f) == 1, "ground hazard ticks at the configured interval");
    expect(hazard.update(1.0f) == 2, "ground hazard reports every elapsed tick");
    expect(hazard.update(0.5f) == 1, "ground hazard includes a tick at its expiry boundary");
    expect(!hazard.isActive(), "ground hazard expires after its configured duration");
    expect(hazard.update(1.0f) == 0, "expired ground hazard cannot tick again");

    GroundHazard invalid({0.0f, 0.0f}, {});
    expect(!invalid.isActive(), "empty ground hazard definition is inactive");
    expect(invalid.update(1.0f) == 0, "invalid ground hazard does not enter a zero-interval loop");

    const auto& brimstone = BossLibrary::forMapLevel(1);
    const auto magmaIt = std::find_if(
        brimstone.skills.begin(), brimstone.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Magma Slam"; }
    );
    expect(magmaIt != brimstone.skills.end(), "Brimstone defines Magma Slam");
    if (magmaIt != brimstone.skills.end()) {
        expect(magmaIt->groundHazard.isValid(), "Magma Slam leaves a valid ground hazard");
        expect(magmaIt->groundHazard.source == "Magma Pool",
            "Magma Slam hazard has a player-facing damage source");
        expect(magmaIt->groundHazard.radius < magmaIt->radius,
            "Magma Pool is smaller than the initial slam telegraph");
    }

    int configuredHazards = 0;
    for (const auto& boss : BossLibrary::all()) {
        for (const auto& skill : boss.skills) {
            configuredHazards += skill.groundHazard.isValid() ? 1 : 0;
        }
    }
    expect(configuredHazards == 1, "only Brimstone Magma Slam creates a v1 ground hazard");
}

// --- Boss mobility skills ---
void testBossDashStateAndStormPattern() {
    section("Boss dash state and Storm Herald pattern");

    BossDashState dash;
    dash.begin({0.0f, 0.0f}, {200.0f, 0.0f}, 0.5f, 400.0f);
    expect(dash.isTelegraphing(), "boss dash starts in telegraph phase");
    expect(dash.target().x == 200.0f && dash.target().y == 0.0f,
        "boss dash snapshots its target position");
    expect(dash.update(0.25f, {0.0f, 0.0f}).lengthSquared() == 0.0f,
        "boss dash does not move during telegraph");
    expect(std::abs(dash.telegraphProgress() - 0.5f) < 0.0001f,
        "boss dash reports remaining telegraph progress");
    expect(dash.update(0.25f, {0.0f, 0.0f}).lengthSquared() == 0.0f,
        "boss dash transition frame does not move early");
    expect(dash.isMoving(), "boss dash enters moving phase after telegraph");

    const Vector2 firstStep = dash.update(0.25f, {0.0f, 0.0f});
    expect(std::abs(firstStep.x - 100.0f) < 0.0001f && firstStep.y == 0.0f,
        "boss dash movement uses configured speed");
    expect(dash.consumeHit(), "boss dash exposes its first collision hit");
    expect(!dash.consumeHit(), "boss dash cannot hit twice during one movement");

    const Vector2 finalStep = dash.update(0.25f, {100.0f, 0.0f});
    expect(std::abs(finalStep.x - 100.0f) < 0.0001f,
        "boss dash reaches its locked target without overshoot");
    expect(dash.phase() == BossDashPhase::Impact, "boss dash enters impact phase at target");
    expect(dash.consumeCompletion(), "boss dash completion is consumed once");
    expect(!dash.consumeCompletion() && !dash.isActive(),
        "completed boss dash returns to idle");

    BossDashState invalidDash;
    invalidDash.begin({5.0f, 5.0f}, {5.0f, 5.0f}, 0.5f, 400.0f);
    expect(!invalidDash.isActive(), "boss dash rejects a zero-distance target");

    const auto& storm = BossLibrary::forMapLevel(2);
    const auto dashIt = std::find_if(
        storm.skills.begin(), storm.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.type == BossSkillType::Dash; }
    );
    expect(dashIt != storm.skills.end(), "Storm Herald defines a dash skill");
    if (dashIt != storm.skills.end()) {
        expect(dashIt->dash.isValid(), "Storm dash has valid distance and speed data");
        expect(dashIt->telegraphDuration > 0.0f, "Storm dash has a warning window");
        expect(dashIt->damage > 0 && dashIt->radius > 0.0f,
            "Storm dash has collision damage and radius");
    }

    int normalDashes = 0;
    for (std::size_t i = 0; i < storm.normalSkillOrder.size(); ++i) {
        normalDashes += storm.skillForCast(i, false).type == BossSkillType::Dash ? 1 : 0;
    }
    int enragedDashes = 0;
    for (std::size_t i = 0; i < storm.enragedSkillOrder.size(); ++i) {
        enragedDashes += storm.skillForCast(i, true).type == BossSkillType::Dash ? 1 : 0;
    }
    expect(normalDashes >= 1, "Storm normal pattern schedules Tempest Rush");
    expect(enragedDashes > normalDashes, "Storm enrage pattern increases dash pressure");

    MapInstance map(1, 0);
    Enemy movingBoss(map.playerStart(), 10, 1, EnemyType::Boss);
    movingBoss.moveBy({10000.0f, 0.0f}, map);
    expect(movingBoss.position().x <= map.size().x - movingBoss.radius(),
        "boss movement remains inside map bounds");
    expect(!map.intersectsObstacle(movingBoss.position(), movingBoss.radius()),
        "boss movement resolves around map obstacles");
}

// --- Map options ---
void testMapOptionGeneration() {
    section("MapOptionLibrary distinct modifiers");

    const auto options = MapOptionLibrary::generateOptions(2);
    expect(options.size() == 3, "generateOptions yields three map options");

    expect(options[0].modifier.name != options[1].modifier.name
            && options[1].modifier.name != options[2].modifier.name
            && options[0].modifier.name != options[2].modifier.name,
        "three options have distinct modifier names");

    // Distinct life/damage vs loot profiles (criteria: different modifiers).
    bool distinctHp = options[0].modifier.monsterHpMultiplier
            != options[1].modifier.monsterHpMultiplier
        || options[1].modifier.monsterHpMultiplier
            != options[2].modifier.monsterHpMultiplier;
    bool distinctLoot = options[0].modifier.itemQuantityMultiplier
            != options[1].modifier.itemQuantityMultiplier
        || options[1].modifier.itemQuantityMultiplier
            != options[2].modifier.itemQuantityMultiplier;
    bool distinctDmg = options[0].modifier.monsterDamageBonus
            != options[1].modifier.monsterDamageBonus
        || options[1].modifier.monsterDamageBonus
            != options[2].modifier.monsterDamageBonus;
    expect(distinctHp && distinctLoot, "options differ in monster life and loot quantity");
    expect(distinctDmg || distinctHp, "options differ in damage bonus or life");

    expect(options[0].modifier.eliteWeightBonus < options[1].modifier.eliteWeightBonus
            && options[1].modifier.eliteWeightBonus < options[2].modifier.eliteWeightBonus,
        "map options scale elite pressure from moderate to high");
    expect(options[1].modifier.bossHpMultiplier > 1.0f
            && options[1].modifier.bossDamageMultiplier > 1.0f,
        "Savage Hollow increases Boss risk");
    expect(options[2].modifier.itemLevelBonus == 1,
        "Gilded Ruins grants one effective item level");

    // Template indices map to themed maps (linked in generateOptions).
    expect(options[0].templateIndex == 0
            && options[1].templateIndex == 1
            && options[2].templateIndex == 2,
        "map options bind to distinct map templates 0/1/2");
}

// --- Map rewards ---
void testMapRewardGeneration() {
    section("MapRewardLibrary unlock skill/support options");

    std::set<std::string> unlockedSkills = {
        SkillLibrary::spreadShot().name,
        SkillLibrary::meteor().name,
        SkillLibrary::pulse().name,
        SkillLibrary::dash().name,
    };
    std::set<std::string> unlockedSupports;

    std::srand(7);
    const auto rewards = MapRewardLibrary::generateOptions(unlockedSkills, unlockedSupports);
    expect(rewards.size() == 3, "map rewards yield three options");

    int skillUnlocks = 0;
    for (const auto& reward : rewards) {
        if (reward.type == MapRewardType::UnlockSkill) {
            ++skillUnlocks;
            expect(!reward.skillName.empty(), "skill unlock reward names a skill");
            expect(unlockedSkills.count(reward.skillName) == 0,
                "skill unlock is not already unlocked: " + reward.skillName);
        }
    }
    expect(skillUnlocks >= 1, "at least one reward unlocks a new skill while skills remain locked");
}

// --- Passive + equip pipeline matches Player.recalculateStats ---
void testPassiveAndEquipPipeline() {
    section("Passive + equip pipeline via Player");

    Player player;
    // Force a talent point by gaining enough exp.
    while (player.talentPoints() < 1) {
        player.gainExp(1);
    }
    expect(player.canSpendTalentPoint(), "player has talent points after level-up");

    const Stats before = player.stats();
    expect(player.spendPassivePoint(10), "spend point on Survival root (Vigour, no prereq)");
    expect(player.stats().maxHp > before.maxHp, "passive maxHp applied to combined stats");
    expect(player.maxHp() > Config::PlayerHp, "player maxHp reflects passive bonus");

    expect(!player.spendPassivePoint(12), "reject allocate Iron Heart without middle nodes");
}

// --- Item container ownership and capacity ---
void testItemContainers() {
    section("Inventory and Stash capacity/ownership");

    Stash stash;
    expect(stash.size() == 0, "stash starts empty");
    expect(stash.capacity() == static_cast<std::size_t>(Config::StashCapacity),
        "stash exposes configured capacity");

    Item preserved;
    preserved.name = "Preserved Relic";
    preserved.baseId = "test.relic";
    preserved.itemLevel = 7;
    expect(stash.add(preserved), "stash accepts a complete Item");
    auto taken = stash.take(0);
    expect(taken.has_value() && taken->name == "Preserved Relic"
            && taken->baseId == "test.relic" && taken->itemLevel == 7,
        "stash take preserves complete Item fields");
    expect(!stash.take(0).has_value(), "stash rejects out-of-range take");

    for (int index = 0; index < Config::StashCapacity; ++index) {
        Item item;
        item.name = "Stash Item " + std::to_string(index);
        expect(stash.add(std::move(item)), "stash accepts item " + std::to_string(index));
    }
    expect(stash.isFull() && stash.size() == stash.capacity(), "stash reaches capacity");
    Item rejected;
    rejected.name = "Must Stay With Caller";
    expect(!stash.add(rejected), "full stash rejects without growing");
    expect(rejected.name == "Must Stay With Caller", "full stash leaves rejected Item untouched");
    expect(stash.items().back().name == "Stash Item " + std::to_string(Config::StashCapacity - 1),
        "full stash keeps existing items unchanged");
    stash.clear();
    expect(stash.size() == 0 && !stash.isFull(), "stash clear resets capacity state");

    Inventory inventory;
    Item first;
    first.name = "First";
    Item second;
    second.name = "Second";
    expect(inventory.add(std::move(first)) && inventory.add(std::move(second)),
        "inventory accepts items");
    Item inserted;
    inserted.name = "Inserted";
    expect(inventory.insert(1, std::move(inserted)), "inventory can restore an item at its source index");
    expect(inventory.items().size() == 3 && inventory.items()[1].name == "Inserted",
        "inventory insert preserves ordering");

    for (int index = static_cast<int>(inventory.size());
        index < static_cast<int>(inventory.capacity());
        ++index) {
        Item item;
        item.name = "Inventory Item " + std::to_string(index);
        inventory.add(std::move(item));
    }
    Item inventoryRejected;
    inventoryRejected.name = "Inventory Must Stay";
    expect(!inventory.add(std::move(inventoryRejected)), "full inventory rejects an Item");
    expect(inventoryRejected.name == "Inventory Must Stay",
        "full inventory leaves rejected Item untouched");
}

} // namespace

int main() {
    std::cout << "ARPG pure-logic tests (shipped headers)\n";

    testPassiveTreePrerequisitesAndStats();
    testPassiveKeystones();
    testSkillBarAssignSkillAndSupport();
    testManaResourceAndSkillCastGates();
    testCombatMathDamageRadiusPierce();
    testSkillAilments();
    testAilmentResistances();
    testPlayerArmorMitigation();
    testEquipmentChangesCombatStats();
    testLootGeneration();
    testItemBaseTypes();
    testAffixTagsAndWeights();
    testCraftingChoiceOperations();
    testEliteModifierDefinitions();
    testChargerStateMachine();
    testFlaskChargeRewards();
    testBossSummonDefinitions();
    testGroundHazardLifecycle();
    testBossDashStateAndStormPattern();
    testMapOptionGeneration();
    testMapRewardGeneration();
    testPassiveAndEquipPipeline();
    testItemContainers();

    std::cout << "\n========================================\n";
    std::cout << "Passed: " << g_passed << "  Failed: " << g_failures << '\n';
    std::cout << "========================================\n";
    return g_failures == 0 ? 0 : 1;
}
