#include <SDL2/SDL.h>
#include <iostream>
#include "BMSParser.h"
#include "BMSData.h"
#include "Judge.h"
#include "Renderer.h"

bool IsKeyCurrentlyPressed(int lane_channel)
{
    // 仮実装（本来は SDL_GetKeyboardState 等で実装）
    return false;
}

int main(int argc, char** argv)
{
    // ----------------------------
    // SDL 初期化
    // ----------------------------
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        std::cout << "SDL init failed\n";
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "BMS Player",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1280, 960,
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        std::cout << "window error\n";
        return 1;
    }

    SDL_Renderer* rendererSDL = SDL_CreateRenderer(window, -1, 0);

    // ----------------------------
    // BMS 読み込み
    // ----------------------------
    BMSData bms;
    BMSParser parser;

    if (!parser.Parse("test.bms", bms)) {
        std::cout << "Parse failed\n";
        return 1;
    }

    // ----------------------------
    // メインループ
    // ----------------------------
    double current_time = 0.0;
    Uint64 last = SDL_GetPerformanceCounter();
    bool running = true;

    while (running)
    {
        // ---- delta time ----
        Uint64 now = SDL_GetPerformanceCounter();
        double delta = (double)(now - last) / SDL_GetPerformanceFrequency() * 1000.0;
        last = now;
        current_time += delta;

        // ---- イベント処理 ----
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN)
            {
                int lane = 11; // 仮
                JudgeKeyHit(bms, lane, current_time);
            }
            if (ev.type == SDL_KEYUP)
            {
                int lane = 11;
                ProcessLNKeyRelease(lane, current_time);
            }
        }

        // ---- 自動 MISS ----
        ProcessScrollOutMisses(bms, current_time);
        ProcessLNEnds(current_time);

        // ---- 描画準備 ----
        auto draw_notes = GetNotesForRendering(bms.notes, current_time);

        // ---- 画面クリア ----
        SDL_SetRenderDrawColor(rendererSDL, 0, 0, 0, 255);
        SDL_RenderClear(rendererSDL);

        // ---- NOTE 描画 ----
        SDL_SetRenderDrawColor(rendererSDL, 255, 255, 255, 255);
        for (auto& dn : draw_notes)
        {
            SDL_Rect r;
            r.x = 600;
            r.y = (int)dn.y_position;
            r.w = 20;
            r.h = (int)dn.length + 20; // LN or TAP

            SDL_RenderFillRect(rendererSDL, &r);
        }

        SDL_RenderPresent(rendererSDL);
    }

    SDL_DestroyRenderer(rendererSDL);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
