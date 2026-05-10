# 🌸 Visual Novel C++/Qt6

Uma Visual Novel moderna focada em **acessibilidade total**, escrita em C++17 com o framework Qt6.

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

## 🛠️ Como Instalar Dependências

### Usando o Script Automático (Ubuntu, Arch, Fedora, OpenSUSE)
Se você não usa Nix, pode rodar o script que eu criei para instalar tudo o que o jogo precisa:
```bash
chmod +x install_deps.sh
./install_deps.sh
```

### Usando Nix (Recomendado)
Se você tem o Nix instalado:
1. Entre no ambiente: `nix-shell`

## 🔨 Como Compilar e Rodar
Depois de ter as dependências instaladas:
1. Compile o projeto: `cmake . && make`
2. Rode o jogo: `./vn_app`

## 🪟 Versão Windows
O código é 100% compatível com Windows via Qt6 + MinGW. O repositório inclui scripts na pasta de Windows para facilitar a compilação.

---
*Feito com ❤️ por Kim.*
