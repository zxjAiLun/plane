# Mini ARPG 项目接手与开发手册

更新日期：2026-07-13

玩法代码基线：`bbd6dd7 Add item base types and implicit stats`

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
```

### 3.3 测试

```powershell
cmd /c "`"C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\Tools\VsDevCmd.bat`" -arch=amd64 >nul 2>&1 && ctest --test-dir build --output-on-failure"
```

当前纯逻辑测试基线：`535 passed / 0 failed`。

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
- `BossDefinition.hpp`
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

| 输入 | Playing | Passive Tree | Skill Panel | MapComplete |
|---|---|---|---|---|
| `WASD` / 方向键 | 移动 | 仍可移动 | 仍可移动 | 不处理战斗移动 |
| 左键按住 | Primary 技能 | 点击天赋节点，不射击 | 不射击 | 不射击 |
| 右键 | Secondary 技能 | 不施法 | 不施法 | 不施法 |
| `Q` | Utility 技能 | 不施法 | 不施法 | 不施法 |
| `Space` | Movement / Dash | 不施法 | 不施法 | 不施法 |
| `G` | 使用生命药瓶 | 不处理 | 不处理 | 不处理 |
| `F` | 优先交互地图事件，否则拾取最近物品 | 无 | 无 | 拾取 Boss 战利品 |
| `1-9` | 装备对应背包物品 | `1-0` 点节点 1-10 | `1-8` 分配技能 | 按阶段选择奖励或下一图 |
| `F1-F10` | 无普通战斗语义 | 点节点 11-20 | `F1-F4` 切换四个槽位 Support | 无 |
| `P` | 打开天赋盘 | 关闭天赋盘 | 打开时会关闭 Skill Panel | 禁止打开 |
| `K` | 打开技能面板 | 打开时会关闭天赋盘 | 关闭技能面板 | 禁止打开 |
| `Tab` | 循环选择背包物品 | 不处理 | 不处理 | 循环选择背包物品 |
| `Delete` | 丢弃选中物品 | 不处理 | 不处理 | 丢弃选中物品 |
| `C` | 分解选中物品 | 不处理 | 不处理 | 分解选中物品 |
| `V` | 使用碎片强化选中物品 | 不处理 | 不处理 | 强化选中物品 |
| `E` | 无 | 无 | 无 | 奖励和地图均选完后进入下一图 |
| `R` | 无 | 无 | 无 | 重新开始 run |
| `Esc` | 退出 | 退出 | 退出 | 退出 |

重要约束：

- Playing 中 `F` 的事件交互优先级高于拾取。
- 拾取目标必须使用 `GameWorld::focusedDroppedItemIndex()`，显示目标和实际拾取目标必须一致。
- MapComplete 中数字键绝不能装备物品。
- MapComplete 中允许 `F` 拾取、`Tab` 选择、`Delete` 丢弃、`C` 分解、`V` 强化。
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
- `V` 消耗碎片强化物品，最高 `+3`。

### 5.11 地图选择与奖励

- 第一张图使用默认普通地图。
- Boss 击杀后先三选一角色奖励，再三选一下一张地图。
- 奖励优先解锁技能；技能全部解锁后优先解锁 Support；之后使用全局伤害、HP、Future Item Quantity fallback。
- 下一图选项包含地图模板、名称、描述、推荐等级、怪物 HP/伤害、掉落倍率、Boss 风险、Elite 压力和 item level bonus。
- 当前 HUD 显示地图名和 modifier 摘要。
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

尚无自动化覆盖：

- SFML Renderer 布局。
- Input event 到 GameWorld 的完整集成。
- 从出生到 Boss 再到下一图的端到端流程。
- 地面拾取、满包、分解、强化的完整组合流程。
- 保存/读取，因为当前尚无存档。

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
| `MapModifier.hpp` | 下一图选项和风险收益倍率 | 数值必须能在 GameWorld 中找到实际应用点 |
| `SkillLibrary.hpp` | 主动技能定义 | 不直接实现命中循环 |
| `SupportLibrary.hpp` | Support 数据和兼容性 | 新 Support 必须有可观察行为和测试 |
| `SkillBar.hpp` | 四槽技能/Support 分配、冷却 | 不负责解锁奖励 |
| `CombatMath.hpp` | 无状态、纯战斗计算 | 供 GameWorld、Renderer 预览和测试复用 |
| `Ailment.hpp` | 状态异常数据类型 | 不放 Enemy 生命周期状态 |
| `ItemBase.hpp` | 普通装备 Base、Boss relic Base 和 implicit 数据 | Base 选择必须由 LootGenerator 驱动，不让 Renderer 参与 |
| `LootGenerator.hpp` | rarity、affix、tier、Boss relic 生成 | UI 文本不得参与数值计算 |
| `Equipment.hpp` | 装备 Item，并返回被替换 Item | 不允许静默吞掉旧装备 |
| `Inventory.hpp` | 有容量的 Item 容器 | 满包 add 返回 false，不产生副作用 |
| `PassiveTree.hpp` | 20 节点数据、前置、命中查询、属性聚合 | SP 扣除仍由 Player 管理 |
| `MapRewardLibrary.hpp` | 技能/Support/fallback 奖励生成 | GameWorld 只过滤、抽取、应用 |
| `Renderer.*` | 只读 GameWorld 并绘制世界和 UI | 禁止在 Renderer 中修改游戏状态或复制玩法公式 |
| `tests/arpg_logic_tests.cpp` | 无 SFML 的纯逻辑回归测试 | 新数据化规则必须补断言 |

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

### 7.4 世界与渲染

- Player、Enemy、Projectile、DroppedItem 均使用世界坐标。
- HUD、菜单、天赋盘、技能面板使用屏幕坐标。
- 世界移动必须通过 MapInstance 的边界/障碍解析。
- Renderer 只读，不驱动行为。

### 7.5 进度状态

- `reset()` 开始新 run，清空装备、天赋、解锁和 Future Item Quantity。
- `startNextMap()` 保留本 run 的装备、天赋、技能/Support 解锁和永久奖励。
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

当前已满足 1、2 的基础版，3、4、5、6 已形成可玩雏形但仍需深度和稳定性，7 尚未实现，8、9 部分完成。天赋 Keystone、异常抗性和装备 Base/implicit 已让构筑出现第一层真实取舍；装备词缀还缺少 tags/weights 和冲突选择规则。

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

任务 C2：词缀标签与权重 v1

- AffixDefinition 增加 tags 和 weight。
- 同一物品避免明显冲突/重复词缀。
- 地图模板或 Boss theme 可以提高特定 tag 权重。
- RNG 选择必须可测试。

任务 C3：锻造选择 v2

- 当前 `V` 只有线性强化。扩展为一次明确选择：强化现有词缀、重铸一条词缀、提升 affix tier 三者之一。
- 仍使用 Forge Fragments，不引入完整货币经济。
- 所有失败路径保留原物品。

任务 C4：Stash v1

- 地图间提供有限 stash，至少 24 格。
- MapComplete 可在 Inventory 和 Stash 间移动物品。
- startNextMap 保留 stash，reset 清空或由存档策略决定。
- 不做拖拽，先用键盘/选中操作完成。

Milestone C 验收：玩家会因为 base、implicit、affix、tier 和构筑方向做真实取舍，而不是只看总伤害。

### Milestone D：地图深度

任务 D1：地图布局变体 v2

- 每个 MapTemplate 至少三套障碍/事件布局 seed。
- 保证 Start 到 BossGate 可达。
- 先使用预制布局组合，不手写复杂寻路生成器。
- 加可达性纯逻辑测试。

任务 D2：Minimap Reveal v1

- 小地图只显示玩家探索过的区域。
- Boss Gate 方向可以保留目标指示，但不直接暴露全部事件。
- MapComplete 时可显示全图。

任务 D3：Map Modifier 扩展 v2

- 增加可组合 modifier，而不是一个模板只带一个固定词缀。
- 至少覆盖怪物速度、Projectile 数量、Elite modifier 权重、事件奖励和异常抗性。
- 每个 modifier 同时声明风险与收益。
- HUD 对过长词缀做多行或摘要，不允许溢出。

Milestone D 验收：连续三张地图在路线、遭遇、风险和奖励上有明显变化。

### Milestone E：可持续运行与产品化

任务 E1：Deterministic RNG Service

- 替换散落的 `std::rand()`。
- GameWorld 持有 run seed，地图、掉落、奖励使用明确 RNG 流。
- 同 seed 的纯逻辑生成结果可复现。
- 不在 Renderer 使用 RNG。

任务 E2：Local Save v1

- 保存 Player level/EXP/SP、天赋节点、装备、Inventory、Stash、技能/Support 解锁、当前地图等级和永久 run 奖励。
- 使用版本化存档格式。
- 加载失败时安全回到新 run，不覆盖损坏文件。
- 不保存地图中瞬时 Enemy/Projectile 状态，加载后从地图起点恢复。

任务 E3：Pause / Settings / Input Help

- Esc 改为 Pause 菜单，菜单中提供 Continue / Restart Run / Quit。
- 音量和窗口设置先保留结构，即使暂无音频资源。
- 控制说明必须来自 Input binding 数据或单一表，避免手写多处漂移。

任务 E4：Visual/Audio Pass

- 用可合法分发的 bitmap/sprite、音效替换主要占位几何。
- 保持碰撞体与视觉尺寸一致。
- 不在玩法逻辑稳定前投入复杂动画系统。

Milestone E 验收：玩家可以关闭程序后继续 run，能稳定完成至少 5 张连续地图，重开和异常退出不会破坏存档。

## 13. 推荐的下一项任务

建议立即交给 hy3：`Affix Tags and Weights v1`。

原因：Base、implicit、rarity、tier 和构筑词缀已经存在，但当前词缀仍是无权重随机抽取，可能在同一件物品上重复堆叠冲突方向。下一轮应让词缀选择可控、可测试并能受地图/Boss 主题影响，再进入有选择的 crafting。

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
- 不实现词缀 tags/weights、重铸、选择式 crafting；这些分别留给 C2/C3。

### 13.5 hy3 实施任务：Affix Tags and Weights v1

目标：让词缀选择从“同槽位随机抽取”变成可解释、可测试、能受到地图/Boss 主题影响的选择。必须保留现有 Base、implicit、rarity、tier、Boss relic 和装备所有权，不新增 UI 大系统。

开始前必须阅读：

- `include/LootGenerator.hpp`：当前 AffixDefinition、词缀池、tier 计算和随机抽取。
- `include/Item.hpp`、`include/ItemBase.hpp`、`include/Stats.hpp`：Item 的 Base/implicit/affix contribution 结构。
- `include/BossDefinition.hpp`、`include/MapModifier.hpp`、`src/GameWorld.cpp`：Boss loot theme、当前地图 modifier 和掉落调用点。
- `src/Renderer.cpp`：物品详情、词缀显示和比较面板；不得让 Renderer 参与词缀选择。
- `tests/arpg_logic_tests.cpp`：LootGenerator、Boss relic 和随机测试风格。

必须实现：

1. 新增明确的 `enum class AffixTag`，至少覆盖 `Damage`、`AttackSpeed`、`MoveSpeed`、`Pickup`、`Survival`、`Projectile`、`Area`、`Armor`；`AffixDefinition` 增加 tags 和正数 weight。字段追加到现有 aggregate initializer 末尾，或一次性显式修正所有数据，禁止字段错位。
2. 给当前全部词缀配置至少一个 tag 和明确 weight；每条词缀的 tag 必须与其 `AffixStat` 一致，例如 ProjectileDamage 不能只有 Damage 而没有 Projectile。不要使用词缀显示文本作为逻辑条件。
3. 重写同一件物品的候选选择：只从同槽位、未使用的词缀中按 weight 抽取；至少避免同一 `AffixStat` 重复，避免同一 tag 的明显冲突组合。若候选耗尽必须有稳定 fallback，不能死循环、越界或生成空词缀。
4. 增加可选的 `LootBias`/`AffixTag` 生成上下文。普通地图默认无偏置；地图或 Boss theme 可以提高对应 tag 的权重。推荐让 `GameWorld` 传递当前地图/Boss 主题到生成器，不要在 LootGenerator 读取 GameWorld 全局状态。
5. Boss relic 的固定主题 Base 和固定主题词缀继续保留；本轮只让 Boss 额外普通掉落或通用生成路径使用主题 bias，不得把 relic 变成完全随机物品。
6. 权重计算必须集中在 `LootGenerator` 的纯 helper；基础 weight、主题 bonus 和候选过滤不能复制到 Renderer。暂不替换全项目 `std::rand()`，E1 才做统一 RNG service。
7. 详情面板可以显示词缀 tag 摘要，但不能改变当前紧凑布局；至少保证玩家能从词缀名/现有属性看到结果。不要做 loot filter、词缀搜索或 crafting UI。
8. 保持 `ItemAffix::stats` contribution 和 Base/implicit 聚合不变；生成出的 Item 仍必须能由 `implicitStats + affix.stats` 重建。所有权、升级、分解和满包拾取路径不改。

测试必须覆盖：

- 每条 AffixDefinition 的 tags 非空、weight > 0，tag 与 AffixStat 映射正确。
- 同一物品不会重复同一 AffixStat；候选不足时生成数量稳定且不崩溃。
- 使用固定随机种子验证权重选择可复现；用纯 helper 验证主题 bias 提高目标 tag 的选择权重。
- Weapon/Armor/Ring/Amulet 的生成均保留合法 Base、implicit 和 affix contribution。
- Boss relic 的 Base、主题属性和固定掉落逻辑继续通过；现有 rarity/tier/Item Base 回归测试继续通过。
- 最终执行 clean build、CTest、直接测试和 3 秒启动 smoke test。

明确不做：

- 不做 C3 的重铸/强化词缀选择，不做 C4 Stash，不做完整经济系统。
- 不新增装备槽、技能、Support、地图、Boss 或天赋节点。
- 不在本轮替换 `std::rand()`，不引入第三方 RNG 库。
- 不提交代码；保持工作区未提交，交给主 review Agent 验收、修正和 commit。

交付报告必须列出：改动文件、AffixTag/weight 数据表、候选过滤与 fallback、主题 bias 来源、Boss relic 处理、测试数量、clean build、启动 smoke test、已知风险和 `git status --short`。

## 14. 项目进度看板

| 领域 | 状态 | 说明 |
|---|---|---|
| 主动战斗 | v1 完成 | 四槽技能、Support、异常、药瓶已形成基础构筑 |
| 开放地图 | v1 完成 | 大地图、相机、障碍、事件和 Boss 路线已完成 |
| 怪物生态 | 可玩 | 近战、远程、精英、冲锋均有，pack 协同仍弱 |
| Boss | v1 完成 | Brood 召唤、Brimstone 火区、Storm 锁定突进形成三种独立机制 |
| 天赋盘 | v1 完成 | 20 节点、四个 Keystone、前置和 HUD/hover 反馈已完成 |
| 状态异常 | 可玩 | Ignite/Chill、Enemy/Boss 抗性和 Support 穿透已有，异常种类仍少 |
| 装备掉落 | v1 完成 | base/implicit/affix/tier/rarity/relic/比较/满包安全已有，缺 tags/weights |
| 地图选择 | v1 完成 | 三选图和风险收益已有，缺组合 modifier 和布局变体 |
| 经济/锻造 | 原型 | 分解碎片和 +3 强化已有，缺有选择的 crafting |
| 存档 | 未开始 | 完成定义中的最大缺口 |
| 美术音频 | 原型 | 主要为 SFML 几何和文字 |
| 自动化测试 | 原型 | 纯逻辑 535 条通过，缺 UI 和端到端测试 |

维护本表时只使用“未开始 / 原型 / 可玩 / v1 完成 / 完成”五种状态。每个 milestone 完成后由主 review Agent 更新本文档和基线 commit。
