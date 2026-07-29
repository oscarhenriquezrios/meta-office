#!/bin/bash
set -e

# Preguntar usuario y token
read -p "GitHub username: " GH_USER
read -sp "GitHub token (classic con repo scope): " GH_TOKEN
echo ""

echo "Creando repositorio meta-office..."
curl -s -X POST \
  -H "Authorization: token $GH_TOKEN" \
  https://api.github.com/user/repos \
  -d '{
    "name": "meta-office",
    "description": "Oficina virtual isometrica 2D en C++ con raylib y agentes IA via OpenAI API",
    "private": false,
    "auto_init": false
  }' | python3 -c "import sys,json; r=json.load(sys.stdin); print(r.get('clone_url','ERROR: '+str(r)))"

cd /root/meta-office

# Inicializar git
git init
git checkout -b main
git add -A
git commit -m "Initial commit: Meta-Office C++/raylib"

# Push
git remote add origin "https://$GH_USER:$GH_TOKEN@github.com/$GH_USER/meta-office.git"
git push -u origin main

echo ""
echo "Repo creado: https://github.com/$GH_USER/meta-office"
