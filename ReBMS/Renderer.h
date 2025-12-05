#pragma once
#include <vector>
#include "Note.h"

// ------------------------------
// 描画用ノーツ構造体
// ------------------------------
struct DrawNote {
    const Note* source;
    double y_position;
    double length;
};

// 描画用リスト生成
std::vector<DrawNote> GetNotesForRendering(
    const std::vector<Note>& notes,
    double current_time);

// 必要な描画設定
extern const double JUDGELINE_Y;
extern const double SCROLL_SPEED;
extern const double VISIBLE_DURATION_MS;
