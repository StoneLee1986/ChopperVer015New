// ============================================================================
//  Game 类：游戏的全部状态和逻辑都在这里
// ============================================================================
//  CHOPPER 音乐游戏的总入口类。持有各子系统：MusicSystem（节奏/判定）、
//  DisplayManager（窗口/显示）、Scoreboard（榜单）、NameEntryUI（名字输入）、
//  SettingsMenu（设置界面），以及场景精灵与游戏状态机。
// ============================================================================
#ifndef CHOPPER_GAME_H
#define CHOPPER_GAME_H

#include "DisplayManager.h"
#include "Scoreboard.h"
#include "NameEntryUI.h"
#include "SettingsMenu.h"
#include "ILevel.h"
#include "RhythmChopLevel.h"
#include "BeatDriveLevel.h"
#include "Fx.h"

class Game {
public:
    Game();       // 构造函数：加载资源、摆好初始画面
    void run();   // 主循环：输入 → 更新 → 渲染，一直跑直到关窗

private:
    // 关卡类经 friend 访问 Game 的共享资源（纹理/随机/窗口/结算）
    friend class RhythmChopLevel;
    friend class BeatDriveLevel;

    // ---- 主循环里的几个阶段 ----
    void processEvents();                 // ① 读键盘/鼠标输入
    void update(const Time& dt);          // ② 按帧时间推进游戏状态
    void render();                        // ③ 把所有东西画到屏幕上
    void startNewGame(config::GameMode mode);  // 开始/重开一局（主菜单选关或暂停重开都带关卡）
    void setMessage(const sf::String& text); // 设置屏幕中央的提示文字（sf::String 才能正确渲染中文）
    void centerText(Text& t, float x, float y); // 把文字中心放到(x,y)（居中辅助）
    enum class MenuKind { Main, StageSelect, Pause }; // 菜单类型：主菜单 / 关卡选择 / 暂停
    void setupMenu(MenuKind kind);   // 按菜单类型填选项文字并重置选中项
    void returnToMainMenu();         // 统一"回主菜单"：菜单文字/选中项/BGM 一起复位
    void endRound(const sf::String& msg, bool failed); // 统一"结束本局"：暂停/提示/停乐/进结算
    int  fillMenuText(MenuKind kind);// 填某类菜单的文字并返回选项数（语言刷新复用）
    void enterStage(config::GameMode mode); // 进入某关"按回车开始"准备界面（主/关卡选择共用）
    void applyLanguage();           // 语言切换时刷新 Game 自身界面文字（菜单/确认/分数）
    void checkLanguage();           // 渲染前检测语言变化并统一刷新（任何界面下实时生效）
    void pauseBgm();                // 暂停当前关卡背景声（关卡3=鼓声，其余=MusicSystem 旋律）
    void resumeBgm();               // 恢复当前关卡背景声
    void stopBgm();                 // 停止当前关卡背景声

    // ---- 榜单/名字输入已分别归 Scoreboard / NameEntryUI；alignRow 是通用 UI 辅助（fps 也用） ----
    void alignRow(Text& t, float x, float y, int mode); // 行内对齐辅助（0居中 1左 2右）

    // ---- 资源加载与场景初始化 ----
    bool loadTexture(Texture& out, const char* path, const char* name); // 加载单张纹理
    void prewarmFont();     // 预烘焙字形：避免游戏中首次出现新字时卡一下
    void loadAssets();      // 一次性加载所有纹理、字体、音效
    void initScene();       // 把每个精灵摆好初始位置

    // ---- 关卡策略（两个关卡完全自包含：背景/树/小人/判定位置都在关卡内，零共享可变状态） ----
    ILevel* m_level = nullptr;            // 当前激活的关卡（startNewGame/enterStage 按 mode 绑定）
    RhythmChopLevel m_levelRhythm;        // 关卡 A · 021 砍树驱动（回归原版砍树逻辑）
    BeatDriveLevel  m_levelBeat;          // 关卡 B · 020 鼓点驱动（鼓点驱动下落 + A/B字母）

    // ---- 随机数引擎 ----
    std::mt19937 m_rng;    // 梅森旋转随机引擎，比 srand/rand 质量高

    // ---- 时钟与显示（窗口/视图/显示状态归 DisplayManager） ----
    Clock m_clock;          // 帧时钟：算出每帧花了多少秒
    DisplayManager m_disp;  // 窗口、显示模式、分辨率与配置持久化
    Scoreboard m_scoreboard; // 最高分榜单：数据、持久化与榜单界面
    NameEntryUI m_nameUI;    // 最高分名字输入界面（网格/预览/边沿检测）

    // 设置界面的状态与边沿检测已内聚到 SettingsMenu（m_settingsMenu）

    // ---- 内置 FPS 显示（计数与绘制留 Game，开关状态在 DisplayManager） ----
    Text  m_fpsText;            // 右上角 FPS 显示
    Clock m_fpsClock;           // FPS 统计计时器
    int   m_fpsFrames = 0;      // 累计帧数
    float m_fpsAccum  = 0.0f;   // 累计时间
    float m_fpsLastMs = 0.0f;   // 本帧耗时（毫秒）
    float m_fpsMaxMs  = 0.0f;   // 本轮统计窗口内的最差帧（毫秒）
    float m_upMs      = 0.0f;   // 本帧逻辑耗时
    float m_rndMs     = 0.0f;   // 本帧渲染耗时
    float m_upMax     = 0.0f;   // 窗口内最差逻辑帧
    float m_rndMax    = 0.0f;   // 窗口内最差渲染帧
    Clock m_probeClock;         // 分段计时用
    int   m_lastScoreShown = -1; // 已显示过的分数，避免每帧重排文字
    float m_lastTimeRemaining = -1.0f; // 上一帧剩余时间（用于时间条回血/扣血反馈）
    float m_timeFlash = 0.0f;          // 时间条高亮剩余时间（秒）

    // ---- 纹理（图片数据，一份就行，多个精灵共用；元素精灵放关卡内） ----
    Texture m_texBackground, m_texTree, m_texBee, m_texCloud;
    Texture m_texBranch, m_texPlayer, m_texRip;
    Texture m_texAxe, m_texLog;   // 斧头 / 圆木（砍树动画）
    Font m_font;   // 字体

    // ---- 音效 ----
    SoundBuffer m_soundBuffer;   // 砍树音效的音频数据
    Sound m_sound;               // 砍树音效播放器（关卡命中打击音效）
    // ---- BGM（Chiptune 离线渲染 → SoundBuffer → Sound 循环播放） ----
    sf::SoundBuffer m_bgmBufMenu;
    sf::SoundBuffer m_bgmBufLevelA;
    sf::SoundBuffer m_bgmBufLevelB;
    sf::Sound       m_bgmPlayer;
    enum class BgmState { None, Menu, LevelA, LevelB } m_currentBgm = BgmState::None;
    void switchBgm(BgmState target);

    // ---- 游戏状态变量 ----
    bool  m_paused              = true;   // 是否暂停（开局是暂停的）
    bool  m_escPaused           = false;  // 是否被 Esc 手动暂停（游戏进行中按 Esc 暂停）
    bool  m_escHeld             = false;  // Esc 是否已被按住（边沿检测，避免每帧重复切换）
    bool  m_acceptInput         = false;   // 是否接受方向键输入
    bool  m_enterHeld          = false;   // Enter 是否已被按住（边沿检测，避免每帧重复触发）

    // ---- 关卡模式（当前单一关卡：节奏鼓点；保留枚举与层级，未来加关卡再加值） ----
    config::GameMode m_mode = config::GameMode::ChopRhythm;  // 当前关卡

    // ---- 玩法风格选择（首次启动强制选一次；主菜单按 Tab 可随时改） ----
    bool m_styleSelect    = false;   // 是否正在显示风格选择界面
    int  m_styleSelection = 1;       // 当前高亮项（0=纯享受 1=标准 2=挑战）
    bool m_tabHeld        = false;   // 主菜单 Tab 键边沿检测
    void loadStyle();                // 读 playstyle.txt；没读过就进入选择界面
    void saveStyle();                // 写 playstyle.txt
    void drawStyleSelect();          // 画风格选择界面

    // ---- UI 元素 ----
    RectangleShape m_timeBar;   // 红色时间条
    Text m_messageText, m_scoreText; // 中央提示文字 + 左上角分数

    // ---- 菜单（主菜单 + 暂停菜单共用选项数组） ----
    bool  m_inMenu          = true;   // 是否处于主菜单
    bool  m_inStageSelect   = false;  // 是否处于"关卡选择"子菜单（m_inMenu 为真时生效）
    bool  m_inPauseMenu     = false;  // 是否处于暂停菜单（游戏中按 Esc 进入）
    int   m_menuSelection   = 0;      // 当前选中菜单项
    bool  m_menuInputHeld   = false;  // 上下键边沿检测，避免按住快速跳动
    Text  m_menuTitle;                // 菜单标题（仅主菜单显示 CHOPPER）
    Text  m_menuOptions[5];           // 菜单项文字（主菜单 5 项 / 暂停菜单 4 项）
    config::Lang m_lastLang = config::Lang::Zh; // 记录上次语言，检测切换后刷新各界面文字

    // ---- 退出二次确认 ----
    bool  m_confirmExit        = false;  // 是否显示退出确认界面
    int   m_confirmSelection   = 0;      // 确认界面选中项（0=NO 取消 1=YES 退出）
    bool  m_qHeld              = false;  // Q 键边沿检测，避免按住重复弹窗
    bool  m_confirmMoveHeld    = false;  // 确认界面 ↑↓ 边沿检测
    bool  m_confirmCancelHeld  = false;  // 确认界面 Esc/Q 取消边沿检测
    Text  m_confirmTitle;                // 确认标题：EXIT GAME?
    Text  m_confirmOptions[2];           // 确认选项：NO / YES

    // ---- 最高分 ----
    bool  m_enterNamePending = false;   // 游戏结束画面是否等待进入名字输入
    bool  m_confirmClear     = false;   // 是否显示"清除记录"确认界面
    bool  m_clearHeld        = false;   // C 键边沿检测（清除记录）
    Clock m_gameOverClock;              // 游戏结束画面停留计时
    // 名字输入界面（网格/预览/边沿检测）内聚到 NameEntryUI（m_nameUI）

    // ---- 设置菜单（当前为骨架，功能占位） ----
    SettingsMenu m_settingsMenu;  // 设置界面（14 项 UI + 边沿检测 + 返回编排）

    // ---- 特效系统（关卡经 friend 访问；震动/闪光/hit-stop 统一在这里） ----
    Fx m_fx;
    float m_dtDebt = 0.0f;   // hit-stop 欠下的时间，下一帧补回，保证节拍时钟不漂移

    // ---- 帧耗时统计（perf.log，定位卡顿用） ----
    float m_perfAccum  = 0.0f;
    float m_perfMax    = 0.0f;
    int   m_perfFrames = 0;
};

#endif
