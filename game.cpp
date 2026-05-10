#include <QApplication>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#include <QTextToSpeech>
#include <QVoice>
#include <QLocale>
#include <QPixmap>
#include <QPalette>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QRandomGenerator>
#include <QKeyEvent>
#include <QDialog>
#include <QPainter>
#include <vector>
#include <map>
#include <functional>

// --- Cores do jogo (Vibe DDLC) ---
// Aqui a gente define os tons de rosa e roxo pra ficar tudo bonitinho
const QString ROSA = "#FFB7C5";
const QString CREME = "#FFF0F5";
const QString LILAS = "#C9A0DC";
const QString TEXTO_ESCURO = "#3D2B3D";
const QString ROSA_BORDA = "#ffccd5";
const QString BRANCO = "#FFFFFF";

// Cores pro modo de Alto Contraste (preto e amarelo pra quem precisa enxergar melhor)
const QString HC_BG = "#000000";
const QString HC_PANEL = "#1a1a1a";
const QString HC_ACCENT = "#FFD700";
const QString HC_TEXT = "#FFFFFF";
const QString HC_BORDER = "#FFFF00";

// --- Guardando as preferências da galera ---
// Essa struct segura tudo o que o usuário mexeu nas configs
struct Prefs {
    bool reading_mode = false;
    bool reduce_motion = false;
    bool high_contrast = false;
    int font_scale = 100;
    bool custom_cursor = false;
    bool visible_focus = true;
    QString colorblind = "none";
    bool text_to_speech = false;
    bool keyboard_shortcuts = true;

    // Transforma as configs num objeto JSON pra salvar no arquivo
    QJsonObject toJson() const {
        QJsonObject obj;
        obj["reading_mode"] = reading_mode;
        obj["reduce_motion"] = reduce_motion;
        obj["high_contrast"] = high_contrast;
        obj["font_scale"] = font_scale;
        obj["custom_cursor"] = custom_cursor;
        obj["visible_focus"] = visible_focus;
        obj["colorblind"] = colorblind;
        obj["text_to_speech"] = text_to_speech;
        obj["keyboard_shortcuts"] = keyboard_shortcuts;
        return obj;
    }

    // Pega o JSON do arquivo e joga de volta pra nossa struct
    static Prefs fromJson(const QJsonObject &obj) {
        Prefs p;
        if (obj.contains("reading_mode")) p.reading_mode = obj["reading_mode"].toBool();
        if (obj.contains("reduce_motion")) p.reduce_motion = obj["reduce_motion"].toBool();
        if (obj.contains("high_contrast")) p.high_contrast = obj["high_contrast"].toBool();
        if (obj.contains("font_scale")) p.font_scale = obj["font_scale"].toInt();
        if (obj.contains("custom_cursor")) p.custom_cursor = obj["custom_cursor"].toBool();
        if (obj.contains("visible_focus")) p.visible_focus = obj["visible_focus"].toBool();
        if (obj.contains("colorblind")) p.colorblind = obj["colorblind"].toString();
        if (obj.contains("text_to_speech")) p.text_to_speech = obj["text_to_speech"].toBool();
        if (obj.contains("keyboard_shortcuts")) p.keyboard_shortcuts = obj["keyboard_shortcuts"].toBool();
        return p;
    }
};

// Estrutura simples pras escolhas que aparecem na tela
struct Choice {
    QString text;      // O que tá escrito no botão
    QString nextScene; // Pra onde o jogo vai se clicar aqui
};

// Estrutura de cada cena do jogo
struct Scene {
    QString bgPath;              // Caminho da imagem de fundo
    QString text;                // Texto da história que aparece embaixo
    std::vector<Choice> choices; // Lista de botões que o player pode clicar
};

// --- Efeito das Pétalas caindo (Sakura) ---
// Cada pétala tem sua posição, tamanho e velocidade sorteada
struct Petal {
    float x, y, size, speed, drift;
    void reset(int w, int h) {
        x = QRandomGenerator::global()->bounded(w > 0 ? w : 1280);
        y = -20 - QRandomGenerator::global()->bounded(200); // Começa acima da tela
        size = 10 + QRandomGenerator::global()->bounded(15);
        speed = 1.0 + QRandomGenerator::global()->generateDouble() * 2.5;
        drift = -1.0 + QRandomGenerator::global()->generateDouble() * 2.0; // Balanço pros lados
    }
};

// Esse cara aqui é o que desenha as pétalas na tela
class PetalCanvas : public QWidget {
public:
    PetalCanvas(QWidget *parent) : QWidget(parent) {
        // MUITO IMPORTANTE: Isso faz o mouse ignorar as pétalas. 
        // Se não, o player ia tentar clicar num botão e a pétala ia "roubar" o clique.
        setAttribute(Qt::WA_TransparentForMouseEvents);
        for(int i=0; i<25; ++i) {
            Petal p; p.reset(1280, 720);
            petals.push_back(p);
        }
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &PetalCanvas::updateAnimation);
    }

    void start() { timer->start(30); } // Roda a animação em 30 fps
    void stop() { timer->stop(); update(); }

protected:
    // Aqui é onde a mágica do desenho acontece
    void paintEvent(QPaintEvent *) override {
        if (timer->isActive()) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing); // Deixa as bordas das bolinhas lisas
            painter.setBrush(Qt::white);
            painter.setPen(QPen(QColor(ROSA_BORDA), 1));
            for(auto &p : petals) {
                painter.drawEllipse(p.x, p.y, p.size, p.size);
            }
        }
    }

    // Move cada pétala pra baixo e reseta se sair da tela
    void updateAnimation() {
        for(auto &p : petals) {
            p.y += p.speed;
            p.x += p.drift;
            if (p.y > height()) p.reset(width(), height());
        }
        update(); // Avisa o Qt que precisa desenhar de novo
    }

private:
    std::vector<Petal> petals;
    QTimer *timer;
};

// --- Menu de Acessibilidade (O que abre em tela cheia) ---
class AccessibilityMenu : public QDialog {
public:
    AccessibilityMenu(QWidget *parent, Prefs &currentPrefs, std::function<void(const Prefs&)> onApply) 
        : QDialog(parent), prefs(currentPrefs), applyCallback(onApply) {
        setWindowTitle("Configurações");
        setModal(true); // Trava a janela de trás enquanto essa tá aberta
        setStyleSheet(QString("background-color: %1;").arg(ROSA));

        // Coloca as pétalas no fundo do menu
        petalCanvas = new PetalCanvas(this);
        petalCanvas->setGeometry(0, 0, 1920, 1080);
        petalCanvas->lower(); 

        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(40, 40, 40, 40);
        
        auto *header = new QLabel("✦ CONFIGURAÇÕES ✦", this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(QString("font: bold 32pt 'Helvetica'; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        mainLayout->addWidget(header);

        // ScrollArea pra quando tiver muita opção no menu
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: transparent; }");

        auto *scrollContent = new QFrame();
        scrollContent->setObjectName("scrollContent");
        // Estilo da moldura branca redondinha
        scrollContent->setStyleSheet(QString(
            "QFrame#scrollContent { background-color: %1; border: 4px solid %2; border-radius: 20px; }"
        ).arg(CREME).arg(ROSA_BORDA));
        
        auto *scrollLayout = new QVBoxLayout(scrollContent);
        scrollLayout->setContentsMargins(20, 20, 20, 20);
        
        // --- Adicionando os botõezinhos no menu ---
        addToggle(scrollLayout, "📖 Modo Leitura", "Foca o texto no centro.", &prefs.reading_mode);
        addSeparator(scrollLayout);
        addColorblindSelect(scrollLayout);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "🚫 Reduzir Movimento", "Para as pétalas.", &prefs.reduce_motion);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "◑ Alto Contraste", "Cores vibrantes.", &prefs.high_contrast);
        addSeparator(scrollLayout);
        addSlider(scrollLayout);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "🖱 Cursor Personalizado", "Aumenta o mouse.", &prefs.custom_cursor);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "🔲 Foco Visível", "Bordas no teclado.", &prefs.visible_focus);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "🔊 Falar Texto", "Ativa o narrador em Português.", &prefs.text_to_speech);
        addSeparator(scrollLayout);
        addToggle(scrollLayout, "⌨ Atalhos de Teclado", "Números 1-9 para opções.", &prefs.keyboard_shortcuts);

        scroll->setWidget(scrollContent);
        mainLayout->addWidget(scroll);

        // Botão de fechar e salvar tudo
        auto *btnClose = new QPushButton("✅ Sair e Salvar", this);
        btnClose->setCursor(Qt::PointingHandCursor);
        btnClose->setMinimumHeight(50);
        btnClose->setStyleSheet(QString(
            "QPushButton { background-color: %1; color: white; font: bold 16pt; border-radius: 10px; }"
            "QPushButton:hover { background-color: #b388ff; }"
        ).arg(LILAS));
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(btnClose);

        if (!prefs.reduce_motion) petalCanvas->start();

        // Gambiarra necessária pro Wayland (Linux) não crashar ao abrir tela cheia direto
        QTimer::singleShot(0, this, [this](){ showFullScreen(); });
    }

protected:
    // Garante que as pétalas acompanhem o tamanho da janela
    void resizeEvent(QResizeEvent *event) override {
        petalCanvas->setGeometry(0, 0, width(), height());
        QDialog::resizeEvent(event);
    }

private:
    Prefs &prefs;
    std::function<void(const Prefs&)> applyCallback;
    PetalCanvas *petalCanvas;

    // Linha de separação bonitinha entre as opções
    void addSeparator(QVBoxLayout *layout) {
        auto *line = new QFrame();
        line->setFixedHeight(2);
        line->setStyleSheet(QString("background: %1;").arg(ROSA_BORDA));
        layout->addWidget(line);
    }

    // Cria um interruptor (Checkbox) com descrição
    void addToggle(QVBoxLayout *layout, QString title, QString desc, bool *val) {
        auto *row = new QFrame();
        auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel(QString("<b>%1</b><br>%2").arg(title).arg(desc));
        lTitle->setStyleSheet(QString("font-size: 14pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *cb = new QCheckBox();
        cb->setChecked(*val);
        cb->setFixedSize(35, 35);
        cb->setStyleSheet("QCheckBox::indicator { width: 30px; height: 30px; border: 2px solid #C9A0DC; border-radius: 5px; }"
                          "QCheckBox::indicator:checked { background-color: #C9A0DC; }");
        connect(cb, &QCheckBox::toggled, this, [=](bool checked){ *val = checked; applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1);
        rowLayout->addWidget(cb);
        layout->addWidget(row);
    }

    // Cria o seletor de modo daltônico (Combobox)
    void addColorblindSelect(QVBoxLayout *layout) {
        auto *row = new QFrame();
        auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel("<b>🎨 Modo Daltônico</b>");
        lTitle->setStyleSheet(QString("font-size: 14pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *combo = new QComboBox();
        combo->addItems({"Nenhum", "Deuteranopia", "Protanopia", "Tritanopia"});
        std::map<QString, QString> displayToInternal = {{"Nenhum", "none"}, {"Deuteranopia", "deuteranopia"}, {"Protanopia", "protanopia"}, {"Tritanopia", "tritanopia"}};
        std::map<QString, QString> internalToDisplay;
        for(auto const& [k, v] : displayToInternal) internalToDisplay[v] = k;
        combo->setCurrentText(internalToDisplay[prefs.colorblind]);
        combo->setStyleSheet("QComboBox { border: 2px solid #C9A0DC; border-radius: 5px; padding: 5px; font-size: 12pt; }");
        connect(combo, &QComboBox::currentTextChanged, this, [=, map=displayToInternal](QString t){ prefs.colorblind = map.at(t); applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1);
        rowLayout->addWidget(combo);
        layout->addWidget(row);
    }

    // Cria a barra de ajuste do tamanho das letras (Slider)
    void addSlider(QVBoxLayout *layout) {
        auto *row = new QFrame();
        auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel("<b>🔤 Tamanho do Texto</b>");
        lTitle->setStyleSheet(QString("font-size: 14pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *slider = new QSlider(Qt::Horizontal);
        slider->setRange(80, 160);
        slider->setValue(prefs.font_scale);
        connect(slider, &QSlider::valueChanged, this, [=](int v){ prefs.font_scale = v; applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1);
        rowLayout->addWidget(slider);
        layout->addWidget(row);
    }

    // Se a pessoa fizer bagunça nas configs, esse botão volta tudo pro padrão
    void resetDefaults() {
        prefs = Prefs();
        applyCallback(prefs);
        accept();
    }
};

// --- A Visual Novel de Verdade ---
class VisualNovel : public QMainWindow {
public:
    VisualNovel() {
        setWindowTitle("Clube de Literatura");
        resize(1280, 720);
        loadPrefs(); // Carrega o que tava salvo no JSON
        
        // Inicia o narrador e configura pro PT-BR
        speech = new QTextToSpeech(this);
        setupVoice();

        setupUI();     // Cria a tela (labels, frames, etc)
        setupScenes(); // Carrega a história (roteiro)
        applyPrefs(prefs);
        goToScene("inicio_bedroom"); // Começa do começo
    }

private:
    Prefs prefs;
    QTextToSpeech *speech;
    QLabel *bgLabel;
    QFrame *textFrame;
    QLabel *textLabel;
    QFrame *choicesFrame;
    QVBoxLayout *choicesLayout;
    QPushButton *btnA11y;
    QString currentSceneId;
    std::map<QString, Scene> scenes;

    // Configura a voz pra não parecer um robô americano falando português
    void setupVoice() {
        QLocale locale(QLocale::Portuguese, QLocale::Brazil);
        speech->setLocale(locale);
        
        // Tenta achar uma voz brasileira real no sistema
        const auto voices = speech->availableVoices();
        for (const QVoice &voice : voices) {
            if (voice.locale().language() == QLocale::Portuguese && 
                (voice.locale().country() == QLocale::Brazil || voice.locale().territory() == QLocale::Brazil)) {
                speech->setVoice(voice);
                break;
            }
        }
        
        speech->setRate(-0.1); // Fala um pouco mais calmo
        speech->setPitch(0.0);
    }

    // Monta os componentes básicos da janela
    void setupUI() {
        auto *central = new QWidget(this);
        setCentralWidget(central);
        bgLabel = new QLabel(central);
        bgLabel->setGeometry(0, 0, 1280, 720);
        bgLabel->setScaledContents(true);
        textFrame = new QFrame(central);
        textLabel = new QLabel(textFrame);
        textLabel->setWordWrap(true);
        textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        choicesFrame = new QFrame(central);
        choicesFrame->setStyleSheet("background: transparent;");
        choicesLayout = new QVBoxLayout(choicesFrame);
        choicesLayout->setAlignment(Qt::AlignCenter);
        btnA11y = new QPushButton("⚙ Acessibilidade", central);
        btnA11y->setCursor(Qt::PointingHandCursor);
        connect(btnA11y, &QPushButton::clicked, this, &VisualNovel::openA11yMenu);
    }

    // Aqui tá o roteiro do jogo todo. É só copiar e mudar os textos se quiser mais cenas!
    void setupScenes() {
        QString base = QDir::currentPath() + "/assets/";
        scenes["inicio_bedroom"] = {base + "bedroom.png", "Manhã de segunda-feira. O sol mal iluminava meu quarto...\nEu estava exausto das jogatinas, mas o Clube de Literatura me aguardava.", 
            {{"Levantar imediatamente e ir cedo para a escola", "cena_chegar_cedo"}, {"Dormir só mais 5 minutinhos...", "cena_chegar_atrasado"}}};
        scenes["cena_chegar_cedo"] = {base + "class.png", "Cheguei na sala de aula. Yuri já estava lá no canto, lendo um livro envolvente em silêncio.\nO que devo fazer?", 
            {{"Sentar ao lado dela e tentar ler também", "final_yuri"}, {"Começar a escrever poemas na minha carteira", "final_poema"}}};
        scenes["cena_chegar_atrasado"] = {base + "class.png", "Entrei correndo e ofegante na sala. Natsuki, irritada, cruzou os braços:\n'Sempre deixando as pessoas esperando, né?!'", 
            {{"Pedir miiiil desculpas e dar um bolinho de presente", "final_natsuki"}, {"Ser sarcástico e sentar direto sem ligar pra ela", "final_rude"}}};
        scenes["final_yuri"] = {base + "class.png", "Yuri percebe seu olhar, cora levemente e compartilha um pedaço do livro com você.\n\n[ FINAL 1: A Rota Sombria e Silenciosa ]", {}};
        scenes["final_poema"] = {base + "class.png", "A presidente Monika adorou o seu poema!\n...Na verdade, ela adora tudo em você.\n\n[ FINAL 2: Só Monika ]", {}};
        scenes["final_natsuki"] = {base + "class.png", "Ela abre um sorrisinho, desvia o olhar e diz:\n'N-não é como se eu tivesse gostado que você chegou... Baka!'\n\n[ FINAL 3: A Rota Doce ]", {}};
        scenes["final_rude"] = {base + "class.png", "Dá uma treta enorme no clube. Sayori tenta separar, mas o clima fica péssimo. Você volta pra casa.\n\n[ FINAL 4: Um Clube Desfeito ]", {}};
    }

    // A função principal: troca o fundo, o texto e os botões quando você muda de cena
    void goToScene(QString id) {
        speech->stop(); // Cala a boca do narrador se ele tiver falando da cena antiga
        currentSceneId = id;
        Scene &scene = scenes[id];
        
        // Troca a imagem de fundo
        QPixmap pix(scene.bgPath);
        if (!pix.isNull()) bgLabel->setPixmap(pix);
        else bgLabel->setStyleSheet(QString("background-color: %1;").arg(ROSA));
        
        textLabel->setText(scene.text);

        // Se a voz de acessibilidade tiver ligada, manda o narrador ler a cena
        if (prefs.text_to_speech) {
            QString speakText = scene.text;
            if (!scene.choices.empty()) {
                speakText += ". ... . Escolha uma opção: ... ."; // Pausas dramáticas
                for(int i=0; i<(int)scene.choices.size(); ++i) {
                    speakText += QString(" Opção %1: %2. ... .").arg(i+1).arg(scene.choices[i].text);
                }
            }
            speech->say(speakText);
        }

        // Deleta os botões da cena antiga e cria os novos
        QLayoutItem *item;
        while ((item = choicesLayout->takeAt(0)) != nullptr) {
            if (item->widget()) delete item->widget();
            delete item;
        }

        auto addButton = [&](QString txt, std::function<void()> cb) {
            auto *btn = new QPushButton(txt);
            btn->setFixedWidth(600);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; font: bold %3pt; border: 3px raised %1; border-radius: 10px; padding: 10px; }"
                                       "QPushButton:hover { background-color: %4; color: white; }")
                                       .arg(prefs.high_contrast ? HC_BG : CREME).arg(prefs.high_contrast ? HC_ACCENT : "#8B0086")
                                       .arg(qMax(10, (int)(12 * prefs.font_scale / 100))).arg(prefs.high_contrast ? HC_PANEL : LILAS));
            connect(btn, &QPushButton::clicked, this, [=, cb=cb](){ speech->stop(); cb(); });
            choicesLayout->addWidget(btn);
        };

        if (scene.choices.empty()) {
            QString txt = ">> Voltar ao início <<";
            if (prefs.keyboard_shortcuts) txt = "[1] " + txt;
            addButton(txt, [=](){ goToScene("inicio_bedroom"); });
        } else {
            for(int i=0; i<(int)scene.choices.size(); ++i) {
                QString txt = scene.choices[i].text;
                if (prefs.keyboard_shortcuts) txt = QString("[%1] %2").arg(i+1).arg(txt);
                addButton(txt, [=, next=scene.choices[i].nextScene](){ goToScene(next); });
            }
        }
    }

    // Abre o menu gigante de configurações
    void openA11yMenu() {
        speech->stop();
        AccessibilityMenu menu(this, prefs, [=](const Prefs &p){ applyPrefs(p); savePrefs(); });
        if (menu.exec() == QDialog::Accepted) {
            // Se o player mudou algo importante, recarrega a cena pra aplicar visual e voz
            goToScene(currentSceneId);
        }
    }

    // Aplica todas as cores e tamanhos que o usuário escolheu
    void applyPrefs(const Prefs &p) {
        prefs = p;
        struct Palette { QString bg, panel, accent, text, border; };
        // Mapas de cores pro daltonismo
        std::map<QString, Palette> pals = {{"none", {ROSA, BRANCO, LILAS, TEXTO_ESCURO, ROSA}}, {"deuteranopia", {"#005b96", "#b3cde0", "#ffc100", "#000000", "#005b96"}}, {"protanopia", {"#005b96", "#b3cde0", "#ffc100", "#000000", "#005b96"}}, {"tritanopia", {"#c81d25", "#fbc4ab", "#0081a7", "#000000", "#c81d25"}}};
        auto pal = pals[prefs.colorblind];
        
        // Aplica Alto Contraste ou Cores Normais/Daltonismo
        if (prefs.high_contrast) {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(HC_BG));
            textFrame->setStyleSheet(QString("background-color: %1; border: 5px solid %2; border-radius: 15px;").arg(HC_PANEL).arg(HC_BORDER));
            textLabel->setStyleSheet(QString("background-color: %1; color: %2; font: bold %3pt; padding: 15px; border-radius: 10px;").arg(HC_PANEL).arg(HC_TEXT).arg((int)(16 * prefs.font_scale/100)));
            btnA11y->setStyleSheet(QString("background-color: %1; color: %2; font: bold 11pt; border-radius: 5px;").arg(HC_ACCENT).arg(HC_BG));
        } else {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(pal.bg));
            textFrame->setStyleSheet(QString("background-color: %1; border: 5px solid %2; border-radius: 15px;").arg(pal.border));
            textLabel->setStyleSheet(QString("background-color: %1; color: %2; font: bold %3pt; padding: 15px; border-radius: 10px;").arg(pal.panel).arg(pal.text).arg((int)(16 * prefs.font_scale/100)));
            btnA11y->setStyleSheet(QString("background-color: %1; color: %2; font: bold 11pt; border-radius: 5px;").arg(pal.accent).arg(pal.panel));
        }
        
        // Muda o layout se o "Modo Leitura" tiver ligado (deixa o texto mais focado)
        if (prefs.reading_mode) { textFrame->setGeometry(1280*0.2, 720*0.55, 1280*0.6, 720*0.35); textLabel->setGeometry(20, 20, (1280*0.6)-40, (720*0.35)-40); }
        else { textFrame->setGeometry(1280*0.1, 720*0.7, 1280*0.8, 720*0.25); textLabel->setGeometry(20, 20, (1280*0.8)-40, (720*0.25)-40); }
        
        choicesFrame->setGeometry(0, 720*0.2, 1280, 720*0.4);
        btnA11y->setGeometry(1100, 20, 160, 40);
        setCursor(prefs.custom_cursor ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }

    // Lê o arquivo de preferências
    void loadPrefs() {
        QFile f(QDir::currentPath() + "/a11y_prefs.json");
        if (f.open(QIODevice::ReadOnly)) prefs = Prefs::fromJson(QJsonDocument::fromJson(f.readAll()).object());
    }
    // Salva o arquivo de preferências
    void savePrefs() {
        QFile f(QDir::currentPath() + "/a11y_prefs.json");
        if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(prefs.toJson()).toJson());
    }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    VisualNovel v;
    v.show();
    return a.exec();
}
