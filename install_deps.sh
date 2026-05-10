#!/bin/bash

# --- Script de Instalação de Dependências (Para quem não usa Nix) ---
# Esse script tenta adivinhar qual Linux você tá usando e instala as peças do Qt6.

echo "🌸 Iniciando instalador de dependências para Visual Novel..."

# Função pra dar aquele aviso se der ruim
error_exit() {
    echo "❌ Erro: $1"
    exit 1
}

# 1. Descobrir qual é o Linux da pessoa
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    error_exit "Não consegui descobrir qual é o seu Linux. Instale o Qt6 e CMake manualmente."
fi

echo "🔍 Detectado: $NAME ($OS)"

case "$OS" in
    ubuntu|debian|pop|mint)
        echo "📦 Usando APT (Ubuntu/Debian family)..."
        sudo apt update || error_exit "Não consegui atualizar a lista do APT."
        sudo apt install -y build-essential cmake qt6-base-dev qt6-speech-dev \
            libgl-dev libpulse-dev speech-dispatcher || error_exit "Falha ao instalar pacotes no Ubuntu."
        ;;

    arch|manjaro|endeavouros)
        echo "📦 Usando Pacman (Arch family)..."
        sudo pacman -Syu --needed --noconfirm base-devel cmake qt6-base qt6-speech \
            libpulse speech-dispatcher || error_exit "Falha ao instalar pacotes no Arch."
        ;;

    fedora|nobara)
        echo "📦 Usando DNF (Fedora family)..."
        sudo dnf install -y gcc-c++ cmake qt6-qtbase-devel qt6-qtspeech-devel \
            pulseaudio-libs-devel speech-dispatcher-devel || error_exit "Falha ao instalar pacotes no Fedora."
        ;;

    opensuse*|suse)
        echo "📦 Usando Zypper (OpenSUSE)..."
        sudo zypper install -y -t pattern devel_C_C++
        sudo zypper install -y cmake qt6-base-devel qt6-speech-devel \
            libpulse-devel speech-dispatcher-devel || error_exit "Falha ao instalar pacotes no OpenSUSE."
        ;;

    *)
        echo "⚠️  Seu sistema ($OS) não tá na minha lista automática."
        echo "Tente instalar manualmente: cmake, gcc, qt6-base e qt6-speech."
        exit 1
        ;;
esac

echo "✅ Tudo pronto! Agora é só rodar: cmake . && make"
