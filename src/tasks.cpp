#include "meta_office.hpp"
#include <fstream>
#include <chrono>
#include <curl/curl.h>
#include <cstdlib>

// Globals de tareas
std::vector<Task> g_tasks;
int g_nextTaskId = 1;
bool g_showTaskPanel = false;
int g_taskPanelTargetId = -1;

// ============================================================
// Web Fetch — descargar URL con curl
// ============================================================
static size_t writeCb(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string webFetch(const std::string& url) {
    CURL* curl = curl_easy_init();
    if (!curl) return "Error: no se pudo iniciar curl";

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    // User agent
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MetaOfficeBot/1.0");

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) return "Error de red: " + std::string(curl_easy_strerror(res));
    if (httpCode != 200) return "HTTP " + std::to_string(httpCode);

    // Limpiar HTML básico: quitar tags
    std::string clean;
    bool inTag = false;
    int count = 0;
    for (char c : response) {
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; continue; }
        if (!inTag) {
            // Decodificar entidades básicas
            if (c == '\n' || c == '\r' || c == '\t') {
                if (!clean.empty() && clean.back() != ' ') clean += ' ';
            } else {
                clean += c;
            }
            count++;
            if (count > 4000) break; // limitar a 4KB de texto
        }
    }
    // Colapsar espacios múltiples
    std::string result;
    bool lastSpace = false;
    for (char c : clean) {
        if (c == ' ') {
            if (!lastSpace) result += ' ';
            lastSpace = true;
        } else {
            result += c;
            lastSpace = false;
        }
    }
    return result.empty() ? "Pagina vacia" : result;
}

// ============================================================
// Extraer URL de una respuesta del LLM
// Busca patrones como: fetch(https://...) o navegar(https://...)
// ============================================================
static std::string extractUrl(const std::string& text) {
    // Buscar "fetch(" o "navegar(" o "url:"
    std::string lower = text;
    for (auto& c : lower) c = tolower(c);

    size_t pos = std::string::npos;
    const char* markers[] = {"fetch(", "navegar(", "url:", "buscar ", "visita "};
    for (auto m : markers) {
        size_t p = lower.find(m);
        if (p != std::string::npos) { pos = p + strlen(m); break; }
    }
    if (pos == std::string::npos) return "";

    // Extraer URL
    std::string url;
    while (pos < text.size() && text[pos] != ')' && text[pos] != '\n' && text[pos] != ' ') {
        url += text[pos++];
    }
    // Quitar paréntesis o comillas
    while (!url.empty() && (url.front() == '(' || url.front() == '"' || url.front() == ' '))
        url.erase(url.begin());
    while (!url.empty() && (url.back() == ')' || url.back() == '"' || url.back() == ' '))
        url.pop_back();

    // Asegurar que es una URL válida
    if (url.find("http") != 0) return "";
    return url;
}

// ============================================================
// Extraer si el LLM dice que terminó
// ============================================================
static bool isTaskComplete(const std::string& text) {
    std::string lower = text;
    for (auto& c : lower) c = tolower(c);
    return lower.find("listo") != std::string::npos ||
           lower.find("terminad") != std::string::npos ||
           lower.find("completad") != std::string::npos ||
           lower.find("resultado:") != std::string::npos ||
           lower.find("fin.") != std::string::npos;
}

// ============================================================
// Procesar un step de una tarea (multi-turn con tool use)
// ============================================================
void processTaskStep(Task& task, Entity& agent, std::vector<LogEntry>& logs) {
    if (task.status != TaskStatus::InProgress) return;
    if (g_llmConfig.apiKey.empty() || g_llmConfig.apiKey == "sk-...") {
        task.status = TaskStatus::Failed;
        task.result = "LLM no configurado";
        return;
    }

    double now = std::chrono::duration<double>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Si es el primer turn, inicializar contexto
    if (task.context.empty()) {
        task.context.push_back({"system", getSystemPrompt(agent.type)});
        task.context.push_back({"system",
            "Te asignaron esta tarea: " + task.description +
            "\n\nPuedes navegar internet usando el formato: fetch(https://url.com)"
            "\nCuando termines, escribe 'RESULTADO:' seguido de tu respuesta final."
            "\nResponde en espanol chileno, maximo 3 oraciones por turno."});
        task.context.push_back({"user", task.description});
        task.steps.push_back("Tarea iniciada: " + task.description);
        agent.agentLog.push_back({"Tarea iniciada: " + task.description, now, agent.color});
    }

    // Llamar al LLM
    LLMConfig cfg = g_llmConfig;
    cfg.maxTokens = 200;  // más espacio para razonar
    std::string reply = llmChat(cfg, task.context);
    if (reply.empty()) {
        task.status = TaskStatus::Failed;
        task.result = "Error llamando al LLM";
        task.steps.push_back("Error: LLM no respondio");
        agent.agentLog.push_back({"Error: LLM no respondio", now, {244,63,94,255}});
        return;
    }

    task.context.push_back({"assistant", reply});
    task.turnCount++;

    // ¿Pidió navegar a una URL?
    std::string url = extractUrl(reply);
    if (!url.empty()) {
        task.steps.push_back("Navegando: " + url);
        agent.agentLog.push_back({"Navegando: " + url, now, {56,189,248,255}});
        logs.push_back({TextFormat("%s navega: %s", agent.name.c_str(), url.c_str()), GetTime(), agent.color});

        std::string content = webFetch(url);
        // Truncar para no saturar el contexto
        if (content.size() > 2000) content = content.substr(0, 2000) + "... (truncado)";
        task.context.push_back({"user", "Contenido de " + url + ":\n" + content +
            "\n\nAnaliza esto y continua. Si necesitas otra URL, usa fetch(). Si terminaste, escribe RESULTADO:"});

        agent.speech = {"Navegando...", now + 10.0};
        return; // otro turno
    }

    // ¿Terminó?
    if (isTaskComplete(reply) || task.turnCount >= 5) {
        task.status = TaskStatus::Done;
        task.completedAt = now;
        // Extraer resultado (después de "RESULTADO:" o usar toda la respuesta)
        size_t rp = reply.find("RESULTADO:");
        if (rp != std::string::npos)
            task.result = reply.substr(rp + 9);
        else
            task.result = reply;

        if (task.result.size() > 300) task.result = task.result.substr(0, 297) + "...";

        task.steps.push_back("Tarea completada");
        agent.agentLog.push_back({"Tarea completada: " + task.result.substr(0, 60), now, {16,185,129,255}});
        logs.push_back({TextFormat("%s completo tarea #%d", agent.name.c_str(), task.id), GetTime(), {16,185,129,255}});

        agent.isActive = false;
        agent.currentTask.clear();
        agent.currentTaskId = -1;
        agent.status = Status::Idle;
        agent.speech = {"Tarea completada!", now + 5.0};

        // Guardar en memoria del agente
        agent.memory.pastTasks.push_back(task.description + " -> " + task.result);

        // Persistir
        saveTasks();
        saveMemory({agent}); // guarda solo este agente (simplificación)
    } else {
        // Continuar otro turno
        task.context.push_back({"user", "Continua trabajando en la tarea."});
        agent.speech = {"Trabajando... (turno " + std::to_string(task.turnCount) + ")", now + 8.0};
    }
}

// ============================================================
// Persistencia — Memoria
// ============================================================
static std::string memDir() {
    return "memory/";
}

void saveMemory(const std::vector<Entity>& entities) {
    // Crear directorio
    system("mkdir -p memory");

    for (auto& e : entities) {
        if (e.type == EntityType::Human) continue;
        std::string fn = memDir() + "agent_" + std::to_string(e.id) + ".mem";
        std::ofstream f(fn);
        if (!f.is_open()) continue;

        f << "# Memoria de " << e.name << "\n\n";

        f << "[FACTS]\n";
        for (auto& fact : e.memory.facts) f << fact << "\n";

        f << "\n[PAST_TASKS]\n";
        for (auto& t : e.memory.pastTasks) f << t << "\n";

        f << "\n[CONVERSATIONS]\n";
        for (auto& c : e.memory.conversations) f << c << "\n";

        f.close();
    }
}

void loadMemory(std::vector<Entity>& entities) {
    for (auto& e : entities) {
        if (e.type == EntityType::Human) continue;
        std::string fn = memDir() + "agent_" + std::to_string(e.id) + ".mem";
        std::ifstream f(fn);
        if (!f.is_open()) continue;

        std::string line;
        std::string section;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line[0] == '[') {
                section = line;
                continue;
            }
            if (section == "[FACTS]") e.memory.facts.push_back(line);
            else if (section == "[PAST_TASKS]") e.memory.pastTasks.push_back(line);
            else if (section == "[CONVERSATIONS]") e.memory.conversations.push_back(line);
        }
    }
}

// ============================================================
// Persistencia — Tareas
// ============================================================
void saveTasks() {
    std::ofstream f("memory/tasks.log");
    if (!f.is_open()) return;
    for (auto& t : g_tasks) {
        f << "TASK|" << t.id << "|" << (int)t.status << "|" << t.assignedTo << "|"
          << t.description << "|" << t.result << "\n";
    }
}

void loadTasks() {
    std::ifstream f("memory/tasks.log");
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.substr(0, 5) != "TASK|") continue;
        // Parse simple
        Task t;
        size_t pos = 5;
        auto next = [&]() {
            size_t p = line.find('|', pos);
            std::string s = line.substr(pos, p - pos);
            pos = p + 1;
            return s;
        };
        t.id = std::atoi(next().c_str());
        t.status = (TaskStatus)std::atoi(next().c_str());
        t.assignedTo = std::atoi(next().c_str());
        t.description = next();
        t.result = line.substr(pos);
        g_tasks.push_back(t);
        if (t.id >= g_nextTaskId) g_nextTaskId = t.id + 1;
    }
}

// ============================================================
// Panel de tareas — abrir/cerrar
// ============================================================
void openTaskPanel(int entityId) {
    g_taskPanelTargetId = entityId;
    g_showTaskPanel = true;
}

void closeTaskPanel() {
    g_showTaskPanel = false;
    g_taskPanelTargetId = -1;
}
