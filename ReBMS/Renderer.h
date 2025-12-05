#pragma once
#include <vector>
#include "Note.h"

// ---- 描画用構造体 ----
struct DrawNote {
    const Note* source;
    double y_position;
    double length;
};

std::vector<DrawNote> GetNotesForRendering(
    const std::vector<Note>& notes,
    double current_time);
