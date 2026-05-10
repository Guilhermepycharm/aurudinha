import tkinter as tk
from tkinter import ttk
import json
import os
import math
import subprocess

current_speech_proc = None

def stop_speech():
    global current_speech_proc
    if current_speech_proc:
        try:
            current_speech_proc.kill()
            current_speech_proc.wait(timeout=1)
        except Exception:
            pass
        current_speech_proc = None

def speak_text(text):
    global current_speech_proc
    stop_speech()
            
    try:
        # Usa o subprocess para chamar o espeak-ng direto, rodando levinho e lisinho sem congelar o Tkinter
        # -v pt-br: Voz em português-BR
        # -s 130: Velocidade BEM mais calma (o normal é 175, 130 já fica bem entendível)
        current_speech_proc = subprocess.Popen(
            ['espeak-ng', '-v', 'pt-br', '-s', '130', text],
            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL
        )
    except Exception as e:
        print("Erro ao tentar falar texto (tem espeak-ng no sistema?):", e)


# ─────────────── Cores bem vibes DDLC ───────────────
ROSA = "#FFB7C5"
CREME = "#FFF0F5"
LILAS = "#C9A0DC"
TEXTO_ESCURO = "#3D2B3D"
ROSA_BORDA = "#ffccd5"
BRANCO = "#FFFFFF"

# Modo diferentão (Alto Contraste pra quem manja)
HC_BG = "#000000"
HC_PANEL = "#1a1a1a"
HC_ACCENT = "#FFD700"
HC_TEXT = "#FFFFFF"
HC_BORDER = "#FFFF00"

PREFS_FILE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "a11y_prefs.json")

DEFAULT_PREFS = {
    "reading_mode": False,
    "reduce_motion": False,
    "high_contrast": False,
    "font_scale": 100,
    "custom_cursor": False,
    "visible_focus": True,
    "colorblind": "none",
    "text_to_speech": False,
    "keyboard_shortcuts": True
}


def load_prefs():
    if os.path.exists(PREFS_FILE):
        try:
            with open(PREFS_FILE, "r") as f:
                saved = json.load(f)
                prefs = DEFAULT_PREFS.copy()
                prefs.update(saved)
                return prefs
        except Exception:
            pass
    return DEFAULT_PREFS.copy()


def save_prefs(prefs):
    with open(PREFS_FILE, "w") as f:
        json.dump(prefs, f)


# ═══════════════════════════════════════════
#       TELA DE CONFIGURAÇÕES (Aquele pop-up maroto)
# ═══════════════════════════════════════════
class AccessibilityMenu(tk.Toplevel):
    def __init__(self, parent, prefs, on_apply):
        super().__init__(parent)
        self.prefs = prefs.copy()
        self.on_apply = on_apply
        self.title("✦ Configurações ✦")
        self.geometry("700x680")
        self.resizable(False, False)
        self.configure(bg=ROSA)

        # ── Guardando as escolhas da galera ──
        self.var_reading = tk.BooleanVar(value=self.prefs["reading_mode"])
        self.var_motion = tk.BooleanVar(value=self.prefs["reduce_motion"])
        self.var_contrast = tk.BooleanVar(value=self.prefs["high_contrast"])
        self.var_font_scale = tk.IntVar(value=self.prefs["font_scale"])
        self.var_cursor = tk.BooleanVar(value=self.prefs["custom_cursor"])
        self.var_focus = tk.BooleanVar(value=self.prefs["visible_focus"])
        self.var_colorblind = tk.StringVar(value=self.prefs["colorblind"])
        self.var_tts = tk.BooleanVar(value=self.prefs.get("text_to_speech", False))
        self.var_shortcuts = tk.BooleanVar(value=self.prefs.get("keyboard_shortcuts", True))

        # Bora jogar umas pétalas caindo? (Criei primeiro pra ficar lá no fundão e não atrapalhar o texto)
        self.petals = []
        self.petal_canvas = tk.Canvas(self, bg=ROSA, highlightthickness=0, width=700, height=680)
        self.petal_canvas.place(x=0, y=0)

        self._build_ui()

        if not self.prefs["reduce_motion"]:
            self._spawn_petals()
            self._animate_petals()

        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self.grab_set()
        self.focus_set()

    def _build_ui(self):
        bg = ROSA
        panel_bg = CREME

        # ── A caixona principal que segura todas as paradas ──
        container = tk.Frame(self, bg=bg)
        container.place(relx=0.5, rely=0.5, anchor="center", relwidth=0.92, relheight=0.95)

        # ── Cabeçalho ──
        header = tk.Label(container, text="✦ Configurações ✦",
                          font=("Helvetica", 28, "bold"), bg=bg, fg=TEXTO_ESCURO)
        header.pack(pady=(5, 15))

        # ── Painel com scroll ──
        panel = tk.Frame(container, bg=panel_bg, bd=3, relief="groove",
                         highlightbackground=ROSA_BORDA, highlightthickness=3)
        panel.pack(fill="both", expand=True, padx=5, pady=5)

        # Canvas interno com scroll
        canvas = tk.Canvas(panel, bg=panel_bg, highlightthickness=0)
        scrollbar = tk.Scrollbar(panel, orient="vertical", command=canvas.yview)
        self.scroll_frame = tk.Frame(canvas, bg=panel_bg)

        self.scroll_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )

        canvas.create_window((0, 0), window=self.scroll_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)

        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        # Permitir scroll com roda do mouse
        def _on_mousewheel(event):
            canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")
        canvas.bind_all("<MouseWheel>", _on_mousewheel)
        canvas.bind_all("<Button-4>", lambda e: canvas.yview_scroll(-1, "units"))
        canvas.bind_all("<Button-5>", lambda e: canvas.yview_scroll(1, "units"))

        # ── A lista de botõezinhos sensacionais ──
        items = [
            ("📖 Modo Leitura",
             "Aumenta espaçamento e limita largura para leitura confortável.",
             "toggle", self.var_reading),
            ("🎨 Modo Daltônico",
             "Filtros de cor: Deuteranopia, Protanopia, Tritanopia.",
             "select", self.var_colorblind),
            ("🚫 Reduzir Movimento",
             "Desativa animações e transições.",
             "toggle", self.var_motion),
            ("◑ Alto Contraste",
             "Paleta preto/branco/amarelo de alta legibilidade.",
             "toggle", self.var_contrast),
            ("🔤 Tamanho do Texto",
             "Ajuste a escala da fonte (80%–160%).",
             "slider", self.var_font_scale),
            ("🖱 Cursor Personalizado",
             "Usa um cursor maior e mais visível.",
             "toggle", self.var_cursor),
            ("🔲 Foco Visível",
             "Destaque forte ao navegar com o teclado.",
             "toggle", self.var_focus),
            ("🔊 Falar Texto",
             "Lê as histórias e menus em voz alta.",
             "toggle", self.var_tts),
            ("⌨ Atalhos de Teclado",
             "Permite usar os números (1, 2, 3...) para escolher opções.",
             "toggle", self.var_shortcuts),
        ]

        for i, (title, desc, kind, var) in enumerate(items):
            row = tk.Frame(self.scroll_frame, bg=panel_bg, pady=8)
            row.pack(fill="x", padx=15)

            # Info lado esquerdo
            info = tk.Frame(row, bg=panel_bg)
            info.pack(side="left", fill="x", expand=True)

            tk.Label(info, text=title, font=("Helvetica", 15, "bold"),
                     bg=panel_bg, fg=TEXTO_ESCURO, anchor="w").pack(anchor="w")
            tk.Label(info, text=desc, font=("Helvetica", 11),
                     bg=panel_bg, fg="#7a5a7a", anchor="w", wraplength=380).pack(anchor="w")

            # Controle lado direito
            ctrl = tk.Frame(row, bg=panel_bg)
            ctrl.pack(side="right", padx=5)

            if kind == "toggle":
                cb = tk.Checkbutton(ctrl, variable=var,
                                    onvalue=True, offvalue=False,
                                    bg=panel_bg, activebackground=panel_bg,
                                    selectcolor=LILAS, indicatoron=True,
                                    font=("Helvetica", 14),
                                    command=self._on_change)
                cb.pack()

            elif kind == "select":
                combo = ttk.Combobox(ctrl, textvariable=var, state="readonly", width=15,
                                     values=["none", "deuteranopia", "protanopia", "tritanopia"])
                combo.pack()
                combo.bind("<<ComboboxSelected>>", lambda e: self._on_change())

            elif kind == "slider":
                s = tk.Scale(ctrl, variable=var, from_=80, to=160,
                             orient="horizontal", length=150,
                             bg=panel_bg, fg=TEXTO_ESCURO,
                             troughcolor=ROSA_BORDA, highlightthickness=0,
                             command=lambda v: self._on_change())
                s.pack()

            # Separador
            if i < len(items) - 1:
                sep = tk.Frame(self.scroll_frame, bg=ROSA_BORDA, height=2)
                sep.pack(fill="x", padx=20, pady=2)

        # ── Aquele botão de "vish, deu ruim, volta pro começo" ──
        btn_reset = tk.Button(container, text="🔄 Restaurar Padrões",
                              font=("Helvetica", 14, "bold"),
                              bg=ROSA, fg=TEXTO_ESCURO, bd=3,
                              activebackground=LILAS, activeforeground=BRANCO,
                              relief="raised", cursor="hand2",
                              command=self._reset_all)
        btn_reset.pack(pady=(10, 5), ipadx=15, ipady=5)

    def _on_change(self):
        """Atualiza prefs e aplica em tempo real."""
        self.prefs["reading_mode"] = self.var_reading.get()
        self.prefs["reduce_motion"] = self.var_motion.get()
        self.prefs["high_contrast"] = self.var_contrast.get()
        self.prefs["font_scale"] = self.var_font_scale.get()
        self.prefs["custom_cursor"] = self.var_cursor.get()
        self.prefs["visible_focus"] = self.var_focus.get()
        self.prefs["colorblind"] = self.var_colorblind.get()
        self.prefs["text_to_speech"] = self.var_tts.get()
        self.prefs["keyboard_shortcuts"] = self.var_shortcuts.get()
        save_prefs(self.prefs)
        self.on_apply(self.prefs)

    def _reset_all(self):
        defaults = DEFAULT_PREFS.copy()
        self.var_reading.set(defaults["reading_mode"])
        self.var_motion.set(defaults["reduce_motion"])
        self.var_contrast.set(defaults["high_contrast"])
        self.var_font_scale.set(defaults["font_scale"])
        self.var_cursor.set(defaults["custom_cursor"])
        self.var_focus.set(defaults["visible_focus"])
        self.var_colorblind.set(defaults["colorblind"])
        self.var_tts.set(defaults["text_to_speech"])
        self.var_shortcuts.set(defaults["keyboard_shortcuts"])
        self.prefs = defaults
        save_prefs(self.prefs)
        self.on_apply(self.prefs)

    def _on_close(self):
        self._on_change()
        self.grab_release()
        self.destroy()

    # ── Fazendo chover pétalas de sakura bonito demais ──
    def _spawn_petals(self):
        import random
        for _ in range(15):
            x = random.randint(0, 700)
            y = random.randint(-200, -20)
            size = random.randint(8, 18)
            speed = random.uniform(0.5, 2.0)
            drift = random.uniform(-0.5, 0.5)
            petal_id = self.petal_canvas.create_oval(
                x, y, x + size, y + size,
                fill=BRANCO, outline=ROSA_BORDA, width=1
            )
            self.petals.append({"id": petal_id, "speed": speed, "drift": drift, "size": size})

    def _animate_petals(self):
        import random
        if not self.winfo_exists():
            return
        for p in self.petals:
            coords = self.petal_canvas.coords(p["id"])
            if not coords:
                continue
            x1, y1, x2, y2 = coords
            if y1 > 700:
                # Caiu? Joga a pétala lá pra cima de novo
                new_x = random.randint(0, 700)
                self.petal_canvas.coords(p["id"], new_x, -20, new_x + p["size"], -20 + p["size"])
            else:
                self.petal_canvas.move(p["id"], p["drift"], p["speed"])
        self.after(30, self._animate_petals)


# ═══════════════════════════════════════════
#       A VISUAL NOVEL PRA VALER (Aqui o bicho pega)
# ═══════════════════════════════════════════
class VisualNovel:
    def __init__(self, root):
        self.root = root
        self.root.title("Clube de Literatura - Visual Novel")
        self.root.geometry("1280x720")

        self.prefs = load_prefs()

        # Tentando puxar os cenários de fundo (se der ruim a gente avisa)
        import sys
        if hasattr(sys, '_MEIPASS'):
            base_dir = os.path.join(sys._MEIPASS)
        else:
            base_dir = os.path.dirname(os.path.abspath(__file__))
            
        try:
            self.bg_bedroom = tk.PhotoImage(file=os.path.join(base_dir, "assets", "bedroom.png"))
            self.bg_class = tk.PhotoImage(file=os.path.join(base_dir, "assets", "class.png"))
        except Exception as e:
            print("Erro ao carregar imagens, verifique se os arquivos são .png válidos:", e)
            self.bg_bedroom = None
            self.bg_class = None

        # ── O panofundo da tela ──
        self.bg_label = tk.Label(root)
        self.bg_label.place(relx=0.5, rely=0.5, anchor="center")

        # ── A moldura rosa bonitinha ──
        self.caixa_texto_frame = tk.Frame(root, bg=ROSA, bd=5)
        self.caixa_texto_frame.place(relwidth=0.8, relheight=0.25, relx=0.1, rely=0.7)

        self.texto_label = tk.Label(self.caixa_texto_frame, text="",
                                    font=("Helvetica", 16, "bold"),
                                    wraplength=900, justify="left",
                                    bg="white", fg="#4B0082", padx=20, pady=20)
        self.texto_label.place(relwidth=1, relheight=1)

        # ── Onde a galera clica pra tomar decisão ──
        self.botoes_frame = tk.Frame(root, bg=ROSA, bd=0)
        self.botoes_frame.place(relx=0.5, rely=0.4, anchor="center")

        # ── O botãozinho maroto de opciones que fica lá no canto ──
        self.btn_a11y = tk.Button(root, text="⚙ Acessibilidade",
                                  font=("Helvetica", 11, "bold"),
                                  bg=LILAS, fg=BRANCO, bd=2,
                                  activebackground=ROSA, activeforeground=TEXTO_ESCURO,
                                  relief="raised", cursor="hand2",
                                  command=self._open_a11y_menu)
        self.btn_a11y.place(relx=0.98, rely=0.02, anchor="ne")

        # ── O roteirinho da nossa história louca ──
        self.cenas = {
            "inicio_bedroom": {
                "bg": self.bg_bedroom,
                "texto": "Manhã de segunda-feira. O sol mal iluminava meu quarto...\nEu estava exausto das jogatinas, mas o Clube de Literatura me aguardava.",
                "escolhas": [
                    ("Levantar imediatamente e ir cedo para a escola", "cena_chegar_cedo"),
                    ("Dormir só mais 5 minutinhos...", "cena_chegar_atrasado")
                ]
            },
            "cena_chegar_cedo": {
                "bg": self.bg_class,
                "texto": "Cheguei na sala de aula. Yuri já estava lá no canto, lendo um livro envolvente em silêncio.\nO que devo fazer?",
                "escolhas": [
                    ("Sentar ao lado dela e tentar ler também", "final_yuri"),
                    ("Começar a escrever poemas na minha carteira", "final_poema")
                ]
            },
            "cena_chegar_atrasado": {
                "bg": self.bg_class,
                "texto": "Entrei correndo e ofegante na sala. Natsuki, irritada, cruzou os braços:\n'Sempre deixando as pessoas esperando, né?!'",
                "escolhas": [
                    ("Pedir miiiil desculpas e dar um bolinho de presente", "final_natsuki"),
                    ("Ser sarcástico e sentar direto sem ligar pra ela", "final_rude")
                ]
            },
            "final_yuri": {
                "bg": self.bg_class,
                "texto": "Yuri percebe seu olhar, cora levemente e compartilha um pedaço do livro com você.\n\n[ FINAL 1: A Rota Sombria e Silenciosa ]",
                "escolhas": []
            },
            "final_poema": {
                "bg": self.bg_class,
                "texto": "A presidente Monika adorou o seu poema!\n...Na verdade, ela adora tudo em você.\n\n[ FINAL 2: Só Monika ]",
                "escolhas": []
            },
            "final_natsuki": {
                "bg": self.bg_class,
                "texto": "Ela abre um sorrisinho, desvia o olhar e diz:\n'N-não é como se eu tivesse gostado que você chegou... Baka!'\n\n[ FINAL 3: A Rota Doce ]",
                "escolhas": []
            },
            "final_rude": {
                "bg": self.bg_class,
                "texto": "Dá uma treta enorme no clube. Sayori tenta separar, mas o clima fica péssimo. Você volta pra casa.\n\n[ FINAL 4: Um Clube Desfeito ]",
                "escolhas": []
            }
        }

        self._apply_prefs(self.prefs)
        self.ir_para_cena("inicio_bedroom")

    def ir_para_cena(self, id_cena):
        cena = self.cenas[id_cena]

        if cena["bg"] is not None:
            self.bg_label.config(image=cena["bg"])

        self.texto_label.config(text=cena["texto"])
        if self.prefs.get("text_to_speech", False):
            # Formata pra ele ler a história e as opções de forma fluida
            texto_falar = cena["texto"]
            if cena["escolhas"]:
                texto_falar += " ... Opções: ... "
                for i, (texto_escolha, _) in enumerate(cena["escolhas"], 1):
                    texto_falar += f" Opção {i}: ... {texto_escolha} ... "
            speak_text(texto_falar)

        for widget in self.botoes_frame.winfo_children():
            widget.destroy()

        font_size = max(10, int(12 * self.prefs["font_scale"] / 100))
        estilo_botao = {
            "font": ("Helvetica", font_size, "bold"),
            "width": 55,
            "bg": getattr(self, "current_btn_bg", CREME),
            "fg": getattr(self, "current_btn_fg", "#8B0086"),
            "bd": 3,
            "relief": tk.RAISED,
            "cursor": "hand2"
        }

        # Limpar os atalhos antigos (de 1 a 9) que a gente possa ter setado
        for i in range(1, 10):
            self.root.unbind(str(i))

        shortcuts_enabled = self.prefs.get("keyboard_shortcuts", True)

        if not cena["escolhas"]:
            text_inicio = ">> Voltar ao início <<"
            if shortcuts_enabled:
                text_inicio = "[1] " + text_inicio
                self.root.bind("1", lambda e: self.ir_para_cena("inicio_bedroom"))
            
            btn = tk.Button(self.botoes_frame, text=text_inicio,
                      command=lambda: self.ir_para_cena("inicio_bedroom"),
                      **estilo_botao)
            btn.pack(pady=8)
        else:
            for i, (texto_escolha, proxima_cena) in enumerate(cena["escolhas"], 1):
                display_text = texto_escolha
                if shortcuts_enabled:
                    display_text = f"[{i}] {texto_escolha}"
                    # Binda a tecla pro mano que gosta de jogar no teclado
                    self.root.bind(str(i), lambda e, prox=proxima_cena: self.ir_para_cena(prox))

                tk.Button(self.botoes_frame, text=display_text,
                          command=lambda prox=proxima_cena: self.ir_para_cena(prox),
                          **estilo_botao).pack(pady=8)

    def _open_a11y_menu(self):
        AccessibilityMenu(self.root, self.prefs, self._apply_prefs)

    def _apply_prefs(self, prefs):
        self.prefs = prefs

        # ── Dando aquela ajeitada no tamanho das letras ──
        scale = prefs["font_scale"] / 100
        base_text = max(12, int(16 * scale))
        self.texto_label.config(font=("Helvetica", base_text, "bold"))

        # ── Ajuste brabo pro modo de leitura não ficar cansativo ──
        if prefs["reading_mode"]:
            self.caixa_texto_frame.place_configure(relwidth=0.6, relheight=0.35, relx=0.2, rely=0.6)
            self.texto_label.config(pady=30, wraplength=550)
        else:
            self.caixa_texto_frame.place_configure(relwidth=0.8, relheight=0.25, relx=0.1, rely=0.7)
            self.texto_label.config(pady=20, wraplength=900)

        # ── Trocando a cara toda da aplicação (Daltonismo/Contraste) ──
        # Nossas paletinhas de cor estilosas
        palettes = {
            "none": {"bg": ROSA, "panel": BRANCO, "accent": LILAS, "text": TEXTO_ESCURO, "border": ROSA},
            "deuteranopia": {"bg": "#005b96", "panel": "#b3cde0", "accent": "#ffc100", "text": "#000000", "border": "#005b96"},
            "protanopia": {"bg": "#005b96", "panel": "#b3cde0", "accent": "#ffc100", "text": "#000000", "border": "#005b96"},
            "tritanopia": {"bg": "#c81d25", "panel": "#fbc4ab", "accent": "#0081a7", "text": "#000000", "border": "#c81d25"}
        }

        mode = prefs.get("colorblind", "none")
        if mode not in palettes: mode = "none"
        active_palette = palettes[mode]

        # Alto contraste manda na parada toda, sobrescreve mesmo
        if prefs["high_contrast"]:
            self.root.config(bg=HC_BG)
            self.caixa_texto_frame.config(bg=HC_BORDER)
            self.texto_label.config(bg=HC_PANEL, fg=HC_TEXT)
            self.botoes_frame.config(bg=HC_BG)
            self.btn_a11y.config(bg=HC_ACCENT, fg=HC_BG)
            
            self.current_btn_bg = HC_BG
            self.current_btn_fg = HC_ACCENT
        else:
            self.root.config(bg=active_palette["bg"])
            self.caixa_texto_frame.config(bg=active_palette["border"])
            self.texto_label.config(bg=active_palette["panel"], fg=active_palette["text"])
            self.botoes_frame.config(bg=active_palette["bg"])
            self.btn_a11y.config(bg=active_palette["accent"], fg=active_palette["panel"])
            
            self.current_btn_bg = active_palette["panel"]
            self.current_btn_fg = active_palette["text"]

        # ── E esse mouse diferentão aí? ──
        if prefs["custom_cursor"]:
            self.root.config(cursor="hand2")  # O Tkinter dá BO com imagem pra cursor na web, ent bora de mãozinha mesmo q é sucesso
        else:
            self.root.config(cursor="")

        # ── Aquele contorno pra salvar quem vai só no teclado ──
        if prefs["visible_focus"]:
            self.root.option_add("*highlightThickness", 4)
            self.root.option_add("*highlightColor", HC_BORDER if prefs["high_contrast"] else active_palette["accent"])
        else:
            self.root.option_add("*highlightThickness", 0)

        # Dá um refresh maroto na cena de agora pras cores e fontes novas entrarem
        # Se desligou o TTS, para de falar na hora
        if not prefs.get("text_to_speech", False):
            stop_speech()

        for key, cena in self.cenas.items():
            if cena["texto"] == self.texto_label.cget("text"):
                self.ir_para_cena(key)
                break


if __name__ == "__main__":
    root = tk.Tk()
    app = VisualNovel(root)
    root.mainloop()
