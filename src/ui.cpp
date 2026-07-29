#include "meta_office.hpp"
#include <cstring>

static char inputEndpoint[256] = "";
static char inputApiKey[256] = "";
static char inputModel[128] = "";
static int editField = 0;
static bool inputActive = false;
static char taskInput[256] = "";
static bool taskInputActive = false;
static int taskAssignTarget = -1;

void toggleLlmConfig() {
    g_showLlmConfig = !g_showLlmConfig;
    if (g_showLlmConfig) {
        strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1);
        strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1);
        if (strlen(inputApiKey) > 8)
            for (size_t i = 4; i < strlen(inputApiKey)-4; i++) inputApiKey[i] = '*';
        strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1);
        editField = 0;
        inputActive = false;
    }
}

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

// ============================================================
// LLM Config Panel
// ============================================================
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
    if (display.length() > 8) display = display.substr(0, 4) + "...." + display.substr(display.length()-4);
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
        testCfg.endpoint = inputEndpoint; testCfg.apiKey = inputApiKey; testCfg.model = inputModel;
        auto reply = llmChat(testCfg, {{"user", "Responde solo: OK"}});
        if (!reply.empty()) { g_llmConfig.endpoint = inputEndpoint; g_llmConfig.apiKey = inputApiKey; g_llmConfig.model = inputModel; g_showLlmConfig = false; }
    }
    if (drawButton(btnClose, "CERRAR", alpha(RED, 150), 13)) g_showLlmConfig = false;
    yy += 54;
    DrawText("Proveedores: OpenAI | OpenRouter | DeepSeek | Ollama local", px + 24, yy, 10, {100,116,139,255});

    if (inputActive && editField > 0) {
        int key = GetCharPressed();
        while (key > 0) {
            char* buf = nullptr; size_t max = 0;
            if (editField == 1) { buf = inputEndpoint; max = sizeof(inputEndpoint)-1; }
            else if (editField == 2) { buf = inputApiKey; max = sizeof(inputApiKey)-1; }
            else if (editField == 3) { buf = inputModel; max = sizeof(inputModel)-1; }
            if (buf) {
                size_t len = strlen(buf);
                if (len < max - 1 && key >= 32 && key <= 126) { buf[len] = (char)key; buf[len+1] = '\0'; }
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            char* buf = nullptr;
            if (editField == 1) buf = inputEndpoint;
            else if (editField == 2) buf = inputApiKey;
            else if (editField == 3) buf = inputModel;
            if (buf) { size_t len = strlen(buf); if (len > 0) buf[len-1] = '\0'; }
        }
        if (IsKeyPressed(KEY_ESCAPE)) { inputActive = false; editField = 0; g_showLlmConfig = false; }
    }
}

// ============================================================
// Chat Panel
// ============================================================
void drawChatPanel(const std::vector<Entity>& entities) {
    if (!g_showChat) return;
    Entity* target = nullptr;
    for (auto& e : entities) if (e.id == g_chatTargetId) { target = const_cast<Entity*>(&e); break; }
    if (!target) { closeChat(); return; }
    int pw = 440, ph = 420;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2 - 30;
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, alpha(target->color, 180));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 46}, 0.06f, 6, alpha(target->color, 35));
    DrawText(TextFormat("Chat con %s", target->name.c_str()), px + 16, py + 10, 14, WHITE);
    DrawText(target->role.c_str(), px + 16, py + 28, 10, {148,163,184,255});
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 6, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) { closeChat(); return; }
    int yy = py + 54;
    DrawRectangle(px + 12, yy, pw - 24, ph - 140, alpha(BLACK, 100));
    DrawRectangleLines(px + 12, yy, pw - 24, ph - 140, alpha(WHITE, 15));
    int msgY = yy + 10;
    int maxMsg = ph - 160;
    int start = std::max(0, (int)g_chatHistory.size() - 20);
    if (g_chatHistory.empty()) {
        DrawText("Escribe un mensaje para hablarle al agente.", px + 24, msgY, 11, {100,116,139,255});
        DrawText("Enter: enviar | ESC: cerrar", px + 24, msgY + 18, 10, {80,90,105,255});
    }
    for (int i = start; i < (int)g_chatHistory.size(); i++) {
        if (msgY - yy > maxMsg) break;
        bool isAgent = (g_chatHistory[i].first == "assistant");
        Color bg = isAgent ? alpha(target->color, 25) : alpha({56,189,248,255}, 18);
        int tw = MeasureText(g_chatHistory[i].second.c_str(), 10);
        float actualW = std::min((float)tw + 20, (float)(pw - 80));
        float bubbleX = isAgent ? px + 18 : px + pw - actualW - 30;
        DrawRectangleRounded({bubbleX, (float)msgY, actualW, 24}, 0.2f, 4, bg);
        std::string text = g_chatHistory[i].second;
        if ((int)text.size() > 80) text = text.substr(0, 77) + "...";
        DrawText(text.c_str(), bubbleX + 10, msgY + 6, 10, isAgent ? WHITE : (Color){148,163,184,255});
        msgY += 28;
    }
    int iy = py + ph - 70;
    DrawRectangle(px + 12, iy, pw - 24, 32, alpha(WHITE, 8));
    DrawRectangleLines(px + 12, iy, pw - 24, 32, alpha(target->color, 60));
    DrawText(g_chatInput.c_str(), px + 20, iy + 8, 11, WHITE);
    if ((int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(g_chatInput.c_str(), 11);
        DrawText("|", px + 20 + tw + 1, iy + 8, 11, WHITE);
    }
    if (g_chatInput.empty()) DrawText("Escribe aqui...", px + 20, iy + 8, 11, {80,90,105,255});
}

// ============================================================
// Log Panel
// ============================================================
void drawLogPanel(const std::vector<Entity>& entities) {
    if (!g_showLog) return;
    Entity* target = nullptr;
    for (auto& e : entities) if (e.id == g_logTargetId) { target = const_cast<Entity*>(&e); break; }
    if (!target) { closeLogPanel(); return; }
    int pw = 460, ph = 440;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2 - 20;
    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, alpha(target->color, 180));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha(target->color, 35));
    DrawText("LOG", px + 16, py + 10, 18, WHITE);
    DrawText(target->name.c_str(), px + 60, py + 10, 14, WHITE);
    DrawText(TextFormat("Tests:%d  Q:%d", target->testCount, target->queryCount), px + pw - 120, py + 16, 11, {148,163,184,255});
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 8, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) { closeLogPanel(); return; }
    int logY = py + 58, logH = ph - 70;
    DrawRectangle(px + 12, logY, pw - 24, logH, alpha(BLACK, 100));
    DrawRectangleLines(px + 12, logY, pw - 24, logH, alpha(WHITE, 15));
    if (target->agentLog.empty()) {
        DrawText("Sin actividad registrada.", px + 24, logY + 14, 11, {100,116,139,255});
    }
    int entryY = logY + 10;
    int startIdx = std::max(0, (int)target->agentLog.size() - 25);
    for (int i = startIdx; i < (int)target->agentLog.size(); i++) {
        if (entryY - logY > logH - 20) break;
        auto& entry = target->agentLog[i];
        DrawCircle(px + 20, entryY + 5, 2, entry.color);
        std::string text = entry.text;
        if ((int)text.size() > 55) text = text.substr(0, 52) + "...";
        DrawText(text.c_str(), px + 28, entryY, 9, entry.color);
        entryY += 15;
    }
    DrawText(TextFormat("%zu entradas | ESC: cerrar", target->agentLog.size()), px + 16, py + ph - 18, 10, {100,116,139,255});
}

// ============================================================
// Task Panel
// ============================================================
static const char* taskStatusStr(TaskStatus s) {
    switch (s) {
        case TaskStatus::Pending: return "PEND";
        case TaskStatus::InProgress: return "PROG";
        case TaskStatus::Done: return "DONE";
        case TaskStatus::Failed: return "FAIL";
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
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {245,158,11,180});
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha({245,158,11,255}, 25));
    DrawText("TAREAS", px + 20, py + 12, 18, WHITE);
    DrawText(TextFormat("(%zu)", g_tasks.size()), px + 120, py + 16, 12, {148,163,184,255});
    Rectangle btnClose = {(float)px + pw - 40, (float)py + 8, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) { closeTaskPanel(); return; }

    int yy = py + 60;
    DrawText("NUEVA TAREA", px + 20, yy, 13, {245,158,11,255});
    yy += 22;
    DrawRectangle(px + 20, yy, pw - 40, 30, taskInputActive ? alpha({245,158,11,255}, 20) : alpha(WHITE, 8));
    DrawRectangleLines(px + 20, yy, pw - 40, 30, taskInputActive ? (Color){245,158,11,180} : alpha(WHITE, 20));
    DrawText(taskInput, px + 28, yy + 8, 12, WHITE);
    if (taskInputActive && (int)(GetTime() * 2) % 2 == 0) {
        int tw = MeasureText(taskInput, 12);
        DrawText("|", px + 28 + tw + 1, yy + 8, 12, WHITE);
    }
    if (!taskInputActive && strlen(taskInput) == 0)
        DrawText("Ej: Investiga precios de VPS en Chile...", px + 28, yy + 8, 11, {80,90,105,255});
    Rectangle taskInputBox = {(float)px + 20, (float)yy, (float)(pw - 40), 30};
    Vector2 mp = GetMousePosition();
    if (CheckCollisionPointRec(mp, taskInputBox) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        taskInputActive = true;
    yy += 40;

    DrawText("Asignar a:", px + 20, yy + 4, 12, {148,163,184,255});
    int btnX = px + 100;
    for (int i = 0; i < (int)entities.size(); i++) {
        if (entities[i].type == EntityType::Human) continue;
        Rectangle btn = {(float)btnX, (float)yy, 80, 28};
        bool sel = (taskAssignTarget == entities[i].id);
        bool hover = CheckCollisionPointRec(mp, btn);
        DrawRectangleRounded(btn, 0.15f, 4, sel ? alpha(entities[i].color, 80) : alpha(WHITE, hover ? 12 : 5));
        DrawRectangleRoundedLines(btn, 0.15f, 4, sel ? entities[i].color : alpha(WHITE, 20));
        const char* sn = entities[i].name.c_str();
        if (entities[i].type == EntityType::CodeBot) sn = "CodeBot";
        else if (entities[i].type == EntityType::DataBot) sn = "DataBot";
        else if (entities[i].type == EntityType::Orchestrator) sn = "Orch.";
        DrawText(sn, btn.x + 10, btn.y + 7, 10, WHITE);
        if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) taskAssignTarget = entities[i].id;
        btnX += 88;
    }
    yy += 40;

    // Input handling del task panel
    if (taskInputActive) {
        int key = GetCharPressed();
        while (key > 0) {
            size_t len = strlen(taskInput);
            if (len < sizeof(taskInput) - 1 && key >= 32 && key <= 126) {
                taskInput[len] = (char)key; taskInput[len+1] = '\0';
            }
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE)) {
            size_t len = strlen(taskInput);
            if (len > 0) taskInput[len-1] = '\0';
        }
    }

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
        DrawRectangle(px + 14, entryY - 2, pw - 28, 30, alpha(WHITE, 4));
        Color sc = taskStatusColor(t.status);
        DrawCircle(px + 22, entryY + 8, 4, sc);
        DrawText(taskStatusStr(t.status), px + 32, entryY + 2, 9, sc);
        DrawText(TextFormat("#%d", t.id), px + 80, entryY + 2, 9, {100,116,139,255});
        std::string desc = t.description;
        if ((int)desc.size() > 40) desc = desc.substr(0, 37) + "...";
        DrawText(desc.c_str(), px + 110, entryY + 2, 10, WHITE);
        const char* agentName = "?";
        for (auto& e : entities) if (e.id == t.assignedTo) agentName = e.name.c_str();
        DrawText(agentName, px + 380, entryY + 2, 9, {148,163,184,255});
        if (t.status == TaskStatus::Done || t.status == TaskStatus::Failed) {
            std::string res = t.result;
            if ((int)res.size() > 55) res = res.substr(0, 52) + "...";
            DrawText(res.c_str(), px + 32, entryY + 16, 9, sc);
        }
        if (t.status == TaskStatus::InProgress && !t.steps.empty()) {
            std::string step = t.steps.back();
            if ((int)step.size() > 50) step = step.substr(0, 47) + "...";
            DrawText(step.c_str(), px + 32, entryY + 16, 9, {56,189,248,180});
        }
        entryY += 34;
    }
    if (g_tasks.empty())
        DrawText("No hay tareas. Crea una arriba.", px + 28, yy + 20, 11, {100,116,139,255});
}

// ============================================================
// drawUI
// ============================================================

// ============================================================
// Deliverables Panel — lista y visor de entregables
// ============================================================
void drawDeliverablesPanel() {
    if (!g_showDeliverables) return;

    int pw = 560, ph = 500;
    int px = (screenW - pw) / 2;
    int py = (screenH - ph) / 2;

    DrawRectangle(0, 0, screenW, screenH, alpha(BLACK, 160));
    DrawRectangleRounded({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {15,23,42,245});
    DrawRectangleRoundedLines({(float)px, (float)py, (float)pw, (float)ph}, 0.06f, 6, {16,185,129,180});
    DrawRectangleRounded({(float)px, (float)py, (float)pw, 50}, 0.06f, 6, alpha({16,185,129,255}, 25));
    DrawText("ENTREGABLES", px + 20, py + 12, 18, WHITE);
    DrawText(TextFormat("(%zu)", g_deliverables.size()), px + 160, py + 16, 12, {148,163,184,255});

    Rectangle btnClose = {(float)px + pw - 40, (float)py + 8, 32, 32};
    if (drawButton(btnClose, "X", alpha(RED, 100), 14)) { closeDeliverablesPanel(); return; }

    Vector2 mp = GetMousePosition();

    if (g_viewDeliverableId >= 0) {
        // === VISTA DE CONTENIDO ===
        Deliverable* d = nullptr;
        for (auto& del : g_deliverables) if (del.id == g_viewDeliverableId) { d = &del; break; }
        if (!d) { g_viewDeliverableId = -1; return; }

        // Header del entregable
        int yy = py + 60;
        DrawText(deliverableTypeName(d->type), px + 20, yy, 12, deliverableTypeColor(d->type));
        DrawText(d->title.c_str(), px + 90, yy, 14, WHITE);
        yy += 20;
        DrawText(TextFormat("Por: %s | Archivo: %s", d->agentName.c_str(), d->filename.c_str()),
                 px + 20, yy, 10, {148,163,184,255});
        yy += 24;

        // Boton "volver"
        Rectangle btnBack = {(float)px + 20, (float)yy, 100, 28};
        if (drawButton(btnBack, "< VOLVER", alpha(WHITE, 10), 11)) { g_viewDeliverableId = -1; return; }
        yy += 36;

        // Area de contenido (con scroll manual via lineas)
        int contentH = py + ph - yy - 16;
        DrawRectangle(px + 12, yy, pw - 24, contentH, alpha(BLACK, 120));
        DrawRectangleLines(px + 12, yy, pw - 24, contentH, alpha(WHITE, 15));

        // Renderizar contenido linea por linea
        std::string content = d->content;
        int lineY = yy + 8;
        size_t lineStart = 0;
        while (lineStart < content.size() && lineY < yy + contentH - 10) {
            size_t lineEnd = content.find('\n', lineStart);
            if (lineEnd == std::string::npos) lineEnd = content.size();
            std::string line = content.substr(lineStart, lineEnd - lineStart);
            // Truncar linea larga
            if ((int)line.size() > 75) line = line.substr(0, 72) + "...";
            DrawText(line.c_str(), px + 20, lineY, 9, {200,210,220,255});
            lineY += 12;
            lineStart = lineEnd + 1;
        }

        DrawText(TextFormat("%zu caracteres", d->content.size()),
                 px + 20, py + ph - 18, 10, {100,116,139,255});
    } else {
        // === LISTA DE ENTREGABLES ===
        int yy = py + 60;
        DrawText("ARCHIVOS ENTREGADOS", px + 20, yy, 13, {16,185,129,255});
        yy += 24;

        int listH = py + ph - yy - 16;
        DrawRectangle(px + 12, yy, pw - 24, listH, alpha(BLACK, 100));
        DrawRectangleLines(px + 12, yy, pw - 24, listH, alpha(WHITE, 15));

        if (g_deliverables.empty()) {
            DrawText("Sin entregables.", px + 28, yy + 20, 12, {100,116,139,255});
            DrawText("Asigna tareas a los bots y produciran archivos.", px + 28, yy + 40, 10, {80,90,105,255});
        }

        int entryY = yy + 8;
        int startIdx = std::max(0, (int)g_deliverables.size() - 15);
        for (int i = startIdx; i < (int)g_deliverables.size(); i++) {
            if (entryY - yy > listH - 16) break;
            auto& d = g_deliverables[i];

            Rectangle row = {(float)px + 14, (float)entryY - 2, (float)(pw - 28), 50};
            bool hover = CheckCollisionPointRec(mp, row);
            DrawRectangle(row.x, row.y, row.width, row.height, hover ? alpha(WHITE, 8) : alpha(WHITE, 3));
            DrawRectangleLines(row.x, row.y, row.width, row.height, alpha(deliverableTypeColor(d.type), hover ? 100 : 30));

            // Tipo
            Color tc = deliverableTypeColor(d.type);
            DrawRectangleRounded({row.x + 4, row.y + 6, 60, 18}, 0.2f, 3, alpha(tc, 60));
            DrawText(deliverableTypeName(d.type), row.x + 8, row.y + 9, 9, tc);

            // Titulo
            std::string title = d.title;
            if ((int)title.size() > 35) title = title.substr(0, 32) + "...";
            DrawText(title.c_str(), row.x + 72, row.y + 6, 12, WHITE);

            // Autor
            DrawText(TextFormat("Por %s", d.agentName.c_str()), row.x + 72, row.y + 24, 9, {148,163,184,255});

            // Preview
            std::string prev = d.preview;
            if ((int)prev.size() > 50) prev = prev.substr(0, 47) + "...";
            DrawText(prev.c_str(), row.x + 72, row.y + 36, 8, {100,116,139,255});

            // Click para ver
            if (hover && IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                g_viewDeliverableId = d.id;

            entryY += 56;
        }
    }
}

void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused) {
    // TOP BAR
    for (int x = 0; x < screenW && x < 1280; x++) {
        float t = (float)x / screenW;
        Color c = {(unsigned char)(15+t*8), (unsigned char)(23+t*5), (unsigned char)(42+t*3), 230};
        DrawLine(x, 0, x, 50, c);
    }
    DrawLineEx({0, 50}, {(float)screenW, 50}, 2, {56,189,248,40});
    DrawText("META-OFFICE", 20, 12, 20, WHITE);
    DrawText("2D", 20 + MeasureText("META-OFFICE", 20) + 6, 16, 12, {56,189,248,255});

    bool llmOk = !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...";
    float llmPulse = sinf(GetTime() * 2) * 0.3f + 0.7f;
    DrawCircle(380, 22, 5, llmOk ? alpha({16,185,129,255}, (int)(255 * llmPulse)) : ORANGE);
    DrawText(llmOk ? "LLM OK" : "LLM OFF", 392, 16, 11, llmOk ? (Color){16,185,129,255} : ORANGE);

    if (!entities.empty())
        DrawText(TextFormat("Pos: (%.0f,%.0f)", entities[0].x, entities[0].y), 450, 18, 12, {148,163,184,255});

    DrawCircle(screenW - 400, 16, 5, paused ? ORANGE : GREEN);
    DrawText(paused ? "PAUSADO" : "LIVE", screenW - 388, 12, 12, paused ? ORANGE : GREEN);

    // Botón ARCHIVOS
    Rectangle btnFiles = {(float)screenW - 400, 8, 80, 34};
    bool filesHover = CheckCollisionPointRec(GetMousePosition(), btnFiles);
    DrawRectangleRounded(btnFiles, 0.2f, 4, g_showDeliverables ? alpha({16,185,129,255}, 60) : alpha(WHITE, filesHover ? 15 : 8));
    DrawRectangleRoundedLines(btnFiles, 0.2f, 4, g_showDeliverables ? (Color){16,185,129,200} : alpha(WHITE, 30));
    DrawText("ARCHIVOS", btnFiles.x + 4, btnFiles.y + 9, 11, WHITE);

        // Botón ARCHIVOS

    // Botón TAREAS
    Rectangle btnTasks = {(float)screenW - 310, 8, 80, 34};
    Vector2 mp = GetMousePosition();
    bool tasksHover = CheckCollisionPointRec(mp, btnTasks);
    int activeTasks = 0;
    for (auto& t : g_tasks) if (t.status == TaskStatus::Pending || t.status == TaskStatus::InProgress) activeTasks++;
    DrawRectangleRounded(btnTasks, 0.2f, 4, g_showTaskPanel ? alpha({245,158,11,255}, 60) : alpha(WHITE, tasksHover ? 15 : 8));
    DrawRectangleRoundedLines(btnTasks, 0.2f, 4, g_showTaskPanel ? (Color){245,158,11,200} : alpha(WHITE, 30));
    DrawText("TAREAS", btnTasks.x + 8, btnTasks.y + 9, 12, WHITE);
    if (activeTasks > 0)
        DrawText(TextFormat("(%d)", activeTasks), btnTasks.x + 58, btnTasks.y + 4, 10, {245,158,11,255});

    // Botón CFG
    Rectangle btnConfig = {(float)screenW - 218, 8, 40, 34};
    bool configHover = CheckCollisionPointRec(mp, btnConfig);
    DrawRectangleRounded(btnConfig, 0.2f, 4, g_showLlmConfig ? alpha({56,189,248,255}, 60) : alpha(WHITE, configHover ? 15 : 8));
    DrawRectangleRoundedLines(btnConfig, 0.2f, 4, g_showLlmConfig ? (Color){56,189,248,200} : alpha(WHITE, 30));
    DrawText("CFG", btnConfig.x + 6, btnConfig.y + 9, 12, WHITE);

    // Botón PAUSAR
    Rectangle btnPause = {(float)screenW - 168, 8, 80, 34};
    drawButton(btnPause, paused ? "PLAY" : "PAUSA", paused ? (Color){245,158,11,80} : alpha(WHITE, 12), 12);

    // SIDEBAR
    int sx = screenW - 300;
    for (int y = 50; y < screenH; y++) {
        float t = (float)(y - 50) / (screenH - 50);
        Color c = {(unsigned char)(10+t*3), (unsigned char)(14+t*2), (unsigned char)(23+t*2), 210};
        DrawLine(sx, y, screenW, y, c);
    }
    DrawLineEx({(float)sx, 50}, {(float)sx, (float)screenH}, 2, {56,189,248,30});
    DrawText("MIEMBROS", sx + 16, 60, 14, {148,163,184,255});
    DrawText(TextFormat("(%zu)", entities.size()), sx + 106, 60, 14, {100,116,139,255});
    DrawLineEx({(float)sx + 12, 78}, {(float)screenW - 12, 78}, 1, alpha(WHITE, 10));

    int yy = 86;
    for (auto& e : entities) {
        bool sel = (e.id == selectedId);
        bool hover = CheckCollisionPointRec(mp, {(float)sx + 10, (float)yy, 280, 62});
        Rectangle card = {(float)sx + 10, (float)yy, 280, 62};
        DrawRectangleRounded({card.x+2, card.y+2, card.width, card.height}, 0.08f, 4, alpha(BLACK, 50));
        DrawRectangleRounded(card, 0.08f, 4, sel ? alpha(e.color, 35) : alpha(WHITE, hover ? 10 : 4));
        DrawRectangleRoundedLines(card, 0.08f, 4, sel ? e.color : alpha(WHITE, hover ? 25 : 12));
        const char* icon = e.type == EntityType::Human ? "USR" : e.type == EntityType::CodeBot ? "BOT" : e.type == EntityType::DataBot ? "DAT" : "ORQ";
        DrawRectangleRounded({(float)sx+18, (float)yy+10, 36, 36}, 0.15f, 4, e.color);
        DrawText(icon, sx + 22, yy + 18, 10, WHITE);
        DrawText(e.name.c_str(), sx + 62, yy + 13, 13, WHITE);
        DrawText(e.role.c_str(), sx + 62, yy + 30, 9, {148,163,184,255});
        if (e.testCount > 0) DrawText(TextFormat("T:%d", e.testCount), sx + 62, yy + 44, 9, {56,189,248,180});
        if (e.queryCount > 0) DrawText(TextFormat("Q:%d", e.queryCount), sx + 90, yy + 44, 9, {16,185,129,180});
        const char* st = "IDLE"; Color sc = {148,163,184,255};
        if (e.status == Status::Walking) { st = "ACTIVO"; sc = {56,189,248,255}; }
        else if (e.status == Status::Busy) { st = "OCUPADO"; sc = ORANGE; }
        else if (e.status == Status::Intervening) { st = "AYUDANDO"; sc = {168,85,247,255}; }
        DrawCircle(sx + 216, yy + 18, 4, sc);
        DrawText(st, sx + 226, yy + 14, 10, sc);
        if (!e.agentLog.empty() && e.type != EntityType::Human)
            DrawText(TextFormat("[%zu]", e.agentLog.size()), sx + 260, yy + 14, 9, alpha(e.color, 180));
        if (e.hasGreetedHuman && e.type != EntityType::Human) {
            float bounce = sinf(GetTime() * 4) * 2;
            DrawCircle(sx + 260, yy + 38 + bounce, 3, alpha(e.color, 200));
        }
        yy += 68;
    }

    yy += 6;
    DrawLineEx({(float)sx+12, (float)yy}, {(float)screenW-12, (float)yy}, 1, alpha(WHITE, 10));
    yy += 10;
    DrawText("ACTIVIDAD", sx + 16, yy, 14, {148,163,184,255});
    yy += 22;
    int start = std::max(0, (int)logs.size() - 9);
    for (int i = start; i < (int)logs.size(); i++) {
        DrawCircle(sx + 18, yy + 5, 2, logs[i].color);
        DrawText(logs[i].text.c_str(), sx + 26, yy, 9, logs[i].color);
        yy += 14;
    }

    // HUD
    for (int x = 0; x < 620; x++) {
        float t = 1.0f - (float)x / 620;
        DrawLine(10 + x, screenH - 40, 10 + x, screenH - 10, {0, 0, 0, (unsigned char)(160 * t)});
    }
    DrawText("Clic izq: charlar | Clic der: log | WASD: mover | T: tareas | ESC: cerrar",
             18, screenH - 33, 12, {148,163,184,255});

    // Overlays
    drawLlmConfigPanel();
    drawChatPanel(entities);
    drawLogPanel(entities);
    drawDeliverablesPanel();
    drawTaskPanel(entities);
}

// ============================================================
// handleInput
// ============================================================
void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY) {
    // Deliverables panel
    if (g_showDeliverables) {
        if (IsKeyPressed(KEY_ESCAPE)) { g_viewDeliverableId = -1; if (!g_viewDeliverableId) closeDeliverablesPanel(); }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 560, ph = 500;
            int ppx = (screenW - pw) / 2;
            int ppy = (screenH - ph) / 2;
            if (!CheckCollisionPointRec(mp, {(float)ppx, (float)ppy, (float)pw, (float)ph}))
                closeDeliverablesPanel();
        }
        return;
    }

    // Task panel abierto
    if (g_showTaskPanel) {
        if (IsKeyPressed(KEY_ESCAPE)) closeTaskPanel();
        // Click fuera del panel lo cierra
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 500, ph = 480;
            int px = (screenW - pw) / 2;
            int py = (screenH - ph) / 2;
            if (!CheckCollisionPointRec(mp, {(float)px, (float)py, (float)pw, (float)ph}))
                closeTaskPanel();
        }
        return;
    }

    // Log panel abierto
    if (g_showLog) {
        if (IsKeyPressed(KEY_ESCAPE)) { closeLogPanel(); return; }
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 460, ph = 440;
            int px = (screenW - pw) / 2;
            int py = (screenH - ph) / 2 - 20;
            if (!CheckCollisionPointRec(mp, {(float)px, (float)py, (float)pw, (float)ph}))
                closeLogPanel();
        }
        return;
    }

    // Chat abierto
    if (g_showChat) {
        int key = GetCharPressed();
        while (key > 0) {
            if (key >= 32 && key <= 126 && g_chatInput.size() < 200) g_chatInput += (char)key;
            key = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !g_chatInput.empty()) g_chatInput.pop_back();
        if (IsKeyPressed(KEY_ENTER) && !g_chatInput.empty()) {
            Entity* target = nullptr;
            for (auto& e : entities) if (e.id == g_chatTargetId) { target = &e; break; }
            if (target && !g_llmConfig.apiKey.empty() && g_llmConfig.apiKey != "sk-...") {
                g_chatHistory.push_back({"user", g_chatInput});
                g_chatHistory.push_back({"assistant", "pensando..."});
                std::lock_guard<std::mutex> lock(g_llmMutex);
                g_llmQueue.push({g_chatTargetId, getSystemPrompt(target->type), g_chatInput, GetTime() + 15.0});
            }
            g_chatInput.clear();
        }
        if (IsKeyPressed(KEY_ESCAPE)) closeChat();
        return;
    }

    // LLM Config abierto
    if (g_showLlmConfig) {
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            Vector2 mp = GetMousePosition();
            int pw = 540, px = (screenW - pw) / 2;
            int py = (screenH - 400) / 2;
            int labelW = 130, yy = py + 68, fieldW = pw - 150;
            if (mp.x > px+labelW && mp.x < px+labelW+fieldW && mp.y > yy && mp.y < yy+26) { editField=1; inputActive=true; strncpy(inputEndpoint, g_llmConfig.endpoint.c_str(), sizeof(inputEndpoint)-1); }
            yy += 38;
            if (mp.x > px+labelW && mp.x < px+labelW+fieldW && mp.y > yy && mp.y < yy+26) { editField=2; inputActive=true; strncpy(inputApiKey, g_llmConfig.apiKey.c_str(), sizeof(inputApiKey)-1); }
            yy += 38;
            if (mp.x > px+labelW && mp.x < px+labelW+fieldW && mp.y > yy && mp.y < yy+26) { editField=3; inputActive=true; strncpy(inputModel, g_llmConfig.model.c_str(), sizeof(inputModel)-1); }
        }
        return;
    }

    // Tecla T = abrir tareas
    if (IsKeyPressed(KEY_T)) { g_showTaskPanel = !g_showTaskPanel; return; }
    if (IsKeyPressed(KEY_P)) paused = !paused;

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
        Vector2 mp = GetMousePosition();

    // Botón ARCHIVOS

        // Botón ARCHIVOS

        // Botón ARCHIVOS
        Rectangle btnFilesH = {(float)screenW - 400, 8, 80, 34};
        if (CheckCollisionPointRec(mp, btnFilesH)) { g_showDeliverables = !g_showDeliverables; g_viewDeliverableId = -1; return; }

        // Botón TAREAS
        Rectangle btnTasks = {(float)screenW - 310, 8, 80, 34};
        if (CheckCollisionPointRec(mp, btnTasks)) { g_showTaskPanel = !g_showTaskPanel; return; }

        // Botón CFG
        Rectangle btnConfig = {(float)screenW - 218, 8, 40, 34};
        if (CheckCollisionPointRec(mp, btnConfig)) { toggleLlmConfig(); return; }

        // Botón PAUSA
        Rectangle btnPause = {(float)screenW - 168, 8, 80, 34};
        if (CheckCollisionPointRec(mp, btnPause)) { paused = !paused; return; }

        // Click en sidebar
        int sxx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sxx + 10, (float)(86 + i * 68), 280, 62};
            if (CheckCollisionPointRec(mp, card)) {
                selectedId = entities[i].id;
                if (entities[i].type != EntityType::Human) openChat(entities[i].id);
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
                if (!clickedAgent && !entities.empty()) {
                    entities[0].targetX = gx;
                    entities[0].targetY = gy;
                    selectedId = -1;
                }
            }
        }
    }

    // Clic derecho → log panel
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        Vector2 mp = GetMousePosition();
        int sxx = screenW - 300;
        for (int i = 0; i < (int)entities.size(); i++) {
            Rectangle card = {(float)sxx + 10, (float)(86 + i * 68), 280, 62};
            if (CheckCollisionPointRec(mp, card)) { openLogPanel(entities[i].id); return; }
        }
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

    // WASD
    if (!entities.empty()) {
        auto* human = &entities[0];
        float nx = human->targetX, ny = human->targetY;
        if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) ny = std::max(0.0f, ny - 1);
        if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) ny = std::min(15.0f, ny + 1);
        if (IsKeyPressed(KEY_A) || IsKeyPressed(KEY_LEFT)) nx = std::max(0.0f, nx - 1);
        if (IsKeyPressed(KEY_D) || IsKeyPressed(KEY_RIGHT)) nx = std::min(15.0f, nx + 1);
        if (nx != human->targetX || ny != human->targetY) { human->targetX = nx; human->targetY = ny; }
    }

    float wheel = GetMouseWheelMove();
    panY += wheel * 5;
    origin.y += wheel * 5;
}