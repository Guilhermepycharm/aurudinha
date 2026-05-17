# Aurudinha - Visual Novel Acessível

Este é um projeto de Visual Novel desenvolvido em **C++17** e **Qt6**, focado em acessibilidade e narrativa imersiva. O jogo conta a história de Aurudinha, explorando temas de neurodivergência e suporte familiar.

## 🚀 Funcionalidades de Acessibilidade

- **Narrador (TTS):** Suporte total a Text-to-Speech em Português (pt-BR).
- **Modo Daltônico:** Filtros em tempo real para Deuteranopia, Protanopia e Tritanopia que afetam toda a tela.
- **Alto Contraste:** Interface adaptada para melhor legibilidade.
- **Escala de Fonte:** Ajuste dinâmico do tamanho do texto (80% a 160%).
- **Modo Leitura:** Foca o texto no centro da tela para reduzir distrações.
- **Atalhos de Teclado:** Navegação completa pelos números 1-9 para escolhas.
- **Layout Responsivo:** Ajusta-se automaticamente a qualquer resolução (Full HD, 4K, etc).

## 🛠️ Como Compilar

### No Linux (recomendado com Nix)

Se você tem o **Nix** instalado, basta entrar no ambiente:

```bash
nix-shell
cmake .
make
./aurudinha_game
```

### No Linux (Manual)

Você precisará do **Qt6 (Widgets, TextToSpeech)** e **CMake**:

```bash
cmake .
make
./aurudinha_game
```

### No Windows

Você pode baixar o pacote pronto para Windows diretamente neste repositório:

- [`aurudinha-windows.zip`](aurudinha-windows.zip)

Depois de baixar:
1. Extraia o arquivo `.zip`.
2. Abra a pasta extraída.
3. Execute `aurudinha_game.exe`.

O projeto também utiliza **GitHub Actions** para tentar gerar binários automaticamente. Você pode acompanhar as builds na aba **Actions** deste repositório.

Para compilar manualmente no Windows:
1. Instale o Qt 6.5+ (MinGW).
2. Use o CMake para gerar o projeto.
3. Utilize `windeployqt` para incluir as DLLs necessárias na pasta do executável.

## 📁 Estrutura de Arquivos

- `game.cpp`: Código-fonte principal em C++.
- `assets/`: Sprites e cenários do jogo.
- ` roteiro`: Script original da história.
- `CMakeLists.txt`: Configuração de build.
- `shell.nix`: Ambiente de desenvolvimento isolado.

---
Desenvolvido com ❤️ para promover a inclusão.
