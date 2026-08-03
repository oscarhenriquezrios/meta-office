# MetaOffice AI — Documentación Técnica y de Arquitectura

**MetaOffice AI** es un sistema operativo virtual inmersivo de alto rendimiento que unifica la colaboración humana en tiempo real (mediante audio espacial isométrico) con una fuerza laboral de agentes de IA residentes que ejecutan código y analizan datos de forma autónoma.

A diferencia de las aplicaciones web pesadas basadas en Electron que devoran memoria RAM, MetaOffice AI está construido desde cero como un **binario nativo en C++20 con Raylib**, garantizando fluidez a 60 FPS inquebrantables, consumo mínimo de recursos y portabilidad absoluta.

---

## 1. Arquitectura del Sistema (Tech Stack)

- **Core & Motor Gráfico:** C++20, Raylib (bucle de renderizado optimizado, gestión de entidades, colisiones geométricas y proyección isométrica 2.5D).
- **Capa de Inteligencia Artificial & Agentes:** Arquitectura asíncrona multi-hilos (`std::thread`, `std::queue`, `std::mutex`) conectada a pasarelas de Modelos de Lenguaje (LLM) compatibles con la especificación de la API de OpenAI (DeepSeek, Claude, Llama, OpenRouter/DeepInfra).
- **Audio Espacial:** Sistema de coordenadas $(X, Y)$ para cálculo de proximidad entre avatares humanos y agentes sintéticos.
- **Persistencia & Memoria:** Gestión de tareas, entregables generados por bots y memorias persistentes locales en formato estructurado.

---

## 2. Estructura del Repositorio (`/root/meta-office/`)

```text
meta-office/
├── Makefile              # Reglas de compilación y enlace con g++ y Raylib
├── src/
│   ├── main.cpp          # Punto de entrada, bucle principal, manejo de eventos y rediseño de ventana
│   ├── render.cpp        # Motor de renderizado isométrico, tiles, mobiliario escalado, paredes y UI 3D/2D
│   ├── agents.cpp        # Lógica de simulación, bucle asíncrono de IA, estados de bots y audio
│   ├── tasks.cpp         # Sistema de tareas, entregables y persistencia en disco
│   ├── ui.cpp            # Paneles de control, chats y configuración de LLM
│   ├── network.cpp       # Peticiones HTTP y fetch web para los agentes
│   ├── llm_client.cpp    # Cliente HTTP para comunicación con pasarelas LLM
│   └── meta_office.hpp   # Cabecera principal con estructuras de entidades, tareas y configuración
└── deliverables/         # Archivos y código fuente generado por los agentes en tiempo real
```

---

## 3. Funcionamiento de los Componentes Clave

### A. Bucle Principal y Renderizado Isométrico (`main.cpp` & `render.cpp`)
El motor arranca inicializando la ventana de Raylib (con soporte MSAA 4X y modo redimensionable/pantalla completa). En cada iteración del bucle (`60 FPS`):
1. Procesa la entrada del usuario (`handleInput`).
2. Actualiza la física y estados de la simulación (`updateSimulation`).
3. Dibuja el escenario en orden de profundidad (del fondo al frente): fondo estrellado, tiles del suelo por zonas, alfombras, mobiliario escalado con efectos de *hover* e iluminación, paredes seudo-3D y avatares ordenados por coordenada $Y$.

### B. Sistema de Agentes Residentes (`agents.cpp`)
Los agentes no son simples chatbots estáticos en una pestaña de navegador; tienen presencia física en la oficina:
- **CodeBot (Área de Desarrollo):** Especializado en análisis y refactorización de código en tiempo real.
- **DataBot (Sala de Servidores):** Especializado en análisis de datos, métricas y consultas analíticas.
- **Orchestrator (Zona Central):** Supervisa y coordina flujos de trabajo multi-agente.
- **Comunicación Asíncrona:** Las peticiones al LLM corren en un hilo secundario (`llmWorker`) mediante una cola protegida por *mutex*, evitando que el motor gráfico sufra bloqueos o caídas de FPS.

### C. Sistema de Audio Espacial y Proximidad
La simulación calcula en tiempo real la distancia euclidiana entre el avatar del humano supervisor y los bots:
$$\text{Distancia} = \sqrt{(x_{\text{bot}} - x_{\text{human}})^2 + (y_{\text{bot}} - y_{\text{human}})^2}$$
Cuando la distancia es $\le 3.0$ unidades, se activa el canal de audio espacial y las burbujas de diálogo flotantes muestran las interacciones en tiempo real.

---

## 4. Compilación y Ejecución Local

Para compilar y ejecutar la aplicación nativa en C++:

```bash
cd /root/meta-office
make clean
make
make run
```

Esto compilará el código fuente mediante `g++ -std=c++20` enlazando las librerías nativas de Raylib, pthread y cURL, generando el binario portable `meta-office`.
