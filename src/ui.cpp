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
        strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
        strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
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
                  active ? alpha({56,189,248,255}, 30) : alpha(WHITE, 8));
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

    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 180));

    int pw = 540, ph = 400;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2;

    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {56,189,248,120});

    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha({56,189,248,255}, 20));
    DrawText("CONFIGURACION LLM", px + 24, py + 14, 18, WHITE);
    DrawText("Protocolo compatible con OpenAI API", px + 24, py + 36, 11, {148,163,184,255});

    int yy = py + 68;
    int fieldW = pw - 150;

    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "Endpoint:", inputEndpoint, editField == 1);
    yy += 38;

    std::string display = inputApiKey;
    if (display.length() > 8) {
        display = display.substr(0, 4) + "...." + display.substr(display.length()-4);
    }
    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "API Key:", display.c_str(), editField == 2);
    yy += 38;

    drawTextField({(float)px + 130, (float)yy, (float)fieldW, 26}, "Modelo:", inputModel, editField == 3);
    yy += 56;

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

    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, alpha(target->color, 180));

    // Header
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 46}, 0.06f, 6, alpha(target->color, 35));
    const char* icon = "BOT";
    if (target->type == EntityType::Human) icon = "USR";
    else if (target->type == EntityType::DataBot) icon = "DAT";
    else if (target->type == EntityType::Orchestrator) icon = "ORQ";
    DrawText(icon, px + 16, py + 14, 14, WHITE);
    DrawText(TextFormat("Chat con %s", target->name.c_str()), px + 52, py + 10, 14, WHITE);
    DrawText(target->role.c_str(), px + 52, py + 28, 10, {148,163,184,255});

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

    if (g_chatHistory.empty()) {
        DrawText("Escribe un mensaje para hablarle al agente.", px + 24, msgY, 11, {100,116,139,255});
        DrawText("Enter para enviar | ESC para cerrar", px + 24, msgY + 18, 10, {80,90,105,255});
    }

    for (int i = start; i < (int)g_chatHistory.size(); i++) {
        if (msgY - yy > maxMsg) break;
        bool isAgent = (g_chatHistory[i].first == "assistant");
        Color bg = isAgent ? alpha(target->color, 25) : alpha({56,189,248,255}, 18);
        float bubbleX = isAgent ? px + 18 : px + 60;
        float bubbleW = pw - 80;
        int tw = MeasureText(g_chatHistory[i].second.c_str(), 10);
        float actualW = std::min((float)tw + 20, bubbleW);
        if (!isAgent) bubbleX = px + pw - actualW - 30;

        DrawRectangleRounded({bubbleX, (float)msgY, actualW, 24}, 0.2f, 4, bg);
        DrawRectangleRoundedLines({bubbleX, (float)msgY, actualW, 24}, 0.2f, 4, alpha(bg, 200));

        std::string text = g_chatHistory[i].second;
        if ((int)text.size() > 80) text = text.substr(0, 77) + "...";
        DrawText(text.c_str(), bubbleX + 10, msgY + 6, 10,
                 isAgent ? WHITE : (Color){148,163,184,255});
        msgY += 28;
    }

    // Input box
    int iy = py + ph - 70;
    DrawRectangle(px + 12, iy, pw - 24, 32, alpha(WHITE, 8));
    DrawRectangleLines(px + 12, iy, pw - 24, 32, alpha(target->color, 60));
    DrawText(g_chatInput.c_str(), px + 20, iy + 8, 11, WHITE);
    if ((int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(g_chatInput.c_str(), 11);
        DrawText("|", px + 20 + tw + 1, iy + 8, 11, WHITE);
    }
    if (g_chatInput.empty()) {
        DrawText("Escribe aqui...", px + 20, iy + 8, 11, {80,90,105,255});
    }
    DrawText("Enter: enviar | ESC: cerrar", px + 12, iy + 38, 9, {80,90,105,255});
}

// ============================================================
// Log Panel — ver el log individual de un agente
// ============================================================
void drawLogPanel(const std::vector<Entity>& entities) {
    if (!g_showLog) return;

    Entity* target = nullptr;
    for (auto& e : entities) if (e.id == g_logTargetId) { target = const_cast<Entity*>(&e); break; }
    if (!target) { closeLogPanel(); return; }

    int pw = 460, ph = 440;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2 - 20;

    // Fondo
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, alpha(target->color, 180));

    // Header
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha(target->color, 35));
    DrawText("LOG", px + 16, py + 10, 18, WHITE);
    DrawText(target->name.c_str(), px + 60, py + 10, 14, WHITE);
    DrawText(target->role.c_str(), px + 60, py + 28, 10, {148,163,184,255});

    // Stats rápidas
    DrawText(TextFormat("Tests: %d", target->testCount), px + pw - 140, py + 12, 11, {56,189,248,255});
    DrawText(TextFormat("Queries: %d", target->queryCount), px + pw - 140, py + 28, 11, {16,185,129,255});

    // Botón cerrar
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 8, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) {
        closeLogPanel();
        return;
    }

    // Area de log
    int logY = py + 58;
    int logH = ph - 70;
    DrawRectangle(px + 12, logY, pw - 24, logH, alpha(BLACK, 100));
    DrawRectangleLines(px + 12, logY, pw - 24, logH, alpha(WHITE, 15));

    if (target->agentLog.empty()) {
        DrawText("Sin actividad registrada.", px + 24, logY + 14, 11, {100,116,139,255});
        DrawText("Este agente aun no ha trabajado.", px + 24, logY + 32, 10, {80,90,105,255});
    }

    int entryY = logY + 10;
    int maxEntries = logH - 20;
    int startIdx = std::max(0, (int)target->agentLog.size() - 25);

    for (int i = startIdx; i < (int)target->agentLog.size(); i++) {
        if (entryY - logY > maxEntries) break;
        auto& entry = target->agentLog[i];

        // Fila con fondo alterno
        bool altRow = (i % 2 == 0);
        DrawRectangle(px + 14, entryY - 2, pw - 28, 14, altRow ? alpha(WHITE, 4) : alpha(BLACK, 0));

        // Bullet de color
        DrawCircle(px + 20, entryY + 5, 2, entry.color);

        // Texto truncado
        std::string text = entry.text;
        if ((int)text.size() > 55) text = text.substr(0, 52) + "...";
        DrawText(text.c_str(), px + 28, entryY, 9, entry.color);

        entryY += 15;
    }

    // Footer
    DrawText(TextFormat("%zu entradas", target->agentLog.size()), px + 16, py + ph - 18, 10, {100,116,139,255});
    DrawText("ESC: cerrar", px + pw - 90, py + ph - 18, 10, {100,116,139,255});
}

// ============================================================
// Task Panel — crear y ver tareas
// ============================================================
static char taskInput[256] = "";
static bool taskInputActive = false;
static int taskAssignTarget = -1; // ID del agente a asignar

static const char* taskStatusStr(TaskStatus s) {
    switch (s) {
        case TaskStatus::Pending: return "PENDIENTE";
        case TaskStatus::InProgress: return "EN PROGRESO";
        case TaskStatus::Done: return "COMPLETADA";
        case TaskStatus::Failed: return "FALLIDA";
    }
    return "?";
}

static Color taskStatusColor(TaskStatus s) {
    switch (s) {
        case TaskStatus::Pending: return {148,163,184,255};
        case TaskStatus::InProgress: return {56,189,248,255};
        case TaskStatus::Done: return {16,185,129,255};
        case TaskStatus::Failed: return {244,63,94,255};
    }
    return WHITE;
}

void drawTaskPanel(const std::vector<Entity>& entities) {
    if (!g_showTaskPanel) return;

    int pw = 500, ph = 480;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2;

    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px+4, (float)py+4, (float)pw, (float)ph}, 0.06f, 6, alpha(BLACK, 100));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {245,158,11,180});

    // Header
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha({245,158,11,255}, 25));
    DrawText("TAREAS", px + 20, py + 12, 18, WHITE);
    DrawText(TextFormat("(%zu total)", g_tasks.size()), px + 120, py + 16, 12, {148,163,184,255});

    // Botón cerrar
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 8, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) {
        closeTaskPanel();
        return;
    }

    // Sección crear tarea
    int yy = py + 60;
    DrawText("NUEVA TAREA", px + 20, yy, 13, {245,158,11,255});
    yy += 22;

    // Input de descripción
    DrawRectangle(px + 20, yy, pw - 40, 30, taskInputActive ? alpha({245,158,11,255}, 20) : alpha(WHITE, 8));
    DrawRectangleLines(px + 20, yy, pw - 40, 30, taskInputActive ? (Color){245,158,11,180} : alpha(WHITE, 20));
    DrawText(taskInput, px + 28, yy + 8, 12, WHITE);
    if (taskInputActive && (int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(taskInput, 12);
        DrawText("|", px + 28 + tw + 1, yy + 8, 12, WHITE);
    }
    if (!taskInputActive && strlen(taskInput) == 0)
        DrawText("Ej: Investiga precios de VPS en Chile...", px + 28, yy + 8, 11, {80,90,105,255});
    if (CheckCollisionPointRec(GetMousePosition(), {(float)px + 20, (float)yy, (float)(pw - 40), 30}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        taskInputActive = true;
    else if (!CheckCollisionPointRec(GetMousePosition(), {(float)px + 20, (float)yy, (float)(pw - 40), 30}) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        taskInputActive = false;
    yy += 40;

    // Selector de agente
    DrawText("Asignar a:", px + 20, yy + 4, 12, {148,163,184,255});
    int btnX = px + 100;
    for (int i = 0; i < (int)entities.size(); i++) {
        if (entities[i].type == EntityType::Human) continue;
        Rectangle btn = {(float)btnX, (float)yy, 80, 28};
        bool sel = (taskAssignTarget == entities[i].id);
        bool hover = CheckCollisionPointRec(GetMousePosition(), btn);
        DrawRectangleRounded(btn, 0.15f, 4, sel ? alpha(entities[i].color, 80) : alpha(WHITE, hover ? 12 : 5));
        DrawRectangleRoundedLines(btn, 0.15f, 4, sel ? entities[i].color : alpha(WHITE, 20));
        const char* shortName = entities[i].name.c_str();
        if (strlen(shortName) > 8) {
            // Usar nombre corto
            if (entities[i].type == EntityType::CodeBot) shortName = "CodeBot";
            else if (entities[i].type == EntityType::DataBot) shortName = "DataBot";
            else if (entities[i].type == EntityType::Orchestrator) shortName = "Orch.";
        }
        DrawText(shortName, btn.x + 10, btn.y + 7, 10, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
            taskAssignTarget = entities[i].id;
        btnX += 88;
    }
    yy += 40;

    // Botón crear
    Rectangle btnCreate = {(float)px + 20, (float)yy, 160, 34};
    if (drawButton(btnCreate, "+ CREAR TAREA", {245,158,11,200}, 13)) {
        if (strlen(taskInput) > 3 && taskAssignTarget >= 0) {
            Task t;
            t.id = g_nextTaskId++;
            t.description = taskInput;
            t.assignedTo = taskAssignTarget;
            t.status = TaskStatus::Pending;
            t.createdAt = GetTime();
            g_tasks.push_back(t);
            taskInput[0] = '\0';
            taskInputActive = false;
        }
    }
    yy += 50;

    // Lista de tareas
    DrawText("TAREAS ACTIVAS", px + 20, yy, 13, {148,163,184,255});
    yy += 22;

    int listH = py + ph - yy - 16;
    DrawRectangle(px + 12, yy, pw - 24, listH, alpha(BLACK, 100));
    DrawRectangleLines(px + 12, yy, pw - 24, listH, alpha(WHITE, 15));

    int entryY = yy + 8;
    int maxEntries = listH - 16;
    int startIdx = std::max(0, (int)g_tasks.size() - 12);

    for (int i = startIdx; i < (int)g_tasks.size(); i++) {
        if (entryY - yy > maxEntries) break;
        auto& t = g_tasks[i];

        // Fila
        DrawRectangle(px + 14, entryY - 2, pw - 28, 30, alpha(WHITE, 4));

        // Estado (punto de color)
        Color sc = taskStatusColor(t.status);
        DrawCircle(px + 22, entryY + 8, 4, sc);
        DrawText(taskStatusStr(t.status), px + 32, entryY + 2, 9, sc);

        // ID
        DrawText(TextFormat("#%d", t.id), px + 130, entryY + 2, 9, {100,116,139,255});

        // Descripción (truncada)
        std::string desc = t.description;
        if ((int)desc.size() > 45) desc = desc.substr(0, 42) + "...";
        DrawText(desc.c_str(), px + 165, entryY + 2, 10, WHITE);

        // Agente asignado
        const char* agentName = "?";
        for (auto& e : entities) if (e.id == t.assignedTo) agentName = e.name.c_str();
        DrawText(agentName, px + 380, entryY + 2, 9, {148,163,184,255});

        // Resultado si está completada
        if (t.status == TaskStatus::Done || t.status == TaskStatus::Failed) {
            std::string res = t.result;
            if ((int)res.size() > 60) res = res.substr(0, 57) + "...";
            DrawText(res.c_str(), px + 32, entryY + 16, 9, sc);
        }

        // Pasos si en progreso
        if (t.status == TaskStatus::InProgress && !t.steps.empty()) {
            std::string lastStep = t.steps.back();
            if ((int)lastStep.size() > 50) lastStep = lastStep.substr(0, 47) + "...";
            DrawText(lastStep.c_str(), px + 32, entryY + 16, 9, {56,189,248,180});
        }

        entryY += 34;
    }

    if (g_tasks.empty()) {
        DrawText("No hay tareas. Crea una arriba.", px + 28, yy + 20, 11, {100,116,139,255});
    }

    // Input handling
    if (taskInputActive) {
        int key = GetCharPressed();
        while (key > 0) {
            size_t len = strlen(taskInput);
            if (len < sizeof(taskInput) - 1 && key >= 32 && key <= 126) {
                taskInput[len] = (char)key;
                taskInput[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            size_t len = strlen(taskInput);
            if (len > 0) taskInput[len-1] = '\0';
        }
    }
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

    DrawText("META-OFFICE", 20, 12, 20, WHITE);
    DrawText("2D", 20 + MeasureText("META-OFFICE", 20) + 6, 16, 12, {56,189,248,255});
    DrawText("Oficina Virtual C++", 180, 20, 11, {148,163,184,255});

    bool llmOk = !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...";
    float llmPulse = sinf(GetTime() * 2) * 0.3f + 0.7f;
    DrawCircle(380, 22, 5, llmOk ? alpha({16,185,129,255}, (int)(255 * llmPulse)) : ORANGE);
    DrawText(llmOk ? "LLM Conectado" : "LLM sin config", 392, 16, 11,
             llmOk ? (Color){16,185,129,255} : ORANGE);

    if (!entities.empty()) {
        auto& h = entities[0];
        DrawText(TextFormat("Pos: (%.0f, %.0f)", h.x, h.y), 520, 18, 12, {148,163,184,255});
    }

    DrawCircle(screenW - 260, 16, 5, paused ? ORANGE : GREEN);
    DrawText(paused ? "PAUSADO" : "EN TIEMPO REAL", screenW - 248, 12, 12,
             paused ? ORANGE : GREEN);

    Rectangle btnTasks = {(float)screenW - 300, 8, 80, 34};
    bool tasksHover = CheckCollisionPointRec(GetMousePosition(), btnTasks);
    int activeTasks = 0;
    for (auto& t : g_tasks) if (t.status == TaskStatus::Pending || t.status == TaskStatus::InProgress) activeTasks++;
    DrawRectangleRounded(btnTasks, 0.2f, 4, g_showTaskPanel ? alpha({245,158,11,255}, 60) : alpha(WHITE, tasksHover ? 15 : 8));
    DrawRectangleRoundedLines(btnTasks, 0.2f, 4, g_showTaskPanel ? (Color){245,158,11,200} : alpha(WHITE, 30));
    DrawText("TAREAS", btnTasks.x + 8, btnTasks.y + 9, 12, WHITE);
    if (activeTasks > 0)
        DrawText(TextFormat("(%d)", activeTasks), btnTasks.x + 58, btnTasks.y + 4, 10, {245,158,11,255});
    if (tasksHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        g_showTaskPanel = !g_showTaskPanel;

    Rectangle btnConfig = {(float)screenW - 204, 8, 40, 34};
    bool configHover = CheckCollisionPointRec(GetMousePosition(), btnConfig);
    DrawRectangleRounded(btnConfig, 0.2f, 4, g_showLlmConfig ? alpha({56,189,248,255}, 60) : alpha(WHITE, configHover ? 15 : 8));
    DrawRectangleRoundedLines(btnConfig, 0.2f, 4, g_showLlmConfig ? (Color){56,189,248,200} : alpha(WHITE, 30));
    DrawText("CFG", btnConfig.x + 6, btnConfig.y + 9, 12, WHITE);
    if (configHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        toggleLlmConfig();
    }

    Rectangle btnPause = {(float)screenW - 120, 8, 110, 34};
    drawButton(btnPause, paused ? "REANUDAR" : "PAUSAR",
               paused ? (Color){245,158,11,80} : alpha(WHITE, 12), 13);

    // ===== SIDEBAR =====
    int sx = screenW - 300;
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

    DrawText("MIEMBROS", sx + 16, 60, 14, {148,163,184,255});
    DrawText(TextFormat("(%zu)", entities.size()), sx + 106, 60, 14, {100,116,139,255});
    DrawLineEx({(float)sx + 12, 78}, {(float)screenW - 12, 78}, 1, alpha(WHITE, 10));

    int yy = 86;
    for (auto& e : entities) {
        bool sel = (e.id == selectedId);
        bool hover = CheckCollisionPointRec(GetMousePosition(), {(float)sx + 10, (float)yy, 280, 62});
        Rectangle card = {(float)sx + 10, (float)yy, 280, 62};

        DrawRectangleRounded({card.x + 2, card.y + 2, card.width, card.height}, 0.08f, 4, alpha(BLACK, 50));
        DrawRectangleRounded(card, 0.08f, 4, sel ? alpha(e.color, 35) : alpha(WHITE, hover ? 10 : 4));
        DrawRectangleRoundedLines(card, 0.08f, 4, sel ? e.color : alpha(WHITE, hover ? 25 : 12));

        const char* icon = "?";
        if (e.type == EntityType::Human) icon = "USR";
        else if (e.type == EntityType::CodeBot) icon = "BOT";
        else if (e.type == EntityType::DataBot) icon = "DAT";
        else if (e.type == EntityType::Orchestrator) icon = "ORQ";

        DrawRectangleRounded({(float)sx + 18, (float)yy + 10, 36, 36}, 0.15f, 4, e.color);
        DrawText(icon, sx + 22, yy + 18, 10, WHITE);

        DrawText(e.name.c_str(), sx + 62, yy + 13, 13, WHITE);
        DrawText(e.role.c_str(), sx + 62, yy + 30, 9, {148,163,184,255});

        if (e.testCount > 0)
            DrawText(TextFormat("Tests: %d", e.testCount), sx + 62, yy + 44, 9, {56,189,248,180});
        if (e.queryCount > 0)
            DrawText(TextFormat("Queries: %d", e.queryCount), sx + 130, yy + 44, 9, {16,185,129,180});

        const char* st = "IDLE";
        Color sc = {148,163,184,255};
        if (e.status == Status::Walking) { st = "ACTIVO"; sc = {56,189,248,255}; }
        else if (e.status == Status::Error) { st = "ERROR"; sc = RED; }
        else if (e.status == Status::Busy) { st = "OCUPADO"; sc = ORANGE; }
        else if (e.status == Status::Intervening) { st = "AYUDANDO"; sc = {168,85,247,255}; }

        DrawCircle(sx + 216, yy + 18, 4, sc);
        DrawText(st, sx + 226, yy + 14, 10, sc);

        // Indicador de entradas en log
        if (!e.agentLog.empty() && e.type != EntityType::Human) {
            DrawText(TextFormat("[%zu]", e.agentLog.size()), sx + 260, yy + 14, 9, alpha(e.color, 180));
        }

        if (e.hasGreetedHuman && e.type != EntityType::Human) {
            float bounce = sinf(GetTime() * 4) * 2;
            DrawCircle(sx + 260, yy + 38 + bounce, 3, alpha(e.color, 200));
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
        DrawCircle(sx + 18, yy + 5, 2, logs[i].color);
        DrawText(logs[i].text.c_str(), sx + 26, yy, 9, logs[i].color);
        yy += 14;
    }

    // ===== HUD inferior =====
    for (int x = 0; x < 620; x++) {
        float t = 1.0f - (float)x / 620;
        Color c = {0, 0, 0, (unsigned char)(160 * t)};
        DrawLine(10 + x, screenH - 40, 10 + x, screenH - 10, c);
    }
    DrawText("Clic izq: charlar | Clic der: log | WASD: mover | P: pausar | CFG: config",
             18, screenH - 33, 12, {148,163,184,255});

    // ===== Overlays =====
    drawLlmConfigPanel();
    drawChatPanel(entities);
    drawLogPanel(entities);
    drawTaskPanel(entities);
}

void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY) {
    // Si el log panel está abierto, solo ESC para cerrar
    // Si el task panel esta abierto
    if (g_showTaskPanel) {
        if (IsKeyPressed(KEY_ESCAPE)) closeTaskPanel();
        return;
    }

    if (g_showLog) {
        if (IsKeyPressed(KEY_ESCAPE)) closeLogPanel();
        // Click en botón cerrar del log panel se maneja en drawLogPanel
        // Pero necesitamos capturar clicks aquí para que no pasen al mapa
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            // Verificar si el click fue fuera del panel
            int pw = 460, ph = 440;
            int px = (screenW - pw) / 2;
            int py = (screenH - ph) / 2 - 20;
            Vector2 mp = GetMousePosition();
            if (!CheckCollisionPointRec(mp, {(float)px, (float)py, (float)pw, (float)ph})) {
                closeLogPanel();
            }
        }
        return;
    }

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
            Entity* target = nullptr;
            for (auto& e : entities) if (e.id == g_chatTargetId) { target = &e; break; }
            if (target && !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...") {
                g_chatHistory.push_back({"user", g_chatInput});
                g_chatHistory.push_back({"assistant", "🧠 pensando..."});
                std::string fullPrompt = g_chatInput;
                std::lock_guard<std::mutex> lock(g_llmMutex);
                g_llmQueue.push({g_chatTargetId, getSystemPrompt(target->type), fullPrompt, GetTime() + 15.0});
            }
            g_chatInput.clear();
        }
        if (IsKeyPressed(KEY_ESCAPE)) closeChat();
        return;
    }

    if (g_showLlmConfig) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 540;
            int px = (screenW - pw) / 2;
            int py = (screenH - 400) / 2;
            int labelW = 130;
            int yy = py + 68;
            int fieldW = pw - 150;

            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 1; inputActive = true;
                strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
            }
            yy += 38;
            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 2; inputActive = true;
                strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
            }
            yy += 38;
            if (mp.x > px + labelW && mp.x < px + labelW + fieldW &&
                mp.y > yy && mp.y < yy + 26) {
                editField = 3; inputActive = true;
                strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1);
            }
        }
        return;
    }

    if (IsKeyPressed(KEY_P)) paused = !paused;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();

        Rectangle btnTasks = {(float)screenW - 300, 8, 80, 34};
    bool tasksHover = CheckCollisionPointRec(GetMousePosition(), btnTasks);
    int activeTasks = 0;
    for (auto& t : g_tasks) if (t.status == TaskStatus::Pending || t.status == TaskStatus::InProgress) activeTasks++;
    DrawRectangleRounded(btnTasks, 0.2f, 4, g_showTaskPanel ? alpha({245,158,11,255}, 60) : alpha(WHITE, tasksHover ? 15 : 8));
    DrawRectangleRoundedLines(btnTasks, 0.2f, 4, g_showTaskPanel ? (Color){245,158,11,200} : alpha(WHITE, 30));
    DrawText("TAREAS", btnTasks.x + 8, btnTasks.y + 9, 12, WHITE);
    if (activeTasks > 0)
        DrawText(TextFormat("(%d)", activeTasks), btnTasks.x + 58, btnTasks.y + 4, 10, {245,158,11,255});
    if (tasksHover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        g_showTaskPanel = !g_showTaskPanel;

    Rectangle btnConfig = {(float)screenW - 204, 8, 40, 34};
        if (CheckCollisionPointRec(mp, btnConfig)) {
            toggleLlmConfig();
            return;
        }

        Rectangle btnPause = {(float)screenW - 120, 8, 110, 34};
        if (CheckCollisionPointRec(mp, btnPause)) {
            paused = !paused;
            return;
        }

        // Click en sidebar — clic izq abre chat, clic der abre log
        int sxx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sxx + 10, (float)(86 + i * 68), 280, 62};
            if (CheckCollisionPointRec(mp, card)) {
                selectedId = entities[i].id;
                openChat(entities[i].id);
                break;
            }
        }

        // Click en mapa
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
                if (!clickedAgent) {
                    auto* human = entities.empty() ? nullptr : &entities[0];
                    if (human) {
                        human->targetX = gx;
                        human->targetY = gy;
                        selectedId = -1;
                    }
                }
            }
        }
    }

    // Clic derecho → abrir log panel
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector2 mp = GetMousePosition();

        // Sidebar
        int sxx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sxx + 10, (float)(86 + i * 68), 280, 62};
            if (CheckCollisionPointRec(mp, card)) {
                openLogPanel(entities[i].id);
                return;
            }
        }

        // Mapa
        if (mp.x < screenW - 300) {
            Vector2 grid = isoToGrid(mp.x, mp.y, origin);
            int gx = roundf(grid.x), gy = roundf(grid.y);
            if (gx >= 0 && gx < GRID_W && gy >= 0 && gy < GRID_H) {
                for (auto& e : entities) {
                    if (e.type == EntityType::Human) continue;
                    float d = sqrtf(powf(e.renderX - gx, 2) + powf(e.renderY - gy, 2));
                    if (d < 1.5f) { openLogPanel(e.id); break; }
                }
            }
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

    float wheel = GetMouseWheelMove();
    panY += wheel * 5;
    origin.y += wheel * 5;
}