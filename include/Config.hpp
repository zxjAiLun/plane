#pragma once

namespace Config {
    constexpr int WindowWidth = 800;
    constexpr int WindowHeight = 600;

    constexpr float PlayerSpeed = 300.0f;
    constexpr float PlayerRadius = 20.0f;
    constexpr int PlayerHp = 3;

    constexpr float BulletSpeed = 500.0f;
    constexpr float BulletRadius = 5.0f;
    constexpr int BulletDamage = 1;

    constexpr float ShootCooldown = 0.2f;

    constexpr float EnemySpeed = 120.0f;
    constexpr float EnemyRadius = 20.0f;
    constexpr int EnemyHp = 1;
    constexpr int EnemyContactDamage = 1;

    constexpr float EnemySpawnInterval = 1.0f;
}
