// Unit tests for shipped pure ARPG logic (no SFML).
// Each assertion drives real headers/types used by the game binary.

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>

#include "CombatMath.hpp"
#include "Config.hpp"
#include "Equipment.hpp"
#include "Item.hpp"
#include "LootGenerator.hpp"
#include "MapModifier.hpp"
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
        "reject any support on Movement slot");

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
    expect(pierce != nullptr && amplify != nullptr && quickcast != nullptr, "supports exist in library");

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
}

// --- Armor mitigation via Player (shipped path) ---
void testPlayerArmorMitigation() {
    section("Player armor damage mitigation");

    Player player;
    const int hpFull = player.hp();
    expect(hpFull == player.maxHp(), "fresh player at full HP");

    // No armor: 2 damage should remove 2 HP.
    player.takeDamage(2);
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
    player.takeDamage(3);
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
    testPlayerArmorMitigation();
    testEquipmentChangesCombatStats();
    testLootGeneration();
    testMapOptionGeneration();
    testMapRewardGeneration();
    testPassiveAndEquipPipeline();

    std::cout << "\n========================================\n";
    std::cout << "Passed: " << g_passed << "  Failed: " << g_failures << '\n';
    std::cout << "========================================\n";
    return g_failures == 0 ? 0 : 1;
}
