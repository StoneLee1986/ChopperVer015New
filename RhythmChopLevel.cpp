// CHOPPER - RhythmChopLevel 关卡3·节奏鼓点 实现
#include "Game.h"
#include <algorithm>   // std::min

RhythmChopLevel::RhythmChopLevel(Game& g) : m_game(g), m_scenery(g.m_rng) {}

// 一次性初始化：合成鼓声 + 建提示框/固定文字/字母/判定/连击 UI
void RhythmChopLevel::initScene() {
    synthDrum();       // 程序合成 60BPM 规律鼓声
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

    // 常驻大号 X（节拍引导，不再分左右/淡入淡出）
    m_letterText.setFont(m_game.m_font);
    m_letterText.setString("X");
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
}

// 每局开始：复位全部状态 + 从头播放鼓声
void RhythmChopLevel::onStart() {
    // 加载谱面（文件不存在自动回退概率模式）
    m_beatmap = Beatmap::load("beatmaps/rhythm_chop.json");
    m_useBeatmap = !m_beatmap.empty();
    for (Side& b : m_branches) b = Side::None;   // 6 槽树枝清空
    m_player.reset();        // 小人回左边、墓碑藏起来（本关自控位置）
    m_playerSide   = Side::Left;                 // 小人回左边
    m_xPulse = 0.0f;                             // X 脉冲复位（鼓声/perfect 时才触发放大）
    m_letterText.setFillColor(Color::White);     // X 开局为白
    m_score = 0; m_combo = 0; m_maxCombo = 0;
    m_timeRemaining = config::START_TIME;
    m_currentBeat = 0;
    m_beatClock   = 0.0f;   // 独立节拍时钟归零（与鼓声同步起跑）
    m_cleared = false; m_dead = false;
    m_logActive = false;   // 圆木复位（隐藏）
    m_logSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
    m_axeSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);   // 斧头开局隐藏（砍击时才出现）
    m_drum.stop();
    m_drum.play();   // 从头播放鼓声（getPlayingOffset 作 tick 基准）
}

// 玩家按键 = 砍一次树：回归原版逻辑 —— 树枝下移一格+生成新枝，最底槽在该侧则被压死；
// 砍树反馈(音效+圆木)每次砍都触发；节拍判定只驱动 X 放大(perfect)与时间条/得分(计分后做)。
void RhythmChopLevel::handleInput(Side side) {
    m_playerSide = side;   // 小人移动到该侧（站位决定撞死判定）
    m_player.onSide(side); // 小人精灵切到对应一侧（本关自控站位）
    // 斧头始终跟随玩家所在侧（原版：砍就换侧；即使时机失误也瞬移，不留在原处）
    m_axeSpr.setPosition(side == Side::Left ? config::RHYTHM_AXE_LEFT_X : config::RHYTHM_AXE_RIGHT_X,
                         config::RHYTHM_AXE_Y);

    // 原版砍树：每次砍树，树枝整体下落一格 + 顶部生成新枝
    // 先记住"即将压到玩家"的枝(index4)：砍后它落到玩家层(index5)
    const Side incoming = m_branches[config::RHYTHM_BRANCH_COUNT - 2];
    advanceBranches();
    // 若玩家砍的是反侧(躲开那根)，它已用过，立即清除，避免停在玩家层干扰(同 B 关卡的"躲开即消失")
    if (incoming != Side::None && incoming != side)
        m_branches[config::RHYTHM_BRANCH_COUNT - 1] = Side::None;
    // 判死：最底槽树枝在玩家所砍那一侧 → 被压死（先下落再判，原版顺序）
    checkCollision();
    if (m_dead) return;   // 已死：不再播音效/圆木/判定

    // 砍树反馈（原版每次砍都触发）：打击音效 + 单块圆木向反侧飞出
    m_game.m_sound.play();
    m_logSpr.setPosition(config::TREE_X, config::RHYTHM_LOG_Y);
    m_logVx = (side == Side::Left ? 1.0f : -1.0f) * config::RHYTHM_LOG_VX;
    m_logVy = config::RHYTHM_LOG_VY;
    m_logActive = true;

    // 节拍判定（计分后做；现只驱动 X 放大 + 时间条/得分/连击）
    const float now  = m_beatClock;   // 以独立节拍时钟判拍（不依赖鼓声 offset，其后段失效）
    const float beat = 60.0f / config::RHYTHM_BPM;
    const float nearest = std::round(now / beat) * beat;   // 最近的鼓声拍
    const float diff    = std::fabs(now - nearest);
    if (diff <= config::NOTE_GOOD_WINDOW) {                 // 在判定窗口内按＝命中
        const bool perfect = diff <= config::NOTE_PERFECT_WINDOW;
        ++m_combo;
        if (m_combo > m_maxCombo) m_maxCombo = m_combo;
        m_score += perfect ? 100 : 50;
        m_timeRemaining = std::min(config::START_TIME, m_timeRemaining +
                                   (perfect ? config::HIT_TIME_PERFECT : config::HIT_TIME_GOOD));
        setJudge(perfect ? "完美" : "好", perfect ? "PERFECT" : "GOOD",
                 perfect ? Color::Yellow : Color::Green);
        // X 颜色反馈：perfect → 红；good → 白（鼓声每拍仍会放大，颜色只由判定决定）
        m_letterText.setFillColor(perfect ? Color::Red : Color::White);
        if (perfect) m_xPulse = 1.0f;   // perfect：X 额外放大一次强调
    } else {                                               // 远离鼓声拍＝时机失误
        m_combo = 0;
        m_timeRemaining -= config::MISS_TIME_PENALTY;
        setJudge("失误", "MISS", Color::Red);
        m_letterText.setFillColor(Color::White);   // 失误 → X 回白
    }
}

// 每帧推进：节拍 tick + 树枝下落 + 字母动画 + 时间条 + 碰撞/结束判定
void RhythmChopLevel::update(float dt)
{
    if (m_dead || m_cleared) return;   // 已结束不再推进

    const float beat = 60.0f / config::RHYTHM_BPM;
    m_beatClock += dt;   // 独立节拍时钟：由 dt 累计驱动，不依赖鼓声 getPlayingOffset（其播放后段会失效）

    // 跨拍检测：每过一拍（鼓声响起）只推进拍数用于通关计数 + X 放大缩小一次（鼓声驱动的节拍引导）
    while (m_currentBeat < config::RHYTHM_DRUM_BEATS &&
        m_beatClock >= static_cast<float>(m_currentBeat + 1) * beat)
    {
        ++m_currentBeat;
        m_xPulse = 1.0f;   // 鼓声每响一次，X 放大缩小一次（提醒跟拍）
    }

    // 通关判定：走满谱面（或拍数）→ 必须显式结算，否则画面会静止卡住
    const int clearBeat = (m_useBeatmap && !m_beatmap.empty())
                              ? static_cast<int>(m_beatmap.lastBeat()) + 1
                              : config::RHYTHM_DRUM_BEATS;
    if (m_currentBeat >= clearBeat) {
        m_cleared = true;
        doGameOver(tr("通关！", "CLEAR!"), false);   // 通关不播失败音
        return;
    }

    // X 脉冲衰减（perfect 触发后 0.15s 内回落到 1.0，驱动放大缩小）
    if (m_xPulse > 0.0f) {
        m_xPulse -= dt / 0.15f;
        if (m_xPulse < 0.0f) m_xPulse = 0.0f;
    }

    // 砍树动作推进：圆木按水平+垂直初速飞出（原版：水平∓5000 + 向上-1500，无重力）
    if (m_logActive) {
        m_logSpr.move(m_logVx * dt, m_logVy * dt);
        if (m_logSpr.getPosition().x < -100.0f || m_logSpr.getPosition().x > 2000.0f)
            m_logActive = false;   // 飞出左右屏幕边界即停（原版 LOG_OFFSCREEN）
    }

    // 背景装饰推进：云 + 蜜蜂漂移（本关自持场景元素）
    m_scenery.update(sf::seconds(dt));

    // 时间条自然衰减
    m_timeRemaining -= config::TIME_DECAY_PER_SEC * dt;
    if (m_timeRemaining < 0.0f) m_timeRemaining = 0.0f;
    if (m_timeRemaining <= 0.0f) {
        doGameOver(tr("时间到！！", "Out of time!!"), true);
        return;
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
// 玩家砍树触发：全部槽位下移一格
void RhythmChopLevel::advanceBranches() {
    // 全部槽位下移一格
    for (int i = config::RHYTHM_BRANCH_COUNT - 1; i > 0; --i)
        m_branches[i] = m_branches[i - 1];

    // 顶部（槽位0）按谱面或概率生成
    if (m_useBeatmap) {
        const Note* note = m_beatmap.query((float)m_currentBeat, 0.5f);
        m_branches[0] = note ? note->side : Side::None;
    }
    else {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        float r = dist(m_game.m_rng);
        if (r < m_pLeft)           m_branches[0] = Side::Left;
        else if (r < m_pLeft + m_pRight) m_branches[0] = Side::Right;
        else                        m_branches[0] = Side::None;
    }
}


// 碰撞：最底层(玩家层)树枝与玩家同侧 → 被压死（树枝撞到玩家，原版判定）
void RhythmChopLevel::checkCollision() {
    const Side bottom = m_branches[config::RHYTHM_BRANCH_COUNT - 1];
    if (bottom != Side::None && bottom == m_playerSide) {
        m_dead = true;
        m_player.die(m_playerSide);   // 压死视觉：小人移出屏幕、墓碑留在被压那一侧
        doGameOver(tr("你被压死了！", "SQUISHED!"), true);
    }
}

// 结束本局（死亡/时间到/通关）：停帧 + 提示 + 失败音效(可选) + 上榜结算
void RhythmChopLevel::doGameOver(const sf::String& msg, bool playFailSound) {
    m_game.m_paused = true;
    m_game.setMessage(msg);
    if (playFailSound) {
        m_fail.play();
        // 失败（压死/时间到）统一让斧头/圆木消失，避免残留画面
        m_axeSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
        m_logSpr.setPosition(config::BRANCH_OFFSCREEN, 0.0f);
        m_logActive = false;
    }
    m_drum.stop();
    m_game.stopBgm();   // 结束时立刻停掉关卡 BGM，别让音乐继续盖在结算画面上
    if (m_game.m_scoreboard.qualifiesForHighscore(m_score)) {
        m_game.m_enterNamePending = true;
        m_game.m_gameOverClock.restart();
    }
}

// 画本关全部场景（背景/云/蜜蜂 → 树 → 树枝 → 小人/墓碑 → 圆木 → 斧头）
void RhythmChopLevel::renderScene(sf::RenderWindow& w) {
    m_scenery.render(w);   // 背景 + 云 + 蜜蜂（最底层）
    w.draw(m_tree);        // 树（静态完整树，本关自持）

    // 下落中的树枝（长在树上，画在树前）
    for (int i = 0; i < config::RHYTHM_BRANCH_COUNT; ++i) {
        if (m_branches[i] == Side::None) continue;
        Sprite branch(m_game.m_texBranch);
        branch.setOrigin(config::BRANCH_ORIGIN_X, config::BRANCH_ORIGIN_Y);
        // 原版树枝无缩放：440 宽、锚点中心，无缩放时端部正好贴住树身（左→830/右→1110）
        const float y = i * config::RHYTHM_BRANCH_SPACING;
        if (m_branches[i] == Side::Left) { branch.setPosition(config::NOTE_LEFT_X, y); branch.setRotation(180.0f); }
        else                              { branch.setPosition(config::NOTE_RIGHT_X, y); branch.setRotation(0.0f); }
        w.draw(branch);
    }

    if (m_logActive) w.draw(m_logSpr);   // 单块圆木水平飞出（原版砍树反馈）
    m_player.render(w);                  // 小人 + 墓碑
    w.draw(m_axeSpr);                    // 斧头（瞬移到玩家砍击侧）
}

// 画按键提示 UI（提示框 + 常驻 X + 判定/连击），画在分数/时间条附近
void RhythmChopLevel::renderUI(sf::RenderWindow& w) {
    w.draw(m_hintBox);

    // 常驻大号 X：perfect 时放大缩小一次（scale = 1 + 脉冲×进度）
    float scale = 1.0f + config::HINT_LETTER_PULSE * m_xPulse;
    m_letterText.setScale(scale, scale);
    centerText(m_letterText, config::HINT_BOX_X + config::HINT_BOX_W / 2.0f, config::HINT_BOX_Y + 125.0f);
    w.draw(m_letterText);

    if (m_judgeClock.getElapsedTime().asSeconds() < 1.0f) w.draw(m_judgeText);
    if (m_combo > 1)                                       w.draw(m_comboText);
}

// ============================================================================
//  私有辅助
// ============================================================================

// 程序合成 60BPM 规律鼓声（节拍器"咚"，每拍一个低频短脉冲；无需外部音频文件）
void RhythmChopLevel::synthDrum() {
    // 1. 基础参数校验
    if (config::RHYTHM_BPM <= 0 || config::RHYTHM_DRUM_BEATS <= 0 || config::SAMPLE_RATE <= 0) {
        std::cerr << "[Error] synthDrum 参数无效 (BPM/Beats/SampleRate)\n";
        return;
    }

    const float beat   = 60.0f / config::RHYTHM_BPM;
    const float total  = config::RHYTHM_DRUM_BEATS * beat;   // 32 拍 × 1.0s = 32 秒
    const int   frames = static_cast<int>(total * config::SAMPLE_RATE);

    // 2. 安全分配
    std::vector<Int16> samples(static_cast<size_t>(frames), 0);

    // 4. 合成鼓点
    const int pulseLen = static_cast<int>(0.18f * config::SAMPLE_RATE);
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

    // 5. 加载到 SoundBuffer（SFML 会拷贝，安全）
    if (!m_drumBuffer.loadFromSamples(samples.data(),
        static_cast<sf::Uint64>(samples.size()),
        1, config::SAMPLE_RATE)) {
        std::cerr << "[Warning] 节奏鼓点合成失败 (loadFromSamples)\n";
    }
    m_drum.setBuffer(m_drumBuffer);
    m_drum.setVolume(config::DRUM_VOLUME);   // 鼓声是判定基准，音量独立于 BGM
}

// 程序合成失败音效：柔和下行三连音（G4→E4→C4），替代全局难听的死亡音效（仅本关用）
void RhythmChopLevel::synthFailSound() {
    const float freqs[3] = { 392.0f, 330.0f, 262.0f }; // G4→E4→C4
    const float noteLen = 0.12f;
    const float total = 3 * noteLen + 0.05f;

    if (config::SAMPLE_RATE <= 0 || total <= 0.0f || total > 5.0f) {
        std::cerr << "[Error] synthFailSound 参数异常\n";
        return;
    }

    const int frames = static_cast<int>(total * config::SAMPLE_RATE);
    if (frames <= 0 || frames > 220500) { // 44100 * 5
        std::cerr << "[Error] synthFailSound 采样数异常: " << frames << "\n";
        return;
    }

    std::vector<Int16> samples;
    try {
        samples.resize(frames, 0);
    }
    catch (const std::bad_alloc&) {
        std::cerr << "[Error] synthFailSound 内存分配失败\n";
        return;
    }

    for (int n = 0; n < 3; ++n) {
        const int start = static_cast<int>(n * noteLen * config::SAMPLE_RATE);
        const int len = static_cast<int>(noteLen * config::SAMPLE_RATE);
        for (int j = 0; j < len && start + j < frames; ++j) {
            const double t = j / static_cast<double>(config::SAMPLE_RATE);
            const double env = 1.0 - static_cast<double>(j) / len;
            const double phase = 6.283185307 * freqs[n] * t;
            samples[start + j] = static_cast<Int16>(5000.0 * env * std::sin(phase));
        }
    }

    if (!m_failBuffer.loadFromSamples(samples.data(),
        static_cast<sf::Uint64>(samples.size()),
        1, config::SAMPLE_RATE)) {
        std::cerr << "[Warning] 失败音效合成失败\n";
    }
    m_fail.setBuffer(m_failBuffer);
}

// 把文字中心放到 (x,y)
void RhythmChopLevel::centerText(Text& t, float x, float y) {
    const FloatRect rect = t.getLocalBounds();
    t.setOrigin(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    t.setPosition(x, y);
}

// 刷新判定文字
void RhythmChopLevel::setJudge(const char* zh, const char* en, Color c) {
    m_judgeText.setString(tr(zh, en));
    m_judgeText.setFillColor(c);
    centerText(m_judgeText, config::SCREEN_CENTER_X, 320.0f);
    m_judgeClock.restart();
}
