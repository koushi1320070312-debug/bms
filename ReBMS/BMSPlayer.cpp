#include "BMSPlayer.h"
#include <iostream>
#include <iomanip>
#include <sstream> // std::stod のためのインクルード

// --------------------------------------------------------
// コンストラクタ
// --------------------------------------------------------
BMSPlayer::BMSPlayer(const std::vector<Note>& initial_notes, double initial_bpm)
    : initial_bpm(initial_bpm)
{
    // 1. ノーツを時間順にソート（必須）
    notes = initial_notes;
    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
        return a.time_ms < b.time_ms;
    });

    // 2. BPM変化イベントとその他イベントを分離し、イベントキューを作成
    std::vector<Note> all_events;
    std::map<double, double> temp_bpm_changes;

    for (const auto& n : notes) {
        // チャンネル3はBPM変化
        if (n.channel == 0x03) { 
            try {
                // 16進数文字列を double に変換
                double new_bpm = std::stod(n.value, nullptr, 16); 
                temp_bpm_changes[n.time_ms] = new_bpm;
            } catch (...) {
                std::cerr << "Warning: Invalid BPM value in BMS data." << std::endl;
            }
        } 
        // チャンネル01-09はBGA/WAV/Layerイベント
        else if (n.channel >= 0x01 && n.channel <= 0x09) { 
            all_events.push_back(n);
        }
    }
    
    // 3. 初期BPMをマップに追加
    temp_bpm_changes[0.0] = initial_bpm;
    
    // 4. BPM変化マップを時間でソートして最終的なマップを作成
    std::vector<std::pair<double, double>> sorted_bpm(temp_bpm_changes.begin(), temp_bpm_changes.end());
    std::sort(sorted_bpm.begin(), sorted_bpm.end(), [](const auto& a, const auto& b) {
        return a.first < b.first;
    });
    for(const auto& pair : sorted_bpm) {
        bpm_changes[pair.first] = pair.second;
    }

    // 5. イベントキューにBGA/WAVイベントを追加
    event_queue = all_events;
}

// --------------------------------------------------------
// Setters
// --------------------------------------------------------
void BMSPlayer::SetJudgeOffset(double offset_ms)
{
    judge_offset_ms = offset_ms;
    std::cout << "Player Judge Offset Updated: " << std::fixed << std::setprecision(1) 
              << offset_ms << " ms" << std::endl;
}

void BMSPlayer::SetAutoPlayMode(bool is_auto)
{
    if (is_auto_play_mode != is_auto) {
        is_auto_play_mode = is_auto;
        auto_processed_notes.clear(); 
        std::cout << "Player AutoPlay Mode: " << (is_auto ? "ON" : "OFF") << std::endl;
    }
}

// --------------------------------------------------------
// 内部ロジック - 判定ウィンドウ
// --------------------------------------------------------

JudgeResult BMSPlayer::CalculateJudgment(double diff_ms) const
{
    // diff_ms: 判定補正後の時間差 (0.0msが理想)
    double abs_diff = std::abs(diff_ms);
    
    // 判定ウィンドウの定義 (ミリ秒) - 調整可能
    // 実際のBMS判定ウィンドウよりも簡略化している点に注意
    if (abs_diff <= 16.7) return JudgeResult::P_GREAT; // 約1/60秒
    if (abs_diff <= 33.3) return JudgeResult::GREAT;   // 約1/30秒
    if (abs_diff <= 83.3) return JudgeResult::GOOD;    // 約1/12秒
    if (abs_diff <= 166.7) return JudgeResult::BAD;   // 約1/6秒
    if (abs_diff <= 250.0) return JudgeResult::POOR;  // 約1/4秒
    return JudgeResult::MISS; 
}

double BMSPlayer::GetJudgeWindow(JudgeResult result) const
{
    // CalculateJudgment で使用した値を返す
    if (result == JudgeResult::P_GREAT) return 16.7; 
    if (result == JudgeResult::GREAT) return 33.3;
    if (result == JudgeResult::GOOD) return 83.3;
    if (result == JudgeResult::BAD) return 166.7;
    if (result == JudgeResult::POOR) return 250.0;
    return 0.0;
}

// --------------------------------------------------------
// イベント処理 (BGA/WAV/Layer)
// --------------------------------------------------------
void BMSPlayer::ProcessBGAEvent(const Note& event)
{
    // BGA (0x07) または Layer (0x04-0x06) イベント
    try {
        // value は16進数の文字列 (例: "01", "0A")
        int bmp_id = std::stoi(event.value, nullptr, 16); 
        
        if (event.channel == 0x07) { // BGA (Background Animation)
            current_bga_bmp_id = bmp_id;
            std::cout << "BGA Event: Set BGA to BMP ID " << bmp_id << std::endl;
        } else if (event.channel >= 0x04 && event.channel <= 0x06) { // Layer
            current_layer_bmp_ids[event.channel] = bmp_id;
            std::cout << "Layer Event: Channel " << event.channel << " Set to BMP ID " << bmp_id << std::endl;
        }

        // WAV再生イベント (0x01-0x09) - ここではコンソールログのみ
        if (event.channel >= 0x01 && event.channel <= 0x09) {
            std::cout << "WAV Playback Event: Play WAV ID " << event.value << std::endl;
            // ★ WebAssembly環境では、ここでJavaScriptのAudio APIを呼び出す必要がある
        }

    } catch (...) {
        std::cerr << "Warning: Invalid BMP/WAV ID format in event: " << event.value << std::endl;
    }
}

void BMSPlayer::ProcessEvents()
{
    // current_time_ms に達したイベントを処理
    auto it = event_queue.begin();
    while (it != event_queue.end()) {
        if (it->time_ms <= current_time_ms) {
            // BPM変化の処理はUpdate関数内の時間計算で間接的に行われる
            if (it->channel != 0x03) {
                ProcessBGAEvent(*it);
            }
            it = event_queue.erase(it); // 処理済みのイベントをキューから削除
        } else {
            break; // 時間順にソートされているため、これ以降のノーツは未到達
        }
    }
}

// --------------------------------------------------------
// オートプレイ実行ロジック
// --------------------------------------------------------
void BMSPlayer::ProcessAutoPlay()
{
    if (!is_auto_play_mode) return;

    // 1. ノーツの自動判定（押下処理）
    // ループ中にnotesが変更される可能性があるため、イテレータを使用
    for (auto it = notes.begin(); it != notes.end(); ) 
    {
        Note& n = *it;

        // LN終点ノーツ (0x6x) は押下処理では無視
        if (n.channel >= 0x61 && n.channel <= 0x69) {
            ++it;
            continue;
        }
        
        // 理想的な判定時刻: ノーツの時刻 + 判定オフセット
        // オートプレイなので、この時刻に正確に判定を出す
        double ideal_judge_time = n.time_ms + judge_offset_ms;
        
        // オート判定のトリガー: 理想時刻に達したとき
        if (current_time_ms >= ideal_judge_time)
        {
            // Judgeを呼び出し、自動でノーツを消費させる
            // 通常ノーツ (1x) または LN開始ノーツ (5x) のチャンネルで判定
            Judge(n.channel); 
            
            // Judgeが成功すればノーツは削除されているため、次の要素へ
            // (notes.erase(it)はJudge内で発生する)
            // ノーツがリストから削除されたので、eraseが返したイテレータに進む
            // ただし、Judgeの実装では、ここでイテレータ処理を外部に任せるため、
            // ここでitを削除するロジックをBMSPlayer.cppのJudgeから移動
            
            // Judgeが成功した（消費された）ノーツはここで削除する
            // JudgeがPOOR(早すぎる)でreturnした場合、ノーツは残る
            // Judge処理後、対象ノーツがリストに残っているかを確認し、残っていなければ次の要素へ
            // ノーツはJudge内で削除される前提で実装されているため、ここでは次の要素へ進む
            it = notes.erase(it);
        } else {
            // ノーツが時間範囲外（まだ早い）
            ++it;
        }
    }
    
    // 2. LN終点ノーツの自動解放処理 (JudgeKeyReleaseの実行)
    std::vector<int> lanes_to_release;
    for (const auto& kv : ln_states)
    {
        const LNState& st = kv.second;
        if (st.is_active)
        {
            // 理想的なLN終点時刻: 終点時刻 + 判定オフセット
            double ideal_release_time = st.end_time_ms + judge_offset_ms;

            if (current_time_ms >= ideal_release_time)
            {
                lanes_to_release.push_back(st.lane_channel);
            }
        }
    }

    // 解放処理を実行
    for (int lane : lanes_to_release)
    {
        JudgeKeyRelease(lane);
    }
}

// --------------------------------------------------------
// メイン更新ループ
// --------------------------------------------------------
void BMSPlayer::Update(double delta_time_ms)
{
    // 1. BPM変化に基づいて現在のBPMを決定（現在は時間ベースのゲーム進行のため未使用だが、BPM変化は考慮済み）
    // double current_bpm = initial_bpm;
    // auto it = bpm_changes.upper_bound(current_time_ms);
    // if (it != bpm_changes.begin()) {
    //     --it;
    //     current_bpm = it->second;
    // }
    
    // 2. 時間更新 (BMSノーツは絶対時間で計算されているため、実時間で進める)
    current_time_ms += delta_time_ms;

    // 3. イベントの処理 (BGA, WAV, BPM変化など)
    ProcessEvents();

    // 4. オートプレイロジックの実行
    if (is_auto_play_mode) {
        ProcessAutoPlay();
    }

    // 5. 判定期限切れのノーツのMISS処理
    ProcessScrollOutMisses();

    // 6. LN終点ノーツの強制MISS処理
    ProcessLNEnds();
}

// --------------------------------------------------------
// キー押下判定 (Judge)
// --------------------------------------------------------
void BMSPlayer::Judge(int lane_channel)
{
    // アクティブなLNがある場合、押下判定は無視する
    if (ln_states.count(lane_channel) && ln_states.at(lane_channel).is_active) {
        return;
    }

    // 1. レーンに対応するノーツを検索 (時間的に最も早いノーツ)
    Note* target_note = nullptr;
    size_t index = -1;
    
    for (size_t i = 0; i < notes.size(); ++i) {
        // 通常ノーツ(1x) または LN開始ノーツ(5x) かつ、レーンが一致
        if ((notes[i].channel == lane_channel || notes[i].channel == lane_channel + 0x40) && notes[i].time_ms > current_time_ms - GetJudgeWindow(JudgeResult::POOR) - std::abs(judge_offset_ms)) 
        {
             // 判定ウィンドウ内にある可能性のある最初のノーツ
             if (notes[i].channel == lane_channel || notes[i].channel == lane_channel + 0x40) {
                 target_note = &notes[i];
                 index = i;
                 break;
             }
        }
    }

    if (!target_note) { 
        // 叩くノーツがない（空押し）
        std::cout << "Judge Miss (Empty Press): Lane " << lane_channel << std::endl;
        combo = 0; // 空押しもコンボリセット対象とする
        return; 
    } 

    // 2. ノーツの理想時間からの時間差 (diff) を計算
    double raw_diff = current_time_ms - target_note->time_ms;
    
    // 3. ★ 判定オフセットの適用 (補正後の時間差)
    double offset_diff = raw_diff - judge_offset_ms; 

    // 4. 判定結果の計算
    JudgeResult result = CalculateJudgment(offset_diff); 

    // 5. 早すぎるMISS/POOR判定 (offset_diff < 0)
    if (offset_diff < 0 && result != JudgeResult::MISS) {
        // 判定ウィンドウ内だが早すぎる（POOR/BADなど）
        std::cout << "Judge POOR (Too Early): Lane " << lane_channel << ", Result: " << (int)result << std::endl;
        judge_counts[JudgeResult::POOR]++;
        combo = 0; // 早すぎる判定でもコンボリセット
        // ノーツは消さない（ノーツが判定ラインに到達していないため）
        return;
    } else if (offset_diff < 0 && result == JudgeResult::MISS) {
        // 早すぎるMISSの場合は、POORとして扱う
        std::cout << "Judge POOR (Very Early): Lane " << lane_channel << std::endl;
        judge_counts[JudgeResult::POOR]++;
        combo = 0;
        return;
    }

    // 6. 有効な判定が成立 (GREAT, GOOD, BAD, POOR - 遅い側)
    std::cout << "Judge Hit: Lane " << lane_channel << ", Result: " << (int)result << std::endl;
    judge_counts[result]++;

    // 7. スコア、コンボの更新
    if (result != JudgeResult::BAD && result != JudgeResult::POOR && result != JudgeResult::MISS) {
        combo++;
        if (combo > max_combo) max_combo = combo;
        score += 10; // 仮のスコア加算
    } else {
        combo = 0;
    }

    // 8. ノーツの消費とLN状態のセット
    if (target_note->channel >= 0x51 && target_note->channel <= 0x59) {
        // ロングノーツ開始 (51-59) の場合
        LNState newState;
        newState.lane_channel = target_note->channel - 0x40; // 1xチャンネルに戻す
        newState.start_time_ms = target_note->time_ms;

        // LN終点ノーツ (6x) を検索し、終点時間を取得
        bool ln_end_found = false;
        for (size_t j = index + 1; j < notes.size(); ++j) {
            // チャンネルが 6x であることを確認 (例: 51の次は61)
            if (notes[j].channel == target_note->channel + 0x10) { 
                 newState.end_time_ms = notes[j].time_ms;
                 newState.is_active = true;
                 newState.is_released_naturally = false;
                 ln_states[target_note->channel - 0x40] = newState;
                 
                 // LN終点ノーツ (6x) もリストから削除 (JudgeKeyReleaseで処理するため)
                 notes.erase(notes.begin() + j);
                 ln_end_found = true;
                 break;
            }
        }

        if (ln_end_found) {
             std::cout << "LN Started: Lane " << newState.lane_channel << " Ends at: " << newState.end_time_ms << std::endl;
        } else {
             std::cerr << "Error: LN Start Note found without corresponding LN End Note." << std::endl;
             // 終点がない場合は通常ノーツとして消費し、LN状態はセットしない
        }
    }
    
    // ノーツをリストから削除
    notes.erase(notes.begin() + index);
}

// --------------------------------------------------------
// キー離し判定 (LN終点処理)
// --------------------------------------------------------
void BMSPlayer::JudgeKeyRelease(int lane_channel)
{
    // LNチャンネル (1x) で管理されているLN状態を取得
    if (ln_states.count(lane_channel)) {
        LNState& st = ln_states.at(lane_channel);

        // LNがアクティブな状態であること
        if (st.is_active) {
            double raw_diff = current_time_ms - st.end_time_ms;
            
            // ★ 判定オフセットの適用
            double offset_diff = raw_diff - judge_offset_ms; 
            
            JudgeResult result = CalculateJudgment(offset_diff);

            // POOR, MISS以外は成功とする
            if (result != JudgeResult::POOR && result != JudgeResult::MISS) {
                // 成功
                std::cout << "LN Release Hit: Lane " << lane_channel << ", Result: " << (int)result << std::endl;
                judge_counts[result]++;
                // コンボは継続（LN終点は加点・コンボ加算はないが、ミスではないためリセットしない）
                st.is_released_naturally = true;
                
                // LN状態の削除
                ln_states.erase(lane_channel);
            } else {
                // 失敗（早すぎるリリース）
                std::cout << "LN Release POOR (Too Early/Late): Lane " << lane_channel << std::endl;
                judge_counts[JudgeResult::POOR]++;
                combo = 0; // 失敗はコンボリセット
                
                // 強制的にLN状態を解除
                st.is_active = false;
                ln_states.erase(lane_channel);
            }
        } else {
             // アクティブでないLN（既にミスなどで終了したLN）のリリースは無視
             ln_states.erase(lane_channel); 
        }
    }
}


// --------------------------------------------------------
// 判定期限切れノーツのMISS処理
// --------------------------------------------------------
void BMSPlayer::ProcessScrollOutMisses()
{
    // MISSウィンドウ（最も遅いPOORの範囲外）を過ぎたノーツを処理
    // POORのウィンドウ: 250.0ms
    const double MISS_WINDOW = GetJudgeWindow(JudgeResult::POOR); 
    
    // ループ中にnotesが変更される可能性があるため、イテレータを使用
    for (auto it = notes.begin(); it != notes.end(); ) 
    {
        Note& n = *it;
        
        // 判定対象のノーツか？ (通常ノーツ/LN開始ノーツ)
        if (n.channel >= 0x11 && n.channel <= 0x19 || n.channel >= 0x51 && n.channel <= 0x59)
        {
            // ノーツの最終判定時刻 (ノーツ時間 + MISSウィンドウ + オフセット)
            // この時刻を current_time_ms が超えたら MISS 確定
            double final_judge_time = n.time_ms + MISS_WINDOW + judge_offset_ms;

            if (current_time_ms > final_judge_time) {
                // MISS確定
                std::cout << "Note MISS (Scroll Out): Lane " << n.channel << std::endl;
                judge_counts[JudgeResult::MISS]++;
                combo = 0; // コンボリセット
                
                // LN開始ノーツ (5x) の場合は、LN終点ノーツ (6x) も連動して削除
                if (n.channel >= 0x51 && n.channel <= 0x59) {
                    // 次のノーツが終点ノーツ(6x)であれば、それも削除
                    if (std::next(it) != notes.end() && std::next(it)->channel == n.channel + 0x10) {
                        it = notes.erase(std::next(it)); 
                    }
                }

                // ノーツをリストから削除
                it = notes.erase(it);
            } else {
                ++it;
            }
        } else {
            // イベントノーツやLN終点ノーツ(6x)は無視
            ++it;
        }
    }
}

// --------------------------------------------------------
// LNの強制MISS処理
// --------------------------------------------------------
void BMSPlayer::ProcessLNEnds()
{
    // LNの終点時間がMISSウィンドウを過ぎたのに、まだアクティブなLNを強制終了する
    const double LN_MISS_WINDOW = GetJudgeWindow(JudgeResult::POOR);

    std::vector<int> lanes_to_remove;
    
    for (auto& kv : ln_states)
    {
        LNState& st = kv.second;
        
        if (st.is_active)
        {
            // LNの最終解放時刻 (終点時刻 + MISSウィンドウ + オフセット)
            double final_release_time = st.end_time_ms + LN_MISS_WINDOW + judge_offset_ms;

            if (current_time_ms > final_release_time) {
                // 強制MISS確定
                std::cout << "LN End MISS (Overtime): Lane " << st.lane_channel << std::endl;
                judge_counts[JudgeResult::MISS]++;
                combo = 0; // コンボリセット
                st.is_active = false; // 強制終了
                lanes_to_remove.push_back(st.lane_channel);
            }
        }
    }

    // 処理中にイテレータを壊さないように、後で削除
    for (int lane : lanes_to_remove) {
        ln_states.erase(lane);
    }
}
