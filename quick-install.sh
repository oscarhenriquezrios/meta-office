#!/bin/bash
# ============================================================
# Meta-Office — Instalación rápida desde GitHub
# Solo dependencias + clonar + compilar
# ============================================================
set -e

echo "=== Meta-Office: Instalación rápida ==="

echo "[1/3] Dependencias del sistema..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    build-essential cmake pkg-config \
    libx11-dev libxrandr-dev libxi-dev \
    libgl1-mesa-dev libglu1-mesa-dev \
    libxinerama-dev libxcursor-dev libxkbcommon-dev \
    libcurl4-openssl-dev \
    git wget tar

echo "[2/3] raylib 5.5..."
if [ ! -f /usr/local/lib/libraylib.a ]; then
    cd /tmp
    wget -q https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -O raylib.tar.gz
    tar xzf raylib.tar.gz
    cd raylib-5.5 && mkdir -p build && cd build
    cmake -DBUILD_SHARED_LIBS=OFF ..
    make -j$(nproc)
    sudo make install && sudo ldconfig
    cd /tmp && rm -rf raylib-5.5*
    echo "   raylib instalado."
else
    echo "   raylib ya instalado."
fi

echo "[3/3] Clonando y compilando..."
cd ~
if [ -d meta-office ]; then
    cd meta-office && git pull && make -j$(nproc)
else
    git clone https://github.com/oscarhenriquezrios/meta-office.git
    cd meta-office && make -j$(nproc)
fi

echo ""
echo "=== LISTO ==="
echo "  cd ~/meta-office && ./meta-office"
echo "  ⚙️  Configura IA desde el boton engranaje en la app"
