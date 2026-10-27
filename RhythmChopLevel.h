// ============================================================================
//  RhythmChopLevel：关卡3 · 节奏鼓点（回归原版砍树逻辑的音游砍树）
// ============================================================================
//  玩法：回归原版砍树逻辑 —— 玩家每砍一次树 = 树枝整体下落一格 + 顶部按概率(左45/右45/无10)生成新枝，
//  且若最底槽树枝在玩家所砍那一侧，一砍即被压死（先下落再判，原版顺序）。鼓声不再驱动树枝，
//  只作节拍引导：右下提示框内常驻一个大号 X，提示玩家跟着鼓点节奏砍树；砍树时刻踩中节拍(perfect)
//  时 X 明显放大缩小一次。计分后做；通关 = 鼓声播完(32拍)；时间条初始 12s，命中回血/失误扣血/自然衰减。
//  本类自持鼓声/树枝/判定/时间/得分，完全独立于 MusicSystem 的下落音符机制。
//  经 friend 访问 Game 的共享资源（纹理/字体/随机/音效/窗口/结算）。
// ============================================================================
#ifndef CHOPPER_RHYTHMCHOPLEVEL_H
#define CHOPPER_RHYTHMCHOPLEVEL_H

#include "ILevel.h"
#include "config.h"
#include "Scenery.h"
#include "Player.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>

class Game;   // 前置声明：关卡类经 friend 访问 Game 私有成员

class RhythmChopLevel : public ILevel {
public:
    explicit RhythmChopLevel(Game& g);

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
    void advanceBranches();    // 全部槽位下移一格 + 顶部按概率生成新枝（玩家砍树触发，原版）
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
    bool        m_cleared = false; // 鼓声播完＝通关
    bool        m_dead    = false; // 被压死

    // ---- 树枝（3 槽位：0 顶 … 2 底=玩家层） ----
    Side m_branches[config::RHYTHM_BRANCH_COUNT] = {};
    // ---- 玩家站位（按方向键更新） ----
    Side m_playerSide = Side::Left;

    // ---- 节拍引导 X（常驻，不再分左右/淡入淡出） ----
    Text  m_letterText;                // 常驻大号 X（perfect 时放大缩小提示节拍）
    float m_xPulse = 0.0f;             // X 脉冲进度 0~1（perfect 瞬间置 1，随帧衰减）
    Text  m_judgeText, m_comboText;    // 判定文字 / 连击文字
    RectangleShape m_hintBox;          // 提示框（右半边靠下）
    Clock m_judgeClock;                // 判定文字显示计时

    // ---- 得分 / 时间条（沿用现有机制：开局 12s，命中回血、Miss 扣血、每秒自然衰减） ----
    int   m_score = 0, m_combo = 0, m_maxCombo = 0;
    float m_timeRemaining = config::START_TIME;
    float m_timeBarWidthPerSecond = config::TIME_BAR_WIDTH / config::START_TIME;

    // ---- 树枝生成概率接口（可挂难度；默认值来自 config） ----
    float m_pLeft  = config::RHYTHM_P_LEFT;
    float m_pRight = config::RHYTHM_P_RIGHT;
    float m_pNone  = config::RHYTHM_P_NONE;
};

#endif
