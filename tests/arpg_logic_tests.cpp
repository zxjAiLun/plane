// Unit tests for shipped pure ARPG logic (no SFML).
// Each assertion drives real headers/types used by the game binary.

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "BossDefinition.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"
#include "EliteModifier.hpp"
#include "Enemy.hpp"
#include "EnemyDefinition.hpp"
#include "Equipment.hpp"
#include "GroundHazard.hpp"
#include "Item.hpp"
#include "LootGenerator.hpp"
#include "MapModifier.hpp"
#include "MapInstance.hpp"
#include "MapRewardLibrary.hpp"
#include "PassiveTree.hpp"
#include "Player.hpp"
#include "SkillBar.hpp"
#include "SkillLibrary.hpp"
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

} // namespace

int main() {
    std::cout << "ARPG pure-logic tests (shipped headers)\n";

    testPassiveTreePrerequisitesAndStats();
    testSkillBarAssignSkillAndSupport();
    testCombatMathDamageRadiusPierce();
    testSkillAilments();
    testPlayerArmorMitigation();
    testEquipmentChangesCombatStats();
    testLootGeneration();
    testEliteModifierDefinitions();
    testChargerStateMachine();
    testFlaskChargeRewards();
    testBossSummonDefinitions();
    testGroundHazardLifecycle();
    testMapOptionGeneration();
    testMapRewardGeneration();
    testPassiveAndEquipPipeline();

    std::cout << "\n========================================\n";
    std::cout << "Passed: " << g_passed << "  Failed: " << g_failures << '\n';
    std::cout << "========================================\n";
    return g_failures == 0 ? 0 : 1;
}
