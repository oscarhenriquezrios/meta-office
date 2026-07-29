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
// Tile drawing
// ============================================================
struct ZoneDef {
    int xMin, xMax, yMin, yMax;
    const char* name;
    Color fill, stroke;
    Color floorEven, floorOdd;
};

static const ZoneDef zones[] = {
    {0,7,0,7, "AREA DE DESARROLLO",   {20,40,70,20},  {56,189,248,200},  {17,30,48,255}, {14,24,40,255}},
    {8,15,0,7, "SALA DE SERVIDORES",  {15,40,25,20},  {16,185,129,200},  {15,36,29,255}, {10,26,21,255}},
    {0,7,8,15, "SALA DE CONFERENCIAS",{40,20,50,20},  {168,85,247,200},  {31,21,46,255}, {23,15,36,255}},
    {8,15,8,15, "LOUNGE & CAFETERIA", {50,35,10,20},  {245,158,11,200},  {38,27,17,255}, {30,21,12,255}},
};

static int getZone(int gx, int gy) {
    if (gx <= 7 && gy <= 7) return 0;
    if (gx >= 8 && gy <= 7) return 1;
    if (gx <= 7 && gy >= 8) return 2;
    return 3;
}

static void drawTile(Vector2 p, int gx, int gy) {
    int z = getZone(gx, gy);
    Color c = ((gx + gy) % 2 == 0) ? zones[z].floorEven : zones[z].floorOdd;
    DrawTriangle(p, {p.x+TILE_W/2.0f, p.y+TILE_H/2.0f}, {p.x, p.y+TILE_H}, c);
    DrawTriangle(p, {p.x, p.y+TILE_H}, {p.x-TILE_W/2.0f, p.y+TILE_H/2.0f}, c);
    DrawLineEx(p, {p.x+TILE_W/2.0f, p.y+TILE_H/2.0f}, 0.5f, alpha(WHITE, 8));
    DrawLineEx({p.x+TILE_W/2.0f, p.y+TILE_H/2.0f}, {p.x, p.y+TILE_H}, 0.5f, alpha(WHITE, 8));
    DrawLineEx({p.x, p.y+TILE_H}, {p.x-TILE_W/2.0f, p.y+TILE_H/2.0f}, 0.5f, alpha(WHITE, 8));
    DrawLineEx({p.x-TILE_W/2.0f, p.y+TILE_H/2.0f}, p, 0.5f, alpha(WHITE, 8));
}

static void drawZoneOverlay(const ZoneDef& z, Vector2 origin) {
    auto p1 = gridToIso(z.xMin, z.yMin, origin);
    auto p2 = gridToIso(z.xMax+1, z.yMin, origin);
    auto p3 = gridToIso(z.xMax+1, z.yMax+1, origin);
    auto p4 = gridToIso(z.xMin, z.yMax+1, origin);
    DrawLineEx(p1, p2, 1, z.stroke); DrawLineEx(p2, p3, 1, z.stroke);
    DrawLineEx(p3, p4, 1, z.stroke); DrawLineEx(p4, p1, 1, z.stroke);

    auto lbl = gridToIso(z.xMin+0.5f, z.yMin+0.8f, origin);
    DrawText(z.name, lbl.x - 40, lbl.y - 6, 10, z.stroke);
}

// ============================================================
// Furniture
// ============================================================
static void drawDesk(Vector2 pos, Color col) {
    DrawEllipse(pos.x, pos.y+24, 28, 12, alpha(BLACK, 100));
    DrawRectangle(pos.x-22, pos.y-8, 44, 18, {30,41,59,255});
    DrawRectangleLines(pos.x-22, pos.y-8, 44, 18, {71,85,105,255});
    DrawRectangle(pos.x-16, pos.y-24, 14, 12, {15,23,42,255});
    DrawRectangle(pos.x+2, pos.y-24, 14, 12, {15,23,42,255});
    DrawRectangle(pos.x-14, pos.y-22, 10, 8, col);
    DrawRectangle(pos.x+4, pos.y-22, 10, 8, {16,185,129,200});
}

static void drawServerRack(Vector2 pos) {
    DrawRectangle(pos.x-16, pos.y-35, 32, 45, {15,23,42,255});
    DrawRectangleLines(pos.x-16, pos.y-35, 32, 45, {51,65,85,255});
    for (int i=0;i<5;i++) {
        float y = pos.y-30+i*8;
        DrawRectangle(pos.x-12, y, 24, 5, {30,41,59,255});
        DrawCircle(pos.x-8, y+2.5f, 1.5f, {16,185,129,255});
        DrawCircle(pos.x-3, y+2.5f, 1.5f, {56,189,248,255});
    }
}

static void drawMeetingTable(Vector2 pos) {
    DrawEllipse(pos.x, pos.y+16, 36, 16, alpha(BLACK, 80));
    DrawEllipse(pos.x, pos.y, 32, 14, {51,65,85,255});
    DrawEllipseLines(pos.x, pos.y, 32, 14, {100,116,139,255});
    DrawCircle(pos.x, pos.y, 8, alpha({168,85,247}, 100));
}

static void drawCoffeeLounge(Vector2 pos) {
    DrawRectangle(pos.x-12, pos.y-4, 24, 8, {92,64,51,255});
    DrawRectangle(pos.x-10, pos.y-2, 20, 4, {139,94,60,255});
    DrawCircle(pos.x, pos.y-8, 4, {254,243,199,255});
    DrawCircle(pos.x, pos.y-8, 2, {217,119,6,255});
    DrawRectangle(pos.x+18, pos.y-8, 8, 10, {139,94,60,255});
    DrawCircle(pos.x+22, pos.y-14, 6, {34,197,94,255});
}

static void drawWhiteboard(Vector2 pos) {
    DrawRectangle(pos.x-16, pos.y-14, 32, 20, {51,65,85,255});
    DrawRectangle(pos.x-14, pos.y-12, 28, 16, {226,232,240,255});
    DrawRectangle(pos.x-10, pos.y-10, 8, 6, {251,191,36,255});
    DrawRectangle(pos.x+2, pos.y-8, 8, 6, {244,114,182,255});
    DrawRectangle(pos.x-6, pos.y-2, 8, 5, {56,189,248,255});
    DrawText("SPRINT", pos.x-8, pos.y+2, 8, {30,41,59,255});
    DrawText("GOALS", pos.x+4, pos.y+2, 8, {30,41,59,255});
}

static void drawWallClock(Vector2 pos, double t) {
    DrawCircle(pos.x, pos.y, 6, {30,41,59,255});
    DrawCircleLines(pos.x, pos.y, 6, {71,85,105,255});
    float hour = t * 0.05f, min = t * 0.6f;
    DrawLine(pos.x, pos.y, pos.x+sinf(hour)*3, pos.y-cosf(hour)*3, {148,163,184,255});
    DrawLine(pos.x, pos.y, pos.x+sinf(min)*4, pos.y-cosf(min)*4, {203,213,225,255});
}

static void drawPottedPlant(Vector2 pos) {
    DrawCircle(pos.x, pos.y-12, 6, {34,197,94,255});
    DrawRectangle(pos.x-3, pos.y-2, 6, 8, {139,94,60,255});
}

// ============================================================
// Entity drawing
// ============================================================
static void drawEntity(const Entity& e, Vector2 pos) {
    float centerY = pos.y + TILE_H/2.0f;
    float bob = (e.status == Status::Walking) ? sinf(GetTime()*15)*3 : sinf(GetTime()*3)*1.5f;
    float avY = centerY - 16 + bob;

    DrawEllipse(pos.x, centerY+8, 14, 7, alpha(BLACK, 100));

    switch (e.type) {
        case EntityType::Human:
            DrawCircle(pos.x, avY-8, 8, {245,158,11,255});
            DrawCircleLines(pos.x, avY-8, 8, WHITE);
            DrawCircle(pos.x, avY+4, 10, {217,119,6,255});
            break;
        case EntityType::CodeBot:
        case EntityType::DataBot: {
            DrawLine(pos.x, avY-14, pos.x, avY-20, e.color);
            DrawCircle(pos.x, avY-21, 3, WHITE);
            DrawRectangle(pos.x-10, avY-14, 20, 12, {30,41,59,255});
            DrawRectangleLines(pos.x-10, avY-14, 20, 12, e.color);
            Color eye = e.hasCriticalError ? RED : e.color;
            DrawRectangle(pos.x-6, avY-10, 4, 4, eye);
            DrawRectangle(pos.x+2, avY-10, 4, 4, eye);
            DrawRectangle(pos.x-12, avY, 24, 14, {15,23,42,255});
            DrawRectangleLines(pos.x-12, avY, 24, 14, e.color);
            break;
        }
        case EntityType::Orchestrator:
            DrawCircle(pos.x, avY, 10, {126,34,206,255});
            DrawCircleLines(pos.x, avY, 10, {168,85,247,255});
            DrawCircle(pos.x, avY-6, 6, {168,85,247,255});
            DrawCircleLines(pos.x, avY-6, 6, WHITE);
            break;
    }

    // Name
    int tw = MeasureText(e.name.c_str(), 10);
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
    // Tiles
    for (int gx = 0; gx < GRID_W; gx++)
        for (int gy = 0; gy < GRID_H; gy++)
            drawTile(gridToIso(gx, gy, origin), gx, gy);

    // Zone overlays
    for (auto& z : zones) drawZoneOverlay(z, origin);

    // Furniture
    drawDesk(gridToIso(3, 3, origin), {56,189,248,255});
    drawDesk(gridToIso(12, 4, origin), {16,185,129,255});
    drawServerRack(gridToIso(12, 2, origin));
    drawServerRack(gridToIso(13, 2, origin));
    drawMeetingTable(gridToIso(3, 11, origin));
    drawCoffeeLounge(gridToIso(12, 12, origin));
    drawWallClock(gridToIso(0, 10, origin), time);
    drawWhiteboard(gridToIso(7, 10, origin));
    drawPottedPlant(gridToIso(1, 1, origin));
    drawPottedPlant(gridToIso(14, 14, origin));
    drawPottedPlant(gridToIso(7, 14, origin));

    // Entities sorted by depth
    std::vector<int> idx(entities.size());
    for (int i = 0; i < (int)entities.size(); i++) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) {
        return entities[a].renderX + entities[a].renderY <
               entities[b].renderX + entities[b].renderY;
    });
    for (int i : idx)
        drawEntity(entities[i], gridToIso(entities[i].renderX, entities[i].renderY, origin));
}
