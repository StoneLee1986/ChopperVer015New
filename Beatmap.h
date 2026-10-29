#pragma once
// ============================================================================
//  Beatmap.h：谱面数据结构（③ 谱面数据化的核心）
// ============================================================================
#ifndef CHOPPER_BEATMAP_H
#define CHOPPER_BEATMAP_H

#include "config.h"
#include <vector>
#include <string>

struct Note {
    float beat;       // 第几拍（浮点，支持半拍/三连音）
    Side side;        // Left / Right / None
};

class Beatmap {
public:
    float bpm = 60.0f;
    float offsetSec = 0.0f;
    std::vector<Note> notes;   // 按 beat 升序排列

    // 从 JSON 文件加载（文件不存在返回空 Beatmap，bpm=0）
    static Beatmap load(const std::string& path);

    // 保存到 JSON 文件（编辑器用）
    bool save(const std::string& path) const;

    // 查询：返回当前拍附近 [beat - window, beat + window] 内第一个匹配的 note
    // 找不到返回 nullptr
    const Note* query(float currentBeat, float windowBeats) const;

    // 返回谱面最后一拍（用于通关判定）
    float lastBeat() const {
        return notes.empty() ? 0.0f : notes.back().beat;
    }

    bool empty() const { return notes.empty(); }
};

#endif // CHOPPER_BEATMAP_H