#pragma once
#include <raylib.h>
#include <string>
#include <vector>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include "llm_client.hpp"

// Config
constexpr int SCREEN_W_DEF = 1280;
constexpr int SCREEN_H_DEF = 720;
constexpr int TILE_W = 64;
constexpr int TILE_H = 32;
constexpr int GRID_W = 16;
constexpr int GRID_H = 16;

extern int screenW;
extern int screenH;

enum class EntityType { Human, CodeBot, DataBot, Orchestrator };
enum class Status { Idle, Walking, Busy, Error, Intervening };

struct SpeechBubble {
    std::string text;
    double expiry = 0;
};

struct LogEntry {
    std::string text;
    double time;
    Color color;
};

// ============================================================
// Sistema de Tareas
// ============================================================
enum class TaskStatus { Pending, InProgress, Done, Failed };

struct Task {
    int id = 0;
    std::string description;
    int assignedTo = -1;       // entity ID
    TaskStatus status = TaskStatus::Pending;
    std::string result;
    double createdAt = 0;
    double completedAt = 0;
    std::vector<std::string> steps;  // log de pasos ejecutados
    // Multi-turn LLM context
    std::vector<LLMMessage> context;
    bool needsProcessing = false;
    int turnCount = 0;
};

// ============================================================
// Memoria persistente por agente
// ============================================================
struct AgentMemory {
    int entityId = 0;
    std::vector<std::string> facts;       // cosas que aprendió
    std::vector<std::string> pastTasks;   // descripciones de tareas completadas
    std::vector<std::string> conversations; // resúmenes de charlas
};

struct Entity {
    int id = 0;
    std::string name;
    std::string role;
    EntityType type = EntityType::Human;
    Color color = WHITE;
    float x = 0, y = 0;
    float targetX = 0, targetY = 0;
    float renderX = 0, renderY = 0;
    Status status = Status::Idle;
    bool hasCriticalError = false;
    double lastActionTime = 0;
    double lastChatTime = 0;
    SpeechBubble speech;
    int testCount = 0, queryCount = 0;
    bool hasGreetedHuman = false;
    bool isIntervening = false;
    double repairStartTime = 0;
    std::vector<LogEntry> agentLog;
    bool isActive = false;
    std::string currentTask;
    int currentTaskId = -1;
    AgentMemory memory;
};

// Forward declarations
Vector2 gridToIso(float gx, float gy, Vector2 origin);
Vector2 isoToGrid(float sx, float sy, Vector2 origin);

inline Color alpha(Color c, int a) { return {c.r, c.g, c.b, (unsigned char)a}; }

// Config LLM global
extern LLMConfig g_llmConfig;
extern bool g_showLlmConfig;
void saveLlmConfig();

// Cola de LLM async
struct LlmRequest {
    int entityId;
    std::string systemPrompt;
    std::string userMessage;
    double expiryTime;
};
struct LlmResponse {
    int entityId;
    std::string text;
    double expiryTime;
};
extern std::queue<LlmRequest> g_llmQueue;
extern std::vector<LlmResponse> g_llmResults;
extern std::mutex g_llmMutex;
extern std::atomic<bool> g_llmThreadRunning;
void startLlmThread();
void stopLlmThread();

const char* getSystemPrompt(EntityType type);

void initSimulation(std::vector<Entity>& entities, std::vector<LogEntry>& logs);
void updateSimulation(std::vector<Entity>& entities, std::vector<LogEntry>& logs,
                      bool paused, double dt, double time);

void drawScene(const std::vector<Entity>& entities, Vector2 origin, double time);
void unloadTextures();
void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused);
void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY);

// Chat
extern bool g_showChat;
extern int g_chatTargetId;
extern std::vector<std::pair<std::string, std::string>> g_chatHistory;
extern std::string g_chatInput;
void openChat(int entityId);
void closeChat();

// Log panel
extern bool g_showLog;
extern int g_logTargetId;
void openLogPanel(int entityId);
void closeLogPanel();

// ============================================================
// Tareas — globals y funciones
// ============================================================
extern std::vector<Task> g_tasks;
extern int g_nextTaskId;
extern bool g_showTaskPanel;
extern int g_taskPanelTargetId; // -1 = panel general, >0 = asignar a este agente
void openTaskPanel(int entityId);
void closeTaskPanel();

// Web fetch (curl)
std::string webFetch(const std::string& url);

// Procesar una tarea (multi-turn con tools)
void processTaskStep(Task& task, Entity& agent, std::vector<LogEntry>& logs);

// Memoria persistente
void saveMemory(const std::vector<Entity>& entities);
void loadMemory(std::vector<Entity>& entities);
void saveTasks();
void loadTasks();
