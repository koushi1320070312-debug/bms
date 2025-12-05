#include <iostream>
#include <SDL.h> // SDL2のコアライブラリをインクルード

// =========================================================
// 移植性の高いゲームコア構造 (C++ サンプル - SDL2実装)
// 
// SDL2を使用して、ウィンドウ、入力、描画の抽象化レイヤーを
// 実際に実装しています。Update内のゲームロジックは
// プラットフォーム非依存のままです。
// =========================================================

// ゲームの基本設定
constexpr int SCREEN_WIDTH = 1280;
constexpr int SCREEN_HEIGHT = 720;
constexpr int TARGET_FPS = 60;
constexpr Uint32 FRAME_TIME_MS = 1000 / TARGET_FPS; // SDLではUint32を使用

// SDL2固有のグローバルオブジェクト
SDL_Window* g_window = nullptr;
SDL_Renderer* g_renderer = nullptr;

// プレイヤーの位置や状態を表す構造体
struct GameState {
    float player_x = SCREEN_WIDTH / 2.0f;
    float player_y = SCREEN_HEIGHT / 2.0f;
    float speed = 200.0f; // 1秒あたりのピクセル移動量に変更 (Update関数でdelta_timeを使うため)
    int score = 0;
    bool running = true; // メインループの実行状態
};

// ゲームの状態
GameState state;

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
        "Portable SDL2 Game Core",
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

    // 3. レンダラーの作成 (ハードウェアアクセラレーションを使用)
    g_renderer = SDL_CreateRenderer(
        g_window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC // VSyncを有効にしてFPSをディスプレイに合わせる
    );
    if (g_renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // 描画色を黒に設定
    SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF);

    std::cout << "Screen: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << std::endl;
    std::cout << "Assets loaded." << std::endl;
    std::cout << "Initialization successful." << std::endl;
    return true;
}

/**
 * @brief 入力処理 (SDL2固有の実装)
 * * キーボード、コントローラーの入力を処理し、状態を更新します。
 */
void HandleInput() {
    SDL_Event e;
    // イベントキューにあるすべてのイベントを処理
    while (SDL_PollEvent(&e) != 0) {
        // ウィンドウを閉じるイベント
        if (e.type == SDL_QUIT) {
            state.running = false;
        }
        
        // キーボードのキーダウンイベント
        if (e.type == SDL_KEYDOWN) {
            switch (e.key.keysym.sym) {
                case SDLK_ESCAPE:
                    state.running = false;
                    break;
                // 他のキー入力はUpdate関数内で処理するためここでは省略
            }
        }
    }
    
    // 現在のキーボードの状態を取得 (同時押し対応)
    const Uint8* current_keys = SDL_GetKeyboardState(NULL);

    // W, A, S, D または矢印キーによるプレイヤーの移動処理
    // 移動処理はロジック層(Update)で行うため、ここでは入力フラグを立てるなどの処理が理想ですが、
    // シンプルにするためここではロジックを直接更新しています。
    // Updateでdelta_timeを使うため、ここではロジックを更新しません。Updateで処理します。
}

/**
 * @brief ゲームロジックの更新
 * * 物理演算、衝突判定、AI、スコア更新などのロジックをフレームレートに合わせて実行します。
 * この関数内のコードは、プラットフォームに依存しない純粋なC++ロジックとして設計します。
 * * @param delta_time 前フレームからの経過時間（秒）
 */
void Update(float delta_time) {
    // スコアの更新 (デモとして)
    state.score = (int)SDL_GetTicks() / 100; // 経過時間でスコアが増加

    const Uint8* current_keys = SDL_GetKeyboardState(NULL);
    float move_distance = state.speed * delta_time; // デルタタイムを使って移動距離を計算

    // W, A, S, D または矢印キーによるプレイヤーの移動
    if (current_keys[SDL_SCANCODE_W] || current_keys[SDL_SCANCODE_UP]) {
        state.player_y -= move_distance;
    }
    if (current_keys[SDL_SCANCODE_S] || current_keys[SDL_SCANCODE_DOWN]) {
        state.player_y += move_distance;
    }
    if (current_keys[SDL_SCANCODE_A] || current_keys[SDL_SCANCODE_LEFT]) {
        state.player_x -= move_distance;
    }
    if (current_keys[SDL_SCANCODE_D] || current_keys[SDL_SCANCODE_RIGHT]) {
        state.player_x += move_distance;
    }


    // プレイヤーの位置を画面内に制限
    if (state.player_x < 25) state.player_x = 25; // 描画サイズを考慮
    if (state.player_x > SCREEN_WIDTH - 25) state.player_x = SCREEN_WIDTH - 25;
    if (state.player_y < 25) state.player_y = 25;
    if (state.player_y > SCREEN_HEIGHT - 25) state.player_y = SCREEN_HEIGHT - 25;
}

/**
 * @brief 描画処理 (SDL2固有の実装)
 * * ゲーム画面にオブジェクトを描画します。
 */
void Render() {
    // 1. 描画バッファのクリア（背景色で塗りつぶし）
    SDL_SetRenderDrawColor(g_renderer, 0x00, 0x00, 0x00, 0xFF); // 黒に設定
    SDL_RenderClear(g_renderer);

    // 2. 描画コマンド
    
    // プレイヤーを描画 (赤色の正方形)
    SDL_Rect player_rect = { 
        (int)(state.player_x - 25), 
        (int)(state.player_y - 25), 
        50, 
        50 
    };
    SDL_SetRenderDrawColor(g_renderer, 0xFF, 0x00, 0x00, 0xFF); // 赤色に設定
    SDL_RenderFillRect(g_renderer, &player_rect);
    
    // 3. 描画バッファのフリップ（画面表示）
    SDL_RenderPresent(g_renderer);
}

/**
 * @brief 終了処理 (SDL2固有の実装)
 * * 初期化で確保したリソースの解放や、ライブラリ/SDKの終了処理を行います。
 */
void Cleanup() {
    std::cout << "--- Game Cleanup ---" << std::endl;
    
    // 1. SDLリソースの破棄
    if (g_renderer) {
        SDL_DestroyRenderer(g_renderer);
        g_renderer = nullptr;
    }
    if (g_window) {
        SDL_DestroyWindow(g_window);
        g_window = nullptr;
    }
    
    // 2. SDL2の終了
    SDL_Quit();
    
    std::cout << "Resources released. Goodbye." << std::endl;
}

/**
 * @brief メイン関数とゲームループ
 */
int main(int argc, char* args[]) { // SDLのmain関数形式に合わせる
    // フレーム時間計測のための変数
    Uint32 last_time = 0;
    
    // 1. 初期化
    if (!Initialize()) {
        Cleanup();
        return 1;
    }

    // 初期化後、最初の時刻を取得
    last_time = SDL_GetTicks();

    std::cout << "\n--- Game Loop Start ---" << std::endl;

    // 2. ゲームループ
    while (state.running) {
        Uint32 frame_start_time = SDL_GetTicks();
        
        // デルタタイム (前フレームからの経過時間。Update関数に渡すために秒に変換)
        Uint32 current_time = SDL_GetTicks();
        float delta_time = (current_time - last_time) / 1000.0f;
        last_time = current_time;

        // a. 入力処理
        HandleInput();

        // b. 更新処理 (ゲームロジック)
        // delta_time (秒) を使用することで、フレームレートに依存しない移動を実現
        Update(delta_time);

        // c. 描画処理
        Render();
        
        // d. フレームレート制御 (TARGET_FPSに合わせて待機)
        Uint32 frame_duration_ms = SDL_GetTicks() - frame_start_time;

        if (frame_duration_ms < FRAME_TIME_MS) {
            SDL_Delay(FRAME_TIME_MS - frame_duration_ms);
        }
    }

    std::cout << "--- Game Loop End ---" << std::endl;

    // 3. 終了処理
    Cleanup();

    return 0;
}
