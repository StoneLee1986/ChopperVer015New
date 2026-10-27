// ============================================================================
//  NameEntryUI 类：最高分名字输入界面
// ============================================================================
//  负责"进入新纪录后输入名字"的界面与交互：字母网格（A-Z+0-9）+ SKIP/DONE，
//  支持方向键导航、直接打字、BackSpace 删除、Enter/Esc 提交。
//  提交时把成绩写入 Scoreboard（注入引用）并进入榜单界面；Game 检测 hasCommitted()
//  后负责收尾（退出游戏画面状态）。所有边沿检测内部持有，不依赖 Game 的共享标志。
// ============================================================================
#ifndef CHOPPER_NAMEENTRYUI_H
#define CHOPPER_NAMEENTRYUI_H

#include "Scoreboard.h"

class NameEntryUI {
public:
    static constexpr const char* NAME_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    void init(const Font& font);                 // 建 UI 文字
    void attachScoreboard(Scoreboard& sb) { m_sb = &sb; } // 注入榜单（提交时写入）
    void begin(int score);                       // 进入名字输入（清名字、cell=0）
    bool isActive() const { return m_active; }
    bool hasCommitted() const { return m_committed; } // 本局是否已提交（Game 收尾用）
    void clearCommitted() { m_committed = false; }
    void appendChar(wchar_t ch);                 // 直接打字追加（TextEntered 事件）
    void handleInput(bool up, bool down, bool left, bool right,
                     bool enter, bool backspace, bool escape); // 每帧处理按键
    void refresh();                              // 刷新名字预览文字
    void draw(RenderWindow& window);             // 绘制界面
    void applyLanguage();                         // 语言切换时刷新标题/SKIP/DONE
private:
    void submit(const std::string& name);        // 提交：写榜单 + 进榜单界面
    void moveCell(int dc, int dr);               // 网格导航
    void center(Text& t, float x, float y);      // 居中辅助

    Scoreboard* m_sb = nullptr;
    bool m_active = false;
    bool m_committed = false;
    int  m_score = 0;                            // 本局分数（提交用）
    std::string m_playerName;                    // 当前输入的名字
    int  m_cell = 0;                             // 高亮格（0-35 字母数字，36=SKIP，37=DONE）
    bool m_moveHeld = false;                     // 方向键边沿
    bool m_backHeld = false;                     // BackSpace 边沿
    bool m_enterHeld = false;                    // Enter 边沿
    bool m_escHeld   = false;                    // Esc 边沿
    Text m_title, m_preview;                     // 标题 + 名字预览
    Text m_grid[36];                             // 36 格字母/数字
    Text m_skip, m_done;                         // SKIP / DONE 功能项
};

#endif
