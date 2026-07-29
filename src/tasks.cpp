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

// Globals de entregables
std::vector<Deliverable> g_deliverables;
int g_nextDeliverableId = 1;
bool g_showDeliverables = false;
int g_viewDeliverableId = -1;

// ============================================================
// Web Fetch
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
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MetaOfficeBot/1.0");
    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    curl_easy_cleanup(curl);
    if (res != CURLE_OK) return "Error de red: " + std::string(curl_easy_strerror(res));
    if (httpCode != 200) return "HTTP " + std::to_string(httpCode);

    std::string clean;
    bool inTag = false;
    int count = 0;
    for (char c : response) {
        if (c == '<') { inTag = true; continue; }
        if (c == '>') { inTag = false; continue; }
        if (!inTag) {
            if (c == '\n' || c == '\r' || c == '\t') {
                if (!clean.empty() && clean.back() != ' ') clean += ' ';
            } else { clean += c; }
            count++;
            if (count > 4000) break;
        }
    }
    std::string result;
    bool lastSpace = false;
    for (char c : clean) {
        if (c == ' ') { if (!lastSpace) result += ' '; lastSpace = true; }
        else { result += c; lastSpace = false; }
    }
    return result.empty() ? "Pagina vacia" : result;
}

// ============================================================
// Extraer URL de una respuesta del LLM
// ============================================================
static std::string extractUrl(const std::string& text) {
    std::string lower = text;
    for (auto& c : lower) c = tolower(c);
    size_t pos = std::string::npos;
    const char* markers[] = {"fetch(", "navegar(", "url:", "visita "};
    for (auto m : markers) {
        size_t p = lower.find(m);
        if (p != std::string::npos) { pos = p + strlen(m); break; }
    }
    if (pos == std::string::npos) return "";
    std::string url;
    while (pos < text.size() && text[pos] != ')' && text[pos] != '\n' && text[pos] != ' ')
        url += text[pos++];
    while (!url.empty() && (url.front() == '(' || url.front() == '"' || url.front() == ' '))
        url.erase(url.begin());
    while (!url.empty() && (url.back() == ')' || url.back() == '"' || url.back() == ' '))
        url.pop_back();
    if (url.find("http") != 0) return "";
    return url;
}

static bool isTaskComplete(const std::string& text) {
    std::string lower = text;
    for (auto& c : lower) c = tolower(c);
    return lower.find("resultado:") != std::string::npos ||
           lower.find("listo") != std::string::npos;
}

// ============================================================
// Extraer entregable de la respuesta del LLM
// Formato: ENTREGABLE: [titulo]\n```lenguaje\n[contenido]\n```
// ============================================================
static bool extractDeliverable(const std::string& text, std::string& title,
                               std::string& content, std::string& lang) {
    size_t pos = text.find("ENTREGABLE:");
    if (pos == std::string::npos) return false;

    // Extraer titulo (hasta el salto de linea)
    size_t titleStart = pos + 11;
    size_t titleEnd = text.find('\n', titleStart);
    if (titleEnd == std::string::npos) return false;
    title = text.substr(titleStart, titleEnd - titleStart);
    // Limpiar titulo
    while (!title.empty() && (title.front() == ' ' || title.front() == '['))
        title.erase(title.begin());
    while (!title.empty() && (title.back() == ' ' || title.back() == ']'))
        title.pop_back();
    if (title.empty()) title = "Entregable";

    // Buscar ``` despues del titulo
    size_t codeStart = text.find("```", titleEnd);
    if (codeStart == std::string::npos) return false;

    // Extraer lenguaje (entre ``` y el salto de linea)
    size_t langStart = codeStart + 3;
    size_t langEnd = text.find('\n', langStart);
    if (langEnd == std::string::npos) return false;
    lang = text.substr(langStart, langEnd - langStart);
    // Limpiar lenguaje
    while (!lang.empty() && lang.front() == ' ') lang.erase(lang.begin());
    if (lang.empty()) lang = "texto";

    // Extraer contenido (hasta el cierre ```)
    size_t contentStart = langEnd + 1;
    size_t contentEnd = text.find("```", contentStart);
    if (contentEnd == std::string::npos) return false;
    content = text.substr(contentStart, contentEnd - contentStart);

    return !content.empty();
}

// ============================================================
// Determinar tipo de entregable segun el lenguaje
// ============================================================
static DeliverableType detectDeliverableType(const std::string& lang) {
    if (lang == "python" || lang == "py") return DeliverableType::Code;
    if (lang == "cpp" || lang == "c" || lang == "c++") return DeliverableType::Code;
    if (lang == "javascript" || lang == "js" || lang == "ts") return DeliverableType::Code;
    if (lang == "bash" || lang == "sh" || lang == "shell") return DeliverableType::Code;
    if (lang == "sql") return DeliverableType::Data;
    if (lang == "json" || lang == "csv") return DeliverableType::Data;
    if (lang == "html") return DeliverableType::WebContent;
    return DeliverableType::Report;
}

// ============================================================
// Crear y guardar entregable
// ============================================================
int createDeliverable(const std::string& title, const std::string& content,
                      DeliverableType type, int taskId, int agentId,
                      const std::string& agentName) {
    system("mkdir -p deliverables");

    Deliverable d;
    d.id = g_nextDeliverableId++;
    d.title = title;
    d.content = content;
    d.type = type;
    d.taskId = taskId;
    d.agentId = agentId;
    d.agentName = agentName;
    d.createdAt = GetTime();

    // Extension segun tipo
    const char* ext = ".txt";
    switch (type) {
        case DeliverableType::Code: ext = ".py"; break;
        case DeliverableType::Report: ext = ".md"; break;
        case DeliverableType::Data: ext = ".csv"; break;
        case DeliverableType::Text: ext = ".txt"; break;
        case DeliverableType::WebContent: ext = ".html"; break;
    }

    // Nombre de archivo seguro
    std::string safeTitle;
    for (char c : title) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_')
            safeTitle += c;
        else safeTitle += '_';
    }
    if (safeTitle.empty()) safeTitle = "entregable";
    d.filename = "deliverables/" + safeTitle + "_" + std::to_string(d.id) + ext;

    // Preview (primeros 200 chars)
    d.preview = content.substr(0, std::min((size_t)200, content.size()));
    if (content.size() > 200) d.preview += "...";

    // Guardar archivo
    saveDeliverable(d);

    g_deliverables.push_back(d);
    return d.id;
}

void saveDeliverable(const Deliverable& d) {
    std::ofstream f(d.filename);
    if (!f.is_open()) return;
    f << d.content;
    f.close();
}

void loadDeliverables() {
    // Cargar el indice de entregables desde deliverables/index.txt
    std::ifstream f("deliverables/index.txt");
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.substr(0, 4) != "DEL|") continue;
        Deliverable d;
        size_t pos = 4;
        auto next = [&]() {
            size_t p = line.find('|', pos);
            std::string s = line.substr(pos, p - pos);
            pos = p + 1;
            return s;
        };
        d.id = std::atoi(next().c_str());
        d.title = next();
        d.filename = next();
        d.type = (DeliverableType)std::atoi(next().c_str());
        d.taskId = std::atoi(next().c_str());
        d.agentId = std::atoi(next().c_str());
        d.agentName = next();
        // Cargar contenido del archivo
        std::ifstream cf(d.filename);
        if (cf.is_open()) {
            std::string content((std::istreambuf_iterator<char>(cf)),
                                 std::istreambuf_iterator<char>());
            d.content = content;
            d.preview = content.substr(0, std::min((size_t)200, content.size()));
            if (content.size() > 200) d.preview += "...";
        }
        g_deliverables.push_back(d);
        if (d.id >= g_nextDeliverableId) g_nextDeliverableId = d.id + 1;
    }
}

static void saveDeliverableIndex() {
    system("mkdir -p deliverables");
    std::ofstream f("deliverables/index.txt");
    if (!f.is_open()) return;
    for (auto& d : g_deliverables) {
        f << "DEL|" << d.id << "|" << d.title << "|" << d.filename << "|"
          << (int)d.type << "|" << d.taskId << "|" << d.agentId << "|" << d.agentName << "\n";
    }
}

const char* deliverableTypeName(DeliverableType t) {
    switch (t) {
        case DeliverableType::Code: return "CODIGO";
        case DeliverableType::Report: return "INFORME";
        case DeliverableType::Data: return "DATOS";
        case DeliverableType::Text: return "TEXTO";
        case DeliverableType::WebContent: return "WEB";
    }
    return "?";
}

Color deliverableTypeColor(DeliverableType t) {
    switch (t) {
        case DeliverableType::Code: return {56,189,248,255};
        case DeliverableType::Report: return {168,85,247,255};
        case DeliverableType::Data: return {16,185,129,255};
        case DeliverableType::Text: return {148,163,184,255};
        case DeliverableType::WebContent: return {245,158,11,255};
    }
    return WHITE;
}

void openDeliverablesPanel() { g_showDeliverables = true; g_viewDeliverableId = -1; }
void closeDeliverablesPanel() { g_showDeliverables = false; g_viewDeliverableId = -1; }

// ============================================================
// Procesar un step de una tarea
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

    // Inicializar contexto en el primer turn
    if (task.context.empty()) {
        task.context.push_back({"system", getTaskSystemPrompt(agent.type)});
        task.context.push_back({"user", task.description});
        task.steps.push_back("Tarea iniciada: " + task.description);
        agent.agentLog.push_back({"Tarea iniciada: " + task.description, now, agent.color});
    }

    // Llamar al LLM
    LLMConfig cfg = g_llmConfig;
    cfg.maxTokens = 400;
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
        if (content.size() > 2000) content = content.substr(0, 2000) + "... (truncado)";
        task.context.push_back({"user", "Contenido de " + url + ":\n" + content +
            "\n\nAnaliza esto y continua. Si necesitas otra URL, usa fetch(). Si terminaste, escribe RESULTADO:"});
        agent.speech = {"Navegando...", now + 10.0};
        return;
    }

    // ¿Terminó?
    if (isTaskComplete(reply) || task.turnCount >= 5) {
        task.status = TaskStatus::Done;
        task.completedAt = now;

        // ¿Tiene entregable?
        std::string delTitle, delContent, delLang;
        if (extractDeliverable(reply, delTitle, delContent, delLang)) {
            DeliverableType dtype = detectDeliverableType(delLang);
            int delId = createDeliverable(delTitle, delContent, dtype, task.id, agent.id, agent.name);
            task.deliverableId = delId;
            task.result = "Entregable creado: " + delTitle;
            task.steps.push_back("Entregable: " + delTitle + " (" + std::string(deliverableTypeName(dtype)) + ")");
            agent.agentLog.push_back({"Entregable: " + delTitle, now, {16,185,129,255}});
            logs.push_back({TextFormat("%s entrego: %s", agent.name.c_str(), delTitle.c_str()),
                           GetTime(), {16,185,129,255}});
            saveDeliverableIndex();
        } else {
            // Sin entregable, extraer resultado
            size_t rp = reply.find("RESULTADO:");
            if (rp != std::string::npos)
                task.result = reply.substr(rp + 9);
            else
                task.result = reply;
        }

        if (task.result.size() > 300) task.result = task.result.substr(0, 297) + "...";

        task.steps.push_back("Tarea completada");
        agent.agentLog.push_back({"Tarea completada: " + task.result.substr(0, 60), now, {16,185,129,255}});
        logs.push_back({TextFormat("%s completo tarea #%d", agent.name.c_str(), task.id), GetTime(), {16,185,129,255}});

        agent.isActive = false;
        agent.currentTask.clear();
        agent.currentTaskId = -1;
        agent.status = Status::Idle;
        agent.speech = {"Tarea completada!", now + 5.0};
        agent.memory.pastTasks.push_back(task.description + " -> " + task.result);

        saveTasks();
        saveMemory({agent});
    } else {
        task.context.push_back({"user", "Continua trabajando en la tarea."});
        agent.speech = {"Trabajando... (turno " + std::to_string(task.turnCount) + ")", now + 8.0};
    }
}

// ============================================================
// Persistencia — Memoria
// ============================================================
static std::string memDir() { return "memory/"; }

void saveMemory(const std::vector<Entity>& entities) {
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
    }
}

void loadMemory(std::vector<Entity>& entities) {
    for (auto& e : entities) {
        if (e.type == EntityType::Human) continue;
        std::string fn = memDir() + "agent_" + std::to_string(e.id) + ".mem";
        std::ifstream f(fn);
        if (!f.is_open()) continue;
        std::string line, section;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            if (line[0] == '[') { section = line; continue; }
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
          << t.description << "|" << t.result << "|" << t.deliverableId << "\n";
    }
}

void loadTasks() {
    std::ifstream f("memory/tasks.log");
    if (!f.is_open()) return;
    std::string line;
    while (std::getline(f, line)) {
        if (line.substr(0, 5) != "TASK|") continue;
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
        t.result = next();
        t.deliverableId = std::atoi(line.substr(pos).c_str());
        g_tasks.push_back(t);
        if (t.id >= g_nextTaskId) g_nextTaskId = t.id + 1;
    }
}

void openTaskPanel(int entityId) { g_taskPanelTargetId = entityId; g_showTaskPanel = true; }
void closeTaskPanel() { g_showTaskPanel = false; g_taskPanelTargetId = -1; }
