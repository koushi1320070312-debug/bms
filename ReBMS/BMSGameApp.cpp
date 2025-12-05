// ... (既存の関数定義は省略) ...

// --------------------------------------------------------
// 入力処理（キーバインド対応）
// --------------------------------------------------------
void BMSGameApp::ProcessInput()
{
    if (player->IsAutoPlayMode()) {
        return; 
    }
    
    // 仮想のキー状態取得 (前フレームとの比較を行う)
    std::map<int, bool> current_key_states = renderer->GetCurrentKeyStates();

    // 既存のキー状態と現在のキー状態を比較して、エッジ検出を行う
    for (const auto& pair : key_to_lane_map)
    {
        int key_code = pair.first;      // ユーザーが押した物理キーのコード
        int lane_channel = pair.second; // それに対応するゲームのレーンチャンネル (例: 11-19)
        
        bool is_currently_down = current_key_states.count(key_code) && current_key_states.at(key_code);
        bool was_previously_down = previous_key_states.count(key_code) && previous_key_states.at(key_code);

        // キー押下（DOWNエッジ）
        if (is_currently_down && !was_previously_down)
        {
            player->Judge(lane_channel); // 判定ロジックへ
        }
        
        // キー離し（UPエッジ）
        else if (!is_currently_down && was_previously_down)
        {
            player->JudgeKeyRelease(lane_channel); // LN終点判定ロジックへ
        }
    }

    // 状態の更新
    previous_key_states = current_key_states;
}
