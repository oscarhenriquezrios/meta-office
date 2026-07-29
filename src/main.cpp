#include "meta_office.hpp"
#include <cstdio>

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "Meta-Office 2D | Oficina Virtual C++");
    SetTargetFPS(60);

    std::vector<Entity> entities;
    std::vector<LogEntry> logs;
    Vector2 origin = {(float)SCREEN_W / 2 - 150, (float)SCREEN_H / 5};
    bool paused = false;
    int selectedId = -1;
    float panY = 0;

    initSimulation(entities, logs);

    while (!WindowShouldClose()) {
        double time = GetTime();
        float dt = GetFrameTime();

        handleInput(entities, origin, selectedId, paused, panY);
        updateSimulation(entities, logs, paused, dt, time);

        BeginDrawing();
        ClearBackground({9, 13, 22, 255});

        drawScene(entities, origin, time);
        drawUI(entities, logs, selectedId, paused);

        DrawFPS(10, SCREEN_H - 65);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
