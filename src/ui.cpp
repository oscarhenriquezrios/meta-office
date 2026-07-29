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

// Helper: dibujar un botón con efecto hover
static bool drawButton(Rectangle btn, const char* label, Color bg, int fontSize) {
    Vector2 mp = GetMousePosition();
    bool hover = CheckCollisionPointRec(mp, btn);
    if (hover) bg = ColorBrightness(bg, 0.1f);
    DrawRectangleRounded(btn, 0.2f, 4, bg);
    DrawRectangleRoundedLines(btn, 0.2f, 4, hover ? alpha(WHITE, 80) : alpha(WHITE, 30));
    int tw = MeasureText(label, fontSize);
    DrawText(label, btn.x + (btn.width - tw)/2, btn.y + (btn.height - fontSize)/2, fontSize, WHITE);
    return hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON);
}

// Helper: dibujar campo de texto
static void drawTextField(Rectangle field, const char* label, const char* value, bool active) {
    DrawText(label, field.x - 110, field.y + 4, 12, {148,163,184,255});
    DrawRectangle(field.x, field.y, field.width, field.height,
                  active ? alpha({56,189,248}, 30) : alpha(WHITE, 8));
    DrawRectangleLines(field.x, field.y, field.width, field.height,
                       active ? (Color){56,189,248,200} : (Color){255,255,255,25});
    DrawText(value, field.x + 8, field.y + 5, 11, active ? WHITE : (Color){180,190,205,255});
    if (active && (int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(value, 11);
        DrawText("|", field.x + 8 + tw + 1, field.y + 5, 11, WHITE);
    }
}

void drawLlmConfigPanel() {
    if (!g_showLlmConfig) return;

    // Fondo semi-transparente
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 180));

    int pw = 540, ph = 400;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2;

    // Panel con sombra
    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {56,189,248,120});

    // Header con degradado
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha({56,189,248}, 20));
    DrawText("CONFIGURACION LLM", px + 24, py + 14, 18, WHITE);
    DrawText("Protocolo compatible con OpenAI API", px + 24, py + 36, 11, {148,163,184,255});

    int yy = py + 68;
    int fieldW = pw - 150;

    // Endpoint
    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "Endpoint:", inputEndpoint, editField == 1);
    yy += 38;

    // API Key
    std::string display = inputApiKey;
    if (display.length() > 8) {
        display = display.substr(0, 4) + "...." + display.substr(display.length()-4);
    }
    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "API Key:", display.c_str(), editField == 2);
    yy += 38;

    // Model
    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "Modelo:", inputModel, editField == 3);
    yy += 56;

    // Botones
    Rectangle btnSave = {(float)px + 20, (float)yy, 150, 34};
    Rectangle btnTest = {(float)px + 190, (float)yy, 120, 34};
    Rectangle btnClose = {(float)px + pw - 130, (float)yy, 110, 34};

    if (drawButton(btnSave, "GUARDAR", {56,189,248,220}, 13)) {
        if (strlen(inputEndpoint) > 5 && strlen(inputApiKey) > 5) {
            g_llmConfig.endpoint = inputEndpoint;
            g_llmConfig.apiKey = inputApiKey;
            if (strlen(inputModel) > 2) g_llmConfig.model = inputModel;
            saveLlmConfig();
            g_showLlmConfig = false;
        }
    }

    if (drawButton(btnTest, "TEST", {16,185,129,200}, 13)) {
        LLMConfig testCfg = g_llmConfig;
        if (!inputEndpoint[0]) strncpy(inputEndpoint, testCfg.endpoint.c_str(), sizeof(inputEndpoint)-1);
        if (!inputApiKey[0]) strncpy(inputApiKey, testCfg.apiKey.c_str(), sizeof(inputApiKey)-1);
        if (!inputModel[0]) strncpy(inputModel, testCfg.model.c_str(), sizeof(inputModel)-1);
        testCfg.endpoint = inputEndpoint;
        testCfg.apiKey = inputApiKey;
        testCfg.model = inputModel;
        auto reply = llmChat(testCfg, {{"user", "Responde solo: OK"}});
        if (!reply.empty()) {
            g_llmConfig.endpoint = inputEndpoint;
            g_llmConfig.apiKey = inputApiKey;
            g_llmConfig.model = inputModel;
            g_showLlmConfig = false;
        }
    }

    if (drawButton(btnClose, "CERRAR", alpha(RED, 150), 13)) {
        g_showLlmConfig = false;
    }

    // Ayuda
    yy += 54;
    DrawText("Proveedores compatibles:", px + 24, yy, 10, {100,116,139,255});
    yy += 16;
    DrawText("OpenAI:    https://api.openai.com/v1/chat/completions", px + 24, yy, 9, {148,163,184,255});
    yy += 13;
    DrawText("OpenRouter: https://openrouter.ai/api/v1/chat/completions", px + 24, yy, 9, {148,163,184,255});
    yy += 13;
    DrawText("DeepSeek:  https://api.deepseek.com/v1/chat/completions", px + 24, yy, 9, {148,163,184,255});
    yy += 13;
    DrawText("Local:     http://localhost:11434/v1/chat/completions", px + 24, yy, 9, {148,163,184,255});

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

void drawChatPanel(const std::vector<Entity>& entities) {
    if (!g_showChat) return;

    Entity* target = nullptr;
    for (auto& e : entities) if (e.id == g_chatTargetId) { target = const_cast<Entity*>(&e); break; }
    if (!target) { closeChat(); return; }

    int pw = 440, ph = 420;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2 - 30;

    // Fondo
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    // Sombra
    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, alpha(target->color, 180));

    // Header con color del agente
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 46}, 0.06f, 6, alpha(target->color, 35));
    // Icono
    const char* icon = "🤖";
    if (target->type == EntityType::Human) icon = "👤";
    else if (target->type == EntityType::CodeBot) icon = "🤖";
    else if (target->type == EntityType::DataBot) icon = "📊";
    else if (target->type == EntityType::Orchestrator) icon = "🔮";
    DrawText(icon, px + 16, py + 12, 18, WHITE);
    DrawText(TextFormat("Chat con %s", target->name.c_str()), px + 42, py + 10, 14, WHITE);
    DrawText(target->role.c_str(), px + 42, py + 28, 10, {148,163,184,255});

    // Botón cerrar
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 6, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) {
        closeChat();
        return;
    }

    int yy = py + 54;

    // Area de mensajes
    DrawRectangle(px + 12, yy, pw - 24, ph - 140, alpha(BLACK, 100));
    DrawRectangleLines(px + 12, yy, pw - 24, ph - 140, alpha(WHITE, 15));

    int msgY = yy + 10;
    int maxMsg = ph - 160;
    int start = std::max(0, (int)g_chatHistory.size() - 20);

    // Mensaje de bienvenida si no hay historial
    if (g_chatHistory.empty()) {
        DrawText("Escribe un mensaje para hablarle al agente.", px + 24, msgY, 11, {100,116,139,255});
        DrawText("Enter para enviar | ESC para cerrar", px + 24, msgY + 18, 10, {80,90,105,255});
    }

    for (int i = start; i < (int)g_chatHistory.size(); i++) {
        if (msgY - yy > maxMsg) break;
        bool isAgent = (g_chatHistory[i].first == "assistant");
        Color bg = isAgent ? alpha(target->color, 25) : alpha({56,189,248}, 18);
        float bubbleX = isAgent ? px + 18 : px + 60;
        float bubbleW = pw - 80;
        // Medir texto
        int tw = MeasureText(g_chatHistory[i].second.c_str(), 10);
        float actualW = std::min((float)tw + 20, bubbleW);
        if (!isAgent) bubbleX = px + pw - actualW - 30;

        DrawRectangleRounded({bubbleX, (float)msgY, actualW, 24}, 0.2f, 4, bg);
        DrawRectangleRoundedLines({bubbleX, (float)msgY, actualW, 24}, 0.2f, 4, alpha(bg, 200));

        // Texto truncado
        std::string text = g_chatHistory[i].second;
        if ((int)text.size() > 80) text = text.substr(0, 77) + "...";
        DrawText(text.c_str(), bubbleX + 10, msgY + 6, 10,
                 isAgent ? WHITE : (Color){148,163,184,255});
        msgY += 28;
    }

    // Input box mejorado
    int iy = py + ph - 70;
    DrawRectangle(px + 12, iy, pw - 24, 32, alpha(WHITE, 8));
    DrawRectangleLines(px + 12, iy, pw - 24, 32, alpha(target->color, 60));
    DrawText(g_chatInput.c_str(), px + 20, iy + 8, 11, WHITE);
    // Cursor parpadeante
    if ((int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(g_chatInput.c_str(), 11);
        DrawText("|", px + 20 + tw + 1, iy + 8, 11, WHITE);
    }
    // Placeholder
    if (g_chatInput.empty()) {
        DrawText("Escribe aqui...", px + 20, iy + 8, 11, {80,90,105,255});
    }
    // Hint
    DrawText("Enter: enviar | ESC: cerrar", px + 12, iy + 38, 9, {80,90,105,255});
}

void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused) {
    // ===== TOP BAR con degradado =====
    for (int x = 0; x < screenW && x < 1280; x++) {
        float t = (float)x / screenW;
        Color c = {
            (unsigned char)(15 + t * 8),
            (unsigned char)(23 + t * 5),
            (unsigned char)(42 + t * 3),
            230
        };
        DrawLine(x, 0, x, 50, c);
    }
    DrawLineEx({0, 50}, {(float)screenW, 50}, 2, {56,189,248,40});

    // Logo / titulo
    DrawText("META-OFFICE", 20, 12, 20, WHITE);
    DrawText("2D", 20 + MeasureText("META-OFFICE", 20) + 6, 16, 12, {56,189,248,255});
    DrawText("Oficina Virtual C++", 180, 20, 11, {148,163,184,255});

    // Estado LLM indicador
    bool llmOk = !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...";
    float llmPulse = sinf(GetTime() * 2) * 0.3f + 0.7f;
    DrawCircle(380, 22, 5, llmOk ? alpha({16,185,129}, (int)(255 * llmPulse)) : ORANGE);
    DrawText(llmOk ? "LLM Conectado" : "LLM sin config", 392, 16, 11,
             llmOk ? (Color){16,185,129,255} : ORANGE);

    if (!entities.empty()) {
        auto& h = entities[0];
        DrawText(TextFormat("Pos: (%.0f, %.0f)", h.x, h.y), 520, 18, 12, {148,163,184,255});
    }

    // Estado de simulacion
    DrawCircle(screenW - 260, 16, 5, paused ? ORANGE : GREEN);
    DrawText(paused ? "PAUSADO" : "EN TIEMPO REAL", screenW - 248, 12, 12,
             paused ? ORANGE : GREEN);

    // Botón Config LLM (engranaje)
    Rectangle btnConfig = {(float)screenW - 170, 8, 40, 34};
    bool configHover = CheckCollisionPointRec(GetMousePosition(), btnConfig);
    DrawRectangleRounded(btnConfig, 0.2f, 4, g_showLlmConfig ? alpha({56,189,248}, 60) : alpha(WHITE, configHover ? 15 : 8));
    DrawRectangleRoundedLines(btnConfig, 0.2f, 4, g_showLlmConfig ? (Color){56,189,248,200} : alpha(WHITE, 30));
    DrawText("CFG", btnConfig.x + 6, btnConfig.y + 9, 12, WHITE);
    if (configHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        toggleLlmConfig();
    }

    // Botón Pausar
    Rectangle btnPause = {(float)screenW - 120, 8, 110, 34};
    drawButton(btnPause, paused ? "REANUDAR" : "PAUSAR",
               paused ? (Color){245,158,11,80} : alpha(WHITE, 12), 13);

    // ===== SIDEBAR =====
    int sx = screenW - 300;
    // Fondo sidebar
    for (int y = 50; y < screenH; y++) {
        float t = (float)(y - 50) / (screenH - 50);
        Color c = {
            (unsigned char)(10 + t * 3),
            (unsigned char)(14 + t * 2),
            (unsigned char)(23 + t * 2),
            210
        };
        DrawLine(sx, y, screenW, y, c);
    }
    DrawLineEx({(float)sx, 50}, {(float)sx, (float)screenH}, 2, {56,189,248,30});

    // Header sidebar
    DrawText("MIEMBROS", sx + 16, 60, 14, {148,163,184,255});
    DrawText(TextFormat("(%zu)", entities.size()), sx + 106, 60, 14, {100,116,139,255});
    DrawLineEx({(float)sx + 12, 78}, {(float)screenW - 12, 78}, 1, alpha(WHITE, 10));

    int yy = 86;
    for (auto& e : entities) {
        bool sel = (e.id == selectedId);
        bool hover = CheckCollisionPointRec(GetMousePosition(), {(float)sx + 10, (float)yy, 280, 62});
        Rectangle card = {(float)sx + 10, (float)yy, 280, 62};

        // Sombra tarjeta
        DrawRectangleRounded({card.x + 2, card.y + 2, card.width, card.height}, 0.08f, 4, alpha(BLACK, 50));
        DrawRectangleRounded(card, 0.08f, 4, sel ? alpha(e.color, 35) : alpha(WHITE, hover ? 10 : 4));
        DrawRectangleRoundedLines(card, 0.08f, 4, sel ? e.color : alpha(WHITE, hover ? 25 : 12));

        const char* icon = "?";
        if (e.type == EntityType::Human) icon = "USR";
        else if (e.type == EntityType::CodeBot) icon = "BOT";
        else if (e.type == EntityType::DataBot) icon = "DAT";
        else if (e.type == EntityType::Orchestrator) icon = "ORQ";

        // Icono con fondo de color
        DrawRectangleRounded({(float)sx + 18, (float)yy + 10, 36, 36}, 0.15f, 4, e.color);
        DrawText(icon, sx + 22, yy + 18, 10, WHITE);

        // Nombre y rol
        DrawText(e.name.c_str(), sx + 62, yy + 13, 13, WHITE);
        DrawText(e.role.c_str(), sx + 62, yy + 30, 9, {148,163,184,255});

        // Stats
        if (e.testCount > 0)
            DrawText(TextFormat("Tests: %d", e.testCount), sx + 62, yy + 44, 9, {56,189,248,180});
        if (e.queryCount > 0)
            DrawText(TextFormat("Queries: %d", e.queryCount), sx + 130, yy + 44, 9, {16,185,129,180});

        // Estado
        const char* st = "IDLE";
        Color sc = {148,163,184,255};
        if (e.status == Status::Walking) { st = "ACTIVO"; sc = {56,189,248,255}; }
        else if (e.status == Status::Error) { st = "ERROR"; sc = RED; }
        else if (e.status == Status::Busy) { st = "OCUPADO"; sc = ORANGE; }
        else if (e.status == Status::Intervening) { st = "AYUDANDO"; sc = {168,85,247,255}; }

        // Indicador de estado (punto)
        DrawCircle(sx + 216, yy + 18, 4, sc);
        DrawText(st, sx + 226, yy + 14, 10, sc);

        // Indicador "disponible"
        if (e.hasGreetedHuman && e.type != EntityType::Human) {
            float bounce = sinf(GetTime() * 4) * 2;
            DrawCircle(sx + 260, yy + 18 + bounce, 4, alpha(e.color, 200));
            DrawText(">", sx + 252, yy + 13 + bounce, 10, WHITE);
        }

        yy += 68;
    }

    // Seccion actividad
    yy += 6;
    DrawLineEx({(float)sx + 12, (float)yy}, {(float)screenW - 12, (float)yy}, 1, alpha(WHITE, 10));
    yy += 10;
    DrawText("ACTIVIDAD", sx + 16, yy, 14, {148,163,184,255});
    DrawLineEx({(float)sx + 12, (float)yy + 20}, {(float)screenW - 12, (float)yy + 20}, 1, alpha(WHITE, 10));
    yy += 26;

    int start = std::max(0, (int)logs.size() - 9);
    for (int i = start; i < (int)logs.size(); i++) {
        // Punto de color
        DrawCircle(sx + 18, yy + 5, 2, logs[i].color);
        DrawText(logs[i].text.c_str(), sx + 26, yy, 9, logs[i].color);
        yy += 14;
    }

    // ===== HUD inferior =====
    // Fondo degradado del HUD
    for (int x = 0; x < 540; x++) {
        float t = 1.0f - (float)x / 540;
        Color c = {0, 0, 0, (unsigned char)(160 * t)};
        DrawLine(10 + x, screenH - 40, 10 + x, screenH - 10, c);
    }
    DrawText("WASD: mover | Clic agente: charlar | Clic mapa: mover | P: pausar | CFG: config LLM",
             18, screenH - 33, 12, {148,163,184,255});

    // ===== LLM Config Panel (overlay) =====
    drawLlmConfigPanel();

    // ===== Chat Panel =====
    drawChatPanel(entities);
}

void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY) {
    // Si el chat está abierto, el input va al chat
    if (g_showChat) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && g_chatInput.size() < 200)
                g_chatInput += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !g_chatInput.empty())
            g_chatInput.pop_back();
        if (IsKeyPressed(KEY_ENTER) && !g_chatInput.empty()) {
            // Enviar mensaje al LLM
            Entity* target = nullptr;
            for (auto& e : entities) if (e.id == g_chatTargetId) { target = &e; break; }
            if (target && !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...") {
                g_chatHistory.push_back({"user", g_chatInput});
                g_chatHistory.push_back({"assistant", "🧠 pensando..."});
                // Encolar con historial completo como contexto
                std::string fullPrompt = g_chatInput;
                std::lock_guard<std::mutex> lock(g_llmMutex);
                g_llmQueue.push({g_chatTargetId, getSystemPrompt(target->type), fullPrompt, GetTime() + 15.0});
            }
            g_chatInput.clear();
        }
        if (IsKeyPressed(KEY_ESCAPE)) closeChat();
        return;
    }

    // No procesar input normal si el panel de config está abierto
    if (g_showLlmConfig) {
        // Click en campos de texto para activar edición
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 540;
            int px = (screenW - pw) / 2;
            int py = (screenH - 400) / 2;
            int labelW = 130;
            int yy = py + 68;
            int fieldW = pw - 150;

            // Endpoint field
            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 1; inputActive = true;
                strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
            }
            yy += 38;
            // API Key field
            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 2; inputActive = true;
                strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
            }
            yy += 38;
            // Model field
            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 3; inputActive = true;
                strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1);
            }
        }
        return;
    }

    if (IsKeyPressed(KEY_P)) paused = !paused;

    // Botón CFG en top bar
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

        // Botón pausar en top bar
        Rectangle btnPause = {(float)screenW - 120, 8, 110, 34};
        if (CheckCollisionPointRec(mp, btnPause)) {
            paused = !paused;
            return;
        }

        if (mp.x < screenW - 300) {
            Vector2 grid = isoToGrid(mp.x, mp.y, origin);
            int gx = roundf(grid.x), gy = roundf(grid.y);
            if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                bool clickedAgent = false;
                for (auto& e : entities) {
                    if (e.type == EntityType::Human) continue;
                    float d = sqrtf(powf(e.renderX - gx, 2) + powf(e.renderY - gy, 2));
                    if (d < 1.5f) { selectedId = e.id; openChat(e.id); clickedAgent = true; break; }
                }
                if (!clickedAgent && human) {
                    human->targetX = gx;
                    human->targetY = gy;
                    selectedId = -1;
                }
            }
        }

        // Click en sidebar
        int sxx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sxx + 10, (float)(86 + i * 68), 280, 62};
            if (CheckCollisionPointRec(mp, card)) { selectedId = entities[i].id; openChat(entities[i].id); break; }
        }
    }

    float wheel = GetMouseWheelMove();
    panY += wheel * 5;
    origin.y += wheel * 5;
}