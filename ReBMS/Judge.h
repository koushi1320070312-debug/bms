#pragma once
#include <vector>
#include <map>
#include <cmath>
#include "BMSData.h"

// -------------------------------------
// 判定結果
// -------------------------------------
enum class JudgeResult {
    NONE,
    COOL,
    GOOD,
    BAD,
    MISS
};

// LN 離鍵結果
enum class LNReleaseResult {
    NONE,
    SUCCESS,
    BREAK,
    MISS
};

// -------------------------------------
// LN 状態（BMSData.h にも宣言する）
// -------------------------------------
struct LNState {
    bool is_holding = false;
    double end_time_ms = 0.0;
};

// -------------------------------------
// 外部変数（Judge.cpp で定義する）
// -------------------------------------
extern std::map<int, LNState> ln_states;

// -------------------------------------
// 判定ウィンドウ定数
// （Config にまとめても良いが、とりあえずここに）
// -------------------------------------
const double JUDGE_COOL_MS = 30.0;
const double JUDGE_GOOD_MS = 60.0;

// -------------------------------------
// 関数宣言
// -------------------------------------

// キー押下判定
JudgeResult JudgeKeyHit(BMSData& data, int lane_channel, double current_time);

// 判定ラインを通過してしまった MISS チェック
void ProcessScrollOutMisses(BMSData& data, double current_time);

// キー離鍵の処理（LN BREAK 判定）
LNReleaseResult ProcessLNKeyRelease(int lane_channel, double current_time);

// LN 終端判定（LN SUCCESS / BREAK / MISS）
void ProcessLNEnds(double current_time);

// 外部から呼び出すための API（ログ用）
void LogLNResult(int lane, LNReleaseResult result);

// 現在キーが押されているかどうか（main.cpp 側で実装）
bool IsKeyCurrentlyPressed(int lane_channel);

