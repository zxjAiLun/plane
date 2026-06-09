#pragma once

namespace Config {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;

    constexpr float PlayerSpeed = 300.0f;
    constexpr float PlayerRadius = 20.0f;
    constexpr int PlayerHp = 3;

    constexpr float ProjectileSpeed = 500.0f;
    constexpr float ProjectileRadius = 5.0f;
    constexpr int ProjectileDamage = 1;

    constexpr float WeaponCooldown = 0.3f;

    constexpr float DashDistance = 120.0f;
    constexpr float DashCooldown = 0.8f;

    constexpr float NovaRadius = 110.0f;
    constexpr int NovaDamage = 2;
    constexpr float NovaCooldown = 1.5f;
    constexpr float NovaEffectDuration = 0.25f;
    constexpr float SecondarySkillCooldown = 1.0f;

    constexpr float EnemySpeed = 120.0f;
    constexpr float EnemyRadius = 20.0f;
    constexpr int EnemyHp = 1;
    constexpr int EnemyContactDamage = 1;

    constexpr float EnemySpawnInterval = 1.0f;

    constexpr float ExpOrbRadius = 8.0f;
    constexpr int ExpPerKill = 1;
    constexpr float PickupRange = 50.0f;
    constexpr int BaseExpToLevel = 5;

    constexpr float ItemDropRadius = 7.0f;
    constexpr float ItemPickupRange = 32.0f;
    constexpr int ItemDropChancePercent = 35;

    constexpr int MapWaveCount = 3;
    constexpr int BaseEnemiesPerWave = 6;
    constexpr int EnemiesPerMapLevel = 2;
}
