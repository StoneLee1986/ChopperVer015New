// ============================================================================
//  Fx.cpp：特效实现
// ============================================================================
#include "Fx.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace {
inline float rnd01() { return static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX); }
inline float rnd(float a, float b) { return a + (b - a) * rnd01(); }
} // namespace

Fx::Fx() {
    m_quad.setSize(sf::Vector2f(1.0f, 1.0f));
    m_circle.setRadius(1.0f);
    m_circle.setFillColor(sf::Color::Transparent);

    m_flashRect.setSize(sf::Vector2f(static_cast<float>(config::SCREEN_WIDTH),
                                     static_cast<float>(config::SCREEN_HEIGHT)));
    m_flashRect.setPosition(0.0f, 0.0f);

    m_crt = config::FX_CRT_ENABLED;
    if (m_crt) {
        m_scanlines.setPrimitiveType(sf::Lines);
        for (unsigned int y = 0; y < config::SCREEN_HEIGHT; y += 4) {
            const sf::Color c(0, 0, 0, 38);
            m_scanlines.append(sf::Vertex(sf::Vector2f(0.0f, static_cast<float>(y)), c));
            m_scanlines.append(sf::Vertex(sf::Vector2f(static_cast<float>(config::SCREEN_WIDTH),
                                                       static_cast<float>(y)), c));
        }
    }
}

// ---------------------------------------------------------------------------
void Fx::update(float realDt) {
    m_time += realDt;

    if (m_hitStopT > 0.0f) m_hitStopT -= realDt;
    if (m_slowT    > 0.0f) m_slowT    -= realDt;

    // 定格优先于慢动作
    m_timeScale = (m_hitStopT > 0.0f) ? 0.02f
                : (m_slowT    > 0.0f) ? m_slowScale
                                      : 1.0f;

    const float dt = realDt * m_timeScale;

    // 震动：二次衰减，用正弦噪声避免纯随机的抖动感
    if (m_shakeT < m_shakeDur) {
        m_shakeT += realDt;
        const float k   = 1.0f - std::min(1.0f, m_shakeT / m_shakeDur);
        const float amp = m_shakeAmp * config::FX_SHAKE_SCALE * k * k;
        m_shakeOffset = sf::Vector2f(std::sin(m_time * 137.0f) * amp,
                                     std::cos(m_time * 191.0f) * amp);
    } else {
        m_shakeOffset = sf::Vector2f(0.0f, 0.0f);
    }

    if (m_flashT < m_flashDur) m_flashT += realDt;
    if (m_beatGlow > 0.0f)     m_beatGlow = std::max(0.0f, m_beatGlow - realDt / 0.18f);

    for (auto& p : m_particles) {
        if (!p.active) continue;
        p.life -= dt;
        if (p.life <= 0.0f) { p.active = false; continue; }
        p.vel.y += p.gravity * dt;
        p.pos   += p.vel * dt;
    }
    for (auto& r : m_rings) {
        if (!r.active) continue;
        r.t += dt;
        if (r.t >= r.dur) r.active = false;
    }
}

// ---------------------------------------------------------------------------
void Fx::drawScene(sf::RenderTarget& rt) {
    // 冲击环
    for (auto& r : m_rings) {
        if (!r.active) continue;
        const float k   = std::min(1.0f, r.t / r.dur);
        const float rad = r.r0 + (r.r1 - r.r0) * k;
        sf::Color c = r.color;
        c.a = static_cast<sf::Uint8>(static_cast<float>(c.a) * (1.0f - k));
        m_circle.setRadius(rad);
        m_circle.setOrigin(rad, rad);
        m_circle.setPosition(r.pos);
        m_circle.setOutlineColor(c);
        m_circle.setOutlineThickness(std::max(1.0f, 8.0f * (1.0f - k)));
        rt.draw(m_circle);
    }

    // 粒子：全部塞进一个 VertexArray，一次 draw call（原来是每颗粒子一次绘制）
    m_batch.clear();
    for (auto& p : m_particles) {
        if (!p.active) continue;
        const float k = std::max(0.0f, p.life / p.maxLife);
        sf::Color c = p.color;
        c.a = static_cast<sf::Uint8>(255.0f * k);
        const float h = p.size * (0.5f + 0.5f * k) * 0.5f;   // 半边长，越接近消散越小
        const float x = p.pos.x, y = p.pos.y;

        m_batch.append(sf::Vertex(sf::Vector2f(x - h, y - h), c));
        m_batch.append(sf::Vertex(sf::Vector2f(x + h, y - h), c));
        m_batch.append(sf::Vertex(sf::Vector2f(x + h, y + h), c));
        m_batch.append(sf::Vertex(sf::Vector2f(x - h, y - h), c));
        m_batch.append(sf::Vertex(sf::Vector2f(x + h, y + h), c));
        m_batch.append(sf::Vertex(sf::Vector2f(x - h, y + h), c));
    }
    if (m_batch.getVertexCount() > 0) rt.draw(m_batch);
}

// ---------------------------------------------------------------------------
void Fx::drawOverlay(sf::RenderTarget& rt) {
    if (m_flashT < m_flashDur) {
        const float k = 1.0f - std::min(1.0f, m_flashT / m_flashDur);
        sf::Color c = m_flashColor;
        c.a = static_cast<sf::Uint8>(static_cast<float>(m_flashColor.a) * k);
        m_flashRect.setFillColor(c);
        rt.draw(m_flashRect);
    }
    if (m_beatGlow > 0.0f) {
        m_flashRect.setFillColor(sf::Color(255, 255, 255,
                                           static_cast<sf::Uint8>(20.0f * m_beatGlow)));
        rt.draw(m_flashRect);
    }
    if (m_crt) rt.draw(m_scanlines);
}

// ---------------------------------------------------------------------------
//  事件
// ---------------------------------------------------------------------------
void Fx::spawnParticle(sf::Vector2f pos, sf::Vector2f vel, float life,
                       float size, sf::Color c, float gravity) {
    Particle& p = m_particles[m_cursor];
    m_cursor = (m_cursor + 1) % kMaxParticles;
    p.pos     = pos;
    p.vel     = vel;
    p.life    = life;
    p.maxLife = life;
    p.size    = size;
    p.color   = c;
    p.gravity = gravity;
    p.active  = true;
}

void Fx::burst(sf::Vector2f pos, sf::Color c, int count, float speed,
               float size, float gravity, float life) {
    for (int i = 0; i < count; ++i) {
        const float ang = rnd(0.0f, 6.2831853f);
        const float sp  = speed * rnd(0.45f, 1.0f);
        spawnParticle(pos,
                      sf::Vector2f(std::cos(ang) * sp, std::sin(ang) * sp - speed * 0.35f),
                      life * rnd(0.7f, 1.2f),
                      size * rnd(0.6f, 1.3f),
                      c, gravity);
    }
}

void Fx::dust(sf::Vector2f pos, int count) {
    for (int i = 0; i < count; ++i)
        spawnParticle(pos,
                      sf::Vector2f(rnd(-120.0f, 120.0f), rnd(-260.0f, -80.0f)),
                      rnd(0.25f, 0.5f),
                      rnd(4.0f, 10.0f),
                      sf::Color(210, 200, 180), 260.0f);
}

void Fx::ring(sf::Vector2f pos, sf::Color c, float r0, float r1, float dur) {
    for (auto& r : m_rings) {
        if (r.active) continue;
        r.pos = pos; r.color = c; r.r0 = r0; r.r1 = r1; r.dur = dur; r.t = 0.0f;
        r.active = true;
        return;
    }
}

void Fx::hit(sf::Vector2f pos, bool perfect) {
    const sf::Color c = perfect ? sf::Color(255, 220, 120) : sf::Color(190, 240, 180);
    burst(pos, c, perfect ? 14 : 8,
          perfect ? 900.0f : 620.0f,
          perfect ? 12.0f : 9.0f, 1200.0f, 0.55f);
    if (perfect)
        burst(pos, sf::Color(255, 255, 255), 6, 420.0f, 6.0f, -300.0f, 0.35f);

    ring(pos, perfect ? sf::Color(255, 230, 160, 220) : sf::Color(200, 255, 200, 180),
         10.0f, perfect ? 150.0f : 100.0f, 0.25f);

    shake(perfect ? 5.0f : 3.0f, 0.12f);
    hitStop(config::FX_HITSTOP_SEC);
}

void Fx::miss() {
    flash(sf::Color(255, 70, 70, 90), 0.28f);
    shake(7.0f, 0.20f);
}

void Fx::death(sf::Vector2f pos) {
    burst(pos, sf::Color(150, 95, 55), 40, 1100.0f, 15.0f, 1400.0f, 0.9f);
    burst(pos, sf::Color(255, 200, 90), 16,  700.0f,  8.0f,  900.0f, 0.7f);
    shake(22.0f, 0.55f);
    flash(sf::Color(190, 25, 25, 150), 0.55f);
    slowMo(0.22f, 0.55f);
}

void Fx::beatPulse() {
    m_beatGlow = 1.0f;
    ring(sf::Vector2f(config::SCREEN_CENTER_X, config::TIME_BAR_Y + 40.0f),
         sf::Color(255, 255, 255, 120), 30.0f, 220.0f, 0.30f);
}

void Fx::shake(float amp, float dur) {
    // 当前震动还剩多少强度；新震动更强或已衰减完才覆盖，避免连打时抖动被削弱
    const float remain = (m_shakeDur > m_shakeT)
                             ? m_shakeAmp * (1.0f - m_shakeT / m_shakeDur)
                             : 0.0f;
    if (amp < remain) return;
    m_shakeAmp = amp;
    m_shakeDur = dur;
    m_shakeT   = 0.0f;
}

void Fx::flash(sf::Color c, float dur) {
    m_flashColor = c;
    m_flashDur   = dur;
    m_flashT     = 0.0f;
}

void Fx::hitStop(float dur) {
    m_hitStopT = std::max(m_hitStopT, dur);
}

void Fx::slowMo(float scale, float dur) {
    m_slowScale = scale;
    m_slowT     = std::max(m_slowT, dur);
}
