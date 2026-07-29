# Meta-Office 🏢

**Oficina virtual isométrica 2D** con agentes IA que conversan en tiempo real vía OpenAI API. Construido en C++20 con raylib. Sin navegador, sin Docker, sin dependencias web — un solo binario nativo.

![Meta-Office](https://github.com/oscarhenriquezrios/meta-office/raw/main/screenshot.png)

## ✨ Características

- 🗺️ **Mapa isométrico 16×16** con 4 zonas (Desarrollo, Servidores, Conferencias, Lounge)
- 🪑 **Mobiliario con texturas procedurales**: escritorios con monitores, racks LED, mesa de reuniones, pizarra, reloj, cafetería, plantas, cuadros
- 🤖 **3 agentes IA autónomos**: CodeBot, DataBot y Orchestrator con personalidades únicas
- 💬 **Diálogos reales vía LLM** — cada agente responde según su rol usando OpenAI API
- 🗨️ **Chat interactivo** — clic en un agente para conversarle y asignar tareas
- 🔊 **Sonido procedural** — beeps de éxito, error y reparación
- 🚶 **Movimiento WASD** + clic en el mapa
- 🖥️ **Ventana redimensionable** + F11 pantalla completa
- ⚙️ **Config LLM desde la app** — endpoint, API key y modelo, persistido en `llm_config.env`
- 🔧 **Simulación de bugs**: CodeBot detecta errores → Orchestrator interviene y repara

## ⚡ Instalación

### Opción 1: Script automático

```bash
curl -sL https://raw.githubusercontent.com/oscarhenriquezrios/meta-office/main/quick-install.sh | bash
cd ~/meta-office && ./meta-office
```

### Opción 2: Manual

```bash
# Dependencias
sudo apt install -y build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev libcurl4-openssl-dev git

# Raylib 5.5
cd /tmp
wget -q https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -O raylib.tar.gz
tar xzf raylib.tar.gz && cd raylib-5.5 && mkdir build && cd build
cmake -DBUILD_SHARED_LIBS=OFF .. && make -j$(nproc) && sudo make install && sudo ldconfig

# Meta-Office
cd ~ && git clone https://github.com/oscarhenriquezrios/meta-office.git
cd meta-office && make -j$(nproc)
```

## 🤖 Configurar IA

**Desde la app**: clic en ⚙️ (esquina superior derecha)

O edita `llm_config.env` manualmente:

```ini
LLM_ENDPOINT=https://api.openai.com/v1/chat/completions
OPENAI_API_KEY=sk-...
LLM_MODEL=gpt-4o-mini
```

Compatible con: OpenAI, OpenRouter, DeepSeek, Ollama local, o cualquier endpoint OpenAI-compatible.

Sin API key los agentes muestran "LLM no configurado" pero la oficina funciona igual.

## 🎮 Controles

| Tecla | Acción |
|-------|--------|
| WASD / flechas | Mover humano |
| Clic en mapa | Mover a posición |
| Clic en agente | Abrir chat |
| Enter | Enviar mensaje |
| Escape | Cerrar chat |
| P | Pausar |
| F11 | Pantalla completa |

## 📚 Documentación

Ver [DOCUMENTATION.md](DOCUMENTATION.md) para detalles técnicos completos: arquitectura, sistema de agentes, render isométrico, LLM async, audio, etc.

## 📝 Licencia

MIT