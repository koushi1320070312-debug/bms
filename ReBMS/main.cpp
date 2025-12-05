#include "BMSGameApp.h"
#include <iostream>
#include <memory>
#include "BMSData.h" 
// ... (他の依存関係は省略) ...

// --------------------------------------------------------
// メイン関数
// --------------------------------------------------------
int main()
{
    // 1. ダミーデータの作成 (省略)
    // ...
    
    // 2. Rendererインスタンスの作成 (省略)
    std::unique_ptr<Renderer> renderer = std::make_unique<DummyRenderer>();

    // 3. アプリケーションの起動
    try {
        BMSGameApp app(std::move(renderer), dummy_data);
        std::cout << "Starting BMS Game Application..." << std::endl;
        
        // ★★★ React側から読み込んだ設定値を適用 ★★★
        const double initial_scroll_speed_from_config = 3.5; // 例: React/Firestoreから取得した値
        app.SetScrollSpeed(initial_scroll_speed_from_config);
        
        app.Run();
        
    } catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
