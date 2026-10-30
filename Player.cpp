// ============================================================================
//  Player：玩家化身组件实现
// ============================================================================
#include "Player.h"

void Player::init(const Texture& texPlayer, const Texture& texRip) {
    m_player.setTexture(texPlayer);
    m_player.setOrigin(64.0f, 128.0f);      // 脚底中心（潜水员贴图 128x128）
    m_player.setScale(1.7f, 1.7f);
    m_player.setPosition(config::PLAYER_LEFT_X, config::PLAYER_Y);
    m_rip.setTexture(texRip);
    m_rip.setOrigin(64.0f, 128.0f);
    m_rip.setPosition(config::RIP_HIDE_X, config::RIP_HIDE_Y);
}

void Player::onSide(Side side) {
    m_player.setPosition(side == Side::Left ? config::PLAYER_LEFT_X : config::PLAYER_RIGHT_X,
                         config::PLAYER_Y);
}

void Player::reset() {
    m_rip.setPosition(config::RIP_HIDE_X, config::RIP_HIDE_Y);   // 墓碑藏起来
    m_player.setPosition(config::PLAYER_LEFT_X, config::PLAYER_Y); // 小人回左边
}

// 被压死：小人移出屏幕，墓碑留在被压那一侧脚边（原版 timber.cpp 死亡视觉）
void Player::die(Side side) {
    m_player.setPosition(0.0f, config::RIP_HIDE_Y);   // 小人移出屏幕
    m_rip.setPosition(side == Side::Left ? config::RIP_SQUISH_LEFT_X
                                         : config::RIP_SQUISH_RIGHT_X,
                      config::RIP_Y);
}

void Player::render(sf::RenderWindow& w) const {
    w.draw(m_player);
    w.draw(m_rip);
}
