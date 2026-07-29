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
// Texturas procedurales — VERSION GRANDE (2x)
// ============================================================
static Texture2D texFloor[4][2];
// Texturas de muebles a tamano grande
static Texture2D texDesk, texServer, texMeeting, texLounge, texWhiteboard, texClock, texPlant, texFrame;
static bool texturesLoaded = false;

static constexpr float WALL_H = 28.0f;

// ============================================================
// Info de muebles (para hover y interaccion)
// ============================================================
struct FurnitureInfo {
    const char* name;
    const char* description;
    int gx, gy;           // posicion en el grid
    float drawW, drawH;   // tamano de dibujo
    float offX, offY;     // offset
};

static FurnitureInfo furnitureList[] = {
    {"Escritorio de Trabajo", "Estacion de desarrollo - 2 monitores",  3, 3,  92, 72, -46, -10},
    {"Escritorio de Analisis", "Estacion de datos - 2 monitores",       12, 4,  92, 72, -46, -10},
    {"Rack Servidor A", "Servidor de produccion - LEDs activos",        12, 2,  64, 92, -32, -72},
    {"Rack Servidor B", "Servidor de respaldo - LEDs activos",          13, 2,  64, 92, -32, -72},
    {"Mesa de Reuniones", "Mesa con holograma - zona de conferencias",  3, 11, 120, 64, -60, -28},
    {"Lounge & Cafeteria", "Sofa, mesa, cafe y plantas",                12, 12, 120, 64, -60, -16},
    {"Reloj de Pared", "Hora del sistema en tiempo real",                 0, 10,  36, 36, -18, -22},
    {"Pizarra de Diagramas", "Diagramas y notas del equipo",              7, 10,  72, 48, -36, -32},
    {"Planta Decorativa", "Ficus - purifica el aire",                    1, 1,  48, 44, -24, -24},
    {"Planta Decorativa", "Ficus - purifica el aire",                   14, 14,  48, 44, -24, -24},
    {"Planta Decorativa", "Ficus - purifica el aire",                     7, 14,  48, 44, -24, -24},
    {"Cuadro Abstracto", "Arte generativo - zona reunion",               11, 7,  40, 32, -20, -28},
    {"Cuadro Abstracto", "Arte generativo - zona desarrollo",             0, 5,  40, 32, -20, -28},
};
static constexpr int FURNITURE_COUNT = 13;

static int g_hoverFurniture = -1; // indice del mueble bajo el mouse

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

    // --- Escritorio GRANDE (96x72) ---
    {
        Image img = GenImageColor(96, 72, {0,0,0,0});
        // Sombra
        ImageDrawRectangle(&img, 6, 48, 84, 20, (Color){0,0,0,90});
        // Superficie
        ImageDrawRectangle(&img, 0, 0, 96, 40, (Color){38, 52, 72, 255});
        ImageDrawRectangle(&img, 0, 0, 96, 4, (Color){62, 80, 105, 255});
        // Monitor izq
        ImageDrawRectangle(&img, 8, 10, 28, 22, (Color){12, 18, 32, 255});
        ImageDrawRectangle(&img, 12, 14, 20, 14, (Color){56, 189, 248, 220});
        ImageDrawPixel(&img, 14, 16, (Color){200,240,255,255});
        ImageDrawPixel(&img, 20, 19, (Color){200,240,255,255});
        // Monitor der
        ImageDrawRectangle(&img, 56, 10, 28, 22, (Color){12, 18, 32, 255});
        ImageDrawRectangle(&img, 60, 14, 20, 14, (Color){16, 185, 129, 220});
        ImageDrawPixel(&img, 62, 16, (Color){100,255,200,255});
        ImageDrawPixel(&img, 68, 19, (Color){100,255,200,255});
        // Teclado
        ImageDrawRectangle(&img, 24, 32, 48, 6, (Color){65, 75, 95, 220});
        // Base/patas
        ImageDrawRectangle(&img, 4, 36, 88, 4, (Color){25, 35, 50, 255});
        texDesk = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Rack servidor GRANDE (64x96) ---
    {
        Image img = GenImageColor(64, 96, {0,0,0,0});
        ImageDrawRectangle(&img, 6, 84, 52, 12, (Color){0,0,0,80});
        ImageDrawRectangle(&img, 0, 0, 64, 96, (Color){18, 26, 45, 255});
        ImageDrawRectangleLines(&img, (Rectangle){0, 0, 64, 96}, 2, (Color){55, 70, 95, 255});
        // Ventilación
        ImageDrawRectangle(&img, 8, 4, 48, 8, (Color){10, 15, 25, 255});
        for (int i = 0; i < 7; i++) {
            int y = 16 + i * 11;
            ImageDrawRectangle(&img, 6, y, 52, 8, (Color){28, 38, 55, 255});
            ImageDrawRectangle(&img, 6, y, 52, 1, (Color){45, 55, 75, 255});
            ImageDrawCircle(&img, 16, y+4, 3, (Color){16, 185, 129, 255});
            ImageDrawCircle(&img, 16, y+4, 1, (Color){160, 255, 210, 255});
            ImageDrawCircle(&img, 48, y+4, 3, (Color){56, 189, 248, 255});
            ImageDrawCircle(&img, 48, y+4, 1, (Color){160, 230, 255, 255});
        }
        texServer = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Mesa reuniones GRANDE (120x64) ---
    {
        Image img = GenImageColor(120, 64, {0,0,0,0});
        ImageDrawRectangle(&img, 8, 48, 104, 14, (Color){0,0,0,70});
        ImageDrawRectangle(&img, 8, 20, 104, 24, (Color){48, 58, 80, 255});
        ImageDrawRectangleLines(&img, (Rectangle){8, 20, 104, 24}, 1, (Color){95, 110, 135, 255});
        // Holograma centro brillante
        ImageDrawCircle(&img, 60, 32, 14, (Color){168, 85, 247, 50});
        ImageDrawCircle(&img, 60, 32, 9, (Color){200, 120, 255, 80});
        ImageDrawCircle(&img, 60, 32, 5, (Color){220, 150, 255, 120});
        ImageDrawCircle(&img, 60, 32, 2, (Color){240, 200, 255, 200});
        // Sillas (4 puntitos alrededor)
        ImageDrawCircle(&img, 20, 8, 6, (Color){70, 80, 100, 200});
        ImageDrawCircle(&img, 100, 8, 6, (Color){70, 80, 100, 200});
        ImageDrawCircle(&img, 20, 56, 6, (Color){70, 80, 100, 200});
        ImageDrawCircle(&img, 100, 56, 6, (Color){70, 80, 100, 200});
        texMeeting = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Lounge GRANDE (120x64) ---
    {
        Image img = GenImageColor(120, 64, {0,0,0,0});
        ImageDrawRectangle(&img, 4, 48, 112, 14, (Color){0,0,0,50});
        // Mesa centro
        ImageDrawRectangle(&img, 38, 24, 44, 14, (Color){85, 58, 42, 255});
        ImageDrawRectangle(&img, 42, 28, 36, 6, (Color){130, 85, 55, 255});
        // Taza de café
        ImageDrawCircle(&img, 60, 16, 7, (Color){254, 243, 199, 255});
        ImageDrawCircle(&img, 60, 16, 4, (Color){160, 90, 20, 255});
        ImageDrawCircle(&img, 60, 16, 1, (Color){200, 120, 40, 255});
        // Vapor del café
        ImageDrawLine(&img, 57, 8, 55, 2, (Color){200,200,220,120});
        ImageDrawLine(&img, 63, 8, 65, 2, (Color){200,200,220,120});
        // Maceta
        ImageDrawRectangle(&img, 92, 24, 16, 18, (Color){125, 82, 52, 255});
        ImageDrawRectangle(&img, 90, 20, 20, 5, (Color){155, 100, 65, 255});
        ImageDrawCircle(&img, 100, 12, 10, (Color){34, 197, 94, 255});
        ImageDrawCircle(&img, 92, 14, 7, (Color){22, 163, 74, 240});
        ImageDrawCircle(&img, 108, 12, 7, (Color){50, 210, 110, 240});
        ImageDrawLine(&img, 100, 22, 96, 12, (Color){20,120,50,200});
        ImageDrawLine(&img, 100, 22, 106, 14, (Color){20,120,50,200});
        // Sofá grande
        ImageDrawRectangle(&img, 4, 30, 26, 18, (Color){70, 50, 40, 255});
        ImageDrawRectangle(&img, 4, 26, 26, 6, (Color){90, 65, 50, 255});
        ImageDrawRectangle(&img, 8, 30, 18, 2, (Color){60, 42, 35, 255});
        // Cojines del sofa
        ImageDrawCircle(&img, 12, 35, 5, (Color){100, 70, 55, 255});
        ImageDrawCircle(&img, 22, 35, 5, (Color){100, 70, 55, 255});
        texLounge = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Pizarra GRANDE (72x44) ---
    {
        Image img = GenImageColor(72, 44, {0,0,0,0});
        // Marco
        ImageDrawRectangle(&img, 0, 0, 72, 44, (Color){55, 70, 90, 255});
        ImageDrawRectangle(&img, 4, 4, 64, 36, (Color){230, 235, 245, 255});
        // Notas de colores
        ImageDrawRectangle(&img, 8, 8, 16, 12, (Color){251, 191, 36, 255});
        ImageDrawRectangle(&img, 30, 10, 16, 12, (Color){244, 114, 182, 255});
        ImageDrawRectangle(&img, 44, 22, 16, 10, (Color){56, 189, 248, 255});
        // Diagrama (líneas conectando)
        ImageDrawLine(&img, 24, 14, 30, 16, (Color){50,50,50,255});
        ImageDrawLine(&img, 46, 16, 44, 22, (Color){50,50,50,255});
        ImageDrawLine(&img, 16, 20, 30, 20, (Color){50,50,50,255});
        // Texto simulado (puntitos)
        for (int i = 0; i < 4; i++)
            ImageDrawLine(&img, 10 + i*4, 30, 12 + i*4, 30, (Color){80,80,80,255});
        texWhiteboard = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Reloj GRANDE (36x36) ---
    {
        Image img = GenImageColor(36, 36, {0,0,0,0});
        ImageDrawCircle(&img, 18, 18, 16, (Color){28, 38, 55, 255});
        ImageDrawCircleLines(&img, 18, 18, 16, (Color){75, 90, 115, 255});
        ImageDrawCircle(&img, 18, 18, 11, (Color){200, 205, 218, 255});
        ImageDrawCircle(&img, 18, 18, 3, (Color){40, 50, 65, 255});
        // Marcas de horas
        for (int h = 0; h < 12; h++) {
            float a = h * 30.0f * 3.14159f / 180.0f;
            ImageDrawCircle(&img, 18 + (int)(cosf(a) * 13), 18 + (int)(sinf(a) * 13), 1, (Color){80,80,90,255});
        }
        // Manecillas
        ImageDrawLine(&img, 18, 18, 18, 8, (Color){30,30,30,255});
        ImageDrawLine(&img, 18, 18, 28, 18, (Color){30,30,30,255});
        ImageDrawLine(&img, 18, 18, 22, 24, (Color){200,50,50,255});
        texClock = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Planta GRANDE (48x44) ---
    {
        Image img = GenImageColor(48, 44, {0,0,0,0});
        // Maceta
        ImageDrawRectangle(&img, 14, 26, 22, 18, (Color){130, 85, 55, 255});
        ImageDrawRectangle(&img, 11, 22, 28, 5, (Color){160, 105, 70, 255});
        // Hojas grandes
        ImageDrawCircle(&img, 24, 14, 14, (Color){34, 197, 94, 255});
        ImageDrawCircle(&img, 14, 12, 8, (Color){24, 163, 74, 240});
        ImageDrawCircle(&img, 34, 10, 8, (Color){50, 210, 110, 240});
        ImageDrawCircle(&img, 24, 6, 6, (Color){70, 225, 130, 220});
        // Brillo en hojas
        ImageDrawCircle(&img, 20, 10, 3, (Color){120, 230, 160, 180});
        ImageDrawCircle(&img, 28, 8, 2, (Color){150, 250, 180, 180});
        // Tallos
        ImageDrawLine(&img, 24, 22, 20, 14, (Color){20,120,50,200});
        ImageDrawLine(&img, 24, 22, 30, 14, (Color){20,120,50,200});
        ImageDrawLine(&img, 24, 22, 24, 10, (Color){20,120,50,200});
        texPlant = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Cuadro GRANDE (40x32) ---
    {
        Image img = GenImageColor(40, 32, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 40, 32, (Color){70, 50, 80, 255});
        ImageDrawRectangle(&img, 3, 3, 34, 26, (Color){115, 80, 130, 255});
        // Arte abstracto
        ImageDrawCircle(&img, 20, 16, 7, (Color){90, 60, 110, 255});
        ImageDrawCircle(&img, 16, 13, 3, (Color){135, 100, 150, 255});
        ImageDrawCircle(&img, 25, 19, 2, (Color){160, 120, 180, 255});
        ImageDrawLine(&img, 8, 8, 15, 20, (Color){100,70,120,255});
        ImageDrawLine(&img, 30, 10, 22, 24, (Color){120,80,140,255});
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
// Mobiliario escalado
// ============================================================
static const Texture2D& getFurnitureTex(int idx) {
    switch (idx) {
        case 0: case 1: return texDesk;
        case 2: case 3: return texServer;
        case 4: return texMeeting;
        case 5: return texLounge;
        case 6: return texClock;
        case 7: return texWhiteboard;
        case 8: case 9: case 10: return texPlant;
        case 11: case 12: return texFrame;
    }
    return texDesk;
}

static void drawFurnitureScaled(Vector2 pos, const Texture2D& tex, float drawW, float drawH, Vector2 offset, bool highlighted) {
    Color tint = highlighted ? ColorBrightness(WHITE, 0.15f) : WHITE;
    DrawTexturePro(tex,
        {0, 0, (float)tex.width, (float)tex.height},
        {pos.x + offset.x, pos.y + offset.y, drawW, drawH},
        {0, 0}, 0, tint);

    if (highlighted) {
        // Glow al hacer hover
        DrawRectangleLines((int)(pos.x + offset.x), (int)(pos.y + offset.y),
                           (int)drawW, (int)drawH, alpha({56,189,248,255}, 120));
    }
}

// ============================================================
// Hover check de muebles
// ============================================================
static void updateFurnitureHover(Vector2 origin) {
    Vector2 mp = GetMousePosition();
    g_hoverFurniture = -1;
    if (g_showChat || g_showLog || g_showLlmConfig) return;
    if (mp.x > screenW - 300) return; // sidebar

    for (int i = 0; i < FURNITURE_COUNT; i++) {
        auto& f = furnitureList[i];
        Vector2 pos = gridToIso(f.gx, f.gy, origin);
        Rectangle bounds = {pos.x + f.offX, pos.y + f.offY, f.drawW, f.drawH};
        if (CheckCollisionPointRec(mp, bounds)) {
            g_hoverFurniture = i;
            break;
        }
    }
}

// ============================================================
// Tooltip de mueble
// ============================================================
static void drawFurnitureTooltip(Vector2 origin) {
    if (g_hoverFurniture < 0) return;
    auto& f = furnitureList[g_hoverFurniture];
    Vector2 mp = GetMousePosition();

    int tw = MeasureText(f.name, 11);
    int dw = MeasureText(f.description, 10);
    int w = std::max(tw, dw) + 20;
    int h = 38;
    int tx = (int)mp.x + 14;
    int ty = (int)mp.y - h - 4;
    if (tx + w > screenW - 310) tx = (int)mp.x - w - 14;
    if (ty < 55) ty = (int)mp.y + 16;

    DrawRectangleRounded({(float)tx+2, (float)ty+2, (float)w, (float)h}, 0.15f, 4, alpha(BLACK, 80));
    DrawRectangleRounded({(float)tx, (float)ty, (float)w, (float)h}, 0.15f, 4, alpha({15,23,42,255}, 230));
    DrawRectangleRoundedLines({(float)tx, (float)ty, (float)w, (float)h}, 0.15f, 4, alpha({56,189,248,255}, 150));
    DrawText(f.name, tx + 10, ty + 6, 11, WHITE);
    DrawText(f.description, tx + 10, ty + 22, 10, {148,163,184,255});
}

// ============================================================
// Paredes pseudo-3D
// ============================================================
static void drawWalls(Vector2 origin) {
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
// Entity drawing
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

    if (!e.speech.text.empty() && GetTime() < e.speech.expiry) {
        int stw = MeasureText(e.speech.text.c_str(), 9);
        float bw = std::min(stw + 16.0f, 160.0f);
        float bx = pos.x - bw/2;
        float by = avY - 44;
        DrawRectangleRounded({bx+2, by+2, bw, 20}, 0.3f, 4, alpha(BLACK, 100));
        DrawRectangleRounded({bx, by, bw, 20}, 0.3f, 4, alpha(BLACK, 200));
        DrawRectangleRoundedLines({bx, by, bw, 20}, 0.3f, 4, e.color);
        DrawText(e.speech.text.c_str(), bx + 8, by + 5, 9, WHITE);
        DrawTriangle({pos.x - 4, by + 20}, {pos.x + 4, by + 20}, {pos.x, by + 26}, alpha(BLACK, 200));
    }
}

// ============================================================
// Sombra de mobiliario
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

    // Actualizar hover de muebles
    updateFurnitureHover(origin);

    // 1. Tiles
    for (int gx = 0; gx < GRID_W; gx++)
        for (int gy = 0; gy < GRID_H; gy++)
            drawTile(gridToIso(gx, gy, origin), gx, gy);

    // 2. Zone overlays
    for (auto& z : zones) drawZoneOverlay(z, origin);

    // 3. Alfombras
    DrawEllipse(gridToIso(2, 11, origin).x, gridToIso(2, 11, origin).y+4, 40, 18, alpha({100, 60, 180, 255}, 25));
    DrawEllipse(gridToIso(10, 10, origin).x, gridToIso(10, 10, origin).y+4, 48, 20, alpha({80, 50, 20, 255}, 30));

    // 4. Sombras de muebles grandes
    drawFurnitureShadow(gridToIso(3, 3, origin), 50, 16);
    drawFurnitureShadow(gridToIso(12, 4, origin), 50, 16);
    drawFurnitureShadow(gridToIso(12, 2, origin), 30, 16);
    drawFurnitureShadow(gridToIso(13, 2, origin), 30, 16);
    drawFurnitureShadow(gridToIso(3, 11, origin), 60, 18);
    drawFurnitureShadow(gridToIso(12, 12, origin), 60, 18);

    // 5. Mobiliario escalado + hover
    for (int i = 0; i < FURNITURE_COUNT; i++) {
        auto& f = furnitureList[i];
        Vector2 pos = gridToIso(f.gx, f.gy, origin);
        const Texture2D& tex = getFurnitureTex(i);
        bool highlighted = (g_hoverFurniture == i);
        drawFurnitureScaled(pos, tex, f.drawW, f.drawH, {f.offX, f.offY}, highlighted);
    }

    // 6. Luces de zona
    for (int i = 0; i < 4; i++) {
        auto& z = zones[i];
        auto center = gridToIso((z.xMin + z.xMax) / 2.0f, (z.yMin + z.yMax) / 2.0f, origin);
        float pulse = sinf(time * 0.8f + i * 1.5f) * 0.15f + 0.85f;
        DrawCircleV(center, 80, alpha(z.stroke, (int)(8 * pulse)));
    }

    // 7. Paredes pseudo-3D
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

    // 9. Tooltip de mueble (al frente)
    drawFurnitureTooltip(origin);
}