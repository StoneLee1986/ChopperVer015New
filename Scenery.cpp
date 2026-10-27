// ============================================================================
//  Scenery：场景装饰组件实现
// ============================================================================
#include "Scenery.h"

Scenery::Scenery(std::mt19937& rng) : m_rng(rng) {}

void Scenery::init(const Texture& texBg, const Texture& texCloud, const Texture& texBee) {
    m_background.setTexture(texBg);
    m_background.setPosition(0.0f, 0.0f);

    m_bee.setTexture(texBee);
    m_bee.setPosition(0.0f, config::BEE_INIT_Y);

    m_cloud1.setTexture(texCloud);
    m_cloud2.setTexture(texCloud);
    m_cloud3.setTexture(texCloud);
    m_cloud1.setPosition(0.0f, 0.0f);
    m_cloud2.setPosition(0.0f, 250.0f);
    m_cloud3.setPosition(0.0f, 500.0f);
}

void Scenery::update(const Time& dt) {
    // 蜜蜂：还没在屏幕上 → 随机给速度和高度，从右边飞进来；飞出左边界后等下一帧重新生成
    if (!m_beeActive) {
        m_beeSpeed = static_cast<float>(m_dBeeSpeed(m_rng));
        m_bee.setPosition(config::BEE_SPAWN_X, static_cast<float>(m_dBeeHeight(m_rng)));
        m_beeActive = true;
    } else {
        m_bee.move(-m_beeSpeed * dt.asSeconds(), 0.0f);
        if (m_bee.getPosition().x < -100.0f)
            m_beeActive = false;
    }

    // 三朵云彩（逻辑都一样，只是高度区间不同）
    updateCloud(m_cloud1Active, m_cloud1Speed, m_cloud1, m_dCloudSpeed, m_dCloud1Height, dt);
    updateCloud(m_cloud2Active, m_cloud2Speed, m_cloud2, m_dCloudSpeed, m_dCloud2Height, dt);
    updateCloud(m_cloud3Active, m_cloud3Speed, m_cloud3, m_dCloudSpeed, m_dCloud3Height, dt);
}

void Scenery::updateCloud(bool& active, float& speed, Sprite& cloud,
                          std::uniform_int_distribution<int>& speedDist,
                          std::uniform_int_distribution<int>& heightDist,
                          const Time& dt) {
    if (!active) {
        // 云彩不在屏幕上 → 随机速度和高度，从左边飘进来
        speed = static_cast<float>(speedDist(m_rng));
        cloud.setPosition(config::CLOUD_SPAWN_X, static_cast<float>(heightDist(m_rng)));
        active = true;
    } else {
        // 云彩正在飘 → 向右移动，飞出右边界后等下一帧重新生成
        cloud.move(speed * dt.asSeconds(), 0.0f);
        if (cloud.getPosition().x > static_cast<float>(config::SCREEN_WIDTH))
            active = false;
    }
}

void Scenery::render(sf::RenderWindow& w) const {
    w.draw(m_background);
    w.draw(m_cloud1);
    w.draw(m_cloud2);
    w.draw(m_cloud3);
    w.draw(m_bee);
}
