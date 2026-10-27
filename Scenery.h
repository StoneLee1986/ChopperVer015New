// ============================================================================
//  Scenery：场景装饰组件（背景 + 三朵云 + 蜜蜂）
// ============================================================================
//  封装"天空背景 + 云彩漂移 + 蜜蜂飞行"这一整套装饰，任何关卡（哪怕题材
//  不同）都直接放一片天空。自持精灵与漂移动画状态，只依赖随机引擎 rng 与
//  绘制窗口；纹理由 Game 传入（共享资源在 Game 统一加载）。不含玩法逻辑。
// ============================================================================
#ifndef CHOPPER_SCENERY_H
#define CHOPPER_SCENERY_H

#include <SFML/Graphics.hpp>
#include <random>
#include "config.h"   // using namespace sf（Sprite/Texture/RenderWindow/Time）+ 场景常量

class Scenery {
public:
    explicit Scenery(std::mt19937& rng);

    void init(const Texture& texBg, const Texture& texCloud, const Texture& texBee); // 贴纹理/摆初始位置
    void update(const Time& dt);                 // 推进蜜蜂飞行 + 三朵云漂移
    void render(sf::RenderWindow& w) const;      // 画背景 + 云 + 蜜蜂（场景背景层）

private:
    void updateCloud(bool& active, float& speed, Sprite& cloud,
                     std::uniform_int_distribution<int>& speedDist,
                     std::uniform_int_distribution<int>& heightDist,
                     const Time& dt);            // 更新一朵云彩

    std::mt19937& m_rng;                         // 共享随机引擎（Game 持有）

    Sprite m_background;                         // 天空背景
    Sprite m_bee;                                // 蜜蜂（远处飞行的装饰）
    Sprite m_cloud1, m_cloud2, m_cloud3;         // 三朵云彩

    bool  m_beeActive    = false;                // 蜜蜂是否正在屏幕上飞
    bool  m_cloud1Active = false;                // 云1是否正在飘
    bool  m_cloud2Active = false;                // 云2是否正在飘
    bool  m_cloud3Active = false;                // 云3是否正在飘
    float m_beeSpeed     = 0.0f;                 // 蜜蜂飞行速度
    float m_cloud1Speed  = 0.0f;                 // 云1速度
    float m_cloud2Speed  = 0.0f;                 // 云2速度
    float m_cloud3Speed  = 0.0f;                 // 云3速度

    // 随机区间（对应原版 rand()%N 的取值范围，从 Game 迁入）
    std::uniform_int_distribution<int> m_dBeeSpeed    {200, 399};   // 蜜蜂速度 200~399
    std::uniform_int_distribution<int> m_dBeeHeight   {500, 999};   // 蜜蜂高度 500~999
    std::uniform_int_distribution<int> m_dCloudSpeed  {0, 199};     // 云彩速度 0~199
    std::uniform_int_distribution<int> m_dCloud1Height {0, 149};    // 云1高度 0~149
    std::uniform_int_distribution<int> m_dCloud2Height {-150, 149}; // 云2高度 -150~149
    std::uniform_int_distribution<int> m_dCloud3Height {-150, 299}; // 云3高度 -150~299
};

#endif
