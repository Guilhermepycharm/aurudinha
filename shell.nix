{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = with pkgs; [
    gcc
    cmake
    pkg-config
    qt6.qtbase
    qt6.qtspeech
    qt6.qtmultimedia
    qt6.qtdeclarative
    espeak-ng
    speechd
    pipewire
    libpulseaudio
  ];

  shellHook = ''
    export QT_QPA_PLATFORM=wayland
    # Configura o caminho dos plugins do Qt para encontrar o plugin de voz (speechd)
    export QT_PLUGIN_PATH="${pkgs.qt6.qtspeech}/lib/qt-6/plugins:${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    export LD_LIBRARY_PATH="${pkgs.qt6.qtbase}/lib:${pkgs.espeak-ng}/lib:${pkgs.pipewire}/lib:$LD_LIBRARY_PATH"
    
    echo "✦ Ambiente C++/Qt6 atualizado com Áudio e Voz! ✦"
    echo "1. Saia e entre no nix-shell novamente."
    echo "2. Execute: cmake . && make && ./vn_app"
  '';
}
