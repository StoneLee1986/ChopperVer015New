// CHOPPER - BeatDriveLevel 关卡B·鼓点驱动下落(020) 实现
#include "Game.h"
#include <algorithm>   // std::min

BeatDriveLevel::BeatDriveLevel(Game& g) : m_game(g), m_scenery(g.m_rng)
    , m_useAudioSync(config::AUDIO_SYNC_ENABLED), m_audioOffset(0.0f) {}

// 一次性初始化：合成鼓声 + 建提示框/固定文字/字母/判定/连击 UI
void BeatDriveLevel::initScene() {
    synthDrum();       // 程序合成 60BPM 规律鼓声
    m_audioOffset = 0.0f;   // 初始化音频偏移量为 0（首次加载或重新配置后）
    synthFailSound();  // 程序合成失败音效（柔和下行提示音）
    // 本关场景元素（完全自包含，不再由 Game 全局共用）：背景/树/小人
    m_scenery.init(m_game.m_texBackground, m_game.m_texCloud, m_game.m_texBee);
    m_player.init(m_game.m_texPlayer, m_game.m_texRip);
    m_tree.setTexture(m_game.m_texTree);
    m_tree.setOrigin(150.0f, 900.0f);   // 锚点贴图底部中心（静态完整树）
    m_tree.setPosition(config::TREE_ANCHOR_X, config::TREE_GROUND_Y);
    // 复现原版砍树动作（timber.cpp）：斧头瞬移换侧 + 单块圆木向反侧飞出
    m_axeSpr.setTexture(m_game.m_texAxe);
    // 按原版：无缩放/无旋转/锚点左上角(默认)，砍击时瞬移到对应侧
    m_axeSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);   // 斧头初始隐藏（砍击时才瞬移出现）
    m_logSpr.setTexture(m_game.m_texLog);
    m_logSpr.setOrigin(150.0f, 60.0f);               // 锚点居中（同关卡1 圆木）
    m_logSpr.setScale(config::RHYTHM_LOG_SCALE, config::RHYTHM_LOG_SCALE);   // 原版整块太大，缩小
    m_logSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);   // 初始藏到屏幕外

    // 按键提示框（右半边靠下，半透明底 + 亮色描边，风格对齐现有判定框）
    m_hintBox.setSize(Vector2f(config::HINT_BOX_W, config::HINT_BOX_H));
    m_hintBox.setPosition(config::HINT_BOX_X, config::HINT_BOX_Y);
    m_hintBox.setFillColor(Color(0, 0, 0, 120));          // 半透明黑底，不遮挡树枝/树
    m_hintBox.setOutlineThickness(3.0f);
    m_hintBox.setOutlineColor(Color(255, 215, 0, 220));   // 亮金描边

    // 固定提示行（双语，渲染时每帧重设以实时跟随语言切换）
    m_hintFixed.setFont(m_game.m_font);
    m_hintFixed.setCharacterSize(config::HINT_FONT_SIZE);
    m_hintFixed.setFillColor(Color::White);

    // A/B 大号字母
    m_letterText.setFont(m_game.m_font);
    m_letterText.setCharacterSize(config::HINT_LETTER_SIZE);
    m_letterText.setFillColor(Color::White);

    // 判定文字 / 连击文字（放屏幕中央偏上，避免与树身/树枝重叠）
    m_judgeText.setFont(m_game.m_font);
    m_judgeText.setCharacterSize(80);
    m_judgeText.setFillColor(Color::Yellow);
    centerText(m_judgeText, config::SCREEN_CENTER_X, 320.0f);
    m_comboText.setFont(m_game.m_font);
    m_comboText.setCharacterSize(50);
    m_comboText.setFillColor(Color::White);
    centerText(m_comboText, config::SCREEN_CENTER_X, 400.0f);

    // 初始化树枝数组（所有槽位初始为无枝，缩放 1.0）
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        m_branches[i] = { Side::None, 1.0f };
    }
}

// 每局开始：复位全部状态 + 从头播放鼓声
void BeatDriveLevel::onStart() {
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        m_branches[i] = { Side::None, 1.0f };   // 重置为无枝，缩放 1.0
    }
    m_player.reset();        // 小人回左边、墓碑藏起来（本关自控位置）
    m_playerSide   = Side::Left;                 // 小人回左边
    m_audioOffset = 0.0f;   // 新关卡重置音频偏移量（每局从头校准）
    m_letterActive = false;
    m_letterT      = 0.0f;
    m_countdown    = config::RHYTHM_START_COUNTDOWN;   // 开局倒计时开始
    m_score = 0; m_combo = 0; m_maxCombo = 0;
    m_timeRemaining = config::START_TIME;
    m_currentBeat = 0;
    m_beatClock   = 0.0f;   // 独立节拍时钟归零（与鼓声同步起跑）
    m_cleared = false; m_dead = false; m_ended = false;   // 结算锁复位
    m_logActive = false;   // 圆木复位（隐藏）
    m_logSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
    m_axeSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);   // 斧头开局隐藏（砍击时才出现）
    m_drum.stop();
    m_drum.play();   // 从头播放鼓声（getPlayingOffset 作 tick 基准）
}

// 玩家按键：更新站位 + 以鼓声时刻为基准判定（Perfect/Good/Miss）
void BeatDriveLevel::handleInput(Side side) {
    if (m_ended) return;   // 已结算：不再响应按键（防结算后按键重新激活圆木/斧头，造成"原木还在"）
    m_playerSide = side;   // 小人移动到该侧（站位决定撞死判定）
    m_player.onSide(side); // 小人精灵切到对应一侧（本关自控站位）
    // 斧头始终跟随玩家所在侧（原版：砍就换侧；即使时机失误也瞬移，不留在原处）
    m_axeSpr.setPosition(side == Side::Left ? config::RHYTHM_AXE_LEFT_X : config::RHYTHM_AXE_RIGHT_X,
                         config::RHYTHM_AXE_Y);

    // 玩家按对(移到反侧)躲开即将压到的那根树枝(index4)：它已用过，立即消失，避免停在玩家层干扰
    const int  pending  = config::RHYTHM_BRANCH_COUNT - 2;
    const Side incoming = m_branches[pending].side;
    if (incoming != Side::None && incoming != side) {
        m_branches[pending] = { Side::None, 1.0f };   // 躲开 → 该树枝立即清除（红框随之消失）
        // 特效：躲开瞬间的白色冲击环
        m_game.m_fx.ring(Vector2f(side == Side::Left ? config::PLAYER_LEFT_X : config::PLAYER_RIGHT_X,
                                  config::PLAYER_Y - 80.0f),
                         sf::Color(200, 230, 255, 190), 20.0f, 170.0f, 0.3f);
    }

    // 倒计时阶段（m_countdown >= 0）：玩家还没真正开始，只响应站位移动，不判定也不计时
    if (m_countdown >= 0) return;

    const float beat = 60.0f / config::RHYTHM_BPM;
    const float nearest = std::round(m_beatClock / beat) * beat;   // 以独立节拍时钟判拍（不依赖鼓声 offset）
    const float diff    = std::fabs(m_beatClock - nearest);
    if (diff <= config::NOTE_GOOD_WINDOW) {                 // 在判定窗口内按＝命中
        const bool perfect = diff <= config::NOTE_PERFECT_WINDOW;
        ++m_combo;
        if (m_combo > m_maxCombo) m_maxCombo = m_combo;
        m_score += perfect ? 100 : 50;
        m_timeRemaining = std::min(config::START_TIME, m_timeRemaining +
                                   (perfect ? config::HIT_TIME_PERFECT : config::HIT_TIME_GOOD));
        setJudge(perfect ? "完美" : "好", perfect ? "PERFECT" : "GOOD",
                 perfect ? Color::Yellow : Color::Green);
        m_game.m_sound.play();   // 命中打击音效（chop.wav）
        // 复现原版砍树（timber.cpp）：单块圆木向反侧飞出（仅在命中时）
        m_logSpr.setPosition(config::TREE_X, config::RHYTHM_LOG_Y);          // 圆木从树心出发
        m_logVx = (side == Side::Left ? 1.0f : -1.0f) * config::RHYTHM_LOG_VX;  // 砍左→向右飞、砍右→向左飞
        m_logVy = config::RHYTHM_LOG_VY;                                          // 原版向上初速
        m_logActive = true;
        // 特效：命中木屑 + 冲击环 + 轻震 + 定格
        m_game.m_fx.hit(Vector2f(side == Side::Left ? config::RHYTHM_AXE_LEFT_X
                                                    : config::RHYTHM_AXE_RIGHT_X,
                                 config::RHYTHM_AXE_Y), perfect);
    } else {                                               // 远离鼓声拍＝时机失误
        m_combo = 0;
        m_timeRemaining -= config::MISS_TIME_PENALTY;
        setJudge("失误", "MISS", Color::Red);
        m_game.m_fx.miss();                        // 特效：红闪 + 震屏
    }
}

// ============================================================================
// 每帧推进：节拍 tick + 树枝下落 + 字母动画 + 时间条 + 碰撞/结束判定
void BeatDriveLevel::update(float dt) {
    if (m_ended || m_dead || m_cleared) return;   // 已结算不再推进

    const float beat = 60.0f / config::RHYTHM_BPM;

    m_beatClock += dt;   // 独立节拍时钟：每帧由 dt 累计推进（此前缺失，导致树枝/字母/通关全部静止）

    // 跨拍检测：每过一拍触发一次"树枝下落 + 新枝生成 + 字母"
    // 【关键修复】：使用基于时间的直接计算，确保准确推进到下一拍
    while (m_currentBeat < config::RHYTHM_DRUM_BEATS &&
           m_beatClock >= static_cast<float>(m_currentBeat + 1) * beat) {
        ++m_currentBeat;
        m_game.m_fx.beatPulse();   // 特效：每拍画面脉冲（视觉节拍器，听不见也能跟拍）
        advanceBranches();
        checkCollision();      // 撞死会置 m_dead 并结算
        if (m_dead || m_cleared) return;   // 增加 m_cleared 检查
    }

    // 32 拍走完＝通关（currentBeat 由独立时钟推进，必定走满）
    if (m_currentBeat >= config::RHYTHM_DRUM_BEATS) {
        m_cleared = true;

        doGameOver(tr("通关！", "CLEAR!"), false);   // 通关不播失败音
        return;
    }

    // 字母动画推进（一拍内：淡入 → 脉冲回弹 → 淡出）
    if (m_letterActive) {
        m_letterT += dt / beat;
        if (m_letterT >= 1.0f) m_letterActive = false;
    }

    // 砍树动作推进：圆木按水平+垂直初速飞出（原版：水平∓5000 + 向上-1500，无重力）
    if (m_logActive) {
        m_logSpr.move(m_logVx * dt, m_logVy * dt);
        if (m_logSpr.getPosition().x < -100.0f || m_logSpr.getPosition().x > 2000.0f)
            m_logActive = false;   // 飞出左右屏幕边界即停（原版 LOG_OFFSCREEN）
    }

    // 背景装饰推进：云 + 蜜蜂漂移（本关自持场景元素）
    m_scenery.update(sf::seconds(dt));

    // 时间条：倒计时阶段（m_countdown >= 0）玩家还没开始砍，不衰减也不判负
    if (m_countdown < 0) {
        m_timeRemaining -= config::TIME_DECAY_PER_SEC * dt;
        if (m_timeRemaining <= 0.0f) {
            m_timeRemaining = 0.0f;
            doGameOver(tr("时间到！！", "Out of time!!"), true);
            return;
        }
    }

    // 连击文字（>1 才显示）
    if (m_combo > 1) {
        m_comboText.setString("x" + std::to_string(m_combo));
        m_comboText.setFillColor(Color::White);
        centerText(m_comboText, config::SCREEN_CENTER_X, 400.0f);
    } else {
        m_comboText.setString("");
    }
}

// 一拍：全部槽位下移一格 + 顶部按概率生成新枝 + 决定本拍字母
void BeatDriveLevel::advanceBranches() {
    if (m_countdown >= 0) --m_countdown;   // 开局倒计时：每拍递减 4→3→2→1→0→-1（到 -1 结束，接 AB）
    for (int i = config::RHYTHM_BRANCH_COUNT - 1; i > 0; --i) {
        m_branches[i] = m_branches[i - 1];   // 下移一格（携带侧别与缩放）
        if (m_branches[i].side != Side::None)
            m_branches[i].scale *= config::RHYTHM_BRANCH_GROW_PER_SLOT;   // 每靠近玩家一槽 +2%
    }

    // 顶部按概率接口生成新枝（关卡 B 默认 左50/右50/无0；缩放从 1.0 起步，随下落逐格放大）
    const int r  = static_cast<int>(m_game.m_rng() % 1000);
    const int l  = static_cast<int>(m_pLeft  * 1000.0f);
    const int rp = static_cast<int>(m_pRight * 1000.0f);
    if      (r < l)              m_branches[0] = { Side::Left,  1.0f };
    else if (r < l + rp)         m_branches[0] = { Side::Right, 1.0f };
    else                         m_branches[0] = { Side::None,  1.0f };

    // 字母提示 = 即将压到玩家的树枝（index4，下一拍进入玩家层 index5）的反侧：
    // 该枝在左→提示按右(B)、在右→提示按左(A)、该槽无枝→不显示（进度清零）
    const int pending = config::RHYTHM_BRANCH_COUNT - 2;   // 即将进入玩家层的槽位（6槽→index4）
    if      (m_branches[pending].side == Side::Left)  { m_letterActive = true; m_letterSide = Side::Right; m_letterT = 0.0f; }
    else if (m_branches[pending].side == Side::Right) { m_letterActive = true; m_letterSide = Side::Left;  m_letterT = 0.0f; }
    else { m_letterActive = false; m_letterT = 0.0f; }   // 该槽无枝：绝无字母出现
}

// 碰撞：最底层(玩家层)树枝与玩家同侧 → 被压死（树枝撞到玩家，原版判定）
void BeatDriveLevel::checkCollision() {
    const Side bottom = m_branches[config::RHYTHM_BRANCH_COUNT - 1].side;
    if (bottom != Side::None && bottom == m_playerSide) {
        m_dead = true;
        m_player.die(m_playerSide);   // 压死视觉：小人移出屏幕、墓碑留在被压那一侧
        doGameOver(tr("你被压死了！", "SQUISHED!"), true);
    }
}

// 结束本局（死亡/时间到/通关）：停帧 + 提示(带得分) + 失败音效(可选) + 清理残留 + 上榜结算
void BeatDriveLevel::doGameOver(const sf::String& msg, bool playFailSound) {
    if (m_ended) return;              // 结算互斥：三种结局只执行最先触发的那一种
    m_ended = true;
    m_game.m_paused = true;
    // 结束画面消息带本局得分（通关/压死/时间到都显示，结算更明确）
    m_game.setMessage(msg + sf::String(" ") +
                      sf::String(std::to_string(m_score)) + sf::String(tr("分", "pts")));
    // 任何结束状态（压死/时间到/通关）都清理残留的斧头与圆木，避免停在画面干扰
    m_axeSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
    m_logSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
    m_logActive = false;
    if (playFailSound) {
        m_game.m_fx.death(Vector2f(m_playerSide == Side::Left ? config::PLAYER_LEFT_X
                                                              : config::PLAYER_RIGHT_X,
                                   config::PLAYER_Y));
        m_fail.play();
        // 失败（压死/时间到）：小人移出屏幕、墓碑留在该侧（通关保留小人画面）
        m_player.die(m_playerSide);
    }
    m_drum.stop();
    m_game.stopBgm();   // 结束时立刻停掉关卡 BGM，别让音乐继续盖在结算画面上
    // 所有结局（压死/时间到/通关）统一走结算：进入高分榜（是否上榜由 Game 主循环裁定）
    m_game.m_enterNamePending = true;
    m_game.m_gameOverClock.restart();
}

// 画本关全部场景（背景/云/蜜蜂 → 树 → 树枝 → 小人/墓碑 → 圆木 → 斧头）
void BeatDriveLevel::renderScene(sf::RenderWindow& w) {
    m_scenery.render(w);   // 背景 + 云 + 蜜蜂（最底层）
    w.draw(m_tree);        // 树（静态完整树，本关自持）

    // 下落中的树枝（长在树上，画在树前）
    const int pending = config::RHYTHM_BRANCH_COUNT - 2;   // 字母对应的树枝 = 即将压到玩家的槽位(index4)
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        if (m_branches[i].side == Side::None) continue;
        Sprite branch(m_game.m_texBranch);
        branch.setOrigin(config::BRANCH_ORIGIN_X, config::BRANCH_ORIGIN_Y);
        // 越靠近玩家一槽，树枝放大 +2%（scale 在 advanceBranches 下移时累积）
        const float scale = m_branches[i].scale;
        branch.setScale(scale, scale);
        // 加 RHYTHM_BRANCH_Y_OFFSET：让压死层(最底槽)贴近小人头顶，消除"树枝远离玩家"的视觉空隙
        const float y = i * config::RHYTHM_BRANCH_SPACING + config::RHYTHM_BRANCH_Y_OFFSET;
        if (m_branches[i].side == Side::Left) { branch.setPosition(config::NOTE_LEFT_X, y); branch.setRotation(180.0f); }
        else                                   { branch.setPosition(config::NOTE_RIGHT_X, y); branch.setRotation(0.0f); }
        w.draw(branch);

        // 校对红框：框住字母对应的那根树枝(index4)，粗红框，明暗节奏与字母完全同步（同相位闪烁）
        if (i == pending && m_letterActive) {
            const sf::Vector2u tsz = m_game.m_texBranch.getSize();
            const float margin = 12.0f;
            // 红框尺寸与树枝缩放同步放大（边距也随缩放等比例）
            const float scaledMargin = margin * scale;
            RectangleShape box(Vector2f(tsz.x * scale + 2.0f * scaledMargin,
                                        tsz.y * scale + 2.0f * scaledMargin));
            // 原点(中心)固定，缩放后红框半宽 = (BRANCH_ORIGIN + margin) × scale
            box.setPosition(branch.getPosition().x - (config::BRANCH_ORIGIN_X + margin) * scale,
                            branch.getPosition().y - (config::BRANCH_ORIGIN_Y + margin) * scale);
            box.setFillColor(Color::Transparent);
            box.setOutlineThickness(6.0f);   // 粗框，防不明显
            // 与字母相同的淡入/保持/淡出相位 → 与字母同频率闪烁
            float a = 255.0f;
            if      (m_letterT < 0.10f) a = 255.0f * (m_letterT / 0.10f);
            else if (m_letterT < 0.60f) a = 255.0f;
            else                        a = 255.0f * (1.0f - (m_letterT - 0.60f) / 0.40f);
            box.setOutlineColor(Color(255, 0, 0, static_cast<sf::Uint8>(a)));
            w.draw(box);
        }
    }

    if (m_logActive) w.draw(m_logSpr);   // 单块圆木水平飞出（原版砍树反馈）
    m_player.render(w);                  // 小人 + 墓碑
    w.draw(m_axeSpr);                    // 斧头（瞬移到玩家砍击侧）
}

// 画按键提示 UI（提示框 + 固定文字 + A/B 字母 + 判定/连击），画在分数/时间条附近
void BeatDriveLevel::renderUI(sf::RenderWindow& w) {
    w.draw(m_hintBox);

    // 固定提示行（双语，每帧重设以实时跟随语言切换）
    m_hintFixed.setString(tr("A 按左   B 按右", "A=LEFT   B=RIGHT"));
    centerText(m_hintFixed, config::HINT_BOX_X + config::HINT_BOX_W / 2.0f, config::HINT_BOX_Y + 45.0f);
    w.draw(m_hintFixed);

    // 开局倒计时提示：显示 4,3,2,1,0，随鼓点每拍递减；0 显示后直接接 AB（countdown>=0 时覆盖，-1 后进 AB）
    if (m_countdown >= 0) {
        m_letterText.setString(std::to_string(m_countdown));
        m_letterText.setFillColor(Color::Yellow);
        centerText(m_letterText, config::HINT_BOX_X + config::HINT_BOX_W / 2.0f, config::HINT_BOX_Y + 125.0f);
        w.draw(m_letterText);
    }
    // A/B 字母：鼓响瞬间放大回弹 + 淡入淡出（perfect 时机提示）
    else if (m_letterActive) {
        m_letterText.setString(m_letterSide == Side::Right ? "B" : "A");
        float scale = 1.0f;
        if (m_letterT < 0.15f)   // 0~0.15s：从最大快速回落到 1.0（鼓响瞬间脉冲）
            scale += config::HINT_LETTER_PULSE * (1.0f - m_letterT / 0.15f);
        m_letterText.setScale(scale, scale);
        float a = 255.0f;
        if      (m_letterT < 0.10f) a = 255.0f * (m_letterT / 0.10f);          // 淡入
        else if (m_letterT < 0.60f) a = 255.0f;                                 // 保持
        else                         a = 255.0f * (1.0f - (m_letterT - 0.60f) / 0.40f); // 淡出
        m_letterText.setFillColor(Color(255, 255, 255, static_cast<sf::Uint8>(a)));
        centerText(m_letterText, config::HINT_BOX_X + config::HINT_BOX_W / 2.0f, config::HINT_BOX_Y + 125.0f);
        w.draw(m_letterText);
    }

    if (m_judgeClock.getElapsedTime().asSeconds() < 1.0f) w.draw(m_judgeText);
    if (m_combo > 1)                                       w.draw(m_comboText);
}

// ============================================================================
//  私有辅助
// ============================================================================

// 程序合成 60BPM 规律鼓声（节拍器"咚"，每拍一个低频短脉冲；无需外部音频文件）
void BeatDriveLevel::synthDrum() {
    const float beat = 60.0f / config::RHYTHM_BPM;               // 一拍 = 1.0s
    const float total = config::RHYTHM_DRUM_BEATS * beat;        // 整段时长
    const int frames = static_cast<int>(total * config::SAMPLE_RATE);
    std::vector<Int16> samples(frames, 0);
    const int pulseLen = static_cast<int>(0.18f * config::SAMPLE_RATE);  // 每个"咚"约 0.18s
    for (int k = 0; k < config::RHYTHM_DRUM_BEATS; ++k) {
        const int start = static_cast<int>(k * beat * config::SAMPLE_RATE);
        double ph = 0.0;
        for (int j = 0; j < pulseLen && start + j < frames; ++j) {
            const double t   = j / static_cast<double>(config::SAMPLE_RATE);
            const double env = std::exp(-t * 24.0);
            const double f   = 75.0 + 155.0 * std::exp(-t * 45.0);   // 230Hz 快速滑到 75Hz：有冲击力的"咚"
            ph += 6.283185307179586 * f / config::SAMPLE_RATE;
            samples[start + j] = static_cast<Int16>(20000.0 * env * std::sin(ph));
        }
    }
    if (!m_drumBuffer.loadFromSamples(samples.data(), static_cast<sf::Uint64>(samples.size()),
                                      1, config::SAMPLE_RATE))
        std::cerr << "[Warning] 节奏鼓点合成失败\n";
    m_drum.setBuffer(m_drumBuffer);
    m_drum.setVolume(config::DRUM_VOLUME);   // 鼓声是判定基准，音量独立于 BGM
}

// 程序合成失败音效：柔和下行三连音（G4→E4→C4），替代全局难听的死亡音效（仅本关用）
void BeatDriveLevel::synthFailSound() {
    const float freqs[3] = { 392.0f, 330.0f, 262.0f };   // G4 → E4 → C4 下行提示
    const float noteLen = 0.12f;                          // 每个音 0.12s
    const int frames = static_cast<int>((3 * noteLen + 0.05f) * config::SAMPLE_RATE);
    std::vector<Int16> samples(frames, 0);
    for (int n = 0; n < 3; ++n) {
        const int start = static_cast<int>(n * noteLen * config::SAMPLE_RATE);
        const int len   = static_cast<int>(noteLen * config::SAMPLE_RATE);
        for (int j = 0; j < len && start + j < frames; ++j) {
            const double t     = j / static_cast<double>(config::SAMPLE_RATE);
            const double env   = 1.0 - static_cast<double>(j) / len;   // 线性衰减
            const double phase = 6.283185307 * freqs[n] * t;
            samples[start + j] = static_cast<Int16>(5000.0 * env * std::sin(phase));
        }
    }
    if (!m_failBuffer.loadFromSamples(samples.data(), static_cast<sf::Uint64>(samples.size()),
                                      1, config::SAMPLE_RATE))
        std::cerr << "[Warning] 失败音效合成失败\n";
    m_fail.setBuffer(m_failBuffer);
}

// 把文字中心放到 (x,y)
void BeatDriveLevel::centerText(Text& t, float x, float y) {
    const FloatRect rect = t.getLocalBounds();
    t.setOrigin(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    t.setPosition(x, y);
}

// 刷新判定文字
void BeatDriveLevel::setJudge(const char* zh, const char* en, Color c) {
    m_judgeText.setString(tr(zh, en));
    m_judgeText.setFillColor(c);
    centerText(m_judgeText, config::SCREEN_CENTER_X, 320.0f);
    m_judgeClock.restart();
}
