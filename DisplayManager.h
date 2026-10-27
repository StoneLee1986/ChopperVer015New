// ============================================================================
//  DisplayManager 类：窗口、显示模式、分辨率与配置持久化
// ============================================================================
//  负责创建/重建游戏窗口、管理显示模式（全屏/无边框/窗口化）与分辨率候选、
//  垂直同步与帧率档位、内置 FPS 显示开关，并把整套设置读写到 settings.txt。
//  窗口与视图归它所有，Game 通过 window()/view() 访问；绘制的 1920x1080 逻辑坐标不变。
// ============================================================================
#ifndef CHOPPER_DISPLAYMANAGER_H
#define CHOPPER_DISPLAYMANAGER_H

#include "config.h"

enum class DisplayMode { Fullscreen, Borderless, Windowed }; // 全屏 / 无边框 / 窗口化

class DisplayManager {
public:
    void init();   // 读配置 + 建窗（构造函数里调用一次）

    RenderWindow& window() { return m_window; }   // 供主循环轮询/绘制
    View&         view()   { return m_view; }     // 1920x1080 逻辑坐标视图（分辨率自适应）

    // 状态读取（设置界面显示 / 渲染判断用）
    DisplayMode displayMode() const { return m_displayMode; }
    int  resIndex() const { return m_resIndex; }
    VideoMode resolution() const { return m_resolution; }
    bool vsyncEnabled() const { return m_vsyncEnabled; }
    int  framerateIndex() const { return m_framerateIndex; }
    bool showFps() const { return m_showFps; }
    std::pair<unsigned int, unsigned int> commonResolution(int idx) const; // 候选下标→宽高

    // 设置界面：切换显示参数，立即生效并写回配置
    void cycleMode();
    void cycleResolution();
    void cycleVsync();
    void cycleFramerate();
    void cycleShowFps();
    void saveSettings();     // 写配置文件（MODE+RESOLUTION+VSYNC+FRAMERATE+SHOWFPS+LANG）；语言切换也写盘
    void updateViewport();   // 窗口尺寸变化时重算视图，使游戏适配各种分辨率

private:
    void loadSettings();     // 读配置文件，缺失/非法回退默认
    void applyDisplay();     // 按当前模式建窗：选Style/尺寸→校验回退→create→重算视图→保存
    void applySyncSettings();// 应用垂直同步/帧率上限（create 会重置二者，重建后须重调）
    bool resolutionSupported(unsigned int w, unsigned int h) const; // 该分辨率是否被当前显示器全屏支持

    RenderWindow m_window;   // 游戏窗口
    View m_view;             // 视图：把内部 1920x1080 逻辑坐标映射到实际窗口（分辨率自适应）
    DisplayMode m_displayMode = DisplayMode::Fullscreen; // 当前显示模式，默认全屏
    VideoMode m_resolution;   // 当前生效的分辨率 / 窗口尺寸
    int  m_resIndex = 0;      // 手动分辨率候选下标，0 = AUTO（跟随桌面）
    bool m_vsyncEnabled = true;   // 垂直同步开关，默认开（玩家可调）
    int  m_framerateIndex = 0;    // 帧率档位下标，默认 AUTO
    bool m_showFps = true;      // 内置 FPS 显示开关，默认开（验证高刷新率）
};

#endif
