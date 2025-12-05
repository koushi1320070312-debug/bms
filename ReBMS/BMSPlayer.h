#pragma once

#include <string>
#include <map>
#include <vector>

#include <SDL2/SDL.h>

#include "BMSData.h"
#include "Judge.h"
#include "Renderer.h"

// ------------------------------------------------------------
// BMSPlayer
//  - BMSData を保持
//  - 再生時間の管理 (current_time_ms)
//  - キー入力状態の管理
//  - 判定処理 / 自動MISS / LN処理 の呼び出し
//  - 描画用ノーツリストの生成と Render 呼び出し
// ------------------------------------------------------------
class BMSPlayer
{
public:
    BMSPlayer();

    // BMSファイルの読み込み
    bool Load(const std::string& filepath);

    // 再生開始/停止/リセット
    void Reset();
    void Start();
    void Stop();

    // 毎フレーム呼ばれる更新処理
    // delta_ms : 経過時間（ミリ秒）
    void Update(double delta_ms);

    // 描画処理
    // SDL_Renderer に対してノーツを描画する
    void Render(SDL_Renderer* sdl_renderer);

    // キーイベント
    // lane_channel : BMSチャンネル (例: 11,12,...)
    void OnKeyDown(int lane_channel);
    void OnKeyUp(int lane_channel);

    // 現在の再生時間（ms）
    double GetCurrentTimeMs() const { return current_time_ms_; }

    // データ参照
    const BMSData& GetData() const { return bms_data_; }
    BMSData& GetData() { return bms_data_; }

    // レーンの押下状態を問い合わせ（Judge.cpp から呼ばれる想定）
    bool IsLaneKeyPressed(int lane_channel) const;

private:
    // BMS データ本体
    BMSData bms_data_;

    // 現在の再生時間（ms）
    double current_time_ms_ = 0.0;

    // 再生フラグ
    bool is_playing_ = false;

    // レーンごとのキー押下状態
    //  true : 押されている / false : 離されている
    std::map<int, bool> lane_key_state_;

    // 描画用一時バッファ
    std::vector<DrawNote> draw_notes_cache_;

    // 内部ヘルパ（Update 内で使用）
    void UpdateJudges();
    void UpdateLNStates();
};
