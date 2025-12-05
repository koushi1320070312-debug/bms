#include <iostream>
#include <vector>
#include <algorithm>
#include <SDL.h>     
#include <SDL_mixer.h> // ★ 追加: 音楽再生用のライブラリ

// =========================================================
// 移植性の高いゲームコア構造 (C++ サンプル - リズムゲーム実装)
// =========================================================

// ゲームの基本設定
// ... (変更なし)

// リズムゲームの描画設定
// ... (変更なし)

// SDL2固有のグローバルオブジェクト
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;
Mix_Music* g_music = nullptr; // ★ 追加: BGM用のポインタ

/**
 * @brief 音楽ノート/イベントの構造体
// ... (変更なし)

// プレイヤーの位置や状態を表す構造体
struct GameState {
    // リズムゲームコア
    std::vector<NoteEvent> chart_data; 
    // ★ 変更: 時間計測の基準をSDL_mixerの再生時間に変更する
    float game_time = 0.0f;            
    bool music_started = false; // ★ 追加: 音楽再生フラグ
    
    // スコアリング
// ... (変更なし)
};

// ゲームの状態
GameState state;

/**
 * @brief ノートの当たり判定の許容範囲 (秒)
 */
constexpr float JUDGEMENT_WINDOW = 0.15f; 

// ... (CreateDummyChart 関数 - 変更なし)

/**
 * @brief ゲームの初期化処理 (SDL2固有の実装 - SDL_mixerを追加)
 * @return true 成功
 * @return false 失敗
 */
bool Initialize() {
    std::cout << "--- Game Initialization ---" << std::endl;
    
    // 1. SDL2の初期化 (AUDIOを初期化に追加)
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_AUDIO) < 0) { // ★ 変更
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
    
    // 4. SDL_mixerの初期化
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "SDL_mixer could not initialize! Mix Error: " << Mix_GetError() << std::endl;
        // 音楽がなくてもゲームは動かせるが、今回は必須とする
        return false; 
    }
    
    // 5. 音楽のロード (仮に "music.ogg" が存在すると仮定)
    g_music = Mix_LoadMUS("music.ogg"); // ★ 変更: 実際のファイルパスを使用してください
    if (g_music == nullptr) {
        std::cerr << "Failed to load music! Mix Error: " << Mix_GetError() << std::endl;
        // 音楽がないとリズムゲームとして成り立たないため終了
        return false;
    }
    
    CreateDummyChart();
    
    std::cout << "Initialization successful." << std::endl;
    return true;
}

/**
 * @brief 入力処理 (SDL2固有の実装)
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
            
            // ★ 追加: スペースキーで音楽再生開始
            if (e.key.keysym.sym == SDLK_SPACE && !state.music_started) {
                if (Mix_PlayMusic(g_music, 0) == -1) { // ループなしで再生
                    std::cerr << "Failed to play music! Mix Error: " << Mix_GetError() << std::endl;
                } else {
                    state.music_started = true;
                    std::cout << "Music playback started!" << std::endl;
                }
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
                // 音楽が開始されていない場合は判定しない
                if (!state.music_started) return; 

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
    // SDL_mixerが再生中の場合のみ時間を更新する
    if (state.music_started && Mix_PlayingMusic()) { // ★ 変更: 音楽の再生時間に同期
        // Mix_GetMusicPosition()は倍精度浮動小数点数を返すため、floatに安全にキャスト
        double position = Mix_GetMusicPosition(g_music);
        state.game_time = static_cast<float>(position);
    } else if (state.music_started && !Mix_PlayingMusic()) {
        // 音楽が終了した場合の処理 (全ノート処理が終わっていればゲーム終了など)
        state.game_time = 999.0f; // 音楽終了を示す仮の値
    }
    
    // 音楽が始まっていない場合は、ノートがスクロールしないようにここで処理を中断しても良い
    if (!state.music_started) return;

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

    if (all_notes_processed && state.game_time > state.chart_data.back().time_seconds + 5.0f) { 
         // 最後のノートから5秒経ったら終了
         // state.running = false; 
    }
}

/**
 * @brief 描画処理 (SDL2固有の実装)
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
    // 音楽が始まっていない場合は「Press SPACE to Start」のようなメッセージを描画すべき
    if (state.music_started) {
        SDL_SetRenderDrawColor(g_renderer, 0xFF, 0xFF, 0x00, 0xFF); // ノートの色 (黄色)
        for (const auto& note : state.chart_data) {
            if (note.hit) continue; // 既に判定済みのノートは描画しない

            // 判定ラインからの距離 (ピクセル)
            float distance = (note.time_seconds - state.game_time) * SCROLL_SPEED;
            
            // 画面上でのY座標を計算
            float note_y = HIT_LINE_Y - distance; 
            
            // ノートが画面内にある場合のみ描画
            if (note_y > -NOTE_HEIGHT && note_y < SCREEN_HEIGHT + NOTE_HEIGHT) { // 画面外からも入ってくるように範囲を広げる
                SDL_Rect note_rect = {
                    LANE_START_X + note.lane * LANE_WIDTH + 5,  
                    (int)(note_y - NOTE_HEIGHT / 2.0f),
                    LANE_WIDTH - 10,
                    (int)NOTE_HEIGHT
                };
                SDL_RenderFillRect(g_renderer, &note_rect);
            }
        }
    }
    
    // 4. スコア表示 (簡易的にコンソールに出力、SDL_ttfの代替)
    // FPS表示は正確ではないため削除し、時間とスコアのみに
    std::cout << "\rGameTime: " << state.game_time << "s | Score: " << state.score << " | Combo: " << state.combo;
    if (!state.music_started) {
        std::cout << " (Press SPACE to Start)";
    }
    std::cout.flush();

    // 5. 描画バッファのフリップ（画面表示）
    SDL_RenderPresent(g_renderer);
}

/**
 * @brief 終了処理 (SDL2固有の実装 - SDL_mixerのクリーンアップを追加)
 */
void Cleanup() {
    std::cout << "\n--- Game Cleanup ---" << std::endl;
    
    // 1. SDL_mixerのリソース解放
    if (g_music) {
        Mix_FreeMusic(g_music);
        g_music = nullptr;
    }
    Mix_CloseAudio(); // ★ 追加: オーディオデバイスを閉じる
    
    // 2. SDLビデオ関連リソースの解放
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    
    // 3. SDL2の終了
    SDL_Quit();
    
    std::cout << "Resources released. Goodbye." << std::endl;
}

/**
 * @brief メイン関数とゲームループ
 */
int main(int argc, char* args[]) {
    // ... (変更なし - ループ内のロジックはUpdate/Renderに集約されているためそのまま)
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
