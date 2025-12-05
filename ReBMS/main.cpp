#include "BMSGameApp.h"
#include <iostream>
#include <chrono>
#include <thread>
#include <iomanip>

// --------------------------------------------------------
// グローバルなBMSGameAppインスタンス
// --------------------------------------------------------
std::unique_ptr<BMSGameApp> g_app = nullptr;

// --------------------------------------------------------
// 外部依存関数プロトタイプ (これらの関数を実装する必要があります)
// --------------------------------------------------------

/**
 * 
 * ネイティブ環境の初期化を行う（ウィンドウ、グラフィックス、オーディオ）
 * SDLやOpenGLなどのライブラリ初期化をここに記述します。
 * @return 成功した場合 true
 */
bool InitializeNativeEnvironment();

/**
 * 入力イベントを処理する（キー押下、ウィンドウ終了など）
 * @param running ウィンドウが閉じられた場合などに false に設定されます。
 */
void HandleInputAndEvents(bool& running);

/**
 * 画面にゲームの状態を描画する
 * @param app 描画に必要なデータを提供するゲームアプリインスタンス
 */
void RenderGameScreen(BMSGameApp* app);

/**
 * オーディオライブラリから現在の再生時間 (ms) を取得する
 */
double GetAudioPlaybackTime();

/**
 * ネイティブ環境の後処理（ウィンドウ破棄、ライブラリ解放）
 */
void CleanupNativeEnvironment();

// --------------------------------------------------------
// ユーティリティ関数
// --------------------------------------------------------

/**
 * ユーティリティ関数: std::string の 16進数値を int に変換
 */
int hex_to_int(const std::string& hex) {
    try {
        return std::stoi(hex, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

// --------------------------------------------------------
// Windows/Native アプリケーションのエントリーポイント
// --------------------------------------------------------

int main() {
    // 1. ネイティブ環境の初期化
    if (!InitializeNativeEnvironment()) {
        std::cerr << "Fatal Error: Failed to initialize native environment (Window/Graphics/Audio)." << std::endl;
        return 1;
    }
    
    // 2. ゲームアプリケーションの初期化
    g_app = std::make_unique<BMSGameApp>();
    std::cout << "BMSGameApp Initialized for Native Environment." << std::endl;
    
    // 3. BMSデータのロード（仮実装）
    std::vector<Note> notes;
    std::vector<RenderNote> render_data;
    std::map<std::string, std::string> wavs;
    std::map<std::string, std::string> bmps;

    // 仮データの設定（テスト用）
    // 例: 1秒地点 (1000ms) にノーツ
    notes.push_back({1000.0, 0x11, ""});
    render_data.push_back({1, 1000.0, 0.0, false, false});
    
    g_app->LoadBMS(notes, render_data, 120.0, wavs, bmps, "Test Song (Native)", "Native Developer");
    
    // 4. ゲームループの開始
    bool running = true;
    auto last_time = std::chrono::high_resolution_clock::now();
    
    std::cout << "Starting Native Game Loop..." << std::endl;

    while (running) {
        // --- (A) 時間の計測 ---
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> delta = current_time - last_time;
        double delta_time_ms = delta.count();
        last_time = current_time;
        
        // --- (B) プラットフォーム固有のイベント処理（入力、ウィンドウ操作など） ---
        HandleInputAndEvents(running);
        
        // --- (C) オーディオ同期とゲームロジックの更新 ---
        double audio_time_ms = GetAudioPlaybackTime(); // 実際のオーディオ時間を取得
        g_app->SetCurrentTime(audio_time_ms);
        
        // ゲームロジックの更新
        g_app->Update(delta_time_ms);
        
        // --- (D) レンダリング（描画） ---
        RenderGameScreen(g_app.get());

        // --- (E) フレームレート制御 ---
        if (delta_time_ms < 16.6) {
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(16.6 - delta_time_ms));
        }

        // デバッグログ
        static int frame_count = 0;
        frame_count++;
        if (frame_count % 60 == 0) {
            std::cout << "Time: " << std::fixed << std::setprecision(3) << audio_time_ms 
                      << "ms, Combo: " << g_app->GetCombo() << std::endl;
        }
    }

    // 5. 終了処理
    std::cout << "Game loop finished. Shutting down." << std::endl;
    g_app.reset();
    CleanupNativeEnvironment();
    return 0;
}

// --------------------------------------------------------
// 外部依存関数のダミー実装 (TODO: ここを実装する必要があります)
// --------------------------------------------------------

bool InitializeNativeEnvironment() {
    std::cout << "Native Init: Window, Graphics, Audio dummy initialized." << std::endl;
    // TODO: ここに SDL_Init, SDL_CreateWindow, Audio_Init などを実装
    return true;
}

void HandleInputAndEvents(bool& running) {
    // TODO: ここに SDL_PollEvent や Windows API のメッセージ処理を実装
    // 例: if (user_wants_to_quit) { running = false; }
    // 例: if (key_pressed_lane1) { g_app->KeyDown(0x11); }
}

void RenderGameScreen(BMSGameApp* app) {
    // TODO: ここに OpenGL/DirectX/SDL_Renderer などを使った描画コードを実装
    // app->GetRenderNotes() や app->GetCurrentBgaId() などを使って描画
}

double GetAudioPlaybackTime() {
    // TODO: ここにオーディオライブラリから実際の再生時間（ms）を取得するコードを実装
    // 現在は0を返すダミーです
    static double dummy_time = 0.0;
    dummy_time += 16.6; // 60FPSでの進行をシミュレート
    return dummy_time;
}

void CleanupNativeEnvironment() {
    std::cout << "Native Cleanup: Resources freed." << std::endl;
    // TODO: ここに SDL_Quit, Audio_Quit などを実装
}
