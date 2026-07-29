#!/bin/bash
# ============================================================
# Instalación de Meta-Office desde GitHub
# ============================================================
set -e

echo "=== Meta-Office: Instalación desde GitHub ==="

# 1. Dependencias del sistema
echo "[1/5] Instalando dependencias del sistema..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev \
    libcurl4-openssl-dev \
    git wget tar 2>&1 | tail -1

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
    echo "   raylib ya instalado."
fi

# 3. Clonar repositorio
echo "[3/5] Clonando repositorio..."
cd ~
if [ -d meta-office ]; then
    echo "   Ya existe, actualizando..."
    cd meta-office && git pull
else
    git clone https://github.com/oscarhenriquezrios/meta-office.git
    cd meta-office
fi

# 4. Compilar
echo "[4/5] Compilando..."
make clean -s 2>/dev/null
make -j$(nproc) 2>&1 | grep -E "^(g\+\+|error)"

# 5. Listo!
echo ""
echo "=== Instalación completa ==="
echo ""
echo "Para ejecutar:"
echo "  cd ~/meta-office && ./meta-office"
echo ""
echo "Para configurar IA desde la app:"
echo "  Presiona el boton ⚙️ en la esquina superior derecha"
echo ""
echo "Para configurar manual:"
echo "  edita el archivo llm_config.env en ~/meta-office/"
echo ""
echo "Sin API key los agentes muestran 'LLM no configurado'"
echo "pero la oficina igual funciona."
