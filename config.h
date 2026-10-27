// ============================================================================
//  CHOPPER 公共配置（config.h）：共享头文件
//  所有子系统（MusicSystem / DisplayManager / Scoreboard / NameEntryUI /
//  SettingsMenu / Game）都包含本头，统一获得 SFML 命名空间与全部"写死的数字"。
// ============================================================================
#ifndef CHOPPER_CONFIG_H
#define CHOPPER_CONFIG_H

// ---- 头文件 ----
#include <SFML/Graphics.hpp>   // SFML 图形库：窗口、精灵、文字、形状
#include <SFML/Audio.hpp>      // SFML 音频库：音效播放
#include <random>              // C++ 随机数引擎（替代原版 srand+rand）
#include <string>              // std::string / std::to_string
#include <iostream>            // std::cerr，用来打印加载失败警告
#include <fstream>             // 最高分存档文件读写
#include <vector>              // std::vector 榜单
#include <cctype>              // std::toupper（名字输入转大写）
#include <algorithm>           // std::sort（榜单排序）
#include <utility>             // std::pair（分辨率候选表）
#include <cmath>               // std::sin / std::fabs（旋律合成与音符判定）

using namespace sf;            // 省得每次写 sf::RenderWindow 这么长

// 玩家站在树的哪一侧（方向键输入 / 树枝槽位 / 判定共用）。原定义在 MusicSystem.h，
// MusicSystem 已删，移到这里（全局枚举，关卡/Player/Game 共用）。
enum class Side { Left, Right, None };


// ============================================================================
//  config 命名空间：把所有"写死的数字"集中到这里，方便以后调
// ============================================================================
namespace config {
    // ---- 窗口尺寸 ----
    constexpr unsigned int SCREEN_WIDTH   = 1920u;   // 内部"逻辑分辨率"宽（1920），窗口按此自动适配，不再写死实际屏幕
    constexpr unsigned int SCREEN_HEIGHT  = 1080u;   // 内部"逻辑分辨率"高（1080）
    constexpr float SCREEN_CENTER_X = SCREEN_WIDTH  / 2.0f;   // 屏幕中心 X = 960
    constexpr float SCREEN_CENTER_Y = SCREEN_HEIGHT / 2.0f;   // 屏幕中心 Y = 540
    // ---- 显示模式与手动分辨率（运行时由成员控制，这里只放默认与候选表） ----
    constexpr const char* SETTINGS_FILE = "settings.txt";   // 配置文件（记住显示模式与分辨率）
    // 常见分辨率候选（除 AUTO=跟随桌面 外，手动可选的常用横屏分辨率）
    // 用 const 而非 constexpr：std::pair 的构造函数不是 constexpr，用 constexpr 数组在 C++14 下会编译失败
    static const std::pair<unsigned int, unsigned int> COMMON_RESOLUTIONS[] = {
        { 3840u, 2160u },   // 4K
        { 2560u, 1440u },   // 2K
        { 1920u, 1080u },   // 1080p
        { 1600u, 900u },    // 900p
        { 1366u, 768u },    // 768p
        { 1280u, 720u }     // 720p
    };
    constexpr int    COMMON_RES_COUNT = 6;   // 上面候选个数（含 AUTO 则共 7 档）
    constexpr float  WINDOW_MAX_SCALE = 0.9f;// 窗口模式下窗口尺寸上限 = 桌面×0.9（SFML2 无"工作区"API，留边距防超出）
    // ---- 垂直同步与帧率上限（玩家可调，写进 settings.txt） ----
    constexpr int   VSYNC_DEFAULT     = 1;    // 默认开垂直同步
    constexpr int   FRAMERATE_DEFAULT = 0;    // 默认 AUTO（不设帧率上限，跟随显示器刷新率）
    constexpr int   FRAMERATE_COUNT   = 6;    // 帧率档位数
    // 各档位传给 setFramerateLimit 的值（0 = 不限）。与 FRAMERATE_NAMES 下标一一对应
    static const int FRAMERATE_OPTIONS[FRAMERATE_COUNT] = { 0, 60, 120, 144, 240, 0 };
    // 帧率档位显示名：AUTO 与 UNLIMITED 值都是 0（不限），语义上 AUTO=交给系统、UNLIMITED=主动不设上限
    static const char* const FRAMERATE_NAMES[FRAMERATE_COUNT] = { "AUTO","60","120","144","240","UNLIMITED" };
    constexpr int   FPS_SHOW_DEFAULT = 1;    // 默认显示内置 FPS（验证高刷新率；游戏完成后再考虑去掉）

    // ---- 音频采样率与判定窗口（节奏鼓点关共用） ----
    constexpr int    SAMPLE_RATE      = 44100;     // 采样率
    constexpr float  NOTE_LEFT_X      = 610.0f;    // 树枝贴树左轨道 X（原版 BRANCH_LEFT_X，根部贴树左缘）
    constexpr float  NOTE_RIGHT_X     = 1330.0f;   // 树枝贴树右轨道 X（原版 BRANCH_RIGHT_X，根部贴树右缘）
    constexpr float  BRANCH_SCALE     = 0.65f;     // 树枝显示缩放，避免重叠像"一排草"
    constexpr float  NOTE_PERFECT_WINDOW = 0.15f;  // 判定窗口：PERFECT ±0.15s（宽松）
    constexpr float  NOTE_GOOD_WINDOW    = 0.30f;  // 判定窗口：GOOD ±0.30s（宽松，之外 MISS）
    constexpr float  TIME_DECAY_PER_SEC  = 0.30f;  // 时间条每秒自然衰减（秒/秒）
    constexpr float  HIT_TIME_BONUS      = 0.6f;   // 命中回血（秒）
    constexpr float  MISS_TIME_PENALTY   = 0.8f;   // MISS 扣血（秒）

    // ---- 场景里各元素的固定坐标（对齐原版 Timber!!!：玩家站在树两侧外侧、树枝根部贴树） ----
    constexpr float TREE_X          = 810.0f;   // 树的贴图左上角 X
    constexpr float PLAYER_Y        = 720.0f;   // 小人脚的 Y 坐标（原版：贴近树底，被树枝压到才判负）
    constexpr float PLAYER_LEFT_X   = 580.0f;   // 小人站左边时的 X（原版，树枝外侧）
    constexpr float PLAYER_RIGHT_X  = 1200.0f;  // 小人站右边时的 X（原版，树枝外侧）
    constexpr float RIP_Y           = 860.0f;   // 墓碑的 Y 坐标
    constexpr float RIP_HIDE_X      = 675.0f;   // 墓碑隐藏时的 X（移出屏幕）
    constexpr float RIP_HIDE_Y      = 2000.0f;  // 墓碑隐藏时的 Y（移出屏幕）
    constexpr float RIP_SQUISH_LEFT_X  = 560.0f;  // 左侧被压死时墓碑的 X
    constexpr float RIP_SQUISH_RIGHT_X = 1180.0f;  // 右侧被压死时墓碑的 X
    constexpr float BEE_INIT_Y      = 800.0f;  // 蜜蜂初始 Y
    constexpr float BEE_SPAWN_X     = 2000.0f; // 蜜蜂从屏幕右边（X=2000）飞进来
    constexpr float CLOUD_SPAWN_X   = -200.0f; // 云彩从屏幕左边（X=-200）飘进来

    // ---- 树枝 ----
    constexpr float  BRANCH_OFFSCREEN = 3000.0f; // 树枝/斧头/圆木隐藏时的 X（移出屏幕）
    constexpr float  BRANCH_ORIGIN_X = 220.0f;   // 树枝贴图的旋转锚点 X
    constexpr float  BRANCH_ORIGIN_Y = 20.0f;    // 树枝贴图的旋转锚点 Y

    // ---- 关卡1·节奏鼓点的砍树反馈（复现原版 timber.cpp：斧头瞬移换侧 + 单块圆木水平飞出） ----
    constexpr float AXE_SCALE       = 1.6f;      // 斧头放大（152x28 偏小，放大才醒目）
    constexpr float AXE_EMBED_ROT    = 30.0f;    // 嵌在树上时斧头角度(度,左)

    // ---- 关卡/玩法模式（当前单一关卡：节奏鼓点；保留枚举与层级，未来加关卡再加值） ----
    enum class GameMode { ChopRhythm };  // 关卡1·节奏鼓点（唯一关卡）

    // ---- 关卡3·节奏鼓点：节拍器驱动的音游砍树（独立于 MusicSystem 的下落音符机制） ----
    // 每拍鼓声 tick：树枝下落一格 + 字母提示 + 玩家在鼓声瞬间按对方向键
    constexpr float  RHYTHM_BPM             = 60.0f;   // 节拍速度（拍/分钟）＝ 每秒一拍，便于调试
    constexpr int    RHYTHM_DRUM_BEATS      = 32;      // 合成鼓声总拍数（播完＝通关；32 拍≈32 秒）
    constexpr int    RHYTHM_BRANCH_COUNT    = 6;       // 树枝槽位数（从顶部走 6 格到玩家层，对齐原版砍树）
    constexpr float  RHYTHM_BRANCH_SPACING  = 150.0f;  // 相邻槽位垂直间距（6 格：0…750，最底=玩家层）
    // 树枝生成概率接口（默认 左45/右45/无10，可随难度改写 m_pLeft/Right/None）
    constexpr float  RHYTHM_P_LEFT          = 0.45f;
    constexpr float  RHYTHM_P_RIGHT         = 0.45f;
    constexpr float  RHYTHM_P_NONE          = 0.10f;
    // 按键提示框（屏幕右半边靠下，半透明底 + 亮色描边）
    constexpr float  HINT_BOX_X   = 1290.0f;   // 框左上角 X
    constexpr float  HINT_BOX_Y   = 760.0f;    // 框左上角 Y
    constexpr float  HINT_BOX_W   = 500.0f;    // 框宽
    constexpr float  HINT_BOX_H   = 200.0f;    // 框高
    constexpr unsigned int HINT_FONT_SIZE   = 30;  // 固定提示行字号
    constexpr unsigned int HINT_LETTER_SIZE = 90;  // A/B 字母字号
    constexpr float  HINT_LETTER_PULSE = 0.35f;// 鼓响瞬间字母放大幅度（1.0 → 1+幅度 再回弹）
    // 关卡3·原版砍树动作（复现 timber.cpp：斧头换侧瞬移 + 单块圆木向反侧飞出）
    constexpr float  RHYTHM_AXE_LEFT_X  = 700.0f;   // 斧头在左侧位置 X（原版）
    constexpr float  RHYTHM_AXE_RIGHT_X = 1075.0f;  // 斧头在右侧位置 X（原版）
    constexpr float  RHYTHM_AXE_Y       = 830.0f;   // 斧头 Y（原版，贴近树底）
    constexpr float  RHYTHM_LOG_Y       = 720.0f;   // 圆木起点 Y（原版，树中部）
    constexpr float  RHYTHM_LOG_VX      = 5000.0f;  // 圆木水平飞出速度（原版极速，飞出干脆）
    constexpr float  RHYTHM_LOG_VY      = -1500.0f; // 圆木垂直初速（原版向上，负号=屏幕上方）
    constexpr float  RHYTHM_LOG_SCALE   = 0.6f;     // 圆木显示缩放（0.35 太小放大到约 180x72，醒目但不过大）

    // ---- 树（静态完整树，关卡1·节奏鼓点自持；本关不倒/不变细） ----
    constexpr float TREE_ANCHOR_X   = TREE_X + 150.0f;  // 树底部锚点 X = 960（贴图中心）
    constexpr float TREE_GROUND_Y   = 900.0f;           // 树底（贴图高 900，底部固定在地面）

    // ---- 时间条 ----
    constexpr float START_TIME       = 12.0f;  // 开局 12 秒（鼓点慢，给玩家充足反应时间）
    constexpr float TIME_BAR_WIDTH   = 400.0f; // 时间条满时宽 400 像素
    constexpr float TIME_BAR_HEIGHT  = 80.0f;  // 时间条高 80 像素
    constexpr float TIME_BAR_Y      = 980.0f; // 时间条在屏幕底部的 Y

    // ---- 资源文件路径（相对可执行文件） ----
    constexpr const char* ASSET_BACKGROUND = "graphics/background.png"; // 背景图
    constexpr const char* ASSET_TREE       = "graphics/tree.png";        // 树
    constexpr const char* ASSET_BEE        = "graphics/bee.png";         // 蜜蜂
    constexpr const char* ASSET_CLOUD      = "graphics/cloud.png";       // 云彩
    constexpr const char* ASSET_BRANCH     = "graphics/branch.png";     // 树枝
    constexpr const char* ASSET_PLAYER     = "graphics/player.png";     // 小人
    constexpr const char* ASSET_RIP        = "graphics/rip.png";         // 墓碑
    constexpr const char* ASSET_AXE        = "graphics/axe.png";         // 斧头（挥砍动画）
    constexpr const char* ASSET_LOG        = "graphics/log.png";         // 圆木（砍下的木屑）
    constexpr const char* ASSET_FONT       = "fonts/AlibabaPuHuiTi-3-55-Regular.ttf";     // 字体（阿里巴巴普惠体 3.0 Regular，免费商用，支持中文）
    constexpr const char* ASSET_CHOP_SOUND     = "sound/chop.wav";       // 砍树音效

    // ---- 最高分 ----
    constexpr int    HIGHSCORE_MAX = 20;      // 榜单最多 20 名
    constexpr int    NAME_MAX_LEN  = 10;      // 名字最长 10 个字母
    constexpr float  GAME_OVER_DELAY = 0.5f;  // 游戏结束画面停留秒数，给玩家反应时间
    constexpr const char* ASSET_HIGHSCORE = "highscores.txt"; // 存档文件

    // ---- 界面语言（设置界面可切换，持久化到 settings.txt；所有界面文字经 tr() 双语） ----
    enum class Lang { Zh, En };            // 支持的语言（以后加日文 = 加枚举值 + 扩展 tr 调用点）
    inline Lang g_lang = Lang::Zh;         // 当前语言，默认中文
    // 返回 sf::String 并显式按 UTF-8 解码：SFML 的 setString(std::string) 默认把每个字节当
    // 一个字符（不做 UTF-8 解码），中文必须用 fromUtf8 转成 UTF-32 的 sf::String 才能正确渲染。
    inline sf::String tr(const char* zh, const char* en) {
        const char* s = (g_lang == Lang::Zh) ? zh : en;   // 中文/英文二选一
        return sf::String::fromUtf8(s, s + std::char_traits<char>::length(s));
    }
}

// 把 tr 引入全局命名空间：调用点可直接写 tr(中,英)，无需 config:: 前缀
// （其余 config 成员都用 config:: 显式限定，唯独 tr 设计为裸名调用）
using config::tr;
#endif
