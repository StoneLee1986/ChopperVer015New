// CHOPPER - NameEntryUI implementation
#include "NameEntryUI.h"

// ============================================================================
//  NameEntryUI 实现：名字输入界面
// ============================================================================
void NameEntryUI::init(const Font& font) {
    m_title.setFont(font);
    m_title.setCharacterSize(80);
    m_title.setFillColor(Color::White);
    center(m_title, config::SCREEN_CENTER_X, 140.0f);

    m_preview.setFont(font);
    m_preview.setCharacterSize(60);
    m_preview.setFillColor(Color::Yellow);
    m_preview.setString("");

    // 36 格字母数字网格（A-Z + 0-9），6 列 x 6 行
    for (int i = 0; i < 36; ++i) {
        m_grid[i].setFont(font);
        m_grid[i].setString(std::string(1, NAME_CHARS[i]));
        m_grid[i].setCharacterSize(50);
        m_grid[i].setFillColor(Color::White);
        center(m_grid[i], config::SCREEN_CENTER_X - 175.0f + (i % 6) * 70.0f,
                               350.0f + (i / 6) * 75.0f);
    }
    // SKIP / DONE（网格下方一行，左右拉开避免重叠）
    m_skip.setFont(font);
    m_skip.setCharacterSize(50);
    m_skip.setFillColor(Color::White);
    center(m_skip, config::SCREEN_CENTER_X - 320.0f, 800.0f);
    m_done.setFont(font);
    m_done.setCharacterSize(50);
    m_done.setFillColor(Color::White);
    center(m_done, config::SCREEN_CENTER_X + 320.0f, 800.0f);
    applyLanguage();   // 标题/SKIP/DONE 文字按当前语言填写
}

// 语言切换时刷新标题/SKIP/DONE（字母数字网格不译）
void NameEntryUI::applyLanguage() {
    m_title.setString(tr("新纪录！", "NEW HIGH SCORE!"));
    center(m_title, config::SCREEN_CENTER_X, 140.0f);
    m_skip.setString(tr("跳过", "SKIP"));
    center(m_skip, config::SCREEN_CENTER_X - 320.0f, 800.0f);
    m_done.setString(tr("完成", "DONE"));
    center(m_done, config::SCREEN_CENTER_X + 320.0f, 800.0f);
}

// 进入名字输入：清名字、高亮回到第一格，各边沿标记复位
void NameEntryUI::begin(int score) {
    m_active = true;
    m_committed = false;
    m_score = score;
    m_playerName.clear();
    m_cell = 0;
    m_moveHeld = m_backHeld = m_enterHeld = m_escHeld = false;
    refresh();
}

// 直接打字追加字母/数字（TextEntered 事件），超长截断
void NameEntryUI::appendChar(wchar_t ch) {
    if (!m_active) return;
    if ((ch >= L'A' && ch <= L'Z') || (ch >= L'a' && ch <= L'z') ||
        (ch >= L'0' && ch <= L'9')) {
        if (m_playerName.size() < static_cast<size_t>(config::NAME_MAX_LEN))
            m_playerName += static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
}

// 每帧按键处理：方向导航 / Enter / BackSpace / Esc（边沿检测内部持有）
void NameEntryUI::handleInput(bool up, bool down, bool left, bool right,
                              bool enter, bool backspace, bool escape) {
    refresh();  // 每帧刷新名字预览

    // 方向键在网格中移动
    if ((up || down || left || right) && !m_moveHeld) {
        m_moveHeld = true;
        moveCell(right ? 1 : (left ? -1 : 0),
                 down ? 1 : (up ? -1 : 0));
    } else if (!up && !down && !left && !right) {
        m_moveHeld = false;
    }

    // Enter：名字为空时从当前格选第一个字母；名字非空时确认提交
    // （符合"输入完名字后按 Enter 确认"的直觉习惯）；SKIP/DONE 功能格仍有效
    if (enter) {
        if (!m_enterHeld) {
            m_enterHeld = true;
            if (m_cell == 36)      submit("PLAYER");     // SKIP → 默认名
            else if (m_cell == 37) submit(m_playerName); // DONE → 提交
            else if (m_playerName.empty())
                m_playerName += NAME_CHARS[m_cell];      // 名字为空时选字母
            else
                submit(m_playerName);                    // 名字非空 → Enter 确认
        }
    } else {
        m_enterHeld = false;
    }

    // BackSpace：删除最后一个字母
    if (backspace) {
        if (!m_backHeld) {
            m_backHeld = true;
            if (!m_playerName.empty()) m_playerName.pop_back();
        }
    } else {
        m_backHeld = false;
    }

    // Esc：确认提交当前输入的名字（空名则用默认 PLAYER）
    if (escape) {
        if (!m_escHeld) {
            m_escHeld = true;
            submit(m_playerName);
        }
    } else {
        m_escHeld = false;
    }
}

// 刷新名字预览文字
void NameEntryUI::refresh() {
    m_preview.setString(m_playerName);
    center(m_preview, config::SCREEN_CENTER_X, 250.0f);
}

// 绘制界面
void NameEntryUI::draw(RenderWindow& window) {
    window.draw(m_title);
    window.draw(m_preview);
    for (int i = 0; i < 36; ++i) {
        m_grid[i].setFillColor(i == m_cell ? Color::Yellow : Color::White);
        window.draw(m_grid[i]);
    }
    m_skip.setFillColor(m_cell == 36 ? Color::Yellow : Color::White);
    m_done.setFillColor(m_cell == 37 ? Color::Yellow : Color::White);
    window.draw(m_skip);
    window.draw(m_done);
}

// 提交：把成绩写入榜单并进入榜单界面，置 committed 供 Game 收尾
void NameEntryUI::submit(const std::string& name) {
    m_sb->addScore(name, m_score);   // 写入榜单（内部保存）
    m_active = false;                // 退出名字输入界面
    m_committed = true;              // 通知 Game：本次已提交，请收尾
}

// 名字网格方向键导航：6x6 字母区 + 底部 SKIP/DONE 一行
//  cell 0..35 = A-Z + 0-9；36 = SKIP；37 = DONE
void NameEntryUI::moveCell(int dc, int dr) {
    int row = m_cell / 6;   // 0..6（第 6 行是 SKIP/DONE）
    int col = m_cell % 6;   // 0..5
    // 垂直移动（顶行向上 → 到底部功能行；底部向下 → 回顶行）
    if (dr != 0) {
        row += dr;
        if (row < 0) row = 6;
        if (row > 6) row = 0;
    }
    // 水平移动（每行内循环）
    if (dc != 0) {
        col += dc;
        if (col < 0) col = 5;
        if (col > 5) col = 0;
    }
    // 功能行只有 0(SKIP)/1(DONE) 两格，落到无效列时归到有效格
    if (row == 6) {
        if (col >= 2) col = 1;
    }
    m_cell = row * 6 + col;
}

// 居中辅助
void NameEntryUI::center(Text& t, float x, float y) {
    const FloatRect b = t.getLocalBounds();
    t.setOrigin(b.left + b.width / 2.0f, b.top + b.height / 2.0f);
    t.setPosition(x, y);
}
