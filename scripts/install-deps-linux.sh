#!/usr/bin/env bash
# ============================================================
# Aurudinha — Instalador de dependências para Linux
# ============================================================
# Detecta a distribuição e instala Qt6, CMake, GCC e Ninja.
# Suporta: Ubuntu/Debian, Fedora, Arch, openSUSE
# ============================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

info()  { echo -e "${CYAN}[INFO]${NC} $*"; }
ok()    { echo -e "${GREEN}[OK]${NC} $*"; }
warn()  { echo -e "${YELLOW}[AVISO]${NC} $*"; }
error() { echo -e "${RED}[ERRO]${NC} $*"; exit 1; }

# ---- Detectar distribuição ----
detect_distro() {
    if [ -f /etc/os-release ]; then
        . /etc/os-release
        echo "$ID"
    elif command -v lsb_release &>/dev/null; then
        lsb_release -si | tr '[:upper:]' '[:lower:]'
    else
        error "Não foi possível detectar a distribuição Linux."
    fi
}

# ---- Verificar se comando existe ----
has() { command -v "$1" &>/dev/null; }

# ---- Instalar pacotes ----
install_debian() {
    info "Detectado: Ubuntu/Debian"
    sudo apt update
    sudo apt install -y \
        build-essential cmake ninja-build \
        qt6-base-dev qt6-multimedia-dev qt6-speech-dev \
        libqt6texttospeech6 pkg-config
}

install_fedora() {
    info "Detectado: Fedora"
    sudo dnf install -y \
        gcc-c++ cmake ninja-build \
        qt6-qtbase-devel qt6-qtmultimedia-devel qt6-qttexttospeech-devel \
        pkg-config
}

install_arch() {
    info "Detectado: Arch Linux"
    sudo pacman -Syu --noconfirm \
        base-devel cmake ninja \
        qt6-base qt6-multimedia qt6-speech
}

install_opensuse() {
    info "Detectado: openSUSE"
    sudo zypper install -y \
        gcc-c++ cmake ninja \
        qt6-base-devel qt6-multimedia-devel qt6-texttospeech-devel \
        pkg-config
}

# ---- Verificar instalação ----
verify() {
    echo ""
    info "Verificando instalação..."
    local ok=true
    for cmd in g++ cmake ninja; do
        if has "$cmd"; then
            ok "  $cmd → $(command -v "$cmd")"
        else
            error "  $cmd → NÃO ENCONTRADO"
            ok=false
        fi
    done

    # Verificar Qt via pkg-config ou qmake
    if pkg-config --exists Qt6Core 2>/dev/null; then
        ok "  Qt6 → $(pkg-config --modversion Qt6Core)"
    elif has qmake6; then
        ok "  Qt6 → $(qmake6 --version | tail -1)"
    elif has qmake; then
        ok "  Qt6 → $(qmake --version | tail -1)"
    else
        warn "  Qt6 → não verificado (pode precisar reiniciar o terminal)"
    fi

    echo ""
    if $ok; then
        ok "Todas as dependências instaladas com sucesso!"
        echo ""
        info "Para compilar:"
        echo "  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release"
        echo "  ninja -C build"
        echo "  ./build/aurudinha_game"
    else
        error "Algumas dependências não foram encontradas. Tente reiniciar o terminal."
    fi
}

# ---- Main ----
main() {
    echo ""
    echo -e "${GREEN}╔══════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║   Aurudinha — Instalador de Deps    ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════╝${NC}"
    echo ""

    local distro
    distro=$(detect_distro)
    info "Distribuição detectada: $distro"

    # Verificar se já tem tudo
    if has cmake && has ninja && has g++ && (pkg-config --exists Qt6Core 2>/dev/null || has qmake6); then
        warn "Todas as dependências parecem já estar instaladas."
        verify
        exit 0
    fi

    # Instalar conforme a distro
    case "$distro" in
        ubuntu|debian|linuxmint|pop)   install_debian ;;
        fedora|rhel|centos)            install_fedora ;;
        arch|manjaro|endeavouros)      install_arch ;;
        opensuse*|suse)                install_opensuse ;;
        *)
            error "Distribuição '$distro' não suportada automaticamente.\nInstale manualmente: gcc, cmake, ninja, qt6-base, qt6-multimedia, qt6-speech"
            ;;
    esac

    verify
}

main "$@"
