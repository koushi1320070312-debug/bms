#pragma once
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>
#include <memory>
#include <iostream>

// ============================================================
// 定義と構造体
// ============================================================

enum class JudgeResult { 
    P_GREAT, GREAT, GOOD, BAD, POOR, MISS, NONE // 判定結果の種類
};

// BMSファイルから読み込まれたノーツやイベントのデータ構造
struct Note {
    double time_ms; // ノーツやイベントの発生絶対時間 (ms)
    int channel;    // チャンネルID (11-17: 鍵盤, 51-57: LN開始, 61-67: LN終点, 01-09: WAV/BGA/Layer)
    std::string value; // WAV/BGAイベントの場合はID、ノーツの場合は未使用
};

// ロングノーツの状態管理
struct LNState {
    int lane_channel;          // LNが発生しているレーンチャンネル
    double start_time_ms;      // LN開始時刻
    double end_time_ms;        // LN終点時刻
    bool is_active;            // 現在キーが押されているか (押下判定成功後)
    bool is_released_naturally; // プレイヤーによる正常な解放があったか
};

// ============================================================
// BMSPlayer クラス
// ============================================================

class BMSPlayer
{
private:
    double current_time_ms = 0.0; // 現在のゲーム内時間 (ms)
    
    // BMS Data
    std::vector<Note> notes;            // 処理待ちの残りのノーツ (プレイ中に削除される)
    std::map<double, double> bpm_changes; // BPM変化ポイント (time_ms -> new_bpm)
    double initial_bpm;                 // 楽曲の初期BPM

    // Game State
    int score = 0;
    int combo = 0;
    int max_combo = 0;
    std::map<JudgeResult, int> judge_counts; // 判定結果のカウント
    std::map<int, LNState> ln_states;        // 現在アクティブなロングノーツの状態

    // ★ 設定値 (Reactから渡される)
    double judge_offset_ms = 0.0;       // 判定オフセット (ms)
    bool is_auto_play_mode = false;     // オートプレイモードが有効か

    // ★ BGA/Layer 表示状態 (レンダリング用)
    int current_bga_bmp_id = 0;                 // 現在表示中のBGAのBMP ID
    std::map<int, int> current_layer_bmp_ids;   // 現在表示中のLayerのBMP ID (Layer Channel -> BMP ID)

    // BGA/Layerイベント管理用
    std::vector<Note> event_queue;      // BGA/WAVなどのイベントキュー
    std::map<int, bool> auto_processed_notes; // オートプレイで処理済みのノーツを追跡（今回は未使用だが将来拡張用）

public:
    BMSPlayer(const std::vector<Note>& initial_notes, double initial_bpm);
    ~BMSPlayer() = default;

    // ------------------- API for App ----------------------
    /**
     * ゲームの状態を時間経過に基づいて更新する
     * @param delta_time_ms 前フレームからの経過実時間 (ms)
     */
    void Update(double delta_time_ms);
    
    /**
     * キー押下時のノーツ判定を行う
     * @param lane_channel 押下されたキーに対応するレーンチャンネル
     */
    void Judge(int lane_channel); 
    
    /**
     * キー解放時のLN終点判定を行う
     * @param lane_channel 解放されたキーに対応するレーンチャンネル
     */
    void JudgeKeyRelease(int lane_channel);

    // ------------------- Getters --------------------------
    int GetScore() const { return score; }
    int GetCombo() const { return combo; }
    bool IsAutoPlayMode() const { return is_auto_play_mode; }
    
    /**
     * 現在表示すべきBGAのBMP IDを取得
     */
    int GetCurrentBgaId() const { return current_bga_bmp_id; }
    
    /**
     * 現在表示すべきLayerのBMP IDマップを取得
     */
    const std::map<int, int>& GetCurrentLayerIds() const { return current_layer_bmp_ids; }
    
    // ------------------- Setters --------------------------
    /**
     * 判定オフセットを設定する (Reactからロード)
     */
    void SetJudgeOffset(double offset_ms); 
    
    /**
     * オートプレイモードを設定する
     */
    void SetAutoPlayMode(bool is_auto);

private:
    // ------------------- Internal Logic -------------------
    /**
     * ノーツの理想時間からの時間差に基づいて判定結果を計算
     */
    JudgeResult CalculateJudgment(double diff_ms) const;
    
    /**
     * 判定結果に対応する時間ウィンドウの大きさを取得
     */
    double GetJudgeWindow(JudgeResult result) const;
    
    /**
     * BGA/WAV/BPMなどのイベントを処理する
     */
    void ProcessEvents();
    
    /**
     * BGAまたはLayerイベントを処理し、表示状態を更新する
     */
    void ProcessBGAEvent(const Note& event);
    
    /**
     * 判定期限切れのノーツをMISSとして処理し、ノーツリストから削除する
     */
    void ProcessScrollOutMisses();
    
    /**
     * LNの終点判定時間を過ぎたアクティブなLNを強制MISSとして処理する
     */
    void ProcessLNEnds();
    
    /**
     * ★ オートプレイが有効な場合、ノーツを自動で判定する
     */
    void ProcessAutoPlay(); 
};
