// ============================================================================
//  chiptune_demo：把内置曲目渲染成 WAV，用来试听音色与旋律
//  不参与游戏构建，手工编译：
//      cl /EHsc /std:c++17 /utf-8 /O2 /I. tools\chiptune_demo.cpp Chiptune.cpp ^
//         /Fe:tools\chiptune_demo.exe /Fo:tools\
//      tools\chiptune_demo.exe tools\out
// ============================================================================
#include "Chiptune.h"

#include <cmath>
#include <cstdio>
#include <string>

using namespace chip;

// ---- 通用音色 ----
static Instrument lead25() {
    Instrument in;
    in.wave = Wave::Pulse; in.duty = 0.25f;
    in.attack = 0.003f; in.decay = 0.055f; in.sustain = 0.62f; in.release = 0.05f;
    in.volume = 0.55f;
    in.vibratoHz = 5.5f; in.vibratoSemis = 0.12f;
    return in;
}
static Instrument lead12() {
    Instrument in = lead25();
    in.duty = 0.125f; in.volume = 0.42f;
    return in;
}
static Instrument bassTri() {
    Instrument in;
    in.wave = Wave::Triangle;
    in.attack = 0.002f; in.decay = 0.06f; in.sustain = 0.85f; in.release = 0.04f;
    in.volume = 0.85f;
    return in;
}
static Instrument drumKit() {
    Instrument in;
    in.wave = Wave::Noise;
    in.volume = 1.0f;
    return in;
}

// ===========================================================================
//  1) 主菜单 BGM：90 BPM，柔和
// ===========================================================================
static Song makeMenuSong() {
    Song s;
    s.bpm = 90;

    Channel lead;
    lead.inst = lead25();
    lead.pattern =
        "E5 . A5 . C6 . A5 . E5 . A5 . C6 . B5 . "
        "D5 . F5 . A5 . F5 . D5 . F5 . A5 . G5 . "
        "C5 . E5 . G5 . E5 . C5 . E5 . G5 . E5 . "
        "B4 . D5 . G5 . D5 . B4 . D5 . G5 . F5 . ";

    Channel bass;
    bass.inst = bassTri();
    bass.pattern =
        "A2 - - - - - - - A2 - - - - - - - "
        "F2 - - - - - - - F2 - - - - - - - "
        "C3 - - - - - - - C3 - - - - - - - "
        "G2 - - - - - - - G2 - - - - - - - ";

    Channel hats;
    hats.inst = drumKit();
    hats.drums = true;
    hats.inst.volume = 0.25f;
    hats.pattern =
        "h . . . h . . . h . . . h . . . "
        "h . . . h . . . h . . . h . . . "
        "h . . . h . . . h . . . h . . . "
        "h . . . h . . . h . . . h . . . ";

    s.channels = { lead, bass, hats };
    return s;
}

// ===========================================================================
//  2) 关卡 BGM：150 BPM，A 小调 Am–F–C–G，动感推进
// ===========================================================================
static Song makeLevelSong() {
    Song s;
    s.bpm = 150;

    Channel lead;
    lead.inst = lead12();
    lead.pattern =
        "A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 "
        "F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4 "
        "C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 "
        "G4 B4 D5 B4 G4 B4 D5 B4 G4 B4 D5 B4 G4 B4 D5 B4 "
        "A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 A4 C5 E5 C5 "
        "F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4 F4 A4 C5 A4 "
        "C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 C5 E5 G5 E5 "
        "G4 B4 D5 B4 G4 B4 D5 B4 G4 B4 D5 B4 G4 B4 D5 B4 ";

    Channel bass;
    bass.inst = bassTri();
    bass.pattern =
        "A2 - - - A2 - - - A2 - - - A2 - - - "
        "F2 - - - F2 - - - F2 - - - F2 - - - "
        "C3 - - - C3 - - - C3 - - - C3 - - - "
        "G2 - - - G2 - - - G2 - - - G2 - - - "
        "A2 - - - A2 - - - A2 - - - A2 - - - "
        "F2 - - - F2 - - - F2 - - - F2 - - - "
        "C3 - - - C3 - - - C3 - - - C3 - - - "
        "G2 - - - G2 - - - G2 - - - G2 - - - ";

    Channel drums;
    drums.inst = drumKit();
    drums.drums = true;
    drums.pattern =
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . "
        "k . . . s . . . k . . . s . . . ";

    Channel hats;
    hats.inst = drumKit();
    hats.drums = true;
    hats.pattern =
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . "
        "h . h . h . h . h . h . h . h . ";

    s.channels = { lead, bass, drums, hats };
    return s;
}

// ===========================================================================
//  3) 调试用节拍轨：160 BPM，16 分音符贝斯 + 四踩底鼓，方便对拍
// ===========================================================================
static Song makeDebugSong() {
    Song s;
    s.bpm = 160;

    Channel lead;
    lead.inst = lead25();
    lead.pattern =
        "A4 . A4 . C5 . A4 . E5 . D5 . C5 . A4 . "
        "F4 . F4 . A4 . F4 . C5 . A4 . F4 . E4 . "
        "C5 . C5 . E5 . C5 . G5 . E5 . C5 . A4 . "
        "G4 . G4 . B4 . G4 . D5 . B4 . G4 . F4 . ";

    Channel bass;
    bass.inst = bassTri();
    bass.pattern =
        "A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 A2 "
        "F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 F2 "
        "C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 C3 "
        "G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 G2 ";

    Channel drums;
    drums.inst = drumKit();
    drums.drums = true;
    drums.pattern =
        "k . . . k . . . k . . . k . . . "
        "k . . . k . . . k . . . k . . . "
        "k . . . k . . . k . . . k . . . "
        "k . . . k . . . k . . . k . . . ";

    s.channels = { lead, bass, drums };
    return s;
}

// ---------------------------------------------------------------------------
static void report(const char* name, const std::vector<float>& v, int sr) {
    if (v.empty()) { std::printf("%-8s EMPTY\n", name); return; }
    double sum = 0.0;
    float  peak = 0.0f;
    for (float x : v) { sum += double(x) * double(x); peak = std::max(peak, std::fabs(x)); }
    const double rms = std::sqrt(sum / double(v.size()));
    std::printf("%-8s %6.2fs  peak=%.3f  rms=%.3f\n", name, double(v.size()) / sr, peak, rms);
}

int main(int argc, char** argv) {
    const std::string outDir = (argc > 1) ? argv[1] : ".";
    const int sr = 44100;

    struct Item { const char* file; const char* label; Song song; };
    Item items[] = {
        { "menu.wav",  "menu",  makeMenuSong()  },
        { "level.wav", "level", makeLevelSong() },
        { "debug.wav", "debug", makeDebugSong() },
    };

    for (const auto& it : items) {
        const std::vector<float> pcm = renderLooped(it.song, 2, sr);
        report(it.label, pcm, sr);
        const std::string path = outDir + "/" + it.file;
        if (!writeWav(path, pcm, sr)) std::printf("  !! 写入失败: %s\n", path.c_str());
        else                          std::printf("  -> %s\n", path.c_str());
    }
    return 0;
}
