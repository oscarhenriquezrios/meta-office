#include "meta_office.hpp"

// ============================================================
// ISO coords
// ============================================================
Vector2 gridToIso(float gx, float gy, Vector2 origin) {
    return {
        origin.x + (gx - gy) * (TILE_W / 2.0f),
        origin.y + (gx + gy) * (TILE_H / 2.0f)
    };
}

Vector2 isoToGrid(float sx, float sy, Vector2 origin) {
    float rx = sx - origin.x, ry = sy - origin.y;
    return {
        (ry / (TILE_H / 2.0f) + rx / (TILE_W / 2.0f)) / 2.0f,
        (ry / (TILE_H / 2.0f) - rx / (TILE_W / 2.0f)) / 2.0f
    };
}

// ============================================================
// Texturas procedurales
// ============================================================
static Texture2D texFloor[4][2];
static Texture2D texDesk, texServer, texMeeting, texLounge, texWhiteboard, texClock, texPlant, texFrame;
static bool texturesLoaded = false;

// Altura de pared (pseudo-3D)
static constexpr float WALL_H = 28.0f;

static void initTextures() {
    if (texturesLoaded) return;

    Color zoneColors[4][2] = {
        {{28, 42, 74, 255}, {20, 32, 60, 255}},
        {{22, 52, 38, 255}, {16, 42, 28, 255}},
        {{48, 28, 65, 255}, {38, 20, 52, 255}},
        {{58, 40, 20, 255}, {45, 30, 14, 255}}
    };

    for (int z = 0; z < 4; z++) {
        for (int p = 0; p < 2; p++) {
            Image img = GenImageColor(TILE_W, TILE_H, zoneColors[z][p]);
            Color edge = ColorBrightness(zoneColors[z][p], 0.18f);
            ImageDrawRectangle(&img, 0, 0, TILE_W, 1, edge);
            ImageDrawRectangle(&img, 0, TILE_H-1, TILE_W, 1, edge);
            ImageDrawRectangle(&img, 0, 0, 1, TILE_H, edge);
            ImageDrawRectangle(&img, TILE_W-1, 0, 1, TILE_H, edge);
            for (int i = 0; i < 8; i++) {
                int dx = 6 + (i * 11) % (TILE_W - 12);
                int dy = 6 + (i * 7) % (TILE_H - 12);
                ImageDrawPixel(&img, dx, dy, ColorBrightness(zoneColors[z][p], 0.30f));
            }
            texFloor[z][p] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }

    // --- Escritorio ---
    {
        Image img = GenImageColor(48, 36, {0,0,0,0});
        ImageDrawRectangle(&img, 3, 24, 42, 10, (Color){0,0,0,90});
        ImageDrawRectangle(&img, 0, 0, 48, 20, (Color){38, 52, 72, 255});
        ImageDrawRectangle(&img, 0, 0, 48, 2, (Color){62, 80, 105, 255});
        ImageDrawRectangle(&img, 4, 5, 14, 11, (Color){12, 18, 32, 255});
        ImageDrawRectangle(&img, 6, 7, 10, 7, (Color){56, 189, 248, 220});
        ImageDrawPixel(&img, 7, 8, (Color){200,240,255,255});
        ImageDrawRectangle(&img, 28, 5, 14, 11, (Color){12, 18, 32, 255});
        ImageDrawRectangle(&img, 30, 7, 10, 7, (Color){16, 185, 129, 220});
        ImageDrawPixel(&img, 31, 8, (Color){100,255,200,255});
        ImageDrawRectangle(&img, 12, 16, 24, 3, (Color){65, 75, 95, 220});
        ImageDrawRectangle(&img, 2, 18, 44, 2, (Color){25, 35, 50, 255});
        texDesk = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Rack servidor ---
    {
        Image img = GenImageColor(34, 48, {0,0,0,0});
        ImageDrawRectangle(&img, 3, 42, 28, 6, (Color){0,0,0,80});
        ImageDrawRectangle(&img, 0, 0, 34, 48, (Color){18, 26, 45, 255});
        ImageDrawRectangleLines(&img, (Rectangle){0, 0, 34, 48}, 1, (Color){55, 70, 95, 255});
        ImageDrawRectangle(&img, 5, 2, 24, 4, (Color){10, 15, 25, 255});
        for (int i = 0; i < 5; i++) {
            int y = 10 + i * 7;
            ImageDrawRectangle(&img, 4, y, 26, 5, (Color){28, 38, 55, 255});
            ImageDrawCircle(&img, 10, y+2, 1.5f, (Color){16, 185, 129, 255});
            ImageDrawCircle(&img, 10, y+2, 0.5f, (Color){120, 255, 200, 255});
            ImageDrawCircle(&img, 24, y+2, 1.5f, (Color){56, 189, 248, 255});
        }
        texServer = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Mesa reuniones ---
    {
        Image img = GenImageColor(64, 34, {0,0,0,0});
        ImageDrawRectangle(&img, 4, 26, 56, 8, (Color){0,0,0,70});
        ImageDrawRectangle(&img, 4, 10, 56, 13, (Color){48, 58, 80, 255});
        ImageDrawRectangleLines(&img, (Rectangle){4, 10, 56, 13}, 1, (Color){85, 100, 120, 255});
        ImageDrawCircle(&img, 32, 16, 7, (Color){168, 85, 247, 60});
        ImageDrawCircle(&img, 32, 16, 4, (Color){200, 120, 255, 100});
        ImageDrawCircle(&img, 32, 16, 2, (Color){230, 170, 255, 200});
        texMeeting = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Lounge ---
    {
        Image img = GenImageColor(64, 34, {0,0,0,0});
        ImageDrawRectangle(&img, 2, 24, 58, 8, (Color){0,0,0,50});
        ImageDrawRectangle(&img, 20, 12, 24, 8, (Color){85, 58, 42, 255});
        ImageDrawRectangle(&img, 22, 14, 20, 4, (Color){130, 85, 55, 255});
        ImageDrawCircle(&img, 32, 8, 4, (Color){254, 243, 199, 255});
        ImageDrawCircle(&img, 32, 8, 2, (Color){160, 90, 20, 255});
        ImageDrawRectangle(&img, 48, 12, 8, 10, (Color){125, 82, 52, 255});
        ImageDrawRectangle(&img, 48, 22, 8, 2, (Color){100, 65, 40, 255});
        ImageDrawCircle(&img, 52, 6, 6, (Color){34, 197, 94, 255});
        ImageDrawCircle(&img, 48, 8, 4, (Color){22, 163, 74, 220});
        ImageDrawCircle(&img, 56, 7, 4, (Color){50, 210, 110, 220});
        ImageDrawRectangle(&img, 2, 16, 14, 10, (Color){70, 50, 40, 255});
        ImageDrawRectangle(&img, 2, 14, 14, 4, (Color){90, 65, 50, 255});
        texLounge = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Pizarra ---
    {
        Image img = GenImageColor(36, 22, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 36, 22, (Color){48, 60, 82, 255});
        ImageDrawRectangle(&img, 2, 2, 32, 18, (Color){225, 230, 240, 255});
        ImageDrawRectangle(&img, 4, 4, 8, 6, (Color){251, 191, 36, 255});
        ImageDrawRectangle(&img, 16, 6, 8, 6, (Color){244, 114, 182, 255});
        ImageDrawRectangle(&img, 22, 11, 8, 5, (Color){56, 189, 248, 255});
        ImageDrawLine(&img, 12, 7, 16, 9, (Color){60,60,60,255});
        ImageDrawLine(&img, 24, 8, 22, 12, (Color){60,60,60,255});
        texWhiteboard = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Reloj ---
    {
        Image img = GenImageColor(18, 18, {0,0,0,0});
        ImageDrawCircle(&img, 9, 9, 8, (Color){28, 38, 55, 255});
        ImageDrawCircleLines(&img, 9, 9, 8, (Color){65, 80, 100, 255});
        ImageDrawCircle(&img, 9, 9, 5, (Color){200, 205, 218, 255});
        ImageDrawCircle(&img, 9, 9, 1, (Color){30, 35, 50, 255});
        ImageDrawLine(&img, 9, 9, 9, 5, (Color){30,30,30,255});
        ImageDrawLine(&img, 9, 9, 13, 9, (Color){30,30,30,255});
        texClock = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Planta ---
    {
        Image img = GenImageColor(24, 22, {0,0,0,0});
        ImageDrawRectangle(&img, 6, 12, 12, 10, (Color){130, 85, 55, 255});
        ImageDrawRectangle(&img, 5, 10, 14, 3, (Color){155, 100, 65, 255});
        ImageDrawCircle(&img, 12, 8, 8, (Color){34, 197, 94, 255});
        ImageDrawCircle(&img, 7, 7, 5, (Color){24, 163, 74, 240});
        ImageDrawCircle(&img, 17, 6, 5, (Color){50, 210, 110, 240});
        ImageDrawCircle(&img, 12, 4, 4, (Color){60, 220, 120, 220});
        ImageDrawLine(&img, 12, 12, 10, 6, (Color){20,120,50,200});
        ImageDrawLine(&img, 12, 12, 15, 7, (Color){20,120,50,200});
        texPlant = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Cuadro ---
    {
        Image img = GenImageColor(22, 18, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 22, 18, (Color){65, 45, 75, 255});
        ImageDrawRectangle(&img, 2, 2, 18, 14, (Color){105, 75, 125, 255});
        ImageDrawCircle(&img, 11, 9, 4, (Color){85, 55, 105, 255});
        ImageDrawCircle(&img, 9, 7, 2, (Color){125, 95, 145, 255});
        texFrame = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    texturesLoaded = true;
}

void unloadTextures() {
    if (!texturesLoaded) return;
    for (int z = 0; z < 4; z++)
        for (int p = 0; p < 2; p++)
            UnloadTexture(texFloor[z][p]);
    UnloadTexture(texDesk);
    UnloadTexture(texServer);
    UnloadTexture(texMeeting);
    UnloadTexture(texLounge);
    UnloadTexture(texWhiteboard);
    UnloadTexture(texClock);
    UnloadTexture(texPlant);
    UnloadTexture(texFrame);
}

// ============================================================
// Zonas
// ============================================================
struct ZoneDef {
    int xMin, xMax, yMin, yMax;
    const char* name;
    Color stroke;
    int texIdx;
};

static const ZoneDef zones[] = {
    {0,7,0,7, "AREA DE DESARROLLO",   {56, 189, 248, 200}, 0},
    {8,15,0,7, "SALA DE SERVIDORES",  {16, 185, 129, 200}, 1},
    {0,7,8,15, "SALA DE CONFERENCIAS",{168, 85, 247, 200}, 2},
    {8,15,8,15, "LOUNGE & CAFETERIA", {245, 158, 11, 200}, 3},
};

static int getZone(int gx, int gy) {
    if (gx <= 7 && gy <= 7) return 0;
    if (gx >= 8 && gy <= 7) return 1;
    if (gx <= 7 && gy >= 8) return 2;
    return 3;
}

static void drawTile(Vector2 p, int gx, int gy) {
    int z = getZone(gx, gy);
    int parity = (gx + gy) % 2;
    Texture2D tex = texFloor[z][parity];
    DrawTexturePro(tex,
        {0, 0, (float)tex.width, (float)tex.height},
        {p.x, p.y + TILE_H/2.0f, (float)TILE_W, (float)TILE_H},
        {(float)TILE_W/2.0f, (float)TILE_H/2.0f},
        0, WHITE);
}

// ============================================================
// Overlay de zona
// ============================================================
static void drawZoneOverlay(const ZoneDef& z, Vector2 origin) {
    auto p1 = gridToIso(z.xMin, z.yMin, origin);
    auto p2 = gridToIso(z.xMax+1, z.yMin, origin);
    auto p3 = gridToIso(z.xMax+1, z.yMax+1, origin);
    auto p4 = gridToIso(z.xMin, z.yMax+1, origin);
    DrawLineEx(p1, p2, 1, z.stroke);
    DrawLineEx(p2, p3, 1, z.stroke);
    DrawLineEx(p3, p4, 1, z.stroke);
    DrawLineEx(p4, p1, 1, z.stroke);
    auto lbl = gridToIso(z.xMin+0.5f, z.yMin+0.8f, origin);
    DrawText(z.name, lbl.x - MeasureText(z.name, 10)/2, lbl.y - 6, 10, z.stroke);
}

// ============================================================
// Mobiliario
// ============================================================
static void drawFurnitureTex(Vector2 pos, Texture2D tex, Vector2 offset, Color tint) {
    DrawTexturePro(tex,
        {0, 0, (float)tex.width, (float)tex.height},
        {pos.x + offset.x, pos.y + offset.y, (float)tex.width, (float)tex.height},
        {0, 0}, 0, tint);
}

// ============================================================
// Paredes pseudo-3D
// ============================================================
static void drawWalls(Vector2 origin) {
    // Pared izquierda (x=0)
    for (int i = 0; i < GRID_H; i++) {
        Vector2 bottomA = gridToIso(0, i, origin);
        Vector2 bottomB = gridToIso(0, i + 1, origin);
        Vector2 topA = {bottomA.x, bottomA.y - WALL_H};
        Vector2 topB = {bottomB.x, bottomB.y - WALL_H};

        Color wallBase = {30, 40, 60, 200};
        Color wallTop = {50, 65, 90, 220};

        DrawTriangle(bottomA, bottomB, topB, wallBase);
        DrawTriangle(bottomA, topB, topA, wallBase);
        DrawLineEx(topA, topB, 1, wallTop);
    }

    // Pared superior (y=0)
    for (int i = 0; i < GRID_W; i++) {
        Vector2 bottomA = gridToIso(i, 0, origin);
        Vector2 bottomB = gridToIso(i + 1, 0, origin);
        Vector2 topA = {bottomA.x, bottomA.y - WALL_H};
        Vector2 topB = {bottomB.x, bottomB.y - WALL_H};

        Color wallBase = {25, 35, 55, 180};

        DrawTriangle(bottomA, bottomB, topB, wallBase);
        DrawTriangle(bottomA, topB, topA, wallBase);
        DrawLineEx(topA, topB, 1, {50, 65, 90, 200});
    }
}

// ============================================================
// Fondo degradado + estrellas
// ============================================================
static void drawBackground(double time) {
    for (int y = 0; y < screenH; y++) {
        float t = (float)y / screenH;
        Color c = {
            (unsigned char)(9 + t * 4),
            (unsigned char)(13 + t * 6),
            (unsigned char)(22 + t * 10),
            255
        };
        DrawLine(0, y, screenW, y, c);
    }
    for (int i = 0; i < 40; i++) {
        int sx = (i * 137) % screenW;
        int sy = (i * 73) % (screenH / 2);
        float twinkle = sinf(time * 0.5f + i) * 0.5f + 0.5f;
        int starAlpha = (int)(50 + twinkle * 60);
        DrawPixel(sx, sy, alpha(WHITE, starAlpha));
    }
    int horizonY = screenH / 5;
    for (int y = horizonY - 3; y < horizonY + 3; y++) {
        int glowAlpha = 30 - abs(y - horizonY) * 10;
        if (glowAlpha > 0) DrawLine(0, y, screenW, y, alpha({56, 189, 248, 255}, glowAlpha));
    }
}

// ============================================================
// Entity drawing (avatares)
// ============================================================
static void drawEntity(const Entity& e, Vector2 pos) {
    float centerY = pos.y + TILE_H/2.0f;
    float bob = (e.status == Status::Walking) ? sinf(GetTime()*15)*3 : sinf(GetTime()*3)*1.5f;
    float avY = centerY - 16 + bob;

    DrawEllipse(pos.x, centerY+10, 16, 8, alpha(BLACK, 70));

    if (e.hasCriticalError) {
        float pulse = sinf(GetTime() * 6) * 0.3f + 0.7f;
        DrawCircleLines(pos.x, avY, 16, alpha(RED, (int)(120 * pulse)));
        DrawCircleLines(pos.x, avY, 19, alpha(RED, (int)(60 * pulse)));
    }

    switch (e.type) {
        case EntityType::Human: {
            DrawCircle(pos.x, avY+4, 11, {217, 119, 6, 255});
            DrawCircle(pos.x, avY+4, 11, alpha({245, 158, 11, 255}, 30));
            DrawCircle(pos.x, avY-8, 9, {245, 158, 11, 255});
            DrawCircleLines(pos.x, avY-8, 9, WHITE);
            DrawCircle(pos.x-3, avY-9, 1.5f, WHITE);
            DrawCircle(pos.x+3, avY-9, 1.5f, WHITE);
            DrawLine(pos.x-2, avY-5, pos.x+2, avY-5, alpha(BLACK, 150));
            DrawCircle(pos.x-11, avY+2, 4, alpha({200, 100, 10, 255}, 240));
            DrawCircle(pos.x+11, avY+2, 4, alpha({200, 100, 10, 255}, 240));
            DrawEllipse(pos.x, centerY+10, 14, 6, alpha({245,158,11,255}, 40));
            break;
        }
        case EntityType::CodeBot:
        case EntityType::DataBot: {
            Color c = e.color;
            DrawEllipse(pos.x, centerY+10, 14, 6, alpha(c, 30));
            DrawLine(pos.x, avY-14, pos.x, avY-20, c);
            DrawCircle(pos.x, avY-21, 3, WHITE);
            DrawCircle(pos.x, avY-21, 1.5f, c);
            DrawRectangle(pos.x-11, avY-14, 22, 13, {30, 41, 59, 255});
            DrawRectangleLines(pos.x-11, avY-14, 22, 13, c);
            Color eye = e.hasCriticalError ? RED : c;
            DrawRectangle(pos.x-7, avY-10, 5, 4, eye);
            DrawCircle(pos.x-4, avY-8, 1.5f, WHITE);
            DrawRectangle(pos.x+2, avY-10, 5, 4, eye);
            DrawCircle(pos.x+4, avY-8, 1.5f, WHITE);
            DrawRectangle(pos.x-13, avY, 26, 15, {15, 23, 42, 255});
            DrawRectangleLines(pos.x-13, avY, 26, 15, c);
            DrawRectangle(pos.x-4, avY+4, 8, 3, alpha(c, 100));
            DrawLine(pos.x, avY, pos.x, avY+15, alpha(c, 60));
            break;
        }
        case EntityType::Orchestrator: {
            DrawEllipse(pos.x, avY, 20, 9, alpha({168, 85, 247, 255}, 40));
            DrawEllipseLines(pos.x, avY, 20, 9, {168, 85, 247, 120});
            DrawCircle(pos.x, avY, 11, {126, 34, 206, 255});
            DrawCircleLines(pos.x, avY, 11, {168, 85, 247, 255});
            DrawCircle(pos.x, avY, 7, alpha({200, 130, 250, 255}, 180));
            DrawCircle(pos.x, avY-7, 7, {168, 85, 247, 255});
            DrawCircleLines(pos.x, avY-7, 7, WHITE);
            DrawCircle(pos.x, avY-7, 2, WHITE);
            float rot = GetTime() * 2;
            float ringR = 14 + sinf(GetTime() * 3) * 2;
            DrawCircleLines(pos.x, avY, ringR, alpha({200, 130, 250, 255}, 80));
            DrawCircle(pos.x + cosf(rot) * 14, avY + sinf(rot) * 8, 2, {200, 130, 250, 200});
            DrawCircle(pos.x + cosf(rot + 2.1f) * 14, avY + sinf(rot + 2.1f) * 8, 2, alpha({200, 130, 250, 255}, 150));
            break;
        }
    }

    int tw = MeasureText(e.name.c_str(), 10);
    DrawRectangle(pos.x - tw/2 - 4, avY - 28, tw + 8, 14, alpha(BLACK, 100));
    DrawText(e.name.c_str(), pos.x - tw/2 + 1, avY - 26, 10, alpha(BLACK, 150));
    DrawText(e.name.c_str(), pos.x - tw/2, avY - 27, 10, WHITE);

    if (e.hasGreetedHuman && e.type != EntityType::Human) {
        float floatY = sinf(GetTime() * 3) * 2;
        DrawCircle(pos.x + 14, avY - 22 + floatY, 4, alpha(e.color, 220));
        DrawCircle(pos.x + 14, avY - 22 + floatY, 2, WHITE);
    }

    // Speech bubble
    if (!e.speech.text.empty() && GetTime() < e.speech.expiry) {
        int stw = MeasureText(e.speech.text.c_str(), 9);
        float bw = std::min(stw + 16.0f, 160.0f);
        float bx = pos.x - bw/2;
        float by = avY - 44;
        DrawRectangleRounded({bx+2, by+2, bw, 20}, 0.3f, 4, alpha(BLACK, 100));
        DrawRectangleRounded({bx, by, bw, 20}, 0.3f, 4, alpha(BLACK, 200));
        DrawRectangleRoundedLines({bx, by, bw, 20}, 0.3f, 4, e.color);
        DrawText(e.speech.text.c_str(), bx + 8, by + 5, 9, WHITE);
        DrawTriangle(
            {pos.x - 4, by + 20},
            {pos.x + 4, by + 20},
            {pos.x, by + 26},
            alpha(BLACK, 200)
        );
    }
}

// ============================================================
// Sombra proyectada de mobiliario (pseudo-3D)
// ============================================================
static void drawFurnitureShadow(Vector2 pos, float w, float h) {
    DrawEllipse(pos.x, pos.y + 4, w, h, alpha(BLACK, 50));
}

// ============================================================
// Public drawScene
// ============================================================
void drawScene(const std::vector<Entity>& entities, Vector2 origin, double time) {
    drawBackground(time);

    if (!texturesLoaded) initTextures();

    // 1. Tiles
    for (int gx = 0; gx < GRID_W; gx++)
        for (int gy = 0; gy < GRID_H; gy++)
            drawTile(gridToIso(gx, gy, origin), gx, gy);

    // 2. Zone overlays
    for (auto& z : zones) drawZoneOverlay(z, origin);

    // 3. Alfombras
    DrawEllipse(gridToIso(2, 11, origin).x, gridToIso(2, 11, origin).y+4, 24, 12, alpha({100, 60, 180, 255}, 25));
    DrawEllipse(gridToIso(10, 10, origin).x, gridToIso(10, 10, origin).y+4, 30, 14, alpha({80, 50, 20, 255}, 30));

    // 4. Sombras de mobiliario
    drawFurnitureShadow(gridToIso(3, 3, origin), 28, 10);
    drawFurnitureShadow(gridToIso(12, 4, origin), 28, 10);
    drawFurnitureShadow(gridToIso(12, 2, origin), 20, 12);
    drawFurnitureShadow(gridToIso(13, 2, origin), 20, 12);
    drawFurnitureShadow(gridToIso(3, 11, origin), 36, 12);
    drawFurnitureShadow(gridToIso(12, 12, origin), 36, 12);

    // 5. Mobiliario
    drawFurnitureTex(gridToIso(3, 3, origin), texDesk, {-24, -8}, WHITE);
    drawFurnitureTex(gridToIso(12, 4, origin), texDesk, {-24, -8}, WHITE);
    drawFurnitureTex(gridToIso(12, 2, origin), texServer, {-17, -38}, WHITE);
    drawFurnitureTex(gridToIso(13, 2, origin), texServer, {-17, -38}, WHITE);
    drawFurnitureTex(gridToIso(3, 11, origin), texMeeting, {-32, -16}, WHITE);
    drawFurnitureTex(gridToIso(12, 12, origin), texLounge, {-32, -10}, WHITE);
    drawFurnitureTex(gridToIso(0, 10, origin), texClock, {-9, -9}, WHITE);
    drawFurnitureTex(gridToIso(7, 10, origin), texWhiteboard, {-18, -14}, WHITE);
    drawFurnitureTex(gridToIso(1, 1, origin), texPlant, {-12, -12}, WHITE);
    drawFurnitureTex(gridToIso(14, 14, origin), texPlant, {-12, -12}, WHITE);
    drawFurnitureTex(gridToIso(7, 14, origin), texPlant, {-12, -12}, WHITE);
    drawFurnitureTex(gridToIso(11, 7, origin), texFrame, {-11, -12}, WHITE);
    drawFurnitureTex(gridToIso(0, 5, origin), texFrame, {-11, -12}, WHITE);

    // 6. Luces de zona
    for (int i = 0; i < 4; i++) {
        auto& z = zones[i];
        auto center = gridToIso((z.xMin + z.xMax) / 2.0f, (z.yMin + z.yMax) / 2.0f, origin);
        float pulse = sinf(time * 0.8f + i * 1.5f) * 0.15f + 0.85f;
        DrawCircleV(center, 80, alpha(z.stroke, (int)(8 * pulse)));
    }

    // 7. Paredes pseudo-3D (dibujadas después del piso y muebles, antes de entidades)
    drawWalls(origin);

    // 8. Entidades ordenadas por profundidad
    std::vector<int> idx(entities.size());
    for (int i = 0; i < (int)entities.size(); i++) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return entities[a].renderX + entities[a].renderY <
               entities[b].renderX + entities[b].renderY;
    });
    for (int i : idx)
        drawEntity(entities[i], gridToIso(entities[i].renderX, entities[i].renderY, origin));
}