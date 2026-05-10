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

## 🛠️ Como Instalar as Dependências

### 1. Script Automático (Mais fácil)
Se você estiver no **Ubuntu, Arch, Fedora ou OpenSUSE**, basta rodar o comando abaixo para instalar o Qt6 e o CMake automaticamente:

```bash
chmod +x install_deps.sh
./install_deps.sh
```

### 2. Usando Nix (Para quem quer ambiente isolado)
Se você tem o Nix instalado, não precisa rodar o script acima:
```bash
nix-shell
```

## 🔨 Como Compilar e Jogar
Após instalar as dependências com um dos métodos acima, rode:

1. **Configurar e Compilar:**
   ```bash
   cmake . && make
   ```
2. **Abrir o Jogo:**
   ```bash
   ./vn_app
   ```

## 🪟 Versão Windows
O código é 100% compatível com Windows. Para compilar na VM de Windows, use o Qt6 (MinGW) e o script `.bat` incluído na pasta de porte para Windows.

---
*Feito com ❤️ por Kim.*
