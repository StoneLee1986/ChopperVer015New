# 📋 CHOPPER B 关卡树枝缩放改造 - 使用说明

## ✅ 已完成修改的文件

### 1. config.h
**位置**: `E:/BaiduSyncdisk/C++ AND C# Python Pleasure/c++/VisualStudio/ChopperVer015New/config.h`  
**修改内容**: 
```cpp
// 在文件末尾添加：
const float BRANCH_SCALE_INCREMENT = 1.05f;   // 每格缩放增量 (5%)
```

### 2. BeatDriveLevel.h  
**位置**: `E:/BaiduSyncdisk/C++ AND C# Python Pleasure/c++/VisualStudio/ChopperVer015New/BeatDriveLevel.h`

**修改内容**:
```cpp
// 在头文件区（ILevel.h 之后）添加结构体：
struct BranchInfo {
    Side side;          // Left / Right / None
    float scale;        // 当前缩放倍数 (初始 1.0, 每格 +5%)
};

// 将成员变量从：
Side m_branches[config::RHYTHM_BRANCH_COUNT] = {};
// 改为：
BranchInfo m_branches[config::RHYTHM_BRANCH_COUNT];
```

---

## ⚠️ 需要手动完成的修改（BeatDriveLevel.cpp）

### 📁 文件位置
`E:/BaiduSyncdisk/C++ AND C# Python Pleasure/c++/VisualStudio/ChopperVer015New/BeatDriveLevel.cpp`

### 🔧 需要修改的 4 个位置

#### **(a) initScene() - 第 47 行后添加**
```cpp
    // 初始化树枝数组（所有槽位初始为无枝，缩放 1.0）
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        m_branches[i] = { Side::None, 1.0f };
    }
}
```

#### **(b) advanceBranches() - 第 185-207 行**
**原代码**:
```cpp
void BeatDriveLevel::advanceBranches() {
    if (m_countdown >= 0) --m_countdown;
    for (int i = config::RHYTHM_BRANCH_COUNT - 1; i > 0; --i)
        m_branches[i] = m_branches[i - 1];   // ❌ 需要修改

    // 顶部按概率接口生成新枝...
    if      (r < l)              m_branches[0] = Side::Left;
    else if (r < l + rp)         m_branches[0] = Side::Right;
    else                         m_branches[0] = Side::None;
```

**修改后**:
```cpp
void BeatDriveLevel::advanceBranches() {
    if (m_countdown >= 0) --m_countdown;
    const float scaleIncrease = config::BRANCH_SCALE_INCREMENT;
    
    for (int i = config::RHYTHM_BRANCH_COUNT - 1; i > 0; --i) {
        // 下移：先保存当前格信息
        BranchInfo current = m_branches[i];
        
        // 从上一格读取
        if (m_branches[i - 1].side != Side::None) {
            // 下移到当前格
            current.side = m_branches[i - 1].side;
            
            // 计算缩放：初始 1.0，每格 +5%
            const int stepsFromTop = i;
            current.scale = std::pow(scaleIncrease, stepsFromTop);
        } else {
            current.side = Side::None;
            current.scale = 1.0f;
        }
        
        m_branches[i] = current;
    }
    
    // 顶部按概率接口生成新枝（缩放固定为 1.0）
    const int r  = static_cast<int>(m_game.m_rng() % 1000);
    const int l  = static_cast<int>(m_pLeft  * 1000.0f);
    const int rp = static_cast<int>(m_pRight * 1000.0f);
    if (r < l)              m_branches[0] = { Side::Left, 1.0f };
    else if (r < l + rp)   m_branches[0] = { Side::Right, 1.0f };
    else                    m_branches[0] = { Side::None, 1.0f };
```

**字母提示部分保持不变**（只是访问方式从 `m_branches[pending]` 改为 `m_branches[pending].side`）

#### **(c) renderScene() - 第 240-280 行**

**原代码**:
```cpp
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        if (m_branches[i] == Side::None) continue;
        Sprite branch(m_game.m_texBranch);
        branch.setOrigin(config::BRANCH_ORIGIN_X, config::BRANCH_ORIGIN_Y);
        const float y = i * config::RHYTHM_BRANCH_SPACING + config::RHYTHM_BRANCH_Y_OFFSET;
        if (m_branches[i] == Side::Left) { 
            branch.setPosition(config::NOTE_LEFT_X, y); 
            branch.setRotation(180.0f); 
        } else                              { 
            branch.setPosition(config::NOTE_RIGHT_X, y); 
            branch.setRotation(0.0f); 
        }
        w.draw(branch);

        // 红框...
        RectangleShape box(Vector2f(tsz.x + 2.0f * margin, tsz.y + 2.0f * margin));
```

**修改后**:
```cpp
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        if (m_branches[i].side == Side::None) continue;      // ✅ 使用.side
        
        // 设置树枝缩放（核心改动：每格 +5%）
        const float scale = m_branches[i].scale;              // ✅ 使用.scale
        Sprite branch(m_game.m_texBranch);
        branch.setScale(scale, scale);                        // ✅ 应用缩放
        branch.setOrigin(config::BRANCH_ORIGIN_X, config::BRANCH_ORIGIN_Y);
        
        const float y = i * config::RHYTHM_BRANCH_SPACING + config::RHYTHM_BRANCH_Y_OFFSET;
        if (m_branches[i].side == Side::Left) {               // ✅ 使用.side
            branch.setPosition(config::NOTE_LEFT_X, y); 
            branch.setRotation(180.0f); 
        } else                              {                  // ✅ 使用.side
            branch.setPosition(config::NOTE_RIGHT_X, y); 
            branch.setRotation(0.0f); 
        }
        w.draw(branch);

        // 校对红框...
        const float scaledMargin = margin * scale;
        RectangleShape box(Vector2f(
            tsz.x * scale + 2.0f * scaledMargin, 
            tsz.y * scale + 2.0f * scaledMargin
        ));  // ✅ 红框大小与树枝缩放同步
```

#### **(d) handleInput() - 第 85-95 行**

**原代码**:
```cpp
    const int  pending  = config::RHYTHM_BRANCH_COUNT - 2;
    const Side incoming = m_branches[pending];   // ❌ Side 类型无法存储 BranchInfo
```

**修改后**:
```cpp
    const int  pending  = config::RHYTHM_BRANCH_COUNT - 2;
    const Side incoming = m_branches[pending].side;   // ✅ 访问.side 成员
```

#### **(e) checkCollision() - 第 210-216 行**

**原代码**:
```cpp
void BeatDriveLevel::checkCollision() {
    const Side bottom = m_branches[config::RHYTHM_BRANCH_COUNT - 1];   // ❌
    if (bottom != Side::None && bottom == m_playerSide) {
```

**修改后**:
```cpp
void BeatDriveLevel::checkCollision() {
    const Side bottom = m_branches[config::RHYTHM_BRANCH_COUNT - 1].side;   // ✅
    if (bottom != Side::None && bottom == m_playerSide) {
```

---

## 🎯 核心实现逻辑

### 树枝缩放公式
- **槽位 0**（顶部）：`scale = pow(1.05, 0) = 1.0`（初始大小）
- **槽位 1**：`scale = pow(1.05, 1) = 1.05`（+5%）
- **槽位 2**：`scale = pow(1.05, 2) = 1.1025`（+10.25%）
- **槽位 3**：`scale = pow(1.05, 3) = 1.1576`（+15.76%）
- **槽位 4**：`scale = pow(1.05, 4) = 1.2155`（+21.55%）
- **槽位 5**（玩家层）：`scale = pow(1.05, 5) = 1.2763`（+27.63%）

### 红框同步调整
红框大小与树枝缩放同步：
```cpp
const float scaledMargin = margin * scale;
RectangleShape box(Vector2f(
    tsz.x * scale + 2.0f * scaledMargin, 
    tsz.y * scale + 2.0f * scaledMargin
));
```

---

## 📋 修改总结表

| 文件 | 修改位置 | 改动行数 | 说明 |
|------|----------|----------|------|
| config.h | 文件末尾 | +1 | 添加 `BRANCH_SCALE_INCREMENT = 1.05f` |
| BeatDriveLevel.h | 头文件区 | +8 | 声明 `BranchInfo` 结构体和修改成员类型 |
| BeatDriveLevel.cpp | initScene() | +4 | 初始化树枝数组 |
| BeatDriveLevel.cpp | advanceBranches() | +30 | 下移时计算缩放倍数 |
| BeatDriveLevel.cpp | renderScene() | +15 | 应用缩放并调整红框 |
| BeatDriveLevel.cpp | handleInput() | +1 | 使用 `.side` 成员访问 |
| BeatDriveLevel.cpp | checkCollision() | +1 | 使用 `.side` 成员访问 |

---

## ⚠️ 编译测试步骤

1. **打开 Visual Studio 项目**: `ChopperVer015New.slnx`
2. **确认所有修改已应用**（对比原始文件）
3. **重新编译解决方案**
4. **运行游戏观察效果**:
   - 树枝下落一格后应该逐渐变大
   - 红框大小与树枝同步变化
   - 字母提示、判定等功能正常

---

## 🔍 验证清单

- [ ] config.h 中包含 `const float BRANCH_SCALE_INCREMENT = 1.05f;`
- [ ] BeatDriveLevel.h 中定义了 `BranchInfo` 结构体
- [ ] BeatDriveLevel.h 中 `m_branches` 类型为 `BranchInfo[]`
- [ ] initScene() 初始化为 `{ Side::None, 1.0f }`
- [ ] advanceBranches() 下移时计算缩放倍数
- [ ] renderScene() 应用缩放并调整红框大小
- [ ] handleInput() 使用 `.side` 访问成员
- [ ] checkCollision() 使用 `.side` 访问成员

---

## 📌 注意事项

1. **不要改动原始文件**：所有修改应在备份后进行
2. **编译前检查**：确保所有头文件包含正确（需要 `<cmath>` 用于 `std::pow`）
3. **测试建议**：先用小缩放比例（如 1.03）测试效果，确认无误后再改为 1.05

---

**改造完成！树枝下落时每次 +5%，直到最靠近玩家时达到初始大小的 27.63%。** 🌿