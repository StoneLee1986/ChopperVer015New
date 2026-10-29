// ============================================================================
//  Chiptune.cpp：NES 风格合成器实现（纯 C++，无第三方依赖）
// ============================================================================
#include "Chiptune.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>

namespace chip {
namespace {

constexpr float kPi = 3.14159265358979323846f;

float midiToFreq(float midi) {
    return 440.0f * std::pow(2.0f, (midi - 69.0f) / 12.0f);
}

// ---------------------------------------------------------------------------
//  谱面 token
// ---------------------------------------------------------------------------
struct Token {
    enum Kind { Rest, Tie, Note, Drum };
    Kind kind = Rest;
    int  midi = 0;
    char drum = 0;
};

// 音名 → MIDI 音高（C4 = 60）
bool noteToMidi(const std::string& s, int& outMidi) {
    if (s.empty()) return false;
    size_t i = 0;
    int pc = -1;
    switch (s[0]) {
        case 'C': case 'c': pc = 0;  break;
        case 'D': case 'd': pc = 2;  break;
        case 'E': case 'e': pc = 4;  break;
        case 'F': case 'f': pc = 5;  break;
        case 'G': case 'g': pc = 7;  break;
        case 'A': case 'a': pc = 9;  break;
        case 'B': case 'b': pc = 11; break;
        default: return false;
    }
    ++i;
    if (i < s.size() && (s[i] == '#' || s[i] == 's')) { ++pc; ++i; }
    else if (i < s.size() && (s[i] == 'b' || s[i] == 'f')) { --pc; ++i; }
    if (i >= s.size() || s[i] < '0' || s[i] > '9') return false;
    int oct = 0;
    while (i < s.size() && s[i] >= '0' && s[i] <= '9') { oct = oct * 10 + (s[i] - '0'); ++i; }
    if (i != s.size()) return false;
    outMidi = (oct + 1) * 12 + pc;
    return outMidi >= 0 && outMidi < 128;
}

std::vector<Token> tokenize(const std::string& pat) {
    std::vector<Token> out;
    size_t i = 0;
    while (i < pat.size()) {
        while (i < pat.size() && std::isspace(static_cast<unsigned char>(pat[i]))) ++i;
        const size_t j = [&] {
            size_t k = i;
            while (k < pat.size() && !std::isspace(static_cast<unsigned char>(pat[k]))) ++k;
            return k;
        }();
        if (j > i) {
            const std::string t = pat.substr(i, j - i);
            Token tok;
            if (t == ".") {
                tok.kind = Token::Rest;
            } else if (t == "-") {
                tok.kind = Token::Tie;
            } else if (t.size() == 1 && (t[0] == 'k' || t[0] == 's' || t[0] == 'h' || t[0] == 'o')) {
                tok.kind = Token::Drum;
                tok.drum = t[0];
            } else if (noteToMidi(t, tok.midi)) {
                tok.kind = Token::Note;
            }
            out.push_back(tok);
        }
        i = j;
    }
    return out;
}

// ---------------------------------------------------------------------------
//  包络：线性 ADSR（参数极短，接近 NES 的硬切换感）
// ---------------------------------------------------------------------------
float envelope(float t, float dur, const Instrument& in) {
    const float a = std::max(in.attack, 1e-4f);
    const float d = std::max(in.decay, 1e-4f);
    const float r = std::max(in.release, 1e-4f);
    if (t < a) return t / a;
    if (t < a + d) return 1.0f - (1.0f - in.sustain) * ((t - a) / d);
    if (t < dur) return in.sustain;
    if (t < dur + r) return in.sustain * (1.0f - (t - dur) / r);
    return 0.0f;
}

// ---------------------------------------------------------------------------
//  15 位 LFSR 噪声（NES APU 的同款结构）
// ---------------------------------------------------------------------------
struct NoiseGen {
    uint16_t lfsr = 0x7FFFu;
    float    acc  = 0.0f;

    void step() {
        const uint16_t bit = static_cast<uint16_t>(((lfsr >> 0) ^ (lfsr >> 1)) & 1u);
        lfsr = static_cast<uint16_t>((lfsr >> 1) | (bit << 14));
    }
    float next(float rate, float sr) {
        acc += rate / sr;
        while (acc >= 1.0f) { step(); acc -= 1.0f; }
        return (lfsr & 1u) ? 1.0f : -1.0f;
    }
};

float oscillator(Wave wave, float phase, float duty, float noiseValue) {
    switch (wave) {
        case Wave::Pulse:    return (phase < duty) ? 1.0f : -1.0f;
        case Wave::Triangle: return 1.0f - 4.0f * std::fabs(phase - 0.5f);
        case Wave::Noise:    return noiseValue;
    }
    return 0.0f;
}

// ---------------------------------------------------------------------------
//  鼓组
// ---------------------------------------------------------------------------
void renderDrum(char type, long start, int n, int sr, std::vector<float>& out, NoiseGen& ng, float gain) {
    const long total = static_cast<long>(out.size());
    float phase = 0.0f;
    for (int s = 0; s < n; ++s) {
        const long idx = start + s;
        if (idx < 0 || idx >= total) continue;
        const float t = static_cast<float>(s) / static_cast<float>(sr);
        float v = 0.0f;

        if (type == 'k') {                       // 底鼓：三角波急速下滑
            const float env = std::exp(-t * 26.0f);
            const float f   = 45.0f + 150.0f * std::exp(-t * 45.0f);
            phase += f / static_cast<float>(sr);
            if (phase >= 1.0f) phase -= 1.0f;
            v = (1.0f - 4.0f * std::fabs(phase - 0.5f)) * env;
        } else if (type == 's') {                // 军鼓：噪声 + 一点低频
            const float env = std::exp(-t * 30.0f);
            phase += 190.0f / static_cast<float>(sr);
            if (phase >= 1.0f) phase -= 1.0f;
            const float body = (1.0f - 4.0f * std::fabs(phase - 0.5f)) * 0.35f;
            v = (ng.next(9000.0f, static_cast<float>(sr)) * 0.9f + body) * env;
        } else if (type == 'h') {                // 闭镲
            const float env = std::exp(-t * 90.0f);
            v = ng.next(24000.0f, static_cast<float>(sr)) * env * 0.45f;
        } else {                                 // 'o' 开镲
            const float env = std::exp(-t * 22.0f);
            v = ng.next(24000.0f, static_cast<float>(sr)) * env * 0.38f;
        }
        out[static_cast<size_t>(idx)] += v * 0.55f * gain;
    }
}

} // namespace

// ---------------------------------------------------------------------------
//  渲染
// ---------------------------------------------------------------------------
std::vector<float> render(const Song& song, int sampleRate) {
    const int sr = std::max(8000, sampleRate);

    std::vector<std::vector<Token>> toks;
    toks.reserve(song.channels.size());
    int rows = 0;
    for (const auto& c : song.channels) {
        toks.push_back(tokenize(c.pattern));
        rows = std::max(rows, static_cast<int>(toks.back().size()));
    }
    if (rows <= 0) return {};

    const float bpm     = static_cast<float>(std::max(20, song.bpm));
    const int   rpb     = std::max(1, song.rowsPerBeat);
    const float rowDur  = 60.0f / bpm / static_cast<float>(rpb);   // 一行的秒数
    const long  total   = std::lround(static_cast<float>(rows) * rowDur * static_cast<float>(sr));

    std::vector<float> mix(static_cast<size_t>(std::max(1L, total)), 0.0f);

    for (size_t ci = 0; ci < song.channels.size(); ++ci) {
        const Channel& ch = song.channels[ci];
        const auto&    tk = toks[ci];
        NoiseGen       ng;

        if (ch.drums) {
            for (size_t i = 0; i < tk.size(); ++i) {
                if (tk[i].kind != Token::Drum) continue;
                const long start = std::lround(static_cast<float>(i) * rowDur * static_cast<float>(sr));
                const int  n     = static_cast<int>(rowDur * static_cast<float>(sr) * 3.0f);
                renderDrum(tk[i].drum, start, n, sr, mix, ng, ch.inst.volume);
            }
            continue;
        }

        const Instrument& in = ch.inst;
        for (size_t i = 0; i < tk.size();) {
            if (tk[i].kind != Token::Note) { ++i; continue; }
            size_t j = i + 1;
            while (j < tk.size() && tk[j].kind == Token::Tie) ++j;
            const int lenRows = static_cast<int>(j - i);

            const float baseFreq = midiToFreq(static_cast<float>(tk[i].midi));
            const float durSec   = static_cast<float>(lenRows) * rowDur;
            const long  start    = std::lround(static_cast<float>(i) * rowDur * static_cast<float>(sr));
            const int   n        = static_cast<int>(rowDur * static_cast<float>(sr) * static_cast<float>(lenRows));

            const bool  arpOn = (in.arp[0] || in.arp[1] || in.arp[2] || in.arp[3]);
            float phase = 0.0f;

            for (int s = 0; s < n; ++s) {
                const long idx = start + s;
                if (idx < 0 || idx >= static_cast<long>(mix.size())) break;
                const float t   = static_cast<float>(s) / static_cast<float>(sr);
                const float env = envelope(t, durSec, in);
                if (env <= 0.0f) continue;

                float semis = 0.0f;
                if (arpOn) {
                    const int rowIn = static_cast<int>(t / rowDur);
                    const int step  = (rowIn / std::max(1, in.arpRateRows)) & 3;
                    semis += static_cast<float>(in.arp[step]);
                }
                if (in.vibratoHz > 0.0f && in.vibratoSemis != 0.0f)
                    semis += in.vibratoSemis * std::sin(2.0f * kPi * in.vibratoHz * t);

                const float f = baseFreq * std::pow(2.0f, semis / 12.0f);
                phase += f / static_cast<float>(sr);
                if (phase >= 1.0f) phase -= std::floor(phase);

                const float noiseValue = (in.wave == Wave::Noise) ? ng.next(f, static_cast<float>(sr)) : 0.0f;
                mix[static_cast<size_t>(idx)] += oscillator(in.wave, phase, in.duty, noiseValue) * env * in.volume;
            }
            i = j;
        }
    }

    // 软限幅 + 首尾 2ms 淡入淡出（避免循环接缝处的爆音）
    const float g = 0.55f * std::max(0.0f, song.gain);
    for (float& v : mix) v = std::tanh(v * g) * 0.92f;

    const long fade = std::min<long>(static_cast<long>(0.002f * static_cast<float>(sr)), static_cast<long>(mix.size()) / 2);
    for (long i = 0; i < fade; ++i) {
        const float k = static_cast<float>(i) / static_cast<float>(fade);
        mix[static_cast<size_t>(i)] *= k;
        mix[mix.size() - 1 - static_cast<size_t>(i)] *= k;
    }
    return mix;
}

std::vector<float> renderLooped(const Song& song, int loops, int sampleRate) {
    const std::vector<float> one = render(song, sampleRate);

    // 上限 64 次纯属防御失控分配，正常 BGM 只用 1~2 次
    const int    safeLoops = std::max(1, std::min(loops, 64));
    const size_t n         = static_cast<size_t>(safeLoops);

    std::vector<float> out;
    out.reserve(one.size() * n);
    for (size_t i = 0; i < n; ++i) {
        out.insert(out.end(), one.begin(), one.end());
    }
    return out;
}

// ---------------------------------------------------------------------------
//  WAV 导出（16-bit 单声道）
// ---------------------------------------------------------------------------
bool writeWav(const std::string& path, const std::vector<float>& samples, int sampleRate) {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;

    const uint32_t dataBytes = static_cast<uint32_t>(samples.size() * 2u);
    const uint16_t chans     = 1;
    const uint16_t bits      = 16;
    const uint32_t sr        = static_cast<uint32_t>(sampleRate);
    const uint32_t byteRate  = sr * chans * bits / 8;
    const uint16_t blockAlign = static_cast<uint16_t>(chans * bits / 8);

    auto u32 = [&f](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
    auto u16 = [&f](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };

    f.write("RIFF", 4);
    u32(36u + dataBytes);
    f.write("WAVE", 4);
    f.write("fmt ", 4);
    u32(16u);
    u16(1u);              // PCM
    u16(chans);
    u32(sr);
    u32(byteRate);
    u16(blockAlign);
    u16(bits);
    f.write("data", 4);
    u32(dataBytes);

    for (float v : samples) {
        v = std::max(-1.0f, std::min(1.0f, v));
        const int16_t s = static_cast<int16_t>(v * 32767.0f);
        f.write(reinterpret_cast<const char*>(&s), 2);
    }
    return f.good();
}

} // namespace chip
