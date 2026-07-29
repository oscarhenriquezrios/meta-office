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

// Tamaño dinámico de pantalla (se actualiza al redimensionar)
extern int screenW;
extern int screenH;

enum class EntityType { Human, CodeBot, DataBot, Orchestrator };
enum class Status { Idle, Walking, Busy, Error, Intervening };

struct SpeechBubble {
    std::string text;
    double expiry = 0;
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
};

struct LogEntry {
    std::string text;
    double time;
    Color color;
};

// Forward declarations
Vector2 gridToIso(float gx, float gy, Vector2 origin);
Vector2 isoToGrid(float sx, float sy, Vector2 origin);

// Helper
inline Color alpha(Color c, int a) { return {c.r, c.g, c.b, (unsigned char)a}; }

// Config LLM global
extern LLMConfig g_llmConfig;

// Menu de config LLM
extern bool g_showLlmConfig;
void saveLlmConfig();

// Cola de LLM async
struct LlmRequest {
    int entityId;          // -1 si no es para una entidad
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
void drawUI(const std::vector<Entity>& entities, const std::vector<LogEntry>& logs,
            int selectedId, bool paused);
void handleInput(std::vector<Entity>& entities, Vector2& origin,
                 int& selectedId, bool& paused, float& panY);
