// ============================================================================
//  ILevel：关卡策略接口（精简通用版）
// ============================================================================
//  每个关卡**完全自包含**：背景/树/小人/树枝/圆木/判定位置等全部场景元素都由
//  关卡自己管理（Game 不再统一渲染树/背景/小人）。Game 只做通用框架（窗口/
//  菜单/时间条HUD/分数/消息/榜单）并调度当前关卡。以后加新关卡 = 新写一个
//  实现 ILevel 的类 + 关卡选择菜单加一项，不再往 Game.cpp 堆分支。
// ============================================================================
#ifndef CHOPPER_ILEVEL_H
#define CHOPPER_ILEVEL_H

#include <SFML/Graphics.hpp>
#include "config.h"   // Side + using namespace sf

class ILevel {
public:
    virtual ~ILevel() = default;

    // 一次性初始化：给本关专属精灵贴纹理、设锚点/缩放/初始位置（Game::initScene 调用一次）
    virtual void initScene() = 0;
    // 每局开始：复位本关全部状态（Game::startNewGame / enterStage 调用）
    virtual void onStart() = 0;
    // 方向键输入：更新站位 + 判定时机 + 砍树反馈（Perfect/Good/Miss 全在关卡内处理）
    virtual void handleInput(Side side) = 0;
    // 每帧推进本关动画/物理（节拍tick/树枝/字母/圆木/背景/时间/碰撞/结算）
    virtual void update(float dt) = 0;
    // 画本关全部场景（背景/云/蜜蜂 → 树 → 树枝 → 小人/墓碑 → 圆木 → 斧头），画在 HUD 之下
    virtual void renderScene(sf::RenderWindow& w) = 0;
    // 画本关 HUD（按键提示框/A·B字母/判定/连击），画在分数与时间条附近
    virtual void renderUI(sf::RenderWindow& w) = 0;

    // ---- 供 Game 读取的访问器（时间条/分数/结束判定） ----
    virtual int   score() const = 0;              // 当前得分
    virtual float timeRemaining() const = 0;      // 剩余时间
    virtual float timeBarWidthPerSecond() const = 0; // 时间条每秒缩多少像素
    virtual bool  isCleared() const = 0;          // 是否通关（鼓声播完）
    virtual bool  isDead() const = 0;             // 是否死亡（被树枝压死）

    // 暂停/恢复/停止本关背景声（Game 的 Esc 暂停/暂停菜单统一分发）
    virtual void pause()  = 0;
    virtual void resume() = 0;
    virtual void stop()   = 0;

    // 玩家在"要不要带你打"提示里的选择（true = 接受代打）
    virtual void onAssistChosen(bool useAssist) = 0;
};

#endif
