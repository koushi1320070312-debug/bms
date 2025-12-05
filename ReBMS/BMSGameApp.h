#pragma once

#include <SDL2/SDL.h>
#include <memory>
#include <map>
#include <string>
#include <iostream>

#include "BMSPlayer.h"
#include "Renderer.h"

// ============================================================
// キーバインドの型定義
// ============================================================

// SDL_Keycode(int) → lane_channel (int)
using KeyToLaneMap = std::map<int, int>;

// lane_channel (int) → 表示名（例: "A", "S", "D"）
using LaneToKeyNameMap = std::map<int, std::string>;


// ============================================================
// BMSGameApp : SDL + BMSPlayer + Renderer を管理するアプリ本体
// ============================================================
class BMSGameApp
{
public:
    BMSGameApp();
    ~BMSGameApp();

    // SDL 初期化 / BMS 読み込み
    bool Init(const std::string& bms_filepath);

    // メインループ（ゲーム開始）
    void Run();

    // 終了処理
    void Shutdown();

    /**
     * キーバインドを設定する（外部から読み込んだ値を適用）
     */
    void SetKeybinds(
        const KeyToLaneMap& new_key_to_lane_map,
        const LaneToKeyNameMap& new_lane_to_key_name_map
    );

private:
    // ============================================================
    // SDL 関連
    // ============================================================
    SDL_Window* window_ = nullptr;
    SDL_Renderer* rendererSDL_ = nullptr;

    // メインループ制御
    bool is_running_ = false;

    // 時間計測
    Uint64 last_counter_ = 0;


    // ============================================================
    // BMSPlayer / Renderer
    // ============================================================
    std::unique_ptr<BMSPlayer> player;   // 判定・再生・LN管理など
    std::unique_ptr<Renderer> renderer;  // ノーツ描画管理


    // ============================================================
    // キーバインド
    // ============================================================
    KeyToLaneMap key_to_lane_map;          // SDL_Keycode → lane_channel
    LaneToKeyNameMap lane_to_key_name_map; // lane_channel → 表示用キー名


    // ============================================================
    // 内部処理（メインループ内で使用）
    // ============================================================
    void ProcessEvents();
    void Update(double delta_ms);
    void Render();
};
