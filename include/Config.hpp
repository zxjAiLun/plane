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

    constexpr float EnemySpeed = 120.0f;
    constexpr float EnemyRadius = 20.0f;
    constexpr int EnemyHp = 1;
    constexpr int EnemyContactDamage = 1;

    constexpr float EnemySpawnInterval = 1.0f;

    constexpr float ExpOrbRadius = 8.0f;
    constexpr int ExpPerKill = 1;
    constexpr float PickupRange = 50.0f;
    constexpr int BaseExpToLevel = 5;

    constexpr float VictoryTime = 60.0f;
}
