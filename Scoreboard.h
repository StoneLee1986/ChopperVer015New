// ============================================================================
//  Scoreboard 类：分数记录、持久化与榜单显示
// ============================================================================
//  负责最高分榜单的存储（最多 HIGHSCORE_MAX 条，降序）、读写存档文件、
//  判断一局分数是否上榜、榜单界面的绘制。名字输入见 NameEntryUI。
//  提交时通过 addScore() 写入，然后用 open() 进入榜单界面。
// ============================================================================
#ifndef CHOPPER_SCOREBOARD_H
#define CHOPPER_SCOREBOARD_H

#include "config.h"

// 一条最高分记录：名字 + 分数
struct ScoreEntry {
    std::string name;
    int         score;
};

class Scoreboard {
public:
    void init(const Font& font);      // 建榜单 UI 文字 + 读存档
    void load();                      // 从存档文件读榜单
    void save() const;                // 把榜单写回文件（保留前 HIGHSCORE_MAX）
    bool qualifiesForHighscore(int score) const; // 本局分数是否够上榜
    void addScore(const std::string& name, int score); // 插入榜单并保存
    void clearAll();                  // 清空所有记录并保存
    bool isOpen() const { return m_isOpen; }      // 是否显示榜单界面
    void open() { load(); refreshDisplay(); m_isOpen = true; } // 进榜单：读档 + 填充 + 显示
    void close() { m_isOpen = false; }            // 退出榜单界面
    void refreshDisplay();            // 把榜单数据填进三列并各自对齐
    void draw(RenderWindow& window);  // 绘制榜单界面
    void applyLanguage();             // 语言切换时刷新标题/返回提示

private:
    void center(Text& t, float x, float y);            // 文字中心定位
    void alignRow(Text& t, float x, float y, int mode); // 行内对齐：0居中 1左 2右

    std::vector<ScoreEntry> m_scores;   // 榜单（保持降序，最多 HIGHSCORE_MAX 条）
    bool m_isOpen = false;              // 是否显示榜单界面
    Text  m_hsTitle;                    // 最高分菜单标题：HIGH SCORES
    Text  m_hsBack;                     // 返回/清除提示
    Text  m_hsRank[config::HIGHSCORE_MAX];   // 榜单：序号列
    Text  m_hsNames[config::HIGHSCORE_MAX];  // 榜单：名字列
    Text  m_hsScores[config::HIGHSCORE_MAX]; // 榜单：分数列
};

#endif
