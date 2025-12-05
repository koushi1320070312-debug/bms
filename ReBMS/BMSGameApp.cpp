#include "BMSGameApp.h"
#include <emscripten/bind.h>
#include <memory>
#include <iostream>

// グローバルなBMSGameAppインスタンス
// JavaScriptからアクセスできるようにする
std::unique_ptr<BMSGameApp> g_app = nullptr;

/**
 * ゲームアプリケーションの初期化
 * JavaScriptから最初に呼び出される
 */
BMSGameApp* initialize_app() {
    // 既に初期化されていれば既存のインスタンスを返す
    if (!g_app) {
        g_app = std::make_unique<BMSGameApp>();
    }
    std::cout << "BMSGameApp instance created and ready." << std::endl;
    return g_app.get();
}

/**
 * ユーティリティ関数: std::string の 16進数値を int に変換
 * BMSデータ解析時に使用される可能性がある
 */
int hex_to_int(const std::string& hex) {
    try {
        return std::stoi(hex, nullptr, 16);
    } catch (...) {
        return 0; // 変換失敗時は0を返す
    }
}

// ============================================================
// Emscripten Binding (JavaScriptへの公開インターフェース)
// ============================================================

EMSCRIPTEN_BINDINGS(BMSGameApp_bindings) {
    // --------------------------------------------------------
    // 構造体 (JavaScriptへ公開)
    // --------------------------------------------------------
    
    // Note 構造体 (BMSPlayerのノーツデータ)
    emscripten::value_object<Note>("Note")
        .field("time_ms", &Note::time_ms)
        .field("channel", &Note::channel)
        .field("value", &Note::value);
        
    // RenderNote 構造体 (レンダリング用ノーツデータ)
    emscripten::value_object<RenderNote>("RenderNote")
        .field("lane", &RenderNote::lane)
        .field("time_ms", &RenderNote::time_ms)
        .field("duration_ms", &RenderNote::duration_ms)
        .field("is_long_note", &RenderNote::is_long_note)
        .field("is_ln_end", &RenderNote::is_ln_end);

    // --------------------------------------------------------
    // BMSGameApp クラス (JavaScriptへ公開)
    // --------------------------------------------------------
    emscripten::class_<BMSGameApp>("BMSGameApp")
        // メソッド
        .function("loadBMS", &BMSGameApp::LoadBMS)
        .function("setCurrentTime", &BMSGameApp::SetCurrentTime)
        .function("update", &BMSGameApp::Update)
        .function("keyDown", &BMSGameApp::KeyDown)
        .function("keyUp", &BMSGameApp::KeyUp)
        .function("setJudgeOffset", &BMSGameApp::SetJudgeOffset)
        .function("setAutoPlayMode", &BMSGameApp::SetAutoPlayMode)

        // ゲッター (プロパティとしてアクセス可能)
        .function("getCurrentTime", &BMSGameApp::GetCurrentTime)
        .function("getScore", &BMSGameApp::GetScore)
        .function("getCombo", &BMSGameApp::GetCombo)
        .function("getTitle", &BMSGameApp::GetTitle)
        .function("getArtist", &BMSGameApp::GetArtist)
        .function("getRenderNotes", &BMSGameApp::GetRenderNotes)
        .function("getWAVs", &BMSGameApp::GetWAVs)
        .function("getBMPs", &BMSGameApp::GetBMPs)
        .function("getCurrentBgaId", &BMSGameApp::GetCurrentBgaId)
        .function("getCurrentLayerIds", &BMSGameApp::GetCurrentLayerIds)
        ;

    // --------------------------------------------------------
    // グローバル関数 (JavaScriptへ公開)
    // --------------------------------------------------------
    emscripten::function("initializeApp", &initializeApp, emscripten::allow_raw_pointers());
    emscripten::function("hexToInt", &hex_to_int);
}

// main関数はEmscripten環境では通常使用されないが、慣例として残す
int main() {
    // main関数内での処理は基本的に不要
    return 0;
}
