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
    export QT_PLUGIN_PATH="${pkgs.qt6.qtspeech}/lib/qt-6/plugins:${pkgs.qt6.qtbase}/lib/qt-6/plugins"
    export LD_LIBRARY_PATH="${pkgs.qt6.qtbase}/lib:${pkgs.espeak-ng}/lib:${pkgs.pipewire}/lib:$LD_LIBRARY_PATH"
    echo "✦ Ambiente Aurudinha atualizado com Voz e Áudio! ✦"
    echo "Para compilar: cmake . && make"
    echo "Para rodar: ./aurudinha_game"
  '';
}
