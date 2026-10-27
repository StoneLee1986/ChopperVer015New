// CHOPPER - SettingsMenu implementation
#include "SettingsMenu.h"

// ============================================================================
//  SettingsMenu 实现：设置界面
// ============================================================================
//  14 项设置标签的中英文对照（init 与 applyLanguage 共用；值文本另行处理）
namespace {
    const char* const ZH_LABELS[14] = {
        "音量", "音乐音量", "显示模式", "分辨率",
        "垂直同步", "帧率", "显示FPS",
        "难度", "手柄", "震动", "按键设置",
        "语言", "恢复默认", "返回菜单"
    };
    const char* const EN_LABELS[14] = {
        "SOUND VOLUME", "MUSIC VOLUME", "DISPLAY MODE", "RESOLUTION",
        "V-SYNC", "FRAME RATE", "SHOW FPS",
        "DIFFICULTY", "CONTROLLER", "VIBRATION", "KEYBINDINGS",
        "LANGUAGE", "RESET TO DEFAULT", "BACK TO MENU"
    };
    // 各项当前值的初始占位（2-6/11 是实时刷新项，其余保持占位显示）
    const char* const SETTINGS_DEFAULT[14] = {
        "80", "80", "", "", "", "", "",
        "--", "--", "--", "--", "--", "--", ""
    };
}

void SettingsMenu::init(const Font& font) {
    m_title.setFont(font);
    m_title.setCharacterSize(72);        // 标题字号缩小，避免拥挤
    m_title.setFillColor(Color::White);
    alignRow(m_title, config::SCREEN_CENTER_X, 110.0f, 0);

    m_back.setFont(font);
    m_back.setCharacterSize(36);
    m_back.setFillColor(Color::White);
    alignRow(m_back, config::SCREEN_CENTER_X, 1030.0f, 0);

    // 布局：字号 38、行距 55、首项 y=200（正文上移，底部留白不贴边）
    for (int i = 0; i < 14; ++i) {
        const float y = 200.0f + i * 55.0f;
        m_options[i].setFont(font);
        m_options[i].setCharacterSize(38);
        m_options[i].setFillColor(Color::White);
        alignRow(m_options[i], 600.0f, y, 1);

        m_values[i].setFont(font);
        m_values[i].setString(SETTINGS_DEFAULT[i]);   // 未接线的项显示占位（2-6/11 由 refreshValues 每帧刷）
        m_values[i].setCharacterSize(38);
        m_values[i].setFillColor(Color::White);
        alignRow(m_values[i], 1300.0f, y, 2);
    }
    applyLanguage();   // 标题/提示/14 项标签文字按当前语言填写（值由 refreshValues 每帧刷）
}

// 语言切换时刷新本界面文字（标题/操作提示/14 项标签；值文本由 refreshValues 每帧刷新）
void SettingsMenu::applyLanguage() {
    m_title.setString(tr("设置", "SETTINGS"));
    alignRow(m_title, config::SCREEN_CENTER_X, 110.0f, 0);
    m_back.setString(tr("上下: 选择    左右: 调整\n回车: 确认    ESC: 返回",
                        "UP/DOWN: SELECT    LEFT/RIGHT: CHANGE\nENTER: CONFIRM     ESC: BACK"));
    alignRow(m_back, config::SCREEN_CENTER_X, 1030.0f, 0);
    for (int i = 0; i < 14; ++i) {
        m_options[i].setString(tr(ZH_LABELS[i], EN_LABELS[i]));
        alignRow(m_options[i], 600.0f, 200.0f + i * 55.0f, 1);
    }
}

// 切换语言：改全局语言并写盘（其余界面文字由 Game 检测语言变化后统一刷新）
void SettingsMenu::cycleLanguage() {
    config::g_lang = (config::g_lang == config::Lang::Zh)
                   ? config::Lang::En : config::Lang::Zh;
    if (m_disp) m_disp->saveSettings();   // 语言持久化到 settings.txt
}

// 进入设置界面：记录来源（回主菜单还是暂停菜单），选中回到第一项
void SettingsMenu::open(bool fromPause) {
    m_active = true;
    m_returnToPause = fromPause;
    m_selection = 0;
    m_inputHeld = m_enterHeld = false;
    m_modeHeld = m_resHeld = m_vsyncHeld = m_fpsHeld = m_showFpsHeld = false;
    m_langHeld = false;
}

// 每帧按键处理：上下选、左右改值、Esc/Enter 返回；返回 true 表示本帧请求关闭
bool SettingsMenu::handleInput(bool up, bool down, bool left, bool right,
                               bool enter, bool escape) {
    refreshValues();  // 每帧刷新五项可调值的当前显示

    // 上下选择（循环 0-13）
    if ((up || down) && !m_inputHeld) {
        m_inputHeld = true;
        if (up)   m_selection = (m_selection + 13) % 14; // 上移（-1 mod 14）
        if (down) m_selection = (m_selection + 1) % 14;  // 下移
    } else if (!up && !down) {
        m_inputHeld = false;
    }

    // 左右调整当前选中项的值（边沿检测防连跳）
    if (left || right) {
        if (m_selection == 2 && !m_modeHeld)  { m_modeHeld  = true; m_disp->cycleMode(); }
        else if (m_selection == 3 && !m_resHeld)   { m_resHeld   = true; m_disp->cycleResolution(); }
        else if (m_selection == 4 && !m_vsyncHeld) { m_vsyncHeld = true; m_disp->cycleVsync(); }
        else if (m_selection == 5 && !m_fpsHeld)   { m_fpsHeld   = true; m_disp->cycleFramerate(); }
        else if (m_selection == 6 && !m_showFpsHeld) { m_showFpsHeld = true; m_disp->cycleShowFps(); }
        else if (m_selection == 11 && !m_langHeld) { m_langHeld = true; cycleLanguage(); }
    } else {
        m_modeHeld = m_resHeld = m_vsyncHeld = m_fpsHeld = m_showFpsHeld = false;
        m_langHeld = false;
    }

    // 返回：Esc 随时可返回；BACK TO MENU(13) 项选中后按 Enter 也可返回（按来源回主/暂停菜单）
    if (escape || (enter && m_selection == 13)) {
        if (!m_enterHeld) {
            m_enterHeld = true;
            m_active = false;   // 关闭设置界面（来源由 m_returnToPause 记录）
            return true;
        }
    } else {
        m_enterHeld = false;
    }
    return false;
}

// 刷新五项可调值的当前显示（值文本变了要重新右对齐，setString 不会自动更新 origin）
void SettingsMenu::refreshValues() {
    m_values[2].setString(
        m_disp->displayMode() == DisplayMode::Fullscreen  ? tr("全屏", "FULLSCREEN")
      : m_disp->displayMode() == DisplayMode::Borderless ? tr("无边框", "BORDERLESS")
      : tr("窗口化", "WINDOWED"));
    if (m_disp->displayMode() == DisplayMode::Borderless) {
        m_values[3].setString(tr("系统", "SYSTEM"));   // 无边框分辨率固定，不可调
    } else if (m_disp->resIndex() == 0) {
        m_values[3].setString(tr("自动", "AUTO"));
    } else {
        const auto r = m_disp->commonResolution(m_disp->resIndex());
        m_values[3].setString(std::to_string(r.first) + "x" + std::to_string(r.second));
    }
    m_values[4].setString(m_disp->vsyncEnabled() ? tr("开", "ON") : tr("关", "OFF"));
    const int fi = (m_disp->framerateIndex() >= 0 && m_disp->framerateIndex() < config::FRAMERATE_COUNT)
                 ? m_disp->framerateIndex() : 0;
    const char* fn = config::FRAMERATE_NAMES[fi];   // AUTO / UNLIMITED 做双语，数字档不变
    sf::String fpsName;
    if (std::string(fn) == "UNLIMITED") fpsName = tr("不限", "UNLIMITED");
    else if (std::string(fn) == "AUTO") fpsName = tr("自动", "AUTO");
    else fpsName = sf::String(std::string(fn));
    m_values[5].setString(fpsName);
    m_values[6].setString(m_disp->showFps() ? tr("开", "ON") : tr("关", "OFF"));
    m_values[11].setString(tr("中文", "English"));
    // 布局用 200 + idx*55
    for (int i = 2; i <= 6; ++i)
        alignRow(m_values[i], 1300.0f, 200.0f + i * 55.0f, 2);
    alignRow(m_values[11], 1300.0f, 200.0f + 11 * 55.0f, 2);
}

// 绘制界面
void SettingsMenu::draw(RenderWindow& window) {
    window.draw(m_title);
    for (int i = 0; i < 14; ++i) {
        m_options[i].setFillColor(i == m_selection ? Color::Yellow : Color::White);
        window.draw(m_options[i]);
        window.draw(m_values[i]);
    }
    window.draw(m_back);
}

// 行内对齐辅助（0居中 1左 2右）
void SettingsMenu::alignRow(Text& t, float x, float y, int mode) {
    const FloatRect r = t.getLocalBounds();
    if (mode == 2)      t.setOrigin(r.left + r.width, r.top + r.height / 2.0f); // 右对齐
    else if (mode == 1) t.setOrigin(r.left, r.top + r.height / 2.0f);          // 左对齐
    else                t.setOrigin(r.left + r.width / 2.0f, r.top + r.height / 2.0f); // 居中
    t.setPosition(x, y);
}
