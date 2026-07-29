# Meta-Office — Documentación Técnica

## Visión General

Meta-Office es una oficina virtual isométrica 2D construida en C++20 con raylib 5.5.
Simula un entorno de trabajo con agentes IA que conversan mediante OpenAI API (o cualquier
endpoint compatible), con mapa interactivo, mobiliario, sonido y chat en tiempo real.

## Arquitectura

```
meta-office/
├── Makefile                # Compilación (g++ -std=c++20, raylib estático)
├── README.md               # Instrucciones de instalación y uso
├── DOCUMENTATION.md        # Este archivo
├── install.sh              # Instalación completa desde GitHub
├── quick-install.sh        # Instalación rápida
├── .gitignore              # Excluye binarios, .o, llm_config.env, etc.
├── llm_config.env          # Config LLM local (gitignored)
└── src/
    ├── meta_office.hpp     # Header principal: structs, constantes, declaraciones
    ├── main.cpp            # Loop principal: init, input, update, draw, cleanup
    ├── render.cpp          # Render isométrico: texturas procedurales, mobiliario, avatares
    ├── agents.cpp          # Simulación: agentes, LLM async, sonidos, system prompts
    ├── ui.cpp              # UI: topbar, sidebar, panel config LLM, panel chat, input
    ├── llm_client.hpp      # Structs LLMConfig, LLMMessage
    ├── llm_client.cpp      # Cliente HTTP (libcurl) para OpenAI API
    └── network.cpp         # Stub para multi-usuario (WebSocket futuro)
```

## Compilación

### Dependencias

```bash
sudo apt install -y build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev libcurl4-openssl-dev
```

### Raylib 5.5

```bash
cd /tmp
wget https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
tar xzf 5.5.tar.gz && cd raylib-5.5
mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc) && sudo make install && sudo ldconfig
```

### Compilar Meta-Office

```bash
cd meta-office
make -j$(nproc)
```

### Output

- Binario: `./meta-office` (~5MB)
- Estático (raylib embebido), solo depende de libcurl del sistema

## Configuración LLM

### Desde la aplicación

1. Click en **⚙️** (topbar derecha)
2. Completar Endpoint, API Key, Modelo
3. **GUARDAR** → persiste en `llm_config.env`
4. **TEST** → prueba conexión y guarda si funciona

### Manual

Editar `llm_config.env`:

```ini
LLM_ENDPOINT=https://api.openai.com/v1/chat/completions
OPENAI_API_KEY=sk-...
LLM_MODEL=gpt-4o-mini
```

### Endpoints compatibles

| Proveedor  | Endpoint                                                    |
|------------|-------------------------------------------------------------|
| OpenAI     | `https://api.openai.com/v1/chat/completions`                |
| OpenRouter | `https://openrouter.ai/api/v1/chat/completions`             |
| DeepSeek   | `https://api.deepseek.com/v1/chat/completions`              |
| Ollama     | `http://localhost:11434/v1/chat/completions`                |

## Controles

| Tecla           | Acción                          |
|-----------------|---------------------------------|
| W/A/S/D o flechas | Mover avatar humano            |
| Clic en mapa    | Mover humano a esa posición     |
| Clic en agente  | Abrir chat interactivo          |
| Enter           | Enviar mensaje en chat          |
| Escape          | Cerrar chat                     |
| P               | Pausar/reanudar simulación      |
| F11             | Pantalla completa               |
| Scroll          | Paneo vertical del mapa         |

## Sistema de Agentes

### CodeBot (QA & Software Engineer)

- **Ubicación inicial**: (3, 3) — Área de Desarrollo
- **Color**: Cyan (#38bdf8)
- **System prompt**: Ingeniero de software, responde técnico y breve
- **Comportamiento**:
  - Ejecuta tests cada ~12s (15% probabilidad de bug)
  - Si pasa: sonido de éxito, burbuja "Test #N PASSED"
  - Si falla: sonido de error, estado ERROR, Orchestrator interviene
  - Habla con LLM cada ~8s cuando está idle

### DataBot (Analista de Datos & BI)

- **Ubicación inicial**: (12, 4) — Sala de Servidores
- **Color**: Verde (#10b981)
- **System prompt**: Analista de datos, responde con métricas
- **Comportamiento**:
  - Consulta LLM cada ~7s
  - Genera reportes de ventas, ETL, anomalías

### Orchestrator (Orquestador Multi-Agente)

- **Ubicación inicial**: (8, 8) — Centro del mapa
- **Color**: Púrpura (#a855f7)
- **System prompt**: Supervisor, responde calmado y breve
- **Comportamiento**:
  - Monitorea agentes cada ~9s
  - Si CodeBot tiene error: camina hacia él, repara en 4s
  - Regresa a base central tras reparar

### Sistema de Proximidad

- Cuando el humano se acerca (distancia < 5 tiles) a un agente:
  - El agente saluda vía LLM con su personalidad
  - Se muestra burbuja de diálogo sobre el avatar
- Al alejarse (distancia > 6): reset del saludo

## LLM Async (Hilo Separado)

### Flujo

1. `agentThink()` encola `LlmRequest` con entityId, systemPrompt, userMessage
2. Hilo worker (`llmWorker()`) procesa la cola sin bloquear el main loop
3. Respuesta se guarda en `g_llmResults`
4. `updateSimulation()` recoge respuestas y las asigna a `entity.speech`
5. Si el chat está abierto, también se agrega al historial

### Mutex

- `g_llmMutex` protege acceso concurrente a `g_llmQueue` y `g_llmResults`
- Lock granular: solo durante push/pop, no durante la llamada HTTP

## Render Isométrico

### Coordenadas

- `gridToIso(gx, gy, origin)` → pantalla (isoX, isoY)
- `isoToGrid(sx, sy, origin)` → grilla (gx, gy)
- Tile: 64×32px, grilla: 16×16

### Zonas

| Zona                  | Tiles    | Color base       | Mobiliario                    |
|-----------------------|----------|------------------|-------------------------------|
| Área de Desarrollo    | (0,0)-(7,7) | Azul petróleo  | Escritorio + monitores        |
| Sala de Servidores    | (8,0)-(15,7)| Verde oscuro  | 2 racks servidor + escritorio |
| Sala de Conferencias  | (0,8)-(7,15)| Púrpura       | Mesa reuniones + pizarra      |
| Lounge & Cafetería    | (8,8)-(15,15)| Marrón madera| Mesa, taza, macetas, cuadros  |

### Texturas Procedurales

Generadas al iniciar con `GenImageColor` + `ImageDrawRectangle`:
- `texFloor[4][2]`: 8 baldosas (4 zonas × 2 paridades) con patrón de bordes y puntitos
- `texDesk`: escritorio con sombra, monitores duales, teclado
- `texServer`: rack con 5 filas de LEDs verdes/azules
- `texMeeting`: mesa con holograma púrpura
- `texLounge`: mesa ratona, taza de café, maceta
- `texWhiteboard`: pizarra con notas adhesivas
- `texClock`, `texPlant`, `texFrame`: decoración

### Z-Ordering

Entidades ordenadas por `renderX + renderY` (profundidad isométrica)
antes de dibujar, para que los avatares más cercanos tapen a los lejanos.

## Audio

### Dispositivo

- `InitAudioDevice()` al arrancar
- `CloseAudioDevice()` al cerrar
- Verificación con `IsAudioDeviceReady()` antes de reproducir

### Sonidos Procedurales

Generados con ondas sinusoidales + envelope:

| Sonido     | Frecuencia | Duración | Evento                    |
|------------|------------|----------|---------------------------|
| `sndPass`  | 880 Hz     | 0.15s    | Test pasado, reparación   |
| `sndFail`  | 220 Hz     | 0.40s    | Bug crítico detectado     |
| `sndRepair`| 660 Hz     | 0.30s    | Orchestrator interviene   |
| `sndChat`  | 440 Hz     | 0.08s    | (reservado)               |

## UI

### Topbar

- Logo "META-OFFICE 2D"
- Indicador LLM (verde=naranja si no configurado)
- Posición del humano
- Botón ⚙️ (config LLM)
- Botón Pausar/Reanudar

### Sidebar (300px derecha)

- Lista de miembros con icono, nombre, rol, estado
- Log de actividad (últimos 8 eventos)
- Clic en miembro abre chat

### Panel Config LLM

- Modal centrado con campos: Endpoint, API Key, Modelo
- Botones: Guardar, Test, Cerrar
- Input de texto con manejo de teclas (Backspace, Enter, Escape)
- Referencias de endpoints compatibles

### Panel Chat

- Modal centrado con historial de mensajes
- Burbujas diferenciadas: usuario (azul) vs agente (color del agente)
- Input box con escritura en tiempo real
- Enter envía, Escape cierra
- "🧠 pensando..." como feedback mientras espera respuesta LLM

## Persistencia

### llm_config.env

```
LLM_ENDPOINT=...
OPENAI_API_KEY=...
LLM_MODEL=...
```

- Cargado al iniciar (`loadLlmConfig()`)
- Guardado al presionar "GUARDAR" (`saveLlmConfig()`)
- Excluido de git (`.gitignore`)

## Makefile

```makefile
CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 $(shell pkg-config --cflags raylib)
LIBS = $(shell pkg-config --libs raylib) -lm -lpthread -lcurl
SRC = src/main.cpp src/render.cpp src/agents.cpp src/ui.cpp src/network.cpp src/llm_client.cpp
```

- `make` → compila
- `make clean` → limpia .o y binario
- `make run` → compila y ejecuta

## Futuro

- [ ] Multi-usuario con WebSocket (stub en network.cpp)
- [ ] Compilación cruzada para Windows (MinGW)
- [ ] Memoria de conversación por agente (context window)
- [ ] Mas agentes y roles
- [ ] Sistema de tareas asignables desde el chat
- [ ] Sprites PNG para mobiliario y avatares
- [ ] Minimapa
