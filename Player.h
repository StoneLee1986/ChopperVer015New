// ============================================================================
//  Player：玩家化身组件（小人 + 墓碑）
// ============================================================================
//  封装玩家小人及其死亡墓碑：小人切到左右一侧（挥砍视觉反馈）、每局复位、
//  绘制。跨关卡共用（关卡1/2 都有小人）。死亡"原因"由关卡决定，本组件只
//  负责表现。纹理由 Game 传入（共享资源在 Game 统一加载）。
// ============================================================================
#ifndef CHOPPER_PLAYER_H
#define CHOPPER_PLAYER_H

#include <SFML/Graphics.hpp>
#include "config.h"        // using namespace sf + Side + 小人/墓碑常量

class Player {
public:
    void init(const Texture& texPlayer, const Texture& texRip); // 贴纹理/摆初始位置
    void onSide(Side side);    // 小人切到对应一侧（挥砍视觉反馈）
    void reset();              // 小人回左边、墓碑藏起来（每局开始）
    void die(Side side);       // 死亡表现：小人移出屏幕、墓碑留在被压那一侧（原版逻辑）
    void render(sf::RenderWindow& w) const;   // 画小人 + 墓碑

private:
    Sprite m_player;   // 伐木工人小人
    Sprite m_rip;      // 墓碑（死亡/超时时显示）
};

#endif
