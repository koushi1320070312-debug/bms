#pragma once

#include <string>
#include <vector>
#include <map>

// ================================
// 1ノーツ / 時間制御イベント分の情報
// ================================
struct Note
{
    double time_ms;         // 発生時刻（ミリ秒）
    int measure;            // 小節番号
    int channel;            // チャンネル番号（11, 03, 08 など）

    std::string wav_id;     // 通常ノーツ / BGM / BGA 用 ID ("01" など)
    std::string def_id;     // BPM変化・STOP 用の定義ID ("xx")

    int end_measure;        // ロングノート終了小節番号
    double end_pos;         // 小節内の終了位置 (0.0～1.0)
    double end_time_ms;     // 終了時刻（ミリ秒）

    double pos_raw;         // 0.0～1.0
};

// =======================================
// BMS データ本体
// =======================================
struct BMSData
{
    // ------------------------------
    // ヘッダ情報
    // ------------------------------
    std::string title;        // #TITLE
    std::string subtitle;     // #SUBTITLE
    std::string artist;       // #ARTIST
    std::string genre;        // #GENRE
    std::string stagefile;    // #STAGEFILE
    int difficulty = 0;       // #DIFFICULTY
    int play_mode = 0;        // #PLAYMODE

    double initial_bpm = 120.0;

    // ------------------------------
    // 定義情報
    // ------------------------------
    std::map<std::string, std::string> wav_files; // #WAVxx
    std::map<std::string, std::string> bmp_files; // #BMPxx

    std::map<std::string, double> bpm_table;   // #BPMxx → BPM値
    std::map<std::string, double> stop_table;  // #STOPxx → 停止量

    // ------------------------------
    // 小節倍率 (#MEASURE)
    // ------------------------------
    std::map<int, double> measure_rate_map;

    // ------------------------------
    // ノーツ・イベント一覧
    // ------------------------------
    std::vector<Note> notes;

    // ★ロード済みリソース
    std::map<std::string, int> loaded_wavs;
    std::map<std::string, int> loaded_bmps;
    int loaded_stagefile = -1;

};
