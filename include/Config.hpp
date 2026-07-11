#pragma once

namespace Config {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;
    constexpr float MapWidth = 2400.0f;
    constexpr float MapHeight = 1800.0f;
    constexpr float StartSafeRadius = 220.0f;
    constexpr float BossGateRadius = 380.0f;
    constexpr float BossArenaRadius = 260.0f;

    constexpr float PlayerSpeed = 300.0f;
    constexpr float PlayerRadius = 20.0f;
    constexpr int PlayerHp = 3;
    constexpr float PlayerHitCooldown = 0.6f;
    constexpr int LifeFlaskMaxCharges = 3;
    constexpr int LifeFlaskHealAmount = 2;

    constexpr float ProjectileSpeed = 500.0f;
    constexpr float ProjectileRadius = 5.0f;
    constexpr int ProjectileDamage = 1;

    constexpr float PrimarySkillCooldown = 0.35f;
    constexpr int SpreadShotProjectileCount = 3;
    constexpr float SpreadShotSpreadAngle = 30.0f;

    constexpr float DashDistance = 120.0f;
    constexpr float DashCooldown = 0.8f;

    constexpr float NovaRadius = 110.0f;
    constexpr int NovaDamage = 2;
    constexpr float NovaCooldown = 1.5f;
    constexpr float NovaEffectDuration = 0.25f;

    constexpr float PulseRadius = 160.0f;
    constexpr int PulseDamage = 3;
    constexpr float PulseCooldown = 2.0f;
    constexpr float PulseEffectDuration = 0.30f;

    constexpr float SecondarySkillRadius = 85.0f;
    constexpr int SecondarySkillDamage = 1;
    constexpr float SecondarySkillCooldown = 1.0f;
    constexpr float SecondarySkillEffectDuration = 0.20f;

    constexpr float MeteorRadius = 110.0f;
    constexpr int MeteorDamage = 4;
    constexpr float MeteorCooldown = 1.4f;
    constexpr float MeteorEffectDuration = 0.30f;

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
    constexpr int MaxActiveEnemies = 28;
    constexpr float EnemySpawnMinDistance = 520.0f;
    constexpr float EnemySpawnMaxDistance = 760.0f;

    constexpr int ExpPerKill = 1;
    constexpr int BaseExpToLevel = 5;

    constexpr float ItemDropRadius = 7.0f;
    constexpr float ItemPickupRange = 32.0f;
    constexpr int ItemDropChancePercent = 35;
    constexpr int InventoryCapacity = 9;
    constexpr int ForgeUpgradeCost = 3;
    constexpr int MaxItemUpgradeLevel = 3;

    constexpr int MapWaveCount = 3;
    constexpr int BaseEnemiesPerWave = 6;
    constexpr int EnemiesPerMapLevel = 2;
}
