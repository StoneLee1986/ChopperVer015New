// ============================================================================
//  Beatmap.cpp：JSON 解析器（内联，零第三方依赖）
// ============================================================================
#include "Beatmap.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstring>

// ---- 极简 JSON 解析（只支持我们需要的子集） ----
// 支持: { "key": number | "string" | [ ... ], "key": [ { "beat": n, "side": "Left" }, ... ] }
// 不支持: 转义字符、嵌套对象超过两层、注释

static void skipWhitespace(const char*& p) {
    while (*p && isspace(*p)) ++p;
}

static std::string parseString(const char*& p) {
    skipWhitespace(p);
    if (*p != '"') return "";
    ++p;
    std::string s;
    while (*p && *p != '"') {
        if (*p == '\\' && *(p + 1)) { s += *(++p); ++p; continue; }
        s += *p; ++p;
    }
    if (*p == '"') ++p;
    return s;
}

static double parseNumber(const char*& p) {
    skipWhitespace(p);
    char* end = nullptr;
    double v = strtod(p, &end);
    p = end;
    return v;
}

static bool expectChar(const char*& p, char c) {
    skipWhitespace(p);
    if (*p == c) { ++p; return true; }
    return false;
}

static Side parseSide(const std::string& s) {
    if (s == "Left" || s == "left" || s == "L") return Side::Left;
    if (s == "Right" || s == "right" || s == "R") return Side::Right;
    return Side::None;
}

Beatmap Beatmap::load(const std::string& path) {
    Beatmap bm;
    std::ifstream file(path);
    if (!file.is_open()) {
        // 文件不存在 → 返回空谱面（调用方回退概率模式）
        return bm;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string json = buf.str();
    const char* p = json.c_str();

    if (!expectChar(p, '{')) return bm;

    while (*p && *p != '}') {
        std::string key = parseString(p);
        if (!expectChar(p, ':')) break;

        if (key == "bpm") {
            bm.bpm = (float)parseNumber(p);
        }
        else if (key == "offsetSec") {
            bm.offsetSec = (float)parseNumber(p);
        }
        else if (key == "notes") {
            if (!expectChar(p, '[')) break;
            while (*p && *p != ']') {
                if (!expectChar(p, '{')) break;
                Note note;
                while (*p && *p != '}') {
                    std::string nkey = parseString(p);
                    if (!expectChar(p, ':')) break;
                    if (nkey == "beat") {
                        note.beat = (float)parseNumber(p);
                    }
                    else if (nkey == "side") {
                        std::string sval = parseString(p);
                        note.side = parseSide(sval);
                    }
                    else {
                        // 跳过未知值
                        parseString(p); // 或 parseNumber
                    }
                    skipWhitespace(p);
                    if (*p == ',') ++p;
                }
                if (*p == '}') ++p;
                bm.notes.push_back(note);
                skipWhitespace(p);
                if (*p == ',') ++p;
            }
            if (*p == ']') ++p;
        }

        skipWhitespace(p);
        if (*p == ',') ++p;
    }

    // 按 beat 排序（保险）
    std::sort(bm.notes.begin(), bm.notes.end(),
        [](const Note& a, const Note& b) { return a.beat < b.beat; });

    return bm;
}

bool Beatmap::save(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;

    file << "{\n";
    file << "  \"bpm\": " << bpm << ",\n";
    file << "  \"offsetSec\": " << offsetSec << ",\n";
    file << "  \"notes\": [\n";
    for (size_t i = 0; i < notes.size(); ++i) {
        const char* sideStr = (notes[i].side == Side::Left) ? "Left"
            : (notes[i].side == Side::Right) ? "Right" : "None";
        file << "    { \"beat\": " << notes[i].beat << ", \"side\": \"" << sideStr << "\" }";
        if (i + 1 < notes.size()) file << ",";
        file << "\n";
    }
    file << "  ]\n";
    file << "}\n";
    return true;
}

const Note* Beatmap::query(float currentBeat, float windowBeats) const {
    if (notes.empty()) return nullptr;
    float lo = currentBeat - windowBeats;
    float hi = currentBeat + windowBeats;
    // 二分查找（notes 已排序）
    int l = 0, r = (int)notes.size() - 1;
    while (l <= r) {
        int mid = (l + r) / 2;
        if (notes[mid].beat < lo) l = mid + 1;
        else if (notes[mid].beat > hi) r = mid - 1;
        else return &notes[mid];
    }
    // 检查 l 是否落在窗口内
    if (l < (int)notes.size() && notes[l].beat >= lo && notes[l].beat <= hi)
        return &notes[l];
    return nullptr;
}