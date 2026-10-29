// ============================================================================
//  audio_preview：把【游戏里实际的音频混音】渲染成 WAV，用来验证音量配比
// ============================================================================
//  它复刻游戏里的三层音量：BGM(26%) + 关卡鼓声(92%) + 打击音效(60%)，
//  所以听起来应该和游戏里几乎一样，可以先用耳朵验收再进游戏。
//
//  用法：tools\build_preview.bat  然后  tools\audio_preview.exe tools\out
// ============================================================================
#include "../bgm_songs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using namespace chip;

// ---- 复刻 RhythmChopLevel::synthDrum（60BPM，每拍一个 150Hz 低频脉冲） ----
static std::vector<float> synthDrum(float bpm, int beats, int sr) {
    const float beat     = 60.0f / bpm;
    const int   frames   = static_cast<int>(beats * beat * sr);
    const int   pulseLen = static_cast<int>(0.18f * sr);
    std::vector<float> v(static_cast<size_t>(frames), 0.0f);
    for (int k = 0; k < beats; ++k) {
        const int start = static_cast<int>(k * beat * sr);
        double ph = 0.0;
        for (int j = 0; j < pulseLen && start + j < frames; ++j) {
            const double t   = j / static_cast<double>(sr);
            const double env = std::exp(-t * 24.0);
            const double f   = 75.0 + 155.0 * std::exp(-t * 45.0);
            ph += 6.283185307179586 * f / sr;
            v[static_cast<size_t>(start + j)] += static_cast<float>(20000.0 * env * std::sin(ph) / 32768.0);
        }
    }
    return v;
}

// ---- 三层混音：bgm*0.26 + drum*0.92 ----
static std::vector<float> mix(const std::vector<float>& bgm,
                              const std::vector<float>& drum) {
    const size_t n = std::max(bgm.size(), drum.size());
    std::vector<float> out(n, 0.0f);
    for (size_t i = 0; i < n; ++i) {
        const float b = (i < bgm.size())  ? bgm[i]  * 0.26f : 0.0f;
        const float d = (i < drum.size()) ? drum[i] * 0.92f : 0.0f;
        out[i] = std::max(-1.0f, std::min(1.0f, b + d));
    }
    return out;
}

static void report(const char* name, const std::vector<float>& v, int sr) {
    if (v.empty()) { std::printf("%-12s EMPTY\n", name); return; }
    double sum = 0.0; float peak = 0.0f;
    for (float x : v) { sum += double(x) * double(x); peak = std::max(peak, std::fabs(x)); }
    std::printf("%-12s %6.2fs  peak=%.3f  rms=%.3f\n",
                name, double(v.size()) / sr, peak, std::sqrt(sum / double(v.size())));
}

int main(int argc, char** argv) {
    const std::string outDir = (argc > 1) ? argv[1] : ".";
    const int sr = 44100;

    const Song menu   = makeMenuSong();
    const Song levelA = makeLevelASong();
    const Song levelB = makeLevelBSong();

    const std::vector<float> menuPcm = renderLooped(menu,   1, sr);
    const std::vector<float> aPcm    = renderLooped(levelA, 1, sr);
    const std::vector<float> bPcm    = renderLooped(levelB, 1, sr);
    const std::vector<float> drum    = synthDrum(60.0f, 32, sr);

    report("menu",   menuPcm, sr);
    report("levelA", aPcm,    sr);
    report("levelB", bPcm,    sr);
    report("drum32s", drum,   sr);

    const std::vector<float> mixA = mix(aPcm, drum);
    const std::vector<float> mixB = mix(bPcm, drum);
    report("mixA", mixA, sr);
    report("mixB", mixB, sr);

    writeWav(outDir + "/bgm_menu.wav",   menuPcm, sr);
    writeWav(outDir + "/bgm_levelA.wav", aPcm,    sr);
    writeWav(outDir + "/bgm_levelB.wav", bPcm,    sr);
    writeWav(outDir + "/drum_only.wav",  drum,    sr);
    writeWav(outDir + "/mix_levelA.wav", mixA,    sr);
    writeWav(outDir + "/mix_levelB.wav", mixB,    sr);
    std::printf("-> %s\n", outDir.c_str());
    return 0;
}
