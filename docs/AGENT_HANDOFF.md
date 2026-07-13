# Mini ARPG 项目接手与开发手册

更新日期：2026-07-13

玩法代码基线：`e310894 Improve 800px ARPG HUD readability`

本文档由主 review Agent 维护；代码与测试基线以当前 Git HEAD 为准。

目标：完成一个具有《流放之路 2》核心味道的单机 Mini ARPG 原型，而不是复刻完整商业游戏。

## 1. 文档用途与真相来源

本文档用于让新的开发 Agent 快速理解项目、领取单个开发任务并提交可 review 的产物。

真相来源按以下优先级排列：

1. 当前工作区源码和 Git HEAD。
2. `tests/arpg_logic_tests.cpp` 中覆盖的实际行为。
3. 本文档。
4. `docs/Design_0613.md` 等早期设计文档。
5. 历史聊天、旧计划和 commit message。

旧设计文档可以提供灵感，但不能覆盖当前代码行为。开始任务前必须先运行 `git status --short`、阅读相关模块并搜索现有实现，禁止仅凭旧计划重复造功能。

### 1.1 协作角色与完成门禁

- 项目负责人（用户）：决定产品方向、任务优先级和玩法取舍。
- 实现 Agent（hy3）：一次只实现一个已批准任务，完成代码、测试和手动验证，保持改动未提交。
- 主 review Agent（Codex）：维护路线图和项目进度，审查完整 diff，修复行为/架构/风格问题，重新验证并创建最终 commit。

hy3 报告“实现完成”只代表进入 review，不代表任务完成。只有主 review Agent 验收、必要修正、构建测试通过并提交后，任务才进入进度看板的已完成状态。hy3 不应自行扩展任务范围、修改路线图状态或把未 review 的实现作为下一任务基线。

## 2. 一分钟理解项目

这是一个 C++17 + SFML 3.1 的俯视角单机 ARPG 原型。玩家在 `2400 x 1800` 的世界坐标地图中移动，通过鼠标瞄准和主动技能战斗，探索地图事件，进入 Boss 区域，击败 Boss，拾取装备，选择成长奖励和下一张带风险词缀的地图。

当前主循环：

```mermaid
flowchart LR
    A[进入地图] --> B[探索开放区域]
    B --> C[宝箱 / 祭坛 / 精英包]
    C --> D[到达 Boss Gate]
    D --> E[进入 Boss Arena]
    E --> F[击败 Boss]
    F --> G[拾取地面战利品]
    G --> H[选择技能或 Support 奖励]
    H --> I[三选一下一张地图]
    I --> J[装备 / 天赋 / 技能调整]
    J --> A
```

项目已经不是固定屏幕的 Survivor 游戏。以下旧方向已经明确废弃：

- 不使用经验球，击杀后经验直接入账。
- 不使用升级后三选一的阻塞弹窗。
- 升级获得 SP，SP 用于天赋盘。
- 装备先掉在地面，由玩家按 `F` 拾取。
- 地图完成条件是击败 Boss，不是清固定波次计时结束。

## 3. 构建、测试与运行

### 3.1 环境

- Windows
- Visual Studio 2019 BuildTools
- MSVC amd64
- CMake 3.22+
- 当前 `build/` 使用 NMake Makefiles
- SFML 3.1 由 CMake `FetchContent` 获取

首次 configure 需要网络访问 SFML 仓库。已有 `build/` 时一般只需构建。

### 3.2 标准构建

在普通 PowerShell 中运行：

```powershell
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=amd64 >nul 2>&1 && cmake --build build"
```

成功标准：

```text
[100%] Built target PlaneShooter
[100%] Built target arpg_logic_tests
[100%] Built target arpg_save_tests
[100%] Built target arpg_world_tests
```

### 3.3 测试

```powershell
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=amd64 >nul 2>&1 && ctest --test-dir build --output-on-failure"
```

当前测试基线：`arpg_logic_tests 958 passed / 0 failed`，`arpg_save_tests 11 passed / 0 failed`，`arpg_world_tests 254 passed / 0 failed`。

NMake 在本项目中偶尔不会因纯头文件变更正确重编目标。修改以下 header-only 数据表或计算模块后，最终验收必须至少执行一次全量构建：

```powershell
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=amd64 >nul 2>&1 && cmake --build build --clean-first && ctest --test-dir build --output-on-failure"
```

容易触发该问题的文件包括：

- `SkillLibrary.hpp`
- `SupportLibrary.hpp`
- `PassiveTree.hpp`
- `LootGenerator.hpp`
- `MapModifier.hpp`
- `MapInstance.hpp`
- `MapLayout.hpp`
- `MapExploration.hpp`
- `BossDefinition.hpp`
- `RandomService.hpp`
- `CombatMath.hpp`

### 3.4 运行

```powershell
.\build\PlaneShooter.exe
```

### 3.5 每轮最低验证

```powershell
git diff --check
cmake --build build
ctest --test-dir build --output-on-failure
git status --short
```

涉及 UI、输入、地图流程或战斗时，还需要手动运行可执行文件做对应 smoke test。单元测试通过不能证明 UI 没有重叠，也不能证明完整地图流程可玩。

## 4. 当前输入与上下文规则

输入是上下文相关的。新增按键前必须检查 `Input` 和 `GameWorld::update()`，不能让同一个按键在同一状态触发两个动作。

| 输入 | Playing | Passive Tree | Skill Panel | MapComplete | Paused |
|---|---|---|---|---|---|
| `WASD` / 方向键 | 移动 | 仍可移动 | 仍可移动 | 不处理战斗移动 | 不处理 |
| 左键按住 | Primary 技能 | 点击天赋节点，不射击 | 不射击 | 不射击 | 不处理 |
| 右键 | Secondary 技能 | 不施法 | 不施法 | 不施法 | 不处理 |
| `Q` | Utility 技能 | 不施法 | 不施法 | 不施法 | Quit |
| `Space` | Movement / Dash | 不施法 | 不施法 | 不施法 | 不处理 |
| `G` | 使用生命药瓶 | 不处理 | 不处理 | 不处理 | 不处理 |
| `F` | 优先交互地图事件，否则拾取最近物品 | 无 | 无 | 拾取 Boss 战利品 | 不处理 |
| `1-9` | 装备对应背包物品 | `1-0` 点节点 1-10 | `1-8` 分配技能 | 按阶段选择奖励或下一图 | 不处理 |
| `F1-F10` | 无普通战斗语义 | 点节点 11-20 | `F1-F4` 切换四个槽位 Support | 无 | 不处理 |
| `F5` / `F9` | 保存 / 加载当前 run | 天赋节点 5 / 9 | 不处理 | 保存 / 加载当前 run | Save / Load |
| `P` | 打开天赋盘 | 关闭天赋盘 | 打开时会关闭 Skill Panel | 禁止打开 | 不处理 |
| `K` | 打开技能面板 | 打开时会关闭天赋盘 | 关闭技能面板 | 禁止打开 | 不处理 |
| `Tab` | 循环选择背包物品 | 不处理 | 不处理 | 循环选择背包物品 | 不处理 |
| `Delete` | 丢弃选中物品 | 不处理 | 不处理 | 丢弃选中物品 | 不处理 |
| `C` | 分解选中物品 | 不处理 | 不处理 | 分解选中物品 | 不处理 |
| `V` | 打开/关闭锻造面板 | 不处理 | 不处理 | 打开/关闭锻造面板 | 不处理 |
| `I` | 无 | 无 | 无 | 将选中的背包物品移入 Stash | 不处理 |
| `O` | 无 | 无 | 无 | 将选中的 Stash 物品移回背包 | 不处理 |
| `E` | 无 | 无 | 无 | 奖励和地图均选完后进入下一图 | 不处理 |
| `R` | 无 | 无 | 无 | 重新开始 run | Restart |
| `Esc` | 关闭子面板或打开 Pause | 关闭天赋盘 | 关闭技能面板 | 关闭结算子面板或打开 Pause | Continue |

重要约束：

- Playing 中 `F` 的事件交互优先级高于拾取。
- 拾取目标必须使用 `GameWorld::focusedDroppedItemIndex()`，显示目标和实际拾取目标必须一致。
- MapComplete 中数字键绝不能装备物品。
- MapComplete 中允许 `F` 拾取、`Tab` 在 Inventory/Stash 间循环选择、`Delete` 丢弃、`C` 分解、`V` 锻造，以及 `I/O` 双向移动物品。
- `Delete`、`C`、`V` 只作用于当前选中的背包物品；选中 Stash 物品时不会误删、分解或锻造仓库物品。
- `I/O` 只在 MapComplete 生效，Playing、Passive Tree、Skill Panel 和 Crafting 上下文均忽略。
- `R` 在 GameOver 和 MapComplete 中都会执行完整 `reset()`。
- 天赋盘和技能面板互斥。
- 新面板必须明确关闭战斗输入，不能只在 Renderer 中遮住画面。

## 5. 已实现功能清单

### 5.1 世界、地图与相机

- 世界坐标地图，默认尺寸 `2400 x 1800`。
- 相机跟随玩家，世界对象经过 world-to-screen 转换，HUD 使用屏幕坐标。
- 玩家移动受地图边界与矩形障碍阻挡。
- 三套数据化地图模板，包含名称、配色、障碍布局、事件布局和遭遇权重。
- Start、Field、BossGate、BossArena、BossDefeated 区域状态。
- 小地图显示地图边界、玩家、出生区、Boss 区域、Gate 和地图事件。
- HUD 显示当前目标和 Boss 距离。

### 5.2 地图事件

每张地图包含三种一次性事件：

- Loot Cache：靠近后按 `F`，掉落两件装备。
- Shrine：靠近后按 `F`，获得 20 秒 `+35% damage`。
- Elite Pack：进入范围自动生成 1 Elite + 4 Normal，全部击杀后完成。

已有事件提示、短状态消息、剩余敌人数、小地图完成态和 MapComplete 事件统计。Boss 战触发后未完成事件停止响应。

### 5.3 怪物与战斗 AI

已有五类 EnemyType：

- Normal：近战追踪和带前摇攻击。
- Ranged：保持攻击距离并发射敌方投射物。
- Elite：更高奖励，可附带精英 modifier。
- Boss：独立 BossDefinition 和技能循环。
- Charger：前摇提示后沿锁定方向冲锋，每次冲锋只命中一次。

精英 modifier：

- Hardened：更高生命。
- Swift：更高移动速度。
- Volatile：死亡后范围爆炸。

地图模板与地图 modifier 都能改变普通、远程、精英、冲锋怪的遭遇权重。

### 5.4 Boss

已有三个数据化 Boss：

- Brimstone Colossus
- Storm Herald
- Brood Matriarch

已实现：

- 独立 HP、伤害、技能间隔、掉落倍率和保底掉落。
- Circular AoE、Projectile、SummonAdds 和 Dash 四种技能类型。
- 技能顺序数据化。
- 低血量 Enrage 阶段，改变技能顺序、间隔和伤害。
- Brood Matriarch 普通阶段召唤近战幼体，Enrage 阶段加入远程幼体。
- 召唤物按当前地图等级和 modifier 缩放，数量有上限；Boss 死亡时统一清理。
- Brimstone Colossus 的 Magma Slam 留下持续火区，按间隔造成伤害。
- 火区使用世界坐标，可同时存在多个，显示危险边界、tick 节奏和剩余时间。
- Storm Herald 的 Tempest Rush 在前摇开始时锁定目标，显示完整路径和落点后短距离突进。
- Tempest Rush 经过地图边界/障碍解析，每次施放最多命中一次；Chill 只降低其移动速度，不缩短前摇。
- Boss 名称、血条、技能预警、阶段信息。
- 三种主题 Boss relic，分别偏 Weapon、Ring、Amulet 构筑。
- Boss 死亡后进入 MapComplete，并生成奖励和下一图选项。

三个 Boss 已各有独立机制：Brood 召唤增援，Brimstone 制造持续区域危险，Storm 通过带锁定预警的位移改变站位。Milestone A 已完成；后续 Boss 内容先暂停，优先补玩家资源约束和构筑深度。

### 5.5 玩家、生存与成长

- WASD/方向键移动，鼠标瞄准，主动施法。
- 生命、最大生命、护甲和受击无敌时间。
- 受击伤害、来源和受击视觉反馈。
- 生命药瓶：`G` 使用，最多 3 充能，击杀普通怪概率恢复，Elite 固定恢复，Boss 回满。
- 击杀直接获得经验。
- 升级后 SP +1，不暂停游戏。
- SP 只能通过 `Player::spendPassivePoint()` 消费。
- 玩家拥有 100 Mana，按时间恢复；技能消耗 Mana，资源不足时不会施法或启动 cooldown。
- Primary 使用低消耗，Secondary/Utility 使用中高消耗，Dash 不消耗 Mana。
- `reset()` 重置整局成长，`startNextMap()` 保留单局成长。

### 5.6 天赋盘

- 20 个节点。
- Projectile、Area、Survival、Loot 四条分支，每条 5 节点。
- Small / Notable 两种节点大小。
- 节点位置、分支、前置关系数据化。
- 鼠标 hover、点击分配、锁定/可点/已分配状态。
- 数字键与 `F1-F10` 调试/键盘路径。
- HUD build summary。

分支实际效果：

- Projectile：投射物伤害、攻速；末端 Keystone `Volley Doctrine` 额外发射 2 枚投射物，但投射物伤害降低 25%。
- Area：范围伤害、范围半径；末端 Keystone `Concentrated Impact` 范围伤害提高 35%，范围半径降低 25%。
- Survival：HP、护甲、移速。
- Survival 末端 Keystone `Second Wind` 使生命药瓶治疗量提高 50%，不增加最大生命或充能。
- Loot：拾取范围、少量移速；末端 Keystone `Loaded Dice` 使物品掉落数量/概率提高 25%，但承受伤害提高 20%。

Keystone 使用 `PassiveKeystone` 数据字段和 `Stats` 聚合，不依赖节点名称字符串。投射物数量、范围伤害/半径、药瓶治疗、掉落概率/数量和承受伤害均通过 `CombatMath` 或 GameWorld 的真实路径生效；HUD build summary 和天赋 hover 会显示 Keystone 名称及收益/代价。

### 5.7 技能与技能栏

四个固定技能槽位：

- Primary
- Secondary
- Utility
- Movement

已有八个技能：

- Spread Shot
- Flare
- Meteor
- Frost Bomb
- Nova
- Pulse
- Bladestorm
- Dash

已有四种施法类型：

- Projectile
- MouseTargetedArea
- SelfCenteredArea
- Dash

技能面板使用 `K` 打开。默认解锁 Spread Shot、Meteor、Pulse、Dash；其他技能通过 Boss 结算奖励解锁。未解锁技能可预览但不能装备。

每个主动技能定义包含 Mana cost。HUD 和 Skill Bar/Skill Panel 显示当前 Mana、技能消耗和实际 cooldown。

### 5.8 Support 构筑

每个技能槽位最多装备一个 Support。已有七个 Support：

- Pierce：投射物穿透。
- Amplify：扩大范围并增加冷却。
- Quickcast：降低冷却但降低伤害。
- Volley：增加投射物数量和散射，降低伤害。
- Trailblazer：Dash 落点产生范围伤害。
- Combustion：降低直接命中，强化 Ignite DOT 和持续时间。
- Deep Chill：延长并加深 Chill，增加冷却。

Support 兼容性由 `SupportLibrary::supportsSkill()` 决定。所有技能解锁后，Boss 奖励开始提供未解锁 Support。`F1-F4` 分别循环四个槽位的可用 Support。

### 5.9 状态异常

已有：

- Ignite：按实际命中伤害计算周期伤害。
- Chill：降低普通移动、冲锋移动和 Boss 移动速度。

Flare、Meteor 施加 Ignite；Frost Bomb 施加 Chill。状态会在施放时生成快照，飞行中的投射物不会因之后切换 Support 而改变。DOT 击杀复用正常的经验、掉落、地图统计、Elite 事件和 Boss 结算路径。

Renderer 已显示异常颜色和敌人状态环，Skill Panel 显示有效异常持续时间与强度。

### 5.10 装备、掉落与背包

装备槽位：

- Weapon
- Armor
- Ring
- Amulet

稀有度：

- Normal
- Magic
- Rare

已有能力：

- 怪物死亡在地面生成 DroppedItem，经验不落地。
- 地图等级提高 Rare 概率和 affix tier。
- T1 / T2 / T3 词缀。
- 每个槽位三个普通 Base，包含固定 implicit；Boss relic 使用主题专属 Base。
- 词缀名称、Prefix/Suffix、物品名和实际 Stats 分离。
- 全局伤害、攻速、移速、拾取范围、HP、护甲、Projectile、Area damage/radius 等词缀。
- Boss 主题专属 Rare relic。
- 9 格背包，与 `1-9` 装备键一一对应。
- 满包时地面物品不消失。
- `F` 只拾取范围内最近物品。
- 地面目标高亮、名称提示、满包提示。
- 背包 hover/selected 详情、当前同槽装备、属性差值和技能实际影响预览。
- 装备替换后旧装备返回背包，极端满包情况掉到玩家脚下。
- `Tab` 选择、`Delete` 丢弃。
- `C` 分解获得 Forge Fragments。
- `V` 打开锻造面板；数字键选择 Improve / Reroll / Raise Tier，`F1-F3` 选择目标词缀。
- 锻造只在成功操作后消耗 `Forge Fragments`；失败时 Item、资源和选中索引保持不变。
- `ImproveAffix` 只改一条词缀 contribution，`RerollAffix` 只替换同组词缀，`RaiseAffixTier` 只提升一档合法 Tier。
- `ItemAffix` 保存稳定 id、词缀类型、prefix/suffix、tags、tier 和实际 contribution；Item stats 由 implicit + affix contributions 统一重建。
- 9 格 Inventory 满包时保留地面掉落；MapComplete 提供 24 格 Stash，供地图之间保留完整 Item。
- `Tab` 在 MapComplete 的 Inventory/Stash 项目之间循环选择；`I` 存入 Stash，`O` 取回 Inventory。
- Stash 满或 Inventory 满时移动失败且源 Item 不变；`reset()` 清空 Stash，`startNextMap()` 保留 Stash。

### 5.11 地图选择与奖励

- 第一张图使用默认普通地图。
- Boss 击杀后先三选一角色奖励，再三选一下一张地图。
- 奖励优先解锁技能；技能全部解锁后优先解锁 Support；之后使用全局伤害、HP、Future Item Quantity fallback。
- 下一图选项包含地图模板、名称、描述、推荐等级、怪物 HP/伤害、掉落倍率、Boss 风险、Elite 压力和 item level bonus。
- 当前 HUD 显示地图名和 modifier 摘要。
- 每张地图还绑定一个可复现的预制布局变体，HUD 显示 `LAYOUT N/3`；布局不会使用当前时间或 Renderer 随机生成。
- MapComplete 允许先整理背包和拾取 Boss 战利品，再按 `E` 进入下一图。

### 5.12 测试现状

`tests/arpg_logic_tests.cpp` 当前覆盖：

- 天赋前置与属性聚合。
- Skill/Support 槽位兼容性。
- 伤害、范围、冷却、穿透、Volley、Trailblazer 计算。
- Combustion、Deep Chill、Ignite、Chill。
- 护甲减伤、装备影响战斗属性。
- Loot rarity、affix 数量、tier、Boss relic。
- Item Base 数量、唯一 id、implicit + affix 聚合和 Boss theme Base。
- AffixTag/weight 数据、地图/Boss loot bias、候选过滤、重复 AffixStat 防护和固定种子选择。
- CraftingOperation 三种操作、稳定 affix id、Tier contribution 重建、重铸候选过滤和失败回滚规则。
- Inventory/Stash 容量、完整 Item 所有权、满容器失败保护和插入顺序恢复。
- Stash 的 GameWorld 双向移动由 MapComplete 输入上下文和 Renderer 代码路径接入；端到端状态切换仍需手动验证。
- Elite modifier。
- Charger 状态机。
- Boss summon 数据、阶段顺序和数量上限。
- GroundHazard 数据、tick 生命周期和 Brimstone 火区配置。
- BossDash 目标快照、前摇、移动、单次命中、完成消费和 Storm 技能顺序。
- Boss 位移经过地图边界与障碍解析。
- Mana 初始值、消耗、恢复、上限 clamp、八个技能 cost 和资源不足时 cooldown 不变。
- 四个 Passive Keystone 的类型、前置、重复分配、收益/代价、投射物/范围/药瓶/掉落/承伤实际计算。
- 药瓶充能奖励。
- 地图选项差异和风险缩放。
- 地图奖励生成。
- 三个 MapTemplate 的三套布局变体、稳定布局 id、边界/事件交互空间和 Start-to-Boss BFS 可达性。
- `SaveService` 的完整 Item/词缀/装备/技能 Support/Inventory/Stash/掉落 round-trip。
- 存档 magic/version/payload/CRC、RNG state round-trip、未知版本、截断、损坏、缺失文件和原子替换失败保护。

`RandomService` 与真实 GameWorld 随机流程还由以下测试覆盖：

- 同 seed/不同 seed、边界区间、权重选择、chance 和 seed 派生。
- `EnemySpawner` 的显式 RNG 注入与同/不同 seed 行为。
- 真实 `GameWorld` 敌人生成路径的同 seed 可复现，以及 `reset()` 的 seed 生命周期。

尚无自动化覆盖：

- SFML Renderer 布局。
- Input event 到 GameWorld 的完整集成。
- 从出生到 Boss 再到下一图的端到端流程。
- 地面拾取、满包、分解、强化的完整组合流程。
- 从真实窗口事件到 F5/F9 保存/加载的完整 UI 端到端流程。

## 6. 代码结构与职责边界

| 模块 | 当前职责 | 修改原则 |
|---|---|---|
| `Game.cpp` | SFML 窗口、事件循环、更新和渲染入口 | 不放玩法规则 |
| `Input.*` | 将 SFML 事件转换成按帧输入状态 | 不解释 GameState 语义 |
| `GameWorld.*` | 当前总编排器，连接地图、战斗、奖励、输入上下文 | 可做流程编排，避免继续塞纯数据表和可独立算法 |
| `Player.*` | 玩家生命、Mana、经验、SP、装备、天赋与最终属性 | 永久属性刷新必须走 `recalculateStats()`；Mana 只由 Player 持有 |
| `Enemy.*` | 单个敌人移动、攻击状态机、异常生命周期 | 不生成掉落、不直接修改地图奖励 |
| `EnemyDefinition.hpp` | 怪物类型数据 | 新怪先加数据，通用行为再加状态机 |
| `BossDefinition.hpp` | Boss、Boss 技能顺序和奖励主题数据 | Boss 差异优先数据化 |
| `BossDash.hpp` | Boss 突进定义与无 SFML 状态机 | 目标快照、移动和一次性命中/完成状态必须可纯逻辑测试 |
| `GroundHazard.hpp` | 持续地面危险定义、世界实例和 tick 生命周期 | 不直接修改 Player；伤害由 GameWorld 编排 |
| `MapInstance.hpp` | 地图模板、障碍、区域、事件实例和移动解析 | 不处理玩家输入 |
| `MapExploration.hpp` | 世界坐标到固定网格的探索揭示状态 | 只保存纯逻辑状态，不依赖 SFML 或 Renderer |
| `MapModifier.hpp` | 下一图选项和风险收益倍率 | 数值必须能在 GameWorld 中找到实际应用点 |
| `SkillLibrary.hpp` | 主动技能定义 | 不直接实现命中循环 |
| `SupportLibrary.hpp` | Support 数据和兼容性 | 新 Support 必须有可观察行为和测试 |
| `SkillBar.hpp` | 四槽技能/Support 分配、冷却 | 不负责解锁奖励 |
| `CombatMath.hpp` | 无状态、纯战斗计算 | 供 GameWorld、Renderer 预览和测试复用 |
| `Ailment.hpp` | 状态异常数据类型 | 不放 Enemy 生命周期状态 |
| `Affix.hpp` | 词缀 Stat 身份枚举 | 不使用显示名称作为逻辑身份 |
| `Crafting.hpp` | 锻造操作、结果和面板状态 | 不保存 Renderer 状态，不处理资源扣除 |
| `ItemBase.hpp` | 普通装备 Base、Boss relic Base 和 implicit 数据 | Base 选择必须由 LootGenerator 驱动，不让 Renderer 参与 |
| `LootBias.hpp` | 地图/Boss 对词缀标签的权重偏置 | 只保存数据，不读取 GameWorld 全局状态 |
| `LootGenerator.hpp` | rarity、base、implicit、affix、tier、标签权重、Boss relic 生成 | UI 文本不得参与数值计算 |
| `Equipment.hpp` | 装备 Item，并返回被替换 Item | 不允许静默吞掉旧装备 |
| `Inventory.hpp` | 有容量的 Item 容器 | 满包 add 返回 false，不产生副作用 |
| `Stash.hpp` | 当前 run 的 24 格 Item 仓库 | 只在 MapComplete 由 GameWorld 编排移动，不负责输入或 UI |
| `PassiveTree.hpp` | 20 节点数据、前置、命中查询、属性聚合 | SP 扣除仍由 Player 管理 |
| `MapRewardLibrary.hpp` | 技能/Support/fallback 奖励生成 | GameWorld 只过滤、抽取、应用 |
| `RandomService.hpp` | 单局 seed、确定性随机、边界与权重选择 | 玩法路径注入 `RandomService&`；兼容重载只能使用固定 legacy seed；Renderer 禁止调用 |
| `SaveData.hpp` | 不依赖 SFML 的稳定 run 存档数据 | 只保存可验证的值类型和完整 Item；不放 GameWorld/Renderer 指针 |
| `SaveService.*` | v1 存档编码、CRC、原子写入和读取校验 | 不修改 GameWorld；解析失败必须不产生半成品数据 |
| `Renderer.*` | 只读 GameWorld 并绘制世界和 UI | 禁止在 Renderer 中修改游戏状态或复制玩法公式 |
| `tests/arpg_logic_tests.cpp` | 无 SFML 的纯逻辑回归测试 | 新数据化规则必须补断言 |
| `tests/game_world_logic_tests.cpp` | 轻量真实 GameWorld 随机流程测试 | 只覆盖可稳定驱动的状态，不依赖窗口和渲染 |

当前技术债务：`GameWorld.cpp` 约 1577 行，`Renderer.cpp` 约 1514 行。不要为了“清理”一次性重写它们。新增独立机制时优先抽取小型纯逻辑类型或数据 Library；只有存在明确边界和测试时才拆大文件。

## 7. 必须保持的行为不变量

### 7.1 属性

- `Stats` 中乘数默认值必须是 `1.0f`，整数加值默认 `0`。
- 装备词缀通过 `stats.multiplier += value` 形成单件装备倍率，例如 `1.0 + 0.08`。
- 不同来源通过 `combineStats()` 相乘，HP/Armor 相加。
- 技能实际伤害使用 `CombatMath::skillDamage()`。
- Renderer 预览应复用 CombatMath，不能维护另一套逐渐漂移的公式。

### 7.2 击杀与奖励

- 每个敌人只能调用一次 `rewardEnemyKill()`。
- DOT、投射物、范围技能和未来召唤物的击杀都必须进入同一奖励路径。
- Boss 击杀必须触发 BossDefeated、清理 Boss 技能状态、生成地图奖励和下一图选项。
- ElitePack 事件敌人的死亡必须更新事件剩余计数。

### 7.3 装备所有权

- `Inventory::add()` 失败时调用方仍拥有 Item。
- `Equipment::equip()` 返回旧 Item。
- 任何装备、替换、分解、丢弃流程都不能静默销毁 Item。
- 满包拾取失败时地面物品必须保留。
- Stash 满或 Inventory 满时跨容器移动失败，源 Item、数量和索引必须保留。

### 7.4 世界与渲染

- Player、Enemy、Projectile、DroppedItem 均使用世界坐标。
- HUD、菜单、天赋盘、技能面板使用屏幕坐标。
- 世界移动必须通过 MapInstance 的边界/障碍解析。
- Renderer 只读，不驱动行为。

### 7.5 进度状态

- `reset()` 开始新 run，清空装备、天赋、解锁和 Future Item Quantity。
- `startNextMap()` 保留本 run 的装备、天赋、技能/Support 解锁和永久奖励。
- `reset()` 清空本 run Stash；`startNextMap()` 保留 Stash 中完整 Item 字段。
- 当前没有跨进程存档，不要假装已有持久化。

## 8. 代码风格和实现约束

后续 Agent 必须遵循以下风格：

- C++17。
- 4 空格缩进。
- 大括号与函数/条件同一行。
- 类型使用 `PascalCase`，函数和变量使用 `camelCase`，成员使用尾部 `_`。
- 枚举使用 `enum class`。
- 优先 `const`、引用、`std::optional` 和明确所有权。
- 手工编辑使用补丁方式，避免整文件机械重写。
- 只添加解释“为什么”的短注释，不写逐行翻译式注释。
- UI 文本继续使用英文，保持现有界面一致。
- 不引入新的第三方库，除非任务明确要求且先说明理由。
- 不做与任务无关的重命名、格式化或大规模重构。
- 新数据优先进入 `*Definition` / `*Library`，通用算法优先进入纯逻辑 helper。
- 新 switch 必须覆盖所有 enum 值。
- 不通过技能名字符串在 GameWorld 中分支行为。应使用 enum、tag 或数据字段。
- 不让 UI 展示字符串参与属性计算。
- 不复制 CombatMath 到 Renderer。
- 不恢复 ExperienceOrb、LevelUp 弹窗或升级三选一。
- 不提交 `.workbuddy/`、构建产物或 Agent 私有记忆文件。

## 9. Agent 单任务工作流

hy3 或其他实现 Agent 每次只领取一个边界清楚的任务。

开始前：

1. `git status --short`，确认基线和已有用户改动。
2. 阅读任务涉及的所有头文件和实现。
3. 使用 `rg` 搜索相同概念，确认没有现成实现。
4. 列出预计修改文件和不修改范围。

实现中：

1. 先改数据/接口，再改行为，再改 Renderer，最后补测试。
2. 保持已有输入上下文。
3. 不跨模块复制公式。
4. 对 Item、奖励、击杀等所有权路径做失败分支检查。

交付前：

1. `git diff --check`。
2. 标准构建。
3. `ctest --test-dir build --output-on-failure`。
4. header-only 核心变更执行 `--clean-first`。
5. UI/输入/流程变更手动运行 smoke test。
6. 保持改动未提交，交给主 review Agent 检查、修正和提交。

Agent 回复必须包含：

```text
任务：<任务名>

改动文件：
- <path>: <做了什么>

行为变化：
- <玩家可观察结果>

验证：
- git diff --check: pass/fail
- build: pass/fail
- ctest: pass/fail，测试数量
- manual smoke: 测了哪些路径

已知风险：
- <明确风险或“无”>

未触碰：
- <列出任务边界>

Git 状态：
- 保持未提交，等待 review
```

## 10. 主 review Agent 的验收职责

主 review Agent 负责最终质量和 commit，不直接相信实现 Agent 的自述。

每轮 review 顺序：

1. 检查 `git status` 和完整 diff，区分本轮改动与用户已有改动。
2. 先找行为 bug、所有权 bug、输入冲突和状态生命周期问题。
3. 检查数据是否真正作用于玩法，而不是只显示在 UI。
4. 检查 Renderer 是否复制了战斗公式。
5. 检查 enum/switch、新增默认值和 aggregate initializer 顺序。
6. 检查 UI 坐标、文本长度、面板遮挡和不同 GameState 层级。
7. 检查测试是否覆盖真实 shipped path，而不是复制实现做自证。
8. 必要时由主 Agent 直接修改。
9. 重新构建、测试，header 变更执行 clean build。
10. 通过后提交一个语义清楚的 commit，并确认工作区干净。

Review 严重级别：

- P0：崩溃、物品/进度丢失、重复奖励、输入完全不可用、Boss 无法完成。
- P1：核心数值未生效、状态不一致、明显玩法回归、UI 阻塞主流程。
- P2：边界情况、可读性、测试缺口、局部架构问题。
- P3：命名、注释和低风险样式问题。

## 11. PoE2-like 原型的完成定义

本项目的“完成”不是拥有 PoE2 的全部系统，而是以下体验形成稳定、可重复、可构筑的闭环：

1. 玩家能在多张地图中探索、战斗、处理事件并找到 Boss。
2. 普通怪、精英和 Boss 在攻击方式和风险上明显不同。
3. 玩家能通过装备、天赋、主动技能、Support 和状态异常形成至少三种可感知构筑。
4. 掉落物有可比较价值，背包满时不会丢失物品，Boss 奖励能推动下一轮成长。
5. 地图选择有明确风险收益，地图模板和 Boss 不只是换颜色。
6. 一次 run 可以连续推进多张地图，死亡、重开、结算状态稳定。
7. 核心成长可保存和恢复，至少支持单个本地存档。
8. 关键操作、伤害、危险预警、奖励和装备变化均有清晰反馈。
9. 构建、纯逻辑测试和关键手动流程有可重复验收方法。

当前已满足 1、2、7 的基础版，3、4、5、6 已形成可玩雏形但仍需深度和稳定性，8、9 部分完成。天赋 Keystone、异常抗性、装备 Base/implicit、词缀 tags/weights、地图/Boss 掉落偏置、选择式锻造、当前 run Stash 和可复现 RNG 已经让构筑出现第一层真实取舍；存档已经支持单 run 本地恢复，运行时地图生成和更完整的端到端验收仍未完成。

## 12. 后续路线图

路线按“先形成可玩的差异，再扩内容，再做持久化和打磨”排序。每一项应拆成一个独立 review/commit。

### Milestone A：Boss 战机制化

目标：三个 Boss 不再只是不同数值、投射物和 AoE 顺序。

任务 A1：Boss Summon Skill v1（完成：`1e959ee`）

- 新增 `BossSkillType::SummonAdds`。
- `BossSkillDefinition` 增加 summon type/count/radius 数据。
- Brood Matriarch 在普通阶段召唤 Normal，Enrage 阶段混入 Ranged 或 Charger。
- 召唤怪使用当前 map level、map modifier 和 EnemyDefinition 缩放。
- Boss 死亡时清理剩余召唤物，不能冻结在 MapComplete 世界中。
- HUD 显示召唤技能 warning。
- 测试 Boss 数据、召唤数量和技能顺序。
- 不新增寻路，不新增新 EnemyType。

任务 A2：Persistent Hazard v1（完成：`1a20767`）

- 新增数据化地面危险实例，包含位置、半径、持续时间、tick 间隔、伤害和来源。
- Brimstone 的 Magma 技能留下持续火区。
- Hazard 使用世界坐标，Renderer 显示剩余时间和危险边界。
- 玩家受伤继续走 `damagePlayer()` 和受击无敌时间。
- Boss 死亡、下一图、reset 时清空 Hazard。
- 不做复杂粒子系统。

任务 A3：Storm Mobility Pattern v1（完成：`9f19718`）

- 给 Storm Herald 增加一次数据化位置变化或短距离突进。
- 必须有目标线/落点提示和不可连续命中保护。
- 不能复用玩家 Input，不修改 Player Dash。

实现结果：Tempest Rush 使用独立 `BossDashState`，在前摇开始时快照目标；移动和落点经过地图解析，Chill 只影响移动阶段；Renderer 显示锁定路径、落点和冲击反馈；Boss 死亡、下一图和 reset 均清理状态。

Milestone A 验收：已完成。三个 Boss 至少各有一个其他 Boss 没有的可观察机制。

### Milestone B：构筑深度和资源约束

任务 B1：Mana Resource v1（完成：`1f58836`）

- Player 增加 mana/maxMana/regen。
- SkillDefinition 增加 manaCost。
- Primary 低消耗，Secondary/Utility 高消耗，Dash 可零消耗。
- 资源不足时不消耗 cooldown，也不施法。
- HUD 和 Skill Panel 显示当前/最大 Mana、消耗和实际 cooldown。
- 暂不增加 mana flask。

实现结果：`Player` 持有并恢复 Mana；`SkillDefinition` 和 `SkillLibrary` 为八个技能提供显式 cost；`SkillBar` 拆出 `canCast()`/`consumeCooldown()`，GameWorld 四条施法路径统一先检查输入、几何、cooldown 和 Mana，再扣资源并启动 cooldown；reset 新 run 回满，MapComplete/死亡/面板状态不进入施法路径。HUD、Skill Bar 和 Skill Panel 已显示资源信息。

任务 B2：天赋盘 Keystones v1（完成：`f26fe1b`）

- 保留现有 20 节点布局，先替换或扩展 4 个分支末端 Notable 行为。
- Keystone 必须改变玩法，而不是单纯再加 10% 数值。
- 例：Projectile 增加投射物但降低单发伤害；Area 缩小范围换高伤害；Survival 提高药瓶收益；Loot 提高掉落同时提高怪物伤害。
- 行为通过数据/tag 和 CombatMath 接入。
- 不一次扩成上百节点。

实现结果：四个末端 Notable 已数据化为 Volley Doctrine、Concentrated Impact、Second Wind、Loaded Dice。`Stats` 聚合新增投射物数量、药瓶效果、物品数量和承伤倍率；GameWorld 的施法、药瓶、掉落和受击路径均已接入，Renderer 复用 CombatMath 展示实际技能结果。宝箱提示读取实际生成数量，避免 Loaded Dice 激活后出现错误文案。新增 Keystone 纯逻辑回归后测试基线为 366 条。

任务 B3：异常抗性与穿透 v1（完成：`c1e7e56`）

- EnemyDefinition / BossDefinition 增加 Ignite 与 Chill resistance/effectiveness。
- 普通怪、Elite、Boss 使用不同基线。
- UI 至少在 Boss 血条区展示抗性摘要。
- Ignite 伤害和 Chill 强度都通过统一的有效抗性计算；有效抗性为 `max(0, resistance - penetration)`，并限制在 0 至 100。
- Deep Chill 提供 Chill penetration，异常快照在施放时保存穿透值；Combustion 保持原有 Ignite 伤害/持续时间职责。
- Combustion/Deep Chill 的收益必须仍可观察。
- 不做完整元素伤害类型和五种抗性系统。

实现结果：Normal/Ranged/Elite/Charger 通过 `EnemyDefinition` 使用不同 Ignite/Chill 基线，三个 Boss 通过 `BossDefinition` 使用主题化抗性。`GameWorld::applySkillAilment()` 是唯一实际接入点，Ignite tick 和 Chill 移速均经过 `CombatMath` 抗性/穿透 helper；Boss HUD 显示当前 Boss 抗性，异常摘要显示 Support 穿透。新增抗性、穿透、Enemy 生命周期和 Support 快照测试，纯逻辑基线提升至 393 条。

Milestone B 验收：至少存在 Projectile direct-hit、Area Ignite、Cold control 三种手感和配装明显不同的构筑。

### Milestone C：装备价值与掉落循环

任务 C1：Item Base Types v1（完成：`bbd6dd7`）

- 每个槽位增加 3 个 base type。
- Base 提供固定 implicit 或基础属性，affix 仍提供随机属性。
- Item 保存 base id/name，Renderer 展示 Base、Implicit、Affix。
- Boss relic 继续是特殊 base。
- 不新增更多装备槽。

实现结果：四个槽位各有三个普通 Base，另有三个 Boss theme relic Base。Item 保存 `baseId`、`baseName` 和 `implicitStats`；LootGenerator 先选 Base，再以 `ItemAffix::stats` 记录每条词缀贡献，最终 `stats` 由 implicit 与词缀贡献聚合得到。普通掉落名称包含 Base，详情面板显示 Base、Implicit、Affixes；装备、替换、分解、强化和地面掉落继续移动完整 Item。Boss relic 的旧等级缩放数值保持不变。

任务 C2：词缀标签与权重 v1（完成：`abcaffc`）

- `AffixDefinition` 增加 tags 和正数 weight，所有现有词缀按 `AffixStat` 自动获得明确标签。
- 同一件物品优先避免重复 `AffixStat`，候选耗尽时有稳定 fallback，不死循环、不越界。
- `LootBias` 支持地图主题和 Boss theme 提高目标标签权重；普通地图仍可使用无偏置生成。
- Map drop、事件 drop、Boss 非 relic 额外掉落均传递偏置；Boss relic 继续使用固定 Base/主题词缀。
- ItemAffix 保留 tags 和实际 contribution，详情面板显示标签摘要。

任务 C3：锻造选择 v2（完成：`c3e456f`）

- `V` 打开锻造上下文，数字键选择 Improve/Reroll/Raise Tier，F1-F3 选择目标词缀。
- `LootGenerator` 持有稳定 affix id、Tier contribution、候选过滤、Stats rebuild 和纯锻造结果。
- Forge Fragments 只在成功操作后扣除；失败、无效目标、无候选、最高 Tier 和碎片不足均保留原 Item。
- 锻造面板在 Playing/MapComplete 均可用，独占输入上下文；旧线性 `upgradeLevel` 状态已移除。

任务 C4：Stash v1（完成：`93065e4`）

- 新增 24 格 `Stash`，保存完整 Item；满仓 add 失败不消耗调用方物品。
- MapComplete 使用 `Tab` 在 Inventory/Stash 间循环选择，`I/O` 双向移动；Playing 不开放远程仓库。
- `reset()` 清空 Stash，`startNextMap()` 保留 Stash、装备和本 run 成长。
- Stash 与锻造互斥；数字键仍只处理奖励/下一图，未引入拖拽、排序或分页。

Milestone C 验收：玩家会因为 base、implicit、affix、tier 和构筑方向做真实取舍，而不是只看总伤害。

### Milestone D：地图深度

任务 D1：地图布局变体 v2（完成：`1508dcc`）

- 每个 MapTemplate 至少三套障碍/事件布局 seed。
- 保证 Start 到 BossGate 可达。
- 先使用预制布局组合，不手写复杂寻路生成器。
- 加可达性纯逻辑测试。

实现结果：

- 新增 `MapLayout.hpp` / `MapLayoutLibrary`，三个 MapTemplate 各有三个稳定布局 id，共 9 个布局；布局数据只保存障碍和三个事件位置。
- `MapInstance` 增加 `templateIndex`、`layoutIndex`、`layoutId` 和 `layoutDefinition` getter；地图几何由布局库单一来源提供，删除模板中重复的旧障碍/事件坐标。
- `MapInstance::variantForMapLevel()` 以 `max(1, mapLevel) - 1` 对三种布局取模；第一张图和 `startNextMap()` 都按当前 MapOption template 绑定，未使用时间随机数。
- 增加 `geometryIsValid()` 和 `hasReachableBossPath()`；后者使用固定 40px 栅格 BFS 进行纯逻辑可达性检查，不是运行时寻路。
- `resolveMovement()` 仍是玩家、Enemy 和 Boss 位移共用的唯一实际碰撞解析入口；Renderer 小地图继续读取当前 MapInstance 的障碍/事件，HUD 增加布局编号。
- 预制数据经过出生点、Boss Arena、事件交互半径、边界和可达性修正；未来新增布局必须先通过同一组校验。

验收结果：代码提交为 `1508dcc`；clean build、CTest、直接逻辑测试和 3 秒启动 smoke test 均通过，纯逻辑测试为 `902 passed / 0 failed`。

已知约束：BFS 是 40px 采样近似，只用于开发期验证，不保证任意未来几何都能表达连续碰撞可达性；复杂随机地牢和运行时寻路仍明确不在范围内。

任务 D2：Minimap Reveal v1（完成：`bd4fc4d`）

- 小地图只显示玩家探索过的区域；未探索区域用暗色网格遮蔽。
- Boss 未发现时保留有限目标标记，不展示完整 Boss Gate/Arena 细节或事件位置。
- MapComplete 临时显示全图，不修改底层探索状态。

实现结果：`MapExploration` 使用 60px 网格、300px 揭示半径，GameWorld 在移动/Dash 后更新状态，Renderer 只读绘制；reset/startNextMap 获得新地图时自动重置。纯逻辑基线提升至 913 条。

任务 D3：Map Modifier 扩展 v2

- 增加可组合 modifier，而不是一个模板只带一个固定词缀；每个候选地图稳定组合两个定义。
- 已覆盖怪物速度/攻击压力、掉落数量、Elite/Charger 遭遇权重、事件奖励、Boss 风险、掉落等级和异常抗性。
- 每个定义有稳定 id、名称、风险描述、收益描述和明确数值字段；组合通过乘法倍率/加法数值统一聚合，不用显示名称驱动玩法。
- HUD 显示当前组合的短摘要；MapComplete 对候选名称、风险、收益和数值摘要做长度限制，避免遮挡其它面板。

实现结果：代码提交为 `0594ebe`。`MapModifierLibrary` 提供 6 个定义，`MapOptionLibrary` 固定生成三组两词缀组合；Enemy 速度、普通/精英/Charger 权重、宝箱事件掉落、异常抗性、Boss/普通怪生命与伤害、掉落数量/等级和 LootBias 均接入真实 GameWorld 路径。保留三参数 `Enemy::update()` 兼容入口，避免破坏已有纯逻辑调用方。

验收结果：clean build、CTest、直接逻辑测试和 3 秒启动 smoke test 均通过；纯逻辑基线为 `920 passed / 0 failed`，工作区在提交后干净。

已知约束：LootBias 当前最多承载两个标签，组合时按稳定顺序保留前两个有效标签；未来扩展为更多标签前必须先扩展 `LootBias` 和测试，不能静默增加字符串规则。地图选项仍是固定模板池，不是随机地图生成器。

Milestone D 验收：连续三张地图在路线、遭遇、风险和奖励上有明显变化。

### Milestone E：可持续运行与产品化

任务 E1：Deterministic RNG Service（完成：`e058357`）

- 已移除 gameplay 路径中的 `std::rand()` / `std::srand()`。
- `GameWorld` 持有 run seed 和 run-owned `RandomService`；`reset()` 派生新 seed，`startNextMap()` 延续同一随机流。
- LootGenerator、MapRewardLibrary、EnemySpawner 和 GameWorld 的掉落/遭遇/事件随机路径均支持显式 RNG 注入。
- 同 seed 的纯逻辑和轻量 GameWorld 生成结果可复现；Renderer 不持有 RNG。
- 旧纯逻辑调用方保留固定 legacy seed 重载，但新玩法代码不得依赖该兼容路径。

任务 E2：Local Save v1（完成：`7ea69b2`）

- 保存 Player 成长、天赋节点、装备、Inventory、Stash、地面掉落、技能/Support 解锁、当前地图和永久 run 奖励。
- `RandomService` 保存并恢复引擎状态，读档后不会改变下一次随机结果。
- 使用带 magic、schema version、payload length 和 CRC32 的二进制格式；写入采用临时文件加 Windows 原子替换。
- F5 保存、F9 加载；天赋盘、技能面板和锻造面板打开时不抢夺 F1-F10 上下文。
- 加载失败不改变当前内存 run；战斗中的 Enemy、Projectile、Boss 火区和进行中的 ElitePack 不保存，读档回到地图出生点或 MapComplete。

任务 E3：Pause / Settings / Input Help（完成：`57d8d85`）

- `GameState::Paused` 保存暂停前的 Playing/MapComplete 上下文；Esc 按“关闭 Crafting/Passive/Skill 子面板 -> Pause -> Continue”优先级处理，窗口 `Closed` 仍由 `Game` 关闭。
- Pause 早期返回，冻结 Mana、技能冷却、敌人、投射物、地面危险、刷怪、Boss/事件计时、地图统计和 `survivalTime`，没有把 `dt = 0` 分散到各个对象。
- Pause 菜单提供 Continue、F5 Save Run、F9 Load Run、R Restart Run、Q Quit to Desktop；Quit 只设置 `GameWorld` 退出请求，由 `Game` 处理窗口关闭。
- 新增 `InputBinding.hpp` 只读 binding 表，Pause 的 Input Help 从该表绘制；数字键、F、鼠标和玩法输入在 Pause 中不会进入战斗/装备/奖励分支。
- 暂停状态保存时按暂停前上下文写入 SaveData；从 MapComplete 暂停后保存/恢复仍保留结算阶段。

验收结果：

- `arpg_world_tests`：`34 passed / 0 failed`，覆盖子面板关闭优先级、Pause 冻结、Continue、Pause Save/Load、Restart、Quit 请求和 MapComplete 恢复。
- clean build、CTest `3/3` 和 3 秒启动 smoke test 通过；代码提交为 `57d8d85`，工作区在代码提交后干净。

任务 E4：Visual/Audio Pass

- 用可合法分发的 bitmap/sprite、音效替换主要占位几何。
- 保持碰撞体与视觉尺寸一致。
- 不在玩法逻辑稳定前投入复杂动画系统。

Milestone E 验收：玩家可以关闭程序后继续 run，能稳定完成至少 5 张连续地图，重开和异常退出不会破坏存档。

## 13. 推荐的下一项任务

建议交给 hy3：`可玩性验收与难度曲线 v1`。

原因：核心系统已经覆盖战斗、地图事件、Boss、掉落、天赋、技能、锻造、Stash、存档和暂停。下一步应证明这些系统能连续工作，而不是继续增加孤立内容。先用数据和测试把“出生点 -> 探索 -> Boss -> 奖励 -> 下一图”稳定跑通至少 5 张地图，再决定是否投入新技能或美术资源。

### 13.1 已完成任务记录：Mana Resource v1

目标：增加一个可恢复的玩家施法资源。资源不足时施法必须完全失败，不能产生效果、投射物、位移或 cooldown；资源足够时一次施法只扣一次 Mana。

开始前必须阅读：

- `Player.hpp/.cpp`：当前生命、经验、属性和 reset 语义。
- `Skill.hpp`、`SkillLibrary.hpp`：技能定义与 header-only 数据表。
- `SkillBar.hpp`：当前 `tryCast()` 会立即启动 cooldown，这是本任务最容易产生顺序 bug 的位置。
- `GameWorld::tryCastPrimarySkill()`、`tryCastSecondarySkill()`、`tryCastUtilitySkill()`、`tryCastMovementSkill()`：四条真实施法路径。
- `Renderer::drawSkillBar()`、`Renderer::drawSkillPanel()` 和 HUD stats 区域。
- `tests/arpg_logic_tests.cpp`：现有 Player、SkillBar 和 CombatMath 测试风格。

必须实现：

1. `Player` 增加 `mana_`、`maxMana_`、`manaRegenPerSecond_`，以及只读 getter、`canSpendMana(float)` 和 `spendMana(float)`；资源必须 clamp 在 `[0, maxMana]`。
2. `Player::update(dt)` 只在 `dt > 0` 时恢复 Mana，不能超过上限；新 run 满 Mana，`startNextMap()` 保留当前 Mana，不自动回满。
3. `SkillDefinition` 增加 `float manaCost = 0.0f`，所有八个技能显式填写：Primary 低消耗，Secondary/Utility 中高消耗，Dash 为 0。
4. 重构施法判定，确保顺序为“输入及几何前置条件 -> cooldown 是否 ready -> Mana 是否足够 -> 扣 Mana并启动 cooldown -> 生成效果”。不能先调用现有会重置 cooldown 的 `SkillBar::tryCast()` 再检查 Mana。
5. 推荐将 `SkillBar` 拆为 `canCast(slot)` 和 `consumeCooldown(slot)`，或提供等价的原子接口；禁止把 Player/Mana 所有权塞进 `SkillBar`。
6. Primary 按住左键时，Mana 不足只是不施法；后续 Mana 恢复且 cooldown ready 后可自然再次发射，不需要重新按键。
7. HUD 常驻显示 `Mana current/max`；Skill Bar 或 Skill Panel 显示每个技能的 `Mana N`，Locked 技能也可预览消耗。
8. `reset()` 恢复默认满 Mana；死亡、MapComplete 和面板上下文不得继续施法或扣 Mana。

测试必须覆盖：

- Player 初始 Mana、扣除、余额不足拒绝、恢复和上限 clamp。
- `dt <= 0` 不恢复；`reset()` 后恢复默认值。
- 每个技能 Mana cost 已显式设置，Dash 为零，Primary 低于 Secondary/Utility。
- cooldown 未 ready 时不扣 Mana；Mana 不足时不启动 cooldown；成功施法同时扣一次 Mana并启动 cooldown。
- Primary 零方向、Secondary/Utility 无输入、Movement 无 dash 输入等既有早退路径不扣 Mana。
- 构建后运行现有全部测试，最终执行一次 `--clean-first`，因为 `SkillDefinition` 和 `SkillLibrary` 是 header-only 数据。

明确不做：

- 不做 Mana flask、资源保留、Energy Shield、按命中回蓝、击杀回蓝。
- 不新增 Mana 装备词缀或天赋节点；先只交付基础资源闭环。
- 不改技能解锁、Support 兼容性、地图奖励、Boss、敌人 AI。
- 不顺手重构整个 `GameWorld` 或 Renderer。
- 不提交代码；保持工作区未提交，交给主 review Agent 验收、修正和 commit。

交付报告必须列出：改动文件、接口变化、各技能 Mana cost、施法判定顺序、测试数量、clean build 结果、已知风险和 `git status --short`。

### 13.2 已完成任务记录：Passive Tree Keystones v1

目标：把现有 20 节点中的四个末端 Notable（Projectile 4、Area 9、Survival 14、Loot 19）升级为有明显取舍的 Keystone。普通节点、位置、前置关系、SP 输入和天赋盘 UI 的基本交互保持不变。

开始前必须阅读：

- `PassiveTree.hpp`：20 节点数据、`Stats` 聚合、前置关系和分支统计。
- `Stats.hpp`、`Player.hpp/.cpp`：最终属性聚合和生命/药瓶逻辑。
- `CombatMath.hpp`、`SkillBar.hpp`：投射物数量、范围伤害、范围半径和 Support 计算。
- `GameWorld.cpp` 中 `damageForPlayerSkill()`、`radiusForPlayerSkill()`、`projectileCountForPlayerSkill()`、`tryUseLifeFlask()`、`rewardEnemyKill()`：真实效果接入点。
- `Renderer.cpp` 的 `drawPassiveTree()`、HUD build summary 和 Skill Panel 预览。
- `tests/arpg_logic_tests.cpp`：现有纯逻辑测试风格和 PassiveTree 测试。

实际实现：

1. 为 `PassiveNode` 增加明确的 Keystone 类型或等价数据字段，默认值为 `None`；不要依赖节点名称字符串判断行为。四个末端 Notable 各绑定一个唯一 Keystone。
2. Projectile Keystone：`Volley Doctrine`，投射物技能额外发射 2 枚投射物，但投射物伤害倍率降低 25%。额外投射物必须和 Spread/Volley Support 正确叠加，非 Projectile 技能不受影响。
3. Area Keystone：`Concentrated Impact`，范围技能伤害提高 35%，实际范围半径降低 25%。Projectile 和 Dash 不受影响；Renderer 的范围预览必须显示实际半径。
4. Survival Keystone：`Second Wind`，生命药瓶治疗量提高 50%，不提高最大生命、不增加充能数量。治疗结果仍 clamp 到最大生命，药瓶空、满血和死亡的失败路径不扣充能。
5. Loot Keystone：`Loaded Dice`，物品掉落数量/概率提高 25%，但普通/精英/Boss 的实际伤害提高 20%。风险必须接入真实地图掉落与敌人伤害路径，不能只改 UI 文案。
6. Keystone 效果应通过数据/属性或无状态计算 helper 接入真实路径。Renderer 不得复制另一套数值公式；Skill Panel、装备预览和实际施法要使用相同的 CombatMath/Stats 计算。
7. `PassiveTree::combinedStats()` 或等价聚合必须保持普通节点的原有结果；分配/重复分配/前置检查语义不变。Keystone 只能在节点成功分配后生效。
8. HUD build summary 明确显示 Keystone 名称或短标签；天赋盘 hover 描述显示收益与代价。不能只显示 `Notable`。

推荐接口方向：

- 新增 `enum class PassiveKeystone { None, VolleyDoctrine, ConcentratedImpact, SecondWind, LoadedDice }`。
- 用数据字段保存 Keystone；可以在 `PassiveTree` 提供 `hasKeystone()` / `keystoneSummary()`，也可以把可聚合的数值放入 `Stats`。
- 投射物数量、范围伤害/半径、药瓶治疗和掉落/敌人伤害必须有单一计算来源；不要在 `GameWorld` 和 `Renderer` 各写一套 Keystone switch。
- 如果给 `Stats` 增加字段，必须同步 `combineStats()`、装备预览的 `statsDelta()` 和所有 Stats 聚合初始化，避免聚合字段错位。

测试必须覆盖：

- 四个末端节点的 Keystone 类型、名称、描述和前置关系。
- Keystone 未分配时不改变默认 Projectile/Area/Flask/Loot 行为；分配后才生效，重复分配不重复叠加。
- Projectile：投射物数量 +2、投射物伤害下降 25%，Area/Movement 不变；和 Volley/Pierce Support 组合不崩溃。
- Area：范围伤害提高 35%、半径降低 25%；Projectile/Dash 不变，实际伤害和半径与 Renderer 预览使用同一计算结果。
- Survival：药瓶治疗量提高 50%，满血/死亡/空瓶失败路径不消耗充能，治疗不超过 max HP。
- Loot：掉落概率/数量提高 25%，普通/Elite/Boss 伤害提高 20%；至少覆盖真实 `enemyDamageForMap()` 和 `rewardEnemyKill()` 的计算 helper。
- 现有全部回归测试仍通过；header-only 天赋数据变更后必须 clean build。

明确不做：

- 不新增节点、不改变 20 节点布局、不做天赋重置和职业起点。
- 不新增第五条分支、不做复杂 Keystone 互斥树。
- 不新增装备词缀、Mana 系统扩展、技能解锁、地图模板或 Boss 技能。
- 不在 Renderer 中修改状态，不复制 CombatMath 公式。
- 不提交代码；保持工作区未提交，交给主 review Agent 验收、修正和 commit。

交付结果：代码提交为 `f26fe1b`，四个 Keystone 均通过纯逻辑测试；clean build、CTest、直接测试和 3 秒启动 smoke test 均通过，测试为 `366 passed / 0 failed`。本记录保留原始约束，便于回看为什么这些属性必须走真实路径。

### 13.3 已完成任务记录：Ailment Resistances and Penetration v1

代码已通过主 review 并提交为 `c1e7e56 Add ailment resistances and penetration`。

实现结果：

- `EnemyDefinition` 增加 Ignite/Chill resistance；Normal 为 `0/0`，Ranged 为 `10/10`，Elite 为 `15/15`，Charger 为 `15/10`。
- `BossDefinition` 为 Brimstone、Storm、Brood 分别提供 `35/20`、`20/35`、`30/30` 的抗性配置。
- `SupportDefinition` 增加穿透字段；Deep Chill 提供 `20% Chill penetration`，Combustion 保持原有 Ignite 伤害/持续时间职责。
- `CombatMath` 统一提供有效抗性、Ignite tick 和 Chill slow 的计算；有效抗性为 `max(0, resistance - penetration)`，抗性限制在 `0..100`。
- `GameWorld::applySkillAilment()` 将计算结果接入实际命中路径，Ignite 抗性只影响 DOT，Chill 抗性只影响 slow；原始命中、击杀、经验、掉落路径不变。
- 异常数据继续在投射物生成时快照；Renderer 显示 Boss 抗性和当前异常的 penetration 摘要。

验收结果：clean build、CTest、逻辑测试和 3 秒启动 smoke test 均通过；测试为 `393 passed / 0 failed`。未发现 P0/P1 问题。

明确不做：

- 不新增元素伤害类型、抗性装备词缀、第五种异常、免疫系统或完整元素抗性面板。
- 不新增技能、Support、技能栏、地图奖励、Boss 技能或存档。

### 13.4 已完成任务记录：Item Base Types v1

代码已通过主 review 并提交为 `bbd6dd7 Add item base types and implicit stats`。

实现结果：

- 新增 `ItemBaseLibrary`，四个装备槽各有三个普通 Base，另有 Brimstone、Storm、Brood 三个 Boss relic Base。
- `Item` 保存 `baseId`、`baseName`、`implicitStats`；`ItemAffix` 保存该词缀的实际 Stats contribution。
- `LootGenerator` 先选 Base，再按 rarity 生成词缀；普通和 Boss Item 的最终 Stats 都可以由 `implicitStats + affix.stats` 重建。
- 生成物品名称包含 Base，详情面板显示 Base、Implicit、Affixes；负向 Base 属性也会显示，便于看到取舍。
- 装备替换、背包、分解、强化、地面掉落和 MapComplete 所有权流程继续移动完整 Item；Boss relic 的旧等级缩放数值保持。

验收结果：clean build、CTest、逻辑测试和 3 秒启动 smoke test 均通过；测试为 `535 passed / 0 failed`。中途发现并排除了 NMake 增量构建造成的旧 Item ABI 混链问题，最终以 clean build 为准。

明确不做：

- 不新增装备槽、背包分页、stash、装备等级需求、宝石或技能物品。
- 不实现重铸、选择式 crafting；这些留给 C3。词缀 tags/weights 已在 C2 完成。

### 13.5 已完成任务记录：Affix Tags and Weights v1

代码已通过主 review 并提交为 `abcaffc Add affix tags and weighted loot`。

实现结果：

- 新增 `LootBias.hpp`，定义 `AffixTag` 和地图/Boss 可传递的双标签权重偏置。
- `AffixDefinition` 增加 tags/weight；当前词缀按 `AffixStat` 获得 `Damage`、`Projectile`、`Area`、`Survival` 等逻辑标签，权重集中在 `LootGenerator` 计算。
- 同一物品优先避免重复 `AffixStat`；候选不足时回退到同槽位未用词缀，再回退到同槽位池，确保不会死循环或越界。
- 普通地图掉落、地图事件掉落和 Boss 非 relic 额外掉落使用地图/Boss theme bias；Brimstone、Storm、Brood relic 仍保留固定 Base 和固定主题词缀。
- `ItemAffix` 保存 tags 和实际 Stats contribution；Renderer 详情面板显示标签摘要，未把 UI 文本用于玩法判断。

验收结果：clean build、CTest、逻辑测试和 3 秒启动 smoke test 均通过；测试为 `707 passed / 0 failed`，工作区在提交后干净。

明确不做：

- 不替换全项目 `std::rand()`，不引入第三方 RNG；统一 RNG 留给 E1。
- 不实现重铸、选择式 crafting、stash、loot filter、装备等级需求或新装备槽。

### 13.6 已完成任务记录：Crafting Choice v2

代码已通过主 review 并提交为 `c3e456f Add choice-based affix crafting`。

实现结果：

- 新增 `Affix.hpp` 和 `Crafting.hpp`；`ItemAffix` 保存稳定 id、AffixStat、prefix/suffix 身份和 contribution。
- `LootGenerator` 提供 `ImproveAffix`、`RerollAffix`、`RaiseAffixTier` 纯操作，统一负责候选过滤、Tier 数值和 Item stats rebuild。
- `V` 在 Playing/MapComplete 打开锻造面板；数字键选操作，F1-F3 选词缀，Escape 关闭；面板打开时不会装备、拾取、选奖励或切换 P/K 面板。
- Forge Fragments 仅在成功操作后扣除；Boss relic 固定词缀不可重铸，Base/implicit/其他词缀在单条加工中保持不变。
- 移除旧的线性 `upgradeLevel`/`MaxItemUpgradeLevel` 状态，避免两套强化规则并存。

验收结果：clean build、CTest、逻辑测试和 3 秒启动 smoke test 均通过；测试为 `765 passed / 0 failed`，工作区在提交后干净。

明确不做：

- 不做完整货币、商店、交易、装备锁定、工艺配方、风险失败或物品毁坏。
- 当时不做 stash、存档、统一 RNG 或复杂鼠标工艺台；Stash 后续已由 C4 完成，下一项转为 D1 地图布局变体。

### 13.7 已完成任务记录：Stash v1

代码已通过主 review 并提交为 `93065e4 Add map-complete stash management`。

实现结果：

- 新增 24 格 `Stash`，提供 `add`、`insert`、`take`、`items`、`size`、`capacity`、`isFull` 和 `clear`；满容量失败在移动调用方 Item 前返回。
- `GameWorld` 在 MapComplete 持有并展示 Stash；`reset()` 清空，`startNextMap()` 保留；跨容器移动复制完整 Item 字段且失败时恢复源容器。
- `Tab` 在 Inventory/Stash 所有物品之间循环，`I` 存入 Stash，`O` 取回 Inventory；`Delete/C/V` 在选中 Stash 时被屏蔽，数字键仍只用于结算奖励/地图。
- Stash 与锻造上下文互斥；Renderer 只读显示 `Inventory X/9`、`Stash Y/24`、选中项、容量提示和移动操作。

验收结果：clean build、CTest、逻辑测试和 3 秒启动 smoke test 均通过；测试为 `804 passed / 0 failed`，工作区在提交后干净。

明确不做：

- 不做跨运行存档、共享仓库、多页仓库、物品堆叠、标签过滤、拖拽或仓库排序。
- 不新增装备槽、技能、Support、地图、Boss 或经济系统。

### 13.8 已完成任务记录：Map Layout Variants v2

代码已通过主 review 并提交为 `1508dcc Add deterministic map layout variants`。

实现结果：

- 新增 `include/MapLayout.hpp` / `MapLayoutLibrary`，三个 MapTemplate 各提供三套稳定布局，共 9 个全局唯一 layout id。
- `MapInstance` 增加 `templateIndex`、`layoutIndex`、`layoutId`、`layoutDefinition`；障碍和事件坐标只从布局库读取，模板中的重复旧坐标已移除。
- 默认布局选择为 `max(1, mapLevel) - 1` 对三种布局取模；`startNextMap()` 显式绑定当前 MapOption 的 template 和该 map level 的 layout。没有时间随机数，也没有 Renderer 侧随机。
- 增加 `geometryIsValid()` 和 `hasReachableBossPath()`。可达性使用固定 40px 栅格 BFS 进行开发期验证，不是运行时寻路。
- `resolveMovement()` 仍是实际移动碰撞的唯一入口；小地图继续读取 MapInstance 的障碍/事件，HUD 显示 `LAYOUT N/3`。
- 9 个布局均通过边界、出生点、Boss Arena、事件交互半径和 Start-to-Boss 检查；测试覆盖地图边界/障碍移动解析。

验收结果：clean build、CTest、直接逻辑测试、3 秒启动 smoke test 均通过；纯逻辑基线为 `902 passed / 0 failed`，工作区干净。

已知约束：BFS 是 40px 栅格采样，只用于验证预制数据，不保证未来任意连续几何的严格拓扑可达性；复杂随机地牢和运行时寻路仍不在范围内。

### 13.9 已完成任务记录：Minimap Reveal v1

代码已通过主 review 并提交为 `bd4fc4d Add minimap exploration reveal`。

实现结果：

- 新增无 SFML 的 `MapExploration`，使用 60px 固定网格和 300px 揭示半径保存探索状态；揭示集合只增不减，越界请求不会错误揭示地图边缘。
- `MapInstance` 在新地图构造时揭示出生点，并提供只读探索状态；`GameWorld::movePlayerBy()` 在边界/障碍解析后更新玩家周围揭示区域，因此普通移动和 Dash 都能推进探索。
- Playing 小地图用暗色网格遮蔽未探索区域，只绘制已揭示障碍和事件；Boss 未发现时保留一个有限目标标记，不显示 Boss Gate/Arena 细节。MapComplete 只在 Renderer 临时显示全图，不修改探索状态。
- reset/startNextMap 通过重新构造 MapInstance 获得新探索状态；地图几何、事件、Boss、掉落、Stash、锻造和输入语义未改变。
- HUD 小地图底部显示布局编号和探索百分比，未增加输入键。

验收结果：clean build、CTest、直接逻辑测试、3 秒启动 smoke test 均通过；纯逻辑基线为 `913 passed / 0 failed`，工作区干净。

已知约束：未探索区域只影响小地图可读性，大地图世界暂不增加迷雾遮罩；探索网格是显示层近似，不是地图寻路或碰撞网格。

### 13.10 已完成任务记录：Composable Map Modifiers v2

目标：把当前“一张地图一个 modifier”升级为 2 个可组合的风险/收益词缀，让连续刷图在怪物、掉落和事件层面产生更明显的取舍；不改变地图布局、探索、Boss AI 和输入上下文。

代码已通过主 review 并提交为 `0594ebe Add composable map modifiers`。

实现结果：

- `MapModifierEffect` / `MapModifierDefinition` 保存稳定 id、名称、风险描述、收益描述和可组合数值；`MapModifierLibrary` 提供 6 个数据定义。
- `MapOptionLibrary` 仍生成 3 个候选，每个候选稳定组合 2 个不重复 modifier；默认第一张地图使用 neutral modifier，不额外增加难度。
- 倍率字段相乘、整数压力/奖励字段相加；地图等级缩放在组合完成后统一执行。`MapModifier::lootBias()` 保持旧接口并提供主/次两个标签。
- 速度接入 `Enemy::update()`，Elite/Charger 权重接入地图遭遇选择，宝箱数量接入事件奖励倍率，Ignite/Chill 接入地图异常抗性；普通敌人、Boss、掉落数量、Boss 保底掉落、item level、LootBias 和 Future Item Quantity 仍走原有奖励路径。
- HUD 当前地图显示组合摘要；MapComplete 候选文本使用长度限制，保留地图模板、风险/收益和推荐等级可读性。
- 为 Enemy 保留无 modifier 的三参数 `update()` 重载，旧调用方默认使用 `1.0f` 地图速度。

测试与验收：

- 测试覆盖三候选、两组件身份、组合后的速度/事件/抗性、伤害/Boss 掉落、Charger/item level、同等级稳定生成和 neutral 默认地图。
- clean build 通过，CTest 通过，直接运行 `arpg_logic_tests.exe` 为 `920 passed / 0 failed`。
- `PlaneShooter.exe` 启动 3 秒后进程保持运行，工作区在提交后干净。

已知约束：`LootBias` 目前最多承载两个标签，组合时按稳定顺序保留前两个有效标签；未来扩展更多标签前必须先扩展该数据结构和测试。地图选项仍来自固定模板池，不是随机地形生成器。

### 13.11 已完成任务记录：Deterministic RNG Service v1

目标：将地图、掉落、奖励、敌人遭遇和事件奖励从散落的随机调用收敛到可注入、可复现的单局随机服务，为存档、Bug report 和端到端测试提供稳定基础。

实现结果：

- 新增无 SFML 的 `RandomService`，提供 `nextInt`、`nextUInt64`、`nextFloat01`、`chance`、空安全的 `nextIndex`、按权重选择和 seed 派生。
- `GameWorld` 构造函数接受 run seed；默认 seed 便于复现，`reset()` 派生新 seed，显式 `reset(seed)` 可恢复指定 run，`startNextMap()` 不重新播种。
- `LootGenerator`、`MapRewardLibrary`、`EnemySpawner` 和 GameWorld 的掉落、词缀、事件、敌人类型、地图奖励路径均使用显式 RNG 引用。
- 保留旧纯逻辑 API 的固定 legacy seed 重载，避免测试调用方被迫依赖全局状态；新增玩法代码必须使用 run-owned RNG。
- Boss relic 当前仍走原有确定性生成路径，不能在下一轮存档中假设它已有 RNG 状态依赖。

验收结果：

- `arpg_logic_tests`：`920 passed / 0 failed`。
- `arpg_world_tests`：`7 passed / 0 failed`，覆盖真实 GameWorld 敌人生成、同/不同 seed 和 reset 生命周期。
- clean build、CTest `2/2` 和 3 秒启动 smoke test 通过；代码提交为 `e058357`，工作区干净。
- `include/`、`src/`、`tests/` 中不再存在 `std::rand`、`std::srand` 或裸 `rand` 调用。

已知约束：当前 RNG 默认 seed 是固定值，适合复现和测试；以后如需用户可配置 seed，应通过明确的 run 创建入口接入，不能在 Renderer 或每张地图内部按时间播种。E2 已补充引擎状态快照，但 RNG stream 仍不应暴露给 Renderer。

### 13.12 已完成任务记录：Local Save v1

目标：实现单机单文件、版本化、可恢复的当前 run 存档，并保证损坏或不兼容文件不会污染当前内存状态。

实现结果：

- 新增纯数据 `SaveData`、`SaveService` 和 `SaveService.cpp`；存档不依赖 Renderer、SFML 或 GameWorld 指针。
- 保存 Player 的 level/EXP/SP/Mana/HP、升级属性源、20 个天赋分配、四槽装备、完整 Item 字段、Inventory、Stash、地面掉落、技能/Support 槽位和 cooldown elapsed。
- 保存当前地图等级、模板/布局、探索格、事件完成态、MapComplete 奖励阶段、地图选项、run progression、Forge Fragments、life flask charges 和 RNG 引擎状态。
- 不保存 Enemy、Projectile、Boss 投射物、GroundHazard、瞄准状态和进行中的 ElitePack 敌人；读档时清空瞬时对象并把玩家放到地图出生点，MapComplete 存档恢复 Boss Defeated 阶段。
- 二进制格式为 little-endian：`magic`、`schema version`、payload length、CRC32、payload；写入使用 `.tmp` 加 `MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)`。
- F5 保存、F9 加载；加载失败保留当前 run，成功/失败通过现有 HUD event status 显示。

验收结果：

- `arpg_logic_tests`：`920 passed / 0 failed`。
- `arpg_save_tests`：`11 passed / 0 failed`，覆盖 RNG continuation、带词缀 Item/装备/Support/Inventory/Stash/掉落 round-trip、CRC、版本、截断、缺失和原子替换失败。
- `arpg_world_tests`：`20 passed / 0 failed`，覆盖保存后改变再加载、瞬时敌人清理、安全出生点、坏档不污染当前 run 和 MapComplete 恢复。
- clean build、CTest `3/3` 和 3 秒启动 smoke test 通过；代码提交为 `7ea69b2`，工作区在代码提交后干净。

已知约束：当前只提供单个默认存档文件 `plane_shooter.save`，没有自动启动加载、多个存档槽、云存档或战斗中断恢复；Playing 状态保存后按安全策略从地图起点继续。F5/F9 在 Passive Tree、Skill Panel 和 Crafting 上下文中不抢夺面板快捷键。

### 13.13 hy3 实施任务：Pause / Settings / Input Help v1

目标：把当前 Esc 直接退出改为明确的 Pause 上下文，并让 Save/Load 成为可发现、可验证的主流程。暂停必须冻结战斗模拟，不得通过 `dt = 0` 的零散判断把暂停逻辑扩散到各系统。

开始前必须阅读：

- `include/GameWorld.hpp`、`src/GameWorld.cpp`：当前 GameState、F5/F9 存档入口和面板上下文。
- `include/Input.hpp`、`src/Input.cpp`、`src/Game.cpp`：事件边沿、按键释放、窗口关闭和当前 Esc 语义。
- `include/Renderer.hpp`、`src/Renderer.cpp`：现有 HUD、MapComplete、Passive/Skill/Crafting 面板层级。
- `docs/AGENT_HANDOFF.md` 第 4、6、7、11 节：输入不变量、职责边界、完成定义和验收门禁。
- `tests/game_world_logic_tests.cpp`：无窗口状态测试风格。

必须实现：

1. 新增明确的 Pause 状态或等价输入上下文；Esc 在 Playing、Passive Tree、Skill Panel、Crafting、MapComplete 中先关闭当前子面板，下一次 Esc 才打开 Pause，不能直接关闭窗口。窗口 `Closed` 事件仍然允许退出。
2. Pause 时停止玩家 Mana 恢复、技能 cooldown、敌人移动/攻击、刷怪、投射物、地面危险、事件计时、Boss 技能计时、地图统计和 `survivalTime`；Renderer 继续绘制冻结世界并覆盖 Pause 菜单。
3. Pause 菜单只提供 `Continue`、`Save Run`、`Load Run`、`Restart Run`、`Quit` 五个明确动作。优先复用 F5/F9 和现有 reset/load API，不复制保存逻辑。
4. 增加 Input binding 的单一只读数据表或等价 helper，Renderer 的 Input Help 从该表绘制；禁止继续在 Renderer 里手写一份与 `Input.cpp` 可能漂移的按键说明。
5. Pause/Load 失败不能改变当前世界；Restart 必须走现有 `reset()`，Quit 只设置退出请求，由 `Game` 处理窗口关闭，不让 Renderer 直接关闭窗口。
6. Pause/Settings 面板与 Passive Tree、Skill Panel、Crafting、MapComplete 互斥；数字键、F 键、鼠标左键在 Pause 中不得触发战斗、装备、拾取、奖励或锻造。
7. 测试必须覆盖：Pause 冻结前后玩家 Mana/cooldown/敌人位置不变；Esc 子面板关闭优先级；Continue 恢复；Pause Save/Load 调用现有入口；Restart 清理当前 run；Pause 数字/F/鼠标输入无玩法副作用。
8. UI 只使用现有 SFML 文字和几何，先保证 800x600 可读；面板按钮区域不能遮住状态提示，键位文案必须来自统一 binding 数据。

明确不做：

- 不新增音频、窗口分辨率、画质、键位重绑定或多个存档槽；Settings 只保留可扩展结构。
- 不重写 GameWorld 主循环，不在每个 Enemy/Skill 内新增 Pause flag。
- 不新增技能、Support、Boss、地图 modifier、商店或 loot filter。
- 不提交代码；保持工作区未提交，交给主 review Agent 做完整 diff review、clean build、CTest、直接测试、启动 smoke test 后提交。

交付报告必须列出：改动文件、Pause 状态/输入优先级、冻结的计时器和模拟对象、菜单动作、binding 数据来源、测试数量、clean build、CTest、启动 smoke test、已知风险和 `git status --short`。

### 13.14 已完成任务记录：Pause / Input Help v1

代码已通过主 review 并提交为 `57d8d85 Add pause state and input help`。

实现结果：

- `GameState::Paused` 记录暂停前的 `Playing` 或 `MapComplete` 状态；Esc 的优先级固定为关闭 Crafting、Passive Tree、Skill Panel，随后才进入 Pause，再次 Esc 恢复原状态。
- Pause 在 `GameWorld::update()` 的状态边界提前返回，因此不会推进 Player Mana、SkillBar cooldown、Enemy/Projectile/GroundHazard、刷怪、Boss 技能、事件计时、地图统计或 `survivalTime`。
- Pause 菜单通过现有 API 提供 Continue、F5 Save Run、F9 Load Run、R Restart Run、Q Quit to Desktop；Quit 只产生 `GameWorld` 请求，由 `Game` 关闭窗口。
- 新增 `InputBinding.hpp` 供 Pause Input Help 读取；数字键、F、鼠标和其它玩法边沿在 Pause 中均不会进入战斗、装备、拾取、奖励或锻造分支。
- 暂停时保存会按暂停前上下文写入 `SaveData`；MapComplete 暂停、保存、加载和恢复均保留结算阶段。

验收结果：

- `arpg_logic_tests`：`920 passed / 0 failed`。
- `arpg_save_tests`：`11 passed / 0 failed`。
- `arpg_world_tests`：`34 passed / 0 failed`，新增子面板关闭优先级、冻结、Continue、Pause Save/Load、Restart、Quit 和 MapComplete 恢复测试。
- clean build、CTest `3/3` 和 3 秒启动 smoke test 通过；代码提交后工作区干净。

已知约束：Input Help 表集中管理展示文案，但 SFML 事件到 `Input` 状态的 switch 仍在 `src/Input.cpp`；新增按键时必须同时更新两处并补输入上下文测试。Pause 仍是单文件存档、无多槽位、无音频和无设置页面。

### 13.15 已完成任务记录：可玩性验收与难度曲线 v1

目标：不增加新系统，证明当前“探索地图 -> 事件 -> Boss -> 结算奖励 -> 选择下一图 -> 继续刷图”能够连续完成至少 5 张地图，并把明显的数值断点、状态丢失和进度回退修掉。

开始前必须阅读：

- 本文档第 2、4、5、7、11、13.12、13.14 节。
- `GameWorld::startNextMap()`、`isMapCleared()`、`triggerBossIfNeeded()`、`rewardEnemyKill()` 和 `restoreFromSaveData()`。
- `MapInstance`、`MapOptionLibrary`、`MapModifierLibrary`、`BossLibrary`、`EnemyDefinition`、`Config`。
- `tests/game_world_logic_tests.cpp`、`tests/arpg_logic_tests.cpp`、`tests/save_logic_tests.cpp`。

实施约束：

1. 先做静态审计和可复现运行记录：使用固定 seed 检查地图 1 到 5 的模板、布局、Boss、modifier、敌人强度、掉落等级和奖励状态；不要用当前时间播种，不要在 Renderer 生成随机数。
2. 只修复影响“连续刷图可完成性”的问题：地图切换必须保留 Player、装备、Inventory、Stash、天赋、技能/Support 解锁、Forge Fragments 和 Future Item Quantity；新地图必须清理旧地图敌人、投射物、Boss 技能残留和地面掉落。
3. 检查并必要时调整已有数据表的数值曲线：普通怪 HP/伤害、Boss HP/伤害、刷怪间隔、事件奖励、掉落数量和 map level。调整必须小而有依据，优先改 `Config`/Definition 数据，不把平衡常数散落到 `GameWorld.cpp`。
4. 保持 Boss 门口、Boss Arena、MapComplete 两阶段选择和 `E` 进入下一图的规则不变；不要把“清普通怪”重新变成地图完成条件。
5. 增加可自动运行的进度测试，不新增面向生产的 debug API。推荐使用现有 `SaveData`/`SaveService` 构造 MapComplete fixture，再通过真实输入 `1/2/3`、`E` 推进地图；覆盖连续 5 次 `mapLevel` 增长、地图状态重置和成长状态保留。
6. 增加至少一条失败保护测试：奖励未选不能进图、地图未选不能进图、坏档/非法地图选项不能污染当前 run、MapComplete 的背包/地面掉落不被错误清空。
7. 如果发现 Renderer 只读接口或 HUD 与实际状态不一致，只做最小修正；不得顺手新增技能、怪物 AI、地图类型、装备槽、商店、loot filter、音频或美术资源。

必须验证：

- MSVC clean build，`ctest --test-dir build --output-on-failure` 全部通过。
- 直接运行三类测试并报告精确数量；`arpg_world_tests` 至少保留当前 `34 passed` 基线并增加连续地图覆盖。
- `PlaneShooter.exe` 启动 3 秒保持运行；若做 UI 调整，至少检查 800x600 下 Pause、MapComplete、Inventory/Stash 不重叠。
- 交付报告列出审计到的公式/数据、实际修改文件、测试场景、未修复风险和 `git status --short`；实现 Agent 不提交代码，由主 review Agent 复核后提交。

明确不做：完整难度选择界面、随机地牢生成、更多技能/Support、职业系统、交易/经济扩展、音频、复杂美术、跨运行存档和大型 GameWorld 重构。

验收结果：

- `GameWorld::restoreFromSaveData()` 现在校验当前地图模板与 MapOption 一致，并拒绝越界选择、未选奖励却已选下一图、Playing 存档携带结算选择等非法阶段组合。
- `arpg_world_tests` 使用 `SaveData/SaveService` 构造合法 MapComplete 存档，再通过真实 `1 -> 1 -> E` 输入连续推进五次，从 Map 1 到达 Map 6。
- 连续推进验证 Inventory、Stash、玩家等级、技能奖励状态保留；旧地面掉落被清空；新地图事件重新初始化；未满足奖励/地图选择条件时 `E` 不推进。
- `arpg_logic_tests`：`920 passed / 0 failed`；`arpg_save_tests`：`11 passed / 0 failed`；`arpg_world_tests`：`85 passed / 0 failed`。
- clean build、CTest `3/3` 和 3 秒启动 smoke test 通过；代码提交为 `7e541a7 Harden map progression save validation`。

已知约束：连续地图测试通过 SaveData fixture 模拟 Boss 击杀，因此仍缺少无辅助输入的完整战斗通关自动化；这不是生产 debug API 的理由，后续应优先增加可测试的纯战斗/遭遇边界，而不是暴露私有世界容器。

### 13.16 已完成任务记录：多 Support Link v1

目标：把每个技能的单 Support 限制升级为最小的 PoE-like 双 Link 构筑，并让实际施法、技能面板、装备比较和存档使用同一份结果。

实现结果：

- `SupportLibrary` 定义固定双 Link 数据结构；非 Movement 技能提供 2 个 Link，Movement 保持 1 个 Link；`SkillBar` 提供按 Link 查询、分配、清除和兼容性校验。
- 同一 Support 不可重复安装，非兼容 Support、Movement 第二 Link、未知 Support 和坏存档字段都会被拒绝；未解锁 Support 只能通过结算奖励解锁，不能被 Skill Panel 快捷键绕过。
- `CombatMath` 统一聚合全部 Link：伤害/冷却/范围倍率相乘，穿透/额外投射物/散射角累加，Ignite/Chill 的持续时间、强度和穿透按数据字段聚合；GameWorld 的 Projectile、Area、异常和 Movement 施法使用该路径。
- `Renderer` 的 Skill Panel、装备替换预览和技能实际值预览改用多 Link 结果；面板显示两个 Link、兼容 Support 池以及 `E/A/L` 状态（已装备/可用/锁定）。F5/F6 选择 Link，F1-F4 在对应槽位循环 Support。
- `SaveData::Version` 升级为 2；SkillBar 的每个槽位序列化两个 Support 名称。旧 v1、截断、CRC 损坏、非法重复和超出 Link 数量的存档不会污染当前运行。

验收结果：

- `arpg_logic_tests`：`930 passed / 0 failed`，覆盖双 Link 聚合、兼容/重复/Movement 边界和 SkillBar round-trip。
- `arpg_save_tests`：`11 passed / 0 failed`，覆盖双 Link 序列化、旧 v1 拒绝、未知版本、CRC、截断和原子替换保护。
- `arpg_world_tests`：`85 passed / 0 failed`，地图推进、Pause、MapComplete、存档恢复回归通过。
- MSVC clean build、CTest `3/3`、三套测试直接运行和 3 秒启动 smoke 全部通过。
- 主 review 修正了 Grok 产物中“数据结构已经双 Link、但战斗/Renderer 仍只读取第一条”的闭环缺口，最终提交为 `595f03f Add multi-support skill links`。

已知约束：当前仍没有无辅助输入的完整 Boss 通关端到端测试；现有世界测试用合法 SaveData fixture 进入 MapComplete，这是测试边界而非生产 debug API。Support 仍是单局解锁、固定 Link 数量，没有颜色、等级、质量或 Support 物品。

### 13.17 已完成任务记录：战斗反馈与完整 Boss 通关验证 v1

代码已通过主 review 并提交为 `5a798dc Add combat hit feedback and Boss flow tests`。

实现结果：

- 新增有界的 `CombatFeedback` 数据通道：每条记录保存世界坐标、实际伤害、技能来源和剩余时间；全局最多 32 条，生命周期 0.8 秒，过期和切图时清理。Renderer 只读该通道并绘制上浮伤害数字。
- `Enemy::takeDamage()` 现在只返回实际扣除的生命值并把生命值限制在零以上；`claimKillReward()` 保证普通怪、精英和 Boss 的经验、掉落、地图统计和 Boss 结算最多执行一次。
- Primary 投射物、Secondary/Utility 范围技能和 Dash 伤害均记录实际命中值；投射物保存施放时的技能来源，不会因玩家随后切换技能而改变历史反馈。玩家受击仍复用原有护甲、Shrine/Support 外公式和无敌帧，并保留现有 `HIT -N source` 反馈。
- 新增真实 GameWorld 战斗测试：通过合法 `SaveData` fixture 仅提高测试角色的移动、伤害和承伤容错，使用真实 WASD 进入 Boss Arena，再用真实右键技能降低 Boss HP，验证反馈、MapComplete 和 Boss 保底地面掉落。
- 验收基线：`arpg_logic_tests 930 / 0`、`arpg_save_tests 11 / 0`、`arpg_world_tests 101 / 0`；CTest `3/3`、MSVC clean build、三套测试直接运行和 3 秒启动 smoke 全部通过。

已知约束：当前伤害数字覆盖直接技能命中，Ignite 周期伤害仍复用 Enemy 的异常生命周期并通过正常死亡奖励路径结算，但没有单独生成每个 DOT tick 的数字；Renderer/UI 尚未纳入自动像素测试。后续涉及异常反馈时必须先补测试，不得在 Renderer 中复制伤害计算。

原计划目标：在继续增加内容前，证明玩家能理解并完成“移动 -> 命中 -> 受伤 -> 进入 Boss Arena -> 击杀 Boss -> 拾取掉落”的真实战斗闭环。重点是战斗反馈和可测试边界，不新增职业、技能或地图系统。

开始前必须阅读：

- 本文档第 2、4、5、7、11、13.15、13.16 节。
- `GameWorld::tryCast*()`、`handleCollisions()`、`handleBossProjectileCollisions()`、`rewardEnemyKill()`、`isMapCleared()`、`triggerBossIfNeeded()`。
- `Enemy.hpp`、`EnemyDefinition.hpp`、`BossDefinition.hpp`、`CombatMath.hpp`、`Renderer.cpp` 的敌人/Boss 绘制和 HUD。
- `tests/arpg_logic_tests.cpp`、`tests/game_world_logic_tests.cpp`、`tests/save_logic_tests.cpp`；先运行三套测试并记录基线。

实施范围：

1. 增加一个有上限的战斗反馈数据通道，例如短生命周期的 `CombatFeedback`/damage number 记录；记录来源、伤害值、世界坐标和剩余时间，不能让 Renderer 修改 GameWorld 状态。单帧/全局数量必须有硬上限，避免长时间刷怪导致无限增长。
2. 玩家技能命中普通怪、精英和 Boss 时显示可读的伤害反馈；Boss 受伤时 HUD 血条、名称、阶段提示和伤害反馈必须同步，不得出现“血条掉了但没有命中反馈”或“显示命中但实际没有扣血”。
3. 玩家受到普通怪接触伤害、敌人投射物和 Boss AoE/冲刺伤害时提供统一受击反馈；保持现有无敌帧、护甲、异常和 Shrine/Support 公式，不在 Renderer 复制伤害计算。
4. 增加可自动测试的战斗边界，优先使用纯 `CombatMath`/Enemy 生命周期测试或现有公开行为；不要为了测试暴露生产环境的敌人容器、强制击杀 API 或 debug 按钮。至少覆盖：Support 双 Link 命中伤害、Boss HP 下降后血条查询一致、死亡奖励只结算一次、Boss 击杀后进入 MapComplete 且保底掉落存在。
5. 如果必须补公开只读 getter，只暴露稳定业务信息（Boss 当前 HP/max HP、反馈记录只读视图）；不要把私有碰撞步骤搬到测试专用分支。事件短提示、地面拾取最近目标、MapComplete 背包管理必须保持不回归。
6. UI 只做 800x600 可读性修正：伤害数字不遮挡目标名称和 Boss 血条，MapComplete 的奖励/地图选项、Inventory/Stash、Skill Panel 不新增重叠。不要引入贴图、粒子库、音频或复杂动画系统。

必须验证：

- 固定 seed 下，至少一条真实技能命中路径能观察到普通怪和 Boss 的 HP 变化；Support 双 Link、Projectile/Area 专精和 Shrine buff 的最终伤害与反馈数字一致。
- 普通怪、精英、Boss 的死亡奖励不会重复计数；Boss 击杀后掉落、MapComplete、F 拾取和下一图选择流程完整。
- `arpg_logic_tests`、`arpg_save_tests`、`arpg_world_tests` 全部通过并报告精确数量；至少新增 Boss/战斗边界测试，不接受只说“构建通过”。
- MSVC clean build、CTest `3/3`、三套测试直接运行、3 秒启动 smoke；完成后保持工作区未提交，由主 review Agent 检查 diff、修复、提交并更新本手册。

明确不做：新技能、新 Support、新 Boss、新地图事件、职业系统、装备新槽、掉落过滤、自动拾取、音频、美术资源、网络、跨运行存档和大型 GameWorld 重构。

### 13.18 已完成任务记录：构筑数值一致性与战斗回归基准 v1

目标：在继续增加技能、Support、Boss 或经济内容前，证明“天赋/装备/Support/Shrine -> 技能实际伤害与范围/冷却 -> 命中反馈 -> 装备预览”的数值链路只有一套真相。重点是消除 Renderer 预览、CombatMath、GameWorld 施法三者之间的漂移，让玩家能相信面板上看到的数值。

开始前必须阅读：

- 本文档第 2、4、5、7、11、13.16、13.17 节。
- `include/CombatMath.hpp`、`include/Stats.hpp`、`include/Skill.hpp`、`include/SkillBar.hpp`、`include/SupportLibrary.hpp`。
- `GameWorld::damageForPlayerSkill()`、`radiusForPlayerSkill()`、`pierceCountForPlayerSkill()`、`projectileCountForPlayerSkill()`、`spreadAngleForPlayerSkill()`、`ailmentForPlayerSkill()`。
- `src/Renderer.cpp` 中的 `effectiveSkillDamage()`、`effectiveSkillRadius()`、`effectiveSkillCooldown()`、装备替换预览和 Skill Panel 预览。
- `tests/arpg_logic_tests.cpp`、`tests/game_world_logic_tests.cpp`、`tests/save_logic_tests.cpp`；先运行三套测试并记录基线。

实施范围：

1. 先做公式审计，列出每个技能槽位最终使用的伤害、半径、冷却、投射物数量、散射角、穿透和异常快照来源。优先复用 `CombatMath` 与 `SkillBar::supportDefinitionsFor()`；如果发现 Renderer 中存在与 `CombatMath` 重复的公式，应抽成无 SFML 的纯 helper 或直接改为调用现有 helper，不能复制第三套计算。
2. 增加纯逻辑构筑矩阵测试，至少覆盖以下固定场景：
   - Projectile：Projectile 天赋/装备专精 + 两个合法 Support + Shrine 开启时，Primary 实际命中伤害与 `skillDamage()` 一致；Area 专精不能额外放大该伤害。
   - Area：Area 天赋/装备专精 + 两个合法 Support + Shrine 开启时，Secondary/Utility 的伤害和实际判定半径与 `skillDamage()`/`skillRadius()` 一致；Projectile 专精不能额外放大 Area 技能。
   - Primary 攻速只影响冷却；Secondary、Utility、Movement 不得因为 `attackSpeedMultiplier` 被错误缩短。
   - Support 的额外投射物、散射角、穿透、半径、异常持续时间/穿透按两个 Link 聚合；同一个 Support、非法槽位和未解锁 Support 继续被拒绝。
   - Shrine 只作为临时技能伤害乘区，不改变永久 `Stats`、装备属性、技能半径或冷却；buff 结束后数值恢复。
3. 增加至少一条真实 GameWorld 路径回归：对同一个固定 seed 和同一目标，比较施放前后 `combatFeedback` 的实际伤害、目标 HP 变化和 `SkillBar` 当前定义；至少覆盖一个 Projectile 和一个 Area 技能。测试不允许直接修改 GameWorld 私有容器，不新增强制击杀、跳图或 debug 输入。
4. 校正装备详情面板与 Skill Panel 的预览：预览必须使用与施法相同的 Support 列表和 `CombatMath` helper；特别检查双 Link、Primary cooldown、Projectile/Area 专精、Shrine 不在预览中被错误永久化。若只需改显示格式，保持现有 800x600 布局，不重做面板。
5. 如发现数值异常，只做最小修正并补对应测试。禁止借机调整整体难度、装备掉率、Boss HP、地图 modifier、天赋节点数值或技能基础值；任何平衡改动另开任务。

强制约束：

- 不新增技能、Support、Boss、地图事件、敌人类型、装备槽、货币、商店、loot filter、存档字段或输入键。
- 不在 `Renderer.cpp` 读取/修改 GameWorld 私有状态，不把测试专用 getter 暴露给生产代码；只允许稳定业务只读接口。
- 不按技能名称写分支。技能差异必须由 `SkillCastType`、`SkillDefinition`、Support 数据和 `CombatMath` 表达。
- 不修改 Shrine/Support/天赋/装备的设计语义；不要为了通过测试把期望值写成当前实现的硬编码快照。
- 实现 Agent 不提交代码；保持工作区未提交，由主 review Agent 审查、修正、clean build 后提交。

必须验证：

- `arpg_logic_tests`、`arpg_save_tests`、`arpg_world_tests` 全部通过，并报告精确数量；不得只报告构建成功。
- MSVC clean build、CTest `3/3`、三套测试直接运行、`PlaneShooter.exe` 启动 3 秒 smoke 全部通过。
- 交付报告必须列出：公式审计结论、实际改动文件、每个构筑矩阵的预期/实际值、真实命中路径、UI 预览与施法值的对照、未修复风险和 `git status --short`。

完成定义：所有已有技能的面板预览、装备替换预览和真实施法都调用同一套计算路径；至少一条 Projectile 和一条 Area 真实命中测试证明反馈伤害等于目标 HP 的实际变化；三套测试和启动 smoke 通过；主 review Agent 提交后才更新进度看板。

实现结果：

- 新增 `CombatMath::skillCooldown()`，将 Support 冷却倍率和 Primary 攻速规则集中到纯计算 helper；`SkillBar` 的冷却刷新、Renderer 预览和现有技能栏数据现在使用同一套公式。
- 新增 `SkillBar::supportDefinitionsFor()`，按候选技能的 `SkillCastType`/Support 兼容性过滤当前 Link。Skill Panel、装备替换详情和 GameWorld 的伤害、范围、投射物、散射、穿透、异常计算均不会再把当前槽位的非法 Support 错误带入候选技能预览或实际施法。
- Shrine 持续时间和 `+35%` 伤害倍率收敛到 `Config`，HUD、事件提示、Dash 和玩家技能伤害读取同一数据源；Shrine 仍是临时伤害乘区，不修改永久属性、范围或冷却。
- 新增纯逻辑构筑矩阵：Projectile/Area 专精隔离、双 Support 聚合、Primary 攻速冷却、Secondary 冷却不受攻速影响、Shrine 不污染 `Stats`，并覆盖候选技能 Support 预览与实际切换清理。
- 新增真实 GameWorld 回归：通过合法 `SaveData` fixture 恢复双 Link 构筑，真实移动进入 Boss Arena，使用真实左键 Projectile 和右键 Area 命中 Boss，验证 CombatFeedback 伤害、Boss HP 变化和 Area 半径与 `CombatMath` 一致。
- 主 review 发现并修正了 `SkillBar::applyStats()` 仍读取未过滤 Support 列表的潜在路径分叉，最终代码提交为 `05ff74a Unify skill build math and previews`。

验收结果：

- `arpg_logic_tests`：`948 passed / 0 failed`。
- `arpg_save_tests`：`11 passed / 0 failed`。
- `arpg_world_tests`：`111 passed / 0 failed`。
- MSVC `--clean-first` 构建、CTest `3/3`、三套测试直接运行和 `PlaneShooter.exe` 3 秒启动 smoke 全部通过。

已知约束：本轮仍没有 Renderer/UI 像素级自动测试；直接命中已有伤害数字，但 Ignite 周期伤害尚未产生单独、可区分的每 tick 反馈。后续异常反馈必须继续复用 `CombatMath`，不能在 Renderer 复制伤害公式。

后续路线（不属于本轮实施范围）：

1. 敌群与精英可读性 v2：补精英包角色、词缀风险和掉落价值反馈，保持现有 AI 与地图路线。
2. 构筑内容扩展 v2：在数值基准稳定后再增加少量技能/Support，并要求每个新增数据定义同时提供兼容性、预览和自动测试。
3. 运行完成度 v2：补结算统计、失败/重试节奏和更完整的 UI 视觉验收；不以新增系统数量代替可玩性验证。

### 13.19 已完成任务记录：状态异常反馈与死亡奖励一致性 v2

目标：把 Ignite 的周期伤害纳入与直接命中相同的可读反馈和死亡结算链路，证明“异常 tick -> 实际 HP 变化 -> 反馈数字 -> 一次性经验/掉落/地图统计”没有重复或漂移。只处理现有 Ignite/Chill 和现有敌人，不借机扩展内容。

开始前必须阅读：

- 本文档第 2、4、5、7、11、13.17、13.18 节；本任务开始时基线为 `948/11/111`。
- `include/Enemy.hpp`、`src/Enemy.cpp` 或 Enemy 异常生命周期实现、`Enemy::updateAilments()`、`Enemy::applyIgnite()`、`Enemy::takeDamage()`、`Enemy::claimKillReward()`。
- `include/CombatMath.hpp` 中 Ignite/Chill 抗性、穿透、tick damage 和 duration helper；`include/EnemyDefinition.hpp`、`include/BossDefinition.hpp` 中现有抗性数据。
- `GameWorld::updateObjects()`、`handleCollisions()`、`rewardEnemyKill()`、`addCombatFeedback()` 以及 `src/Renderer.cpp` 的 CombatFeedback 绘制和 HUD。
- `tests/arpg_logic_tests.cpp`、`tests/game_world_logic_tests.cpp`、`tests/save_logic_tests.cpp`；理解现有“实际伤害而非原始伤害”和“死亡奖励 claim 一次”的测试写法。

实施范围：

1. 让 Enemy 异常更新返回实际 tick 结果：优先返回一个小型纯数据结果，例如 `{actualDamage, expired, killed}`，或等价的 `std::optional` 结果。Enemy 不得依赖 GameWorld、Renderer 或存档；tick damage 必须沿用现有快照的 Ignite 数据、抗性和穿透计算。
2. 在 GameWorld 接收 tick 结果后写入 `CombatFeedback`，来源固定为稳定文本（建议 `Ignite` 或 `Ignite: <skill>`），位置使用敌人当前位置，伤害使用 Enemy 实际扣除 HP 的值。反馈必须经过现有 `MaxCombatFeedback`/`CombatFeedbackDuration` 上限，不允许新增无界队列。
3. 统一 DOT 死亡处理：Ignite tick 将敌人 HP 降到 0 时，必须复用现有 `takeDamage()`/`claimKillReward()` 语义；经验、`mapKills`、掉落、ElitePack 完成和 Boss `MapComplete` 最多结算一次。过量 tick 不得产生负 HP 或超过实际 HP 的反馈数字。
4. 保持现有规则：Chill 不产生伤害 tick；Boss/Elite/Normal 的抗性来自定义数据；Support 的 Ignite damage、duration 和 penetration 仍由施法时快照决定；异常刷新/覆盖规则不能被改成按名称硬编码。
5. Renderer 只增加最小可读性：让 Ignite 反馈与直接命中可区分（颜色或稳定 source 文本即可），不得复制 tick 计算、修改 GameWorld 状态或重做 HUD/结算面板。若当前 `CombatFeedback` 已足够显示来源，只需补颜色/文案测试，不要增加复杂动画。
6. 补测试，至少包括：
   - 纯逻辑：Ignite tick 经过抗性和穿透后的实际伤害；tick 伤害在 HP 不足时 clamp；Chill 始终无 tick；duration 到期后不再 tick。
   - 纯逻辑：死亡敌人不能再次受到 tick 或重复 claim reward。
   - GameWorld：通过现有合法 `SaveData` fixture 或公开行为触发一个 Ignite 敌人，验证产生 `Ignite` feedback、反馈伤害等于 HP 变化，并验证死亡后奖励/MapComplete 只发生一次。禁止暴露强制击杀或测试专用生产 API。

强制约束：

- 不新增技能、Support、Boss、敌人类型、地图事件、装备槽、货币、商店、存档字段或输入键。
- 不修改 Ignite/Chill 基础数值、Boss HP、敌人生成密度、地图 modifier、掉率和奖励数值；发现平衡问题只记录，不在本任务修平衡。
- 不在 Renderer 或测试中复制 `CombatMath` 公式；生产路径和测试必须调用现有 helper。
- 不以技能名称写 GameWorld 分支；异常类型和数据使用现有 `AilmentDefinition`/`SkillCastType`/定义字段表达。
- 不暴露 GameWorld 私有敌人容器的可写访问，不新增强制伤害/跳图/debug 输入。只允许稳定业务只读的 feedback 或 Boss 信息。
- 保持现有 F 最近拾取、Tab/Delete 背包管理、P 天赋盘、K 技能面板、MapComplete 奖励/地图选择和 Pause 输入上下文不变。
- 实现 Agent 不提交代码，不修改本手册；完成后保持工作区未提交，交付完整 diff、测试数量、clean build、CTest、启动 smoke、未修复风险。

必须验证：

- 三套测试全部通过，并报告精确数量；新增测试数量必须相对 `948/11/111` 增长。
- 必须执行 MSVC `cmake --build build --clean-first`、CTest `3/3`、三套测试直接运行和 `PlaneShooter.exe` 启动 3 秒 smoke。
- 至少一条真实 Ignite tick 路径验证：CombatFeedback 实际伤害 = 敌人 HP 实际减少量；若 tick 击杀敌人，经验/掉落/地图统计/MapComplete 不重复。
- 交付报告列出：改动文件、Enemy tick API、GameWorld 接入点、feedback source/color、过量伤害处理、奖励 claim 证据、测试场景与数量、构建结果、未修复风险和 `git status --short`。

完成定义：Ignite tick 使用现有数值真相，真实反馈可见且有上限，HP/反馈/死亡奖励一致，现有三套测试和启动 smoke 通过；主 review Agent 完成 diff review、必要修正并提交后，才把本任务标记为完成。

实现结果：

- `Enemy::updateAilments()` 现在返回纯 `AilmentTickResult`，包含异常类型、聚合后的实际伤害、tick 数和是否击杀；Enemy 不依赖 GameWorld、Renderer 或存档。
- Ignite tick 继续使用现有快照伤害、抗性和穿透结果，并通过 `takeDamage()` 返回实际扣血；过量伤害自动限制到剩余 HP，死亡后不再产生 tick。
- `GameWorld::updateObjects()` 接收 tick 结果，使用现有 `addCombatFeedback()` 写入有界 `Ignite` 反馈，再复用 `rewardEnemyKill()`/`claimKillReward()`，因此经验、地图击杀、掉落、ElitePack 完成和 Boss 结算不会重复。
- Renderer 对 `Ignite` 来源使用暖色数字；直接命中继续使用原有颜色，反馈仍受 `MaxCombatFeedback` 和 `CombatFeedbackDuration` 限制。
- 新增纯逻辑覆盖实际 tick、过量 clamp、死亡后禁止 tick、Chill 无伤害和 Ignite 到期；新增真实 GameWorld 测试，通过合法 SaveData fixture、真实 Secondary 施法和真实高等级 Normal/Ranged 目标验证 Ignite 反馈、HP 变化和一次性击杀奖励。
- 主 review 发现并修正了随机目标导致的 DOT 击杀测试不稳定，最终提交为 `700a760 Add Ignite combat feedback`。

验收结果：

- `arpg_logic_tests`：`953 passed / 0 failed`。
- `arpg_save_tests`：`11 passed / 0 failed`。
- `arpg_world_tests`：`119 passed / 0 failed`。
- MSVC `--clean-first` 构建、CTest `3/3`、三套测试直接运行和 `PlaneShooter.exe` 3 秒启动 smoke 全部通过。

已知约束：同一帧内多个 Ignite tick 会聚合为一条反馈；反馈没有携带敌人 ID，当前需求只要求来源和实际总伤害可读。Renderer/UI 仍没有自动像素测试；后续 UI 任务必须保留 800x600 可读性检查。

### 13.20 已完成任务记录：敌群与精英可读性 v2

目标：让玩家在开放地图中能快速判断精英/精英包的危险、当前状态和潜在收益，形成“看见风险 -> 决定是否处理 -> 拾取价值”的 ARPG 反馈闭环。只增强现有 EnemyDefinition、EliteModifier、ElitePack 事件和 HUD/敌人标签，不新增敌人 AI、敌人类型或地图生成。

开始前必须阅读：

- 本文档第 2、4、5、7、11、13.18、13.19 节；先运行三套测试，记录当前基线 `953/11/119`。
- `include/EnemyDefinition.hpp`、`include/EliteModifier.hpp`、`include/Enemy.hpp`、`src/Enemy.cpp`；确认名称、HP/速度/爆炸数据和 `isElite()` 的现有来源。
- `GameWorld::spawnEnemies()`、`triggerElitePackEvent()`、`noteElitePackEnemyDefeated()`、`rewardEnemyKill()`、`activeEliteEventEnemiesRemaining()`。
- `src/Renderer.cpp` 的 `drawEnemies()`、HUD Events 区、小地图和 `drawCombatFeedback()`；确认当前颜色/标签和世界坐标到屏幕坐标规则。
- `tests/arpg_logic_tests.cpp`、`tests/game_world_logic_tests.cpp`、`tests/save_logic_tests.cpp`；先找现有 EliteModifier、ElitePack、掉落和地图事件断言，禁止复制已有测试。

实施范围：

1. 数据化精英说明：为现有 `EliteModifierDefinition` 增加短 `description` 或等价的显示字段，覆盖 Hardened、Swift、Volatile；None 仍为空。说明必须从数据表读取，不能在 Renderer 通过 modifier 名称写一套 switch。数值字段和现有平衡保持不变。
2. 统一精英展示文本：Renderer 继续使用 `EnemyDefinition`/`EliteModifierDefinition` 生成名称，并在精英标签或附近 HUD 展示一行风险摘要，例如 Hardened 的生命风险、Swift 的速度风险、Volatile 的死亡爆炸风险。实际数值格式化集中在一个小 helper，不能在多个绘制点复制字符串拼接。
3. 增加当前精英焦点信息：在 Playing 状态下从现有 `world.enemies()` 只读选择距离玩家最近且在可读范围内的 Elite/Boss，HUD 显示名称、当前 HP/max HP、modifier 摘要和必要的掉落/奖励提示。没有合适目标时不显示；不得新增可写敌人容器或目标锁定输入。
4. 强化 ElitePack 反馈：保留现有自动触发和 `Events X/Y`，在事件进行中同时显示当前剩余数量和已生成精英的 modifier 摘要；事件完成后显示一次短提示，不改变事件完成条件、生成数量、掉落数量和奖励倍率。
5. 地面奖励可读性：只复用现有 `mapItemsDropped`、稀有度颜色和最近拾取目标，在 Elite/ElitePack 的掉落提示中显示“来源为 Elite Pack”或等价稳定文案；不添加 loot filter、自动拾取、稀有度重排或新的掉落池。
6. 补测试，至少包括：
   - 纯逻辑：三个现有 EliteModifier 都有非空描述；None 不产生风险摘要；描述与对应数值字段一致，不能出现 Volatile 文案但爆炸数据为零等错配。
   - 纯逻辑：EnemyDefinition 和 EliteModifier 的 HP/速度/爆炸聚合公式保持当前值，普通敌人不会错误显示精英 modifier。
   - GameWorld：真实 ElitePack 事件生成 1 Elite + 4 Normal，事件完成数量只在全部死亡后增加；至少一条现有掉落/奖励断言保持通过。
   - 回归：Boss 标签、Ignite/Chill 反馈、最近拾取、高亮、MapComplete 背包和三选图输入不回归。

强制约束：

- 不新增技能、Support、Boss、敌人类型、地图事件类型、装备槽、货币、商店、存档字段或输入键。
- 不修改 EliteModifier、EnemyDefinition、地图 modifier、Boss HP、生成数量、掉率、经验和奖励倍率；本任务只增加可读数据和显示，任何平衡改动另开任务。
- 不在 Renderer 复制敌人/掉落/奖励计算；数值仍来自现有定义和 GameWorld 只读接口。不得按 modifier 名称散落写逻辑分支。
- 不暴露 GameWorld 私有容器的可写访问，不引入目标锁定、自动拾取、loot filter 或 UI 大重构。
- 保持 WASD、左键/右键/Q/Space、F 最近拾取、Tab/Delete、P/K、Pause 和 MapComplete 输入上下文不变。
- 实现 Agent 不提交代码，不修改本手册；完成后保持工作区未提交，交付完整 diff、测试数量、clean build、CTest、启动 smoke、未修复风险。

必须验证：

- 三套测试全部通过，且测试数量相对 `953/11/119` 增长或至少新增明确的精英/事件断言。
- 必须执行 MSVC `cmake --build build --clean-first`、CTest `3/3`、三套测试直接运行和 `PlaneShooter.exe` 启动 3 秒 smoke。
- 手动检查 800x600：普通敌人、Elite、Volatile 标签、Boss 血条、Events 区、Ignite 数字和最近掉落提示不互相覆盖。
- 交付报告列出：新增数据字段、文本来源、焦点选择规则、ElitePack 状态变化、掉落来源文案、输入回归、测试场景与数量、构建结果、未修复风险和 `git status --short`。

完成定义：精英风险和事件进度在真实地图中可读，展示使用数据定义而非名称分支，ElitePack 状态/奖励不改变且测试与启动 smoke 通过；主 review Agent 完成 diff review、必要修正并提交后，才把本任务标记为完成。

实现结果：

- `EliteModifierDefinition` 增加数据化 `description`：Hardened、Swift、Volatile 分别描述生命、速度和死亡爆炸风险，None 保持空描述；没有改变任何 modifier 数值、颜色或奖励规则。
- Renderer 新增统一的精英名称、风险摘要和焦点选择 helper。Playing HUD 会从只读 `world.enemies()` 选择 520 世界单位内距离玩家最近的 Elite/Boss，显示名称、当前 HP/max HP 和 modifier 风险；精英标签继续使用 `EnemyDefinition`/`EliteModifierDefinition` 数据。
- ElitePack 进行中显示剩余数量和与活动事件位置最近的精英 modifier 摘要；完成提示改为 `Elite pack cleared - check nearby loot`，为地面奖励提供稳定上下文，不新增掉落来源字段或改变掉率。
- 新增纯逻辑断言：三个 modifier 描述与数值数据一致，None 无描述，普通敌人不会保留 Elite modifier。
- 新增真实 GameWorld ElitePack 测试：合法 SaveData fixture 通过真实移动进入事件，验证生成 1 Elite + 4 Normal、活动状态不计为完成，真实 Area 技能击杀五个事件敌人后才完成并显示掉落反馈。
- 主 review 发现并修正了活动 ElitePack 摘要可能误读其他场上 Elite 的风险：现在按未完成 ElitePack 事件位置选择对应精英，不使用名称 switch。

验收结果：

- `arpg_logic_tests`：`957 passed / 0 failed`。
- `arpg_save_tests`：`11 passed / 0 failed`。
- `arpg_world_tests`：`127 passed / 0 failed`，其中包含 ElitePack 真实事件回归。
- MSVC `cmake --build build --clean-first`、CTest `3/3`、三套测试直接运行和 `PlaneShooter.exe` 3 秒启动 smoke 全部通过。
- 最终代码提交：`4d1c639 Improve elite and pack readability`。

已知约束：当前仍没有 Renderer/UI 像素级自动测试；焦点目标是只读的最近目标，不提供锁定输入；掉落来源使用 ElitePack 完成短提示而不是为每个 `DroppedItem` 增加持久化 source 字段。后续若需要精确区分多个事件来源，应先设计不破坏存档版本的掉落来源模型。

### 13.21 已完成任务记录：连续刷图完成度 v2

目标：在继续增加新技能、Support 或敌人之前，证明系统可以稳定完成至少 5 张连续地图，并保护死亡、结算、拾取、背包/Stash、奖励、地图选择和存档恢复边界。

实现与验收结果：

- `tests/game_world_logic_tests.cpp` 新增共享的真实移动、Boss 瞄准施法和统计比较 helper。
- 新增 5 张连续真实流程：WASD 移动到 Boss Arena，真实右键 Area 技能击杀 Boss，F 拾取最近掉落，Tab + I 转存 Stash，数字键分阶段选择奖励/地图，E 进入下一张地图。
- 每张地图验证 Boss 掉落存在、地面掉落和三张事件在下一图清空、Inventory/Stash 所有权保留、等级/装备/天赋分配/技能解锁保留，并显式验证 `Pierce` Support link 保留。
- 新增 GameOver 边界：真实承伤进入 GameOver，移动输入无效，R 生成新 run 并清空旧 Inventory/Stash/掉落/事件。
- 新增存档阶段边界：Paused MapComplete 保存后恢复为 MapComplete；只选择奖励的中间存档加载后仍阻止 E，补选地图后才能进入下一图。
- 生产代码没有增加 debug kill、debug teleport、测试专用可写容器、输入键、存档字段或玩法内容。

验收结果：

- 代码提交：`601b279 Harden continuous map progression tests`。
- MSVC `cmake --build build --clean-first`：通过。
- CTest：`3/3` 通过。
- 直接测试：`arpg_logic_tests 958/0`、`arpg_save_tests 11/0`、`arpg_world_tests 254/0`。
- `PlaneShooter.exe` 启动 3 秒 smoke：通过。

已知约束：本轮连续流程测试没有提供 Renderer 截图；13.22 已补上固定坐标审计和启动 smoke，但当前仍无 Renderer/UI 像素级自动测试。

### 13.22 已完成任务记录：800x600 UI 可读性与流程提示 v1

目标：在不改变玩法状态机、输入语义和数值的前提下，让 `800x600` 原型窗口中的探索、Boss、掉落、结算和构筑面板保持可读。

实现与 review 结果：

- Playing HUD 的地图、Boss、目标、事件、拾取和 Build 文案统一做宽度限制；事件提示限制在 Boss 面板左侧安全列，避免动态文本穿透 Boss 血条。
- 装备和背包列表统一截断；普通装备详情面板收敛到 `500x250`，最多展示 3 条词缀并报告剩余数量，避免压到底部技能栏；MapComplete 地面掉落详情使用 `300x240` 紧凑面板。
- Skill Panel 改为两列 Equipped/Skills/Supports 布局，技能和 Support 池文本有界，第二行 Support 仍位于面板底部安全区。
- Boss 血条移到中间安全列并缩短附加文本；小地图移到右下角、技能栏上方，避免遮住顶部地图/装备信息。
- MapComplete 改为左侧拾取与掉落详情、中间 Boss/奖励/地图选择、右侧背包与 Stash 的三列布局，保留奖励选择、地图选择、E 进入、F 拾取和 Tab/Delete 管理。
- 仅修改 `include/Renderer.hpp` 与 `src/Renderer.cpp`，没有新增玩法、输入、存档字段或测试专用接口。

验收结果：

- 代码提交：`e310894 Improve 800px ARPG HUD readability`。
- MSVC `cmake --build build --clean-first`：通过；随后增量构建也通过。
- CTest：`3/3` 通过。
- 直接测试：`arpg_logic_tests 958/0`、`arpg_save_tests 11/0`、`arpg_world_tests 254/0`。
- `PlaneShooter.exe` 启动 3 秒 smoke：通过，且清理了本轮启动产生的残留进程。
- 主 review 做了固定坐标/文本宽度审计和运行 smoke；当前环境没有截图/像素级 UI 测试，因此不宣称完成自动像素验收。

已知约束：Renderer/UI 仍没有像素级自动测试；当前验证是代码坐标审计、clean build、现有业务回归和启动 smoke。后续若再调整布局，必须保留 800x600 安全列和 MapComplete 三阶段检查。

### 13.23 hy3 下一项实施任务：构筑内容扩展 v3（2 技能 + 2 Support）

目标：在现有“技能解锁奖励 -> Skill Panel 分配 -> 双 Support Link -> CombatMath 实际施法”闭环稳定后，增加少量可验证内容，让玩家的 Projectile/Area 构筑出现新的选择。此任务只增加数据和通用计算覆盖，不扩展技能槽、输入系统、地图系统或存档格式。

开始前必须阅读：

- 本文档第 2、3、7、11、13.16、13.18、13.21、13.22 节；代码基线为 `e310894`，测试基线为 `958/11/254`。
- `include/Skill.hpp`、`include/SkillLibrary.hpp`、`include/SupportLibrary.hpp`、`include/SkillBar.hpp`、`include/CombatMath.hpp`；确认现有 `SkillCastType`、双 Link、兼容性和预览路径。
- `include/MapRewardLibrary.hpp`、`GameWorld` 的 `RunProgression`/解锁校验、`SaveData` 的技能和 Support 集合；奖励必须自动看到新定义，不能另写名称列表。
- `src/GameWorld.cpp` 的 Primary/Secondary/Utility 通用施法路径与 `src/Renderer.cpp` 的 Skill Panel/装备详情预览；新增内容不能依赖 Renderer 分支。
- `tests/arpg_logic_tests.cpp`、`tests/game_world_logic_tests.cpp`、`tests/save_logic_tests.cpp`；先运行并记录 `958/11/254`，不得复制已有测试。

固定内容范围：

1. 新增主动技能 `Arc Bolt`：`Primary`、`Projectile`，单投射物、无散射、基础伤害 `3`、冷却 `0.65s`、Mana `2`；不得新增新的 CastType 或 GameWorld 技能名称分支。
2. 新增主动技能 `Shockwave`：`Utility`、`SelfCenteredArea`，半径 `120`、基础伤害 `3`、冷却 `1.50s`、Mana `6`；使用现有 Area 专精、Support、Shrine 和半径计算路径。
3. 新增 Support `Barrage`：仅兼容 Projectile；`extraProjectileCount=1`、`extraSpreadAngle=12°`、`damageMultiplier=0.88`，不新增独立计算公式。
4. 新增 Support `Concentration`：仅兼容 MouseTargetedArea/SelfCenteredArea；`damageMultiplier=1.22`、`radiusMultiplier=0.78`、`cooldownMultiplier=1.12`，不影响 Projectile 或 Dash。
5. 新技能初始保持锁定；新 Support 初始保持锁定。`MapRewardLibrary` 必须从 `SkillLibrary::all()`/`SupportLibrary::all()` 自动过滤并生成奖励，不允许在 `GameWorld` 或 Renderer 写新名称白名单。
6. Skill Panel、MapComplete 奖励预览、装备详情预览应通过现有数据驱动路径自动显示新条目；不要新增 UI 特例。

强制实现约束：

- 优先只改 `SkillLibrary.hpp`、`SupportLibrary.hpp` 及必要的通用测试；若必须扩展 enum，只允许增加对应 `SupportKind` 并让 `SupportLibrary::supportsSkill()` 表达兼容性。
- 不新增技能槽、输入键、地图事件、Boss、敌人类型、装备槽、货币、商店、loot filter、自动拾取、跨运行存档或存档字段。
- 不修改已有技能/Support/天赋/装备/地图的数值；新数据的数值只能使用本任务固定值。
- 不在 `GameWorld` 按技能名称写 if/switch；实际伤害、半径、冷却、投射物和 Support 兼容性必须走现有 `CombatMath`/`SkillBar` 聚合。
- 不绕过解锁校验：数字键只能分配已解锁技能，F 键/Support 路径只能使用已解锁 Support；非法槽位、重复 Support、非法存档字段继续拒绝。
- 不修改 Renderer 的固定布局；若新名称导致文本超宽，只使用现有 `truncateText`/摘要 helper，不重做 UI。
- hy3 不提交代码、不修改本手册；工作区保持未提交，交付完整 diff、测试数量、clean build、CTest、直接测试、启动 smoke 和未修复风险。

必须验证：

- 纯逻辑：`SkillLibrary::all()` 包含 10 个技能；Arc Bolt/Shockwave 的槽位、CastType、数值准确；Barrage/Concentration 的兼容性只覆盖目标 CastType。
- 纯逻辑：Arc Bolt 受 Projectile 专精和 Projectile Support 影响，Area 专精不影响；Shockwave 受 Area 伤害/半径和 Area Support 影响，Projectile 专精不影响；Concentration 的伤害/半径/冷却聚合准确。
- 纯逻辑：新增技能/Support 初始未解锁，MapRewardLibrary 能在未解锁时生成对应奖励，解锁后不再重复生成；非法分配仍被拒绝。
- GameWorld：至少一条真实 `Arc Bolt` Projectile 施法和一条真实 `Shockwave` Area 施法，反馈伤害等于目标实际 HP 变化；至少一条 Support Link 组合通过 Skill Panel/SkillBar 数据路径生效。
- 存档：解锁新技能、新 Support 和装备 Link 后保存/加载，集合和 Link 顺序保留；损坏/非法字段不污染当前运行。
- MSVC `cmake --build build --clean-first`、CTest `3/3`、三套直接测试、`PlaneShooter.exe` 启动 3 秒 smoke 全部通过；测试数量必须高于 `958/11/254`。

完成定义：2 个主动技能和 2 个 Support 完全由现有数据/CombatMath/奖励/解锁/预览路径接入，纯逻辑、真实施法和存档边界有覆盖；主 review Agent 完成 diff review、必要修正、全量验证并提交后，才更新进度看板。

## 14. 项目进度看板

| 领域 | 状态 | 说明 |
|---|---|---|
| 主动战斗 | v1 完成 | 四槽技能、Support、异常、药瓶已形成基础构筑 |
| 构筑内容扩展 | 原型 | 8 个技能、7 个 Support、双 Link 和解锁奖励已可玩；下一步固定增加 2 技能 + 2 Support |
| 开放地图 | v1 完成 | 大地图、相机、预制布局、探索小地图、事件和 Boss 路线已完成 |
| 怪物生态 | v1 完成 | 近战、远程、精英、冲锋、ElitePack 均有，精英风险和事件进度已有数据化可读反馈 |
| Boss | v1 完成 | Brood 召唤、Brimstone 火区、Storm 锁定突进形成三种独立机制 |
| 天赋盘 | v1 完成 | 20 节点、四个 Keystone、前置和 HUD/hover 反馈已完成 |
| 状态异常 | 可玩 | Ignite/Chill、Enemy/Boss 抗性、Support 穿透和 Ignite tick 反馈已有，异常种类仍少 |
| 装备掉落 | v1 完成 | base/implicit/affix/tier/rarity/relic/tags/weights/地图主题偏置/比较/满包安全已有 |
| 地图选择 | v1 完成 | 三选图、风险收益、模板绑定、稳定布局变体和两词缀组合已有 |
| 经济/锻造 | v1 完成 | 分解、Forge Fragments、三种选择式词缀加工和当前 run Stash 已有 |
| 存档 | v1 完成 | 单文件版本化存档、RNG 恢复、坏档保护、MapComplete/安全出生点恢复已有 |
| 暂停/恢复 | v1 完成 | Pause 冻结模拟、Esc 上下文优先级、Save/Load/Restart/Quit 和 Input Help 已有 |
| 连续刷图验收 | v1 完成 | 五张真实 Boss -> 拾取/管理掉落 -> 选奖励/地图 -> E 推进，且死亡/暂停/中间存档边界已有自动保护 |
| 美术音频 | 原型 | 主要为 SFML 几何和文字 |
| 自动化测试 | 原型 | 纯逻辑 958 条、存档 11 条、GameWorld 254 条通过；已覆盖五张连续真实 Boss 流程、GameOver/Restart、MapComplete/Paused 存档、Projectile/Area 命中、Ignite tick、ElitePack、Boss 击杀和保底掉落，仍缺 Renderer/UI 像素级验收 |

维护本表时只使用“未开始 / 原型 / 可玩 / v1 完成 / 完成”五种状态。每个 milestone 完成后由主 review Agent 更新本文档和基线 commit。
