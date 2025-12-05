#include <iostream>
#include <vector>
#include <algorithm> // std::find_if, std::abs
#include <SDL.h>     // SDL2のコアライブラリをインクルード

// =========================================================
// 移植性の高いゲームコア構造 (C++ サンプル - リズムゲーム実装)
// 
// リズムゲームの核となるデータ構造と時間管理、描画ロジックを
// SDL2のフレームワークに組み込みました。
// =========================================================

// ゲームの基本設定
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TARGET_FPS = 60;
constexpr Uint32 FRAME_TIME_MS = 1000 / TARGET_FPS; 

// リズムゲームの描画設定
constexpr int LANE_COUNT = 4;
constexpr int LANE_WIDTH = 150;
constexpr int LANE_START_X = (SCREEN_WIDTH - (LANE_COUNT * LANE_WIDTH)) / 2;
constexpr int HIT_LINE_Y = 650; // ノートを叩く判定ラインのY座標
constexpr float NOTE_HEIGHT = 20.0f;
constexpr float SCROLL_SPEED = 500.0f; // 1秒あたりのピクセル移動量

// SDL2固有のグローバルオブジェクト
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;

/**
 * @brief 音楽ノート/イベントの構造体
 * time_seconds: 音楽開始からの時間（秒）。この時間に判定ラインに到達する
 * lane: どのレーンに出現するか (0からLANE_COUNT-1)
 * hit: 既に判定されたか
 */
struct NoteEvent {
    float time_seconds;
    int lane;
    bool hit = false;
};

// プレイヤーの位置や状態を表す構造体
struct GameState {
    // リズムゲームコア
    std::vector<NoteEvent> chart_data; // ノートデータの配列 (本来はBMSからロード)
    float game_time = 0.0f;            // 音楽開始からの経過時間（秒）
    
    // スコアリング
    int score = 0;
    int combo = 0;
    
    bool running = true; // メインループの実行状態
};

// ゲームの状態
GameState state;

/**
 * @brief ノートの当たり判定の許容範囲 (秒)
 */
constexpr float JUDGEMENT_WINDOW = 0.15f; // ±150ms

/**
 * @brief ダミーのノートデータを作成 (本来はBMSファイルをパースする)
 */
void CreateDummyChart() {
    // 0.5秒ごとにノートを配置
    for (int i = 0; i < 20; ++i) {
        float time = 1.0f + i * 0.5f; // 1秒後からスタート
        int lane = i % LANE_COUNT;
        state.chart_data.push_back({time, lane});
    }
    std::cout << "Dummy chart with " << state.chart_data.size() << " notes created." << std::endl;
}

/**
 * @brief ゲームの初期化処理 (SDL2固有の実装)
 * @return true 成功
 * @return false 失敗
 */
bool Initialize() {
    std::cout << "--- Game Initialization ---" << std::endl;
    
    // 1. SDL2の初期化
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 2. ウィンドウの作成
    g_window = SDL_CreateWindow(
        "Portable SDL2 Rhythm Game Core",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    if (g_window == nullptr) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }

    // 3. レンダラーの作成
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (g_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // ダミーチャートの作成
    CreateDummyChart();
    
    std::cout << "Initialization successful." << std::endl;
    return true;
}

/**
 * @brief 入力処理 (SDL2固有の実装)
 * * キーボード（レーンに対応）の入力を処理し、状態を更新します。
 */
void HandleInput() {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            state.running = false;
        }
        
        if (e.type == SDL_KEYDOWN) {
            if (e.key.keysym.sym == SDLK_ESCAPE) {
                state.running = false;
            }
            
            // レーン判定用のキーマップ (移植時はJoy-Con/Vitaボタンに置き換え)
            int pressed_lane = -1;
            switch (e.key.keysym.sym) {
                case SDLK_f: pressed_lane = 0; break; // Lane 1
                case SDLK_g: pressed_lane = 1; break; // Lane 2
                case SDLK_h: pressed_lane = 2; break; // Lane 3
                case SDLK_j: pressed_lane = 3; break; // Lane 4
            }

            if (pressed_lane != -1) {
                // 押されたレーンに未判定のノートがあるかチェック
                auto it = std::find_if(
                    state.chart_data.begin(),
                    state.chart_data.end(),
                    [&](const NoteEvent& note) {
                        return !note.hit && note.lane == pressed_lane;
                    }
                );

                if (it != state.chart_data.end()) {
                    float time_diff = std::abs(it->time_seconds - state.game_time);

                    if (time_diff <= JUDGEMENT_WINDOW) {
                        // 判定成功: Good, Perfectなどのロジックを追加可能
                        std::cout << "Hit! Lane: " << pressed_lane << ", Diff: " << time_diff * 1000.0f << "ms" << std::endl;
                        it->hit = true;
                        state.score += 100;
                        state.combo++;
                    } else if (it->time_seconds < state.game_time - JUDGEMENT_WINDOW) {
                        // タイミングが遅すぎたが、判定期間外のため見逃す (Miss判定はUpdateで処理)
                    }
                }
            }
        }
    }
}

/**
 * @brief ゲームロジックの更新
 * * 時間の進行、ミス判定（見逃し）、スコア更新などのロジックを実行します。
 * * @param delta_time 前フレームからの経過時間（秒）
 */
void Update(float delta_time) {
    // 1. ゲーム時間の進行 (ロジックの核)
    state.game_time += delta_time; 

    // 2. ミス判定（判定ラインを通り過ぎたノートの処理）
    for (auto& note : state.chart_data) {
        if (!note.hit) {
            // ノートの時間 - 判定時間窓 < 現在のゲーム時間 
            // つまり、判定ラインを通り過ぎてしまった場合
            if (note.time_seconds < state.game_time - JUDGEMENT_WINDOW) {
                std::cout << "Miss! Lane: " << note.lane << std::endl;
                note.hit = true; // 判定済みにする
                state.combo = 0; // コンボリセット
                // スコア減点などの処理をここに追加
            }
        }
    }
    
    // 3. ゲーム終了判定 (全てのノートが判定されたか)
    bool all_notes_processed = std::all_of(
        state.chart_data.begin(),
        state.chart_data.end(),
        [](const NoteEvent& note){ return note.hit; }
    );

    if (all_notes_processed && state.game_time > 5.0f) { // 最後に数秒待機
        // state.running = false; // デモでは自動終了させない
    }
}

/**
 * @brief 描画処理 (SDL2固有の実装)
 * * レーン、判定ライン、スクロールするノートを描画します。
 */
void Render() {
    // 1. 描画バッファのクリア（背景色で塗りつぶし）
    SDL_SetRenderDrawColor(g_renderer, 0x1A, 0x1A, 0x1A, 0xFF); // 暗い背景
    SDL_RenderClear(g_renderer);
    
    // --- 2. レーンと判定ラインの描画 ---
    
    // 判定ライン (青色のライン)
    SDL_SetRenderDrawColor(g_renderer, 0x00, 0xAA, 0xFF, 0xFF);
    SDL_RenderDrawLine(g_renderer, 
                       LANE_START_X, HIT_LINE_Y, 
                       LANE_START_X + LANE_COUNT * LANE_WIDTH, HIT_LINE_Y);

    // レーンの描画
    SDL_SetRenderDrawColor(g_renderer, 0x33, 0x33, 0x33, 0xFF); // レーンの境界線
    for (int i = 0; i <= LANE_COUNT; ++i) {
        int x = LANE_START_X + i * LANE_WIDTH;
        SDL_RenderDrawLine(g_renderer, x, 0, x, SCREEN_HEIGHT);
    }

    // --- 3. ノートの描画 ---
    SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0x00, 0xFF); // ノートの色 (黄色)
    for (const auto& note : state.chart_data) {
        if (note.hit) continue; // 既に判定済みのノートは描画しない

        // 判定ラインからの距離 (ピクセル)
        float distance = (note.time_seconds - state.game_time) * SCROLL_SPEED;
        
        // 画面上でのY座標を計算
        float note_y = HIT_LINE_Y - distance; 
        
        // ノートが画面内にある場合のみ描画
        if (note_y > 0 && note_y < SCREEN_HEIGHT + NOTE_HEIGHT) {
            SDL_Rect note_rect = {
                LANE_START_X + note.lane * LANE_WIDTH + 5,  // レーン内の左端 + 少し隙間
                (int)(note_y - NOTE_HEIGHT / 2.0f),
                LANE_WIDTH - 10,
                (int)NOTE_HEIGHT
            };
            SDL_RenderFillRect(g_renderer, &note_rect);
        }
    }
    
    // 4. スコア表示 (簡易的にコンソールに出力、本来はSDL_ttfで描画)
    std::cout << "\rTime: " << state.game_time << "s | Score: " << state.score << " | Combo: " << state.combo << " | FPS: " << 1.0f / (SDL_GetTicks() - SDL_GetTicks()) * 1000.0f << " (Approx)";
    std::cout.flush();

    // 5. 描画バッファのフリップ（画面表示）
    SDL_RenderPresent(g_renderer);
}

/**
 * @brief 終了処理 (SDL2固有の実装)
 */
void Cleanup() {
    std::cout << "\n--- Game Cleanup ---" << std::endl;
    
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    
    SDL_Quit();
    
    std::cout << "Resources released. Goodbye." << std::endl;
}

/**
 * @brief メイン関数とゲームループ
 */
int main(int argc, char* args[]) {
    Uint32 last_time = 0;
    
    if (!Initialize()) {
        Cleanup();
        return 1;
    }

    last_time = SDL_GetTicks();

    std::cout << "\n--- Game Loop Start ---" << std::endl;

    while (state.running) {
        Uint32 frame_start_time = SDL_GetTicks();
        
        // デルタタイム (前フレームからの経過時間。秒に変換)
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        // a. 入力処理
        HandleInput();

        // b. 更新処理 (ゲームロジック)
        Update(delta_time);

        // c. 描画処理
        Render();
        
        // d. フレームレート制御
        Uint32 frame_duration_ms = SDL_GetTicks() - frame_start_time;

        if (frame_duration_ms < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frame_duration_ms);
        }
    }

    std::cout << "--- Game Loop End ---" << std::endl;

    Cleanup();

    return 0;
}
