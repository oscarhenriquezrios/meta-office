#!/bin/bash
# ============================================================
# Meta-Office — Instalación automática para Linux x86_64
# No requiere compilar, solo descomprime y ejecuta
# ============================================================
set -e

echo "=== Meta-Office: Instalación rápida ==="

# 1. Dependencias del sistema
echo "[1/3] Dependencias del sistema..."
sudo apt-get update -qq
sudo apt-get install -y -qq \
    libx11-6 libxrandr2 libxi6 \
    libgl1-mesa-glx libglu1-mesa \
    libxinerama1 libxcursor1 libxkbcommon0 \
    libcurl4 wget tar xz-utils

# 2. Descargar raylib si no está
echo "[2/3] raylib runtime..."
if [ ! -f /usr/local/lib/libraylib.a ]; then
    cd /tmp
    wget -q https://github.com/raysan5/raylib/releases/download/5.5/raylib-5.5_linux_amd64.deb -O raylib.deb
    sudo dpkg -i raylib.deb 2>/dev/null || {
        # Si no hay .deb precompilado, compilar desde fuente
        wget -q https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz -O raylib.tar.gz
        tar xzf raylib.tar.gz
        cd raylib-5.5 && mkdir -p build && cd build
        cmake -DBUILD_SHARED_LIBS=OFF ..
        make -j$(nproc)
        sudo make install
        sudo ldconfig
        cd /tmp && rm -rf raylib-5.5*
    }
    sudo ldconfig
fi

# 3. Descargar el binario (lo pasas como argumento o lo dejamos para descarga manual)
echo "[3/3] Ready!"
echo ""
echo "Descarga el binario desde el VPS:"
echo "  scp root@169.58.6.79:/root/meta-office/meta-office-linux-x86_64.tar.gz ./"
echo ""
echo "Luego:"
echo "  tar xzf meta-office-linux-x86_64.tar.gz"
echo "  cd meta-office"
echo "  export OPENAI_API_KEY='sk-...'"
echo "  ./meta-office"
echo ""
echo "O sin IA (los agentes mostraran 'no configurado'):"
echo "  ./meta-office"
