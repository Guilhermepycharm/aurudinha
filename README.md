# 🎮 Aurudinha

Uma visual novel em C++/Qt6 sobre conscientização do autismo. Acompanhe Aurudinha em sua jornada e faça escolhas que moldam o destino dela.

![Aurudinha](docs/screenshots/screenshot_2026-05-12.png)

## ✨ Funcionalidades

- **Acessibilidade completa:** alto contraste, modos daltônicos, escala de fonte, redução de animação, texto para voz (pt-BR), navegação por teclado
- **Múltiplos finais:** suas escolhas determinam o destino de Aurudinha
- **Efeitos visuais:** pétalas animadas, paletas daltônicas, cursor customizado

## 🎯 Requisitos

### Linux
- GCC 12+ ou Clang 15+
- Qt 6.5+ (Core, Gui, Widgets, Multimedia, TextToSpeech)
- CMake 3.16+
- Ninja ou Make

### Windows
- MSYS2 UCRT64
- Qt 6.5+ (via pacman)
- CMake, Ninja, GCC (via pacman)

## 🐧 Linux — Instalação Rápida

### Opção 1: Script automático

```bash
chmod +x scripts/install-deps-linux.sh
./scripts/install-deps-linux.sh
```

### Opção 2: Nix

```bash
cd config/
nix-shell shell.nix --run "cd .. && cmake -B build -G Ninja && ninja -C build"
./build/aurudinha_game
```

### Opção 3: Manual

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake ninja-build \
  qt6-base-dev qt6-multimedia-dev qt6-speech-dev \
  libqt6texttospeech6

# Fedora
sudo dnf install gcc-c++ cmake ninja-build \
  qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qttexttospeech-devel

# Arch
sudo pacman -S base-devel cmake ninja qt6-base qt6-multimedia qt6-speech

# Compilar
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C build
./build/aurudinha_game
```

## 🪟 Windows — Instalação

### Opção 1: Download pré-compilado

Baixe o `.zip` mais recente em [Releases](https://github.com/Guilhermepycharm/aurudinha/releases) e extraia. Execute `aurudinha_game.exe`.

### Opção 2: Compilar manualmente

1. Instale o [MSYS2](https://www.msys2.org/)
2. Abra o terminal **MSYS2 UCRT64** e instale as dependências:

```bash
pacman -S mingw-w64-ucrt-x86_64-gcc \
  mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja \
  mingw-w64-ucrt-x86_64-qt6-base \
  mingw-w64-ucrt-x86_64-qt6-multimedia \
  mingw-w64-ucrt-x86_64-qt6-tools \
  mingw-w64-ucrt-x86_64-qt6-speech
```

3. Clone e compile:

```bash
git clone https://github.com/Guilhermepycharm/aurudinha.git
cd aurudinha
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_PREFIX_PATH=/ucrt64/lib/cmake/Qt6
ninja -C build
```

4. Execute:

```bash
windeployqt build/aurudinha_game.exe
./build/aurudinha_game.exe
```

## 📁 Estrutura do Projeto

```
aurudinha/
├── src/game.cpp          # Código principal (tudo em um arquivo)
├── assets/
│   ├── backgrounds/      # Imagens de fundo (PNG)
│   └── sprites/          # Sprites dos personagens
│       ├── aurudinha/
│       ├── mae/
│       ├── pai/
│       └── professora/
├── config/
│   ├── a11y_prefs.json   # Preferências de acessibilidade
│   ├── shell.nix         # Ambiente Nix (Linux)
│   └── cross-shell.nix   # Cross-compilação Nix (Windows)
├── docs/
│   ├── roteiro.md        # Roteiro completo do jogo
│   └── screenshots/      # Capturas de tela
├── scripts/
│   └── install-deps-linux.sh  # Script de instalação (Linux)
├── CMakeLists.txt
└── .github/workflows/    # CI/CD (build Windows automático)
```

## 🎮 Controles

| Tecla | Ação |
|-------|------|
| `1-9` | Selecionar escolha |
| `C` | Abrir configurações de acessibilidade |
| `Esc` | Fechar menu de configurações |

## 📜 Licença

Este projeto é de código aberto para fins educacionais e de conscientização.
