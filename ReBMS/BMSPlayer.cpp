#include "BMSPlayer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <iomanip>

// --------------------------------------------------------
// 定数
// --------------------------------------------------------

// 判定許容範囲 (ms) - 例として±80ms
constexpr double JUDGE_RANGE_WONDERFUL = 20.0;
constexpr double JUDGE_RANGE_GREAT = 40.0;
constexpr double JUDGE_RANGE_GOOD = 80.0;

// --------------------------------------------------------
// コンストラクタ
// --------------------------------------------------------
BMSPlayer::BMSPlayer(const std::vector<Note>& initial_notes, double initial_bpm)
    : notes(initial_notes), bpm(initial_bpm), score(0), combo(0), max_combo(0), judge_offset_ms(0.0), is_auto_play(false)
{
    // 初期化時にノーツを時間順にソートしておく
    std::sort(notes.begin(), notes.end(), [](const Note& a, const Note& b) {
        return a.time_ms < b.time_ms;
    });

    // 初期BPMイベントを生成
    // 最初のノーツが始まる前の時間 0ms に初期BPMを設定
    ProcessBPMEvent(0.0, initial_bpm);
    
    std::cout << "BMSPlayer initialized. Total notes: " << notes.size() << std::endl;
}

// --------------------------------------------------------
// メイン更新処理
// --------------------------------------------------------
void BMSPlayer::Update(double delta_time_ms)
{
    // 1. ノーツのミス判定とオートプレイ処理
    ProcessMissedNotes();

    // 2. BGA/BPM イベントの処理 (時間同期は BMSGameApp::SetCurrentTime で行われる)
    ProcessEvents();

    // 3. オートプレイ時のノーツ自動処理
    if (is_auto_play) {
        AutoPlayJudge();
    }
}

// --------------------------------------------------------
// 判定処理 (キーダウン時)
// --------------------------------------------------------
void BMSPlayer::Judge(int lane_channel)
{
    if (is_auto_play) return;

    // 1. 現在時間
    // game_time_ms は BMSGameApp::SetCurrentTime で設定されている
    double current_time = game_time_ms + judge_offset_ms;

    // 2. 判定対象のノーツを検索
    // 未処理で、現在の時間に近いノーツを探す
    // `notes` リストは時間順にソートされているため、二分探索で効率化できるが、
    // ここでは単純な線形探索で、未処理のノーツの先頭から最も近いものを探す。
    
    int best_note_index = -1;
    double min_time_diff = JUDGE_RANGE_GOOD; // 最もゆるい判定範囲で初期化

    for (size_t i = 0; i < notes.size(); ++i) {
        if (!notes[i].is_judged && notes[i].channel == lane_channel) {
            double time_diff = std::abs(notes[i].time_ms - current_time);

            if (time_diff <= min_time_diff) {
                // ロングノーツの終点ノーツは、キーダウン判定では対象外とする
                // ただし、BMSフォーマットによってはLN終点も同じチャンネルを持つため、
                // 厳密にはLN終点フラグもチェックすべきだが、ここでは簡単な実装に留める。
                // 実際にはLN終点ノーツは JudgeKeyRelease で処理される。
                if (notes[i].is_long_note_end) continue; 

                // 現在、最も時間の近いノーツ
                min_time_diff = time_diff;
                best_note_index = i;
            } else if (notes[i].time_ms > current_time + min_time_diff) {
                // ノーツの時間が判定範囲を外れ始めたら、これ以上探す必要はない
                break; 
            }
        }
    }

    // 3. 判定実行
    if (best_note_index != -1) {
        Note& judged_note = notes[best_note_index];
        PerformJudge(judged_note, current_time);

        // WAVイベントの処理 (実際のオーディオ再生をシミュレート)
        ProcessWAVEvent(judged_note.channel, judged_note.value);
    } else {
        std::cout << "Input: Miss (No note found in range for channel " << std::hex << lane_channel << std::dec << " at " << current_time << "ms)" << std::endl;
        // 実際にはBADとして扱うか、無視する
    }
}

// --------------------------------------------------------
// 判定処理 (キーアップ時 - LN終点用)
// --------------------------------------------------------
void BMSPlayer::JudgeKeyRelease(int lane_channel)
{
    if (is_auto_play) return;
    
    // キーダウンと同様に、未処理のLN終点ノーツを検索し、判定する
    double current_time = game_time_ms + judge_offset_ms;
    
    // LN終点ノーツは channel + 1 のチャンネルに格納されている場合があるが、
    // ここでは簡略化のため、通常のノーツリスト内で is_long_note_end が true のものを探す。

    int best_ln_end_index = -1;
    double min_time_diff = JUDGE_RANGE_GOOD;

    for (size_t i = 0; i < notes.size(); ++i) {
        if (!notes[i].is_judged && notes[i].is_long_note_end && notes[i].channel == lane_channel) {
            double time_diff = std::abs(notes[i].time_ms - current_time);

            if (time_diff <= min_time_diff) {
                min_time_diff = time_diff;
                best_ln_end_index = i;
            } else if (notes[i].time_ms > current_time + min_time_diff) {
                break;
            }
        }
    }

    if (best_ln_end_index != -1) {
        Note& judged_note = notes[best_ln_end_index];
        PerformJudge(judged_note, current_time);
    }
}

// --------------------------------------------------------
// 内部判定ロジック
// --------------------------------------------------------
void BMSPlayer::PerformJudge(Note& note, double current_time)
{
    double diff = std::abs(note.time_ms - current_time);
    std::string judgment;

    if (diff <= JUDGE_RANGE_WONDERFUL) {
        judgment = "WONDERFUL";
        score += 1000;
        combo++;
    } else if (diff <= JUDGE_RANGE_GREAT) {
        judgment = "GREAT";
        score += 800;
        combo++;
    } else if (diff <= JUDGE_RANGE_GOOD) {
        judgment = "GOOD";
        score += 500;
        combo++;
    } else {
        // 判定範囲外だが、最も近いノーツとして処理された
        judgment = "BAD/POOR (Too Far)";
        combo = 0;
        score -= 200; // ペナルティ
    }

    // 判定済みとしてマーク
    note.is_judged = true;
    max_combo = std::max(max_combo, combo);
    
    std::cout << "Judge: " << judgment << " (Diff: " << std::fixed << std::setprecision(2) << diff << "ms) - Combo: " << combo << std::endl;
}

// --------------------------------------------------------
// ミス処理
// --------------------------------------------------------
void BMSPlayer::ProcessMissedNotes()
{
    // game_time_ms は BMSGameApp::SetCurrentTime で設定されている
    double current_time = game_time_ms + judge_offset_ms; 

    // 判定許容範囲を過ぎたノーツを POOR/MISS として処理
    for (Note& note : notes) {
        if (!note.is_judged) {
            // ノーツの時間が現在の時間より十分に過去（GOOD範囲+α）ならミス
            if (note.time_ms < current_time - JUDGE_RANGE_GOOD) {
                // POOR/MISS 判定
                note.is_judged = true;
                combo = 0;
                score -= 500; // 大きなペナルティ
                
                std::cout << "Judge: POOR/MISS (Time out at " << note.time_ms << "ms)" << std::endl;
                
                // WAV/BGMノーツの場合は、ここで音を鳴らさないようにする
                // (WAVは本来、イベントで発火するため、タイムアウトで鳴らす必要はない)
            } else {
                // ノーツが時間的にまだ未来にあるか、判定範囲内にある
                // notesがソートされているため、これ以降のノーツはチェック不要
                break;
            }
        }
    }
}

// --------------------------------------------------------
// イベント処理 (BGA, BPM, WAV)
// --------------------------------------------------------

void BMSPlayer::ProcessEvents()
{
    // game_time_ms は BMSGameApp::SetCurrentTime で設定されている
    double current_time = game_time_ms;

    // 未処理のイベントをチェック
    for (Note& note : notes) {
        if (!note.is_processed) {
            // イベント処理のタイミングは、判定とは異なり、ノーツの**絶対時間**を基準とする
            if (note.time_ms <= current_time) {
                note.is_processed = true;
                
                // WAVノーツ
                if (note.channel >= 0x01 && note.channel <= 0x07) {
                    ProcessWAVEvent(note.channel, note.value);
                } 
                // BGMノーツ (BGMは WAV 0x01として処理することが多いが、BMSの仕様次第)
                else if (note.channel == 0x01) {
                    ProcessWAVEvent(note.channel, note.value);
                }
                // BPM/STOP イベント
                else if (note.channel == 0x03) {
                    ProcessBPMEvent(note.time_ms, hex_to_int(note.value));
                }
                // BGA イベント
                else if (note.channel >= 0x04 && note.channel <= 0x06) {
                    ProcessBGAEvent(note.channel, hex_to_int(note.value));
                }
                // レイヤーイベント (LN始点など)
                else if (note.channel == 0x1A || note.channel == 0x2A) {
                    // LN始点/終点はノーツ判定で処理されるため、ここでは主にWAVイベントとして処理
                }
            } else {
                // ノーツは時間順にソートされているため、これ以降は処理不要
                break;
            }
        }
    }
}

// BGA/Layer 処理
void BMSPlayer::ProcessBGAEvent(int channel, int value_id)
{
    if (channel == 0x04) {
        // BGA (メインアニメーション)
        current_bga_id = value_id;
        std::cout << "BGA Change: ID " << value_id << std::endl;
    } else if (channel == 0x06) {
        // LAYER (レイヤーアニメーション)
        // レイヤーは重ねて表示されるため、マップに保存する
        // ID 0 の場合はレイヤーをクリアする
        if (value_id == 0) {
            current_layer_ids.clear();
            std::cout << "Layer Clear" << std::endl;
        } else {
            // 簡略化のため、キー値は常に 1 とする (実際には複数のレイヤーを持つ)
            current_layer_ids[1] = value_id;
            std::cout << "Layer Set: ID " << value_id << std::endl;
        }
    }
}

// BPM 処理
void BMSPlayer::ProcessBPMEvent(double time_ms, double new_bpm)
{
    // 厳密なBMSエンジンでは、ここでBPM変更による以降のノーツ時間補正を行うが、
    // 簡易実装のため、ここではBPM値を保存するに留める。
    // (BMSParserが事前に絶対時間に変換していることを前提とする)
    bpm = new_bpm;
    std::cout << "BPM Change: " << new_bpm << " at " << time_ms << "ms" << std::endl;
}

// WAV 処理 (ネイティブ環境では実際のオーディオ再生に置き換える必要があります)
void BMSPlayer::ProcessWAVEvent(int channel, const std::string& value_id)
{
    // TODO: ネイティブ化では、ここでオーディオライブラリ（例: OpenAL, SDL_mixer）を使って
    // 該当するWAVファイル（value_idに対応）を再生する処理を実装する
    std::cout << "WAV Play: Channel " << std::hex << channel << " (Value: " << value_id << std::dec << ")" << std::endl;
}


// --------------------------------------------------------
// オートプレイ
// --------------------------------------------------------
void BMSPlayer::AutoPlayJudge()
{
    // game_time_ms は BMSGameApp::SetCurrentTime で設定されている
    double current_time = game_time_ms;

    for (Note& note : notes) {
        // 未処理のノーツであり、プレイチャンネルのノーツ（BGM/BGA/BPM以外）
        if (!note.is_judged && note.channel >= 0x11 && note.channel <= 0x19) {
            
            // 判定時間に近いノーツを探す（±5ms などの許容範囲）
            if (std::abs(note.time_ms - current_time) < 5.0) {
                // 自動判定実行
                PerformJudge(note, current_time);
                
                // WAVイベントの処理 (実際のオーディオ再生をシミュレート)
                ProcessWAVEvent(note.channel, note.value);

            } else if (note.time_ms > current_time + 5.0) {
                // ノーツが時間的にまだ未来にあるため、これ以上探す必要はない
                break;
            }
        }
    }
}


// --------------------------------------------------------
// 設定とゲッター
// --------------------------------------------------------

void BMSPlayer::SetJudgeOffset(double offset_ms)
{
    judge_offset_ms = offset_ms;
    std::cout << "Judge Offset set to: " << offset_ms << "ms" << std::endl;
}

void BMSPlayer::SetAutoPlayMode(bool is_auto)
{
    is_auto_play = is_auto;
    std::cout << "Auto Play Mode: " << (is_auto ? "ON" : "OFF") << std::endl;
}

int BMSPlayer::GetScore() const {
    return score;
}

int BMSPlayer::GetCombo() const {
    return combo;
}

double BMSPlayer::GetCurrentBPM() const {
    return bpm;
}

int BMSPlayer::GetCurrentBgaId() const {
    return current_bga_id;
}

const std::map<int, int>& BMSPlayer::GetCurrentLayerIds() const {
    return current_layer_ids;
}
