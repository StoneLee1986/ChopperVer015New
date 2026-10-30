// ============================================================================
//  BeatDriveLevel：关卡 B · 鼓点驱动下落（020 方向，恢复为独立关卡）
// ============================================================================
//  玩法（020 逻辑，鼓点驱动）：60BPM 鼓声每响一次 = 一拍，每拍树枝整体下落一格 + 顶部
//  按概率(左50/右50/无0)生成新枝；右下方提示框出现 A/B 字母（即将压到玩家的枝在左→提示
//  "B"按右避开、在右→提示"A"按左避开、该拍无枝→不显示）；字母做 淡入+鼓响脉冲+淡出。
//  玩家在鼓声瞬间按对应方向键 → 小人移到该侧 + 判定(Perfect/Good/Miss，影响得分/时间/连击)。
//  失败：最底槽树枝与玩家同侧 = 被压死；时间归零也判负；鼓声播完 = 通关。
//  本类完全自包含（背景/树/小人/树枝/判定/音乐/UI 全私有），与 021 砍树驱动关卡
//  RhythmChopLevel 零共享可变状态；经 friend 访问 Game 的只读资源（纹理/字体/随机/音效/窗口/结算）。
// ============================================================================
#ifndef CHOPPER_BEATDRIVELEVEL_H
#define CHOPPER_BEATDRIVELEVEL_H

#include "ILevel.h"
#include "config.h"
#include "Scenery.h"
#include "Player.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>

class Game;   // 前置声明：关卡类经 friend 访问 Game 私有成员

// 树枝信息结构体（带缩放倍数）
struct BranchInfo {
    Side side;          // Left / Right / None
    float scale;        // 当前缩放倍数 (初始 1.0, 每格 +5%)
};

class BeatDriveLevel : public ILevel {
public:
    explicit BeatDriveLevel(Game& g);

    // ---- ILevel（关卡完全自包含：背景/树/小人/判定位置全在本关内管理） ----
    void initScene() override;                 // 合成鼓声 + 建背景/树/小人/斧头/圆木 + 提示框UI
    void onStart()  override;                  // 每局开始：复位背景/树/小人/树枝/字母/得分 + 播放鼓声
    void handleInput(Side side) override;      // 玩家按键：更新站位 + 判定时机(Perfect/Good/Miss)
    void update(float dt) override;            // 推进节拍器 tick + 树枝 + 字母动画 + 圆木 + 背景 + 时间条 + 碰撞
    void renderScene(sf::RenderWindow& w) override;   // 画本关全部场景：背景/云/蜜蜂 → 树 → 树枝 → 小人/墓碑 → 圆木 → 斧头
    void renderUI(sf::RenderWindow& w) override;      // 画本关 HUD：提示框 + 固定文字 + A/B 字母 + 判定/连击文字
    int   score() const override { return m_score; }
    float timeRemaining() const override { return m_timeRemaining; }
    float timeBarWidthPerSecond() const override { return m_timeBarWidthPerSecond; }
    bool  isCleared() const override { return m_cleared; }
    bool  isDead()    const override { return m_dead; }
    void  pause()  override { m_drum.pause(); }
    void  resume() override { m_drum.play(); }
    void  stop()   override { m_drum.stop(); }

    // ---- 概率接口：可挂难度（默认 左45/右45/无10） ----
    void setSpawnProb(float pLeft, float pRight) {
        m_pLeft = pLeft; m_pRight = pRight; m_pNone = 1.0f - pLeft - pRight;
        if (m_pNone < 0.0f) m_pNone = 0.0f;   // 钳位：左右概率之和不允许超 100%
    }

private:
    void synthDrum();          // 程序合成 60BPM 规律鼓声（节拍器"咚"，无需外部文件）
    void synthFailSound();     // 程序合成失败音效（柔和下行三连音，替代全局死亡音效）
    void advanceBranches();    // 全部槽位下移一格 + 顶部按概率生成新枝 + 决定本拍字母
    void checkCollision();     // 最底层树枝与玩家同侧 → 撞死（触发 Game 结算）
    void centerText(Text& t, float x, float y);   // 把文字中心放到 (x,y)
    void setJudge(const char* zh, const char* en, Color c); // 刷新判定文字
    void doGameOver(const sf::String& msg, bool playFailSound); // 结束本局：停帧+提示+失败音效+上榜结算

    Game& m_game;

    // ---- 本关场景元素（完全自包含，不再由 Game 全局共用） ----
    Scenery m_scenery;         // 背景 + 云 + 蜜蜂（场景背景层，自持漂移动画）
    Player  m_player;          // 小人 + 墓碑（玩家化身，位置由本关控制）
    Sprite  m_tree;            // 树（静态完整树，本关自持）

    // ---- 复现原版砍树动作（timber.cpp：斧头换侧瞬移 + 单块圆木向反侧飞出） ----
    Sprite m_axeSpr;             // 斧头：瞬移到砍击侧（无挥砍动画，原版逻辑）
    Sprite m_logSpr;             // 圆木：单块，从树心向砍击反侧飞出
    float  m_logVx = 0.0f;       // 圆木水平速度（砍右→左飞、砍左→右飞）
    float  m_logVy = 0.0f;       // 圆木垂直速度（原版向上 -1500）
    bool   m_logActive = false;  // 圆木是否正在飞出

    // ---- 节拍 / 鼓声 ----
    SoundBuffer m_drumBuffer;      // 鼓声采样数据（60BPM，程序合成）
    Sound       m_drum;            // 鼓声播放器（getPlayingOffset 作 tick 基准）
    SoundBuffer m_failBuffer;      // 失败音效采样数据（程序合成，柔和下行提示音）
    Sound       m_fail;            // 失败音效播放器（替代全局难听的 death.wav，仅本关用）
    int         m_currentBeat = 0; // 已触发的拍数
    float       m_beatClock  = 0.0f; // 独立节拍时钟（由 dt 累计驱动，不依赖鼓声 getPlayingOffset，保证走满/通关可靠）
    
    // ---- 音频同步（自动校准音轨偏移，解决硬件延迟问题） ----
    bool m_useAudioSync = config::AUDIO_SYNC_ENABLED;     // 启用音频同步
    float m_audioOffset = 0.0f;                           // 累积的音频偏移量（秒），每帧平滑更新到独立节拍时钟
    bool        m_ended = false;    // 结算互斥锁：压死/时间到/通关三种结局只执行最先触发的那一种
    bool        m_cleared = false; // 鼓声播完＝通关
    bool        m_dead    = false; // 被压死

    // ---- 树枝（槽位：0 顶 … 底=玩家层） ----
    BranchInfo m_branches[config::RHYTHM_BRANCH_COUNT];
    // ---- 玩家站位（按方向键更新） ----
    Side m_playerSide = Side::Left;

    // ---- 字母提示 ----
    bool  m_letterActive = false;      // 本拍是否显示字母（NONE 拍不显示）
    Side  m_letterSide   = Side::Left; // 本拍"应砍方向"（= 新枝反侧）：新枝左→Right、新枝右→Left
    float m_letterT      = 0.0f;       // 本拍内进度 0~1（驱动淡入/脉冲/淡出）
    int   m_countdown    = 0;          // 开局倒计时拍数（树枝离玩家还远时，在AB位置显示 3,2,1）
    Text  m_hintFixed;                 // 固定提示行（双语）
    Text  m_letterText;                // 大号 A / B 字母
    Text  m_judgeText, m_comboText;    // 判定文字 / 连击文字
    float m_judgeScale = 1.0f;         // 判定文字弹出缩放（1.0 = 正常）
    float m_comboScale = 1.0f;         // 连击文字弹跳缩放
    int   m_lastInputBeat = -100;      // 上一次玩家输入的拍号（纯享受模式据此判断是否该代打）
    RectangleShape m_hintBox;          // 提示框（右半边靠下）
    Clock m_judgeClock;                // 判定文字显示计时

    // ---- 得分 / 时间条（沿用现有机制：开局 12s，命中回血、Miss 扣血、每秒自然衰减） ----
    int   m_score = 0, m_combo = 0, m_maxCombo = 0;
    float m_timeRemaining = config::START_TIME;
    float m_timeBarWidthPerSecond = config::TIME_BAR_WIDTH / config::START_TIME;

    // ---- 树枝生成概率接口（可挂难度；关卡 B 特化：左50/右50/无0，不随全局 config 的 45/45/10） ----
    float m_pLeft  = 0.5f;
    float m_pRight = 0.5f;
    float m_pNone  = 0.0f;
};

#endif
