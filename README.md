# 🌸 Clube de Literatura - Visual Novel C++/Qt6

Uma Visual Novel inspirada na estética de *Doki Doki Literature Club*, focada em **acessibilidade total** e escrita em C++17 com o framework Qt6.

## 🚀 Funcionalidades
- **História Interativa:** Sistema de cenas e escolhas com múltiplos finais.
- **Narrador Inteligente (TTS):** Narração em Português do Brasil que se ajusta ao ritmo do jogador.
- **Visual Sakura:** Efeito de pétalas caindo com performance nativa.
- **Acessibilidade Completa:**
  - Menu em tela cheia.
  - Modos para Daltonismo (Deuteranopia, Protanopia, Tritanopia).
  - Modo de Alto Contraste.
  - Escala de fonte dinâmica (80% a 160%).
  - Modo Leitura (foco no texto centralizado).
  - Atalhos de teclado (1-9).

## 🛠️ Como Compilar (Linux)

Este projeto usa o **Nix** para gerenciar dependências sem poluir o sistema.

1. Entre no ambiente:
   ```bash
   nix-shell
   ```
2. Compile o projeto:
   ```bash
   cmake . && make
   ```
3. Rode o jogo:
   ```bash
   ./vn_app
   ```

## 🪟 Versão Windows
O código é 100% compatível com Windows via Qt6 + MinGW. O repositório inclui scripts para facilitar a compilação em máquinas Windows.

---
*Feito com ❤️ por Kim.*
