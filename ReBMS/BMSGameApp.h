#pragma once

#include "BMSPlayer.h"
#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>

// ============================================================
// 定義と構造体
// ============================================================

// ノーツレンダリング用の情報 (BMSPlayer::Noteとは異なる、描画に特化した構造体)
struct RenderNote {
    int lane;           // レーン番号 (1-9)
    double time_ms;     // ノーツの絶対時間 (ms)
    double duration_ms; // LNの場合の長さ (通常ノーツの場合は 0.0)
    bool is_long_note;  // ロングノーツかどうか
    bool is_ln_end;     // LN終点ノーツかどうか
};

// ============================================================
// BMSGameApp クラス
// ============================================================

class BMSGameApp
{
private:
    // ゲームロジックエンジン
    std::unique_ptr<BMSPlayer> player; 

    // ゲーム内時間 (BMSPlayerと同期される)
    double game_time_ms = 0.0; 

    // 読み込まれたBMSファイルのデータ
    std::string title = "Untitled BMS";
    std::string artist = "Unknown Artist";
    std::map<std::string, std::string> wav_map; // WAV ID (16進数) -> ファイルパス
    std::map<std::string, std::string> bmp_map; // BMP ID (16進数) -> ファイルパス

    // レンダリング用のノーツデータ (プレイ中に消費されない静的なノーツ情報)
    std::vector<RenderNote> render_notes; 

public:
    // ------------------- 初期化と時間管理 ----------------------
    BMSGameApp();
    ~BMSGameApp() = default;

    /**
     * BMSデータをロードし、BMSPlayerとレンダリングデータを初期化する
     * 実際にはReact/JavaScript側でBMSをパースし、整形されたデータを渡す
     * @param initial_notes BMSPlayerに渡すノーツデータ
     * @param render_data レンダリング用の全ノーツデータ
     * @param initial_bpm 楽曲の初期BPM
     */
    void LoadBMS(
        const std::vector<Note>& initial_notes, 
        const std::vector<RenderNote>& render_data, 
        double initial_bpm,
        const std::map<std::string, std::string>& wavs,
        const std::map<std::string, std::string>& bmps,
        const std::string& title,
        const std::string& artist
    );

    /**
     * ゲーム時間を設定する (WebAudioの現在再生時間と同期)
     * @param time_ms 設定するゲーム絶対時間 (ms)
     */
    void SetCurrentTime(double time_ms);

    // ------------------- ゲームループと入力 ----------------------
    /**
     * ゲームの状態を更新する（BMSPlayerのUpdateを呼び出す）
     * @param delta_time_ms 前フレームからの経過実時間 (ms)
     */
    void Update(double delta_time_ms);
    
    /**
     * キー押下時の判定処理
     * @param lane_channel 押されたキーのチャンネル (11-19)
     */
    void KeyDown(int lane_channel); 
    
    /**
     * キー解放時のLN終点判定処理
     * @param lane_channel 離されたキーのチャンネル (11-19)
     */
    void KeyUp(int lane_channel);

    // ------------------- 設定変更 ----------------------
    /**
     * 判定オフセットを設定する
     */
    void SetJudgeOffset(double offset_ms); 
    
    /**
     * オートプレイモードを設定する
     */
    void SetAutoPlayMode(bool is_auto);

    // ------------------- Getters (React/Renderer用) --------------------------
    
    // ゲーム情報
    double GetCurrentTime() const { return game_time_ms; }
    int GetScore() const { return player ? player->GetScore() : 0; }
    int GetCombo() const { return player ? player->GetCombo() : 0; }
    std::string GetTitle() const { return title; }
    std::string GetArtist() const { return artist; }
    
    // レンダリング用データ
    const std::vector<RenderNote>& GetRenderNotes() const { return render_notes; }
    const std::map<std::string, std::string>& GetWAVs() const { return wav_map; }
    const std::map<std::string, std::string>& GetBMPs() const { return bmp_map; }
    
    // BGA/Layer情報
    int GetCurrentBgaId() const { return player ? player->GetCurrentBgaId() : 0; }
    const std::map<int, int>& GetCurrentLayerIds() const { return player ? player->GetCurrentLayerIds() : empty_layer_map; }

private:
    const std::map<int, int> empty_layer_map; // GetCurrentLayerIdsのフォールバック用
};
