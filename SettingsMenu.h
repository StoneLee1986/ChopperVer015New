// ============================================================================
//  SettingsMenu 类：设置界面
// ============================================================================
//  负责设置界面的显示与交互：14 个设置项（上下选择、左右调值、Esc/Enter 返回），
//  其中 DISPLAY MODE/RESOLUTION/V-SYNC/FRAME RATE/SHOW FPS 五项经注入的
//  DisplayManager 即时改值并写回配置。边沿检测全部内部持有，不依赖 Game。
//  返回时通过 returnToPause() 告知 Game 应从哪来回（主菜单或暂停菜单）。
// ============================================================================
#ifndef CHOPPER_SETTINGSMENU_H
#define CHOPPER_SETTINGSMENU_H

#include "DisplayManager.h"

class SettingsMenu {
public:
    void init(const Font& font);                  // 建 UI 文字
    void attachDisplay(DisplayManager& d) { m_disp = &d; } // 注入显示管理器（改值用）
    void open(bool fromPause);                    // 进入设置界面（记录来源）
    bool isOpen() const { return m_active; }
    bool returnToPause() const { return m_returnToPause; } // 来源：true=暂停菜单，false=主菜单
    bool handleInput(bool up, bool down, bool left, bool right,
                     bool enter, bool escape);    // 每帧按键；返回 true 表示本帧请求关闭
    void draw(RenderWindow& window);              // 绘制界面
    void applyLanguage();                         // 语言切换时刷新本界面文字（标题/提示/14 项标签）
private:
    void refreshValues();                         // 刷新五项可调值的当前显示
    void cycleLanguage();                         // 切换语言并写盘
    void alignRow(Text& t, float x, float y, int mode); // 行内对齐（0居中 1左 2右）

    DisplayManager* m_disp = nullptr;
    bool m_active = false;
    bool m_returnToPause = false;                 // 从哪来，回哪去
    int  m_selection = 0;                         // 当前选中项（0-13，末项 BACK TO MENU）
    bool m_inputHeld = false;                     // 上下键边沿
    bool m_enterHeld = false;                     // Enter/Esc 返回边沿
    bool m_modeHeld = false, m_resHeld = false, m_vsyncHeld = false,
         m_fpsHeld = false, m_showFpsHeld = false, m_langHeld = false,
         m_assistHeld = false; // 左右改值边沿（防连跳）
    Text m_title, m_back;                         // 标题 + 操作提示
    Text m_options[15], m_values[15];             // 15 项：标签 + 右侧值（14 = 返回菜单）
};

#endif
