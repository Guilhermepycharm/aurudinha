{ pkgs ? import <nixpkgs> { config = { allowUnsupportedSystem = true; allowBroken = true; }; } }:

let
  mingw = pkgs.pkgsCross.mingwW64;
in
pkgs.mkShell {
  buildInputs = [
    pkgs.cmake
    pkgs.ninja
    mingw.stdenv.cc
    mingw.qt6.qtbase
  ];

  shellHook = ''
    export CC=x86_64-w64-mingw32-gcc
    export CXX=x86_64-w64-mingw32-g++
    echo "✦ Ambiente Cross-Compilação Windows MÍNIMO Pronto! ✦"
  '';
}
