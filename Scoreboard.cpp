// CHOPPER - Scoreboard implementation
#include "Scoreboard.h"

// ============================================================================
//  Scoreboard 实现：初始化 + 榜单读写 + 上榜判定 + 插入 + 清除 + 显示刷新
// ============================================================================
//  初始化：建榜单 UI 文字 + 读存档
void Scoreboard::init(const Font& font) {
    m_hsTitle.setFont(font);
    m_hsTitle.setCharacterSize(100);
    m_hsTitle.setFillColor(Color::White);
    center(m_hsTitle, config::SCREEN_CENTER_X, 120.0f);

    m_hsBack.setFont(font);
    m_hsBack.setCharacterSize(40);
    m_hsBack.setFillColor(Color::White);
    center(m_hsBack, config::SCREEN_CENTER_X, 1000.0f);

    for (int i = 0; i < config::HIGHSCORE_MAX; ++i) {
        m_hsRank[i].setFont(font);
        m_hsRank[i].setCharacterSize(40);
        m_hsRank[i].setFillColor(Color::White);
        m_hsNames[i].setFont(font);
        m_hsNames[i].setCharacterSize(40);
        m_hsNames[i].setFillColor(Color::White);
        m_hsScores[i].setFont(font);
        m_hsScores[i].setCharacterSize(40);
        m_hsScores[i].setFillColor(Color::White);
    }
    applyLanguage();   // 标题/返回提示按当前语言填写
    load();   // 读历史最高分（启动即加载，供上榜判定用）
}

// 语言切换时刷新标题/返回提示（榜单数据文本不译）
void Scoreboard::applyLanguage() {
    m_hsTitle.setString(tr("最高分", "HIGH SCORES"));
    center(m_hsTitle, config::SCREEN_CENTER_X, 120.0f);
    m_hsBack.setString(tr("ESC: 返回     |    C: 清空全部", "ESC: BACK    |    C: CLEAR ALL"));
    center(m_hsBack, config::SCREEN_CENTER_X, 1000.0f);
}

//  从存档文件读榜单到 m_scores（保持降序）
void Scoreboard::load() {
    m_scores.clear();
    std::ifstream in(config::ASSET_HIGHSCORE);
    std::string name;
    int score;
    while (in >> name >> score) {          // 每行 "名字 分数"
        m_scores.push_back({ name, score });
    }
    std::sort(m_scores.begin(), m_scores.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
}

//  把榜单写回文件（只保留前 HIGHSCORE_MAX 名，降序）
void Scoreboard::save() const {
    std::vector<ScoreEntry> s = m_scores;
    std::sort(s.begin(), s.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (s.size() > static_cast<size_t>(config::HIGHSCORE_MAX))
        s.resize(config::HIGHSCORE_MAX);
    std::ofstream out(config::ASSET_HIGHSCORE);
    for (const ScoreEntry& e : s)
        out << e.name << " " << e.score << "\n";
}

//  本局分数是否够进入前 HIGHSCORE_MAX 榜单
bool Scoreboard::qualifiesForHighscore(int score) const {
    if (score <= 0) return false;                    // 0 分不入榜
    if (m_scores.size() < static_cast<size_t>(config::HIGHSCORE_MAX))
        return true;                                 // 榜单未满 → 直接上榜
    return score > m_scores.back().score;            // 满了则要超过最低分
}

//  把一条成绩插入榜单并保存（名字去空白、截断到 NAME_MAX_LEN 字母）
void Scoreboard::addScore(const std::string& name, int score) {
    std::string n = name;
    while (!n.empty() && (n.front() == ' ' || n.front() == '\t')) n.erase(0, 1);   // 去开头空白
    while (!n.empty() && (n.back() == ' ' || n.back() == '\t'))   n.pop_back();    // 去结尾空白
    if (n.empty()) n = "PLAYER";                     // 空名用默认
    if (n.size() > static_cast<size_t>(config::NAME_MAX_LEN))
        n = n.substr(0, config::NAME_MAX_LEN);       // 截断
    m_scores.push_back({ n, score });
    std::sort(m_scores.begin(), m_scores.end(),
              [](const ScoreEntry& a, const ScoreEntry& b) { return a.score > b.score; });
    if (m_scores.size() > static_cast<size_t>(config::HIGHSCORE_MAX))
        m_scores.resize(config::HIGHSCORE_MAX);
    save();
}

//  清空所有分数记录并写回文件（空榜单）
void Scoreboard::clearAll() {
    m_scores.clear();
    save();              // 写空文件
    refreshDisplay();    // 榜单界面显示为空
}

//  把榜单数据填进三列显示，并各自对齐（序号居中 / 名字左对齐 / 分数右对齐）
void Scoreboard::refreshDisplay() {
    for (int i = 0; i < config::HIGHSCORE_MAX; ++i) {
        if (i < static_cast<int>(m_scores.size())) {
            m_hsRank[i].setString(std::to_string(i + 1) + ".");
            m_hsNames[i].setString(m_scores[i].name);
            m_hsScores[i].setString(std::to_string(m_scores[i].score));
        } else {
            m_hsRank[i].setString("");
            m_hsNames[i].setString("");
            m_hsScores[i].setString("");
        }
        const float y = 200.0f + i * 38.0f;
        alignRow(m_hsRank[i],   620.0f, y, 0);   // 序号列：居中
        alignRow(m_hsNames[i],  760.0f, y, 1);   // 名字列：左对齐
        alignRow(m_hsScores[i], 1280.0f, y, 2);  // 分数列：右对齐
    }
}

//  绘制榜单界面（标题 + 三列 + 返回/清除提示）
void Scoreboard::draw(RenderWindow& window) {
    window.draw(m_hsTitle);
    for (int i = 0; i < config::HIGHSCORE_MAX; ++i) {
        window.draw(m_hsRank[i]);
        window.draw(m_hsNames[i]);
        window.draw(m_hsScores[i]);
    }
    window.draw(m_hsBack);
}

//  行内对齐辅助：mode 0=居中 1=左对齐 2=右对齐，垂直均居中
void Scoreboard::alignRow(Text& t, float x, float y, int mode) {
    const FloatRect r = t.getLocalBounds();
    if (mode == 2)      t.setOrigin(r.left + r.width, r.top + r.height / 2.0f); // 右对齐
    else if (mode == 1) t.setOrigin(r.left, r.top + r.height / 2.0f);          // 左对齐
    else                t.setOrigin(r.left + r.width / 2.0f, r.top + r.height / 2.0f); // 居中
    t.setPosition(x, y);
}

//  居中辅助（榜单标题/返回提示定位用）
void Scoreboard::center(Text& t, float x, float y) {
    const FloatRect r = t.getLocalBounds();
    t.setOrigin(r.left + r.width / 2.0f, r.top + r.height / 2.0f);
    t.setPosition(x, y);
}
