// Unit tests for shipped pure ARPG logic (no SFML).
// Each assertion drives real headers/types used by the game binary.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>

#include "BossDash.hpp"
#include "BossDefinition.hpp"
#include "BossRelicEffect.hpp"
#include "CombatMath.hpp"
#include "Config.hpp"
#include "Crafting.hpp"
#include "EliteModifier.hpp"
#include "Enemy.hpp"
#include "EnemyDefinition.hpp"
#include "EnemyPackLibrary.hpp"
#include "EnemySpawner.hpp"
#include "Equipment.hpp"
#include "GroundHazard.hpp"
#include "Inventory.hpp"
#include "Item.hpp"
#include "LootGenerator.hpp"
#include "MapExploration.hpp"
#include "MapModifier.hpp"
#include "MapInstance.hpp"
#include "MapItem.hpp"
#include "MapRewardLibrary.hpp"
#include "MapScaling.hpp"
#include "PassiveTree.hpp"
#include "Player.hpp"
#include "RandomService.hpp"
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

void section(const std::string& title);

void testRandomService() {
    section("RandomService determinism and boundaries");

    RandomService first(123456);
    RandomService second(123456);
    RandomService different(123457);
    bool sameSequence = true;
    bool differentSequence = false;
    for (int index = 0; index < 24; ++index) {
        const int firstValue = first.nextInt(-1000, 1000);
        const int secondValue = second.nextInt(-1000, 1000);
        const int differentValue = different.nextInt(-1000, 1000);
        sameSequence = sameSequence && firstValue == secondValue;
        differentSequence = differentSequence || firstValue != differentValue;
    }
    expect(sameSequence, "same seed reproduces the integer sequence");
    expect(differentSequence, "different seeds produce a different sequence");

    RandomService boundary(9);
    expect(boundary.nextInt(7, 7) == 7, "equal integer bounds return the bound");
    const int reversed = boundary.nextInt(10, 1);
    expect(reversed >= 1 && reversed <= 10, "reversed integer bounds are normalized");
    expect(boundary.nextUInt64(4, 4) == 4, "equal uint64 bounds return the bound");
    expect(boundary.nextIndex(0) == 0, "empty index range returns zero safely");
    const float unitValue = boundary.nextFloat01();
    expect(unitValue >= 0.0f && unitValue < 1.0f,
        "float generation stays inside the unit interval");
    expect(!boundary.chance(0) && boundary.chance(100),
        "chance handles deterministic zero and one hundred percent bounds");

    const std::vector<int> weights{0, 3, 7};
    const std::size_t weightedIndex = boundary.weightedChoiceIndex(weights);
    expect(weightedIndex >= 1 && weightedIndex < weights.size(),
        "weighted choice skips zero-weight buckets");
    expect(boundary.weightedChoiceIndex({}) == 0,
        "weighted choice handles an empty vector");
    expect(boundary.weightedChoiceIndex({1, 2, 3}) < 3,
        "weighted choice returns an in-range bucket");
    expect(RandomService::deriveSeed(1, 0) != RandomService::deriveSeed(1, 1),
        "derived run streams use distinct seeds");
}

void testEnemySpawnerRandomness() {
    section("EnemySpawner injected randomness");

    EnemySpawner firstSpawner;
    EnemySpawner secondSpawner;
    EnemySpawner differentSpawner;
    firstSpawner.update(Config::EnemySpawnInterval);
    secondSpawner.update(Config::EnemySpawnInterval);
    differentSpawner.update(Config::EnemySpawnInterval);
    RandomService firstRandom(3001);
    RandomService secondRandom(3001);
    RandomService differentRandom(3002);

    const auto first = firstSpawner.trySpawn(10, 1, EnemyType::Normal,
        EliteModifier::None, firstRandom);
    const auto second = secondSpawner.trySpawn(10, 1, EnemyType::Normal,
        EliteModifier::None, secondRandom);
    const auto different = differentSpawner.trySpawn(10, 1, EnemyType::Normal,
        EliteModifier::None, differentRandom);
    expect(first.has_value() && second.has_value() && different.has_value(),
        "spawner produces an enemy after its interval");
    if (first && second && different) {
        expect(first->position().x == second->position().x
                && first->position().y == second->position().y,
            "same spawner seed reproduces the spawn position");
        expect(first->position().x != different->position().x
                || first->position().y != different->position().y,
            "different spawner seeds change the spawn position");
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
        && std::abs(lhs.incomingDamageMultiplier - rhs.incomingDamageMultiplier) < 0.0001f
        && std::abs(lhs.fireDamageMultiplier - rhs.fireDamageMultiplier) < 0.0001f
        && std::abs(lhs.coldDamageMultiplier - rhs.coldDamageMultiplier) < 0.0001f
        && std::abs(lhs.lightningDamageMultiplier - rhs.lightningDamageMultiplier) < 0.0001f
        && lhs.fireResistance == rhs.fireResistance
        && lhs.coldResistance == rhs.coldResistance
        && lhs.lightningResistance == rhs.lightningResistance
        && std::abs(lhs.poisonDamageMultiplier - rhs.poisonDamageMultiplier) < 0.0001f
        && lhs.poisonResistance == rhs.poisonResistance
        && std::abs(lhs.maxManaMultiplier - rhs.maxManaMultiplier) < 0.0001f
        && std::abs(lhs.manaRegenMultiplier - rhs.manaRegenMultiplier) < 0.0001f
        && std::abs(lhs.skillCostMultiplier - rhs.skillCostMultiplier) < 0.0001f
        && std::abs(lhs.igniteDamageMultiplier - rhs.igniteDamageMultiplier) < 0.0001f
        && std::abs(lhs.igniteDurationMultiplier - rhs.igniteDurationMultiplier) < 0.0001f
        && std::abs(lhs.chillMagnitudeMultiplier - rhs.chillMagnitudeMultiplier) < 0.0001f
        && std::abs(lhs.chillDurationMultiplier - rhs.chillDurationMultiplier) < 0.0001f
        && std::abs(lhs.shockMagnitudeMultiplier - rhs.shockMagnitudeMultiplier) < 0.0001f
        && std::abs(lhs.shockDurationMultiplier - rhs.shockDurationMultiplier) < 0.0001f
        && std::abs(lhs.poisonDurationMultiplier - rhs.poisonDurationMultiplier) < 0.0001f
        && std::abs(lhs.physicalDamageMultiplier - rhs.physicalDamageMultiplier) < 0.0001f
        && std::abs(lhs.bleedDamageMultiplier - rhs.bleedDamageMultiplier) < 0.0001f
        && std::abs(lhs.bleedDurationMultiplier - rhs.bleedDurationMultiplier) < 0.0001f
        && lhs.bleedPenetration == rhs.bleedPenetration
        && lhs.bleedResistance == rhs.bleedResistance;
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
    expect(survivalStats.maxManaMultiplier > 1.0f
            && survivalStats.manaRegenMultiplier > 1.0f
            && survivalStats.skillCostMultiplier < 1.0f,
        "Survival branch supports a lower-cost Mana sustain build");
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

    PassiveTree poison;
    allocateBranchEndpoint(poison, 20);
    expect(poison.allocate(24), "allocate Toxic Bloom endpoint");
    expect(poison.nodes()[24].branch == PassiveBranch::Poison,
        "Poison endpoint uses the Poison branch");
    const Stats poisonStats = poison.combinedStats();
    expect(poison.allocatedCount(PassiveBranch::Poison) == 5,
        "Poison branch reports all five allocated nodes");
    expect(poisonStats.poisonDamageMultiplier > 1.0f
            && poisonStats.poisonResistance == 22,
        "Poison branch increases Poison damage and resistance");
    expect(skillDamage(SkillLibrary::toxicBurst(), poisonStats, nullptr)
            > skillDamage(SkillLibrary::toxicBurst(), Stats{}, nullptr),
        "Poison branch increases actual Toxic Burst damage");

    PassiveTree physicalBleed;
    for (std::size_t index = 28; index < 33; ++index) {
        expect(physicalBleed.allocate(index),
            "allocate Physical / Bleed branch node " + std::to_string(index));
    }
    expect(physicalBleed.allocatedCount(PassiveBranch::PhysicalBleed) == 5,
        "Physical / Bleed branch reports all five allocated nodes");
    const Stats physicalBleedStats = physicalBleed.combinedStats();
    expect(physicalBleedStats.physicalDamageMultiplier > 1.0f
            && physicalBleedStats.bleedDamageMultiplier > 1.0f
            && physicalBleedStats.bleedDurationMultiplier > 1.0f
            && physicalBleedStats.bleedPenetration == 16,
        "Physical / Bleed branch combines hit, ailment, duration, and penetration bonuses");

    PassiveTree elemental;
    expect(elemental.allocate(25), "allocate Ember Attunement root");
    expect(elemental.allocate(26), "allocate Glacial Attunement root");
    expect(elemental.allocate(27), "allocate Storm Attunement root");
    expect(elemental.nodes()[25].branch == PassiveBranch::Fire
            && elemental.nodes()[26].branch == PassiveBranch::Cold
            && elemental.nodes()[27].branch == PassiveBranch::Lightning,
        "elemental mastery roots use their matching branches");
    const Stats elementalStats = elemental.combinedStats();
    expect(elementalStats.fireDamageMultiplier > 1.0f
            && elementalStats.coldDamageMultiplier > 1.0f
            && elementalStats.lightningDamageMultiplier > 1.0f
            && elementalStats.fireResistance == 5
            && elementalStats.coldResistance == 5
            && elementalStats.lightningResistance == 5,
        "elemental mastery roots add damage and matching resistance");
    expect(elementalStats.igniteDamageMultiplier > 1.0f
            && elementalStats.igniteDurationMultiplier > 1.0f
            && elementalStats.chillMagnitudeMultiplier > 1.0f
            && elementalStats.chillDurationMultiplier > 1.0f
            && elementalStats.shockMagnitudeMultiplier > 1.0f
            && elementalStats.shockDurationMultiplier > 1.0f,
        "elemental mastery roots add matching ailment specialization");
    expect(skillDamage(SkillLibrary::emberLance(), elementalStats, nullptr)
            > skillDamage(SkillLibrary::emberLance(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::glacialShard(), elementalStats, nullptr)
                > skillDamage(SkillLibrary::glacialShard(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::stormfield(), elementalStats, nullptr)
                > skillDamage(SkillLibrary::stormfield(), Stats{}, nullptr),
        "elemental mastery roots increase matching elemental skills");
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
    expect(bar.assignSupport(SkillSlot::Primary, "Volley", 1),
        "Volley attaches to the second projectile link");
    expect(bar.supportAt(SkillSlot::Primary, 1) != nullptr
            && bar.supportAt(SkillSlot::Primary, 1)->name == "Volley",
        "second projectile link stores Volley");
    expect(!bar.assignSupport(SkillSlot::Primary, "Pierce", 1),
        "duplicate support cannot occupy two links");

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

    const SkillBarSaveState saved = bar.saveState();
    SkillBar restored;
    expect(restored.restoreState(saved), "SkillBar restores two support links");
    expect(restored.supportAt(SkillSlot::Primary, 0) != nullptr
            && restored.supportAt(SkillSlot::Primary, 1) != nullptr
            && restored.supportAt(SkillSlot::Primary, 1)->name == "Volley",
        "SkillBar restore preserves link order");

    SkillBarSaveState invalid = saved;
    invalid.supports[static_cast<std::size_t>(SkillSlot::Movement)][1] = "Trailblazer";
    expect(!restored.restoreState(invalid),
        "SkillBar rejects a second Movement support link");

    SkillBar expansionBar;
    expect(expansionBar.assignSkill(SkillSlot::Primary, "Arc Bolt"),
        "assign Arc Bolt to Primary");
    expect(expansionBar.assignSupport(SkillSlot::Primary, "Barrage"),
        "Barrage attaches to Arc Bolt");
    expect(expansionBar.assignSupport(SkillSlot::Primary, "Conductivity", 1),
        "Conductivity attaches to Shock skills");
    expect(!expansionBar.assignSupport(SkillSlot::Secondary, "Barrage"),
        "Barrage rejects non-Projectile skills");
    expect(!expansionBar.assignSupport(SkillSlot::Primary, "Concentration", 1),
        "Concentration rejects Projectile skills");
    expect(expansionBar.assignSkill(SkillSlot::Utility, "Shockwave"),
        "assign Shockwave to Utility");
    expect(expansionBar.assignSupport(SkillSlot::Utility, "Concentration"),
        "Concentration attaches to Shockwave");

    expect(expansionBar.assignSkill(SkillSlot::Primary, "Split Arrow"),
        "assign Split Arrow to Primary");
    expect(expansionBar.assignSkill(SkillSlot::Utility, "Aftershock"),
        "assign Aftershock to Utility");
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
    expect(std::abs(player.restoreMana(20.0f) - 20.0f) < 0.0001f
            && std::abs(player.mana() - 95.0f) < 0.0001f,
        "Mana restoration adds the requested amount while below maximum");
    expect(std::abs(player.restoreMana(20.0f) - 5.0f) < 0.0001f
            && std::abs(player.mana() - player.maxMana()) < 0.0001f,
        "Mana restoration clamps to maximum");
    expect(std::abs(player.restoreMana(20.0f)) < 0.0001f,
        "full Mana does not report a restoration amount");
    player.spendMana(25.0f);
    expect(std::abs(player.restoreMana(-1.0f)) < 0.0001f,
        "negative Mana restoration is rejected");
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

    Stats resourceStats;
    resourceStats.maxManaMultiplier = 1.50f;
    resourceStats.manaRegenMultiplier = 1.25f;
    resourceStats.skillCostMultiplier = 0.80f;
    const Stats combinedResourceStats = combineStats(Stats{}, resourceStats);
    expect(std::abs(combinedResourceStats.maxManaMultiplier - 1.50f) < 0.0001f
            && std::abs(combinedResourceStats.manaRegenMultiplier - 1.25f) < 0.0001f
            && std::abs(combinedResourceStats.skillCostMultiplier - 0.80f) < 0.0001f,
        "resource Stats combine multiplicatively");

    const auto& skills = SkillLibrary::all();
    expect(skills.size() == 22, "skill library exposes the complete build skill set");
    const auto& primary = SkillLibrary::spreadShot();
    const auto& secondary = SkillLibrary::meteor();
    const auto& utility = SkillLibrary::pulse();
    const auto& movement = SkillLibrary::dash();
    const auto& arcBolt = SkillLibrary::arcBolt();
    const auto& shockwave = SkillLibrary::shockwave();
    const auto& splitArrow = SkillLibrary::splitArrow();
    const auto& aftershock = SkillLibrary::aftershock();
    const auto& emberLance = SkillLibrary::emberLance();
    const auto& glacialShard = SkillLibrary::glacialShard();
    const auto& stormfield = SkillLibrary::stormfield();
    const auto& blightRing = SkillLibrary::blightRing();
    const auto& siphonPulse = SkillLibrary::siphonPulse();
    const auto& guardingPulse = SkillLibrary::guardingPulse();
    const auto& manaWard = SkillLibrary::manaWard();
    const auto& rendingVolley = SkillLibrary::rendingVolley();
    const auto& crimsonSweep = SkillLibrary::crimsonSweep();
    expect(primary.manaCost > 0.0f && primary.manaCost < secondary.manaCost,
        "Primary has a lower Mana cost than Meteor");
    expect(secondary.manaCost > 0.0f && utility.manaCost > 0.0f,
        "Secondary and Utility skills have positive Mana costs");
    expect(movement.manaCost == 0.0f, "Dash has zero Mana cost");
    const auto* arcaneEfficiency = SupportLibrary::find("Arcane Efficiency");
    expect(arcaneEfficiency != nullptr
            && SupportLibrary::supportsSkill(*arcaneEfficiency, secondary)
            && !SupportLibrary::supportsSkill(*arcaneEfficiency, movement),
        "Arcane Efficiency supports active skills but not Dash");
    if (arcaneEfficiency != nullptr) {
        const float baseCost = skillManaCost(secondary, resourceStats, nullptr);
        const float supportedCost = skillManaCost(secondary, resourceStats, arcaneEfficiency);
        expect(supportedCost < baseCost
                && skillCooldown(secondary, resourceStats, arcaneEfficiency) > secondary.cooldown,
            "Arcane Efficiency trades lower Mana cost for higher cooldown");
        const auto leveled = SkillProgression::supportAtLevel(*arcaneEfficiency, 5);
        expect(leveled.manaCostMultiplier < arcaneEfficiency->manaCostMultiplier,
            "support gem levels reduce Arcane Efficiency Mana cost");
    }
    expect(std::abs(arcBolt.manaCost - 2.0f) < 0.0001f
            && std::abs(shockwave.manaCost - 6.0f) < 0.0001f,
        "Arc Bolt and Shockwave expose their fixed Mana costs");
    expect(splitArrow.slot == SkillSlot::Primary
            && splitArrow.castType == SkillCastType::Projectile
            && splitArrow.projectileCount == 5
            && splitArrow.baseDamage == 1
            && aftershock.slot == SkillSlot::Utility
            && aftershock.castType == SkillCastType::SelfCenteredArea
            && aftershock.baseDamage == 5
            && aftershock.delivery == SkillDeliveryType::DelayedArea
            && aftershock.castDelay > 0.0f,
        "Split Arrow and Aftershock expose their intended build roles");
    expect(emberLance.slot == SkillSlot::Primary
            && emberLance.damageType == DamageType::Fire
            && emberLance.ailment.type == AilmentType::Ignite
            && glacialShard.slot == SkillSlot::Primary
            && glacialShard.damageType == DamageType::Cold
            && glacialShard.projectileCount == Config::GlacialShardProjectileCount
            && glacialShard.ailment.type == AilmentType::Chill,
        "Ember Lance and Glacial Shard define Fire and Cold projectile paths");
    expect(stormfield.slot == SkillSlot::Secondary
            && stormfield.damageType == DamageType::Lightning
            && stormfield.delivery == SkillDeliveryType::DelayedArea
            && stormfield.groundHazard.isValid()
            && blightRing.slot == SkillSlot::Utility
            && blightRing.damageType == DamageType::Poison
            && blightRing.delivery == SkillDeliveryType::DelayedArea
            && blightRing.groundHazard.isValid(),
        "Stormfield and Blight Ring define persistent Lightning and Poison area paths");
    expect(siphonPulse.slot == SkillSlot::Utility
            && siphonPulse.castType == SkillCastType::SelfCenteredArea
            && siphonPulse.damageType == DamageType::Poison
            && siphonPulse.healOnHit == Config::SiphonPulseHealOnHit
            && siphonPulse.healOnHit > 0,
        "Siphon Pulse defines a Poison area skill with hit-based recovery");
    expect(guardingPulse.slot == SkillSlot::Utility
            && guardingPulse.castType == SkillCastType::SelfCenteredArea
            && guardingPulse.baseDamage == 0
            && guardingPulse.selfDamageTakenMultiplier
                == Config::GuardingPulseDamageTakenMultiplier
            && guardingPulse.effectDuration == Config::GuardingPulseEffectDuration,
        "Guarding Pulse defines a temporary defensive Utility skill");
    expect(manaWard.slot == SkillSlot::Utility
            && manaWard.castType == SkillCastType::SelfCenteredArea
            && manaWard.baseDamage == 0
            && manaWard.wardManaRatio == Config::ManaWardManaRatio
            && manaWard.manaCost == Config::ManaWardManaCost,
        "Mana Ward defines a Mana-powered defensive Utility skill");
    const auto* bloodletting = SupportLibrary::find("Bloodletting");
    const auto* rupture = SupportLibrary::find("Rupture");
    expect(rendingVolley.slot == SkillSlot::Primary
            && rendingVolley.damageType == DamageType::Physical
            && rendingVolley.projectileCount == Config::RendingVolleyProjectileCount
            && rendingVolley.ailment.type == AilmentType::Bleed
            && bloodletting != nullptr
            && bloodletting->physicalPenetration == 20
            && SupportLibrary::supportsSkill(*bloodletting, rendingVolley),
        "Rending Volley and Bloodletting define the physical Bleed path");
    expect(crimsonSweep.slot == SkillSlot::Utility
            && crimsonSweep.castType == SkillCastType::SelfCenteredArea
            && crimsonSweep.damageType == DamageType::Physical
            && crimsonSweep.baseDamage == Config::CrimsonSweepDamage
            && crimsonSweep.radius == Config::CrimsonSweepRadius
            && crimsonSweep.ailment.type == AilmentType::Bleed
            && bloodletting != nullptr
            && SupportLibrary::supportsSkill(*bloodletting, crimsonSweep),
        "Crimson Sweep defines a close-range Physical Bleed alternative");
    expect(rupture != nullptr
            && rupture->ailmentDamageMultiplier > 1.0f
            && rupture->ailmentDurationMultiplier > 1.0f
            && rupture->bleedPenetration == 10
            && SupportLibrary::supportsSkill(*rupture, rendingVolley)
            && !SupportLibrary::supportsSkill(*rupture, SkillLibrary::flare()),
        "Rupture offers an alternative hit tradeoff for Physical Bleed skills");
    const auto* vitality = SupportLibrary::find("Vitality");
    expect(vitality != nullptr
            && SupportLibrary::supportsSkill(*vitality, siphonPulse)
            && !SupportLibrary::supportsSkill(*vitality, guardingPulse),
        "Vitality only supports skills with hit-based recovery");
    if (vitality != nullptr) {
        const int baseRecovery = skillHealOnHit(siphonPulse, nullptr);
        const int supportedRecovery = skillHealOnHit(siphonPulse, vitality);
        const auto leveledVitality = SkillProgression::supportAtLevel(*vitality, 3);
        expect(supportedRecovery == baseRecovery + vitality->healOnHitBonus
                && skillHealOnHit(siphonPulse, &leveledVitality) > supportedRecovery,
            "Vitality adds recovery and scales with Support gem level");
    }
    expect(std::abs(SkillLibrary::flare().manaCost - Config::FlareManaCost) < 0.0001f,
        "Flare exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::meteor().manaCost - Config::MeteorManaCost) < 0.0001f,
        "Meteor exposes its configured Mana cost");
    expect(SkillLibrary::meteor().delivery == SkillDeliveryType::DelayedArea
            && std::abs(SkillLibrary::meteor().castDelay - 0.55f) < 0.0001f,
        "Meteor exposes a data-driven delayed impact window");
    expect(SkillLibrary::meteor().groundHazard.isValid()
            && SkillLibrary::meteor().groundHazard.target == GroundHazardTarget::Enemies,
        "Meteor exposes a persistent enemy burning ground effect");
    expect(std::abs(SkillLibrary::frostBomb().manaCost - Config::FrostBombManaCost) < 0.0001f,
        "Frost Bomb exposes its configured Mana cost");
    expect(SkillLibrary::frostBomb().delivery == SkillDeliveryType::DelayedArea
            && SkillLibrary::frostBomb().castDelay > 0.0f,
        "Frost Bomb exposes a short delayed explosion window");
    expect(SkillLibrary::frostBomb().groundHazard.isValid()
            && SkillLibrary::frostBomb().groundHazard.target == GroundHazardTarget::Enemies,
        "Frost Bomb exposes a persistent enemy chillfield");
    expect(std::abs(SkillLibrary::nova().manaCost - Config::NovaManaCost) < 0.0001f,
        "Nova exposes its configured Mana cost");
    expect(std::abs(SkillLibrary::pulse().manaCost - Config::PulseManaCost) < 0.0001f,
        "Pulse exposes its configured Mana cost");
    expect(SkillLibrary::pulse().damageType == DamageType::Lightning
            && SkillLibrary::pulse().ailment.type == AilmentType::Shock
            && SkillLibrary::pulse().ailment.damageTakenMultiplier > 1.0f,
        "Pulse is a Lightning skill with a Shock payload");
    expect(std::abs(SkillLibrary::bladestorm().manaCost - Config::BladestormManaCost) < 0.0001f,
        "Bladestorm exposes its configured Mana cost");
    expect(SkillLibrary::bladestorm().delivery == SkillDeliveryType::RepeatingArea
            && SkillLibrary::bladestorm().repeatCount == Config::BladestormHitCount
            && std::abs(SkillLibrary::bladestorm().repeatInterval
                - Config::BladestormHitInterval) < 0.0001f,
        "Bladestorm exposes its data-driven repeated hit cadence");
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
    const SupportDefinition* contagion = SupportLibrary::find("Contagion");
    const SupportDefinition* barrage = SupportLibrary::find("Barrage");
    const SupportDefinition* concentration = SupportLibrary::find("Concentration");
    const SupportDefinition* echo = SupportLibrary::find("Echo");
    const SupportDefinition* pinpoint = SupportLibrary::find("Pinpoint");
    const SupportDefinition* emberFocus = SupportLibrary::find("Ember Focus");
    const SupportDefinition* glacialFocus = SupportLibrary::find("Glacial Focus");
    const SupportDefinition* stormFocus = SupportLibrary::find("Storm Focus");
    const SupportDefinition* venomFocus = SupportLibrary::find("Venom Focus");
    expect(pierce != nullptr && amplify != nullptr && quickcast != nullptr
            && volley != nullptr && trailblazer != nullptr
            && combustion != nullptr && deepChill != nullptr
            && barrage != nullptr && concentration != nullptr
            && echo != nullptr && pinpoint != nullptr && contagion != nullptr
            && emberFocus != nullptr && glacialFocus != nullptr
            && stormFocus != nullptr && venomFocus != nullptr,
        "existing and expanded supports exist in library");

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

    expect(SkillLibrary::arcBolt().damageType == DamageType::Lightning
            && SkillLibrary::flare().damageType == DamageType::Fire
            && SkillLibrary::frostBomb().damageType == DamageType::Cold,
        "elemental skills expose their data-driven damage types");
    Stats elementalStats;
    elementalStats.fireDamageMultiplier = 1.25f;
    elementalStats.coldDamageMultiplier = 1.35f;
    elementalStats.lightningDamageMultiplier = 1.45f;
    expect(skillDamage(SkillLibrary::flare(), elementalStats, nullptr)
            > skillDamage(SkillLibrary::flare(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::frostBomb(), elementalStats, nullptr)
                > skillDamage(SkillLibrary::frostBomb(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::arcBolt(), elementalStats, nullptr)
                > skillDamage(SkillLibrary::arcBolt(), Stats{}, nullptr),
        "elemental damage multipliers affect their matching skills");
    expect(damageAfterResistance(100, DamageType::Fire, 40, 0, 0) == 60,
        "Fire resistance mitigates Fire damage");
    expect(damageAfterResistance(100, DamageType::Cold, 0, 40, 0) == 60,
        "Cold resistance mitigates Cold damage");
    expect(damageAfterResistance(100, DamageType::Lightning, 0, 0, 40) == 60,
        "Lightning resistance mitigates Lightning damage");
    expect(damageAfterResistance(100, DamageType::Physical, 100, 100, 100) == 100,
        "elemental resistance does not mitigate Physical damage");
    expect(damageAfterResistance(100, DamageType::Physical, 0, 0, 0, 0, 35) == 65,
        "Physical resistance mitigates Physical damage when explicitly configured");
    expect(damageAfterResistance(100, DamageType::Fire, 100, 0, 0) == 0,
        "full elemental resistance prevents matching damage");
    expect(damageAfterResistance(100, DamageType::Fire, -25, 0, 0) == 125,
        "negative elemental resistance increases matching damage");

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

    const SkillDefinition arcBolt = SkillLibrary::arcBolt();
    const SkillDefinition shockwave = SkillLibrary::shockwave();
    const SkillDefinition splitArrow = SkillLibrary::splitArrow();
    const SkillDefinition aftershock = SkillLibrary::aftershock();
    expect(arcBolt.ailment.type == AilmentType::Shock
            && std::abs(arcBolt.ailment.damageTakenMultiplier - 1.20f) < 0.0001f,
        "Arc Bolt carries the base Shock effect");
    expect(arcBolt.slot == SkillSlot::Primary
            && arcBolt.castType == SkillCastType::Projectile
            && arcBolt.baseDamage == 3
            && std::abs(arcBolt.cooldown - 0.65f) < 0.0001f,
        "Arc Bolt exposes its fixed Projectile definition");
    expect(shockwave.slot == SkillSlot::Utility
            && shockwave.castType == SkillCastType::SelfCenteredArea
            && shockwave.baseDamage == 3
            && std::abs(shockwave.radius - 120.0f) < 0.0001f,
        "Shockwave exposes its fixed Area definition");
    expect(splitArrow.slot == SkillSlot::Primary
            && splitArrow.castType == SkillCastType::Projectile
            && skillProjectileCount(splitArrow, nullptr) == 5
            && aftershock.slot == SkillSlot::Utility
            && aftershock.castType == SkillCastType::SelfCenteredArea
            && skillRadius(aftershock, Stats{}, nullptr) == aftershock.radius,
        "new skills use the existing Projectile and Area combat paths");
    Stats areaSpecializationOnly;
    areaSpecializationOnly.areaDamageMultiplier = 1.60f;
    Stats projectileSpecializationOnly;
    projectileSpecializationOnly.projectileDamageMultiplier = 1.40f;
    expect(skillDamage(arcBolt, stats, nullptr) > skillDamage(arcBolt, Stats{}, nullptr)
            && skillDamage(arcBolt, areaSpecializationOnly, nullptr)
                == skillDamage(arcBolt, Stats{}, nullptr),
        "Arc Bolt uses Projectile specialization only");
    expect(skillDamage(shockwave, stats, nullptr) > skillDamage(shockwave, Stats{}, nullptr)
            && skillDamage(shockwave, projectileSpecializationOnly, nullptr)
                == skillDamage(shockwave, Stats{}, nullptr),
        "Shockwave uses Area specialization only");
    SkillDefinition barrageProbe = arcBolt;
    barrageProbe.baseDamage = 10;
    expect(skillProjectileCount(arcBolt, barrage) == arcBolt.projectileCount + 1
            && skillSpreadAngle(arcBolt, barrage) > arcBolt.spreadAngle
            && skillDamage(barrageProbe, Stats{}, barrage)
                < skillDamage(barrageProbe, Stats{}, nullptr),
        "Barrage adds Projectile spread and trades hit damage");
    expect(skillDamage(shockwave, Stats{}, concentration)
                > skillDamage(shockwave, Stats{}, nullptr)
            && skillRadius(shockwave, Stats{}, concentration) < shockwave.radius
            && skillCooldown(shockwave, Stats{}, concentration) > shockwave.cooldown,
        "Concentration trades Area radius for damage and cooldown");
    expect(skillRepeatCount(shockwave, echo) == 2
            && skillRepeatCount(arcBolt, echo) == 1
            && skillDamage(shockwave, Stats{}, echo) < skillDamage(shockwave, Stats{}, nullptr)
            && skillCooldown(shockwave, Stats{}, echo) > shockwave.cooldown,
        "Echo repeats Area skills while trading hit damage for cooldown");
    expect(skillDamage(arcBolt, Stats{}, pinpoint) > skillDamage(arcBolt, Stats{}, nullptr)
            && skillSpreadAngle(projectile, pinpoint) < projectile.spreadAngle
            && skillCooldown(arcBolt, Stats{}, pinpoint) > arcBolt.cooldown,
        "Pinpoint trades projectile spread and cooldown for hit damage");
    expect(std::abs(barrage->damageMultiplier - 0.88f) < 0.0001f
            && barrage->extraProjectileCount == 1
            && std::abs(barrage->extraSpreadAngle - 12.0f) < 0.0001f
            && std::abs(concentration->damageMultiplier - 1.22f) < 0.0001f
            && std::abs(concentration->radiusMultiplier - 0.78f) < 0.0001f
            && std::abs(concentration->cooldownMultiplier - 1.12f) < 0.0001f
            && echo->repeatCountBonus == 1
            && std::abs(pinpoint->damageMultiplier - 1.28f) < 0.0001f,
        "expanded Support definitions preserve their fixed values");
    expect(SupportLibrary::supportsSkill(*barrage, arcBolt)
            && !SupportLibrary::supportsSkill(*barrage, shockwave)
            && SupportLibrary::supportsSkill(*concentration, shockwave)
            && SupportLibrary::supportsSkill(*concentration, SkillLibrary::meteor())
            && SupportLibrary::supportsSkill(*echo, shockwave)
            && !SupportLibrary::supportsSkill(*echo, arcBolt)
            && SupportLibrary::supportsSkill(*pinpoint, arcBolt)
            && !SupportLibrary::supportsSkill(*pinpoint, shockwave)
            && !SupportLibrary::supportsSkill(*concentration, SkillLibrary::dash()),
        "expanded Supports expose only their intended CastType compatibility");
    expect(SupportLibrary::supportsSkill(*emberFocus, SkillLibrary::flare())
            && SupportLibrary::supportsSkill(*emberFocus, SkillLibrary::emberLance())
            && !SupportLibrary::supportsSkill(*emberFocus, SkillLibrary::frostBomb())
            && SupportLibrary::supportsSkill(*glacialFocus, SkillLibrary::frostBomb())
            && SupportLibrary::supportsSkill(*stormFocus, SkillLibrary::arcBolt())
            && SupportLibrary::supportsSkill(*venomFocus, SkillLibrary::toxicBurst()),
        "elemental Focus supports only their matching damage types");
    expect(skillDamage(SkillLibrary::emberLance(), Stats{}, emberFocus)
                > skillDamage(SkillLibrary::emberLance(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::frostBomb(), Stats{}, emberFocus)
                == skillDamage(SkillLibrary::frostBomb(), Stats{}, nullptr)
            && skillDamage(SkillLibrary::toxicBurst(), Stats{}, venomFocus)
                > skillDamage(SkillLibrary::toxicBurst(), Stats{}, nullptr),
        "elemental Focus supports change only matching skill damage");

    expect(skillPierceCount(nullptr) == 0, "no support => 0 pierce");
    expect(skillPierceCount(pierce) == 1, "Pierce support => 1 pierce");
    expect(skillPierceCount(amplify) == 0, "Amplify does not grant pierce");
    expect(skillProjectileCount(projectile, volley) == projectile.projectileCount + 2,
        "Volley adds two projectiles");
    const SupportList projectileSupports{pierce, volley};
    expect(skillPierceCount(projectileSupports) == pierce->pierceCount,
        "multi-link projectile math sums pierce effects");
    expect(skillProjectileCount(projectile, projectileSupports)
            == projectile.projectileCount + volley->extraProjectileCount,
        "multi-link projectile math sums projectile effects");
    const SupportList areaSupports{amplify, quickcast};
    expect(skillRadius(area, stats, areaSupports) > scaledRadius,
        "multi-link area math multiplies radius supports");
    expect(skillDamage(area, stats, areaSupports) < skillDamage(area, stats, amplify),
        "multi-link area math combines damage tradeoffs");
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

void testSkillPreviewSupportCompatibility() {
    section("Skill preview filters incompatible supports");

    SkillBar skillBar;
    const SkillDefinition meteor = SkillLibrary::meteor();
    const SkillDefinition frostBomb = SkillLibrary::frostBomb();
    const auto* combustion = SupportLibrary::find("Combustion");
    const auto* deepChill = SupportLibrary::find("Deep Chill");
    expect(combustion != nullptr && deepChill != nullptr,
        "preview compatibility fixture finds ailment supports");
    if (combustion == nullptr || deepChill == nullptr) {
        return;
    }

    expect(skillBar.assignSkill(SkillSlot::Secondary, meteor.name),
        "preview compatibility fixture assigns Meteor");
    expect(skillBar.assignSupport(SkillSlot::Secondary, combustion->name, 0),
        "preview compatibility fixture assigns Combustion to Meteor");
    const auto meteorSupports = skillBar.supportDefinitionsFor(meteor);
    expect(meteorSupports[0] != nullptr && meteorSupports[0]->name == combustion->name,
        "current skill preview retains its compatible Support");

    const auto frostSupports = skillBar.supportDefinitionsFor(frostBomb);
    expect(frostSupports[0] == nullptr,
        "candidate skill preview filters the current skill's incompatible Support");

    expect(skillBar.assignSkill(SkillSlot::Secondary, frostBomb.name),
        "preview compatibility fixture switches to Frost Bomb");
    expect(skillBar.supportAt(SkillSlot::Secondary, 0) == nullptr,
        "switching skills clears the incompatible current Support");
    expect(skillBar.assignSupport(SkillSlot::Secondary, deepChill->name, 0),
        "preview compatibility fixture assigns Deep Chill to Frost Bomb");
    const auto frostWithChill = skillBar.supportDefinitionsFor(frostBomb);
    expect(frostWithChill[0] != nullptr && frostWithChill[0]->name == deepChill->name,
        "candidate skill preview retains a compatible Support");
}

void testSkillBuildMathMatrix() {
    section("Skill build math matrix and temporary Shrine multiplier");

    const auto* pierce = SupportLibrary::find("Pierce");
    const auto* volley = SupportLibrary::find("Volley");
    const auto* amplify = SupportLibrary::find("Amplify");
    const auto* quickcast = SupportLibrary::find("Quickcast");
    expect(pierce != nullptr && volley != nullptr && amplify != nullptr && quickcast != nullptr,
        "build matrix fixture finds two Projectile and two Area Supports");
    if (pierce == nullptr || volley == nullptr || amplify == nullptr || quickcast == nullptr) {
        return;
    }

    Stats projectileStats;
    projectileStats.projectileDamageMultiplier = 2.40f;
    projectileStats.areaDamageMultiplier = 1.0f;
    projectileStats.areaRadiusMultiplier = 1.0f;
    projectileStats.attackSpeedMultiplier = 2.0f;
    const SupportList projectileSupports{pierce, volley};
    const int projectileDamage = skillDamage(
        SkillLibrary::spreadShot(), projectileStats, projectileSupports
    );
    const int projectileWithShrine = skillDamage(
        SkillLibrary::spreadShot(), projectileStats, projectileSupports,
        Config::ShrineDamageMultiplier
    );
    expect(projectileWithShrine > projectileDamage,
        "Shrine temporarily increases Projectile skill damage");
    Stats areaOnlyStats;
    areaOnlyStats.areaDamageMultiplier = 1.60f;
    expect(skillDamage(
            SkillLibrary::spreadShot(), areaOnlyStats, projectileSupports
        ) == skillDamage(SkillLibrary::spreadShot(), Stats{}, projectileSupports),
        "Area specialization does not affect Projectile damage");

    Stats areaStats;
    areaStats.projectileDamageMultiplier = 1.0f;
    areaStats.areaDamageMultiplier = 1.60f;
    areaStats.areaRadiusMultiplier = 1.25f;
    areaStats.attackSpeedMultiplier = 2.0f;
    const SupportList areaSupports{amplify, quickcast};
    const int areaDamage = skillDamage(SkillLibrary::meteor(), areaStats, areaSupports);
    const float areaRadius = skillRadius(SkillLibrary::meteor(), areaStats, areaSupports);
    expect(areaDamage == skillDamage(
            SkillLibrary::meteor(), areaStats, areaSupports, 1.0f
        ),
        "Area damage uses the same Support and specialization path without Shrine");
    expect(areaRadius > SkillLibrary::meteor().radius,
        "Area specialization and Amplify increase the real Area radius");
    Stats projectileOnlyStats;
    projectileOnlyStats.projectileDamageMultiplier = 1.40f;
    expect(skillDamage(
            SkillLibrary::meteor(), projectileOnlyStats, areaSupports
        ) == skillDamage(SkillLibrary::meteor(), Stats{}, areaSupports),
        "Projectile specialization does not affect Area damage");

    const float primaryBaseCooldown = skillCooldown(
        SkillLibrary::spreadShot(), Stats{}, projectileSupports
    );
    const float primaryFastCooldown = skillCooldown(
        SkillLibrary::spreadShot(), projectileStats, projectileSupports
    );
    expect(std::abs(primaryFastCooldown - primaryBaseCooldown / 2.0f) < 0.0001f,
        "Primary attack speed changes only the Primary cooldown");

    const float areaBaseCooldown = skillCooldown(
        SkillLibrary::meteor(), Stats{}, areaSupports
    );
    const float areaFastCooldown = skillCooldown(
        SkillLibrary::meteor(), areaStats, areaSupports
    );
    expect(std::abs(areaFastCooldown - areaBaseCooldown) < 0.0001f,
        "Secondary cooldown ignores Primary attack speed");

    const Stats beforeShrine = areaStats;
    skillDamage(SkillLibrary::meteor(), areaStats, areaSupports, Config::ShrineDamageMultiplier);
    expect(std::abs(areaStats.areaDamageMultiplier - beforeShrine.areaDamageMultiplier) < 0.0001f
            && std::abs(areaStats.areaRadiusMultiplier - beforeShrine.areaRadiusMultiplier) < 0.0001f,
        "Shrine damage calculation does not mutate permanent Stats");
}

// --- Ailments ---
void testSkillAilments() {
    section("Skill ailments and enemy lifecycle");

    const SkillDefinition flare = SkillLibrary::flare();
    const SkillDefinition meteor = SkillLibrary::meteor();
    const SkillDefinition frostBomb = SkillLibrary::frostBomb();
    const SkillDefinition toxicBurst = SkillLibrary::toxicBurst();
    const SkillDefinition crimsonSweep = SkillLibrary::crimsonSweep();
    const auto* toxicity = SupportLibrary::find("Toxicity");
    const auto* contagion = SupportLibrary::find("Contagion");
    expect(flare.ailment.type == AilmentType::Ignite, "Flare applies Ignite");
    expect(meteor.ailment.type == AilmentType::Ignite, "Meteor applies Ignite");
    expect(frostBomb.ailment.type == AilmentType::Chill, "Frost Bomb applies Chill");
    expect(toxicBurst.damageType == DamageType::Poison
            && toxicBurst.ailment.type == AilmentType::Poison,
        "Toxic Burst deals Poison damage and applies Poison");
    expect(toxicBurst.groundHazard.isValid()
            && toxicBurst.groundHazard.target == GroundHazardTarget::Enemies,
        "Toxic Burst leaves a persistent enemy poison mire");
    expect(ailmentTickDamage(meteor.ailment, 4) == 2,
        "Ignite tick damage derives from the scaled hit damage");
    expect(ailmentTickDamage(frostBomb.ailment, 4) == 0,
        "Chill does not create damage-over-time ticks");
    expect(ailmentTickDamage(toxicBurst.ailment, 4) == 2,
        "Poison tick damage derives from the scaled hit damage");
    const AilmentDefinition toxicityPoison = skillAilment(toxicBurst, toxicity);
    expect(toxicity != nullptr
            && toxicityPoison.damageMultiplier > toxicBurst.ailment.damageMultiplier
            && toxicityPoison.poisonPenetration == 20,
        "Toxicity increases Poison damage and penetration");
    const AilmentDefinition contagionPoison = skillAilment(toxicBurst, contagion);
    expect(contagion != nullptr
            && SupportLibrary::supportsSkill(*contagion, toxicBurst)
            && contagionPoison.poisonSpreadRadius == 120.0f
            && std::abs(contagionPoison.poisonSpreadMultiplier - 0.45f) < 0.0001f,
        "Contagion attaches a Poison death-spread payload");
    const auto* bloodletting = SupportLibrary::find("Bloodletting");
    const AilmentDefinition bloodlettingBleed = skillAilment(
        SkillLibrary::rendingVolley(), bloodletting
    );
    expect(bloodletting != nullptr
            && bloodlettingBleed.type == AilmentType::Bleed
            && bloodlettingBleed.damageMultiplier
                > SkillLibrary::rendingVolley().ailment.damageMultiplier
            && bloodlettingBleed.bleedPenetration == 20
            && skillPhysicalPenetration(bloodletting) == 20,
        "Bloodletting increases Bleed damage and penetration");
    const AilmentDefinition ruptureBleed = skillAilment(
        SkillLibrary::rendingVolley(), SupportLibrary::find("Rupture")
    );
    expect(ruptureBleed.type == AilmentType::Bleed
            && ruptureBleed.damageMultiplier
                > SkillLibrary::rendingVolley().ailment.damageMultiplier
            && ruptureBleed.duration > SkillLibrary::rendingVolley().ailment.duration
            && ruptureBleed.bleedPenetration == 10,
        "Rupture scales Bleed damage, duration, and penetration");
    const AilmentDefinition crimsonSweepBleed = skillAilment(
        crimsonSweep, SupportLibrary::find("Rupture")
    );
    expect(crimsonSweepBleed.type == AilmentType::Bleed
            && crimsonSweepBleed.duration > crimsonSweep.ailment.duration
            && crimsonSweepBleed.damageMultiplier > crimsonSweep.ailment.damageMultiplier,
        "Rupture supports the close-range Physical Bleed skill");
    Stats poisonStats;
    poisonStats.poisonDamageMultiplier = 1.50f;
    expect(skillDamage(toxicBurst, poisonStats, nullptr)
            > skillDamage(toxicBurst, Stats{}, nullptr),
        "Poison specialization increases Poison skill damage");
    Stats physicalStats;
    physicalStats.physicalDamageMultiplier = 1.50f;
    expect(skillDamage(SkillLibrary::rendingVolley(), physicalStats, nullptr)
            > skillDamage(SkillLibrary::rendingVolley(), Stats{}, nullptr),
        "Physical specialization increases Physical skill damage");

    Stats ailmentStats;
    ailmentStats.igniteDamageMultiplier = 1.25f;
    ailmentStats.igniteDurationMultiplier = 1.20f;
    ailmentStats.chillMagnitudeMultiplier = 1.30f;
    ailmentStats.chillDurationMultiplier = 1.40f;
    ailmentStats.shockMagnitudeMultiplier = 1.25f;
    ailmentStats.shockDurationMultiplier = 1.35f;
    ailmentStats.poisonDurationMultiplier = 1.50f;
    ailmentStats.bleedDamageMultiplier = 1.40f;
    ailmentStats.bleedDurationMultiplier = 1.60f;
    ailmentStats.bleedPenetration = 18;
    const AilmentDefinition scaledIgnite = scaleAilmentWithStats(
        meteor.ailment, ailmentStats
    );
    const AilmentDefinition scaledChill = scaleAilmentWithStats(
        frostBomb.ailment, ailmentStats
    );
    const AilmentDefinition scaledShock = scaleAilmentWithStats(
        SkillLibrary::pulse().ailment, ailmentStats
    );
    const AilmentDefinition scaledPoison = scaleAilmentWithStats(
        toxicBurst.ailment, ailmentStats
    );
    const AilmentDefinition scaledBleed = scaleAilmentWithStats(
        SkillLibrary::rendingVolley().ailment, ailmentStats
    );
    expect(scaledIgnite.damageMultiplier > meteor.ailment.damageMultiplier
            && scaledIgnite.duration > meteor.ailment.duration
            && scaledChill.speedMultiplier < frostBomb.ailment.speedMultiplier
            && scaledChill.duration > frostBomb.ailment.duration
            && scaledShock.damageTakenMultiplier
                > SkillLibrary::pulse().ailment.damageTakenMultiplier
            && scaledShock.duration > SkillLibrary::pulse().ailment.duration
            && scaledPoison.duration > toxicBurst.ailment.duration
            && scaledBleed.damageMultiplier
                > SkillLibrary::rendingVolley().ailment.damageMultiplier
            && scaledBleed.duration > SkillLibrary::rendingVolley().ailment.duration
            && scaledBleed.bleedPenetration
                == SkillLibrary::rendingVolley().ailment.bleedPenetration + 18,
        "elemental ailment stats scale damage, magnitude, and duration");

    Enemy enemy({0.0f, 0.0f}, 10, 1);
    enemy.applyIgnite(2, 2.0f);
    expect(enemy.isIgnited(), "Ignite is active after application");
    const AilmentTickResult firstTick = enemy.updateAilments(Config::AilmentTickInterval);
    expect(firstTick.type == AilmentType::Ignite
            && firstTick.damage == 2
            && firstTick.tickCount == 1
            && !firstTick.killed
            && enemy.hp() == 8,
        "Ignite returns its actual configured tick damage");

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

    enemy.applyShock(1.20f, 2.0f);
    expect(enemy.isShocked()
            && std::abs(enemy.damageTakenMultiplier() - 1.20f) < 0.0001f,
        "Shock increases damage taken while active");
    enemy.applyShock(1.10f, 3.0f);
    expect(std::abs(enemy.damageTakenMultiplier() - 1.20f) < 0.0001f,
        "weaker Shock does not overwrite a stronger Shock");
    enemy.updateAilments(3.1f);
    expect(!enemy.isShocked()
            && std::abs(enemy.damageTakenMultiplier() - 1.0f) < 0.0001f,
        "Shock expires and restores normal damage taken");

    Enemy overkillEnemy({0.0f, 0.0f}, 3, 1);
    overkillEnemy.applyIgnite(8, 2.0f);
    const AilmentTickResult overkillTick = overkillEnemy.updateAilments(
        Config::AilmentTickInterval
    );
    expect(overkillTick.damage == 3 && overkillTick.killed && overkillEnemy.hp() == 0,
        "Ignite tick damage clamps to remaining HP and reports the kill");
    const AilmentTickResult deadTick = overkillEnemy.updateAilments(2.0f);
    expect(deadTick.damage == 0 && deadTick.tickCount == 0,
        "dead enemies cannot receive another Ignite tick");
    expect(overkillEnemy.claimKillReward() && !overkillEnemy.claimKillReward(),
        "Ignite-killed enemy still exposes a one-time reward claim");

    Enemy poisonedEnemy({0.0f, 0.0f}, 50, 1);
    poisonedEnemy.applyPoison(2, 2.0f);
    poisonedEnemy.applyPoison(3, 2.0f);
    for (int stack = 2; stack < Config::MaxPoisonStacks; ++stack) {
        poisonedEnemy.applyPoison(4, 2.0f);
    }
    poisonedEnemy.applyPoison(100, 2.0f);
    const AilmentTickResult poisonTick = poisonedEnemy.updateAilments(
        Config::AilmentTickInterval
    );
    expect(poisonedEnemy.isPoisoned()
            && poisonedEnemy.poisonStacks() == Config::MaxPoisonStacks
            && poisonTick.type == AilmentType::Poison
            && poisonTick.damageFor(AilmentType::Poison) == 17
            && poisonedEnemy.hp() == 33,
        "Poison stacks cap at five applications and deal combined DoT");
    poisonedEnemy.updateAilments(2.0f);
    expect(!poisonedEnemy.isPoisoned() && poisonedEnemy.poisonStacks() == 0,
        "Poison expires and clears its stack state");

    Enemy bleedingEnemy({0.0f, 0.0f}, 50, 1);
    bleedingEnemy.applyBleed(2, 2.0f);
    bleedingEnemy.applyBleed(3, 2.0f);
    for (int stack = 2; stack < Config::MaxBleedStacks; ++stack) {
        bleedingEnemy.applyBleed(4, 2.0f);
    }
    bleedingEnemy.applyBleed(100, 2.0f);
    const AilmentTickResult bleedTick = bleedingEnemy.updateAilments(
        Config::AilmentTickInterval
    );
    expect(bleedingEnemy.isBleeding()
            && bleedingEnemy.bleedStacks() == Config::MaxBleedStacks
            && bleedTick.type == AilmentType::Bleed
            && bleedTick.damageFor(AilmentType::Bleed) == 17
            && bleedingEnemy.hp() == 33,
        "Bleed stacks cap at five applications and deal combined DoT");
    bleedingEnemy.updateAilments(2.0f);
    expect(!bleedingEnemy.isBleeding() && bleedingEnemy.bleedStacks() == 0,
        "Bleed expires and clears its stack state");

    Enemy contagionEnemy({0.0f, 0.0f}, 50, 1);
    contagionEnemy.applyPoison(6, 3.0f, Config::MaxPoisonStacks, 120.0f, 0.45f);
    expect(contagionEnemy.poisonDamagePerTick() == 6
            && contagionEnemy.poisonTimeRemaining() > 0.0f
            && contagionEnemy.poisonSpreadRadius() == 120.0f
            && std::abs(contagionEnemy.poisonSpreadMultiplier() - 0.45f) < 0.0001f,
        "Poisoned enemies retain Contagion spread data for death handling");

    Enemy chillOnlyEnemy({0.0f, 0.0f}, 10, 1);
    chillOnlyEnemy.applyChill(0.55f, 2.0f);
    const AilmentTickResult chillTick = chillOnlyEnemy.updateAilments(
        Config::AilmentTickInterval
    );
    expect(chillTick.type == AilmentType::None && chillTick.damage == 0,
        "Chill never produces damage feedback");

    Enemy expiringEnemy({0.0f, 0.0f}, 10, 1);
    expiringEnemy.applyIgnite(2, 1.0f);
    expiringEnemy.updateAilments(1.0f);
    const AilmentTickResult expiredTick = expiringEnemy.updateAilments(1.0f);
    expect(!expiringEnemy.isIgnited() && expiredTick.damage == 0,
        "Ignite expires without producing ticks after its duration");
}

void testPlayerAilments() {
    section("Player elemental ailments and transient lifecycle");

    Player ignited;
    ignited.applyIgnite(1, 2.0f);
    expect(ignited.isIgnited(), "Player Ignite is active after application");
    const int hpBeforeIgnite = ignited.hp();
    const AilmentTickResult igniteTick = ignited.updateAilments(
        Config::AilmentTickInterval
    );
    expect(igniteTick.type == AilmentType::Ignite
            && igniteTick.damage == 1
            && igniteTick.tickCount == 1
            && ignited.hp() == hpBeforeIgnite - 1,
        "Player Ignite deals one configured tick of damage");
    ignited.updateAilments(2.0f);
    expect(!ignited.isIgnited(), "Player Ignite expires after its duration");

    Player chilled;
    const float normalSpeed = chilled.moveSpeed();
    chilled.applyChill(0.55f, 1.5f);
    expect(chilled.isChilled()
            && chilled.moveSpeed() < normalSpeed
            && std::abs(chilled.chillSpeedMultiplier() - 0.55f) < 0.0001f,
        "Player Chill reduces movement speed");
    chilled.updateAilments(1.5f);
    expect(!chilled.isChilled()
            && std::abs(chilled.moveSpeed() - normalSpeed) < 0.0001f,
        "Player Chill restores movement speed after expiry");

    Player shocked;
    shocked.applyShock(1.20f, 2.0f);
    expect(shocked.isShocked()
            && shocked.takeDamage(1) == 2,
        "Player Shock amplifies subsequent incoming damage");
    shocked.updateAilments(2.0f);
    expect(!shocked.isShocked()
            && std::abs(shocked.damageTakenMultiplier() - 1.0f) < 0.0001f,
        "Player Shock restores normal damage taken after expiry");

    Player poisoned;
    poisoned.applyPoison(2, 2.0f);
    poisoned.applyPoison(3, 2.0f);
    const int hpBeforePoison = poisoned.hp();
    const AilmentTickResult playerPoisonTick = poisoned.updateAilments(
        Config::AilmentTickInterval
    );
    expect(poisoned.hasAilment()
            && poisoned.isPoisoned()
            && poisoned.poisonStacks() == 2
            && playerPoisonTick.damageFor(AilmentType::Poison) == 5
            && poisoned.hp() == hpBeforePoison - 5,
        "Player Poison applies stacked DoT and reports its damage type");
    poisoned.updateAilments(2.0f);
    expect(!poisoned.isPoisoned() && !poisoned.hasAilment(),
        "Player Poison expires with the other transient ailments");

    Player bleeding;
    bleeding.applyBleed(2, 2.0f);
    bleeding.applyBleed(3, 2.0f);
    const int hpBeforeBleed = bleeding.hp();
    const AilmentTickResult playerBleedTick = bleeding.updateAilments(
        Config::AilmentTickInterval
    );
    expect(bleeding.hasAilment()
            && bleeding.isBleeding()
            && bleeding.bleedStacks() == 2
            && playerBleedTick.damageFor(AilmentType::Bleed) == 5
            && bleeding.hp() == hpBeforeBleed - 5,
        "Player Bleed applies stacked DoT and reports its damage type");
    bleeding.updateAilments(2.0f);
    expect(!bleeding.isBleeding() && !bleeding.hasAilment(),
        "Player Bleed expires with the other transient ailments");

    Player restored;
    restored.applyIgnite(1, 2.0f);
    restored.applyChill(0.60f, 2.0f);
    restored.applyShock(1.15f, 2.0f);
    const PlayerSaveState saved = restored.saveState();
    expect(restored.restoreState(saved, {Config::MapWidth, Config::MapHeight}),
        "Player restore accepts a state without transient ailments");
    expect(!restored.isIgnited() && !restored.isChilled() && !restored.isShocked(),
        "Player restore clears transient ailments");
}

// --- Ailment resistances and penetration ---
void testAilmentResistances() {
    section("Ailment resistances and penetration");

    const auto& normal = EnemyLibrary::forType(EnemyType::Normal);
    const auto& ranged = EnemyLibrary::forType(EnemyType::Ranged);
    const auto& elite = EnemyLibrary::forType(EnemyType::Elite);
    const auto& charger = EnemyLibrary::forType(EnemyType::Charger);
    const auto& warden = EnemyLibrary::forType(EnemyType::Warden);
    const auto& summoner = EnemyLibrary::forType(EnemyType::Summoner);
    expect(normal.igniteResistance == 0 && normal.chillResistance == 0,
        "Normal enemies have no ailment resistance");
    expect(ranged.igniteResistance == 10 && ranged.chillResistance == 10,
        "Ranged enemies use the low ailment resistance baseline");
    expect(ranged.fireResistance == 10 && ranged.coldResistance == 10
            && ranged.lightningResistance == 0 && ranged.poisonResistance == 10,
        "Ranged enemies expose separate direct elemental resistance data");
    expect(ranged.projectileDamageType == DamageType::Lightning
            && elite.contactDamageType == DamageType::Fire
            && charger.contactDamageType == DamageType::Fire
            && warden.contactDamageType == DamageType::Cold,
        "enemy attack definitions expose data-driven elemental damage types");
    expect(elite.igniteResistance == 15 && elite.chillResistance == 15,
        "Elite enemies use the elevated ailment resistance baseline");
    expect(charger.igniteResistance == 15 && charger.chillResistance == 10,
        "Charger enemies use the data-driven ailment resistance baseline");
    expect(charger.physicalResistance == 15
            && damageAfterResistance(100, DamageType::Physical, 0, 0, 0, 0, 15) == 85,
        "Charger enemies expose and apply a Physical resistance baseline");
    expect(warden.name == "Warden"
            && warden.igniteResistance == 25
            && warden.chillResistance == 20,
        "Warden exposes a distinct defensive resistance profile");
    Enemy wardenEnemy({400.0f, 400.0f}, 10, 2, EnemyType::Warden);
    expect(wardenEnemy.isWarden() && wardenEnemy.isElite()
            && !wardenEnemy.isRanged() && !wardenEnemy.isCharger(),
        "Warden exposes a defensive melee enemy role");
    expect(summoner.name == "Hexbinder"
            && summoner.attackStyle == EnemyAttackStyle::Summon
            && summoner.summonType == EnemyType::Normal
            && summoner.summonCount == 2,
        "Hexbinder exposes a data-driven summon role");

    const auto& bosses = BossLibrary::all();
    expect(bosses[0].igniteResistance == 35 && bosses[0].chillResistance == 20,
        "Brimstone exposes its Ignite-heavy resistance profile");
    expect(bosses[0].fireResistance == 35 && bosses[0].coldResistance == 20
            && bosses[0].lightningResistance == 35,
        "Brimstone exposes its direct elemental resistance profile");
    expect(bosses[1].igniteResistance == 20 && bosses[1].chillResistance == 35,
        "Storm exposes its Chill-heavy resistance profile");
    expect(bosses[2].igniteResistance == 30 && bosses[2].chillResistance == 30,
        "Brood exposes its balanced resistance profile");
    expect(bosses[2].poisonResistance == 45
            && damageAfterResistance(100, DamageType::Poison, 0, 0, 0, 45) == 55,
        "Brood exposes Poison resistance and mitigates Poison damage");
    expect(bosses[0].bleedResistance > 0
            && bosses[2].bleedResistance > bosses[0].bleedResistance,
        "Bosses expose distinct Bleed resistance profiles");
    expect(bosses[3].name == "Frostbound Warden"
            && bosses[3].chillResistance == 45
            && bosses[3].coldResistance == 50,
        "Frostbound Warden exposes a Cold-heavy resistance profile");
    for (const auto& boss : bosses) {
        expect(boss.igniteResistance >= 0 && boss.igniteResistance <= 100
                && boss.chillResistance >= 0 && boss.chillResistance <= 100
                && boss.bleedResistance >= 0 && boss.bleedResistance <= 100,
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
    expect(ailmentTickDamageAfterResistance(
            ailmentTickDamage(SkillLibrary::rendingVolley().ailment, 4),
                50,
                20
            ) == 1,
        "Bleed tick damage uses the same resistance and penetration path");

    expect(std::abs(damageTakenMultiplierAfterResistance(1.20f, 0, 0) - 1.20f) < 0.0001f
            && std::abs(damageTakenMultiplierAfterResistance(1.20f, 50, 0) - 1.10f) < 0.0001f
            && std::abs(damageTakenMultiplierAfterResistance(1.20f, 0, 20) - 1.20f) < 0.0001f,
        "Shock effect scales with resistance and penetration");

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

// --- Boss elemental skill definitions ---
void testBossElementalSkills() {
    section("Boss elemental skills and hazards");

    const auto& brimstone = BossLibrary::forMapLevel(1);
    const auto magmaIt = std::find_if(
        brimstone.skills.begin(), brimstone.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Magma Slam"; }
    );
    expect(magmaIt != brimstone.skills.end(), "Brimstone exposes its elemental slam");
    if (magmaIt != brimstone.skills.end()) {
        expect(magmaIt->damageType == DamageType::Fire
                && magmaIt->ailment.type == AilmentType::Ignite,
            "Magma Slam deals Fire damage and applies Ignite");
        expect(magmaIt->groundHazard.damageType == DamageType::Fire
                && magmaIt->groundHazard.ailment.type == AilmentType::Ignite,
            "Magma Pool preserves the same Fire/Ignite identity");
    }

    const auto& storm = BossLibrary::forMapLevel(2);
    const auto stormProjectileIt = std::find_if(
        storm.skills.begin(), storm.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Lightning Spear"; }
    );
    const auto stormDashIt = std::find_if(
        storm.skills.begin(), storm.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Tempest Rush"; }
    );
    expect(stormProjectileIt != storm.skills.end() && stormDashIt != storm.skills.end(),
        "Storm Herald exposes its projectile and dash skills");
    if (stormProjectileIt != storm.skills.end() && stormDashIt != storm.skills.end()) {
        expect(stormProjectileIt->damageType == DamageType::Lightning
                && stormProjectileIt->ailment.type == AilmentType::Shock,
            "Lightning Spear deals Lightning damage and applies Shock");
        expect(stormDashIt->damageType == DamageType::Lightning
                && stormDashIt->ailment.type == AilmentType::Shock,
            "Tempest Rush deals Lightning damage and applies Shock");
    }

    const auto& brood = BossLibrary::forMapLevel(3);
    const auto broodProjectileIt = std::find_if(
        brood.skills.begin(), brood.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Acid Spray"; }
    );
    expect(broodProjectileIt != brood.skills.end(), "Brood Matriarch exposes Acid Spray");
    if (broodProjectileIt != brood.skills.end()) {
        expect(broodProjectileIt->damageType == DamageType::Poison
                && broodProjectileIt->ailment.type == AilmentType::Poison,
            "Acid Spray uses the Poison damage model");
    }

    const auto& frost = BossLibrary::forMapLevel(4);
    const auto frostNovaIt = std::find_if(
        frost.skills.begin(), frost.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Frost Nova"; }
    );
    const auto frostSummonIt = std::find_if(
        frost.skills.begin(), frost.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Call Frostbound Wardens"; }
    );
    expect(frost.name == "Frostbound Warden"
            && frostNovaIt != frost.skills.end()
            && frostSummonIt != frost.skills.end(),
        "Frostbound Warden exposes its Cold arena pattern");
    if (frostNovaIt != frost.skills.end()) {
        expect(frostNovaIt->damageType == DamageType::Cold
                && frostNovaIt->ailment.type == AilmentType::Chill
                && frostNovaIt->groundHazard.damageType == DamageType::Cold,
            "Frost Nova carries Cold, Chill and a Frozen Ground hazard");
    }

    expect(storm.enrageHazard.damageType == DamageType::Lightning
            && storm.enrageHazard.ailment.type == AilmentType::Shock,
        "Storm enrage hazard carries Lightning and Shock");
    expect(brood.enrageHazard.damageType == DamageType::Poison
            && brood.enrageHazard.ailment.type == AilmentType::Poison,
        "Brood enrage hazard carries Poison");
    expect(frost.enrageHazard.damageType == DamageType::Cold
            && frost.enrageHazard.ailment.type == AilmentType::Chill,
        "Frost enrage hazard carries Cold and Chill");

    const auto& archive = BossLibrary::forMapLevel(5);
    const auto archiveProjectileIt = std::find_if(
        archive.skills.begin(), archive.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Tidal Quill"; }
    );
    const auto archiveSummonIt = std::find_if(
        archive.skills.begin(), archive.skills.end(),
        [](const BossSkillDefinition& skill) {
            return skill.name == "Call Drowned Wardens";
        }
    );
    expect(archive.name == "Tidebound Archivist",
        "Tidebound Archivist keeps its data-driven name");
    expect(archive.lootTheme == BossLootTheme::Archive,
        "Tidebound Archivist uses the Archive loot theme");
    expect(archiveProjectileIt != archive.skills.end()
            && archiveSummonIt != archive.skills.end(),
        "Tidebound Archivist exposes projectile and summon skills");
    if (archiveProjectileIt != archive.skills.end()) {
        expect(archiveProjectileIt->damageType == DamageType::Cold
                && archiveProjectileIt->ailment.type == AilmentType::Chill
                && archiveProjectileIt->projectileCount == 5
                && archiveProjectileIt->spreadAngle == 42.0f,
            "Tidal Quill carries its Cold fan-projectile configuration");
    }

    const auto& reliquary = BossLibrary::forMapLevel(6);
    const auto reliquaryAoeIt = std::find_if(
        reliquary.skills.begin(), reliquary.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Shardfall"; }
    );
    const auto reliquaryDashIt = std::find_if(
        reliquary.skills.begin(), reliquary.skills.end(),
        [](const BossSkillDefinition& skill) {
            return skill.name == "Blackglass Charge";
        }
    );
    expect(reliquary.name == "Obsidian Tyrant"
            && reliquary.lootTheme == BossLootTheme::Obsidian
            && reliquaryAoeIt != reliquary.skills.end()
            && reliquaryDashIt != reliquary.skills.end(),
        "Obsidian Tyrant exposes the Obsidian Reliquary boss kit");
    if (reliquaryAoeIt != reliquary.skills.end()) {
        expect(reliquaryAoeIt->damageType == DamageType::Fire
                && reliquaryAoeIt->ailment.type == AilmentType::Ignite
                && reliquaryAoeIt->groundHazard.damageType == DamageType::Fire,
            "Shardfall carries its Fire, Ignite and burning-ground configuration");
    }

    const auto& sable = BossLibrary::forMapLevel(8);
    const auto sableBurstIt = std::find_if(
        sable.skills.begin(), sable.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Marrow Burst"; }
    );
    const auto sableSummonIt = std::find_if(
        sable.skills.begin(), sable.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Call Gravebloom"; }
    );
    expect(sable.name == "Gravebloom Sovereign"
            && sable.lootTheme == BossLootTheme::Sable
            && sable.guaranteedDrops == 2
            && sableBurstIt != sable.skills.end()
            && sableSummonIt != sable.skills.end(),
        "Gravebloom Sovereign exposes the Sable Necropolis boss kit");
    if (sableBurstIt != sable.skills.end()) {
        expect(sableBurstIt->damageType == DamageType::Poison
                && sableBurstIt->ailment.type == AilmentType::Poison
                && sableBurstIt->groundHazard.damageType == DamageType::Poison,
            "Marrow Burst carries its Poison and lingering hazard identity");
    }
}

void testWardenProtectionMath() {
    section("Warden protection math");

    expect(wardenProtectedDamage(10, true, 0.70f) == 7,
        "Warden aura reduces incoming damage to the configured multiplier");
    expect(wardenProtectedDamage(10, false, 0.70f) == 10,
        "unprotected enemies take full damage");
    expect(wardenProtectedDamage(1, true, 0.0f) == 1,
        "Warden protection never reduces a positive hit below one");
    expect(wardenProtectedDamage(-5, true, 0.70f) == 0,
        "non-positive damage remains harmless");
}

void testSummonerStateMachine() {
    section("Summoner state machine");

    MapInstance map;
    Enemy summoner({400.0f, 400.0f}, 30, 2, EnemyType::Summoner);
    expect(summoner.isSummoner() && !summoner.isRanged() && !summoner.isElite(),
        "Hexbinder is a support enemy rather than a ranged or elite enemy");

    summoner.update(0.1f, {700.0f, 400.0f}, map);
    expect(summoner.isAttackWindingUp(),
        "Hexbinder begins a summon windup inside its casting range");
    summoner.update(0.6f, {700.0f, 400.0f}, map);
    expect(summoner.consumeAttack(),
        "Hexbinder exposes a completed summon cast through consumeAttack");

    Enemy summoned({450.0f, 400.0f}, 10, 1, EnemyType::Normal,
        EliteModifier::None, -1, true);
    expect(summoned.isSummoned() && !summoned.isElite(),
        "summoned minions carry an explicit summoned marker");
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

    Item gatedWeapon;
    gatedWeapon.baseId = "weapon.warhammer";
    gatedWeapon.slot = EquipmentSlot::Weapon;
    gatedWeapon.rarity = Rarity::Magic;
    expect(player.requiredLevelForItem(gatedWeapon) == std::optional<int>(3),
        "Player resolves the Item Base required level");
    expect(!player.canEquipItem(gatedWeapon),
        "level-one Player rejects a level-three Item Base");
    while (player.level() < 3) {
        player.gainExp(player.expToNextLevel());
    }
    expect(player.canEquipItem(gatedWeapon),
        "Player accepts the gated Item Base at its required level");
    gatedWeapon.baseId = "missing.base";
    expect(!player.canEquipItem(gatedWeapon),
        "Player rejects an unknown Item Base");
}

// --- Loot generation ---
void testLootGeneration() {
    section("LootGenerator rarity/slot/affixes");

    RandomService random(42);
    LootGenerator gen;

    bool sawAffix = false;
    bool sawValidSlot = true;
    bool sawValidRarity = true;
    bool sawName = true;

    for (int i = 0; i < 24; ++i) {
        Item item = gen.generate(3, random);
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
    expect(boss.rarity == Rarity::Unique, "boss relic is Unique");
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
    expect(LootGenerator::rarityForRoll(1, 15, 1.0f) == Rarity::Magic
            && LootGenerator::rarityForRoll(1, 15, 1.5f) == Rarity::Rare,
        "item rarity multiplier upgrades a deterministic Magic roll to Rare");
    RandomService normalLootRandom(712);
    RandomService rarityLootRandom(712);
    int normalRareCount = 0;
    int rarityRareCount = 0;
    for (int index = 0; index < 64; ++index) {
        normalRareCount += gen.generate(1, normalLootRandom).rarity == Rarity::Rare;
        rarityRareCount += gen.generate(1, rarityLootRandom, 2.0f).rarity == Rarity::Rare;
    }
    expect(rarityRareCount > normalRareCount,
        "rarity multiplier increases actual generated Rare item frequency");
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
        std::set<int> normalBuildThemes;
        for (const auto& base : bases) {
            if (base.kind == ItemBaseKind::Normal && base.slot == slot) {
                ++normalBaseCount;
                normalBuildThemes.insert(static_cast<int>(base.buildTheme));
            }
        }
        expect(normalBaseCount >= 3,
            std::string(slotName(slot)) + " has at least three normal base types");
        expect(normalBuildThemes.size() >= 2,
            std::string(slotName(slot)) + " offers at least two build themes");
    }

    const auto requiredLevel = [&bases](const std::string& id) {
        const auto it = std::find_if(bases.begin(), bases.end(),
            [&id](const ItemBaseDefinition& base) { return base.id == id; });
        return it == bases.end() ? -1 : it->requiredLevel;
    };
    bool allRequirementsValid = true;
    for (const auto& base : bases) {
        allRequirementsValid = allRequirementsValid && base.requiredLevel >= 1;
    }
    expect(allRequirementsValid, "every Item Base has a positive level requirement");
    expect(requiredLevel("weapon.rustbound-blade") == 1
            && requiredLevel("armor.iron-vest") == 1
            && requiredLevel("weapon.warhammer") == 3
            && requiredLevel("ring.scavenger-loop") == 3,
        "starter and high-tier Item Base requirements are data-driven");

    const auto* hunterBow = ItemBaseLibrary::find("weapon.hunter-bow");
    const auto* warhammer = ItemBaseLibrary::find("weapon.warhammer");
    const auto* windweave = ItemBaseLibrary::find("armor.windweave");
    const auto* aetherStaff = ItemBaseLibrary::find("weapon.aether-staff");
    const auto* sageweave = ItemBaseLibrary::find("armor.sageweave");
    const auto* aetherLoop = ItemBaseLibrary::find("ring.aether-loop");
    const auto* sageCodex = ItemBaseLibrary::find("amulet.sage-codex");
    const auto* brimstoneBase = ItemBaseLibrary::find("boss.brimstone-brand");
    const auto* stormBase = ItemBaseLibrary::find("boss.storm-signet");
    const auto* broodBase = ItemBaseLibrary::find("boss.brood-talisman");
    const auto* frostBase = ItemBaseLibrary::find("boss.frostbound-loop");
    expect(hunterBow != nullptr && hunterBow->buildTheme == ItemBuildTheme::Projectile
            && hunterBow->implicitStats.projectileDamageMultiplier > 1.0f,
        "Projectile Item Base theme carries projectile implicit scaling");
    expect(warhammer != nullptr && warhammer->buildTheme == ItemBuildTheme::Area
            && warhammer->implicitStats.areaDamageMultiplier > 1.0f,
        "Area Item Base theme carries area implicit scaling");
    expect(windweave != nullptr && windweave->buildTheme == ItemBuildTheme::Loot
            && windweave->implicitStats.itemQuantityMultiplier > 1.0f,
        "Loot Item Base theme carries item quantity implicit scaling");
    expect(aetherStaff != nullptr && aetherStaff->buildTheme == ItemBuildTheme::Mana
            && aetherStaff->implicitStats.maxManaMultiplier > 1.0f
            && aetherStaff->implicitStats.skillCostMultiplier < 1.0f,
        "Mana weapon Base trades damage and attack speed for lower skill cost");
    expect(sageweave != nullptr && sageweave->buildTheme == ItemBuildTheme::Mana
            && sageweave->implicitStats.maxManaMultiplier > 1.0f
            && sageweave->implicitStats.manaRegenMultiplier > 1.0f,
        "Mana armor Base supplies maximum Mana and regeneration");
    expect(aetherLoop != nullptr && sageCodex != nullptr
            && aetherLoop->implicitStats.maxManaMultiplier > 1.0f
            && sageCodex->implicitStats.skillCostMultiplier < aetherLoop->implicitStats.skillCostMultiplier,
        "Mana ring and amulet Bases offer different resource tradeoffs");
    expect(brimstoneBase != nullptr && brimstoneBase->buildTheme == ItemBuildTheme::Fire,
        "Brimstone relic Base is tagged for a Fire build");
    expect(stormBase != nullptr && stormBase->buildTheme == ItemBuildTheme::Lightning,
        "Storm relic Base is tagged for a Lightning build");
    expect(broodBase != nullptr && broodBase->buildTheme == ItemBuildTheme::Poison
            && broodBase->implicitStats.poisonDamageMultiplier > 1.0f,
        "Brood relic Base carries Poison build identity and implicit scaling");
    expect(frostBase != nullptr && frostBase->buildTheme == ItemBuildTheme::Cold
            && frostBase->implicitStats.coldDamageMultiplier > 1.0f,
        "Frost relic Base carries Cold build identity and implicit scaling");

    RandomService random(17);
    LootGenerator generator;
    for (int roll = 0; roll < 16; ++roll) {
        const Item item = generator.generate(3, random);
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

    bool lowLevelCanDropHigherRequirementBase = false;
    for (int seed = 0; seed < 256 && !lowLevelCanDropHigherRequirementBase; ++seed) {
        RandomService lowLevelRandom(seed);
        const Item item = generator.generate(1, lowLevelRandom);
        const auto* base = ItemBaseLibrary::find(item.baseId);
        lowLevelCanDropHigherRequirementBase = base != nullptr && base->requiredLevel > 1;
    }
    expect(lowLevelCanDropHigherRequirementBase,
        "low-level maps can drop higher-requirement bases for meaningful upgrades");

    LootBias manaBaseBias;
    manaBaseBias.baseTheme = ItemBuildTheme::Mana;
    manaBaseBias.baseThemeWeightMultiplier = 4.0f;
    RandomService unweightedBaseRandom(8011);
    RandomService manaBaseRandom(8011);
    int unweightedManaBases = 0;
    int weightedManaBases = 0;
    for (int roll = 0; roll < 128; ++roll) {
        const Item unweighted = generator.generate(3, unweightedBaseRandom);
        const Item weighted = generator.generate(3, manaBaseRandom, 1.0f, manaBaseBias);
        const auto* unweightedBase = ItemBaseLibrary::find(unweighted.baseId);
        const auto* weightedBase = ItemBaseLibrary::find(weighted.baseId);
        unweightedManaBases += unweightedBase != nullptr
            && unweightedBase->buildTheme == ItemBuildTheme::Mana;
        weightedManaBases += weightedBase != nullptr
            && weightedBase->buildTheme == ItemBuildTheme::Mana;
    }
    expect(weightedManaBases > unweightedManaBases,
        "loot base bias increases the real frequency of its build theme");

    const auto hasManaAffix = [](const Item& item) {
        return std::any_of(item.affixes.begin(), item.affixes.end(), [](const ItemAffix& affix) {
            return affix.stats.maxManaMultiplier > 1.0f
                || affix.stats.manaRegenMultiplier > 1.0f;
        });
    };
    RandomService unweightedManaAffixRandom(8012);
    RandomService weightedManaAffixRandom(8012);
    int unweightedManaAffixBases = 0;
    int weightedManaAffixBases = 0;
    int unweightedManaAffixes = 0;
    int weightedManaAffixes = 0;
    for (int roll = 0; roll < 512; ++roll) {
        const Item unweighted = generator.generate(3, unweightedManaAffixRandom);
        const Item weighted = generator.generate(3, weightedManaAffixRandom, 1.0f, manaBaseBias);
        const auto* unweightedBase = ItemBaseLibrary::find(unweighted.baseId);
        const auto* weightedBase = ItemBaseLibrary::find(weighted.baseId);
        if (unweightedBase != nullptr && unweightedBase->buildTheme == ItemBuildTheme::Mana) {
            ++unweightedManaAffixBases;
            unweightedManaAffixes += hasManaAffix(unweighted);
        }
        if (weightedBase != nullptr && weightedBase->buildTheme == ItemBuildTheme::Mana) {
            ++weightedManaAffixBases;
            weightedManaAffixes += hasManaAffix(weighted);
        }
    }
    expect(weightedManaAffixBases > 0 && unweightedManaAffixBases > 0,
        "Mana theme regression has comparable generated base samples");
    expect(weightedManaAffixes * unweightedManaAffixBases
            > unweightedManaAffixes * weightedManaAffixBases,
        "selected Mana bases softly prefer Mana affixes");

    const std::array<std::pair<BossLootTheme, std::string>, 9> bossThemes{{
        {BossLootTheme::Brimstone, "boss.brimstone-brand"},
        {BossLootTheme::Storm, "boss.storm-signet"},
        {BossLootTheme::Brood, "boss.brood-talisman"},
        {BossLootTheme::Frost, "boss.frostbound-loop"},
        {BossLootTheme::Archive, "boss.tidebound-ledger"},
        {BossLootTheme::Obsidian, "boss.obsidian-crown"},
        {BossLootTheme::Aether, "boss.aether-orb"},
        {BossLootTheme::Sable, "boss.sable-venom"},
        {BossLootTheme::Bloodletting, "boss.gorebound-cleaver"},
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

    expect(LootGenerator::bossRelicVariantForMapLevel(1) == 0
            && LootGenerator::bossRelicVariantForMapLevel(4) == 0
            && LootGenerator::bossRelicVariantForMapLevel(5) == 1
            && LootGenerator::bossRelicVariantForMapLevel(8) == 1
            && LootGenerator::bossRelicVariantForMapLevel(9) == 0,
        "Boss relic variants rotate once per four-map boss cycle");

    const std::array<std::tuple<BossLootTheme, ItemBaseTheme, std::string>, 9> alternateBossThemes{{
        {BossLootTheme::Brimstone, ItemBaseTheme::Brimstone, "boss.ashen-crucible"},
        {BossLootTheme::Storm, ItemBaseTheme::Storm, "boss.tempest-bow"},
        {BossLootTheme::Brood, ItemBaseTheme::Brood, "boss.broodscale-band"},
        {BossLootTheme::Frost, ItemBaseTheme::Frost, "boss.winterheart-pendant"},
        {BossLootTheme::Archive, ItemBaseTheme::Archive, "boss.drowned-compass"},
        {BossLootTheme::Obsidian, ItemBaseTheme::Obsidian, "boss.blackglass-heart"},
        {BossLootTheme::Aether, ItemBaseTheme::Aether, "boss.null-crown"},
        {BossLootTheme::Sable, ItemBaseTheme::Sable, "boss.gravebloom-heart"},
        {BossLootTheme::Bloodletting, ItemBaseTheme::Bloodletting, "boss.hemorrhage-signet"},
    }};
    for (const auto& [theme, expectedTheme, expectedBaseId] : alternateBossThemes) {
        const Item item = generator.generateBossReward(5, theme, 1);
        expect(item.rarity == Rarity::Unique && item.baseId == expectedBaseId,
            "alternate Boss relic uses its theme-specific chase base");
        const auto* base = ItemBaseLibrary::find(item.baseId);
        expect(base != nullptr
                && base->kind == ItemBaseKind::BossRelic
                && base->variant == 1
                && base->theme == expectedTheme,
            "alternate Boss relic base keeps a stable relic identity");
        expect(!item.affixes.empty() && item.name == base->name,
            "alternate Boss relic has a readable name and affixes");

        Stats expected = item.implicitStats;
        for (const auto& affix : item.affixes) {
            expected = combineStats(expected, affix.stats);
        }
        expect(statsEqual(item.stats, expected),
            "alternate Boss relic stats equal implicit plus affix contributions");
    }

    const Item brimstone = generator.generateBossReward(5, BossLootTheme::Brimstone);
    const Item storm = generator.generateBossReward(5, BossLootTheme::Storm);
    const Item brood = generator.generateBossReward(5, BossLootTheme::Brood);
    const Item frost = generator.generateBossReward(5, BossLootTheme::Frost);
    const Item archive = generator.generateBossReward(5, BossLootTheme::Archive);
    const Item obsidian = generator.generateBossReward(5, BossLootTheme::Obsidian);
    const Item aether = generator.generateBossReward(5, BossLootTheme::Aether);
    const Item sable = generator.generateBossReward(5, BossLootTheme::Sable);
    const Item bloodletting = generator.generateBossReward(5, BossLootTheme::Bloodletting);
    expect(std::abs(brimstone.stats.damageMultiplier - 1.27f) < 0.0001f
            && std::abs(brimstone.stats.areaDamageMultiplier - 1.14f) < 0.0001f,
        "Brimstone relic preserves its level-scaled combat bonuses");
    expect(std::abs(storm.stats.attackSpeedMultiplier - 1.16f) < 0.0001f
            && std::abs(storm.stats.projectileDamageMultiplier - 1.16f) < 0.0001f
            && std::abs(storm.stats.lightningDamageMultiplier - 1.16f) < 0.0001f,
        "Storm relic preserves its level-scaled combat bonuses");
    expect(std::abs(brood.stats.poisonDamageMultiplier - 1.16f) < 0.0001f
            && std::abs(brood.stats.areaRadiusMultiplier - 1.14f) < 0.0001f,
        "Brood relic preserves its level-scaled combat bonuses");
    expect(std::abs(frost.stats.coldDamageMultiplier - 1.16f) < 0.0001f
            && std::abs(frost.stats.areaRadiusMultiplier - 1.14f) < 0.0001f,
        "Frost relic preserves its level-scaled combat bonuses");
    expect(std::abs(brimstone.stats.fireDamageMultiplier - 1.16f) < 0.0001f,
        "Brimstone relic adds a level-scaled Fire bonus");
    expect(std::abs(archive.stats.coldDamageMultiplier - 1.20f) < 0.0001f
            && std::abs(archive.stats.projectileDamageMultiplier - 1.21f) < 0.0001f,
        "Archive relic adds level-scaled Cold and Projectile bonuses");
    expect(std::abs(obsidian.stats.fireDamageMultiplier - 1.20f) < 0.0001f
            && std::abs(obsidian.stats.areaDamageMultiplier - 1.21f) < 0.0001f
            && std::abs(obsidian.stats.areaRadiusMultiplier - 1.14f) < 0.0001f,
        "Obsidian relic adds level-scaled Fire and Area bonuses");
    expect(aether.stats.maxManaMultiplier > aether.implicitStats.maxManaMultiplier
            && aether.stats.manaRegenMultiplier > aether.implicitStats.manaRegenMultiplier
            && aether.stats.skillCostMultiplier < aether.implicitStats.skillCostMultiplier,
        "Aether relic adds real Mana sustain and skill cost bonuses");
    expect(sable.stats.poisonDamageMultiplier > sable.implicitStats.poisonDamageMultiplier
            && sable.stats.areaRadiusMultiplier > sable.implicitStats.areaRadiusMultiplier,
        "Sable relic adds real Poison and Area reach bonuses");
    expect(bloodletting.stats.physicalDamageMultiplier
                > bloodletting.implicitStats.physicalDamageMultiplier
            && bloodletting.stats.bleedDamageMultiplier
                > bloodletting.implicitStats.bleedDamageMultiplier,
        "Bloodletting relic adds real Physical and Bleed bonuses");
}

void testBossRelicEffects() {
    section("Boss relic effect definitions");

    const auto& molten = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Brimstone);
    const auto& storm = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Storm);
    const auto& brood = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Brood);
    const auto& frost = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Frost);
    const auto& archive = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Archive);
    const auto& obsidian = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Obsidian);
    const auto& aether = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Aether);
    const auto& sable = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Sable);
    const auto& bloodletting = BossRelicEffectLibrary::forTheme(ItemBaseTheme::Bloodletting);
    const auto& none = BossRelicEffectLibrary::forTheme(ItemBaseTheme::None);

    expect(molten.type == BossRelicEffectType::MoltenCore
            && molten.igniteDamageMultiplier > 1.0f
            && molten.igniteDurationMultiplier > 1.0f,
        "Brimstone relic defines a stronger Ignite effect");
    expect(!molten.name.empty() && !molten.description.empty()
            && std::string(rarityName(Rarity::Unique)) == "Unique",
        "Unique relics expose a named effect for item details");
    expect(storm.type == BossRelicEffectType::StormChain
            && storm.lightningChainCount == 2
            && storm.lightningChainRadius > 0.0f
            && storm.lightningChainDamageMultiplier < 1.0f,
        "Storm relic defines a bounded Lightning chain");
    expect(brood.type == BossRelicEffectType::BroodBloom
            && brood.poisonSpreadRadius > 0.0f
            && brood.poisonSpreadMultiplier > 0.0f,
        "Brood relic defines a Poison death spread");
    expect(frost.type == BossRelicEffectType::Frostbite
            && frost.chillSpeedMultiplier < 1.0f
            && frost.chillDurationMultiplier > 1.0f,
        "Frost relic defines a stronger and longer Chill effect");
    expect(archive.type == BossRelicEffectType::ArchiveCurrent
            && archive.chillSpeedMultiplier < 1.0f
            && archive.chillDurationMultiplier > frost.chillDurationMultiplier,
        "Archive relic defines its stronger current Chill effect");
    expect(obsidian.type == BossRelicEffectType::ObsidianFurnace
            && obsidian.igniteDamageMultiplier > 1.0f
            && obsidian.igniteDurationMultiplier > 1.0f,
        "Obsidian relic defines its Furnace Ignite effect");
    expect(aether.type == BossRelicEffectType::AetherReserve
            && !aether.name.empty()
            && !aether.description.empty(),
        "Aether relic defines its Mana reserve effect");
    expect(sable.type == BossRelicEffectType::SableRot
            && !sable.name.empty()
            && !sable.description.empty(),
        "Sable relic defines its Poison gravebloom effect");
    expect(bloodletting.type == BossRelicEffectType::BloodPrice
            && bloodletting.bleedDamageMultiplier > 1.0f
            && bloodletting.bleedDurationMultiplier > 1.0f
            && bloodletting.bleedBurstRadius > 0.0f
            && bloodletting.bleedBurstDamageMultiplier > 0.0f,
        "Bloodletting relic defines its stronger Bleed and death-burst effect");

    const auto* ashenBase = ItemBaseLibrary::find("boss.ashen-crucible");
    const auto* tempestBase = ItemBaseLibrary::find("boss.tempest-bow");
    const auto* broodscaleBase = ItemBaseLibrary::find("boss.broodscale-band");
    const auto* winterheartBase = ItemBaseLibrary::find("boss.winterheart-pendant");
    const auto* drownedCompassBase = ItemBaseLibrary::find("boss.drowned-compass");
    const auto* blackglassHeartBase = ItemBaseLibrary::find("boss.blackglass-heart");
    const auto* hemorrhageSignetBase = ItemBaseLibrary::find("boss.hemorrhage-signet");
    const auto& ashen = ashenBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*ashenBase);
    const auto& tempest = tempestBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*tempestBase);
    const auto& broodscale = broodscaleBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*broodscaleBase);
    const auto& winterheart = winterheartBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*winterheartBase);
    const auto& drownedCompass = drownedCompassBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*drownedCompassBase);
    const auto& blackglassHeart = blackglassHeartBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*blackglassHeartBase);
    const auto& hemorrhageSignet = hemorrhageSignetBase == nullptr
        ? none : BossRelicEffectLibrary::forBase(*hemorrhageSignetBase);
    expect(ashenBase != nullptr
            && ashen.name == "Ashen Bloom"
            && ashen.igniteDamageMultiplier > molten.igniteDamageMultiplier,
        "Ashen Crucible selects its stronger Ignite effect");
    expect(tempestBase != nullptr
            && tempest.name == "Tempest Chain"
            && tempest.lightningChainCount > storm.lightningChainCount
            && tempest.lightningChainDamageMultiplier > storm.lightningChainDamageMultiplier,
        "Tempest Bow selects its wider Lightning chain effect");
    expect(broodscaleBase != nullptr
            && broodscale.name == "Broodscale Bloom"
            && broodscale.poisonSpreadRadius > brood.poisonSpreadRadius
            && broodscale.poisonSpreadMultiplier > brood.poisonSpreadMultiplier,
        "Broodscale Band selects its wider Poison spread effect");
    expect(winterheartBase != nullptr
            && winterheart.name == "Winter's Grasp"
            && winterheart.chillSpeedMultiplier < frost.chillSpeedMultiplier
            && winterheart.chillDurationMultiplier > frost.chillDurationMultiplier,
        "Winterheart Pendant selects its stronger Chill effect");
    expect(drownedCompassBase != nullptr
            && drownedCompass.name == "Drowned Compass"
            && drownedCompass.chillSpeedMultiplier < archive.chillSpeedMultiplier
            && drownedCompass.chillDurationMultiplier > archive.chillDurationMultiplier,
        "Drowned Compass selects its stronger Archive current effect");
    expect(blackglassHeartBase != nullptr
            && blackglassHeart.name == "Blackglass Heart"
            && blackglassHeart.igniteDamageMultiplier > obsidian.igniteDamageMultiplier
            && blackglassHeart.igniteDurationMultiplier > obsidian.igniteDurationMultiplier,
        "Blackglass Heart selects its stronger Obsidian Furnace effect");
    expect(hemorrhageSignetBase != nullptr
            && hemorrhageSignet.name == "Hemorrhage Signet"
            && hemorrhageSignet.bleedBurstRadius > bloodletting.bleedBurstRadius
            && hemorrhageSignet.bleedBurstDamageMultiplier
                > bloodletting.bleedBurstDamageMultiplier,
        "Hemorrhage Signet selects its stronger Bleed death-burst effect");
    expect(none.type == BossRelicEffectType::None && none.name.empty(),
        "non-relic themes have no Boss relic effect");
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
            case AffixStat::MaxManaMultiplier:
            case AffixStat::ManaRegenMultiplier:
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
            case AffixStat::PhysicalDamageMultiplier:
                expect(hasTag(AffixTag::Physical) && hasTag(AffixTag::Damage),
                    affix.name + " maps physical damage to Physical and Damage");
                break;
            case AffixStat::AreaDamageMultiplier:
            case AffixStat::AreaRadiusMultiplier:
                expect(hasTag(AffixTag::Area), affix.name + " maps area scaling to Area");
                break;
            case AffixStat::PoisonDamageMultiplier:
                expect(hasTag(AffixTag::Poison) && hasTag(AffixTag::Damage),
                    affix.name + " maps Poison damage to Poison and Damage");
                break;
            case AffixStat::BleedDamageMultiplier:
            case AffixStat::BleedDurationMultiplier:
            case AffixStat::BleedPenetration:
                expect(hasTag(AffixTag::Bleed) && hasTag(AffixTag::Damage),
                    affix.name + " maps Bleed scaling to Bleed and Damage");
                break;
            case AffixStat::FireDamageMultiplier:
                expect(hasTag(AffixTag::Fire) && hasTag(AffixTag::Damage),
                    affix.name + " maps Fire damage to Fire and Damage");
                break;
            case AffixStat::ColdDamageMultiplier:
                expect(hasTag(AffixTag::Cold) && hasTag(AffixTag::Damage),
                    affix.name + " maps Cold damage to Cold and Damage");
                break;
            case AffixStat::LightningDamageMultiplier:
                expect(hasTag(AffixTag::Lightning) && hasTag(AffixTag::Damage),
                    affix.name + " maps Lightning damage to Lightning and Damage");
                break;
            case AffixStat::PoisonResistance:
                expect(hasTag(AffixTag::Survival),
                    affix.name + " maps Poison resistance to Survival");
                break;
            case AffixStat::BleedResistance:
                expect(hasTag(AffixTag::Bleed) && hasTag(AffixTag::Survival),
                    affix.name + " maps Bleed resistance to Bleed and Survival");
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

    const auto maxManaIt = std::find_if(
        affixes.begin(), affixes.end(),
        [](const AffixDefinition& affix) {
            return affix.stat == AffixStat::MaxManaMultiplier;
        }
    );
    const auto manaRegenIt = std::find_if(
        affixes.begin(), affixes.end(),
        [](const AffixDefinition& affix) {
            return affix.stat == AffixStat::ManaRegenMultiplier;
        }
    );
    expect(maxManaIt != affixes.end() && manaRegenIt != affixes.end(),
        "affix library exposes Max Mana and Mana Regen definitions");
    const auto hasElementalSlotCoverage = [&](EquipmentSlot slot, AffixStat stat) {
        return std::any_of(
            affixes.begin(),
            affixes.end(),
            [slot, stat](const AffixDefinition& affix) {
                return affix.slot == slot && affix.stat == stat;
            }
        );
    };
    expect(hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::FireDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::ColdDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::LightningDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::PoisonDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::PhysicalDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Ring, AffixStat::BleedDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::FireDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::ColdDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::LightningDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::PoisonDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::PhysicalDamageMultiplier)
            && hasElementalSlotCoverage(EquipmentSlot::Amulet, AffixStat::BleedDamageMultiplier),
        "Ring and Amulet affix pools cover elemental and Physical/Bleed damage types");
    if (maxManaIt != affixes.end() && manaRegenIt != affixes.end()) {
        const Stats maxManaContribution = LootGenerator::contributionFor(
            maxManaIt->id, 5, 3
        );
        const Stats manaRegenContribution = LootGenerator::contributionFor(
            manaRegenIt->id, 5, 3
        );
        expect(maxManaContribution.maxManaMultiplier > 1.0f
                && manaRegenContribution.manaRegenMultiplier > 1.0f,
            "resource affixes produce real resource contributions");
    }
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
    expect(mapOptions[2].modifier.itemRarityMultiplier > 1.0f
            && mapOptions[2].modifier.itemQuantityMultiplier > 1.0f,
        "Gilded/Elite map option combines quantity and rarity rewards");

    auto generateSignatures = [](unsigned int seed, const LootBias& bias) {
        RandomService random(seed);
        LootGenerator generator;
        std::vector<std::string> signatures;
        for (int index = 0; index < 10; ++index) {
            const Item item = generator.generate(3, random, bias);
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

    RandomService random(123);
    LootGenerator generator;
    for (int index = 0; index < 18; ++index) {
        const Item item = generator.generate(5, random, areaBias);
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

    RandomService random(2468);
    LootGenerator generator;
    Item item;
    for (int roll = 0; roll < 64; ++roll) {
        item = generator.generate(3, random);
        if (item.affixes.size() >= 2) {
            break;
        }
    }

    expect(item.affixes.size() >= 2, "crafting fixture has at least two affixes");
    if (item.affixes.size() < 2) {
        return;
    }

    expect(craftingCostFor(CraftingOperation::ImproveAffix) == Config::ForgeImproveCost
            && craftingCostFor(CraftingOperation::RerollAffix) == Config::ForgeRerollCost
            && craftingCostFor(CraftingOperation::RaiseAffixTier) == Config::ForgeRaiseTierCost,
        "crafting operations use distinct configured fragment costs");
    Item normalItem = item;
    normalItem.rarity = Rarity::Normal;
    Item magicItem = item;
    magicItem.rarity = Rarity::Magic;
    Item uniqueItem = item;
    uniqueItem.rarity = Rarity::Unique;
    expect(craftingOperationAllowed(CraftingOperation::ImproveAffix, normalItem)
            && !craftingOperationAllowed(CraftingOperation::RerollAffix, normalItem)
            && craftingOperationAllowed(CraftingOperation::RaiseAffixTier, magicItem)
            && !craftingOperationAllowed(CraftingOperation::ImproveAffix, uniqueItem),
        "crafting operation availability follows item rarity");

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
    expect(!statsEqual(item.affixes[0].stats, beforeImprove),
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
    RandomService rerollRandom(97531);
    expect(LootGenerator::rerollAffix(item, 0, rerollRandom) == CraftingResult::Success,
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
    const auto& empowered = EliteModifierLibrary::forModifier(EliteModifier::Empowered);
    const auto& stormbound = EliteModifierLibrary::forModifier(EliteModifier::Stormbound);
    const auto& rejuvenating = EliteModifierLibrary::forModifier(EliteModifier::Rejuvenating);

    expect(none.name.empty(), "None modifier has no display label");
    expect(none.description.empty(), "None modifier has no risk description");
    expect(std::abs(none.hpMultiplier - 1.0f) < 0.0001f
            && std::abs(none.speedMultiplier - 1.0f) < 0.0001f,
        "None modifier leaves elite base stats unchanged");
    expect(hardened.hpMultiplier > 1.0f && hardened.speedMultiplier == 1.0f,
        "Hardened increases life without increasing speed");
    expect(hardened.description == "+60% maximum life and +20% ailment resistance",
        "Hardened risk description matches its life and resistance effects");
    expect(swift.speedMultiplier > 1.0f && swift.hpMultiplier == 1.0f,
        "Swift increases speed without increasing life");
    expect(swift.description == "+45% movement speed",
        "Swift risk description matches its speed multiplier");
    expect(volatileModifier.deathBurstRadius > 0.0f && volatileModifier.deathBurstDamage > 0,
        "Volatile defines a damaging death burst");
    expect(volatileModifier.description == "82 radius death burst for 2 damage",
        "Volatile risk description matches its death burst data");
    expect(empowered.allyDamageMultiplier > 1.0f && empowered.auraRadius > 0.0f,
        "Empowered defines a local damage aura");
    expect(stormbound.pulseInterval > 0.0f
            && stormbound.pulseTelegraphDuration > 0.0f
            && stormbound.pulseRadius > 0.0f
            && stormbound.pulseDamage > 0,
        "Stormbound defines a telegraphed periodic strike");
    expect(rejuvenating.healInterval > 0.0f
            && rejuvenating.healRadius > 0.0f
            && rejuvenating.healFraction > 0.0f
            && rejuvenating.healFraction < 1.0f,
        "Rejuvenating defines a bounded area heal");
    expect(hardened.rewardLootBias.primaryTag == AffixTag::Survival
            && hardened.rewardLootBias.secondaryTag == AffixTag::Armor,
        "Hardened biases survival and armor rewards");
    expect(swift.rewardLootBias.primaryTag == AffixTag::AttackSpeed
            && swift.rewardLootBias.secondaryTag == AffixTag::MoveSpeed,
        "Swift biases attack speed and movement rewards");
    expect(volatileModifier.rewardLootBias.primaryTag == AffixTag::Damage
            && volatileModifier.rewardLootBias.secondaryTag == AffixTag::Area,
        "Volatile biases damage and area rewards");

    Enemy normal({400.0f, 400.0f}, 10, 1, EnemyType::Normal, EliteModifier::Hardened);
    expect(normal.eliteModifier() == EliteModifier::None,
        "normal enemies cannot retain an Elite modifier");
    Enemy wounded({400.0f, 400.0f}, 10, 1, EnemyType::Elite);
    expect(wounded.takeDamage(6) == 6 && wounded.heal(3) == 3
            && wounded.hp() == 7 && wounded.heal(10) == 3 && wounded.hp() == 10,
        "enemy healing is capped at maximum life");
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
                + encounter.eliteWeight + encounter.chargerWeight
                + encounter.wardenWeight + encounter.summonerWeight == 100,
            mapTemplate.name + " encounter weights total 100");
    }
}

void testMapEncounterProfileRolls() {
    section("Map encounter profile composition");

    const auto& profile = MapTemplateLibrary::forIndex(1).encounter;
    RandomService random(60101);
    constexpr std::size_t enemyTypeCount =
        static_cast<std::size_t>(EnemyType::Summoner) + 1;
    std::array<bool, enemyTypeCount> observed{};
    for (int index = 0; index < 1000; ++index) {
        const EnemyType type = profile.rollEnemyType(random);
        observed[static_cast<std::size_t>(type)] = true;
    }

    expect(observed[static_cast<std::size_t>(EnemyType::Normal)],
        "encounter profile can roll Normal enemies");
    expect(observed[static_cast<std::size_t>(EnemyType::Ranged)],
        "Storm profile rolls its ranged signature enemies");
    expect(observed[static_cast<std::size_t>(EnemyType::Elite)],
        "encounter profile can roll Elite enemies");
    expect(observed[static_cast<std::size_t>(EnemyType::Charger)],
        "encounter profile can roll Charger enemies");
    expect(observed[static_cast<std::size_t>(EnemyType::Warden)],
        "encounter profile can roll Warden enemies");
    expect(observed[static_cast<std::size_t>(EnemyType::Summoner)],
        "encounter profile can roll Summoner enemies");

    constexpr std::size_t eliteModifierCount =
        static_cast<std::size_t>(EliteModifier::Volatile) + 1;
    std::array<bool, eliteModifierCount> observedModifiers{};
    for (int index = 0; index < 1000; ++index) {
        const EliteModifier modifier = profile.rollEliteModifier(random);
        observedModifiers[static_cast<std::size_t>(modifier)] = true;
    }

    expect(observedModifiers[static_cast<std::size_t>(EliteModifier::Hardened)],
        "Storm profile can roll Hardened elites");
    expect(observedModifiers[static_cast<std::size_t>(EliteModifier::Swift)],
        "Storm profile rolls Swift signature elites");
    expect(observedModifiers[static_cast<std::size_t>(EliteModifier::Volatile)],
        "Storm profile retains Volatile death-burst elites");
    expect(profile.rollEliteModifier(random, EliteModifier::Swift)
            != EliteModifier::Swift,
        "secondary elite modifier never duplicates the primary modifier");

    for (const auto& mapTemplate : MapTemplateLibrary::all()) {
        const auto& encounter = mapTemplate.encounter;
        expect(encounter.hardenedEliteModifierWeight
                + encounter.swiftEliteModifierWeight
                + encounter.volatileEliteModifierWeight > 0,
            mapTemplate.name + " has a non-empty elite modifier profile");
    }

    const auto& ashen = MapTemplateLibrary::forIndex(0).encounter;
    const auto& storm = MapTemplateLibrary::forIndex(1).encounter;
    const auto& venom = MapTemplateLibrary::forIndex(2).encounter;
    const auto& frost = MapTemplateLibrary::forIndex(3).encounter;
    expect(ashen.hardenedEliteModifierWeight > ashen.swiftEliteModifierWeight,
        "Ashen favors Hardened elites");
    expect(storm.swiftEliteModifierWeight > storm.hardenedEliteModifierWeight,
        "Storm favors Swift elites");
    expect(venom.volatileEliteModifierWeight > venom.hardenedEliteModifierWeight,
        "Venom favors Volatile elites");
    expect(frost.hardenedEliteModifierWeight > frost.swiftEliteModifierWeight,
        "Frost favors Hardened elites");
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
    for (const auto& boss : bosses) {
        expect(!boss.enrageTransitionDescription.empty()
                && boss.enrageSummonCount > 0
                && boss.enrageHazard.isValid(),
            boss.name + " defines a data-driven enrage transition");
        expect(boss.finalPhase.isValid()
                && boss.finalPhase.healthRatio < boss.enrageHealthRatio
                && !boss.finalPhase.patternDescription.empty()
                && !boss.finalPhase.skillOrder.empty()
                && boss.finalPhase.hazard.isValid(),
            boss.name + " defines a data-driven final phase");
        expect(boss.finalPhase.recurringHazard.isValid()
                || (boss.name != "Tidebound Archivist"
                    && boss.name != "Obsidian Tyrant"),
            boss.name + " defines a recurring final-phase hazard when themed");
        expect(std::all_of(
                boss.finalPhase.skillOrder.begin(),
                boss.finalPhase.skillOrder.end(),
                [&boss](std::size_t skillIndex) {
                    return skillIndex < boss.skills.size();
                }),
            boss.name + " final phase skill order references valid skills");
    }

    const std::array<std::tuple<std::string, BossPhaseHazardPattern, DamageType>, 6>
        recurringHazards{{
            {"Brimstone Colossus", BossPhaseHazardPattern::Cross, DamageType::Fire},
            {"Storm Herald", BossPhaseHazardPattern::Cross, DamageType::Lightning},
            {"Brood Matriarch", BossPhaseHazardPattern::Ring, DamageType::Poison},
            {"Frostbound Warden", BossPhaseHazardPattern::Cross, DamageType::Cold},
            {"Tidebound Archivist", BossPhaseHazardPattern::Target, DamageType::Cold},
            {"Obsidian Tyrant", BossPhaseHazardPattern::Ring, DamageType::Fire}
        }};
    for (const auto& expected : recurringHazards) {
        const auto it = std::find_if(
            bosses.begin(), bosses.end(),
            [&expected](const BossDefinition& boss) {
                return boss.name == std::get<0>(expected);
            }
        );
        expect(it != bosses.end()
                && it->finalPhase.recurringHazard.isValid()
                && it->finalPhase.recurringHazard.pattern == std::get<1>(expected)
                && it->finalPhase.recurringHazard.hazard.damageType
                    == std::get<2>(expected),
            std::get<0>(expected) + " final phase has its themed recurring hazard");
    }

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

    bool finalOrderSummonsRanged = false;
    for (std::size_t i = 0; i < broodIt->finalPhase.skillOrder.size(); ++i) {
        const auto& skill = broodIt->skillForCast(i, 2);
        finalOrderSummonsRanged = finalOrderSummonsRanged
            || (skill.type == BossSkillType::SummonAdds
                && skill.summonType == EnemyType::Ranged);
    }
    expect(finalOrderSummonsRanged,
        "Brood final phase order keeps ranged brood pressure");

    const auto executionerIt = std::find_if(
        bosses.begin(), bosses.end(),
        [](const BossDefinition& boss) {
            return boss.name == "Gorebound Executioner";
        }
    );
    expect(executionerIt != bosses.end()
            && executionerIt->lootTheme == BossLootTheme::Bloodletting
            && executionerIt->physicalResistance == 35
            && executionerIt->bleedResistance == 65,
        "Gorebound Executioner defines the physical Bleed boss profile");
    if (executionerIt != bosses.end()) {
        expect(std::any_of(
                executionerIt->skills.begin(), executionerIt->skills.end(),
                [](const BossSkillDefinition& skill) {
                    return skill.damageType == DamageType::Physical
                        && skill.ailment.type == AilmentType::Bleed;
                }),
            "Gorebound Executioner skills apply Physical Bleed pressure");
    }

    const auto ironheartIt = std::find_if(
        bosses.begin(), bosses.end(),
        [](const BossDefinition& boss) {
            return boss.name == "Ironheart Warden";
        }
    );
    expect(ironheartIt != bosses.end()
            && ironheartIt->lootTheme == BossLootTheme::Bloodletting
            && ironheartIt->physicalResistance == 45
            && ironheartIt->bleedResistance == 75
            && ironheartIt->guaranteedDrops == 3,
        "Ironheart Warden defines the high-tier Physical Bleed boss profile");
    if (ironheartIt != bosses.end()) {
        const auto summonIt = std::find_if(
            ironheartIt->skills.begin(), ironheartIt->skills.end(),
            [](const BossSkillDefinition& skill) {
                return skill.name == "Summon Ironbound";
            }
        );
        expect(summonIt != ironheartIt->skills.end()
                && summonIt->summonType == EnemyType::Warden
                && summonIt->summonCount == 2,
            "Ironheart Warden summons Warden adds in its normal pattern");
        expect(ironheartIt->finalPhase.recurringHazard.pattern
                == BossPhaseHazardPattern::Ring
                && ironheartIt->finalPhase.recurringHazard.hazard.ailment.type
                    == AilmentType::Bleed,
            "Ironheart Warden final phase seals the arena with a Bleed ring");
    }

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

    const auto& aether = BossLibrary::forMapLevel(7);
    const auto prismIt = std::find_if(
        aether.skills.begin(),
        aether.skills.end(),
        [](const BossSkillDefinition& skill) { return skill.name == "Prism Pulse"; }
    );
    expect(aether.lootTheme == BossLootTheme::Aether
            && prismIt != aether.skills.end()
            && prismIt->damageType == DamageType::Lightning
            && prismIt->ailment.type == AilmentType::Shock
            && prismIt->groundHazard.source == "Prism Residue",
        "Astral Nullifier adds a Lightning residue hazard to the Boss skill set");

    int configuredHazards = 0;
    for (const auto& boss : BossLibrary::all()) {
        for (const auto& skill : boss.skills) {
            configuredHazards += skill.groundHazard.isValid() ? 1 : 0;
        }
    }
    expect(configuredHazards == 8,
        "Boss skills define the current ground hazard set");
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

    expect(options[0].modifier.componentCount == 2
            && options[1].modifier.componentCount == 2
            && options[2].modifier.componentCount == 2,
        "each map option composes exactly two data definitions");
    expect(options[0].modifier.hasModifier("swift-hunt")
            && options[0].modifier.hasModifier("hardened-front")
            && options[1].modifier.hasModifier("frenzied-march")
            && options[1].modifier.hasModifier("blood-tax")
            && options[2].modifier.hasModifier("gilded-cache")
            && options[2].modifier.hasModifier("elite-tide"),
        "map options retain stable component identities");
    expect(options[0].modifier.monsterSpeedMultiplier > 1.0f
            && options[0].modifier.eventRewardMultiplier > 1.0f
            && options[0].modifier.ailmentResistanceBonus > 0,
        "first map composition exposes speed, event and resistance effects");
    expect(options[1].modifier.monsterDamageBonus > 0
            && options[1].modifier.bossDropBonus > 0
            && options[1].modifier.bossDamageMultiplier > 1.0f,
        "second map composition exposes damage and Boss reward effects");
    expect(options[2].modifier.chargerWeightBonus > 0
            && options[2].modifier.eventRewardMultiplier > 1.0f
            && options[2].modifier.itemLevelBonus > 0,
        "third map composition exposes encounter and item level effects");
    expect(options[0].modifier.hasModifier("cinder-ward")
            && options[1].modifier.hasModifier("stormbound")
            && options[2].modifier.hasModifier("frostbite")
            && options[0].modifier.elementalChallengeType == DamageType::Fire
            && options[1].modifier.elementalChallengeType == DamageType::Lightning
            && options[2].modifier.elementalChallengeType == DamageType::Cold
            && options[0].modifier.playerElementalResistancePenalty >= 25
            && options[1].modifier.monsterElementalResistanceBonus >= 15,
        "map options expose three data-driven elemental resistance challenges");
    const auto poisonOptions = MapOptionLibrary::generateOptions(3);
    expect(poisonOptions[2].modifier.hasModifier("venomtide")
            && poisonOptions[2].modifier.elementalChallengeType == DamageType::Poison
            && poisonOptions[2].modifier.playerElementalResistancePenalty >= 25,
        "higher-tier map options expose the Venomtide Poison challenge");
    expect(damageAfterResistance(
                100,
                options[0].modifier.elementalChallengeType,
                -options[0].modifier.playerElementalResistancePenalty,
                0,
                0
            ) > 100
            && damageAfterResistance(
                100,
                options[0].modifier.elementalChallengeType,
                options[0].modifier.monsterElementalResistanceBonus,
                0,
                0
            ) < 100,
        "elemental map challenge penalties and monster resistance affect matching damage");

    const auto repeatedOptions = MapOptionLibrary::generateOptions(2);
    expect(repeatedOptions[0].modifier.name == options[0].modifier.name
            && repeatedOptions[1].modifier.itemQuantityMultiplier
                == options[1].modifier.itemQuantityMultiplier
            && repeatedOptions[2].modifier.componentCount == options[2].modifier.componentCount,
        "map option generation is stable for the same map level");

    const auto defaultOption = MapOptionLibrary::defaultOption();
    expect(defaultOption.modifier.componentCount == 0
            && defaultOption.modifier.monsterSpeedMultiplier == 1.0f
            && defaultOption.modifier.eventRewardMultiplier == 1.0f,
        "default map remains neutral without modifier components");

    // Template indices map to themed maps (linked in generateOptions).
    expect(options[0].templateIndex == 0
            && options[1].templateIndex == 1
            && options[2].templateIndex == 2,
        "map options bind to distinct map templates 0/1/2");

    bool highTierThemesAligned = true;
    for (int mapLevel = 5; mapLevel <= 12; ++mapLevel) {
        for (const auto& highTierOption : MapOptionLibrary::generateOptions(mapLevel)) {
            const auto& templateDefinition = MapTemplateLibrary::forIndex(
                highTierOption.templateIndex
            );
            highTierThemesAligned = highTierThemesAligned
                && highTierOption.modifier.elementalChallengeType
                    == templateDefinition.signatureDamageType;
        }
    }
    expect(highTierThemesAligned,
        "high-tier map options align elemental challenges with map themes");
}

void testMapItemsAndAtlas() {
    section("MapItem persistence and atlas identity");

    const auto option = MapOptionLibrary::generateOptions(4)[2];
    const MapItem map = MapItemLibrary::fromOption(option, 4, 2);
    expect(map.mapLevel == 4 && map.layoutIndex == 2
            && map.option.modifier.componentCount == 2,
        "map item preserves level, layout and composed modifier");
    expect(!map.id.empty()
            && map.id == MapItemLibrary::fromOption(option, 4, 2).id,
        "map item identity is deterministic");
    expect(MapItemLibrary::displayName(map).find("Normal T4") == 0
            && MapItemLibrary::summary(map).find(map.option.modifier.name)
                != std::string::npos,
        "map item preview includes tier, template and modifier");

    MapOption magicOption = option;
    magicOption.rarity = MapRarity::Magic;
    magicOption.quality = 10;
    magicOption.explicitAffixIds = {"fecund", "guarded"};
    const MapItem magicMap = MapItemLibrary::fromOption(magicOption, 4, 2);
    const MapModifier effectiveModifier = MapItemLibrary::modifierFor(magicOption);
    expect(MapItemLibrary::validOptionMetadata(magicOption)
            && effectiveModifier.itemQuantityMultiplier
                > magicOption.modifier.itemQuantityMultiplier
            && effectiveModifier.bossHpMultiplier
                > magicOption.modifier.bossHpMultiplier,
        "map quality and explicit affixes change final map rewards and danger");
    expect(magicMap.id != map.id
            && MapItemLibrary::summary(magicMap).find("Fecund") != std::string::npos
            && MapItemLibrary::summary(magicMap).find("Guarded") != std::string::npos,
        "map identity and preview include explicit map affixes");

    RandomService mapRolls(7123);
    bool sawMagic = false;
    bool sawRare = false;
    bool allRolledMapsValid = true;
    for (int index = 0; index < 100; ++index) {
        const MapItem rolled = MapItemLibrary::rollFromOption(option, 6, index, mapRolls);
        sawMagic = sawMagic || rolled.option.rarity == MapRarity::Magic;
        sawRare = sawRare || rolled.option.rarity == MapRarity::Rare;
        allRolledMapsValid = allRolledMapsValid
            && MapItemLibrary::validOptionMetadata(rolled.option);
    }
    expect(allRolledMapsValid && sawMagic && sawRare,
        "map roll pool produces magic and rare maps over a run");

    MapAtlas atlas;
    expect(atlas.completedCount() == 0 && !atlas.contains(map.id),
        "new atlas starts empty");
    atlas.record(map);
    atlas.record(map);
    expect(atlas.completedCount() == 1 && atlas.contains(map.id),
        "atlas deduplicates repeated completion of one map identity");
    expect(atlas.atlasPoints() == 1
            && std::abs(atlas.bonuses().itemQuantityMultiplier - 1.03f) < 0.0001f
            && std::abs(atlas.bonuses().itemRarityMultiplier - 1.02f) < 0.0001f
            && atlas.bonuses().eliteWeightBonus == 0
            && atlas.bonuses().bossDropBonus == 0,
        "first atlas completion grants the baseline atlas bonus");
    expect(atlas.availablePoints() == 1
            && atlas.canAllocateNode(0)
            && !atlas.canAllocateNode(1)
            && !atlas.canAllocateNode(99),
        "atlas exposes one point and enforces passive prerequisites");
    expect(atlas.allocateNode(0)
            && atlas.allocatedNodeCount() == 1
            && atlas.availablePoints() == 0
            && atlas.isNodeAllocated(0)
            && atlas.bonuses().itemQuantityMultiplier > 1.03f,
        "atlas allocates a root node and adds its map quantity effect");
    MapItem second = map;
    second.id += ":second";
    MapItem third = map;
    third.id += ":third";
    expect(atlas.record(second) && atlas.record(third)
            && atlas.atlasPoints() == 3
            && atlas.bonuses().eliteWeightBonus == 1
            && atlas.bonuses().bossDropBonus == 1,
        "atlas points unlock elite and Boss reward bonuses");
    expect(atlas.canAllocateNode(1)
            && atlas.allocateNode(1)
            && atlas.isNodeAllocated(1)
            && atlas.availablePoints() == 1,
        "atlas allows a chained node after another map is completed");
    atlas.clear();
    expect(atlas.completedCount() == 0
            && atlas.allocatedNodeCount() == 0
            && atlas.availablePoints() == 0,
        "atlas reset clears completed maps and allocated nodes");
}

void testMapScalingProgression() {
    section("Map scaling progression and modifier monotonicity");

    const auto& bosses = BossLibrary::all();
    for (std::size_t optionIndex = 0; optionIndex < 3; ++optionIndex) {
        int previousEnemyHp = 0;
        int previousEnemyDamage = 0;
        int previousItemLevel = 0;
        for (int mapLevel = 1; mapLevel <= 5; ++mapLevel) {
            const auto options = MapOptionLibrary::generateOptions(mapLevel);
            const auto& modifier = options[optionIndex].modifier;
            const int enemyHp = MapScaling::enemyHp(mapLevel, modifier);
            const int enemyDamage = MapScaling::enemyDamage(mapLevel, modifier);
            const int itemLevel = MapScaling::itemLevel(mapLevel, modifier);
            expect(enemyHp >= previousEnemyHp,
                "map option " + std::to_string(optionIndex + 1)
                    + " keeps enemy HP non-decreasing at level "
                    + std::to_string(mapLevel));
            expect(enemyDamage >= previousEnemyDamage,
                "map option " + std::to_string(optionIndex + 1)
                    + " keeps enemy damage non-decreasing at level "
                    + std::to_string(mapLevel));
            expect(itemLevel >= previousItemLevel,
                "map option " + std::to_string(optionIndex + 1)
                    + " keeps item level non-decreasing at level "
                    + std::to_string(mapLevel));
            previousEnemyHp = enemyHp;
            previousEnemyDamage = enemyDamage;
            previousItemLevel = itemLevel;
        }
    }

    for (const auto& boss : bosses) {
        for (std::size_t optionIndex = 0; optionIndex < 3; ++optionIndex) {
            int previousBossHp = 0;
            int previousBossDamage = 0;
            for (int mapLevel = 1; mapLevel <= 5; ++mapLevel) {
                const auto options = MapOptionLibrary::generateOptions(mapLevel);
                const auto& modifier = options[optionIndex].modifier;
                const int bossHp = MapScaling::bossHp(mapLevel, modifier, boss);
                const int bossDamage = MapScaling::bossContactDamage(
                    mapLevel, modifier, boss
                );
                expect(bossHp >= previousBossHp,
                    boss.name + " keeps Boss HP non-decreasing for map option "
                        + std::to_string(optionIndex + 1) + " at level "
                        + std::to_string(mapLevel));
                expect(bossDamage >= previousBossDamage,
                    boss.name + " keeps contact damage non-decreasing for map option "
                        + std::to_string(optionIndex + 1) + " at level "
                        + std::to_string(mapLevel));
                previousBossHp = bossHp;
                previousBossDamage = bossDamage;
            }
        }
    }

    GroundHazardDefinition baseHazard;
    baseHazard.source = "Scaling Test";
    baseHazard.radius = 100.0f;
    baseHazard.duration = 3.0f;
    baseHazard.tickInterval = 0.75f;
    baseHazard.damage = 4;
    baseHazard.damageType = DamageType::Fire;
    baseHazard.ailment = {AilmentType::Ignite, 2.0f, 0.2f};

    const MapModifier baseline = MapModifierLibrary::empty();
    const auto levelOne = MapScaling::environmentHazard(1, baseline, baseHazard);
    expect(levelOne.damage == baseHazard.damage,
        "map level one ambient hazard keeps its authored damage");

    MapModifier dangerous = baseline;
    dangerous.monsterDamageBonus = 2;
    dangerous.bossDamageMultiplier = 1.25f;
    int previousFieldDamage = 0;
    for (int mapLevel = 1; mapLevel <= 5; ++mapLevel) {
        const auto fieldHazard = MapScaling::environmentHazard(
            mapLevel, dangerous, baseHazard
        );
        expect(fieldHazard.damage >= previousFieldDamage
                && fieldHazard.damage >= baseHazard.damage,
            "ambient field hazard damage scales monotonically at map level "
                + std::to_string(mapLevel));
        previousFieldDamage = fieldHazard.damage;
    }

    const auto arenaHazard = MapScaling::environmentHazard(
        5, dangerous, baseHazard, true
    );
    expect(arenaHazard.damage > previousFieldDamage,
        "Boss arena hazard carries additional Boss danger scaling");
    expect(arenaHazard.damageType == baseHazard.damageType
            && arenaHazard.ailment.type == baseHazard.ailment.type,
        "ambient hazard scaling preserves its elemental identity");
}

void testMapEncounterDefinitions() {
    section("Map encounter content definitions");

    const auto& encounters = MapEncounterLibrary::all();
    const auto bountyIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::BountyHunt;
        }
    );
    const auto cursedIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::CursedReliquary;
        }
    );
    const auto courtIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::WardenCourt;
        }
    );
    const auto frozenIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::FrozenReliquary;
        }
    );
    const auto archiveIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::ArchivePurge;
        }
    );
    const auto forgeIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::ForgeCollapse;
        }
    );
    const auto aetherIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::AetherConvergence;
        }
    );
    const auto sableIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::NecroticOssuary;
        }
    );
    const auto bloodlettingIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::BloodlettingPit;
        }
    );
    const auto ironheartIt = std::find_if(
        encounters.begin(),
        encounters.end(),
        [](const MapEncounterDefinition& encounter) {
            return encounter.type == MapEncounterType::IronheartTrial;
        }
    );
    expect(encounters.size() == 13,
        "map encounter library contains thirteen data-driven encounter definitions");
    expect(std::all_of(
                encounters.begin(), encounters.end(),
                [](const MapEncounterDefinition& encounter) {
                    return encounter.forgeFragmentReward > 0;
                }),
        "every map encounter defines a positive forge fragment reward");
    expect(bountyIt != encounters.end()
            && !bountyIt->id.empty()
            && bountyIt->eliteCount == 2
            && bountyIt->normalCount == 3
            && bountyIt->completionDropCount == 2
            && bountyIt->forgeFragmentReward == 2
            && bountyIt->rewardMultiplier > 1.0f,
        "Bounty Hunt defines its pack size and completion reward in data");
    expect(cursedIt != encounters.end()
            && cursedIt->eliteCount == 1
            && cursedIt->normalCount == 2
            && cursedIt->completionDropCount == 4
            && cursedIt->primaryEnemyType == EnemyType::Elite
            && cursedIt->secondaryEnemyType == EnemyType::Normal
            && cursedIt->forgeFragmentReward == 3,
        "Cursed Reliquary defines a guarded cache pack and larger reward");
    expect(courtIt != encounters.end()
            && courtIt->eliteCount == 2
            && courtIt->normalCount == 2
            && courtIt->primaryEnemyType == EnemyType::Warden
            && courtIt->secondaryEnemyType == EnemyType::Summoner
            && courtIt->forgeFragmentReward == 3,
        "Warden Court defines its Warden and Hexbinder composition");
    expect(frozenIt != encounters.end()
            && frozenIt->eliteCount == 1
            && frozenIt->normalCount == 3
            && frozenIt->completionDropCount == 3
            && frozenIt->primaryEnemyType == EnemyType::Elite
            && frozenIt->secondaryEnemyType == EnemyType::Warden
            && frozenIt->rewardLootBias.primaryTag == AffixTag::Cold
            && frozenIt->rewardLootBias.secondaryTag == AffixTag::Area
            && frozenIt->forgeFragmentReward == 4
            && frozenIt->hazard.damageType == DamageType::Cold
            && frozenIt->hazard.ailment.type == AilmentType::Chill,
        "Frozen Reliquary defines its Cold hazard, loot bias, and Warden composition");
    expect(archiveIt != encounters.end()
            && archiveIt->eliteCount == 1
            && archiveIt->normalCount == 4
            && archiveIt->primaryEnemyType == EnemyType::Warden
            && archiveIt->secondaryEnemyType == EnemyType::Ranged
            && archiveIt->completionDropCount == 4
            && archiveIt->rewardLootBias.primaryTag == AffixTag::Cold
            && archiveIt->rewardLootBias.secondaryTag == AffixTag::Projectile
            && archiveIt->hazard.damageType == DamageType::Cold
            && archiveIt->hazard.ailment.type == AilmentType::Chill
            && archiveIt->hazard.target == GroundHazardTarget::Player
            && archiveIt->overridesEnemyAttackProfile
            && archiveIt->enemyDamageType == DamageType::Cold
            && archiveIt->enemyAilment.type == AilmentType::Chill
            && archiveIt->leaderSkill.isValid()
            && archiveIt->leaderSkill.damageType == DamageType::Cold
            && archiveIt->leaderSkill.ailment.type == AilmentType::Chill
            && archiveIt->leaderSkill.groundHazard.source == "Frozen Ink"
            && archiveIt->bossDropBonus == 1,
        "Archive Purge defines its Cold hazard, Warden screen, and Projectile reward bias");
    expect(forgeIt != encounters.end()
            && forgeIt->eliteCount == 1
            && forgeIt->normalCount == 4
            && forgeIt->primaryEnemyType == EnemyType::Charger
            && forgeIt->secondaryEnemyType == EnemyType::Summoner
            && forgeIt->completionDropCount == 4
            && forgeIt->rewardLootBias.primaryTag == AffixTag::Fire
            && forgeIt->rewardLootBias.secondaryTag == AffixTag::Area
            && forgeIt->hazard.damageType == DamageType::Fire
            && forgeIt->hazard.ailment.type == AilmentType::Ignite
            && forgeIt->hazard.target == GroundHazardTarget::Player
            && forgeIt->overridesEnemyAttackProfile
            && forgeIt->enemyDamageType == DamageType::Fire
            && forgeIt->enemyAilment.type == AilmentType::Ignite
            && forgeIt->leaderSkill.isValid()
            && forgeIt->leaderSkill.damageType == DamageType::Fire
            && forgeIt->leaderSkill.ailment.type == AilmentType::Ignite
            && forgeIt->leaderSkill.groundHazard.source == "Magma Brand"
            && forgeIt->bossDropBonus == 1,
        "Forge Collapse defines its Fire hazard, Charger screen, and Area reward bias");
    expect(aetherIt != encounters.end()
            && aetherIt->eliteCount == 2
            && aetherIt->normalCount == 3
            && aetherIt->primaryEnemyType == EnemyType::Elite
            && aetherIt->secondaryEnemyType == EnemyType::Ranged
            && aetherIt->completionDropCount == 4
            && aetherIt->rewardLootBias.primaryTag == AffixTag::Survival
            && aetherIt->rewardLootBias.secondaryTag == AffixTag::Lightning
            && aetherIt->hazard.damageType == DamageType::Lightning
            && aetherIt->hazard.ailment.type == AilmentType::Shock
            && aetherIt->overridesEnemyAttackProfile
            && aetherIt->enemyDamageType == DamageType::Lightning
            && aetherIt->leaderSkill.isValid()
            && aetherIt->leaderSkill.damageType == DamageType::Lightning
            && aetherIt->leaderSkill.ailment.type == AilmentType::Shock
            && aetherIt->bossDropBonus == 2,
        "Aether Convergence defines its Lightning hazard, ranged screen, and Mana reward bias");
    expect(sableIt != encounters.end()
            && sableIt->eliteCount == 1
            && sableIt->normalCount == 4
            && sableIt->primaryEnemyType == EnemyType::Warden
            && sableIt->secondaryEnemyType == EnemyType::Summoner
            && sableIt->completionDropCount == 5
            && sableIt->rewardLootBias.primaryTag == AffixTag::Poison
            && sableIt->rewardLootBias.secondaryTag == AffixTag::Survival
            && sableIt->hazard.damageType == DamageType::Poison
            && sableIt->hazard.ailment.type == AilmentType::Poison
            && sableIt->leaderSkill.isValid()
            && sableIt->leaderSkill.damageType == DamageType::Poison
            && sableIt->bossDropBonus == 2,
        "Necrotic Ossuary defines its Poison hazard, Warden screen, and Survival reward bias");
    expect(bloodlettingIt != encounters.end()
            && bloodlettingIt->eliteCount == 1
            && bloodlettingIt->normalCount == 4
            && bloodlettingIt->primaryEnemyType == EnemyType::Charger
            && bloodlettingIt->secondaryEnemyType == EnemyType::Normal
            && bloodlettingIt->completionDropCount == 4
            && bloodlettingIt->rewardLootBias.primaryTag == AffixTag::Physical
            && bloodlettingIt->rewardLootBias.secondaryTag == AffixTag::Bleed
            && bloodlettingIt->hazard.damageType == DamageType::Physical
            && bloodlettingIt->hazard.ailment.type == AilmentType::Bleed
            && bloodlettingIt->overridesEnemyAttackProfile
            && bloodlettingIt->enemyDamageType == DamageType::Physical
            && bloodlettingIt->enemyAilment.type == AilmentType::Bleed
            && bloodlettingIt->leaderSkill.isValid()
            && bloodlettingIt->leaderSkill.ailment.type == AilmentType::Bleed
            && bloodlettingIt->bossDropBonus == 2,
        "Bloodletting Pit defines its Physical hazard, Bleed screen, and build reward bias");
    expect(ironheartIt != encounters.end()
            && ironheartIt->eliteCount == 2
            && ironheartIt->normalCount == 3
            && ironheartIt->primaryEnemyType == EnemyType::Elite
            && ironheartIt->secondaryEnemyType == EnemyType::Charger
            && ironheartIt->completionDropCount == 5
            && ironheartIt->rewardLootBias.primaryTag == AffixTag::Physical
            && ironheartIt->rewardLootBias.secondaryTag == AffixTag::Bleed
            && ironheartIt->hazard.damageType == DamageType::Physical
            && ironheartIt->hazard.ailment.type == AilmentType::Bleed
            && ironheartIt->leaderSkill.isValid()
            && ironheartIt->leaderSkill.ailment.type == AilmentType::Bleed
            && ironheartIt->bossDropBonus == 3
            && ironheartIt->bossDefinitionIndex == 9,
        "Ironheart Trial defines its high-tier Physical/Bleed encounter data");
}

// --- Map layout variants ---
void testMapLayoutVariants() {
    section("MapLayoutLibrary deterministic variants and geometry");

    const auto& layouts = MapLayoutLibrary::all();
    expect(layouts.size() == MapLayoutLibrary::TemplateCount,
        "map layout library has one group per map template");

    std::set<std::string> layoutIds;
    for (int templateIndex = 0; templateIndex < MapLayoutLibrary::TemplateCount; ++templateIndex) {
        const auto& variants = layouts[static_cast<std::size_t>(templateIndex)];
        const auto& templateDefinition = MapTemplateLibrary::forIndex(templateIndex);
        const auto& templateBoss = BossLibrary::forIndex(
            templateDefinition.bossDefinitionIndex
        );
        expect(variants.size() >= 3,
            "each map template has at least three layout variants");
        expect(templateDefinition.bossDefinitionIndex == templateIndex
                && !templateBoss.name.empty(),
            "map template binds an explicit Boss definition ["
                + templateDefinition.name + "]");

        for (int variantIndex = 0; variantIndex < MapLayoutLibrary::VariantCount; ++variantIndex) {
            const auto& layout = MapLayoutLibrary::forTemplate(templateIndex, variantIndex);
            const std::string layoutLabel = " [" + layout.id + "]";
            expect(!layout.id.empty() && layoutIds.insert(layout.id).second,
                "layout identity is non-empty and globally unique" + layoutLabel);
            expect(!layout.obstacles.empty() && layout.eventPositions.size() == 3,
                "layout defines obstacles and the three map events" + layoutLabel);

            MapInstance map(1, templateIndex, variantIndex);
            expect(map.templateIndex() == templateIndex
                    && map.layoutIndex() == variantIndex
                    && map.layoutId() == layout.id,
                "MapInstance binds the requested template and layout" + layoutLabel);
            expect(map.events().size() == 4
                    && map.events().back().type == MapEventType::Combination
                    && map.events().back().encounterType != MapEncounterType::None
                    && !map.encounterDefinition().id.empty(),
                "layout adds exactly one data-driven combination encounter" + layoutLabel);
            expect(!map.intersectsObstacle(map.playerStart(), Config::PlayerRadius),
                "layout leaves the player start clear" + layoutLabel);
            expect(!map.intersectsObstacle(map.bossCenter(), Config::BossArenaRadius),
                "layout leaves the Boss arena clear" + layoutLabel);
            for (const auto& event : map.events()) {
                expect(!map.intersectsObstacle(event.position, event.radius),
                    "layout leaves event interaction space clear" + layoutLabel);
            }
            expect(map.geometryIsValid(),
                "layout stays in bounds, clear of protected points, and reachable" + layoutLabel);
            expect(map.hasReachableBossPath(), "layout has a reachable Boss path" + layoutLabel);
        }
    }

    expect(MapInstance(1, 1).layoutIndex() == 0
            && MapInstance(2, 1).layoutIndex() == 1
            && MapInstance(3, 1).layoutIndex() == 2
            && MapInstance(4, 1).layoutIndex() == 0,
        "map level selects layout variants deterministically");
    expect(MapInstance(4).templateIndex() == 3
            && MapInstance(4).definition().name == "Frostbound Pass"
            && MapInstance(4).encounterDefinition().type == MapEncounterType::FrozenReliquary,
        "map level four selects the Frostbound Pass theme and encounter");
    const auto frostOptions = MapOptionLibrary::generateOptions(4);
    expect(frostOptions[2].templateIndex == 3
            && frostOptions[2].modifier.hasModifier("frostbite"),
        "high-tier map selection exposes Frostbound Pass with its Cold challenge");
    const auto archiveOptions = MapOptionLibrary::generateOptions(5);
    const auto reliquaryOptions = MapOptionLibrary::generateOptions(6);
    expect(archiveOptions[0].templateIndex == 4
            && archiveOptions[1].templateIndex == 5
            && reliquaryOptions[0].templateIndex == 5,
        "map levels five and six rotate in the Drowned Archive and Obsidian Reliquary themes");
    expect(MapTemplateLibrary::forIndex(4).name == "Drowned Archive"
            && MapTemplateLibrary::forIndex(4).bossDefinitionIndex == 4
            && MapTemplateLibrary::forIndex(4).signatureDamageType == DamageType::Cold
            && MapTemplateLibrary::forIndex(4).signatureLootBias.primaryTag == AffixTag::Cold
            && MapTemplateLibrary::forIndex(5).name == "Obsidian Reliquary"
            && MapTemplateLibrary::forIndex(5).bossDefinitionIndex == 5
            && MapTemplateLibrary::forIndex(5).signatureDamageType == DamageType::Fire
            && MapTemplateLibrary::forIndex(5).signatureLootBias.primaryTag == AffixTag::Armor
            && MapTemplateLibrary::forIndex(6).name == "Aether Observatory"
            && MapTemplateLibrary::forIndex(6).bossDefinitionIndex == 6
            && MapTemplateLibrary::forIndex(6).signatureDamageType == DamageType::Lightning
            && MapTemplateLibrary::forIndex(6).signatureLootBias.baseTheme == ItemBuildTheme::Mana
            && MapTemplateLibrary::forIndex(7).name == "Sable Necropolis"
            && MapTemplateLibrary::forIndex(7).bossDefinitionIndex == 7
            && MapTemplateLibrary::forIndex(7).signatureDamageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(7).signatureLootBias.baseTheme
                == ItemBuildTheme::Survival,
        "new map themes bind their intended encounter and loot identities");
    expect(MapInstance(5).encounterDefinition().type == MapEncounterType::ArchivePurge
            && MapInstance(6).encounterDefinition().type == MapEncounterType::ForgeCollapse
            && MapInstance(7).encounterDefinition().type == MapEncounterType::AetherConvergence
            && MapInstance(8).encounterDefinition().type == MapEncounterType::NecroticOssuary,
        "new map themes use their own combination encounters instead of legacy content");
    expect(MapInstance(3, 0, 2).encounterDefinition().type
            == MapEncounterType::BloodlettingPit,
        "Ashen Causeway variant exposes the Bloodletting Pit encounter");
    expect(MapInstance(9, 0, 2).encounterDefinition().type
            == MapEncounterType::IronheartTrial
            && MapInstance(9, 0, 2).encounterDefinition().bossDefinitionIndex == 9,
        "high-tier Ashen Causeway variant exposes the Ironheart Trial encounter");
    expect(MapTemplateLibrary::forIndex(0).ambientEffect.isValid()
            && MapTemplateLibrary::forIndex(0).ambientEffect.hazard.damageType == DamageType::Fire
            && MapTemplateLibrary::forIndex(1).ambientEffect.hazard.damageType == DamageType::Lightning
            && MapTemplateLibrary::forIndex(2).ambientEffect.hazard.damageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(3).ambientEffect.hazard.damageType == DamageType::Cold
            && MapTemplateLibrary::forIndex(0).signatureDamageType == DamageType::Fire
            && MapTemplateLibrary::forIndex(1).signatureDamageType == DamageType::Lightning
            && MapTemplateLibrary::forIndex(2).signatureDamageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(3).signatureDamageType == DamageType::Cold
            && MapTemplateLibrary::forIndex(0).signatureLootBias.primaryTag == AffixTag::Fire
            && MapTemplateLibrary::forIndex(1).signatureLootBias.primaryTag == AffixTag::Lightning
            && MapTemplateLibrary::forIndex(2).signatureLootBias.primaryTag == AffixTag::Poison
            && MapTemplateLibrary::forIndex(3).signatureLootBias.primaryTag == AffixTag::Cold
            && MapTemplateLibrary::forIndex(0).signatureLootBias.baseTheme == ItemBuildTheme::Area
            && MapTemplateLibrary::forIndex(1).signatureLootBias.baseTheme == ItemBuildTheme::Projectile
            && MapTemplateLibrary::forIndex(3).signatureLootBias.baseTheme == ItemBuildTheme::Area
            && MapTemplateLibrary::forIndex(0).bossArenaEffect.isValid()
            && MapTemplateLibrary::forIndex(1).bossArenaEffect.isValid()
            && MapTemplateLibrary::forIndex(2).bossArenaEffect.isValid()
            && MapTemplateLibrary::forIndex(3).bossArenaEffect.isValid()
            && MapTemplateLibrary::forIndex(7).ambientEffect.isValid()
            && MapTemplateLibrary::forIndex(7).bossArenaEffect.isValid()
            && MapTemplateLibrary::forIndex(0).bossArenaEffect.hazard.damageType == DamageType::Fire
            && MapTemplateLibrary::forIndex(1).bossArenaEffect.hazard.damageType == DamageType::Lightning
            && MapTemplateLibrary::forIndex(2).bossArenaEffect.hazard.damageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(3).bossArenaEffect.hazard.damageType == DamageType::Cold
            && MapTemplateLibrary::forIndex(7).ambientEffect.hazard.damageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(7).bossArenaEffect.hazard.damageType == DamageType::Poison
            && MapTemplateLibrary::forIndex(0).bossArenaEffect.pattern == MapHazardPattern::Ring
            && MapTemplateLibrary::forIndex(1).bossArenaEffect.pattern == MapHazardPattern::Cross
            && MapTemplateLibrary::forIndex(2).bossArenaEffect.pattern == MapHazardPattern::Ring
            && MapTemplateLibrary::forIndex(3).bossArenaEffect.pattern == MapHazardPattern::Target
            && MapTemplateLibrary::forIndex(7).bossArenaEffect.pattern == MapHazardPattern::Ring
            && MapTemplateLibrary::forIndex(0).signatureAilment.type == AilmentType::Ignite
            && MapTemplateLibrary::forIndex(1).signatureAilment.type == AilmentType::Shock
            && MapTemplateLibrary::forIndex(2).signatureAilment.type == AilmentType::Poison
            && MapTemplateLibrary::forIndex(3).signatureAilment.type == AilmentType::Chill
            && MapTemplateLibrary::forIndex(0).ambientEffect.minimumMapLevel == 2
            && MapTemplateLibrary::forIndex(1).ambientEffect.minimumMapLevel == 2
            && MapTemplateLibrary::forIndex(2).ambientEffect.minimumMapLevel == 2
            && MapTemplateLibrary::forIndex(3).ambientEffect.minimumMapLevel == 2,
        "map themes define valid elemental field hazards from map level two");

    MapInstance map(1, 0, 1);
    expect(!map.intersectsObstacle(map.playerStart(), Config::PlayerRadius),
        "layout obstacle does not cover player start");
    expect(!map.intersectsObstacle(map.bossCenter(), Config::BossArenaRadius),
        "layout obstacle does not cover Boss arena");
    const Vector2 resolved = map.resolveMovement(
        map.playerStart(), Config::PlayerRadius, {10000.0f, -10000.0f});
    expect(resolved.x >= Config::PlayerRadius
            && resolved.x <= map.size().x - Config::PlayerRadius
            && resolved.y >= Config::PlayerRadius
            && resolved.y <= map.size().y - Config::PlayerRadius
            && !map.intersectsObstacle(resolved, Config::PlayerRadius),
        "layout movement resolution still respects bounds and obstacles");
}

// --- Map exploration ---
void testMapExploration() {
    section("MapExploration reveal lifecycle");

    MapInstance map(1, 0, 0);
    const auto& exploration = map.exploration();
    const int initialCells = exploration.exploredCellCount();
    expect(initialCells > 0 && initialCells < exploration.totalCellCount(),
        "new maps reveal a finite area around the player start");
    expect(exploration.isExplored(map.playerStart()),
        "player start is initially explored");
    expect(!exploration.isFullyExplored(),
        "new maps do not reveal the whole minimap");

    bool hiddenEventFound = false;
    for (const auto& event : map.events()) {
        if (!exploration.isExplored(event.position)) {
            hiddenEventFound = true;
        }
    }
    expect(hiddenEventFound, "events outside the starting area remain hidden");

    const Vector2 eventPosition = map.events().front().position;
    map.revealAround(eventPosition);
    const int afterEventReveal = map.exploration().exploredCellCount();
    expect(afterEventReveal > initialCells
            && map.exploration().isExplored(eventPosition),
        "moving into an event area reveals it and nearby cells");

    map.revealAround(eventPosition);
    expect(map.exploration().exploredCellCount() == afterEventReveal,
        "revisiting an explored area does not increase the explored count");

    const int beforeInvalidReveal = map.exploration().exploredCellCount();
    map.revealAround({-100.0f, -100.0f});
    expect(map.exploration().exploredCellCount() == beforeInvalidReveal,
        "out-of-bounds reveal requests do not reveal map edges");

    map.revealAround(map.bossCenter());
    expect(map.exploration().isExplored(map.bossCenter()),
        "approaching the Boss reveals the Boss area");
    expect(map.exploration().exploredCellCount() >= afterEventReveal,
        "exploration is monotonic while the player moves");

    map.resetExploration();
    expect(map.exploration().exploredCellCount() == initialCells
            && map.exploration().isExplored(map.playerStart())
            && !map.exploration().isExplored(map.bossCenter()),
        "reset restores only the starting area");

    MapInstance nextMap(2, 1, 1);
    expect(nextMap.exploration().exploredCellCount() == initialCells
            && nextMap.exploration().isExplored(nextMap.playerStart())
            && !nextMap.exploration().isExplored(nextMap.bossCenter()),
        "a new map starts with a fresh exploration state");
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

    RandomService random(7);
    const auto rewards = MapRewardLibrary::generateOptions(
        unlockedSkills,
        unlockedSupports,
        random
    );
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

    const auto siphonReward = MapRewardLibrary::skillUnlockReward(
        SkillLibrary::siphonPulse()
    );
    expect(siphonReward.skillName == "Siphon Pulse"
            && siphonReward.description.find("Heal 2 HP per enemy hit")
                != std::string::npos,
        "Siphon Pulse reward preview exposes its recovery effect");
    const auto guardingReward = MapRewardLibrary::skillUnlockReward(
        SkillLibrary::guardingPulse()
    );
    expect(guardingReward.description.find("Take 50% damage for 3s")
                != std::string::npos,
        "Guarding Pulse reward preview exposes its mitigation effect");
    const auto manaWardReward = MapRewardLibrary::skillUnlockReward(
        SkillLibrary::manaWard()
    );
    expect(manaWardReward.description.find("Ward 35% of Max Mana for 5s")
                != std::string::npos,
        "Mana Ward reward preview exposes its resource shield");

    RandomService earlySupportRandom(8);
    const auto earlySupportRewards = MapRewardLibrary::generateOptions(
        unlockedSkills,
        unlockedSupports,
        std::map<std::string, int>{},
        std::map<std::string, int>{},
        2,
        DamageType::Physical,
        earlySupportRandom
    );
    const bool hasEarlySupport = std::any_of(
        earlySupportRewards.begin(), earlySupportRewards.end(),
        [](const MapRewardDefinition& reward) {
            return reward.type == MapRewardType::UnlockSupport;
        }
    );
    expect(hasEarlySupport,
        "map level two rewards can unlock a Support compatible with the starter build");

    const auto themedReward = [&](DamageType theme, std::uint64_t seed) {
        RandomService themedRandom(seed);
        return MapRewardLibrary::generateOptions(
            unlockedSkills,
            unlockedSupports,
            std::map<std::string, int>{},
            std::map<std::string, int>{},
            1,
            theme,
            themedRandom
        );
    };
    const auto fireRewards = themedReward(DamageType::Fire, 11);
    const auto lightningRewards = themedReward(DamageType::Lightning, 12);
    const auto poisonRewards = themedReward(DamageType::Poison, 13);
    const auto rewardHasSkillTheme = [](const MapRewardDefinition& reward, DamageType theme) {
        const auto* skill = SkillLibrary::find(reward.skillName);
        return reward.type == MapRewardType::UnlockSkill
            && skill != nullptr
            && skill->damageType == theme;
    };
    expect(rewardHasSkillTheme(fireRewards[0], DamageType::Fire),
        "Brimstone-themed rewards lead with a Fire skill unlock");
    expect(rewardHasSkillTheme(lightningRewards[0], DamageType::Lightning),
        "Storm-themed rewards lead with a Lightning skill unlock");
    expect(rewardHasSkillTheme(poisonRewards[0], DamageType::Poison),
        "Brood-themed rewards lead with a Poison skill unlock");

    std::set<std::string> physicalBuildSkills = unlockedSkills;
    physicalBuildSkills.insert(SkillLibrary::rendingVolley().name);
    physicalBuildSkills.insert(SkillLibrary::crimsonSweep().name);
    RandomService physicalSupportRandom(15);
    const auto physicalRewards = MapRewardLibrary::generateOptions(
        physicalBuildSkills,
        unlockedSupports,
        std::map<std::string, int>{},
        std::map<std::string, int>{},
        1,
        DamageType::Physical,
        physicalSupportRandom
    );
    expect(physicalRewards[0].type == MapRewardType::UnlockSupport
            && (physicalRewards[0].supportName == "Bloodletting"
                || physicalRewards[0].supportName == "Rupture"),
        "Bloodletting-themed rewards lead with a Physical Bleed support");

    RandomService physicalSkillRandom(16);
    const auto physicalSkillRewards = MapRewardLibrary::generateOptions(
        unlockedSkills,
        unlockedSupports,
        std::map<std::string, int>{},
        std::map<std::string, int>{},
        1,
        DamageType::Physical,
        physicalSkillRandom
    );
    expect(physicalSkillRewards[0].type == MapRewardType::UnlockSkill
            && physicalSkillRewards[0].skillName == SkillLibrary::crimsonSweep().name,
        "Physical-themed rewards lead with the Physical Bleed skill unlock");

    std::set<std::string> allSkills;
    for (const auto& skill : SkillLibrary::all()) {
        allSkills.insert(skill.name);
    }
    std::set<std::string> allSupports;
    for (const auto& support : SupportLibrary::all()) {
        allSupports.insert(support.name);
    }
    RandomService themedUpgradeRandom(14);
    const auto lightningUpgrades = MapRewardLibrary::generateOptions(
        allSkills,
        allSupports,
        std::map<std::string, int>{},
        std::map<std::string, int>{},
        2,
        DamageType::Lightning,
        themedUpgradeRandom
    );
    const auto lightningUpgrade = [&]() {
        if (lightningUpgrades[0].type == MapRewardType::UpgradeSkill) {
            const auto* skill = SkillLibrary::find(lightningUpgrades[0].skillName);
            return skill != nullptr && skill->damageType == DamageType::Lightning;
        }
        if (lightningUpgrades[0].type == MapRewardType::UpgradeSupport) {
            const auto* support = SupportLibrary::find(lightningUpgrades[0].supportName);
            return support != nullptr
                && (support->kind == SupportKind::Conductivity
                    || (support->kind == SupportKind::ElementalFocus
                        && support->requiredDamageType == DamageType::Lightning));
        }
        return false;
    };
    expect(lightningUpgrade(),
        "later Storm maps prefer a Lightning skill or support upgrade");

    const std::string arcBoltName = SkillLibrary::arcBolt().name;
    const std::string shockwaveName = SkillLibrary::shockwave().name;
    expect(unlockedSkills.count(arcBoltName) == 0 && unlockedSkills.count(shockwaveName) == 0,
        "expanded skills start locked in a fresh run");

    bool sawArcBolt = false;
    bool sawShockwave = false;
    for (std::uint64_t seed = 0; seed < 128 && (!sawArcBolt || !sawShockwave); ++seed) {
        RandomService rewardSeed(seed);
        const auto options = MapRewardLibrary::generateOptions(
            unlockedSkills, unlockedSupports, rewardSeed
        );
        for (const auto& option : options) {
            sawArcBolt = sawArcBolt || option.skillName == arcBoltName;
            sawShockwave = sawShockwave || option.skillName == shockwaveName;
        }
    }
    expect(sawArcBolt, "MapRewardLibrary can generate Arc Bolt unlock reward");
    expect(sawShockwave, "MapRewardLibrary can generate Shockwave unlock reward");

    std::set<std::string> onlyExpandedSupports = allSupports;
    onlyExpandedSupports.erase("Barrage");
    onlyExpandedSupports.erase("Concentration");
    RandomService expandedSupportRandom(99);
    const auto expandedSupportRewards = MapRewardLibrary::generateOptions(
        allSkills, onlyExpandedSupports, expandedSupportRandom
    );
    bool sawBarrage = false;
    bool sawConcentration = false;
    for (const auto& option : expandedSupportRewards) {
        sawBarrage = sawBarrage || option.supportName == "Barrage";
        sawConcentration = sawConcentration || option.supportName == "Concentration";
    }
    expect(sawBarrage && sawConcentration,
        "MapRewardLibrary generates both expanded Support unlock rewards");

    std::set<std::string> onlyNewSupports = allSupports;
    onlyNewSupports.erase("Echo");
    onlyNewSupports.erase("Pinpoint");
    RandomService newSupportRandom(1001);
    const auto newSupportRewards = MapRewardLibrary::generateOptions(
        allSkills, onlyNewSupports, newSupportRandom
    );
    expect(std::any_of(newSupportRewards.begin(), newSupportRewards.end(),
            [](const MapRewardDefinition& option) { return option.supportName == "Echo"; })
            && std::any_of(newSupportRewards.begin(), newSupportRewards.end(),
            [](const MapRewardDefinition& option) { return option.supportName == "Pinpoint"; }),
        "new Supports remain eligible for map unlock rewards");

    std::set<std::string> poisonSupports = allSupports;
    poisonSupports.erase("Toxicity");
    poisonSupports.erase("Contagion");
    RandomService poisonSupportRandom(1002);
    const auto poisonSupportRewards = MapRewardLibrary::generateOptions(
        allSkills, poisonSupports, poisonSupportRandom
    );
    expect(std::any_of(poisonSupportRewards.begin(), poisonSupportRewards.end(),
            [](const MapRewardDefinition& option) { return option.supportName == "Toxicity"; })
            && std::any_of(poisonSupportRewards.begin(), poisonSupportRewards.end(),
            [](const MapRewardDefinition& option) { return option.supportName == "Contagion"; }),
        "Poison supports appear as targeted late-run rewards");

    std::set<std::string> allSkillsExceptArc = allSkills;
    allSkillsExceptArc.erase(arcBoltName);
    RandomService arcUnlockRandom(100);
    const auto arcUnlockRewards = MapRewardLibrary::generateOptions(
        allSkillsExceptArc, allSupports, arcUnlockRandom
    );
    expect(std::any_of(arcUnlockRewards.begin(), arcUnlockRewards.end(),
            [&arcBoltName](const MapRewardDefinition& option) {
                return option.skillName == arcBoltName;
            }),
        "locked Arc Bolt remains eligible for an unlock reward");

    const std::string splitArrowName = SkillLibrary::splitArrow().name;
    const std::string aftershockName = SkillLibrary::aftershock().name;
    std::set<std::string> allSkillsExceptNew = allSkills;
    allSkillsExceptNew.erase(splitArrowName);
    allSkillsExceptNew.erase(aftershockName);
    RandomService newSkillRandom(102);
    const auto newSkillRewards = MapRewardLibrary::generateOptions(
        allSkillsExceptNew, allSupports, newSkillRandom
    );
    expect(std::any_of(newSkillRewards.begin(), newSkillRewards.end(),
            [&splitArrowName](const MapRewardDefinition& option) {
                return option.skillName == splitArrowName;
            })
            && std::any_of(newSkillRewards.begin(), newSkillRewards.end(),
            [&aftershockName](const MapRewardDefinition& option) {
                return option.skillName == aftershockName;
            }),
        "new skills remain eligible for map unlock rewards");

    RandomService noRepeatRandom(101);
    const auto noRepeatRewards = MapRewardLibrary::generateOptions(
        allSkills, allSupports, noRepeatRandom
    );
    expect(std::none_of(noRepeatRewards.begin(), noRepeatRewards.end(),
            [&arcBoltName](const MapRewardDefinition& option) {
                return option.skillName == arcBoltName;
            }),
        "unlocked Arc Bolt is not offered again after it is obtained");

    auto rewardSignature = [](const std::array<MapRewardDefinition, 3>& options) {
        std::string signature;
        for (const auto& option : options) {
            signature += option.title + "|" + option.skillName + "|" + option.supportName + ";";
        }
        return signature;
    };
    RandomService repeatedRandom(7);
    RandomService differentRandom(8);
    const auto repeatedRewards = MapRewardLibrary::generateOptions(
        unlockedSkills,
        unlockedSupports,
        repeatedRandom
    );
    const auto differentRewards = MapRewardLibrary::generateOptions(
        unlockedSkills,
        unlockedSupports,
        differentRandom
    );
    expect(rewardSignature(rewards) == rewardSignature(repeatedRewards),
        "same reward seed reproduces the reward options");
    expect(rewardSignature(rewards) != rewardSignature(differentRewards),
        "different reward seeds change the reward options");
}

void testGemProgression() {
    section("Skill and support gem progression");

    const SkillDefinition levelOneSkill = SkillLibrary::meteor();
    const SkillDefinition levelThreeSkill = SkillProgression::skillAtLevel(
        levelOneSkill, 3
    );
    expect(levelThreeSkill.baseDamage > levelOneSkill.baseDamage
            && levelThreeSkill.radius > levelOneSkill.radius
            && levelThreeSkill.cooldown < levelOneSkill.cooldown,
        "skill gem levels increase damage/radius and reduce cooldown");
    const SkillDefinition levelOneWard = SkillLibrary::manaWard();
    const SkillDefinition levelThreeWard = SkillProgression::skillAtLevel(
        levelOneWard, 3
    );
    expect(levelThreeWard.wardManaRatio > levelOneWard.wardManaRatio,
        "skill gem levels increase Mana Ward capacity scaling");

    const auto* quickcast = SupportLibrary::find("Quickcast");
    const auto* amplify = SupportLibrary::find("Amplify");
    const auto* pierce = SupportLibrary::find("Pierce");
    const auto levelOneQuickcast = SkillProgression::supportAtLevel(*quickcast, 1);
    const auto levelFourQuickcast = SkillProgression::supportAtLevel(*quickcast, 4);
    const auto levelFourAmplify = SkillProgression::supportAtLevel(*amplify, 4);
    const auto levelFourPierce = SkillProgression::supportAtLevel(*pierce, 4);
    expect(levelFourQuickcast.damageMultiplier > levelOneQuickcast.damageMultiplier
            && levelFourQuickcast.cooldownMultiplier < levelOneQuickcast.cooldownMultiplier,
        "support gem levels improve a damage penalty and cooldown");
    expect(levelFourAmplify.radiusMultiplier > amplify->radiusMultiplier,
        "support gem levels improve area radius");
    expect(levelFourPierce.pierceCount > pierce->pierceCount,
        "support gem levels improve discrete support effects");

    std::set<std::string> unlockedSkills = {
        SkillLibrary::spreadShot().name,
        SkillLibrary::meteor().name,
        SkillLibrary::pulse().name,
        SkillLibrary::dash().name,
    };
    std::set<std::string> unlockedSupports;
    std::map<std::string, int> skillLevels;
    std::map<std::string, int> supportLevels;
    for (const auto& skill : unlockedSkills) {
        skillLevels[skill] = 1;
    }
    RandomService random(73);
    const auto options = MapRewardLibrary::generateOptions(
        unlockedSkills, unlockedSupports, skillLevels, supportLevels, 2, random
    );
    expect(std::any_of(options.begin(), options.end(), [](const MapRewardDefinition& option) {
        return option.type == MapRewardType::UpgradeSkill
            && option.targetLevel == 2;
    }), "map level rewards include a skill gem upgrade");
}

void testEnemyPackLibrary() {
    section("Data-driven field enemy packs");

    const auto& packs = EnemyPackLibrary::all();
    expect(packs.size() == 24, "eight map themes expose three field packs each");
    for (const auto& pack : packs) {
        expect(!pack.id.empty() && !pack.name.empty()
                && pack.enemyCount == static_cast<int>(pack.enemies.size())
                && pack.clearRewardDrops > 0
                && pack.lootBias.primaryTag != AffixTag::None
                && pack.leaderIndex >= 0
                && pack.leaderIndex < pack.enemyCount
                && pack.enemies[static_cast<std::size_t>(pack.leaderIndex)] == EnemyType::Elite
                && !pack.leaderName.empty()
                && !pack.leaderDescription.empty()
                && pack.leaderModifiers[0] != EliteModifier::None
                && pack.leaderDropMultiplier > 1.0f
                && pack.leaderBonusDrops > 0
                && pack.leaderExperienceMultiplier > 1,
            "field pack has a complete composition: " + pack.name);
    }

    const auto& ashen = EnemyPackLibrary::forMap(0, 1, 0);
    const auto& storm = EnemyPackLibrary::forMap(1, 1, 0);
    const auto& venom = EnemyPackLibrary::forMap(2, 1, 0);
    const auto& frost = EnemyPackLibrary::forMap(3, 1, 0);
    const auto& drowned = EnemyPackLibrary::forMap(4, 1, 0);
    const auto& obsidian = EnemyPackLibrary::forMap(5, 1, 0);
    const auto hasType = [](const EnemyPackDefinition& pack, EnemyType type) {
        return std::find(pack.enemies.begin(), pack.enemies.end(), type)
            != pack.enemies.end();
    };
    expect(hasType(ashen, EnemyType::Charger) && hasType(ashen, EnemyType::Elite),
        "Ashen packs combine melee pressure with an elite node");
    expect(hasType(storm, EnemyType::Ranged) && hasType(storm, EnemyType::Summoner),
        "Storm packs combine ranged pressure with a summoner anchor");
    expect(hasType(venom, EnemyType::Warden) && hasType(venom, EnemyType::Charger),
        "Venom packs combine a defensive anchor with chargers");
    expect(hasType(frost, EnemyType::Warden) && hasType(frost, EnemyType::Charger)
            && frost.lootBias.primaryTag == AffixTag::Cold,
        "Frost packs combine Warden pressure with Cold-biased rewards");
    expect(hasType(drowned, EnemyType::Ranged) && hasType(drowned, EnemyType::Warden)
            && drowned.lootBias.primaryTag == AffixTag::Cold,
        "Drowned Archive packs combine ranged pressure with Cold-biased rewards");
    expect(hasType(obsidian, EnemyType::Charger) && hasType(obsidian, EnemyType::Elite)
            && obsidian.lootBias.primaryTag == AffixTag::Fire,
        "Obsidian Reliquary packs combine chargers with Fire-biased rewards");
    expect(EnemyPackLibrary::forMap(0, 1, 0).id
            != EnemyPackLibrary::forMap(0, 1, 1).id,
        "field pack sequence rotates within a map theme");
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

    testRandomService();
    testEnemySpawnerRandomness();
    testPassiveTreePrerequisitesAndStats();
    testPassiveKeystones();
    testSkillBarAssignSkillAndSupport();
    testManaResourceAndSkillCastGates();
    testCombatMathDamageRadiusPierce();
    testSkillPreviewSupportCompatibility();
    testSkillBuildMathMatrix();
    testSkillAilments();
    testPlayerAilments();
    testAilmentResistances();
    testBossElementalSkills();
    testWardenProtectionMath();
    testSummonerStateMachine();
    testPlayerArmorMitigation();
    testEquipmentChangesCombatStats();
    testLootGeneration();
    testItemBaseTypes();
    testBossRelicEffects();
    testAffixTagsAndWeights();
    testCraftingChoiceOperations();
    testEliteModifierDefinitions();
    testChargerStateMachine();
    testMapEncounterProfileRolls();
    testFlaskChargeRewards();
    testBossSummonDefinitions();
    testGroundHazardLifecycle();
    testBossDashStateAndStormPattern();
    testMapOptionGeneration();
    testMapItemsAndAtlas();
    testMapScalingProgression();
    testMapEncounterDefinitions();
    testMapLayoutVariants();
    testMapExploration();
    testMapRewardGeneration();
    testGemProgression();
    testEnemyPackLibrary();
    testPassiveAndEquipPipeline();
    testItemContainers();

    std::cout << "\n========================================\n";
    std::cout << "Passed: " << g_passed << "  Failed: " << g_failures << '\n';
    std::cout << "========================================\n";
    return g_failures == 0 ? 0 : 1;
}
