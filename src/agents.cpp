#include "meta_office.hpp"
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <chrono>

LLMConfig g_llmConfig;
bool g_showLlmConfig = false;
bool g_showChat = false;
int g_chatTargetId = -1;
std::vector<std::pair<std::string, std::string>> g_chatHistory;
std::string g_chatInput;
bool g_showLog = false;
int g_logTargetId = -1;
// Tareas — definidos en tasks.cpp

// Cola de LLM async
std::queue<LlmRequest> g_llmQueue;
std::vector<LlmResponse> g_llmResults;
std::mutex g_llmMutex;
std::atomic<bool> g_llmThreadRunning(false);
static std::thread g_llmThread;

static void llmWorker() {
    while (g_llmThreadRunning) {
        LlmRequest req;
        {
            std::lock_guard<std::mutex> lock(g_llmMutex);
            if (!g_llmQueue.empty()) {
                req = g_llmQueue.front();
                g_llmQueue.pop();
            }
        }
        if (req.entityId != 0 || !req.userMessage.empty()) {
            std::vector<LLMMessage> msgs = {
                {"system", req.systemPrompt},
                {"user", req.userMessage}
            };
            std::string reply = llmChat(g_llmConfig, msgs);
            if (reply.empty()) reply = "...";
            if (reply.size() > 120) reply = reply.substr(0, 117) + "...";
            double expiry = req.expiryTime;
            double now = std::chrono::duration<double>(std::chrono::system_clock::now().time_since_epoch()).count();
            if (now > expiry) expiry = now + 8.0;
            {
                std::lock_guard<std::mutex> lock(g_llmMutex);
                g_llmResults.push_back({req.entityId, reply, expiry});
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void startLlmThread() {
    if (g_llmThreadRunning) return;
    g_llmThreadRunning = true;
    g_llmThread = std::thread(llmWorker);
}

void stopLlmThread() {
    g_llmThreadRunning = false;
    if (g_llmThread.joinable()) g_llmThread.join();
}

static void loadLlmConfig() {
    std::ifstream f("llm_config.env");
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "LLM_ENDPOINT") g_llmConfig.endpoint = val;
        else if (key == "OPENAI_API_KEY") g_llmConfig.apiKey = val;
        else if (key == "LLM_MODEL") g_llmConfig.model = val;
    }
}

void saveLlmConfig() {
    std::ofstream f("llm_config.env");
    if (!f.is_open()) return;
    f << "# Meta-Office LLM Config\n";
    f << "# Protocolo compatible con OpenAI API\n\n";
    f << "LLM_ENDPOINT=" << g_llmConfig.endpoint << "\n";
    f << "OPENAI_API_KEY=" << g_llmConfig.apiKey << "\n";
    f << "LLM_MODEL=" << g_llmConfig.model << "\n";
    f.close();
}

const char* getSystemPrompt(EntityType type) {
    return getChatSystemPrompt(type);
}

// Prompt para CHAT — estricto, solo responde lo que se le pregunta
const char* getChatSystemPrompt(EntityType type) {
    switch (type) {
        case EntityType::CodeBot:
            return "Eres CodeBot, ingeniero de software. "
                   "REGLA: Solo responde exactamente lo que se te pregunta. "
                   "No divagues, no agregues contenido no solicitado. "
                   "Si no sabes, di 'No se'. Responde en 1-3 oraciones maximo. "
                   "Habla en espanol chileno.";
        case EntityType::DataBot:
            return "Eres DataBot, analista de datos. "
                   "REGLA: Solo responde exactamente lo que se te pregunta. "
                   "No divagues, no agregues contenido no solicitado. "
                   "Si no sabes, di 'No se'. Responde en 1-3 oraciones maximo. "
                   "Habla en espanol chileno.";
        case EntityType::Orchestrator:
            return "Eres Orchestrator, supervisor de agentes. "
                   "REGLA: Solo responde exactamente lo que se te pregunta. "
                   "No divagues, no agregues contenido no solicitado. "
                   "Si no sabes, di 'No se'. Responde en 1-3 oraciones maximo. "
                   "Habla en espanol chileno.";
        default:
            return "Responde solo lo que se te pregunta. Breve y directo.";
    }
}

// Prompt para TAREAS — puede producir entregables
const char* getTaskSystemPrompt(EntityType type) {
    switch (type) {
        case EntityType::CodeBot:
            return "Eres CodeBot, ingeniero de software de una oficina virtual. "
                   "Te asignan tareas reales. Puedes navegar internet con fetch(https://url.com). "
                   "Cuando produces codigo, un script o un documento, usa el formato:\n"
                   "ENTREGABLE: [titulo]\n```lenguaje\n[contenido completo]\n```\n"
                   "Despues del entregable, escribe RESULTADO: con un resumen de 1 oracion. "
                   "Si no necesitas entregar un archivo, solo escribe RESULTADO: tu respuesta.\n"
                   "Habla en espanol chileno.";
        case EntityType::DataBot:
            return "Eres DataBot, analista de datos y BI de una oficina virtual. "
                   "Te asignan tareas reales. Puedes navegar internet con fetch(https://url.com). "
                   "Cuando produces un informe, analisis o documento, usa el formato:\n"
                   "ENTREGABLE: [titulo]\n```texto\n[contenido completo]\n```\n"
                   "Despues del entregable, escribe RESULTADO: con un resumen de 1 oracion. "
                   "Si no necesitas entregar un archivo, solo escribe RESULTADO: tu respuesta.\n"
                   "Habla en espanol chileno.";
        case EntityType::Orchestrator:
            return "Eres Orchestrator, supervisor de agentes de una oficina virtual. "
                   "Te asignan tareas de coordinacion. Puedes navegar internet con fetch(https://url.com). "
                   "Cuando produces un informe o plan, usa el formato:\n"
                   "ENTREGABLE: [titulo]\n```texto\n[contenido completo]\n```\n"
                   "Despues del entregable, escribe RESULTADO: con un resumen de 1 oracion. "
                   "Si no necesitas entregar un archivo, solo escribe RESULTADO: tu respuesta.\n"
                   "Habla en espanol chileno.";
        default:
            return "Eres un asistente. Produce entregables con ENTREGABLE: [titulo] y ```contenido```. "
                   "Despues escribe RESULTADO: resumen.";
    }
}

// Sonidos
static Sound genBeep(float freq, float duration, float volume) {
    int sampleRate = 22050;
    int sampleCount = (int)(sampleRate * duration);
    short* samples = new short[sampleCount];
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float val = sinf(2 * 3.14159f * freq * t);
        float env = 1.0f;
        if (t < 0.01f) env = t / 0.01f;
        if (t > duration - 0.01f) env = (duration - t) / 0.01f;
        samples[i] = (short)(val * env * volume * 32767);
    }
    Wave w = {0};
    w.data = samples;
    w.frameCount = sampleCount;
    w.sampleRate = sampleRate;
    w.sampleSize = 16;
    w.channels = 1;
    Sound s = LoadSoundFromWave(w);
    delete[] samples;
    return s;
}

static Sound sndPass = {0};
static Sound sndChat = {0};
static bool soundsLoaded = false;

static void initSounds() {
    if (soundsLoaded) return;
    sndPass = genBeep(880, 0.15f, 0.3f);
    sndChat = genBeep(440, 0.08f, 0.15f);
    soundsLoaded = true;
}

// ============================================================
// Posiciones "home" de cada bot (donde vuelven al terminar)
// ============================================================
static float homeX(EntityType type) {
    switch (type) {
        case EntityType::CodeBot: return 3;
        case EntityType::DataBot: return 12;
        case EntityType::Orchestrator: return 8;
        default: return 11;
    }
}
static float homeY(EntityType type) {
    switch (type) {
        case EntityType::CodeBot: return 3;
        case EntityType::DataBot: return 4;
        case EntityType::Orchestrator: return 8;
        default: return 11;
    }
}

void initSimulation(std::vector<Entity>& entities, std::vector<LogEntry>& logs) {
    g_llmConfig = llmConfigFromEnv();
    loadLlmConfig();
    initSounds();
    startLlmThread();

    entities.clear();
    logs.clear();
    srand(time(nullptr));

    entities.push_back({
        .id = 0, .name = "Supervisor Humano", .role = "Supervisor",
        .type = EntityType::Human, .color = {245,158,11,255},
        .x = 11, .y = 11, .targetX = 11, .targetY = 11,
        .renderX = 11, .renderY = 11,
        .status = Status::Idle
    });

    entities.push_back({
        .id = 1, .name = "CodeBot", .role = "QA & Software Engineer",
        .type = EntityType::CodeBot, .color = {56,189,248,255},
        .x = 3, .y = 3, .targetX = 3, .targetY = 3,
        .renderX = 3, .renderY = 3,
        .status = Status::Idle, .testCount = 42
    });

    entities.push_back({
        .id = 2, .name = "DataBot", .role = "Analista de Datos & BI",
        .type = EntityType::DataBot, .color = {16,185,129,255},
        .x = 12, .y = 4, .targetX = 12, .targetY = 4,
        .renderX = 12, .renderY = 4,
        .status = Status::Idle, .queryCount = 128
    });

    entities.push_back({
        .id = 3, .name = "Orchestrator", .role = "Orquestador Multi-Agente",
        .type = EntityType::Orchestrator, .color = {168,85,247,255},
        .x = 8, .y = 8, .targetX = 8, .targetY = 8,
        .renderX = 8, .renderY = 8,
        .status = Status::Idle
    });

    logs.push_back({"Oficina virtual iniciada.", GetTime(), {56,189,248,255}});
    logs.push_back({"CodeBot listo en Area de Desarrollo.", GetTime(), {56,189,248,255}});
    logs.push_back({"DataBot conectado a data warehouse.", GetTime(), {16,185,129,255}});
    logs.push_back({"Orchestrator monitoreando red de agentes.", GetTime(), {168,85,247,255}});

    loadMemory(entities);
    loadTasks();
    loadDeliverables();
    for (auto& t : g_tasks) {
        if (t.status == TaskStatus::InProgress) t.status = TaskStatus::Pending;
    }
}

// Encola una peticion LLM (no bloquea)
static void agentThink(Entity& e, const std::string& userMsg, double time) {
    if (g_llmConfig.apiKey.empty() || g_llmConfig.apiKey == "sk-...") {
        e.speech = {"Configura el LLM en CFG para hablar conmigo", time + 5.0};
        return;
    }
    e.speech = {"pensando...", time + 30.0};
    std::lock_guard<std::mutex> lock(g_llmMutex);
    g_llmQueue.push({e.id, getSystemPrompt(e.type), userMsg, time + 15.0});
}

// ============================================================
// Encontrar un bot por tipo
// ============================================================
static Entity* findBot(std::vector<Entity>& entities, EntityType type) {
    for (auto& e : entities) if (e.type == type) return &e;
    return nullptr;
}

// ============================================================
// Colaboración entre bots
// ============================================================
struct Collaboration {
    int requesterId = -1;
    int helperId = -1;
    int taskId = -1;
    std::string description;
    double startTime = 0;
    bool meetingActive = false;
    bool meetingDone = false;
    double meetingDuration = 5.0; // segundos juntos antes de completar
};

static std::vector<Collaboration> g_collaborations;

// Iniciar una colaboracion: el requester camina hacia el helper
static void startCollaboration(int requesterId, int helperId, int taskId,
                                const std::string& desc, double time,
                                std::vector<Entity>& entities, std::vector<LogEntry>& logs) {
    Collaboration c;
    c.requesterId = requesterId;
    c.helperId = helperId;
    c.taskId = taskId;
    c.description = desc;
    c.startTime = time;
    c.meetingActive = false;
    c.meetingDone = false;

    Entity* requester = nullptr;
    Entity* helper = nullptr;
    for (auto& e : entities) {
        if (e.id == requesterId) requester = &e;
        if (e.id == helperId) helper = &e;
    }
    if (!requester || !helper) return;

    // El requester camina hacia el helper
    requester->targetX = helper->x + 1;
    requester->targetY = helper->y;
    requester->status = Status::Walking;
    requester->speech = {"Voy con " + helper->name, time + 8.0};

    char logMsg[256];
    snprintf(logMsg, sizeof(logMsg), "%s va con %s: %s",
             requester->name.c_str(), helper->name.c_str(),
             desc.size() > 40 ? (desc.substr(0, 37) + "...").c_str() : desc.c_str());
    logs.push_back({logMsg, GetTime(), requester->color});

    g_collaborations.push_back(c);
}

void updateSimulation(std::vector<Entity>& entities, std::vector<LogEntry>& logs,
                      bool paused, double dt, double time) {
    (void)dt;
    if (paused) return;

    // ============================================================
    // Movimiento suave
    // ============================================================
    const float speed = 0.08f;
    for (auto& e : entities) {
        float dx = e.targetX - e.renderX;
        float dy = e.targetY - e.renderY;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 0.05f) {
            e.renderX += dx * speed;
            e.renderY += dy * speed;
            if (e.status != Status::Busy) e.status = Status::Walking;
        } else {
            e.renderX = e.targetX;
            e.renderY = e.targetY;
            e.x = e.targetX;
            e.y = e.targetY;
            if (e.status == Status::Walking) e.status = e.isActive ? Status::Busy : Status::Idle;
        }
    }

    auto* human = &entities[0];

    // ============================================================
    // Proximidad del humano — indicador "disponible"
    // ============================================================
    if (human) {
        for (auto& e : entities) {
            if (e.type == EntityType::Human) continue;
            float d = sqrtf(powf(e.x - human->x, 2) + powf(e.y - human->y, 2));
            if (d <= 3.0f) {
                e.hasGreetedHuman = true;
            } else if (d > 4.0f) {
                e.hasGreetedHuman = false;
            }
        }
    }

    // ============================================================
    // Procesar colaboraciones (bots que se mueven hacia otros)
    // ============================================================
    for (auto& col : g_collaborations) {
        if (col.meetingDone) continue;

        Entity* requester = nullptr;
        Entity* helper = nullptr;
        for (auto& e : entities) {
            if (e.id == col.requesterId) requester = &e;
            if (e.id == col.helperId) helper = &e;
        }
        if (!requester || !helper) { col.meetingDone = true; continue; }

        float d = sqrtf(powf(requester->x - (helper->x + 1), 2) +
                        powf(requester->y - helper->y, 2));

        if (d < 0.5f && !col.meetingActive) {
            // Llegaron, empezar reunion
            col.meetingActive = true;
            col.startTime = time;
            requester->status = Status::Busy;
            helper->status = Status::Busy;
            requester->speech = {"Colaborando con " + helper->name, time + 10.0};
            helper->speech = {"Colaborando con " + requester->name, time + 10.0};
            if (IsAudioDeviceReady()) PlaySound(sndChat);
            char logMsg[256];
            snprintf(logMsg, sizeof(logMsg), "%s y %s colaborando", requester->name.c_str(), helper->name.c_str());
            logs.push_back({logMsg, GetTime(), {168,85,247,255}});
            requester->agentLog.push_back({"Colaboro con " + helper->name, GetTime(), requester->color});
            helper->agentLog.push_back({"Colaboro con " + requester->name, GetTime(), helper->color});
        }

        if (col.meetingActive && time - col.startTime > col.meetingDuration) {
            // Terminar colaboracion
            col.meetingDone = true;
            col.meetingActive = false;

            // Marcar la tarea como completada
            for (auto& task : g_tasks) {
                if (task.id == col.taskId) {
                    task.status = TaskStatus::Done;
                    task.result = "Completada en colaboracion con " + helper->name;
                    task.completedAt = time;
                    break;
                }
            }

            // Volver a home
            requester->targetX = homeX(requester->type);
            requester->targetY = homeY(requester->type);
            requester->isActive = false;
            requester->currentTask.clear();
            requester->currentTaskId = -1;
            requester->status = Status::Idle;
            requester->speech = {"Tarea completada con " + helper->name, time + 5.0};

            helper->status = Status::Idle;
            helper->speech = {"Colaboracion terminada", time + 4.0};

            if (IsAudioDeviceReady()) PlaySound(sndPass);
            logs.push_back({"Colaboracion completada", GetTime(), {16,185,129,255}});
            saveTasks();
        }
    }

    // Limpiar colaboraciones terminadas
    g_collaborations.erase(
        std::remove_if(g_collaborations.begin(), g_collaborations.end(),
            [](const Collaboration& c) { return c.meetingDone; }),
        g_collaborations.end());

    // ============================================================
    // Procesar respuestas del hilo LLM
    // ============================================================
    {
        std::lock_guard<std::mutex> lock(g_llmMutex);
        for (auto& res : g_llmResults) {
            for (auto& e : entities) {
                if (e.id == res.entityId) {
                    if (g_showChat && g_chatTargetId == res.entityId) {
                        e.speech = {res.text, res.expiryTime};
                        if (!g_chatHistory.empty() && g_chatHistory.back().second == "pensando...")
                            g_chatHistory.back().second = res.text;
                        else
                            g_chatHistory.push_back({"assistant", res.text});
                    }
                    e.agentLog.push_back({res.text, GetTime(), e.color});
                    if (e.agentLog.size() > 50) e.agentLog.erase(e.agentLog.begin());
                    break;
                }
            }
        }
        g_llmResults.clear();
    }

    // ============================================================
    // Limpiar speech bubbles expirados
    // ============================================================
    for (auto& e : entities) {
        if (!e.speech.text.empty()) {
            bool isSystemMsg = (e.speech.text.find("Voy con") != std::string::npos ||
                               e.speech.text.find("Colaborando") != std::string::npos ||
                               e.speech.text.find("Colaboracion") != std::string::npos ||
                               e.speech.text.find("Tarea") != std::string::npos ||
                               e.speech.text.find("Iniciando") != std::string::npos ||
                               e.speech.text.find("Trabajando") != std::string::npos ||
                               e.speech.text.find("Navegando") != std::string::npos ||
                               e.speech.text.find("completada") != std::string::npos);
            if (time > e.speech.expiry || (!isSystemMsg && !(g_showChat && g_chatTargetId == e.id))) {
                e.speech.text.clear();
            }
        }
    }

    // ============================================================
    // Procesar tareas (multi-turn + colaboracion entre bots)
    // ============================================================
    static double lastTaskProcess = 0;
    if (time - lastTaskProcess > 2.0) {
        lastTaskProcess = time;
        for (auto& task : g_tasks) {
            if (task.status != TaskStatus::InProgress) continue;

            Entity* agent = nullptr;
            for (auto& e : entities) {
                if (e.id == task.assignedTo) { agent = &e; break; }
            }
            if (!agent) { task.status = TaskStatus::Failed; task.result = "Agente no encontrado"; continue; }

            // ¿La tarea necesita colaboracion con otro bot?
            // Si la descripcion menciona otro agente, iniciar colaboracion
            std::string descLower = task.description;
            for (auto& c : descLower) c = tolower(c);

            bool needsCollab = false;
            int helperId = -1;
            std::string helperName;

            if (descLower.find("codebot") != std::string::npos && agent->type != EntityType::CodeBot) {
                Entity* helper = findBot(entities, EntityType::CodeBot);
                if (helper && !helper->isActive) { needsCollab = true; helperId = helper->id; helperName = "CodeBot"; }
            } else if (descLower.find("databot") != std::string::npos && agent->type != EntityType::DataBot) {
                Entity* helper = findBot(entities, EntityType::DataBot);
                if (helper && !helper->isActive) { needsCollab = true; helperId = helper->id; helperName = "DataBot"; }
            } else if (descLower.find("orchestrator") != std::string::npos && agent->type != EntityType::Orchestrator) {
                Entity* helper = findBot(entities, EntityType::Orchestrator);
                if (helper && !helper->isActive) { needsCollab = true; helperId = helper->id; helperName = "Orchestrator"; }
            }

            if (needsCollab && helperId >= 0) {
                // Verificar que no haya una colaboracion ya activa para esta tarea
                bool alreadyCollabing = false;
                for (auto& c : g_collaborations) {
                    if (c.taskId == task.id && !c.meetingDone) { alreadyCollabing = true; break; }
                }
                if (!alreadyCollabing) {
                    startCollaboration(agent->id, helperId, task.id, task.description, time, entities, logs);
                    // Marcar que está esperando colaboracion
                    task.steps.push_back("Esperando colaboracion con " + helperName);
                    continue; // no procesar LLM todavia
                }
            }

            // Si está colaborando, no procesar LLM todavia
            bool isCollabing = false;
            for (auto& c : g_collaborations) {
                if (c.taskId == task.id && !c.meetingDone) { isCollabing = true; break; }
            }
            if (isCollabing) continue;

            // Procesar un step normal de la tarea
            processTaskStep(task, *agent, logs);
        }
    }

    // ============================================================
    // Auto-asignar tareas pendientes
    // ============================================================
    for (auto& task : g_tasks) {
        if (task.status != TaskStatus::Pending) continue;
        if (task.assignedTo < 0) continue;

        Entity* agent = nullptr;
        for (auto& e : entities) {
            if (e.id == task.assignedTo && !e.isActive) {
                agent = &e; break;
            }
        }
        if (agent) {
            task.status = TaskStatus::InProgress;
            task.createdAt = time;
            agent->isActive = true;
            agent->currentTask = task.description;
            agent->currentTaskId = task.id;
            agent->status = Status::Busy;
            agent->speech = {"Iniciando tarea...", time + 5.0};
            logs.push_back({TextFormat("%s inicia tarea #%d", agent->name.c_str(), task.id),
                           GetTime(), agent->color});
        }
    }

    // ============================================================
    // Bots inactivos vuelven a su posicion home
    // ============================================================
    for (auto& e : entities) {
        if (e.type == EntityType::Human) continue;
        if (!e.isActive && e.status != Status::Busy && e.status != Status::Intervening) {
            float hx = homeX(e.type);
            float hy = homeY(e.type);
            float d = sqrtf(powf(e.x - hx, 2) + powf(e.y - hy, 2));
            if (d > 1.0f) {
                e.targetX = hx;
                e.targetY = hy;
            }
        }
    }

    while (logs.size() > 40) logs.erase(logs.begin());
}

void openChat(int entityId) {
    g_chatTargetId = entityId;
    g_chatHistory.clear();
    g_chatInput.clear();
    g_showChat = true;
}

void closeChat() {
    g_showChat = false;
    g_chatTargetId = -1;
    g_chatHistory.clear();
    g_chatInput.clear();
}

void openLogPanel(int entityId) {
    g_logTargetId = entityId;
    g_showLog = true;
}

void closeLogPanel() {
    g_showLog = false;
    g_logTargetId = -1;
}
