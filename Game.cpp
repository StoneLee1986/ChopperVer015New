// CHOPPER - Game implementation
#include "Game.h"
#include "bgm_songs.h"
#include <SFML/Audio.hpp>
#include <cstdio>
#include <cstdlib>

namespace {
// 帧耗时日志的绝对路径：写到用户主目录，不依赖进程工作目录（VS 启动时工作目录不确定）
std::string perfLogPath() {
    char*  p = nullptr;
    size_t n = 0;
    _dupenv_s(&p, &n, "USERPROFILE");
    std::string s = std::string(p ? p : ".") + "\\chopper_perf.log";
    if (p) free(p);
    return s;
}
} // namespace

Game::Game()
    : m_levelRhythm(*this),        // 关卡 A · 021 砍树驱动
      m_levelBeat(*this),          // 关卡 B · 020 鼓点驱动
      m_rng(std::random_device{}()) {   // 用真随机种子初始化随机引擎

    m_disp.init();     // 读配置 + 建窗（窗口/视图/显示状态全在 DisplayManager）
    // 垂直同步/帧率已由 applyDisplay 内的 applySyncSettings() 按玩家设置应用，此处不再强制开垂直同步

    loadAssets();   // 加载所有图片、字体、音效
    initScene();    // 把每个精灵摆好位置

    // 配置时间条的外观和位置
    m_timeBar.setSize(Vector2f(config::TIME_BAR_WIDTH, config::TIME_BAR_HEIGHT));
    m_timeBar.setFillColor(Color::Red);
    m_timeBar.setPosition(config::SCREEN_CENTER_X - config::TIME_BAR_WIDTH / 2.0f,
                          config::TIME_BAR_Y);

    // 配置中央提示文字（死亡/超时等画面用）
    m_messageText.setFont(m_font);
    m_messageText.setCharacterSize(75);
    m_messageText.setFillColor(Color::White);

    // ---- 初始化菜单（纯黑背景 + 英文文字，字体不支持中文） ----
    m_menuTitle.setFont(m_font);
    m_menuTitle.setString("CHOPPER");        // 主菜单标题
    m_menuTitle.setCharacterSize(150);
    m_menuTitle.setFillColor(Color::White);
    centerText(m_menuTitle, config::SCREEN_CENTER_X, 300.0f);

    // 两个菜单共用同一个选项数组：先统一样式，再按当前菜单填文字
    for (int i = 0; i < 5; ++i) {
        m_menuOptions[i].setFont(m_font);
        m_menuOptions[i].setCharacterSize(60);
        m_menuOptions[i].setFillColor(Color::White);
    }
    setupMenu(MenuKind::Main);   // 开局显示主菜单
    switchBgm(BgmState::Menu);   // 主菜单 BGM：一进游戏就有音乐

    // ---- 退出二次确认界面 ----
    m_confirmTitle.setFont(m_font);
    m_confirmTitle.setString(tr("确定退出游戏？", "EXIT GAME?"));
    m_confirmTitle.setCharacterSize(100);
    m_confirmTitle.setFillColor(Color::White);
    centerText(m_confirmTitle, config::SCREEN_CENTER_X, 400.0f);

    sf::String confirmLabels[2] = { tr("否", "NO"), tr("是", "YES") };   // 默认选 NO，防误触
    for (int i = 0; i < 2; ++i) {
        m_confirmOptions[i].setFont(m_font);
        m_confirmOptions[i].setString(confirmLabels[i]);
        m_confirmOptions[i].setCharacterSize(60);
        m_confirmOptions[i].setFillColor(Color::White);
        centerText(m_confirmOptions[i], config::SCREEN_CENTER_X - 150.0f + i * 300.0f, 550.0f);
    }

    m_scoreboard.init(m_font);   // 建榜单 UI 文字 + 读取历史最高分（启动即加载，供上榜判定用）

    // ---- 名字输入界面（内聚到 NameEntryUI） ----
    m_nameUI.init(m_font);                   // 建 UI 文字（标题/预览/网格/SKIP/DONE）
    m_nameUI.attachScoreboard(m_scoreboard); // 提交时写入榜单

    // ---- 设置界面（内聚到 SettingsMenu） ----
    m_settingsMenu.init(m_font);                   // 建 UI 文字（标题/提示/14 项标签与值）
    m_settingsMenu.attachDisplay(m_disp);          // 即时改值 + 写回配置

    // 配置左上角分数文字
    m_scoreText.setFont(m_font);
    m_scoreText.setCharacterSize(100);
    m_scoreText.setFillColor(Color::White);
    m_scoreText.setPosition(20.0f, 20.0f);
    m_scoreText.setString(tr("得分 = ", "Score = ") + std::string("0"));

    // 右上角内置 FPS 显示（默认开启，供验证高刷新率；游戏完成后再考虑去掉）
    m_fpsText.setFont(m_font);
    m_fpsText.setString("FPS: --");
    m_fpsText.setCharacterSize(28);
    m_fpsText.setFillColor(Color::Green);
    alignRow(m_fpsText, config::SCREEN_WIDTH - 20.0f, 20.0f, 2);
}

// ============================================================================
//  主循环：一直跑到关窗
// ============================================================================
void Game::run() {
    // 首帧先清黑并上屏：消除窗口创建后、主循环首帧渲染前 Windows 显示的默认白色背景（白屏闪现）
    m_disp.window().clear(sf::Color::Black);
    m_disp.window().display();
    if (config::PERF_LOG) {   // 每次启动写一行标记，便于确认"这一局确实跑的是新版本"
        std::ofstream f(perfLogPath(), std::ios::app);
        if (f) f << "--- run start (build " << __DATE__ << " " << __TIME__ << ") ---\n";
    }
    while (m_disp.window().isOpen()) {
        processEvents();
        // 游戏结束画面停留片刻后，再进入名字输入（给玩家一点反应时间）
        if (m_enterNamePending &&
            m_gameOverClock.getElapsedTime().asSeconds() >= config::GAME_OVER_DELAY) {
            m_enterNamePending = false;
            // 统一结算：上榜→先输入名字；不上榜→直接跳到高分榜（所有结局一致）
            if (m_scoreboard.qualifiesForHighscore(m_level->score()))
                m_nameUI.begin(m_level->score());   // 携带本局分数上榜
            else
                m_scoreboard.open();                // 不上榜也统一跳高分榜
        }
        const Time dt = m_clock.restart();  // 每帧都取耗时：暂停时特效能继续演完
        // ---- 帧耗时统计：每 2 秒一行写进 perf.log（判断卡顿是引擎还是环境） ----
        if (config::PERF_LOG) {
            const float ms = dt.asSeconds() * 1000.0f;
            m_perfAccum += ms;
            ++m_perfFrames;
            if (ms > m_perfMax) m_perfMax = ms;
            if (m_perfAccum >= 2000.0f) {
                std::ofstream f(perfLogPath(), std::ios::app);
                if (f) f << "avg=" << (m_perfAccum / static_cast<float>(m_perfFrames))
                         << "ms max=" << m_perfMax
                         << "ms frames=" << m_perfFrames
                         << " pause=" << (m_paused ? 1 : 0) << "\n";
                m_perfAccum = 0.0f; m_perfMax = 0.0f; m_perfFrames = 0;
            }
        }
        if (!m_paused && !m_escPaused && !m_inMenu && !m_inPauseMenu && !m_confirmExit
            && !m_scoreboard.isOpen() && !m_nameUI.isActive() && !m_settingsMenu.isOpen()) {  // 不在任何菜单/输入/确认、没暂停才更新
            update(dt);
        } else {
            m_fx.update(dt.asSeconds());   // 结算画面/暂停菜单里：死亡爆发、慢动作继续播
        }
        checkLanguage();   // 每次渲染前：语言切换实时刷新所有界面（含菜单/设置/榜单等非 update 状态）
        render();   // 不管暂停与否都重画画面
    }
}

// ============================================================================
//  ① 处理输入
// ============================================================================
void Game::processEvents() {
    Event event;
    // 先处理"事件队列"里的一次性事件（关窗、按键松开等）
    while (m_disp.window().pollEvent(event)) {
        if (event.type == Event::Closed)
            m_disp.window().close();
        // 窗口大小变化（切分辨率、拖拽改大小等）→ 重算视图，画面始终等比居中不变形
        if (event.type == Event::Resized)
            m_disp.updateViewport();
        // 松开任意键后：恢复接受输入
        if (event.type == Event::KeyReleased && !m_paused && !m_confirmExit) {
            m_acceptInput = true;
        }
        // 名字输入界面：直接敲键盘字母/数字即可追加（手柄玩家则用下方网格）
        if (event.type == Event::TextEntered && m_nameUI.isActive())
            m_nameUI.appendChar(static_cast<wchar_t>(event.text.unicode));
    }

    // ---- 名字输入界面：交给 NameEntryUI，只编排提交后的收尾 ----
    if (m_nameUI.isActive()) {
        m_nameUI.handleInput(Keyboard::isKeyPressed(Keyboard::Up),
                             Keyboard::isKeyPressed(Keyboard::Down),
                             Keyboard::isKeyPressed(Keyboard::Left),
                             Keyboard::isKeyPressed(Keyboard::Right),
                             Keyboard::isKeyPressed(Keyboard::Return),
                             Keyboard::isKeyPressed(Keyboard::BackSpace),
                             Keyboard::isKeyPressed(Keyboard::Escape));
        if (m_nameUI.hasCommitted()) {   // 已提交名字并写入榜单 → 统一跳到高分榜
            m_nameUI.clearCommitted();
            m_inMenu = false;
            m_paused = true;
            m_scoreboard.open();         // 提交上榜后显示含本局的高分榜
        }
        return;   // 名字输入阶段不再处理其他输入
    }

    // ---- 确认界面（退出 / 清除记录共用）：只响应确认相关按键 ----
    if (m_confirmExit || m_confirmClear) {
        // Esc 或再按 Q → 取消
        if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::Q)) {
            if (!m_confirmCancelHeld) {
                m_confirmCancelHeld = true;
                m_confirmExit  = false;
                m_confirmClear = false;   // 取消，回到之前的界面
            }
        } else {
            m_confirmCancelHeld = false;
        }
        // ←/→ 在 NO / YES 之间切换（选项是左右排列的，用左右键更符合直觉）
        if (Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::Right)) {
            if (!m_confirmMoveHeld) {
                m_confirmMoveHeld = true;
                m_confirmSelection = (m_confirmSelection + 1) % 2;
            }
        } else {
            m_confirmMoveHeld = false;
        }
        // Enter 确认当前选项
        if (Keyboard::isKeyPressed(Keyboard::Return)) {
            if (!m_enterHeld) {
                m_enterHeld = true;
                if (m_confirmSelection == 1) {
                    if (m_confirmClear) { m_scoreboard.clearAll(); m_confirmClear = false; } // YES → 清除并退出确认界面
                    else                m_disp.window().close();                          // YES → 退出游戏
                } else {
                    m_confirmExit  = false;   // NO → 取消
                    m_confirmClear = false;
                }
            }
        } else {
            m_enterHeld = false;
        }
        return;   // 确认界面阶段不再处理其他输入
    }

    // ---- 最高分榜单界面：Esc/Enter 返回主菜单，C 键清除记录 ----
    if (m_scoreboard.isOpen()) {
        if (Keyboard::isKeyPressed(Keyboard::Escape) || Keyboard::isKeyPressed(Keyboard::Return)) {
            if (!m_enterHeld) {
                m_enterHeld = true;
                m_scoreboard.close();
                returnToMainMenu();   // 回主菜单（重填菜单文字 + 复位选中项 + 切菜单曲）
            }
        } else {
            m_enterHeld = false;
        }
        // C 键：弹出"清除全部记录"确认
        if (Keyboard::isKeyPressed(Keyboard::C)) {
            if (!m_clearHeld) {
                m_clearHeld = true;
                m_confirmClear = true;
                m_confirmSelection = 0;   // 默认 NO，防误删
                m_confirmTitle.setString(tr("清空全部记录？", "CLEAR ALL RECORDS?"));
            }
        } else {
            m_clearHeld = false;
        }
        return;
    }

    // ---- 设置界面：交给 SettingsMenu，只编排返回去向 ----
    if (m_settingsMenu.isOpen()) {
        const bool closed = m_settingsMenu.handleInput(
            Keyboard::isKeyPressed(Keyboard::Up),
            Keyboard::isKeyPressed(Keyboard::Down),
            Keyboard::isKeyPressed(Keyboard::Left),
            Keyboard::isKeyPressed(Keyboard::Right),
            Keyboard::isKeyPressed(Keyboard::Return),
            Keyboard::isKeyPressed(Keyboard::Escape));
        if (closed) {   // 设置界面请求关闭 → 按来源回主/暂停菜单
            if (m_settingsMenu.returnToPause()) m_inPauseMenu = true;  // 回暂停菜单
            else                                returnToMainMenu();    // 回主菜单
        }
        return;
    }

    // Esc：游戏中按 Esc 打开暂停菜单；暂停菜单里再按 Esc 继续
    // 边沿检测：只在按下那一刻触发一次
    if (Keyboard::isKeyPressed(Keyboard::Escape)) {
        if (!m_escHeld) {
            m_escHeld = true;
            if (!m_paused && !m_inMenu && !m_inPauseMenu) {
                // 游戏中按 Esc → 打开暂停菜单（冻结游戏）
                m_escPaused   = true;
                m_inPauseMenu = true;
                pauseBgm();     // 暂停当前关卡背景声（关卡3=鼓声/其他=旋律，与游戏同步暂停）
                setupMenu(MenuKind::Pause);   // 填暂停菜单选项并默认选中 RESUME
            } else if (m_inPauseMenu) {
                // 暂停菜单里按 Esc → 继续游戏
                m_inPauseMenu = false;
                m_escPaused   = false;
                resumeBgm();    // 恢复当前关卡背景声
                m_clock.restart(); // 关键：清掉暂停期间累积的时间，否则恢复瞬间 dt 巨大
            }
        }
    } else {
        m_escHeld = false;
    }

    // 按住 Q 不再直接退出，而是弹退出确认界面（边沿检测，避免按住重复触发）
    if (Keyboard::isKeyPressed(Keyboard::Q)) {
        if (!m_qHeld) {
            m_qHeld = true;
            m_confirmExit      = true;   // 进入退出确认
            m_confirmSelection = 0;      // 默认选中 NO，防误触
            m_confirmTitle.setString(tr("确定退出游戏？", "EXIT GAME?")); // 恢复退出确认标题
        }
    } else {
        m_qHeld = false;
    }

    // 按住 Enter：在主菜单 / 暂停菜单里确认选中项；游戏暂停画面里开始/重开一局
    // 边沿检测：只在按下那一刻触发一次
    if (Keyboard::isKeyPressed(Keyboard::Return)) {
        if (!m_enterHeld) {
            m_enterHeld = true;
            if (m_inMenu) {
                if (m_inStageSelect) {
                    // ---- 关卡选择子菜单（3 项：关卡A·砍树驱动 / 关卡B·鼓点驱动 / 返回主菜单） ----
                    switch (m_menuSelection) {
                        case 0: enterStage(config::GameMode::ChopRhythm); break; // 关卡 A · 021 砍树驱动
                        case 1: enterStage(config::GameMode::BeatDrive);  break; // 关卡 B · 020 鼓点驱动
                        case 2:   // 返回主菜单
                            m_inStageSelect = false;
                            setupMenu(MenuKind::Main);
                            break;
                    }
                } else {
                    // ---- 主菜单（5 项：开始新游戏/关卡选择/设置/最高分/退出） ----
                    switch (m_menuSelection) {
                        case 0:   // 开始新游戏：默认关卡1（唯一关卡），进入"按 Enter 开始"准备
                            enterStage(config::GameMode::ChopRhythm);
                            break;
                        case 1:   // 关卡选择：进入关卡选择子菜单
                            m_inStageSelect = true;
                            setupMenu(MenuKind::StageSelect);
                            break;
                        case 2:   // GAME SETTINGS：进入设置界面（返回回主菜单）
                            m_inMenu = false;
                            m_settingsMenu.open(false);   // 来源=主菜单
                            break;
                        case 3: {   // HIGHEST SCORE：进入最高分榜单
                            m_scoreboard.open();   // 读档 + 填充三列 + 显示榜单
                            m_inMenu = false;
                            break;
                        }
                        case 4:   // EXIT GAME：关闭窗口退出游戏
                            m_disp.window().close();
                            break;
                    }
                }
            } else if (m_inPauseMenu) {
                // ---- 暂停菜单 ----
                switch (m_menuSelection) {
                    case 0:   // RESUME：继续游戏
                        m_inPauseMenu = false;
                        m_escPaused   = false;
                        resumeBgm();  // 恢复当前关卡背景声
                        m_clock.restart(); // 清掉暂停期间累积的时间
                        break;
                    case 1:   // RESTART：本局重开（startNewGame 会退出暂停菜单并重置一切）
                        startNewGame(m_mode);
                        break;
                    case 3:   // BACK TO MENU：放弃本局，回主菜单
                        returnToMainMenu();
                        break;
                    case 2:   // GAME SETTINGS：进入设置界面，返回时回暂停菜单
                        m_inPauseMenu = false;
                        m_settingsMenu.open(true);    // 来源=暂停菜单
                        break;
                }
            } else if (m_paused) {
                startNewGame(m_mode);   // 游戏内暂停画面（死亡/超时/树倒）按 Enter 重开当前关卡
            }
        }
    } else {
        m_enterHeld = false;
    }

    // 菜单（主/暂停共用）：上下方向键切换选中项（边沿检测，避免按住快速跳动）
    if (m_inMenu || m_inPauseMenu) {
        const bool up   = Keyboard::isKeyPressed(Keyboard::Up);
        const bool down = Keyboard::isKeyPressed(Keyboard::Down);
        if ((up || down) && !m_menuInputHeld) {
            m_menuInputHeld = true;
            const int count = m_inMenu ? (m_inStageSelect ? 3 : 5) : 4; // 主菜单5项/关卡选择3项/暂停4项
            if (up)   m_menuSelection = (m_menuSelection + count - 1) % count; // 上移
            if (down) m_menuSelection = (m_menuSelection + 1) % count;         // 下移
        } else if (!up && !down) {
            m_menuInputHeld = false;
        }
    }

    // 如果当前接受输入且不在任何菜单，就检测左右方向键（音乐节奏：砍 = 打拍子）
    if (m_acceptInput && !m_inMenu && !m_inPauseMenu) {
        const bool right = Keyboard::isKeyPressed(Keyboard::Right);
        const bool left  = Keyboard::isKeyPressed(Keyboard::Left);
        if (right || left) {
            const Side side = right ? Side::Right : Side::Left;
            m_level->handleInput(side);   // 关卡内：更新站位 + 鼓声时刻判定 + 砍树反馈
            m_acceptInput = false;   // 锁住输入，等按键松开再解锁（避免按住连判）
        }
    }
}

// ============================================================================
//  开始/重开一局：把所有状态复位
// ============================================================================
void Game::startNewGame(config::GameMode mode) {
    m_mode        = mode;             // 记录当前关卡（暂停重开沿用）
    m_inMenu      = false;            // 离开主菜单，进入游戏
    m_inStageSelect = false;          // 若在关卡选择子菜单也一并退出
    m_inPauseMenu = false;            // 若在暂停菜单也一并退出
    m_paused      = false;            // 取消暂停
    m_enterNamePending = false;       // 清掉"结束画面待进名字输入"的标记
    m_clock.restart();   // 关键：把时钟清零，否则暂停期间累积的时间会让第一帧 dt 巨大
    m_escPaused = false; // 清掉可能的 Esc 暂停标记（重开一局时恢复为运行态）
    m_escHeld   = false; // 同时清掉 Esc 边沿检测标志
    m_acceptInput = true; // 可以开始按方向键了
    if (mode == config::GameMode::ChopRhythm)
        switchBgm(BgmState::LevelA);
    else if (mode == config::GameMode::BeatDrive)
        switchBgm(BgmState::LevelB);//切换关卡音乐

    // ---- 关卡状态复位：按 mode 绑定当前关卡并调用其 onStart（背景/树/小人/鼓声全在关卡内复位） ----
    switch (mode) {
        case config::GameMode::ChopRhythm: m_level = &m_levelRhythm; m_levelRhythm.onStart(); break;
        case config::GameMode::BeatDrive:  m_level = &m_levelBeat;   m_levelBeat.onStart();   break;
    }
}

// 进入某关的"按回车开始"准备界面（主菜单/关卡选择共用）：停在提示，按 Enter 才真正开局
void Game::enterStage(config::GameMode mode) {
    m_mode          = mode;
    m_inMenu        = false;
    m_inStageSelect = false;
    m_paused        = true;   // 停在"按 Enter 开始"，不推进游戏
    // 按 mode 绑定当前关卡并复位其状态（准备界面与开局渲染都需要 m_level 非空；背景/树/小人/墓碑在关卡 onStart 内复位）
    switch (mode) {
        case config::GameMode::ChopRhythm: m_level = &m_levelRhythm; m_levelRhythm.onStart(); break;
        case config::GameMode::BeatDrive:  m_level = &m_levelBeat;   m_levelBeat.onStart();   break;
    }
    m_level->stop();   // 准备界面(按 Enter 开始)不播放鼓声；真正开局时 startNewGame 的 onStart 才播放
    setMessage(tr("按回车开始", "Press Enter to Start"));
    switchBgm(BgmState::Menu);
}

void Game::setMessage(const sf::String& text) {
    m_messageText.setString(text);
    const FloatRect rect = m_messageText.getLocalBounds(); // 算出文字占多大矩形
    // 把锚点设到文字中心，然后放到屏幕正中央
    m_messageText.setOrigin(rect.left + rect.width / 2.0f,
                            rect.top + rect.height / 2.0f);
    m_messageText.setPosition(config::SCREEN_CENTER_X, config::SCREEN_CENTER_Y);
}

// ============================================================================
//  把任意文字的中心放到 (x, y)（居中辅助，开始菜单用）
// ============================================================================
void Game::centerText(Text& t, float x, float y) {
    const FloatRect rect = t.getLocalBounds();
    t.setOrigin(rect.left + rect.width / 2.0f, rect.top + rect.height / 2.0f);
    t.setPosition(x, y);
}

// ============================================================================
//  按菜单类型填入选项文字并重置选中项（主菜单 / 暂停菜单共用一套选项数组）
// ============================================================================
// 填某类菜单的选项文字并返回选项数（语言刷新/进入菜单共用）；同时设置菜单顶部标题
int Game::fillMenuText(Game::MenuKind kind) {
    const int count = (kind == MenuKind::Pause) ? 4 : (kind == MenuKind::StageSelect ? 3 : 5);
    sf::String labels[5];
    if (kind == MenuKind::Pause) {
        labels[0] = tr("继续", "RESUME");
        labels[1] = tr("重新开始", "RESTART");
        labels[2] = tr("游戏设置", "GAME SETTINGS");
        labels[3] = tr("返回主菜单", "BACK TO MENU");
    } else if (kind == MenuKind::StageSelect) {
        m_menuTitle.setString(tr("选择关卡", "SELECT STAGE"));
        labels[0] = tr("关卡A · 砍树驱动", "STAGE A · CHOP DRIVE");
        labels[1] = tr("关卡B · 鼓点驱动", "STAGE B · BEAT DRIVE");
        labels[2] = tr("返回主菜单", "BACK TO MENU");
    } else {   // 主菜单
        m_menuTitle.setString("CHOPPER");
        labels[0] = tr("开始新游戏", "NEW GAME");
        labels[1] = tr("关卡选择", "SELECT STAGE");
        labels[2] = tr("游戏设置", "GAME SETTINGS");
        labels[3] = tr("最高分", "HIGHEST SCORE");
        labels[4] = tr("退出游戏", "EXIT GAME");
    }
    for (int i = 0; i < count; ++i) {
        m_menuOptions[i].setString(labels[i]);
        centerText(m_menuOptions[i], config::SCREEN_CENTER_X, 500.0f + i * 100.0f);
    }
    return count;
}

void Game::setupMenu(Game::MenuKind kind) {
    fillMenuText(kind);
    m_menuSelection = 0;  // 每次进菜单默认选中第一项
    m_menuInputHeld = false;
}

// 统一"回主菜单"入口。
// 以前有几条路径只置 m_inMenu = true（例如榜单按 Enter 关闭），但没重填菜单文字：
// 结果主菜单里显示的仍是上一个界面（暂停菜单）的选项，选中项也停在旧值，
// 按 Enter 就落到了"GAME SETTINGS"上 —— 表现为"回主菜单却进了设置"。
void Game::returnToMainMenu() {
    m_inMenu        = true;
    m_inStageSelect = false;
    m_inPauseMenu   = false;
    m_escPaused     = false;
    m_paused        = true;   // 主菜单阶段不推进游戏
    if (m_currentBgm != BgmState::Menu) switchBgm(BgmState::Menu);
    setupMenu(MenuKind::Main);   // 重填主菜单文字并复位选中项
}

// 统一"结束本局"入口：两个关卡的 doGameOver 都只做本关特有的收尾
// （停音效、藏斧头、小人倒下），共用逻辑全部收敛到这里。
// 以前这些状态分散在两个关卡里各写一遍，很容易改漏一条路径。
void Game::endRound(const sf::String& msg, bool failed) {
    (void)failed;                    // 目前两种结局的共用处理一致；保留参数便于以后区分
    m_paused = true;                 // 冻结游戏推进（但特效仍会继续演）
    setMessage(msg);
    stopBgm();                       // 停掉关卡鼓声 + BGM，结算画面上不再有音乐
    m_enterNamePending = true;       // 是否上榜由主循环按分数裁定
    m_gameOverClock.restart();
}

// 语言切换时刷新 Game 自身界面文字（菜单/确认/分数），并传播到各子界面
void Game::applyLanguage() {
    // 仅在实际处于主/暂停菜单时才刷新菜单选项文字（设置/榜单/名字输入等子界面期间
    // 不碰，避免覆盖返回后还要用的选项文字，例如从设置返回暂停菜单）
    if (m_inMenu || m_inPauseMenu) {
        // 菜单选项：按当前是主菜单/关卡选择/暂停菜单填（不动选中项）
        const MenuKind kind = m_inPauseMenu ? MenuKind::Pause
                              : (m_inStageSelect ? MenuKind::StageSelect : MenuKind::Main);
        fillMenuText(kind);
    }
    // 确认标题/选项：语言切换时不在确认界面，重置为默认退出确认（进入清榜/退出确认时会再设）
    m_confirmTitle.setString(tr("确定退出游戏？", "EXIT GAME?"));
    centerText(m_confirmTitle, config::SCREEN_CENTER_X, 400.0f);
    m_confirmOptions[0].setString(tr("否", "NO"));
    m_confirmOptions[1].setString(tr("是", "YES"));
    centerText(m_confirmOptions[0], config::SCREEN_CENTER_X - 150.0f, 550.0f);
    centerText(m_confirmOptions[1], config::SCREEN_CENTER_X + 150.0f, 550.0f);
    // 分数前缀（分数在关卡内自持）
    m_scoreText.setString(tr("得分 = ", "Score = ") + std::to_string(m_level->score()));
    m_lastScoreShown = m_level ? m_level->score() : -1;   // 同步缓存，避免下一帧重复重排

    // 各子界面文字按当前语言刷新
    m_settingsMenu.applyLanguage();
    m_scoreboard.applyLanguage();
    m_nameUI.applyLanguage();
}

// 渲染前检测语言变化：把刷新从 update 挪到每帧渲染前，任何界面（主/暂停菜单、设置、
// 榜单、名字输入、确认、游戏中）切换语言都立即生效，无需重启
void Game::checkLanguage() {
    if (config::g_lang != m_lastLang) {
        m_lastLang = config::g_lang;
        applyLanguage();
    }
}

// 背景声统一分发：当前关卡自持鼓声（Esc 暂停/暂停菜单共用）
void Game::pauseBgm() 
{
    m_bgmPlayer.pause(); 
    if (m_level) m_level->pause();
}
void Game::resumeBgm()
{
    m_bgmPlayer.play(); 
    if (m_level) m_level->resume();
}
void Game::stopBgm()
{
    m_bgmPlayer.stop(); 
    m_currentBgm = BgmState::None;
    if (m_level) m_level->stop();
}

void Game::alignRow(Text& t, float x, float y, int mode) {
    const FloatRect r = t.getLocalBounds();
    if (mode == 2)      t.setOrigin(r.left + r.width, r.top + r.height / 2.0f); // 右对齐
    else if (mode == 1) t.setOrigin(r.left, r.top + r.height / 2.0f);          // 左对齐
    else                t.setOrigin(r.left + r.width / 2.0f, r.top + r.height / 2.0f); // 居中
    t.setPosition(x, y);
}

void Game::update(const Time& dt) {
    if (!m_level) return;   // 未进入关卡（防御性保护；正常 update 只在游戏阶段调用，m_level 已绑定）
    const float realDt = dt.asSeconds();        // 真实帧时间
    const float scale  = m_fx.timeScale();      // 1 = 正常，≈0 = 命中定格，<1 = 慢动作
    float gameDt = realDt * scale;
    if (scale < 0.999f) {
        // 定格/慢动作：把被"冻住"的时间记成欠账，稍后补回，节拍时钟才不会越打越偏
        m_dtDebt += realDt * (1.0f - scale);
    } else if (m_dtDebt > 0.0f) {
        // 每帧最多多补 50%：平滑追平，不会出现肉眼可见的跳帧
        const float pay = std::min(m_dtDebt, realDt * 0.5f);
        gameDt += pay;
        m_dtDebt -= pay;
    }
    // 当前关卡完全自包含：节拍tick/树枝/字母/圆木/背景/时间/碰撞/结算全在关卡内
    m_level->update(gameDt);
    m_fx.update(realDt);                        // 特效用真实时间推进（内部再按 timeScale 缩放粒子）
    // 时间条与分数由关卡数值驱动
    m_timeBar.setSize(Vector2f(m_level->timeBarWidthPerSecond() * m_level->timeRemaining(),
                               config::TIME_BAR_HEIGHT));
    // 分数没变就不重排文字：中文 Text 每次 setString 都要重建顶点数组，很吃帧
    const int sc = m_level->score();
    if (sc != m_lastScoreShown) {
        m_lastScoreShown = sc;
        m_scoreText.setString(tr("得分 = ", "Score = ") + std::to_string(sc));
    }
}

// （关卡1/2 与 MusicSystem 的逻辑已随重构删除，Game 只做框架 + 调度唯一关卡）

// （场景装饰 蜜蜂/云 与 玩家 小人/墓碑 已迁入 Scenery / Player，见对应 .cpp；此处不再持有）

// （关卡1 砍击反馈/斧头/圆木/树变细倒下 与 关卡2 断枝/刀光等逻辑已迁入
//  ChopTreeLevel / ChopBranchLevel，见对应 .cpp；此处不再持有）

void Game::render() {
    m_disp.window().clear();
    m_disp.window().setView(m_disp.view());  // 所有绘制都在 1920x1080 逻辑坐标下，由视图自动适配到实际窗口
    // ---- 内置 FPS 统计（右上角，0.5s 刷新一次；不依赖外部工具，真实反映渲染帧率） ----
    if (m_disp.showFps()) {
        const float frameSec = m_fpsClock.restart().asSeconds();
        const float frameMs  = frameSec * 1000.0f;
        m_fpsLastMs = frameMs;
        if (frameMs > m_fpsMaxMs) m_fpsMaxMs = frameMs;
        ++m_fpsFrames;
        m_fpsAccum += frameSec;
        if (m_fpsAccum >= 0.5f) {
            const int fps = static_cast<int>(m_fpsFrames / m_fpsAccum + 0.5f);
            // 同时显示本帧耗时与 0.5 秒内的最差帧：判卡顿只看 FPS 会漏掉尖峰
            char buf[96];
            std::snprintf(buf, sizeof(buf), "FPS %d   %.1fms   max %.1fms",
                          fps, static_cast<double>(m_fpsLastMs), static_cast<double>(m_fpsMaxMs));
            m_fpsText.setString(buf);
            alignRow(m_fpsText, config::SCREEN_WIDTH - 20.0f, 20.0f, 2); // 文本变长需重对齐，否则右缘溢出屏幕
            m_fpsFrames = 0;
            m_fpsAccum  = 0.0f;
            m_fpsMaxMs  = 0.0f;
        }
    }
    if (m_nameUI.isActive()) {
        // ---- 名字输入界面：黑底 + 标题 + 名字预览 + 字母网格 + SKIP/DONE ----
        m_nameUI.draw(m_disp.window());
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    if (m_confirmExit || m_confirmClear) {
        // ---- 确认界面（退出 / 清除记录）：黑底 + 标题 + NO/YES ----
        centerText(m_confirmTitle, config::SCREEN_CENTER_X, 400.0f); // 标题每次重新居中（字符串可能变化）
        m_disp.window().draw(m_confirmTitle);
        for (int i = 0; i < 2; ++i) {
            m_confirmOptions[i].setFillColor(i == m_confirmSelection ? Color::Yellow : Color::White);
            m_disp.window().draw(m_confirmOptions[i]);
        }
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    if (m_scoreboard.isOpen()) {
        // ---- 最高分榜单：标题 + 三列(序号/名字/分数) + 返回/清除提示 ----
        m_scoreboard.draw(m_disp.window());
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    if (m_settingsMenu.isOpen()) {
        // ---- 设置界面：标题 + 13 项(标签/值) + 返回提示（值刷新在 handleInput 内） ----
        m_settingsMenu.draw(m_disp.window());
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    if (m_inMenu) {
        // ---- 主菜单 / 关卡选择界面：纯黑背景 + 菜单 + 顶部标题 ----
        const int count = m_inStageSelect ? 3 : 5;
        m_disp.window().draw(m_menuTitle);
        for (int i = 0; i < count; ++i) {
            m_menuOptions[i].setFillColor(i == m_menuSelection ? Color::Yellow : Color::White);
            m_disp.window().draw(m_menuOptions[i]);
        }
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    if (m_inPauseMenu) {
        // ---- 暂停菜单界面：同主菜单布局，但没有顶部标题 ----
        for (int i = 0; i < 4; ++i) {
            m_menuOptions[i].setFillColor(i == m_menuSelection ? Color::Yellow : Color::White);
            m_disp.window().draw(m_menuOptions[i]);
        }
        if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
        m_disp.window().display();
        return;
    }
    // 关卡场景：整体带屏幕震动偏移（HUD 不跟着抖）
    {
        View shakeView = m_disp.view();
        shakeView.move(m_fx.shakeOffset());
        m_disp.window().setView(shakeView);
    }
    m_level->renderScene(m_disp.window());   // 关卡画全部场景（背景/云/蜜蜂→树/树枝/小人/墓碑/圆木/斧头）
    m_fx.drawScene(m_disp.window());         // 粒子与冲击环：场景之上、HUD 之下
    m_disp.window().setView(m_disp.view());  // 恢复视图，HUD 不抖
    m_disp.window().draw(m_scoreText);
    m_level->renderUI(m_disp.window());      // 关卡画 HUD（提示框/A·B字母/判定/连击）
    m_disp.window().draw(m_timeBar);
    if (m_paused || m_escPaused)
        m_disp.window().draw(m_messageText); // 暂停（含 Esc 手动暂停）时画中央提示
    m_fx.drawOverlay(m_disp.window());       // 闪光/鼓点脉冲/CRT：最上层
    if (m_disp.showFps()) m_disp.window().draw(m_fpsText);
    m_disp.window().display();
}

// ============================================================================
//  资源加载辅助函数
// ============================================================================
bool Game::loadTexture(Texture& out, const char* path, const char* name) {
    if (!out.loadFromFile(path)) {   // 加载失败（文件不存在/格式错）
        std::cerr << "[Warning] 纹理加载失败: " << name << " -> " << path << "\n";
        return false;
    }
    return true;
}

void Game::loadAssets() {
    loadTexture(m_texBackground, config::ASSET_BACKGROUND, "background");
    loadTexture(m_texTree,       config::ASSET_TREE,       "tree");
    loadTexture(m_texBee,        config::ASSET_BEE,        "bee");
    loadTexture(m_texCloud,      config::ASSET_CLOUD,      "cloud");
    loadTexture(m_texBranch,     config::ASSET_BRANCH,     "branch");
    loadTexture(m_texPlayer,     config::ASSET_PLAYER,     "player");
    loadTexture(m_texRip,        config::ASSET_RIP,        "rip");
    loadTexture(m_texAxe,        config::ASSET_AXE,        "axe");
    loadTexture(m_texLog,        config::ASSET_LOG,        "log");
    // 加载字体
    if (!m_font.loadFromFile(config::ASSET_FONT))
        std::cerr << "[Warning] 字体加载失败: " << config::ASSET_FONT << "\n";
    prewarmFont();   // 提前把要用到的字烘焙好，避免游戏中首帧出现新字时掉帧
    // 加载命中打击音效（砍树反馈）
    if (!m_soundBuffer.loadFromFile(config::ASSET_CHOP_SOUND))
        std::cerr << "[Warning] 音效加载失败: " << config::ASSET_CHOP_SOUND << "\n";
    else
        m_sound.setBuffer(m_soundBuffer);
    // ---- BGM：Chiptune 离线渲染 → Int16 → SoundBuffer（每首只渲染一次） ----
    auto loadBgm = [this](const chip::Song& song, sf::SoundBuffer& buf) {
        const std::vector<float> f = chip::renderLooped(song, 1, config::SAMPLE_RATE);
        std::vector<sf::Int16> pcm(f.size());
        for (size_t i = 0; i < f.size(); ++i) {
            const float v = std::max(-1.0f, std::min(1.0f, f[i]));
            pcm[i] = static_cast<sf::Int16>(v * 32767.0f);
        }
        buf.loadFromSamples(pcm.data(), pcm.size(), 1, config::SAMPLE_RATE);
    };
    loadBgm(makeMenuSong(),   m_bgmBufMenu);
    loadBgm(makeLevelASong(), m_bgmBufLevelA);
    loadBgm(makeLevelBSong(), m_bgmBufLevelB);

    m_bgmPlayer.setLoop(true);
    m_bgmPlayer.setVolume(config::BGM_VOLUME);   // 背景层：只做铺垫
    m_sound.setVolume(config::SFX_VOLUME);       // 打击音效：必须能听见
}

// 预烘焙字形。
// SFML 的字体是懒加载：某个字符第一次出现在屏幕上时才光栅化并上传纹理，
// 这一下可能耗时好几毫秒——在 160Hz（每帧 6.25ms）下就足以漏掉一整帧。
// 启动时把所有会用到的字符先取一遍，游戏中就不会再有这种尖峰。
void Game::prewarmFont() {
    const unsigned int sizes[] = { 30u, 50u, 60u, 75u, 80u, 90u, 100u, 150u };

    // 可见 ASCII（菜单是英文、数字、标点）
    for (unsigned int s : sizes)
        for (char32_t c = 32; c < 127; ++c)
            m_font.getGlyph(c, s, false);

    // 界面上出现过的中文
    const char32_t zh[] = {
        U'完', U'美', U'好', U'失', U'误', U'通', U'关', U'得', U'分', U'秒',
        U'时', U'间', U'到', U'你', U'被', U'压', U'死', U'了', U'按', U'回',
        U'车', U'开', U'始', U'清', U'空', U'全', U'部', U'记', U'录', U'确',
        U'定', U'退', U'出', U'游', U'戏', U'否', U'是', U'设', U'置', U'语',
        U'言', U'音', U'量', U'关', U'卡', U'选', U'择', U'最', U'高'
    };
    for (unsigned int s : sizes)
        for (char32_t c : zh)
            m_font.getGlyph(c, s, false);
}

// ============================================================================
//  场景初始化：把每个精灵贴上纹理、摆好初始位置
// ============================================================================
void Game::initScene() {
    // 两个关卡各自初始化（背景/树/小人/鼓声/斧头/圆木/提示框全在关卡内 initScene，互不共享可变状态）
    m_levelRhythm.initScene();
    m_levelBeat.initScene();
}

//实现 switchBgm()
void Game::switchBgm(BgmState target) {
    if (target == m_currentBgm) return;
    m_bgmPlayer.stop();
    switch (target) {
    case BgmState::Menu:   m_bgmPlayer.setBuffer(m_bgmBufMenu); break;
    case BgmState::LevelA: m_bgmPlayer.setBuffer(m_bgmBufLevelA); break;
    case BgmState::LevelB: m_bgmPlayer.setBuffer(m_bgmBufLevelB); break;
    case BgmState::None:   return;
    }
    m_bgmPlayer.setLoop(true);
    m_bgmPlayer.play();
    m_currentBgm = target;
}