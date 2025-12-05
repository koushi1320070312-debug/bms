#include "Judge.h"
#include <iostream>

// ---------------------------------------------
// ln_states の実体定義（Judge.h の extern を受ける）
// ---------------------------------------------
std::map<int, LNState> ln_states;

// ---------------------------------------------
// ログ出力（必要に応じて main 側で書き換え可）
// ---------------------------------------------
void LogLNResult(int lane, LNReleaseResult result)
{
    std::cout << "[LN] lane " << lane << " -> ";

    switch (result)
    {
    case LNReleaseResult::SUCCESS: std::cout << "SUCCESS\n"; break;
    case LNReleaseResult::BREAK:   std::cout << "BREAK\n";   break;
    case LNReleaseResult::MISS:    std::cout << "MISS\n";    break;
    default:                       std::cout << "NONE\n";    break;
    }
}

// ---------------------------------------------------------------
// JudgeKeyHit : キー押下判定
// ---------------------------------------------------------------
JudgeResult JudgeKeyHit(BMSData& data, int lane_channel, double current_time)
{
    Note* target_note = nullptr;
    size_t index = 0;

    // レーンに属する未判定ノーツを検索
    for (size_t i = 0; i < data.notes.size(); ++i)
    {
        Note& n = data.notes[i];

        if (n.channel == lane_channel)
        {
            target_note = &n;
            index = i;
            break;
        }
    }

    if (!target_note)
        return JudgeResult::NONE;

    double diff = current_time - target_note->time_ms;
    double abs_diff = std::abs(diff);

    // COOL
    if (abs_diff <= JUDGE_COOL_MS)
    {
        data.notes.erase(data.notes.begin() + index);
        return JudgeResult::COOL;
    }

    // GOOD
    if (abs_diff <= JUDGE_GOOD_MS)
    {
        data.notes.erase(data.notes.begin() + index);
        return JudgeResult::GOOD;
    }

    // MISS（遅すぎ）
    if (diff > JUDGE_GOOD_MS)
    {
        data.notes.erase(data.notes.begin() + index);
        return JudgeResult::MISS;
    }

    // 早すぎ → NONE（消費しない）
    return JudgeResult::NONE;
}

// ---------------------------------------------------------------
// ProcessScrollOutMisses : 判定ラインを通過したノーツの MISS 処理
// ---------------------------------------------------------------
void ProcessScrollOutMisses(BMSData& data, double current_time)
{
    const double miss_time_threshold = current_time - JUDGE_GOOD_MS;

    while (!data.notes.empty())
    {
        const Note& n = data.notes.front();

        if (n.time_ms > miss_time_threshold)
            break;

        // MISS ノーツとして削除
        std::cout << "[MISS] lane=" << n.channel << " time=" << n.time_ms << "\n";
        data.notes.erase(data.notes.begin());
    }
}

// ---------------------------------------------------------------
// ProcessLNKeyRelease : キー離した瞬間の BREAK 判定
// ---------------------------------------------------------------
LNReleaseResult ProcessLNKeyRelease(int lane_channel, double current_time)
{
    auto it = ln_states.find(lane_channel);
    if (it == ln_states.end())
        return LNReleaseResult::NONE;

    LNState& st = it->second;

    if (!st.is_holding)
        return LNReleaseResult::NONE;

    // 終了前に離した → BREAK
    if (current_time < st.end_time_ms)
    {
        st.is_holding = false;
        return LNReleaseResult::BREAK;
    }

    return LNReleaseResult::NONE;
}

// ---------------------------------------------------------------
// ProcessLNEnds : LN 終了時刻に到達したときの判定
// ---------------------------------------------------------------
void ProcessLNEnds(double current_time)
{
    for (auto& kv : ln_states)
    {
        int lane = kv.first;
        LNState& st = kv.second;

        if (!st.is_holding)
            continue;

        double end_t = st.end_time_ms;

        // 許容時間より遅れた → MISS
        if (end_t < current_time - JUDGE_GOOD_MS)
        {
            LogLNResult(lane, LNReleaseResult::MISS);
            st.is_holding = false;
            continue;
        }

        // まだ終了時刻ではない
        if (end_t > current_time + JUDGE_GOOD_MS)
            continue;

        // 終了判定：押していたか？
        if (IsKeyCurrentlyPressed(lane))
        {
            LogLNResult(lane, LNReleaseResult::SUCCESS);
        }
        else
        {
            LogLNResult(lane, LNReleaseResult::BREAK);
        }

        st.is_holding = false;
    }
}
