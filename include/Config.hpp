#pragma once

namespace Config {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    constexpr float MapWidth = 2400.0f;
    constexpr float MapHeight = 1800.0f;
    constexpr float StartSafeRadius = 220.0f;
    constexpr float BossGateRadius = 380.0f;
    constexpr float BossArenaRadius = 260.0f;
    constexpr int BossGateRequiredFieldPacks = 2;

    constexpr float PlayerSpeed = 300.0f;
    constexpr float PlayerRadius = 20.0f;
    constexpr int PlayerHp = 3;
    constexpr float PlayerHitCooldown = 0.6f;
    constexpr float PlayerHitEffectDuration = 0.45f;
    constexpr int LifeFlaskMaxCharges = 3;
    constexpr int LifeFlaskHealAmount = 2;

    constexpr float ProjectileSpeed = 500.0f;
    constexpr float ProjectileRadius = 5.0f;
    constexpr int ProjectileDamage = 1;

    constexpr float PrimarySkillCooldown = 0.35f;
    constexpr float PrimarySkillManaCost = 2.0f;
    constexpr int SpreadShotProjectileCount = 3;
    constexpr float SpreadShotSpreadAngle = 30.0f;
    constexpr float SplitArrowCooldown = 0.55f;
    constexpr float SplitArrowManaCost = 4.0f;
    constexpr int SplitArrowDamage = 1;
    constexpr int SplitArrowProjectileCount = 5;
    constexpr float SplitArrowSpreadAngle = 40.0f;

    constexpr float DashDistance = 120.0f;
    constexpr float DashCooldown = 0.8f;
    constexpr float DashManaCost = 0.0f;

    constexpr float NovaRadius = 110.0f;
    constexpr int NovaDamage = 2;
    constexpr float NovaCooldown = 1.5f;
    constexpr float NovaManaCost = 8.0f;
    constexpr float NovaEffectDuration = 0.25f;

    constexpr float PulseRadius = 160.0f;
    constexpr int PulseDamage = 3;
    constexpr float PulseCooldown = 2.0f;
    constexpr float PulseManaCost = 12.0f;
    constexpr float PulseEffectDuration = 0.30f;

    constexpr float SecondarySkillRadius = 85.0f;
    constexpr int SecondarySkillDamage = 1;
    constexpr float SecondarySkillCooldown = 1.0f;
    constexpr float FlareManaCost = 8.0f;
    constexpr float FrostBombManaCost = 10.0f;
    constexpr float ToxicBurstRadius = 105.0f;
    constexpr int ToxicBurstDamage = 3;
    constexpr float ToxicBurstCooldown = 1.35f;
    constexpr float ToxicBurstManaCost = 11.0f;
    constexpr float ToxicBurstEffectDuration = 0.25f;
    constexpr float ToxicBurstGroundHazardDuration = 1.80f;
    constexpr float ToxicBurstGroundHazardTickInterval = 0.60f;
    constexpr int ToxicBurstGroundHazardDamage = 1;
    constexpr float SecondarySkillEffectDuration = 0.20f;

    constexpr float MeteorRadius = 110.0f;
    constexpr int MeteorDamage = 4;
    constexpr float MeteorCooldown = 1.4f;
    constexpr float MeteorManaCost = 12.0f;
    constexpr float BladestormManaCost = 10.0f;
    constexpr int BladestormHitCount = 5;
    constexpr float BladestormHitInterval = 0.22f;
    constexpr float MeteorEffectDuration = 0.30f;
    constexpr float MeteorGroundHazardDuration = 2.40f;
    constexpr float MeteorGroundHazardTickInterval = 0.60f;
    constexpr int MeteorGroundHazardDamage = 1;
    constexpr float FrostBombGroundHazardDuration = 2.00f;
    constexpr float FrostBombGroundHazardTickInterval = 0.50f;
    constexpr int FrostBombGroundHazardDamage = 1;
    constexpr float AftershockRadius = 125.0f;
    constexpr int AftershockDamage = 5;
    constexpr float AftershockCooldown = 2.6f;
    constexpr float AftershockManaCost = 14.0f;
    constexpr float AftershockEffectDuration = 0.35f;
    constexpr float AftershockCastDelay = 0.60f;

    constexpr float EnemySpeed = 120.0f;
    constexpr float EnemyRadius = 20.0f;
    constexpr int EnemyHp = 1;
    constexpr int EnemyContactDamage = 1;
    constexpr float BossSkillInterval = 2.2f;
    constexpr float BossAoeRadius = 135.0f;
    constexpr int BossAoeDamage = 2;
    constexpr float BossAoeTelegraphDuration = 0.65f;
    constexpr float BossAoeEffectDuration = 0.30f;
    constexpr float BossProjectileSpeed = 360.0f;
    constexpr float BossProjectileRadius = 8.0f;
    constexpr int BossProjectileDamage = 1;

    constexpr float EnemySpawnInterval = 1.0f;
    constexpr float EnemyPackSpawnSpacing = 0.45f;
    constexpr int MaxActiveEnemies = 28;
    constexpr int MaxBossSummonedEnemies = 10;
    constexpr int MaxSummonerMinions = 6;
    constexpr float SummonerMinionSpreadRadius = 64.0f;
    constexpr float EnemySpawnMinDistance = 520.0f;
    constexpr float EnemySpawnMaxDistance = 760.0f;
    constexpr float WardenAuraRadius = 180.0f;
    constexpr float WardenDamageTakenMultiplier = 0.70f;
    constexpr float VolatileExplosionEffectDuration = 0.35f;
    constexpr float AilmentTickInterval = 1.0f;
    constexpr int MaxPoisonStacks = 5;
    constexpr float ShrineBuffDuration = 20.0f;
    constexpr int ShrineDamageBonusPercent = 35;
    constexpr int LootCacheForgeFragmentReward = 1;
    constexpr int ElitePackForgeFragmentReward = 1;
    constexpr int ShrineForgeFragmentReward = 1;
    constexpr float ShrineDamageMultiplier = 1.0f
        + static_cast<float>(ShrineDamageBonusPercent) / 100.0f;
    constexpr int SkillGemMaxLevel = 5;
    constexpr float SkillGemDamagePerLevelMultiplier = 0.10f;
    constexpr float SkillGemRadiusPerLevelMultiplier = 0.04f;
    constexpr float SkillGemCooldownPerLevelMultiplier = 0.97f;
    constexpr float SupportGemDamageStep = 0.04f;
    constexpr float SupportGemRadiusStep = 0.04f;
    constexpr float SupportGemCooldownStep = 0.03f;
    constexpr float SupportGemAilmentStep = 0.06f;
    constexpr float CombatFeedbackDuration = 0.8f;
    constexpr int MaxCombatFeedback = 32;

    constexpr int ExpPerKill = 1;
    constexpr int BaseExpToLevel = 5;
    constexpr float PlayerMaxMana = 100.0f;
    constexpr float PlayerManaRegenPerSecond = 8.0f;

    constexpr float ItemDropRadius = 7.0f;
    constexpr float ItemPickupRange = 32.0f;
    constexpr int ItemDropChancePercent = 35;
    constexpr int InventoryCapacity = 9;
    constexpr int StashCapacity = 24;
    constexpr int ForgeImproveCost = 2;
    constexpr int ForgeRerollCost = 3;
    constexpr int ForgeRaiseTierCost = 4;
    constexpr const char* SaveFileName = "plane_shooter.save";

    constexpr int MapWaveCount = 3;
    constexpr int BaseEnemiesPerWave = 6;
    constexpr int EnemiesPerMapLevel = 2;
}
