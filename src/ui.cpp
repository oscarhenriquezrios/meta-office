#include "meta_office.hpp"

void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused) {
    // ===== TOP BAR =====
    DrawRectangle(0, 0, SCREEN_W, 50, {15,23,42,220});
    DrawLine(0, 50, SCREEN_W, 50, {255,255,255,20});

    DrawText("META-OFFICE 2D", 20, 14, 20, WHITE);
    DrawText("SIMULACION META-OPERATIVA C++", 180, 18, 12, {148,163,184,255});

    if (!entities.empty()) {
        auto& h = entities[0];
        DrawText(TextFormat("Posicion: (%.0f, %.0f)", h.x, h.y), 500, 18, 12, {148,163,184,255});
    }

    DrawCircle(SCREEN_W - 180, 16, 5, paused ? ORANGE : GREEN);
    DrawText(paused ? "PAUSADO" : "EN TIEMPO REAL", SCREEN_W - 170, 12, 12,
             paused ? ORANGE : GREEN);

    DrawRectangle(SCREEN_W - 120, 8, 110, 34, paused ? (Color){245,158,11,60} : (Color){255,255,255,20});
    DrawRectangleLines(SCREEN_W - 120, 8, 110, 34, {255,255,255,40});
    DrawText(paused ? "REANUDAR" : "PAUSAR", SCREEN_W - 90, 16, 14, WHITE);

    // ===== SIDEBAR =====
    int sx = SCREEN_W - 300;
    DrawRectangle(sx, 50, 300, SCREEN_H - 50, {10,14,23,200});
    DrawLine(sx, 50, sx, SCREEN_H, {255,255,255,15});

    DrawText("MIEMBROS", sx + 12, 58, 14, {148,163,184,255});
    DrawText(TextFormat("(%zu)", entities.size()), sx + 100, 58, 14, {100,116,139,255});

    int yy = 80;
    for (auto& e : entities) {
        bool sel = (e.id == selectedId);
        Rectangle card = {(float)sx + 10, (float)yy, 280, 58};
        DrawRectangleRounded(card, 0.1f, 4, sel ? alpha(e.color, 30) : alpha(WHITE, 5));
        DrawRectangleRoundedLines(card, 0.1f, 4, sel ? e.color : alpha(WHITE, 15));

        const char* icon = "?";
        if (e.type == EntityType::Human) icon = "H";
        else if (e.type == EntityType::CodeBot) icon = "C";
        else if (e.type == EntityType::DataBot) icon = "D";
        else if (e.type == EntityType::Orchestrator) icon = "O";

        DrawRectangle(sx + 18, yy + 10, 32, 32, e.color);
        DrawText(icon, sx + 28, yy + 14, 20, WHITE);
        DrawText(e.name.c_str(), sx + 58, yy + 12, 12, WHITE);
        DrawText(e.role.c_str(), sx + 58, yy + 28, 9, {148,163,184,255});

        const char* st = "IDLE";
        Color sc = {148,163,184,255};
        if (e.status == Status::Walking) { st = "CAMINANDO"; sc = {56,189,248,255}; }
        else if (e.status == Status::Error) { st = "ERROR"; sc = RED; }
        else if (e.status == Status::Busy) { st = "OCUPADO"; sc = ORANGE; }
        DrawText(st, sx + 210, yy + 14, 10, sc);

        yy += 64;
    }

    yy += 10;
    DrawText("ACTIVIDAD", sx + 12, yy, 14, {148,163,184,255});
    yy += 22;

    int start = std::max(0, (int)logs.size() - 8);
    for (int i = start; i < (int)logs.size(); i++) {
        DrawText(logs[i].text.c_str(), sx + 12, yy, 9, logs[i].color);
        yy += 14;
    }

    DrawRectangle(10, SCREEN_H - 40, 400, 30, alpha(BLACK, 150));
    DrawText("WASD: mover | Clic en agente: inspeccionar | P: pausar", 18, SCREEN_H - 33, 12, {148,163,184,255});
}

void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY) {
    if (IsKeyPressed(KEY_P)) paused = !paused;

    auto* human = entities.empty() ? nullptr : &entities[0];
    if (human) {
        float nx = human->targetX, ny = human->targetY;
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) ny = std::max(0.0f, ny - 1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) ny = std::min(15.0f, ny + 1);
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) nx = std::max(0.0f, nx - 1);
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) nx = std::min(15.0f, nx + 1);
        if (nx != human->targetX || ny != human->targetY) {
            human->targetX = nx;
            human->targetY = ny;
        }
    }

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();

        if (mp.x < SCREEN_W - 300) {
            if (mp.x > SCREEN_W - 120 && mp.x < SCREEN_W - 10 && mp.y > 8 && mp.y < 42) {
                paused = !paused;
                return;
            }

            Vector2 grid = isoToGrid(mp.x, mp.y, origin);
            int gx = roundf(grid.x), gy = roundf(grid.y);
            if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                bool clickedAgent = false;
                for (auto& e : entities) {
                    if (e.type == EntityType::Human) continue;
                    float d = sqrtf(powf(e.renderX - gx, 2) + powf(e.renderY - gy, 2));
                    if (d < 1.5f) { selectedId = e.id; clickedAgent = true; break; }
                }
                if (!clickedAgent && human) {
                    human->targetX = gx; human->targetY = gy;
                    selectedId = -1;
                }
            }
        }

        int sx = SCREEN_W - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sx + 10, (float)(80 + i * 64), 280, 58};
            if (CheckCollisionPointRec(mp, card)) { selectedId = entities[i].id; break; }
        }
    }

    float wheel = GetMouseWheelMove();
    panY += wheel * 5;
    origin.y += wheel * 5;
}
