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
// Texturas procedurales generadas al cargar
// ============================================================
static Texture2D texFloor[4][2];  // [zona][paridad]
static Texture2D texDesk, texServer, texMeeting, texLounge, texWhiteboard, texClock, texPlant, texFrame;
static bool texturesLoaded = false;

static void initTextures() {
    if (texturesLoaded) return;

    Color zoneColors[4][2] = {
        {{25, 40, 70, 255}, {18, 30, 55, 255}},    // DEV - azul petróneo
        {{20, 50, 35, 255}, {14, 38, 25, 255}},    // DATA - verde oscuro
        {{45, 25, 60, 255}, {35, 18, 48, 255}},    // MEETING - púrpura
        {{55, 38, 18, 255}, {42, 28, 12, 255}}     // LOUNGE - marrón madera
    };

    // Generar baldosas con patrón de cuadrícula
    for (int z = 0; z < 4; z++) {
        for (int p = 0; p < 2; p++) {
            Image img = GenImageColor(TILE_W, TILE_H, zoneColors[z][p]);
            // Línea de borde más clara
            Color edge = ColorBrightness(zoneColors[z][p], 0.15f);
            ImageDrawRectangle(&img, 0, 0, TILE_W, 1, edge);
            ImageDrawRectangle(&img, 0, TILE_H-1, TILE_W, 1, edge);
            ImageDrawRectangle(&img, 0, 0, 1, TILE_H, edge);
            ImageDrawRectangle(&img, TILE_W-1, 0, 1, TILE_H, edge);
            // Patrón de textura (puntitos)
            for (int i = 0; i < 6; i++) {
                int dx = 6 + (i * 11) % (TILE_W - 12);
                int dy = 6 + (i * 7) % (TILE_H - 12);
                ImageDrawPixel(&img, dx, dy, ColorBrightness(zoneColors[z][p], 0.25f));
            }
            texFloor[z][p] = LoadTextureFromImage(img);
            UnloadImage(img);
        }
    }

    // --- Escritorio ---
    {
        Image img = GenImageColor(44, 32, {0,0,0,0});
        // Sombra del escritorio con rectángulo
        ImageDrawRectangle(&img, 2, 22, 40, 8, (Color){0,0,0,80});
        // Superficie
        ImageDrawRectangle(&img, 0, 0, 44, 18, (Color){35, 48, 68, 255});
        // Borde superior claro
        ImageDrawRectangle(&img, 0, 0, 44, 2, (Color){55, 70, 95, 255});
        // Monitor izq
        ImageDrawRectangle(&img, 6, 6, 12, 10, (Color){15, 20, 35, 255});
        ImageDrawRectangle(&img, 8, 8, 8, 6, (Color){56, 189, 248, 200});
        // Monitor der
        ImageDrawRectangle(&img, 26, 6, 12, 10, (Color){15, 20, 35, 255});
        ImageDrawRectangle(&img, 28, 8, 8, 6, (Color){16, 185, 129, 200});
        // Teclado
        ImageDrawRectangle(&img, 10, 16, 24, 3, (Color){60, 70, 90, 200});
        texDesk = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Rack servidor ---
    {
        Image img = GenImageColor(32, 45, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 32, 45, (Color){15, 23, 42, 255});
        ImageDrawRectangleLines(&img, (Rectangle){0, 0, 32, 45}, 1, (Color){51, 65, 85, 255});
        // Ventilación arriba
        ImageDrawRectangle(&img, 4, 2, 24, 4, (Color){10, 15, 25, 255});
        for (int i = 0; i < 5; i++) {
            int y = 10 + i * 7;
            ImageDrawRectangle(&img, 4, y, 24, 5, (Color){25, 35, 50, 255});
            ImageDrawCircle(&img, 10, y+2, 1.5f, (Color){16, 185, 129, 255});
            ImageDrawCircle(&img, 22, y+2, 1.5f, (Color){56, 189, 248, 255});
        }
        texServer = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Mesa reuniones ---
    {
        Image img = GenImageColor(64, 32, {0,0,0,0});
        // Sombra
        ImageDrawRectangle(&img, 4, 24, 56, 8, (Color){0,0,0,60});
        // Mesa ovalada simulada con rectángulo + bordes circulares
        ImageDrawRectangle(&img, 4, 10, 56, 12, (Color){45, 55, 75, 255});
        ImageDrawRectangleLines(&img, (Rectangle){4, 10, 56, 12}, 1, (Color){80, 95, 115, 255});
        // Holograma centro
        ImageDrawCircle(&img, 32, 16, 6, (Color){168, 85, 247, 80});
        texMeeting = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Lounge ---
    {
        Image img = GenImageColor(60, 32, {0,0,0,0});
        // Sombra
        ImageDrawRectangle(&img, 2, 22, 56, 8, (Color){0,0,0,40});
        // Mesa
        ImageDrawRectangle(&img, 18, 10, 24, 8, (Color){80, 55, 40, 255});
        ImageDrawRectangle(&img, 20, 12, 20, 4, (Color){120, 80, 50, 255});
        // Taza
        ImageDrawCircle(&img, 30, 6, 4, (Color){254, 243, 199, 255});
        ImageDrawCircle(&img, 30, 6, 2, (Color){200, 100, 20, 255});
        // Maceta
        ImageDrawRectangle(&img, 44, 10, 8, 10, (Color){120, 80, 50, 255});
        ImageDrawCircle(&img, 48, 4, 6, (Color){34, 197, 94, 255});
        texLounge = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Pizarra ---
    {
        Image img = GenImageColor(32, 20, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 32, 20, (Color){45, 55, 75, 255});
        ImageDrawRectangle(&img, 2, 2, 28, 16, (Color){220, 225, 235, 255});
        // Notas
        ImageDrawRectangle(&img, 4, 4, 8, 6, (Color){251, 191, 36, 255});
        ImageDrawRectangle(&img, 14, 6, 8, 6, (Color){244, 114, 182, 255});
        ImageDrawRectangle(&img, 20, 10, 8, 5, (Color){56, 189, 248, 255});
        texWhiteboard = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Reloj ---
    {
        Image img = GenImageColor(16, 16, {0,0,0,0});
        ImageDrawCircle(&img, 8, 8, 7, (Color){25, 35, 50, 255});
        ImageDrawCircleLines(&img, 8, 8, 7, (Color){60, 75, 95, 255});
        ImageDrawCircle(&img, 8, 8, 1.5f, (Color){200, 200, 220, 255});
        texClock = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Planta ---
    {
        Image img = GenImageColor(20, 18, {0,0,0,0});
        ImageDrawRectangle(&img, 6, 8, 8, 10, (Color){120, 80, 50, 255});
        ImageDrawCircle(&img, 10, 3, 8, (Color){34, 197, 94, 255});
        ImageDrawCircle(&img, 6, 5, 5, (Color){22, 163, 74, 200});
        ImageDrawCircle(&img, 14, 4, 5, (Color){22, 163, 74, 200});
        texPlant = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // --- Cuadro ---
    {
        Image img = GenImageColor(20, 16, {0,0,0,0});
        ImageDrawRectangle(&img, 0, 0, 20, 16, (Color){60, 40, 70, 255});
        ImageDrawRectangle(&img, 2, 2, 16, 12, (Color){100, 70, 120, 255});
        ImageDrawCircle(&img, 10, 8, 3, (Color){80, 50, 100, 255});
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

    // Dibujar textura con clipping isométrico
    Vector2 v1 = {p.x, p.y};
    Vector2 v2 = {p.x + TILE_W/2.0f, p.y + TILE_H/2.0f};
    Vector2 v3 = {p.x, p.y + TILE_H};
    Vector2 v4 = {p.x - TILE_W/2.0f, p.y + TILE_H/2.0f};

    DrawTriangle(v1, v2, v3, WHITE);
    DrawTriangle(v1, v3, v4, WHITE);
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
// Mobiliario con texturas
// ============================================================
static void drawFurnitureTex(Vector2 pos, Texture2D tex, Vector2 offset, Color tint) {
    DrawTexturePro(tex,
        {0, 0, (float)tex.width, (float)tex.height},
        {pos.x + offset.x, pos.y + offset.y, (float)tex.width, (float)tex.height},
        {0, 0}, 0, tint);
}

// ============================================================
// Entity drawing (avatares)
// ============================================================
static void drawEntity(const Entity& e, Vector2 pos) {
    float centerY = pos.y + TILE_H/2.0f;
    float bob = (e.status == Status::Walking) ? sinf(GetTime()*15)*3 : sinf(GetTime()*3)*1.5f;
    float avY = centerY - 16 + bob;

    DrawEllipse(pos.x, centerY+8, 14, 7, alpha(BLACK, 100));

    switch (e.type) {
        case EntityType::Human: {
            // Cuerpo con degradado
            DrawCircle(pos.x, avY+4, 10, {217, 119, 6, 255});
            DrawCircle(pos.x, avY-8, 8, {245, 158, 11, 255});
            DrawCircleLines(pos.x, avY-8, 8, WHITE);
            DrawCircle(pos.x-3, avY-9, 1.5f, WHITE);
            DrawCircle(pos.x+3, avY-9, 1.5f, WHITE);
            // Brazo
            DrawCircle(pos.x-10, avY+2, 3, {200, 100, 10, 200});
            DrawCircle(pos.x+10, avY+2, 3, {200, 100, 10, 200});
            break;
        }
        case EntityType::CodeBot:
        case EntityType::DataBot: {
            Color c = e.color;
            DrawLine(pos.x, avY-14, pos.x, avY-20, c);
            DrawCircle(pos.x, avY-21, 3, WHITE);
            DrawRectangle(pos.x-10, avY-14, 20, 12, {30, 41, 59, 255});
            DrawRectangleLines(pos.x-10, avY-14, 20, 12, c);
            Color eye = e.hasCriticalError ? RED : c;
            DrawRectangle(pos.x-6, avY-10, 4, 4, eye);
            DrawCircle(pos.x-4, avY-8, 1.5f, WHITE);
            DrawRectangle(pos.x+2, avY-10, 4, 4, eye);
            DrawCircle(pos.x+4, avY-8, 1.5f, WHITE);
            DrawRectangle(pos.x-12, avY, 24, 14, {15, 23, 42, 255});
            DrawRectangleLines(pos.x-12, avY, 24, 14, c);
            DrawRectangle(pos.x-4, avY+4, 8, 3, alpha(c, 80));
            break;
        }
        case EntityType::Orchestrator: {
            DrawEllipse(pos.x, avY, 18, 8, alpha({168, 85, 247}, 40));
            DrawEllipseLines(pos.x, avY, 18, 8, {168, 85, 247, 100});
            DrawCircle(pos.x, avY, 10, {126, 34, 206, 255});
            DrawCircleLines(pos.x, avY, 10, {168, 85, 247, 255});
            DrawCircle(pos.x, avY-6, 6, {168, 85, 247, 255});
            DrawCircleLines(pos.x, avY-6, 6, WHITE);
            DrawCircle(pos.x, avY-6, 2, WHITE);
            break;
        }
    }

    // Nombre con sombra
    int tw = MeasureText(e.name.c_str(), 10);
    DrawText(e.name.c_str(), pos.x - tw/2 + 1, avY - 25, 10, alpha(BLACK, 100));
    DrawText(e.name.c_str(), pos.x - tw/2, avY - 26, 10, WHITE);

    // Speech bubble
    if (!e.speech.text.empty() && GetTime() < e.speech.expiry) {
        int stw = MeasureText(e.speech.text.c_str(), 9);
        float bw = std::min(stw + 16.0f, 140.0f);
        float bx = pos.x - bw/2;
        float by = avY - 44;
        DrawRectangleRounded({bx, by, bw, 20}, 0.3f, 4, alpha(BLACK, 190));
        DrawRectangleRoundedLines({bx, by, bw, 20}, 0.3f, 4, e.color);
        DrawText(e.speech.text.c_str(), bx + 8, by + 5, 9, WHITE);
    }
}

// ============================================================
// Public drawScene
// ============================================================
void drawScene(const std::vector<Entity>& entities, Vector2 origin, double time) {
    if (!texturesLoaded) initTextures();

    // 1. Tiles
    for (int gx = 0; gx < GRID_W; gx++)
        for (int gy = 0; gy < GRID_H; gy++)
            drawTile(gridToIso(gx, gy, origin), gx, gy);

    // 2. Zone overlays
    for (auto& z : zones) drawZoneOverlay(z, origin);

    // 3. Alfombras decorativas
    DrawEllipse(gridToIso(2, 11, origin).x, gridToIso(2, 11, origin).y+4, 24, 12, alpha({100, 60, 180}, 25));
    DrawEllipse(gridToIso(10, 10, origin).x, gridToIso(10, 10, origin).y+4, 30, 14, alpha({80, 50, 20}, 30));

    // 4. Mobiliario con texturas
    drawFurnitureTex(gridToIso(3, 3, origin), texDesk, {-22, -8}, WHITE);
    drawFurnitureTex(gridToIso(12, 4, origin), texDesk, {-22, -8}, WHITE);
    drawFurnitureTex(gridToIso(12, 2, origin), texServer, {-16, -35}, WHITE);
    drawFurnitureTex(gridToIso(13, 2, origin), texServer, {-16, -35}, WHITE);
    drawFurnitureTex(gridToIso(3, 11, origin), texMeeting, {-32, -16}, WHITE);
    drawFurnitureTex(gridToIso(12, 12, origin), texLounge, {-30, -10}, WHITE);
    drawFurnitureTex(gridToIso(0, 10, origin), texClock, {-8, -8}, WHITE);
    drawFurnitureTex(gridToIso(7, 10, origin), texWhiteboard, {-16, -14}, WHITE);
    drawFurnitureTex(gridToIso(1, 1, origin), texPlant, {-10, -10}, WHITE);
    drawFurnitureTex(gridToIso(14, 14, origin), texPlant, {-10, -10}, WHITE);
    drawFurnitureTex(gridToIso(7, 14, origin), texPlant, {-10, -10}, WHITE);
    drawFurnitureTex(gridToIso(11, 7, origin), texFrame, {-10, -12}, WHITE);
    drawFurnitureTex(gridToIso(0, 5, origin), texFrame, {-10, -12}, WHITE);

    // 5. Entidades ordenadas por profundidad
    std::vector<int> idx(entities.size());
    for (int i = 0; i < (int)entities.size(); i++) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return entities[a].renderX + entities[a].renderY <
               entities[b].renderX + entities[b].renderY;
    });
    for (int i : idx)
        drawEntity(entities[i], gridToIso(entities[i].renderX, entities[i].renderY, origin));
}
