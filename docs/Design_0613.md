# PlaneShooter 内容设计 v1

> 设计日期：2026-06-13
> 约束：只使用现有 Stats（maxHp / moveSpeedMultiplier / damageMultiplier / attackSpeedMultiplier / pickupRangeMultiplier）和现有技能释放类型（Projectile / SelfCenteredArea / MouseTargetedArea / Dash）。

---

## 任务 1：8 个主动技能设计

分布：Projectile ×1、MouseTargetedArea ×3、SelfCenteredArea ×3、Dash ×1。
不设计召唤物、持续地面效果、复杂寻路技能。

| SkillName | Slot | CastType | Cooldown | BaseDamage | Radius/Speed | Description | Visual | Balance note |
|---|---|---|---|---|---|---|---|---|
| **Spread Shot** | Primary | Projectile | 0.35s | 1 | Speed 500, 3 projectiles in 30° cone | 向前方扇形射出 3 发低伤害弹丸。 | 黄色锥形弹丸同时飞出。 | DPS 略高于 Bolt，但分散伤害对单体效率低。 |
| **Flare** | Secondary | MouseTargetedArea | 1.0s | 2 | Radius 85 | 在鼠标位置瞬间引爆小范围火焰。 | 橙红色爆点 + 快速扩散环。 | 现有 Secondary 技能，高命中但范围小。 |
| **Meteor** | Secondary | MouseTargetedArea | 1.4s | 4 | Radius 110 | 鼠标位置延迟 0.5s 后坠落陨石造成高伤害。 | 地面先出现红圈预警，再落下火球。 | 高伤害但需要预判，对移动目标难命中。 |
| **Frost Bomb** | Secondary | MouseTargetedArea | 1.2s | 2 | Radius 100 | 鼠标位置投掷冰霜炸弹，命中敌人造成范围伤害。 | 蓝色冰雾爆开。 | 中等范围、中等冷却，可靠清群。 |
| **Nova** | Utility | SelfCenteredArea | 1.5s | 2 | Radius 110 | 以自身为中心释放冲击波。 | 绿色圆环从玩家向外扩散。 | 现有 Utility 技能，近战解围用。 |
| **Pulse** | Utility | SelfCenteredArea | 2.0s | 3 | Radius 160 | 以自身为中心释放高伤害大范围脉冲。 | 紫色能量球先收缩再爆发。 | 高冷却高伤害，适合被包围时使用。 |
| **Bladestorm** | Utility | SelfCenteredArea | 1.0s | 1 | Radius 70 | 以自身为中心快速切割周围敌人。 | 白色刀光环绕玩家旋转。 | 低伤害短冷却，可频繁使用。 |
| **Dash** | Movement | Dash | 0.8s | 0 | Distance 120 | 向鼠标方向瞬移一段距离。 | 玩家留下残影。 | 现有 Movement 技能，用于脱战或赶路。 |

### 升级方向（数值化）
- Spread Shot：+1 弹丸数 / +15° 扩散角 / +20% 弹丸速度。
- Flare：+15% 半径 / +25% 伤害 / -10% 冷却。
- Meteor：-20% 延迟 / +20% 半径 / +30% 伤害。
- Frost Bomb：+20% 半径 / +25% 伤害 / -15% 冷却。
- Nova：+15% 半径 / +25% 伤害 / -10% 冷却。
- Pulse：+20% 半径 / +30% 伤害 / -15% 冷却。
- Bladestorm：+20% 半径 / +20% 伤害 / -10% 冷却。
- Dash：+25% 距离 / -15% 冷却 / 落地时产生小范围伤害（可选，非必须）。

---

## 任务 2：天赋盘 v1（20 节点 / 4 分支）

每个节点只影响现有 Stats：HP、DMG、AS、MS、Pickup。
前置节点形成线性链，每个节点需要前一级已点亮。

### Projectile 分支（伤害 / 攻速）

| NodeId | Name | Description | Stats | Prerequisite | Branch |
|---|---|---|---|---|---|
| P1 | Sharpened Bolt | +8% 伤害 | DMG ×1.08 | -1 | Projectile |
| P2 | Rapid Fire | +6% 攻速 | AS ×1.06 | P1 | Projectile |
| P3 | Lethal Force | +10% 伤害 | DMG ×1.10 | P2 | Projectile |
| P4 | Quick Reload | +8% 攻速 | AS ×1.08 | P3 | Projectile |
| P5 | Annihilation | +12% 伤害 | DMG ×1.12 | P4 | Projectile |

### Area 分支（伤害 / 生命）

| NodeId | Name | Description | Stats | Prerequisite | Branch |
|---|---|---|---|---|---|
| A1 | Inner Blaze | +8% 伤害 | DMG ×1.08 | -1 | Area |
| A2 | Thick Skin | +4 最大生命 | HP +4 | A1 | Area |
| A3 | Blast Radius | +10% 伤害 | DMG ×1.10 | A2 | Area |
| A4 | Sturdy Frame | +5 最大生命 | HP +5 | A3 | Area |
| A5 | Cataclysm | +12% 伤害 | DMG ×1.12 | A4 | Area |

### Survival 分支（生命 / 移速）

| NodeId | Name | Description | Stats | Prerequisite | Branch |
|---|---|---|---|---|---|
| S1 | Vigour | +5 最大生命 | HP +5 | -1 | Survival |
| S2 | Swift Foot | +6% 移速 | MS ×1.06 | S1 | Survival |
| S3 | Iron Heart | +6 最大生命 | HP +6 | S2 | Survival |
| S4 | Wind Runner | +8% 移速 | MS ×1.08 | S3 | Survival |
| S5 | Unyielding | +8 最大生命 | HP +8 | S4 | Survival |

### Loot 分支（拾取 / 移速）

| NodeId | Name | Description | Stats | Prerequisite | Branch |
|---|---|---|---|---|---|
| L1 | Scavenger | +15% 拾取范围 | Pickup ×1.15 | -1 | Loot |
| L2 | Hoarder | +10% 拾取范围 | Pickup ×1.10 | L1 | Loot |
| L3 | Lucky Step | +5% 移速 | MS ×1.05 | L2 | Loot |
| L4 | Far Reach | +15% 拾取范围 | Pickup ×1.15 | L3 | Loot |
| L5 | Magnetism | +20% 拾取范围 | Pickup ×1.20 | L4 | Loot |

---

## 任务 3：装备词缀池

每个装备槽至少 6 条词缀，只影响现有 Stats。
前缀通常提供攻击/防御属性，后缀通常提供速度/功能属性。
词缀档位按 `mapLevel = 1 / 3 / 5` 设计。

### Weapon 词缀

| AffixName | PrefixOrSuffix | Slot | Stats | Tier1 (mlvl 1) | Tier2 (mlvl 3) | Tier3 (mlvl 5) |
|---|---|---|---|---|---|---|
| Vicious | Prefix | Weapon | damageMultiplier | ×1.08 | ×1.14 | ×1.20 |
| Serrated | Prefix | Weapon | damageMultiplier | ×1.06 | ×1.10 | ×1.14 |
| Swift | Prefix | Weapon | attackSpeedMultiplier | ×1.05 | ×1.09 | ×1.13 |
| of Force | Suffix | Weapon | damageMultiplier | ×1.04 | ×1.08 | ×1.12 |
| of Swiftness | Suffix | Weapon | attackSpeedMultiplier | ×1.04 | ×1.07 | ×1.10 |
| of Piercing | Suffix | Weapon | damageMultiplier | ×1.03 | ×1.06 | ×1.09 |

### Armor 词缀

| AffixName | PrefixOrSuffix | Slot | Stats | Tier1 | Tier2 | Tier3 |
|---|---|---|---|---|---|---|
| Sturdy | Prefix | Armor | maxHp | +4 | +8 | +12 |
| Reinforced | Prefix | Armor | maxHp | +3 | +6 | +9 |
| Plated | Prefix | Armor | maxHp | +2 | +5 | +8 |
| of Vitality | Suffix | Armor | maxHp | +2 | +4 | +6 |
| of Haste | Suffix | Armor | moveSpeedMultiplier | ×1.04 | ×1.07 | ×1.10 |
| of Reach | Suffix | Armor | pickupRangeMultiplier | ×1.08 | ×1.14 | ×1.20 |

### Ring 词缀

| AffixName | PrefixOrSuffix | Slot | Stats | Tier1 | Tier2 | Tier3 |
|---|---|---|---|---|---|---|
| Glinting | Prefix | Ring | damageMultiplier | ×1.05 | ×1.09 | ×1.13 |
| Agile | Prefix | Ring | attackSpeedMultiplier | ×1.04 | ×1.07 | ×1.10 |
| Runner's | Prefix | Ring | moveSpeedMultiplier | ×1.05 | ×1.08 | ×1.11 |
| of Vitality | Suffix | Ring | maxHp | +2 | +4 | +6 |
| of Swiftness | Suffix | Ring | attackSpeedMultiplier | ×1.03 | ×1.06 | ×1.09 |
| of Haste | Suffix | Ring | moveSpeedMultiplier | ×1.03 | ×1.06 | ×1.09 |

### Amulet 词缀

| AffixName | PrefixOrSuffix | Slot | Stats | Tier1 | Tier2 | Tier3 |
|---|---|---|---|---|---|---|
| Blessed | Prefix | Amulet | maxHp | +3 | +6 | +9 |
| Radiant | Prefix | Amulet | damageMultiplier | ×1.06 | ×1.10 | ×1.14 |
| Gilded | Prefix | Amulet | pickupRangeMultiplier | ×1.10 | ×1.16 | ×1.22 |
| of Vitality | Suffix | Amulet | maxHp | +2 | +4 | +6 |
| of Haste | Suffix | Amulet | moveSpeedMultiplier | ×1.05 | ×1.08 | ×1.11 |
| of Reach | Suffix | Amulet | pickupRangeMultiplier | ×1.08 | ×1.12 | ×1.16 |

---

## 任务 4：Boss 设计 v1

所有 Boss 使用现有追踪移动（向玩家位置直线移动）。
每个 Boss 2 个技能，技能类型仅限：圆形 AoE、直线投射物、冲刺、召唤普通怪。

### Boss 1：Brimstone Colossus（火焰巨像）

| Field | Value |
|---|---|
| BossName | Brimstone Colossus |
| Theme | 熔岩与岩石，缓慢但高血量 |
| HPMultiplier | ×24 基础地图怪物 HP |
| DamageBonus | +2 接触伤害 |
| Skill1 | **Magma Slam** — 玩家当前位置圆形 AoE，半径 130，造成 2 点伤害，有 0.4s 红圈预警。 |
| Skill2 | **Flame Charge** — 向玩家方向冲刺 200 距离，路径上留下火焰轨迹（仅视觉，不持续伤害），冲刺中触碰玩家造成 2 点伤害。 |
| DropMultiplier | ×3.5 |
| Arena note | Boss 竞技场中央有少量岩石障碍，限制玩家走位但不阻挡子弹。 |

### Boss 2：Storm Herald（风暴使者）

| Field | Value |
|---|---|
| BossName | Storm Herald |
| Theme | 雷电云团，移动速度较快 |
| HPMultiplier | ×18 基础地图怪物 HP |
| DamageBonus | +1 接触伤害 |
| Skill1 | **Lightning Spear** — 向玩家发射 1 条高速直线投射物，宽 12、长 600，造成 2 点伤害。 |
| Skill2 | **Thundercall** — 以自身为中心圆形 AoE，半径 150，造成 3 点伤害并短暂击退附近玩家（可选视觉表现，机制上保持为纯伤害）。 |
| DropMultiplier | ×3.0 |
| Arena note | 开阔圆形场地，强调走位躲避直线闪电。 |

### Boss 3：Brood Matriarch（虫群母皇）

| Field | Value |
|---|---|
| BossName | Brood Matriarch |
| Theme | 昆虫巢穴，持续召唤小怪 |
| HPMultiplier | ×20 基础地图怪物 HP |
| DamageBonus | +1 接触伤害 |
| Skill1 | **Acid Spray** — 向玩家方向扇形喷射 3 条直线投射物，每条造成 1 点伤害。 |
| Skill2 | **Spawn Brood** — 在 Boss 周围召唤 3 只普通怪物，每 8s 可释放一次。 |
| DropMultiplier | ×3.2 |
| Arena note | 场地边缘有虫卵装饰，召唤的小怪从边缘刷新。 |

---

## 任务 5：代码实现计划

选择落地技能：**Spread Shot**（Primary，Projectile）。
改动范围：
1. `include/Skill.hpp`：在 `SkillDefinition` 中增加 `projectileCount` 与 `spreadAngle`，用于数据化描述扇形弹丸。
2. `include/Config.hpp`：新增 `SpreadShotProjectileCount`、`SpreadShotSpreadAngle`。
3. `include/SkillBar.hpp`：将 Primary 默认技能改为 Spread Shot，并填充新增字段。
4. `src/GameWorld.cpp`：在 `tryCastPrimarySkill` 中读取 `projectileCount` 与 `spreadAngle`，扇形发射多枚弹丸。

> 说明：Spread Shot 只改变主技能发射逻辑，不改 GameWorld 主流程；通过 SkillDefinition / SkillBar 增加数据，并在 `tryCastPrimarySkill` 内扩展单个小分支实现，符合“弱 agent 不能乱改 GameWorld”的边界要求。
