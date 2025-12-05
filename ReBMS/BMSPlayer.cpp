#include "BMSPlayer.h"
#include "BMSParser.h"
#include <iostream>

// ------------------------------------------------------------
// BMSPlayer コンストラクタ
// ------------------------------------------------------------
BMSPlayer::BMSPlayer()
{
    // レーン押下状態を初期化（一般的なIIDXの7レーンを仮定）
    for (int ch = 11; ch <= 17; ++ch)
        lane_key_state_[ch] = false;
}

// ------------------------------------------------------------
// BMS ファイル読み込み
// ------------------------------------------------------------
bool BMSPlayer::Load(const std::string& filepath)
{
    BMSParser parser;
    if (!parser.Parse(filepath, bms_data_))
    {
        std::cout << "[Error] Failed to parse BMS: " << filepath << "\n";
        return false;
    }

    Reset();
    return true;
}

// ------------------------------------------------------------
// Reset
// ------------------------------------------------------------
void BMSPlayer::Reset()
{
    current_time_ms_ = 0.0;
    is_playing_ = false;

    // レーン押下状態クリア
    for (auto& kv : lane_key_state_)
        kv.second = false;

    // LN 状態のリセット
    for (auto& st : ln_states)
        st.second.is_holding = false;
}

// ------------------------------------------------------------
// Start / Stop
// ------------------------------------------------------------
void BMSPlayer::Start()
{
    is_playing_ = true;
}

void BMSPlayer::Stop()
{
    is_playing_ = false;
}

// ------------------------------------------------------------
// Update : 毎フレーム呼ばれる
// ------------------------------------------------------------
void BMSPlayer::Update(double delta_ms)
{
    if (!is_playing_) return;

    current_time_ms_ += delta_ms;

    // 判定処理（COOL / GOOD / MISS / LN処理）
    UpdateJudges();

    // LN 終端チェック
    UpdateLNStates();

    // 描画用ノーツを更新
    draw_notes_cache_ = GetNotesForRendering(bms_data_.notes, current_time_ms_);
}

// ------------------------------------------------------------
// Render : SDL 上にノーツを描画
// ------------------------------------------------------------
void BMSPlayer::Render(SDL_Renderer* sdl_renderer)
{
    SDL_SetRenderDrawColor(sdl_renderer, 255, 255, 255, 255);

    for (const DrawNote& dn : draw_notes_cache_)
    {
        SDL_Rect rc;

        // X は仮固定（後でレーンごとに分けられる）
        rc.x = 600;
        rc.y = (int)dn.y_position;
        rc.w = 20;
        rc.h = dn.length > 0 ? (int)dn.length : 20;

        SDL_RenderFillRect(sdl_renderer, &rc);
    }
}

// ------------------------------------------------------------
// OnKeyDown : 押下イベント
// ------------------------------------------------------------
void BMSPlayer::OnKeyDown(int lane_channel)
{
    lane_key_state_[lane_channel] = true;

    // 判定処理
    JudgeKeyHit(bms_data_, lane_channel, current_time_ms_);

    // もし LN 開始ノーツなら ln_states に登録する処理を追加してもよい
}

// ------------------------------------------------------------
// OnKeyUp : 離鍵イベント
// ------------------------------------------------------------
void BMSPlayer::OnKeyUp(int lane_channel)
{
    lane_key_state_[lane_channel] = false;

    // LN BREAK 判定
    ProcessLNKeyRelease(lane_channel, current_time_ms_);
}

// ------------------------------------------------------------
// レーンキー押下状態を返す（Judge.cpp から呼ばれる）
// ------------------------------------------------------------
bool BMSPlayer::IsLaneKeyPressed(int lane_channel) const
{
    auto it = lane_key_state_.find(lane_channel);
    if (it == lane_key_state_.end()) return false;
    return it->second;
}

// ------------------------------------------------------------
// UpdateJudges : 自動 MISS（スクロールアウト）処理など
// ------------------------------------------------------------
void BMSPlayer::UpdateJudges()
{
    // 自動 MISS
    ProcessScrollOutMisses(bms_data_, current_time_ms_);
}

// ------------------------------------------------------------
// UpdateLNStates : LN 終端判定
// ------------------------------------------------------------
void BMSPlayer::UpdateLNStates()
{
    ProcessLNEnds(current_time_ms_);
}
