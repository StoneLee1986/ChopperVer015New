// CHOPPER - DisplayManager implementation
#include "DisplayManager.h"

// ============================================================================
//  DisplayManager 实现：窗口、显示模式、分辨率与配置持久化
//  视图自适应：把内部 1920x1080 逻辑坐标映射到实际窗口，采用 letterbox
//  （等比缩放 + 居中）：任何分辨率（含竖屏/异形屏）都画面不变形、不崩溃
//  初始化：读配置 + 建窗（构造函数里调用一次）
// ============================================================================
void DisplayManager::init() {
    loadSettings();    // 读配置（显示模式、分辨率、垂直同步、帧率），缺失/非法回退默认
    applyDisplay();    // 按当前模式建窗（含垂直同步/帧率应用、重算视图、写回配置）
}

// ============================================================================
void DisplayManager::updateViewport() {
    const float winW = static_cast<float>(m_window.getSize().x);
    const float winH = static_cast<float>(m_window.getSize().y);
    if (winW < 1.0f || winH < 1.0f) return;  // 窗口尺寸异常（极小/0）时直接跳过，避免除零崩溃
    const float logicRatio =
        static_cast<float>(config::SCREEN_WIDTH) / static_cast<float>(config::SCREEN_HEIGHT); // 16:9
    const float winRatio = winW / winH;
    float vw, vh;  // 视图占窗口的比例（0~1），其余为黑边
    if (winRatio > logicRatio) {  // 窗口比 16:9 更宽 → 高度铺满，左右留黑边
        vh = 1.0f;
        vw = logicRatio / winRatio;
    } else {                      // 窗口更高或同比例（含竖屏）→ 宽度铺满，上下留黑边
        vw = 1.0f;
        vh = winRatio / logicRatio;
    }
    // 视图范围 = 整个内部逻辑坐标；viewport = 窗口里显示的区域（居中）
    m_view = View(FloatRect(0.0f, 0.0f,
                            static_cast<float>(config::SCREEN_WIDTH),
                            static_cast<float>(config::SCREEN_HEIGHT)));
    m_view.setViewport(FloatRect((1.0f - vw) / 2.0f, (1.0f - vh) / 2.0f, vw, vh));
}

// ============================================================================
//  显示模式与手动分辨率：读配置、保存、建窗、切换
// ============================================================================
//  候选下标 → 宽高（idx 1..COMMON_RES_COUNT；越界回落到末项）
std::pair<unsigned int, unsigned int> DisplayManager::commonResolution(int idx) const {
    if (idx < 1) idx = 1;
    if (idx > config::COMMON_RES_COUNT) idx = config::COMMON_RES_COUNT;
    return config::COMMON_RESOLUTIONS[idx - 1];
}

//  目标分辨率是否被当前显示器"全屏"支持（遍历 getFullscreenModes，SFML2 针对主显示器）
bool DisplayManager::resolutionSupported(unsigned int w, unsigned int h) const {
    for (const VideoMode& m : VideoMode::getFullscreenModes())
        if (m.width == w && m.height == h) return true;
    return false;
}

//  从配置文件读取显示模式与分辨率；文件缺失 / 格式非法 / 值不识别 → 回退默认（全屏 + AUTO）
void DisplayManager::loadSettings() {
    m_displayMode = DisplayMode::Fullscreen;   // 默认全屏
    m_resIndex = 0;                            // 默认 AUTO（跟随桌面）
    m_vsyncEnabled  = config::VSYNC_DEFAULT != 0;      // 默认开垂直同步
    m_framerateIndex = config::FRAMERATE_DEFAULT;      // 默认 AUTO
    m_showFps       = config::FPS_SHOW_DEFAULT != 0;   // 默认显示内置 FPS
    std::ifstream in(config::SETTINGS_FILE);
    std::string key, value;
    while (in >> key >> value) {
        if (key == "MODE") {
            if (value == "borderless") m_displayMode = DisplayMode::Borderless;
            else if (value == "windowed") m_displayMode = DisplayMode::Windowed;
            else m_displayMode = DisplayMode::Fullscreen;
        } else if (key == "RESOLUTION") {
            if (value == "AUTO") {
                m_resIndex = 0;
            } else {
                const auto x = value.find('x');   // 形如 "1920x1080"
                if (x != std::string::npos) {
                    const unsigned int w = static_cast<unsigned int>(std::stoul(value.substr(0, x)));
                    const unsigned int h = static_cast<unsigned int>(std::stoul(value.substr(x + 1)));
                    m_resIndex = 0;   // 默认 AUTO，仅在候选表里匹配到时改成对应档位
                    for (int i = 1; i <= config::COMMON_RES_COUNT; ++i)
                        if (config::COMMON_RESOLUTIONS[i - 1].first  == w &&
                            config::COMMON_RESOLUTIONS[i - 1].second == h) { m_resIndex = i; break; }
                }
            }
        } else if (key == "VSYNC") {
            m_vsyncEnabled = (value == "1");      // 1=开 0=关；非法值按关处理（下次保存会纠正）
        } else if (key == "FRAMERATE") {
            m_framerateIndex = 0;                 // 默认 AUTO，仅在档位名匹配时改成对应档
            for (int i = 0; i < config::FRAMERATE_COUNT; ++i)
                if (value == config::FRAMERATE_NAMES[i]) { m_framerateIndex = i; break; }
        } else if (key == "SHOWFPS") {
            m_showFps = (value == "1");
        } else if (key == "LANG") {
            config::g_lang = (value == "EN") ? config::Lang::En : config::Lang::Zh;
        }
    }
}

//  把当前显示模式与分辨率写回配置文件（两行：MODE / RESOLUTION）
void DisplayManager::saveSettings() {
    std::ofstream out(config::SETTINGS_FILE);
    out << "MODE "
        << (m_displayMode == DisplayMode::Fullscreen ? "fullscreen"
          : m_displayMode == DisplayMode::Borderless ? "borderless"
          : "windowed") << "\n";
    out << "RESOLUTION ";
    if (m_resIndex == 0) {
        out << "AUTO\n";
    } else {
        const auto r = commonResolution(m_resIndex);
        out << r.first << "x" << r.second << "\n";
    }
    out << "VSYNC " << (m_vsyncEnabled ? "1" : "0") << "\n";
    const int fi = (m_framerateIndex >= 0 && m_framerateIndex < config::FRAMERATE_COUNT)
                 ? m_framerateIndex : 0;
    out << "FRAMERATE " << config::FRAMERATE_NAMES[fi] << "\n";
    out << "SHOWFPS " << (m_showFps ? "1" : "0") << "\n";
    out << "LANG " << (config::g_lang == config::Lang::Zh ? "ZH" : "EN") << "\n";
}

//  核心：按当前显示模式建窗。选 Style 与尺寸 → 校验回退 → create → 重算视图 → 立即保存
void DisplayManager::applyDisplay() {
    VideoMode mode;
    Uint32 style;
    switch (m_displayMode) {
        case DisplayMode::Fullscreen: {
            style = Style::Fullscreen;
            if (m_resIndex == 0) {
                mode = VideoMode::getDesktopMode();          // AUTO：跟随主屏桌面分辨率
            } else {
                const auto r = commonResolution(m_resIndex);
                mode = VideoMode(r.first, r.second);
                // 保护：全屏分辨率必须被当前显示器支持，否则回退桌面并纠正为 AUTO（防拷到别台电脑进不去）
                if (!resolutionSupported(mode.width, mode.height)) {
                    mode = VideoMode::getDesktopMode();
                    m_resIndex = 0;
                }
            }
            break;
        }
        case DisplayMode::Borderless: {
            style = Style::None;                             // 无边框窗口铺满主屏，分辨率固定跟随桌面
            mode = VideoMode::getDesktopMode();
            break;
        }
        case DisplayMode::Windowed: {
            style = Style::Titlebar | Style::Close | Style::Resize; // 可拖拽、可缩放
            const VideoMode desk = VideoMode::getDesktopMode();
            const float maxW = desk.width  * config::WINDOW_MAX_SCALE;
            const float maxH = desk.height * config::WINDOW_MAX_SCALE;
            float w, h;
            if (m_resIndex == 0) {                           // AUTO 窗口尺寸 = 桌面 90%
                w = maxW; h = maxH;
            } else {
                const auto r = commonResolution(m_resIndex);
                w = static_cast<float>(r.first);
                h = static_cast<float>(r.second);
                // 窗口尺寸超桌面上限则等比缩小（SFML2 无"工作区"API，用桌面×比例兜底）
                float scale = 1.0f;
                if (w > maxW) scale = maxW / w;
                if (h > maxH) { const float s = maxH / h; if (s < scale) scale = s; }
                w *= scale; h *= scale;
            }
            mode = VideoMode(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
            break;
        }
    }
    m_resolution = mode;
    m_window.create(mode, "Timber!!!", style);   // 重建窗口（用新模式/尺寸）
    applySyncSettings();                         // create 会重置垂直同步/帧率，必须按玩家设置重开（否则切模式掉回默认）
    if (m_displayMode == DisplayMode::Borderless)
        m_window.setPosition(Vector2i(0, 0));    // 无边框：钉在主屏左上角
    updateViewport();                            // 重建后按新尺寸重算视图
    saveSettings();                              // 立即写回配置
}

//  设置界面：切换显示模式（全屏 → 无边框 → 窗口化 → 循环），立即生效
void DisplayManager::cycleMode() {
    if (m_displayMode == DisplayMode::Fullscreen)      m_displayMode = DisplayMode::Borderless;
    else if (m_displayMode == DisplayMode::Borderless) m_displayMode = DisplayMode::Windowed;
    else                                               m_displayMode = DisplayMode::Fullscreen;
    applyDisplay();
}

//  设置界面：切换分辨率候选（AUTO + 6 档，循环）；无边框模式分辨率固定，不响应
void DisplayManager::cycleResolution() {
    if (m_displayMode == DisplayMode::Borderless) return;
    m_resIndex = (m_resIndex + 1) % (config::COMMON_RES_COUNT + 1); // 0..6 循环
    applyDisplay();
}

//  应用垂直同步与帧率上限（不重建窗口，只切显示参数；create 会重置二者，重建后须重调）
void DisplayManager::applySyncSettings() {
    m_window.setVerticalSyncEnabled(m_vsyncEnabled);
    if (m_vsyncEnabled) {
        m_window.setFramerateLimit(0);                       // 垂直同步优先：禁用帧率上限，锁显示器刷新率
    } else {
        const int idx = (m_framerateIndex >= 0 && m_framerateIndex < config::FRAMERATE_COUNT)
                      ? m_framerateIndex : 0;
        m_window.setFramerateLimit(config::FRAMERATE_OPTIONS[idx]); // 0=不限（AUTO/UNLIMITED）
    }
}

//  设置界面：切换垂直同步开关（ON/OFF），立即生效并写回配置
void DisplayManager::cycleVsync() {
    m_vsyncEnabled = !m_vsyncEnabled;
    applySyncSettings();
    saveSettings();
}

//  设置界面：切换帧率档位（AUTO/60/120/144/240/UNLIMITED 循环），立即生效并写回配置
void DisplayManager::cycleFramerate() {
    m_framerateIndex = (m_framerateIndex + 1) % config::FRAMERATE_COUNT;
    applySyncSettings();
    saveSettings();
}

//  设置界面：切换内置 FPS 显示开关（只改显示，立即生效并写回配置）
void DisplayManager::cycleShowFps() {
    m_showFps = !m_showFps;
    saveSettings();
}
