// ============================================================================
//  Fx：程序化特效系统（零素材、零依赖，只用 SFML 图元）
// ============================================================================
//  设计：
//   - 关卡只发"事件"（onHit / onMiss / onBeat / onDeath），不关心实现
//   - 粒子池固定 360 个，环形池 16 个，全部栈上，不 new 不泄漏
//   - 屏幕震动只改 View 偏移，不碰任何游戏坐标
//   - hit-stop / 慢动作通过 timeScale 交给 Game 缩放 dt，逻辑代码零改动
//   - CRT 扫描线用顶点数组画，不用 shader / 不加载外部文件，可在设置里关
// ============================================================================
#ifndef CHOPPER_FX_H
#define CHOPPER_FX_H

#include "config.h"
#include <SFML/Graphics.hpp>
#include <vector>

class Fx {
public:
    Fx();

    // realDt：真实帧时间（秒）。内部按 timeScale 推进粒子，保证慢动作时粒子也慢
    void update(float realDt);

    // 场景层：粒子 + 冲击环（画在关卡场景之上、HUD 之下）
    void drawScene(sf::RenderTarget& rt);
    // 覆盖层：闪光 / 鼓点脉冲 / CRT（画在最上层）
    void drawOverlay(sf::RenderTarget& rt);

    sf::Vector2f shakeOffset() const { return m_shakeOffset; }
    float        timeScale()   const { return m_timeScale; }

    // ---- 事件接口 ----
    void hit(sf::Vector2f pos, bool perfect);   // 砍中：木屑 + 轻震 + 定格
    void miss();                                // 未踩拍：红闪 + 震动
    void death(sf::Vector2f pos);               // 死亡：爆发 + 强震 + 慢动作
    void beatPulse();                           // 每拍：画面脉冲（视觉节拍器）
    void ring(sf::Vector2f pos, sf::Color c, float r0, float r1, float dur);
    void dust(sf::Vector2f pos, int count);     // 尘土（树枝砸下）

    // ---- 基础效果（也可单独调用） ----
    void burst(sf::Vector2f pos, sf::Color c, int count, float speed, float size,
               float gravity, float life);
    void shake(float amp, float dur);
    void flash(sf::Color c, float dur);
    void hitStop(float dur);
    void slowMo(float scale, float dur);
    void fadeIn(float dur);        // 从黑场淡入（进关卡时用）
    void setDim(bool on);          // 结算/暂停时压暗画面（每帧调用即可）

private:
    struct Particle {
        sf::Vector2f pos, vel;
        sf::Color    color;
        float        life = 0.0f, maxLife = 1.0f, size = 8.0f, gravity = 0.0f;
        bool         active = false;
    };
    struct RingFx {
        sf::Vector2f pos;
        sf::Color    color;
        float        t = 0.0f, dur = 0.3f, r0 = 0.0f, r1 = 100.0f;
        bool         active = false;
    };

    void spawnParticle(sf::Vector2f pos, sf::Vector2f vel, float life,
                       float size, sf::Color c, float gravity);

    static const int kMaxParticles = 360;
    static const int kMaxRings     = 16;

    Particle m_particles[kMaxParticles];
    RingFx   m_rings[kMaxRings];
    int      m_cursor = 0;

    // 复用绘制对象，避免每帧构造
    sf::RectangleShape m_quad;
    sf::CircleShape    m_circle;
    sf::VertexArray    m_batch{ sf::Triangles };   // 粒子合批：一帧一次 draw call

    // 震动
    sf::Vector2f m_shakeOffset{ 0.0f, 0.0f };
    float        m_shakeAmp = 0.0f, m_shakeDur = 0.0f, m_shakeT = 0.0f;

    // 闪光
    sf::RectangleShape m_flashRect;
    sf::Color          m_flashColor{ 255, 255, 255, 0 };
    float              m_flashT = 0.0f, m_flashDur = 0.0f;

    // 鼓点脉冲
    float m_beatGlow = 0.0f;

    // 转场与压暗
    float m_fadeT     = 0.0f;   // 淡入已进行时间
    float m_fadeDur   = 0.0f;   // 淡入总时长（0 = 无淡入）
    bool  m_dimWanted = false;  // 是否要求压暗（结算/暂停）
    float m_dimAlpha  = 0.0f;   // 当前压暗程度 0~110

    // 时间缩放
    float m_hitStopT = 0.0f;
    float m_slowT = 0.0f, m_slowScale = 1.0f;
    float m_timeScale = 1.0f;

    float m_time = 0.0f;

    // CRT 扫描线（顶点数组，构造时建一次）
    sf::VertexArray m_scanlines;
    bool            m_crt = false;
};

#endif // CHOPPER_FX_H
