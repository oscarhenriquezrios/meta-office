#pragma once
#include <string>
#include <vector>
#include <functional>

struct LLMConfig {
    std::string endpoint = "https://api.openai.com/v1/chat/completions";
    std::string apiKey = "sk-...";
    std::string model = "gpt-4o-mini";
    float temperature = 0.7f;
    int maxTokens = 100;
};

struct LLMMessage {
    std::string role; // "system", "user", "assistant"
    std::string content;
};

// Inicializar desde variables de entorno
LLMConfig llmConfigFromEnv();

// Llamada síncrona a la API. Retorna el texto de respuesta o vacío en error.
std::string llmChat(const LLMConfig& cfg, const std::vector<LLMMessage>& messages);
