# Meta-Office 🏢

**Oficina virtual interactiva** con agentes IA que conversan en tiempo real vía OpenAI API. Construido en C++20 con raylib. Sin navegador, sin Docker, sin dependencias web — un solo binario nativo de ultra alto rendimiento (60 FPS).

Incluye **dos modos visuales** intercambiables en tiempo real:
1. **Modo 2D Isométrico** (clásico vectorial/pixel-art).
2. **Modo 3D Estilo Habbo / Sims** (suelo de madera, paredes, alfombras pasteles y avatares cúbicos con sombras).

---

## ✨ Características

- 🗺️ **Mapa interactivo 16×16** con 4 zonas (Desarrollo, Servidores, Conferencias, Lounge)
- 🪑 **Mobiliario y decoración procedural** (escritorios, racks de servidores, mesas de reunión)
- 🤖 **3 agentes IA autónomos**: CodeBot, DataBot y Orchestrator con personalidades únicas
- 💬 **Diálogos reales vía LLM** (compatible con OpenAI, OpenRouter, DeepSeek, Ollama, etc.)
- 🎮 **Dos modos gráficos (`F3`)**: Cambia instantáneamente entre 2D Iso y 3D estilo Sims
- 🌐 **Soporte Multiplayer (TCP Server)**: Sincronización de avatares en red local/remota
- 💾 **Persistencia completa**: Configuración, memoria de agentes y archivos entregables guardados en disco

---

## ⚡ Instalación Rápida

```bash
sudo apt install -y build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev libcurl4-openssl-dev git

# Clonar repo
git clone https://github.com/oscarhenriquezrios/meta-office.git
cd meta-office

# Compilar cliente y servidor multiplayer
make clean && make
```

---

## 🎮 Controles

| Tecla | Acción |
|-------|--------|
| **F3** | Alternar entre Modo 2D Isométrico y Modo 3D (Estilo Habbo/Sims) |
| **W/A/S/D** o Flechas | Mover avatar supervisor fluidamente |
| **Clic en mapa** | Mover avatar a posición específica |
| **Clic en agente** | Abrir chat interactivo con IA |
| **T** | Abrir panel de tareas |
| **P** | Pausar simulación |
| **F11** | Pantalla completa |
| **Esc** | Cerrar paneles / Salir |

---

## 🌐 Iniciar Servidor Multiplayer (Opcional)

Para jugar o conectar múltiples instancias en red:
```bash
make server
```
Y en otra terminal conectar el cliente:
```bash
make run
```

---

## 📝 Licencia

MIT