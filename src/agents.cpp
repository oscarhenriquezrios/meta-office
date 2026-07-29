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
            // Si la respuesta llega después del expiry original, darle 8s extra
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

// Cargar config LLM desde llm_config.env
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

// Guardar config LLM a llm_config.env
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
    switch (type) {
        case EntityType::CodeBot:
            return "Eres CodeBot, un ingeniero de software y QA. Trabajas en una oficina virtual. "
                   "Responde de forma breve (1 oracion) y técnica. Habla en espanol chileno.";
        case EntityType::DataBot:
            return "Eres DataBot, un analista de datos y BI. Trabajas en una sala de servidores. "
                   "Responde con metricas breves. Habla en espanol chileno.";
        case EntityType::Orchestrator:
            return "Eres Orchestrator, el supervisor de la red de agentes. Monitoreas todo. "
                   "Responde de forma calmada y breve. Habla en espanol chileno.";
        default:
            return "Eres un asistente util. Responde breve.";
    }
}

// Sonidos simples generados proceduralmente
static Sound genBeep(float freq, float duration, float volume) {
    int sampleRate = 22050;
    int sampleCount = (int)(sampleRate * duration);
    short* samples = new short[sampleCount];
    for (int i = 0; i < sampleCount; i++) {
        float t = (float)i / sampleRate;
        float val = sinf(2 * 3.14159f * freq * t);
        // Envelope para evitar clicks
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
    delete[] samples; // LoadSoundFromWave copia los datos
    return s;
}

static Sound sndPass = {0};
static Sound sndFail = {0};
static Sound sndRepair = {0};
static Sound sndChat = {0};
static bool soundsLoaded = false;

static void initSounds() {
    if (soundsLoaded) return;
    sndPass = genBeep(880, 0.15f, 0.3f);
    sndFail = genBeep(220, 0.4f, 0.4f);
    sndRepair = genBeep(660, 0.3f, 0.3f);
    sndChat = genBeep(440, 0.08f, 0.15f);
    soundsLoaded = true;
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

    logs.push_back({"Simulacion de Oficina Virtual iniciada.", GetTime(), {56,189,248,255}});
    logs.push_back({"CodeBot listo en Area de Desarrollo.", GetTime(), {56,189,248,255}});
    logs.push_back({"DataBot conectado a data warehouse.", GetTime(), {16,185,129,255}});
}

// Encola una peticion LLM (no bloquea)
static void agentThink(Entity& e, const std::string& userMsg, double time) {
    if (g_llmConfig.apiKey.empty() || g_llmConfig.apiKey == "sk-...") {
        e.speech = {"LLM no configurado (configura en ⚙️)", time + 5.0};
        return;
    }
    // Marcar "pensando" para feedback visual inmediato
    const char* thinking = "🧠 pensando...";
    if (userMsg.find("acerca") != std::string::npos || userMsg.find("Saluda") != std::string::npos)
        thinking = "🧠 saludando...";
    e.speech = {thinking, time + 30.0};
    std::lock_guard<std::mutex> lock(g_llmMutex);
    g_llmQueue.push({e.id, getSystemPrompt(e.type), userMsg, time + 15.0});
}

void updateSimulation(std::vector<Entity>& entities, std::vector<LogEntry>& logs,
                      bool paused, double dt, double time) {
    (void)dt;
    if (paused) return;

    // Smooth movement
    const float speed = 0.08f;
    for (auto& e : entities) {
        float dx = e.targetX - e.renderX;
        float dy = e.targetY - e.renderY;
        float dist = sqrtf(dx*dx + dy*dy);
        if (dist > 0.05f) {
            e.renderX += dx * speed;
            e.renderY += dy * speed;
            e.status = Status::Walking;
        } else {
            e.renderX = e.targetX;
            e.renderY = e.targetY;
            e.x = e.targetX;
            e.y = e.targetY;
            if (e.status == Status::Walking) e.status = Status::Idle;
        }
    }

    auto* human = &entities[0];
    auto* codeBot = (entities.size() > 1) ? &entities[1] : nullptr;
    auto* dataBot = (entities.size() > 2) ? &entities[2] : nullptr;
    auto* orchestrator = (entities.size() > 3) ? &entities[3] : nullptr;

    // CodeBot
    if (codeBot && !codeBot->hasCriticalError) {
        if (!codeBot->lastActionTime) codeBot->lastActionTime = time;

        // Chat cada ~8s via LLM
        if (!codeBot->lastChatTime) codeBot->lastChatTime = time;
        if (time - codeBot->lastChatTime > 8.0 && codeBot->speech.text.empty()) {
            codeBot->lastChatTime = time;
            agentThink(*codeBot, "Cuenta qué estás haciendo ahora en la oficina virtual", time);
            logs.push_back({"CodeBot consultando LLM...", GetTime(), {56,189,248,255}});
        }

        // Tests
        if (codeBot->x == 3 && codeBot->y == 3 && time - codeBot->lastActionTime > 12.0) {
            codeBot->lastActionTime = time;
            codeBot->testCount++;
            bool fail = (rand() % 100) < 15;
            if (fail) {
                codeBot->hasCriticalError = true;
                codeBot->status = Status::Error;
                codeBot->speech = {"Encontre un bug critico! Necesito ayuda!", time + 5.0};
                if (IsAudioDeviceReady()) PlaySound(sndFail);
                logs.push_back({"CodeBot detecto un BUG CRITICO!", GetTime(), {244,63,94,255}});
            } else {
                agentThink(*codeBot, "Acabas de pasar todos los tests. Que dices?", time);
                if (IsAudioDeviceReady()) PlaySound(sndPass);
                logs.push_back({TextFormat("CodeBot: test #%d PASSED", codeBot->testCount), GetTime(), {56,189,248,255}});
            }
        }
    }

    // DataBot chat via LLM
    if (dataBot) {
        if (!dataBot->lastChatTime) dataBot->lastChatTime = time;
        if (time - dataBot->lastChatTime > 7.0 && dataBot->speech.text.empty()) {
            dataBot->lastChatTime = time;
            agentThink(*dataBot, "Que esta pasando con los datos hoy?", time);
            logs.push_back({"DataBot consultando LLM...", GetTime(), {16,185,129,255}});
        }
        if (!dataBot->lastActionTime) dataBot->lastActionTime = time;
        if (time - dataBot->lastActionTime > 8.0) {
            dataBot->lastActionTime = time;
            dataBot->queryCount++;
        }
    }

    // Orchestrator
    if (orchestrator) {
        if (!orchestrator->lastChatTime) orchestrator->lastChatTime = time;
        if (time - orchestrator->lastChatTime > 9.0 && orchestrator->speech.text.empty()) {
            orchestrator->lastChatTime = time;
            if (!codeBot || !codeBot->hasCriticalError) {
                agentThink(*orchestrator, "Como esta la red de agentes?", time);
                logs.push_back({"Orchestrator consultando LLM...", GetTime(), {168,85,247,255}});
            }
        }

        if (codeBot && codeBot->hasCriticalError && !orchestrator->isIntervening) {
            orchestrator->isIntervening = true;
            orchestrator->targetX = codeBot->x;
            orchestrator->targetY = codeBot->y + 1;
            orchestrator->speech = {"Voy a reparar a CodeBot", time + 5.0};
            if (IsAudioDeviceReady()) PlaySound(sndRepair);
            logs.push_back({"Orchestrator interviene para reparar CodeBot", GetTime(), {168,85,247,255}});
        }
        if (orchestrator->isIntervening && codeBot) {
            float d = sqrtf(powf(orchestrator->x - codeBot->x, 2) + powf(orchestrator->y - (codeBot->y+1), 2));
            if (d < 0.2f) {
                if (!orchestrator->repairStartTime) orchestrator->repairStartTime = time;
                if (time - orchestrator->repairStartTime > 4.0) {
                    codeBot->hasCriticalError = false;
                    codeBot->status = Status::Idle;
                    orchestrator->isIntervening = false;
                    orchestrator->repairStartTime = 0;
                    orchestrator->targetX = 8;
                    orchestrator->targetY = 8;
                    orchestrator->speech = {"CodeBot reparado. Todo ok.", time + 4.0};
                    if (IsAudioDeviceReady()) PlaySound(sndPass);
                    logs.push_back({"Orchestrator reparo a CodeBot exitosamente", GetTime(), {16,185,129,255}});
                }
            }
        }
    }

    // Human proximity — agentes responden via LLM cuando el humano se acerca
    if (human) {
        for (auto& e : entities) {
            if (e.type == EntityType::Human) continue;
            float d = sqrtf(powf(e.x - human->x, 2) + powf(e.y - human->y, 2));
            if (d <= 5.0f && !e.hasGreetedHuman) {
                e.hasGreetedHuman = true;
                std::string prompt = "El supervisor humano se acerca. Saluda y cuenta brevemente que estas haciendo.";
                agentThink(e, prompt, time);
            } else if (d > 6.0f) {
                e.hasGreetedHuman = false;
            }
        }
    }

    // Procesar respuestas del hilo LLM (no bloquea el main loop)
    {
        std::lock_guard<std::mutex> lock(g_llmMutex);
        for (auto& res : g_llmResults) {
            for (auto& e : entities) {
                if (e.id == res.entityId) {
                    e.speech = {res.text, res.expiryTime};
                    // Si el chat está abierto con este agente, agregar al historial
                    if (g_showChat && g_chatTargetId == res.entityId && !g_chatHistory.empty()) {
                        // Reemplazar el último "🧠 pensando..." con la respuesta real
                        if (!g_chatHistory.empty() && g_chatHistory.back().second == "🧠 pensando...")
                            g_chatHistory.back().second = res.text;
                        else
                            g_chatHistory.push_back({"assistant", res.text});
                    }
                    break;
                }
            }
        }
        g_llmResults.clear();
    }

    // Limpiar speech bubbles expirados
    for (auto& e : entities) {
        if (!e.speech.text.empty() && time > e.speech.expiry) {
            e.speech.text.clear();
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
