#!/bin/bash
# ============================================================
# Instalación de Meta-Office — Oficina Virtual C++ / raylib
# Ejecutar en la laptop (no en el VPS)
# ============================================================
set -e

echo "=== Meta-Office: Instalación ==="

# 1. Dependencias del sistema
echo "[1/5] Instalando dependencias del sistema..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev \
    libcurl4-openssl-dev \
    wget tar 2>&1 | tail -1

# 2. raylib 5.5
echo "[2/5] Instalando raylib 5.5..."
if [ ! -f /usr/local/lib/libraylib.a ]; then
    cd /tmp
    wget -q https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -O raylib-5.5.tar.gz
    tar xzf raylib-5.5.tar.gz
    cd raylib-5.5
    mkdir -p build && cd build
    cmake -DBUILD_SHARED_LIBS=OFF .. 2>&1 | tail -1
    make -j$(nproc) 2>&1 | tail -1
    sudo make install 2>&1 | tail -1
    sudo ldconfig
    cd /tmp && rm -rf raylib-5.5*
    echo "   raylib instalado."
else
    echo "   raylib ya instalado, saltando."
fi

# 3. Copiar el proyecto
echo "[3/5] Copiando proyecto desde VPS..."
scp -r root@169.58.6.79:/root/meta-office/ ~/meta-office 2>/dev/null || {
    echo "   No se pudo copiar. Asegurate de tener acceso SSH al VPS."
    echo "   Alternativa: clona desde GitHub o copia manual."
    exit 1
}

# 4. Compilar
echo "[4/5] Compilando..."
cd ~/meta-office
make clean -s 2>/dev/null
make -j$(nproc) 2>&1 | grep -E "^(g\+\+|error|warning:.*error)"

echo ""
echo "=== Instalación completa ==="
echo ""
echo "Para ejecutar con IA real (OpenAI):"
echo "  export OPENAI_API_KEY=\"sk-...\""
echo "  cd ~/meta-office && ./meta-office"
echo ""
echo "O con OpenRouter:"
echo "  export LLM_ENDPOINT=\"https://openrouter.ai/api/v1/chat/completions\""
echo "  export LLM_MODEL=\"openai/gpt-4o-mini\""
echo "  export OPENAI_API_KEY=\"sk-or-...\""
echo "  cd ~/meta-office && ./meta-office"
echo ""
echo "Sin API key los agentes mostraran 'LLM no configurado'"
echo "pero la oficina igual funciona."
