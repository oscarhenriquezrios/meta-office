#include "meta_office.hpp"
#include <cstdio>

// Variables globales de tamaño (se actualizan al redimensionar)
int screenW = SCREEN_W_DEF;
int screenH = SCREEN_H_DEF;

int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_W_DEF, SCREEN_H_DEF, "Meta-Office 2D | Oficina Virtual C++");
    SetWindowMinSize(960, 540);
    SetTargetFPS(60);

    // Inicializar audio
    InitAudioDevice();
    if (!IsAudioDeviceReady()) {
        TraceLog(LOG_WARNING, "Audio device not available - no sound will play");
    }

    MaximizeWindow(); // Arranca maximizada para verse mejor

    std::vector<Entity> entities;
    std::vector<LogEntry> logs;
    Vector2 origin = {(float)SCREEN_W_DEF / 2 - 150, (float)SCREEN_H_DEF / 5};
    bool paused = false;
    int selectedId = -1;
    float panY = 0;

    initSimulation(entities, logs);

    while (!WindowShouldClose()) {
        double time = GetTime();
        float dt = GetFrameTime();

        // Detectar redimension
        if (IsWindowResized()) {
            screenW = GetScreenWidth();
            screenH = GetScreenHeight();
            // Recalcular origen para que el mapa se centre
            origin.x = (float)screenW / 2 - 150;
            origin.y = (float)screenH / 5 + panY;
        }

        // Alternar pantalla completa con F11
        if (IsKeyPressed(KEY_F11)) {
            ToggleFullscreen();
            screenW = GetScreenWidth();
            screenH = GetScreenHeight();
            origin.x = (float)screenW / 2 - 150;
            origin.y = (float)screenH / 5 + panY;
        }

        handleInput(entities, origin, selectedId, paused, panY);
        updateSimulation(entities, logs, paused, dt, time);

        BeginDrawing();
        ClearBackground({9, 13, 22, 255});

        drawScene(entities, origin, time);
        drawUI(entities, logs, selectedId, paused);

        DrawFPS(10, screenH - 65);

        // Indicador de pantalla completa
        DrawText(IsWindowFullscreen() ? "F11: ventana" : "F11: pantalla completa",
                 screenW - 200, screenH - 20, 10, alpha(WHITE, 60));

        EndDrawing();
    }

    CloseAudioDevice();
    stopLlmThread();
    unloadTextures();
    CloseWindow();
    return 0;
}
