#include "Renderer.h"

// ------------------------------
// 描画設定（お好みで Config.cpp に逃がしても良い）
// ------------------------------
const double JUDGELINE_Y          = 800.0;
const double SCROLL_SPEED         = 0.5;
const double VISIBLE_DURATION_MS  = 3000.0;

// ------------------------------
// 描画リスト生成
// ------------------------------
std::vector<DrawNote> GetNotesForRendering(
    const std::vector<Note>& notes,
    double current_time)
{
    std::vector<DrawNote> draw_notes;
    draw_notes.reserve(notes.size());

    const double start_time = current_time;
    const double end_time   = current_time + VISIBLE_DURATION_MS;

    for (const Note& n : notes)
    {
        if (n.time_ms < start_time || n.time_ms > end_time)
            continue;

        DrawNote dn;
        dn.source = &n;

        double dt = n.time_ms - current_time;
        dn.y_position = JUDGELINE_Y - (dt * SCROLL_SPEED);

        if (n.end_time_ms > n.time_ms)
        {
            double ln_dur = n.end_time_ms - n.time_ms;
            dn.length = ln_dur * SCROLL_SPEED;
        }
        else
        {
            dn.length = 0.0;
        }

        draw_notes.push_back(dn);
    }

    return draw_notes;
}
