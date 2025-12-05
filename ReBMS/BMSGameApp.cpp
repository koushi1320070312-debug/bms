#include "BMSGameApp.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

// --------------------------------------------------------
// コンストラクタ
// --------------------------------------------------------
BMSGameApp::BMSGameApp()
{
    // 初期化時、playerはnullのままにしておく
    // LoadBMSが呼ばれたときに初期化する
    std::cout << "BMSGameApp Initialized. Awaiting BMS data loading." << std::endl;
}

// --------------------------------------------------------
// BMSデータロード
// --------------------------------------------------------
void BMSGameApp::LoadBMS(
    const std::vector<Note>& initial_notes, 
    const std::vector<RenderNote>& render_data, 
    double initial_bpm,
    const std::map<std::string, std::string>& wavs,
    const std::map<std::string, std::string>& bmps,
    const std::string& new_title,
    const std::string& new_artist
)
{
    // 1. BMSPlayerの初期化 (ゲームロジックのコア)
    try {
        // BMSPlayerに、判定処理用のノーツデータと初期BPMを渡して初期化
        player = std::make_unique<BMSPlayer>(initial_notes, initial_bpm);
        std::cout << "BMSPlayer initialized with " << initial_notes.size() << " initial notes." << std::endl;
        
        // 2. メタデータとレンダリングデータの保存
        title = new_title;
        artist = new_artist;
        wav_map = wavs;
        bmp_map = bmps;
        render_notes = render_data;
        game_time_ms = 0.0; // 時間をリセット
        
        std::cout << "BMS Loaded: " << title << " by " << artist << std::endl;
        std::cout << "Total render notes: " << render_notes.size() << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error during BMS loading/BMSPlayer initialization: " << e.what() << std::endl;
        player.reset(); // エラー時はプレイヤーを破棄
    } catch (...) {
        std::cerr << "Unknown error during BMS loading/BMSPlayer initialization." << std::endl;
        player.reset();
    }
}

// --------------------------------------------------------
// 時間管理 (WebAudioとの同期)
// --------------------------------------------------------
void BMSGameApp::SetCurrentTime(double time_ms)
{
    // WebAudioの現在再生時間に基づいてゲーム時間を設定
    // これにより、ゲームの進行がオーディオ再生と厳密に同期される
    // BMSPlayerは内部でこの時間を参照してイベントやミス処理を行う
    game_time_ms = time_ms;
}

// --------------------------------------------------------
// ゲームループと入力
// --------------------------------------------------------
void BMSGameApp::Update(double delta_time_ms)
{
    if (!player) return;

    // BMSPlayerに現在の時間を設定し、ロジックの更新を委譲する
    // delta_time_msは、もしSetCurrentTimeが使えない場合に備えて残すが、
    // 主にBMSPlayerが内部で使うノーツスクロールアウト処理のトリガーとして機能する
    try {
        // 現在、BMSPlayer::Updateはdelta_time_msを受け取る設計なので、そのまま渡す
        // BMSPlayerは内部でこの時間に基づいてイベント処理を行う
        // ※ 厳密な同期のためには、SetCurrentTimeで時間を設定した後、
        //   BMSPlayerのロジック更新（イベント処理やオートプレイ）だけを呼び出す方が良いが、
        //   ここでは既存のUpdateメソッドを維持する
        player->Update(delta_time_ms);
        
    } catch (const std::exception& e) {
        std::cerr << "Error during BMSPlayer Update: " << e.what() << std::endl;
    }
}

void BMSGameApp::KeyDown(int lane_channel)
{
    if (!player) return;
    std::cout << "Key Down: Lane " << lane_channel << std::endl;
    // チャンネル11-17をBMSPlayerのJudgeに転送
    player->Judge(lane_channel);
}

void BMSGameApp::KeyUp(int lane_channel)
{
    if (!player) return;
    std::cout << "Key Up: Lane " << lane_channel << std::endl;
    // チャンネル11-17をBMSPlayerのJudgeKeyReleaseに転送
    player->JudgeKeyRelease(lane_channel);
}

// --------------------------------------------------------
// 設定変更
// --------------------------------------------------------
void BMSGameApp::SetJudgeOffset(double offset_ms)
{
    if (!player) return;
    player->SetJudgeOffset(offset_ms);
}

void BMSGameApp::SetAutoPlayMode(bool is_auto)
{
    if (!player) return;
    player->SetAutoPlayMode(is_auto);
}
