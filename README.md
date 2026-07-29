# Meta-Office 🏢

**Oficina virtual isométrica 2D** con agentes IA que conversan mediante OpenAI API. Construido en C++ con raylib.

![Meta-Office Screenshot](screenshot.png)

## ✨ Características

- 🗺️ **Mapa isométrico 16×16** con 4 zonas: Desarrollo, Servidores, Conferencias, Lounge
- 🪑 **Mobiliario interactivo**: escritorios con monitores, racks de servidores LED, mesa de reuniones, pizarra, reloj de pared, cafetería con macetas
- 🤖 **Agentes IA autónomos**: CodeBot, DataBot y Orchestrator con personalidades distintas
- 💬 **Diálogos reales vía OpenAI API** — cada agente responde según su rol (no frases prefabricadas)
- 🚶 **Movimiento WASD** + clic en el mapa para desplazar al humano
- 🔧 **Simulación de bugs**: CodeBot puede fallar y Orchestrator interviene para repararlo
- 📊 **Sidebar** con estado de agentes y log de actividad en tiempo real
- 🌐 **Extensible**: stub de red listo para conectar múltiples usuarios (WebSocket)

## 🖥️ Requisitos

- Linux x86_64 con servidor gráfico X11 (o Windows con raylib/MinGW)
- OpenGL 4.5 (o superior)

## ⚡ Instalación rápida

```bash
# 1. Descargar el binario compilado
curl -LO https://github.com/oscarhenriquezrios/meta-office/releases/latest/download/meta-office-linux-x86_64.tar.gz
tar xzf meta-office-linux-x86_64.tar.gz

# 2. Instalar dependencias del sistema (solo primera vez)
sudo apt install -y libx11-6 libxrandr2 libxi6 libgl1-mesa-glx \
    libglu1-mesa libxinerama1 libxcursor1 libxkbcommon0 libcurl4

# 3. Instalar raylib 5.5 (solo primera vez)
wget -q https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.deb
sudo dpkg -i raylib-5.5_linux_amd64.deb

# 4. Ejecutar
./meta-office
```

## 🔧 Compilar desde fuente

```bash
# Dependencias de compilación
sudo apt install -y build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev libcurl4-openssl-dev

# Instalar raylib 5.5
cd /tmp
wget -q https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -O raylib.tar.gz
tar xzf raylib.tar.gz
cd raylib-5.5 && mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF ..
make -j$(nproc)
sudo make install && sudo ldconfig

# Compilar Meta-Office
cd ~/meta-office
make clean && make -j$(nproc)
```

## 🤖 Configurar IA de los agentes

Los agentes usan **cualquier API compatible con OpenAI** (OpenAI, OpenRouter, DeepSeek, Ollama local, etc.).

```bash
# Con OpenAI
export OPENAI_API_KEY="sk-..."

# Con OpenRouter
export OPENAI_API_KEY="sk-or-..."
export LLM_MODEL="openai/gpt-4o-mini"

# Con endpoint personalizado
export OPENAI_API_KEY="..."
export LLM_ENDPOINT="https://tu-endpoint/v1/chat/completions"

# Ejecutar
./meta-office
```

Sin API key los agentes muestran "LLM no configurado" pero el resto funciona igual.

## 🎮 Controles

| Tecla | Acción |
|-------|--------|
| `W` / `↑` | Mover humano arriba |
| `S` / `↓` | Mover humano abajo |
| `A` / `←` | Mover humano izquierda |
| `D` / `→` | Mover humano derecha |
| Clic en mapa | Mover humano a esa posición |
| Clic en agente | Seleccionar e inspeccionar |
| `P` | Pausar/reanudar simulación |
| Scroll | Paneo vertical del mapa |

## 🏗️ Estructura del proyecto

```
meta-office/
├── Makefile              # Compilación
├── src/
│   ├── meta_office.hpp   # Header principal
│   ├── main.cpp          # Loop principal
│   ├── render.cpp        # Mapa isométrico + mobiliario
│   ├── agents.cpp        # Simulación de agentes IA
│   ├── ui.cpp            # Interfaz de usuario + input
│   ├── llm_client.cpp    # Cliente HTTP para OpenAI API
│   └── network.cpp       # Stub para WebSocket (multi-usuario)
└── install.sh            # Script de instalación completo
```

## 🪟 Windows

Para compilar en Windows se necesita MinGW y raylib para Windows. Instrucciones próximamente.

## 📝 Licencia

MIT
