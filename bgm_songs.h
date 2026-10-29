// ============================================================================
//  bgm_songs.h：三首 BGM 的 Chiptune 谱面
// ============================================================================
//  设计原则（这条是「不吵」的关键，改曲子前先读）：
//
//  1. BGM 里【不放底鼓和军鼓】。节拍的重音由关卡自己的鼓声（synthDrum）提供，
//     那是判定的唯一听觉基准。BGM 再放一套鼓 = 两套节奏打架 = 玩家说的"吵闹"。
//  2. BGM 只负责和声与律动：贝斯（三角波）+ 旋律（方波）+ 反拍踩镲（噪声）。
//     踩镲落在每拍的"后半拍"，正好填在关卡鼓点的空隙里，两者互相咬合。
//  3. 关卡曲固定 8 小节 @ 60 BPM = 32 秒，与 RHYTHM_DRUM_BEATS = 32 严格等长，
//     循环时与鼓点永远对齐。
//  4. 音量三层：BGM(26) < 打击音效(60) < 鼓声(92)，见 config.h 的 BGM_VOLUME 等。
//
//  谱面语法：一个 token = 一行 = 1/16 拍；"-" 延音，"." 休止。
// ============================================================================
#ifndef CHOPPER_BGM_SONGS_H
#define CHOPPER_BGM_SONGS_H

#include "Chiptune.h"

namespace bgm {

// ---- 通用音色 ----
inline chip::Instrument makeLead(float duty, float volume) {
    chip::Instrument in;
    in.wave = chip::Wave::Pulse; in.duty = duty;
    in.attack = 0.004f; in.decay = 0.10f; in.sustain = 0.55f; in.release = 0.08f;
    in.volume = volume;
    in.vibratoHz = 5.0f; in.vibratoSemis = 0.10f;
    return in;
}
inline chip::Instrument makeBass() {
    chip::Instrument in;
    in.wave = chip::Wave::Triangle;
    in.attack = 0.003f; in.decay = 0.05f; in.sustain = 0.90f; in.release = 0.05f;
    in.volume = 0.70f;
    return in;
}
inline chip::Instrument makeHat(float volume) {
    chip::Instrument in;
    in.wave = chip::Wave::Noise;
    in.volume = volume;
    return in;
}

// 反拍踩镲：每拍的后半拍，一小节 16 行
inline const std::string kHatOffbeat =
    ". . h . . . h . . . h . . . h . ";

// 贝斯 8 分音符（每小节 8 个音）
inline const std::string kBassA = "A2 . A2 . A2 . A2 . A2 . A2 . A2 . A2 . ";
inline const std::string kBassF = "F2 . F2 . F2 . F2 . F2 . F2 . F2 . F2 . ";
inline const std::string kBassC = "C3 . C3 . C3 . C3 . C3 . C3 . C3 . C3 . ";
inline const std::string kBassG = "G2 . G2 . G2 . G2 . G2 . G2 . G2 . G2 . ";

inline std::string repeat8(const std::string& bar) {
    std::string s;
    for (int i = 0; i < 8; ++i) s += bar;
    return s;
}

} // namespace bgm

// ===========================================================================
//  关卡 A：砍树驱动 —— Am–F–C–G，4 分音符旋律，留白给鼓点
// ===========================================================================
inline chip::Song makeLevelASong() {
    using namespace bgm;
    chip::Song s;
    s.bpm = 60;
    s.gain = 0.85f;

    chip::Channel lead;
    lead.inst = makeLead(0.125f, 0.34f);
    lead.pattern =
        "A4 . . . C5 . . . E5 . . . D5 . . . "
        "F4 . . . A4 . . . C5 . . . A4 . . . "
        "C5 . . . E5 . . . G5 . . . E5 . . . "
        "G4 . . . B4 . . . D5 . . . B4 . . . "
        "A5 . . . G5 . . . E5 . . . C5 . . . "
        "F5 . . . E5 . . . C5 . . . A4 . . . "
        "G5 . . . F5 . . . E5 . . . C5 . . . "
        "D5 . . . B4 . . . G4 . . . B4 . . . ";

    chip::Channel bass;
    bass.inst = makeBass();
    bass.pattern = kBassA + kBassF + kBassC + kBassG + kBassA + kBassF + kBassC + kBassG;

    chip::Channel hats;
    hats.inst = makeHat(0.16f);
    hats.drums = true;
    hats.pattern = repeat8(kHatOffbeat);

    s.channels = { lead, bass, hats };
    return s;
}

// ===========================================================================
//  关卡 B：鼓点闪避 —— 同一套和声，旋律走 8 分音符，更紧张
// ===========================================================================
inline chip::Song makeLevelBSong() {
    using namespace bgm;
    chip::Song s;
    s.bpm = 60;
    s.gain = 0.85f;

    chip::Channel lead;
    lead.inst = makeLead(0.25f, 0.30f);
    lead.pattern =
        "A4 . C5 . E5 . C5 . A4 . C5 . B4 . C5 . "
        "F4 . A4 . C5 . A4 . F4 . A4 . G4 . A4 . "
        "C5 . E5 . G5 . E5 . C5 . E5 . D5 . E5 . "
        "G4 . B4 . D5 . B4 . G4 . B4 . A4 . G4 . "
        "A5 . G5 . E5 . C5 . A4 . C5 . E5 . D5 . "
        "F5 . E5 . C5 . A4 . F4 . A4 . C5 . A4 . "
        "G5 . F5 . E5 . C5 . G4 . B4 . D5 . B4 . "
        "D5 . B4 . G4 . D4 . G4 . B4 . D5 . G5 . ";

    chip::Channel bass;
    bass.inst = makeBass();
    bass.pattern = kBassA + kBassF + kBassC + kBassG + kBassA + kBassF + kBassC + kBassG;

    chip::Channel hats;
    hats.inst = makeHat(0.18f);
    hats.drums = true;
    hats.pattern = repeat8(kHatOffbeat);

    s.channels = { lead, bass, hats };
    return s;
}

// ===========================================================================
//  主菜单：90 BPM，只有贝斯 + 轻旋律，无打击乐
// ===========================================================================
inline chip::Song makeMenuSong() {
    using namespace bgm;
    chip::Song s;
    s.bpm = 90;
    s.gain = 0.70f;

    chip::Channel lead;
    lead.inst = makeLead(0.125f, 0.30f);
    lead.pattern =
        "A4 . . . E5 . . . C5 . . . E5 . . . "
        "F4 . . . C5 . . . A4 . . . C5 . . . "
        "C5 . . . G5 . . . E5 . . . G5 . . . "
        "G4 . . . D5 . . . B4 . . . D5 . . . ";

    chip::Channel bass;
    bass.inst = makeBass();
    bass.pattern =
        "A2 - - - - - - - A2 - - - - - - - "
        "F2 - - - - - - - F2 - - - - - - - "
        "C3 - - - - - - - C3 - - - - - - - "
        "G2 - - - - - - - G2 - - - - - - - ";

    s.channels = { lead, bass };
    return s;
}

#endif // CHOPPER_BGM_SONGS_H
