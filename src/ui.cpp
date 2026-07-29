#include "meta_office.hpp"
#include <cstring>

// Estado para el input de configuración LLM
static char inputEndpoint[256] = "";
static char inputApiKey[256] = "";
static char inputModel[128] = "";
static int editField = 0; // 0=ninguno, 1=endpoint, 2=apikey, 3=model
static bool inputActive = false;

void toggleLlmConfig() {
    g_showLlmConfig = !g_showLlmConfig;
    if (g_showLlmConfig) {
        // Cargar valores actuales en los buffers
        strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
        strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
        // Ofuscar key parcialmente para mostrar
        if (strlen(inputApiKey) > 8) {
            for (size_t i = 4; i < strlen(inputApiKey)-4; i++)
                inputApiKey[i] = '*';
        }
        strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1);
        editField = 0;
        inputActive = false;
    }
}

void drawLlmConfigPanel() {
    if (!g_showLlmConfig) return;

    // Fondo semi-transparente
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 180));

    int pw = 520, ph = 380;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2;

    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,240});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {56,189,248,100});

    DrawText("CONFIGURACION LLM", px + 20, py + 16, 18, WHITE);
    DrawText("Protocolo compatible con OpenAI API", px + 20, py + 38, 11, {148,163,184,255});

    int yy = py + 64;
    int labelW = 120;

    // Endpoint
    DrawText("Endpoint:", px + 20, yy + 4, 12, {148,163,184,255});
    DrawRectangle(px + labelW + 10, yy, pw - labelW - 40, 24,
                  editField == 1 ? alpha({56,189,248}, 30) : alpha(WHITE, 10));
    DrawRectangleLines(px + labelW + 10, yy, pw - labelW - 40, 24,
                       editField == 1 ? (Color){56,189,248,200} : (Color){255,255,255,30});
    DrawText(inputEndpoint, px + labelW + 16, yy + 5, 11, WHITE);
    yy += 34;

    // API Key
    DrawText("API Key:", px + 20, yy + 4, 12, {148,163,184,255});
    DrawRectangle(px + labelW + 10, yy, pw - labelW - 40, 24,
                  editField == 2 ? alpha({56,189,248}, 30) : alpha(WHITE, 10));
    DrawRectangleLines(px + labelW + 10, yy, pw - labelW - 40, 24,
                       editField == 2 ? (Color){56,189,248,200} : (Color){255,255,255,30});
    // Mostrar key ofuscada
    std::string display = inputApiKey;
    if (display.length() > 8) {
        display = display.substr(0, 4) + "..." + display.substr(display.length()-4);
    }
    DrawText(display.c_str(), px + labelW + 16, yy + 5, 11, {100,200,100,255});
    yy += 34;

    // Model
    DrawText("Modelo:", px + 20, yy + 4, 12, {148,163,184,255});
    DrawRectangle(px + labelW + 10, yy, pw - labelW - 40, 24,
                  editField == 3 ? alpha({56,189,248}, 30) : alpha(WHITE, 10));
    DrawRectangleLines(px + labelW + 10, yy, pw - labelW - 40, 24,
                       editField == 3 ? (Color){56,189,248,200} : (Color){255,255,255,30});
    DrawText(inputModel, px + labelW + 16, yy + 5, 11, WHITE);
    yy += 50;

    // Botones
    Rectangle btnSave = {(float)px + 20, (float)yy, 140, 32};
    Rectangle btnTest = {(float)px + 180, (float)yy, 140, 32};
    Rectangle btnClose = {(float)px + pw - 120, (float)yy, 100, 32};

    // Guardar
    DrawRectangleRounded(btnSave, 0.2f, 4, {56,189,248,200});
    DrawText("GUARDAR", btnSave.x + 28, btnSave.y + 8, 12, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnSave) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        if (strlen(inputEndpoint) > 5 && strlen(inputApiKey) > 5) {
            g_llmConfig.endpoint = inputEndpoint;
            g_llmConfig.apiKey = inputApiKey;
            if (strlen(inputModel) > 2) g_llmConfig.model = inputModel;
            saveLlmConfig(); // Persistir a llm_config.env
            g_showLlmConfig = false;
        }
    }

    // Test
    DrawRectangleRounded(btnTest, 0.2f, 4, {16,185,129,180});
    DrawText("TEST", btnTest.x + 42, btnTest.y + 8, 12, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnTest) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        LLMConfig testCfg = g_llmConfig;
        if (!inputEndpoint[0]) strncpy(inputEndpoint, testCfg.endpoint.c_str(), sizeof(inputEndpoint)-1);
        if (!inputApiKey[0]) strncpy(inputApiKey, testCfg.apiKey.c_str(), sizeof(inputApiKey)-1);
        if (!inputModel[0]) strncpy(inputModel, testCfg.model.c_str(), sizeof(inputModel)-1);
        testCfg.endpoint = inputEndpoint;
        testCfg.apiKey = inputApiKey;
        testCfg.model = inputModel;
        auto reply = llmChat(testCfg, {{"user", "Responde solo: OK"}});
        if (!reply.empty()) {
            // Conexion exitosa - cerrar panel
            g_llmConfig.endpoint = inputEndpoint;
            g_llmConfig.apiKey = inputApiKey;
            g_llmConfig.model = inputModel;
            g_showLlmConfig = false;
        }
    }

    // Cerrar
    DrawRectangleRounded(btnClose, 0.2f, 4, alpha(RED, 100));
    DrawText("CERRAR", btnClose.x + 18, btnClose.y + 8, 12, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnClose) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        g_showLlmConfig = false;
    }

    // Ayuda
    yy += 50;
    DrawText("Usa el protocolo OpenAI compatible:", px + 20, yy, 10, {100,116,139,255});
    yy += 14;
    DrawText("OpenAI: https://api.openai.com/v1/chat/completions", px + 20, yy, 9, {148,163,184,255});
    yy += 12;
    DrawText("OpenRouter: https://openrouter.ai/api/v1/chat/completions", px + 20, yy, 9, {148,163,184,255});
    yy += 12;
    DrawText("DeepSeek: https://api.deepseek.com/v1/chat/completions", px + 20, yy, 9, {148,163,184,255});
    yy += 12;
    DrawText("Local (Ollama): http://localhost:11434/v1/chat/completions", px + 20, yy, 9, {148,163,184,255});

    // Input handling para campos de texto
    if (inputActive && editField > 0) {
        int key = GetCharPressed();
        while (key > 0) {
            char* buf = nullptr;
            size_t max = 0;
            if (editField == 1) { buf = inputEndpoint; max = sizeof(inputEndpoint)-1; }
            else if (editField == 2) { buf = inputApiKey; max = sizeof(inputApiKey)-1; }
            else if (editField == 3) { buf = inputModel; max = sizeof(inputModel)-1; }
            if (buf) {
                size_t len = strlen(buf);
                if (len < max - 1 && key >= 32 && key <= 126) {
                    buf[len] = (char)key;
                    buf[len+1] = '\0';
                }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            char* buf = nullptr;
            if (editField == 1) buf = inputEndpoint;
            else if (editField == 2) buf = inputApiKey;
            else if (editField == 3) buf = inputModel;
            if (buf) {
                size_t len = strlen(buf);
                if (len > 0) buf[len-1] = '\0';
            }
        }
        if (IsKeyPressed(KEY_ENTER)) inputActive = false;
        if (IsKeyPressed(KEY_ESCAPE)) { inputActive = false; editField = 0; }
    }
}

void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused) {
    // ===== TOP BAR =====
    DrawRectangle(0, 0, screenW, 50, {15,23,42,220});
    DrawLine(0, 50, screenW, 50, {255,255,255,20});

    DrawText("META-OFFICE 2D", 20, 14, 20, WHITE);
    DrawText("SIMULACION META-OPERATIVA C++", 180, 18, 12, {148,163,184,255});

    // Estado LLM
    if (!g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...") {
        DrawCircle(410, 22, 4, {16,185,129,255});
        DrawText("LLM", 420, 16, 11, {16,185,129,255});
    } else {
        DrawCircle(410, 22, 4, ORANGE);
        DrawText("LLM", 420, 16, 11, ORANGE);
    }

    if (!entities.empty()) {
        auto& h = entities[0];
        DrawText(TextFormat("Pos: (%.0f, %.0f)", h.x, h.y), 470, 18, 12, {148,163,184,255});
    }

    DrawCircle(screenW - 240, 16, 5, paused ? ORANGE : GREEN);
    DrawText(paused ? "PAUSADO" : "EN TIEMPO REAL", screenW - 230, 12, 12,
             paused ? ORANGE : GREEN);

    // Botón Config LLM (engranaje)
    Rectangle btnConfig = {(float)screenW - 170, 8, 40, 34};
    DrawRectangleRounded(btnConfig, 0.2f, 4, g_showLlmConfig ? alpha({56,189,248}, 60) : alpha(WHITE, 10));
    DrawRectangleRoundedLines(btnConfig, 0.2f, 4, g_showLlmConfig ? (Color){56,189,248,200} : alpha(WHITE, 30));
    DrawText("⚙️", btnConfig.x + 6, btnConfig.y + 3, 20, WHITE);
    if (CheckCollisionPointRec(GetMousePosition(), btnConfig) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        toggleLlmConfig();
    }

    // Botón Pausar
    DrawRectangle(screenW - 120, 8, 110, 34, paused ? (Color){245,158,11,60} : (Color){255,255,255,20});
    DrawRectangleLines(screenW - 120, 8, 110, 34, {255,255,255,40});
    DrawText(paused ? "REANUDAR" : "PAUSAR", screenW - 90, 16, 14, WHITE);

    // ===== SIDEBAR =====
    int sx = screenW - 300;
    DrawRectangle(sx, 50, 300, screenH - 50, {10,14,23,200});
    DrawLine(sx, 50, sx, screenH, {255,255,255,15});

    DrawText("MIEMBROS", sx + 12, 58, 14, {148,163,184,255});
    DrawText(TextFormat("(%zu)", entities.size()), sx + 100, 58, 14, {100,116,139,255});

    int yy = 80;
    for (auto& e : entities) {
        bool sel = (e.id == selectedId);
        Rectangle card = {(float)sx + 10, (float)yy, 280, 58};
        DrawRectangleRounded(card, 0.1f, 4, sel ? alpha(e.color, 30) : alpha(WHITE, 5));
        DrawRectangleRoundedLines(card, 0.1f, 4, sel ? e.color : alpha(WHITE, 15));

        const char* icon = "?";
        if (e.type == EntityType::Human) icon = "👤";
        else if (e.type == EntityType::CodeBot) icon = "🤖";
        else if (e.type == EntityType::DataBot) icon = "📊";
        else if (e.type == EntityType::Orchestrator) icon = "🔮";

        DrawRectangle(sx + 18, yy + 10, 32, 32, e.color);
        DrawText(icon, sx + 24, yy + 12, 16, WHITE);
        DrawText(e.name.c_str(), sx + 58, yy + 12, 12, WHITE);
        DrawText(e.role.c_str(), sx + 58, yy + 28, 9, {148,163,184,255});

        const char* st = "IDLE";
        Color sc = {148,163,184,255};
        if (e.status == Status::Walking) { st = "CAMINANDO"; sc = {56,189,248,255}; }
        else if (e.status == Status::Error) { st = "ERROR"; sc = RED; }
        else if (e.status == Status::Busy) { st = "OCUPADO"; sc = ORANGE; }
        else if (e.status == Status::Intervening) { st = "AYUDANDO"; sc = {168,85,247,255}; }
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

    // ===== HUD =====
    DrawRectangle(10, screenH - 40, 500, 30, alpha(BLACK, 150));
    DrawText("WASD: mover | Clic: inspeccionar | P: pausar | ⚙️: config LLM", 18, screenH - 33, 12, {148,163,184,255});

    // ===== LLM Config Panel (overlay) =====
    drawLlmConfigPanel();
}

void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY) {
    // No procesar input normal si el panel de config está abierto
    if (g_showLlmConfig) {
        // Click en campos de texto para activar edición
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 520;
            int px = (screenW - pw) / 2;
            int py = (screenH - 380) / 2;
            int labelW = 130;
            int yy = py + 64;

            // Endpoint field
            if (mp.x > px + labelW + 10 && mp.x < px + pw - 30 &&
                mp.y > yy && mp.y < yy + 24) {
                editField = 1; inputActive = true;
                strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
            }
            yy += 34;
            // API Key field
            if (mp.x > px + labelW + 10 && mp.x < px + pw - 30 &&
                mp.y > yy && mp.y < yy + 24) {
                editField = 2; inputActive = true;
                strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
            }
            yy += 34;
            // Model field
            if (mp.x > px + labelW + 10 && mp.x < px + pw - 30 &&
                mp.y > yy && mp.y < yy + 24) {
                editField = 3; inputActive = true;
                strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1);
            }
        }
        return;
    }

    if (IsKeyPressed(KEY_P)) paused = !paused;

    // Botón ⚙️ en top bar
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        Rectangle btnConfig = {(float)screenW - 170, 8, 40, 34};
        if (CheckCollisionPointRec(mp, btnConfig)) {
            toggleLlmConfig();
            return;
        }
    }

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

        if (mp.x < screenW - 300) {
            Rectangle btnPause = {(float)screenW - 120, 8, 110, 34};
            if (CheckCollisionPointRec(mp, btnPause)) {
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

        int sx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sx + 10, (float)(80 + i * 64), 280, 58};
            if (CheckCollisionPointRec(mp, card)) { selectedId = entities[i].id; break; }
        }
    }

    float wheel = GetMouseWheelMove();
    panY += wheel * 5;
    origin.y += wheel * 5;
}
