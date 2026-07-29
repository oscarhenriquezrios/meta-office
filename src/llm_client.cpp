#include "llm_client.hpp"
#include <cstdlib>
#include <cstring>
#include <curl/curl.h>

// Callback para escribir la respuesta de curl
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

LLMConfig llmConfigFromEnv() {
    LLMConfig cfg;
    const char* key = getenv("OPENAI_API_KEY");
    if (key) cfg.apiKey = key;

    const char* ep = getenv("LLM_ENDPOINT");
    if (ep) cfg.endpoint = ep;

    const char* model = getenv("LLM_MODEL");
    if (model) cfg.model = model;

    const char* temp = getenv("LLM_TEMPERATURE");
    if (temp) cfg.temperature = std::atof(temp);

    return cfg;
}

std::string llmChat(const LLMConfig& cfg, const std::vector<LLMMessage>& messages) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";

    // Construir JSON del body
    std::string body = R"({"model":")" + cfg.model + R"(","temperature":)" +
                       std::to_string(cfg.temperature) + R"(,"max_tokens":)" +
                       std::to_string(cfg.maxTokens) + R"(,"messages":[)";

    for (size_t i = 0; i < messages.size(); i++) {
        if (i > 0) body += ",";
        body += R"({"role":")" + messages[i].role + R"(","content":")";
        // Escapar caracteres especiales
        for (char c : messages[i].content) {
            if (c == '"') body += "\\\"";
            else if (c == '\\') body += "\\\\";
            else if (c == '\n') body += "\\n";
            else if (c == '\t') body += "\\t";
            else body += c;
        }
        body += R"("})";
    }
    body += R"(]})";

    std::string response;
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + cfg.apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, cfg.endpoint.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "";

    // Parsear respuesta JSON para extraer "content"
    // Buscamos: "content":"(.*?)"
    auto pos = response.find("\"content\":\"");
    if (pos == std::string::npos) return "";

    pos += 11; // saltar "content":" 
    std::string result;
    bool escape = false;
    for (; pos < response.size(); pos++) {
        char c = response[pos];
        if (escape) {
            if (c == 'n') result += '\n';
            else if (c == 't') result += '\t';
            else if (c == '\\') result += '\\';
            else if (c == '"') result += '"';
            else result += c;
            escape = false;
        } else if (c == '\\') {
            escape = true;
        } else if (c == '"') {
            break;
        } else {
            result += c;
        }
    }

    return result;
}
