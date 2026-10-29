// ============================================================================
//  Chiptune：FC/红白机风格的程序化音乐引擎（纯 C++，不依赖 SFML）
// ============================================================================
//  设计目标（对应需求 5、6）：
//   - 没有 .mid、没有 .wav 素材，音轨就是几行文本谱，文件极小
//   - 方波(3 种占空比) / 三角波 / 噪声 四个声道，就是 NES 的音源配置
//   - 编译期数据 → 运行时合成成 sf::SoundBuffer，可无缝循环
//
//  用法：
//      chip::Song s;
//      s.bpm = 150;
//      s.channels.push_back({ leadInst, "A4 C5 E5 C5 ..." });
//      auto samples = chip::renderLooped(s, 2);   // 单声道 float，-1..1
//      sf::SoundBuffer buf;
//      buf.loadFromSamples(...);                  // 见 ChiptuneSound.h（后续）
//
//  谱面语法：空白分隔，每个 token = 一行（默认 1/16 拍）
//      C4 D#4 Bb3   音符（# 升号，b 降号）
//      -            延音（把上一个音符拉长一行）
//      .            休止
//  鼓通道（drums = true）：
//      k 底鼓   s 军鼓   h 闭镲   o 开镲
// ============================================================================
#ifndef CHOPPER_CHIPTUNE_H
#define CHOPPER_CHIPTUNE_H

#include <string>
#include <vector>

namespace chip {

// ---- 波形 ----
enum class Wave {
    Pulse,      // 方波：主旋律/和声，占空比决定音色（12.5% 尖、25% 亮、50% 厚）
    Triangle,   // 三角波：NES 的贝斯声道，低音厚实不刺耳
    Noise       // 噪声：鼓组与打击乐
};

// ---- 音色（一个声道一套） ----
struct Instrument {
    Wave  wave      = Wave::Pulse;
    float duty      = 0.5f;    // 脉冲占空比：0.125 / 0.25 / 0.5
    float attack    = 0.004f;  // 起音（秒）
    float decay     = 0.08f;   // 衰减到 sustain（秒）
    float sustain   = 0.55f;   // 持续电平 0..1
    float release   = 0.05f;   // 离键余音（秒）
    float volume    = 0.8f;    // 声道音量
    float vibratoHz = 0.0f;    // 揉弦频率，0 = 关闭
    float vibratoSemis = 0.0f; // 揉弦深度（半音）
    int   arpRateRows = 1;     // 琶音换步间隔（行）
    int   arp[4] = { 0, 0, 0, 0 }; // 琶音半音偏移，全 0 = 关闭
};

// ---- 一个声道 = 音色 + 文本谱 ----
struct Channel {
    Instrument  inst;
    std::string pattern;
    bool        drums = false;   // true 时 pattern 用 k/s/h/o
};

// ---- 一首曲子 ----
struct Song {
    int bpm = 120;
    int rowsPerBeat = 4;          // 1 行 = 1/4 拍 = 16 分音符
    std::vector<Channel> channels;
};

// 渲染成单声道 float 采样（-1..1）。长度 = 最长声道的行数 × 行时长。
std::vector<float> render(const Song& song, int sampleRate = 44100);

// 同上，但把整首重复 loops 次（用于无缝循环的 BGM）
std::vector<float> renderLooped(const Song& song, int loops, int sampleRate = 44100);

// 写 16-bit 单声道 WAV（试听/导出用）
bool writeWav(const std::string& path, const std::vector<float>& samples, int sampleRate = 44100);

} // namespace chip

#endif // CHOPPER_CHIPTUNE_H
