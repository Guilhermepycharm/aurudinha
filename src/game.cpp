#include <QApplication>
#include <QCoreApplication>
#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QFrame>
#include <QCheckBox>
#include <QComboBox>
#include <QSlider>
#ifndef __EMSCRIPTEN__
#include <QTextToSpeech>
#endif
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
#include <QDebug>
#include <vector>
#include <map>
#include <functional>

// --- Cores do jogo ---
const QString ROSA = "#FFB7C5";
const QString CREME = "#FFF0F5";
const QString LILAS = "#C9A0DC";
const QString TEXTO_ESCURO = "#3D2B3D";
const QString BRANCO = "#FFFFFF";
const QString HC_BG = "#000000";
const QString HC_PANEL = "#1a1a1a";
const QString HC_ACCENT = "#FFD700";
const QString HC_TEXT = "#FFFFFF";
const QString HC_BORDER = "#FFFF00";

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

struct Choice { QString text; QString nextScene; };
struct Scene { QString text; QString background; QString centerSprite; QString leftSprite; QString rightSprite; std::vector<Choice> choices; };

// --- Efeito das Pétalas ---
struct Petal {
    float x, y, size, speed, drift;
    void reset(int w, int h) {
        x = QRandomGenerator::global()->bounded(w > 0 ? w : 1280);
        y = -20 - QRandomGenerator::global()->bounded(200);
        size = 10 + QRandomGenerator::global()->bounded(15);
        speed = 1.0 + QRandomGenerator::global()->generateDouble() * 2.5;
        drift = -1.0 + QRandomGenerator::global()->generateDouble() * 2.0;
    }
};

class PetalCanvas : public QWidget {
public:
    PetalCanvas(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        for(int i=0; i<30; ++i) { Petal p; p.reset(1920, 1080); petals.push_back(p); }
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &PetalCanvas::updateAnimation);
    }
    void start() { timer->start(30); }
    void stop() { timer->stop(); update(); }
protected:
    void paintEvent(QPaintEvent *) override {
        if (timer->isActive()) {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setBrush(Qt::white);
            painter.setPen(QPen(QColor(ROSA), 1));
            for(auto &p : petals) { painter.drawEllipse(p.x, p.y, p.size, p.size); }
        }
    }
    void updateAnimation() {
        for(auto &p : petals) { p.y += p.speed; p.x += p.drift; if (p.y > height()) p.reset(width(), height()); }
        update();
    }
private:
    std::vector<Petal> petals; QTimer *timer;
};

// --- Menu Acessibilidade ---
class AccessibilityMenu : public QDialog {
public:
    AccessibilityMenu(QWidget *parent, Prefs &currentPrefs, std::function<void(const Prefs&)> onApply) 
        : QDialog(parent), prefs(currentPrefs), applyCallback(onApply) {
        setModal(true); setStyleSheet(QString("background-color: %1;").arg(ROSA));
        petalCanvas = new PetalCanvas(this); petalCanvas->lower();
        auto *mainLayout = new QVBoxLayout(this); mainLayout->setContentsMargins(50, 50, 50, 50);
        auto *header = new QLabel("✦ CONFIGURAÇÕES ✦", this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(QString("font: bold 35pt 'Helvetica'; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        mainLayout->addWidget(header);
        auto *scroll = new QScrollArea(this); scroll->setWidgetResizable(true); scroll->setFrameShape(QFrame::NoFrame); scroll->setStyleSheet("QScrollArea { background-color: transparent; }");
        auto *scrollContent = new QFrame(); scrollContent->setObjectName("scrollContent");
        scrollContent->setStyleSheet(QString("QFrame#scrollContent { background-color: %1; border: 4px solid #C9A0DC; border-radius: 20px; }").arg(CREME));
        auto *scrollLayout = new QVBoxLayout(scrollContent); scrollLayout->setContentsMargins(30, 30, 30, 30);
        addToggle(scrollLayout, "📖 Modo Leitura", "Foca o texto no centro.", &prefs.reading_mode);
        addColorblindSelect(scrollLayout);
        addToggle(scrollLayout, "🚫 Reduzir Movimento", "Para as pétalas.", &prefs.reduce_motion);
        addToggle(scrollLayout, "◑ Alto Contraste", "Cores vibrantes.", &prefs.high_contrast);
        addSlider(scrollLayout, "🔤 Tamanho do Texto", &prefs.font_scale);
        addToggle(scrollLayout, "🖱 Cursor Personalizado", "Aumenta o mouse.", &prefs.custom_cursor);
#ifndef __EMSCRIPTEN__
        addToggle(scrollLayout, "🔊 Falar Texto", "Narrador PT-BR.", &prefs.text_to_speech);
#endif
        scroll->setWidget(scrollContent); mainLayout->addWidget(scroll);
        auto *btnClose = new QPushButton("✅ Sair e Salvar", this); btnClose->setMinimumHeight(60);
        btnClose->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font: bold 18pt; border-radius: 15px; }").arg(LILAS));
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(btnClose);
        if (!prefs.reduce_motion) petalCanvas->start();
        QTimer::singleShot(0, this, [this](){ showFullScreen(); });
    }
protected:
    void resizeEvent(QResizeEvent *e) override { petalCanvas->setGeometry(0, 0, width(), height()); QDialog::resizeEvent(e); }
private:
    Prefs &prefs; std::function<void(const Prefs&)> applyCallback; PetalCanvas *petalCanvas;
    void addToggle(QVBoxLayout *layout, QString title, QString desc, bool *val) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel(QString("<b>%1</b><br>%2").arg(title).arg(desc));
        lTitle->setStyleSheet(QString("font-size: 16pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *cb = new QCheckBox(); cb->setChecked(*val); cb->setFixedSize(40, 40);
        cb->setStyleSheet("QCheckBox::indicator { width: 35px; height: 35px; border: 3px solid #C9A0DC; border-radius: 8px; } QCheckBox::indicator:checked { background-color: #C9A0DC; }");
        connect(cb, &QCheckBox::toggled, this, [=](bool checked){ *val = checked; applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(cb); layout->addWidget(row);
    }
    void addColorblindSelect(QVBoxLayout *layout) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel("<b>🎨 Modo Daltônico</b>");
        lTitle->setStyleSheet(QString("font-size: 16pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *combo = new QComboBox(); combo->addItems({"Nenhum", "Deuteranopia", "Protanopia", "Tritanopia"});
        std::map<QString, QString> m = {{"Nenhum", "none"}, {"Deuteranopia", "deuteranopia"}, {"Protanopia", "protanopia"}, {"Tritanopia", "tritanopia"}};
        std::map<QString, QString> im; for(auto const& [k, v] : m) im[v] = k;
        combo->setCurrentText(im[prefs.colorblind]); combo->setStyleSheet("QComboBox { border: 2px solid #C9A0DC; border-radius: 5px; padding: 5px; font-size: 14pt; }");
        connect(combo, &QComboBox::currentTextChanged, this, [=, m=m](QString t){ prefs.colorblind = m.at(t); applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(combo); layout->addWidget(row);
    }
    void addSlider(QVBoxLayout *layout, QString title, int *val) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel(QString("<b>%1</b>").arg(title));
        lTitle->setStyleSheet(QString("font-size: 16pt; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        auto *slider = new QSlider(Qt::Horizontal); slider->setRange(80, 160); slider->setValue(*val);
        connect(slider, &QSlider::valueChanged, this, [=](int v){ *val = v; applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(slider); layout->addWidget(row);
    }
};

// --- Menu Principal ---
class MainMenu : public QDialog {
public:
    MainMenu(Prefs &prefsRef, std::function<void()> onPlay, std::function<void()> onA11y)
        : QDialog(nullptr), prefs(prefsRef), playCallback(onPlay), a11yCallback(onA11y) {
        setWindowTitle("Aurudinha");
        setModal(false);
        setStyleSheet(QString("background-color: %1;").arg(ROSA));
        auto *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);
        layout->setContentsMargins(80, 80, 80, 80);

        auto *title = new QLabel("✦ AURUDINHA ✦", this);
        title->setAlignment(Qt::AlignCenter);
        title->setStyleSheet(QString("font: bold 48pt 'Helvetica'; color: %1; background: transparent; margin-bottom: 30px;").arg(TEXTO_ESCURO));
        layout->addWidget(title);

        auto *btnPlay = new QPushButton("🎮  Jogar", this);
        btnPlay->setMinimumSize(400, 70);
        btnPlay->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font: bold 22pt 'Helvetica'; border-radius: 15px; padding: 15px; } QPushButton:hover { background-color: #E8A0B0; }").arg(LILAS));
        connect(btnPlay, &QPushButton::clicked, this, [this](){ accept(); playCallback(); });
        layout->addWidget(btnPlay);

        auto *btnA11y = new QPushButton("⚙  Configurações", this);
        btnA11y->setMinimumSize(400, 70);
        btnA11y->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font: bold 22pt 'Helvetica'; border-radius: 15px; padding: 15px; } QPushButton:hover { background-color: #E8A0B0; }").arg(LILAS));
        connect(btnA11y, &QPushButton::clicked, this, [this](){ a11yCallback(); });
        layout->addWidget(btnA11y);

        auto *btnExit = new QPushButton("🚪  Sair", this);
        btnExit->setMinimumSize(400, 70);
        btnExit->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font: bold 22pt 'Helvetica'; border-radius: 15px; padding: 15px; } QPushButton:hover { background-color: #E8A0B0; }").arg(LILAS));
        connect(btnExit, &QPushButton::clicked, this, &QDialog::reject);
        layout->addWidget(btnExit);

        // Pétalas decorativas
        petalCanvas = new PetalCanvas(this);
        petalCanvas->lower();
        if (!prefs.reduce_motion) petalCanvas->start();

        resize(800, 600);
        showFullScreen();
    }
protected:
    void resizeEvent(QResizeEvent *e) override { petalCanvas->setGeometry(0, 0, width(), height()); QDialog::resizeEvent(e); }
private:
    Prefs &prefs;
    std::function<void()> playCallback;
    std::function<void()> a11yCallback;
    PetalCanvas *petalCanvas;
};

class VisualNovel : public QMainWindow {
public:
    VisualNovel() {
        setWindowTitle("Aurudinha");
        loadPrefs();
#ifndef __EMSCRIPTEN__
        speech = new QTextToSpeech(this); setupVoice();
#endif
        setupUI(); setupScenes(); applyPrefs(prefs); goToScene("inicio"); showFullScreen();
    }

protected:
    void resizeEvent(QResizeEvent *e) override { applyPrefs(prefs); QMainWindow::resizeEvent(e); }
    void keyPressEvent(QKeyEvent *e) override {
        if (prefs.keyboard_shortcuts && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9) {
            int idx = e->key() - Qt::Key_1;
            auto it = scenes.find(currentSceneId);
            if (it != scenes.end() && idx < (int)it->second.choices.size()) goToScene(it->second.choices[idx].nextScene);
        }
        QMainWindow::keyPressEvent(e);
    }

private:
    Prefs prefs;
#ifndef __EMSCRIPTEN__
    QTextToSpeech *speech;
#endif
    QLabel *bgLabel, *spriteCenter, *spriteLeft, *spriteRight, *textLabel;
    QFrame *textFrame, *choicesFrame;
    QVBoxLayout *choicesLayout;
    QPushButton *btnA11y, *btnScrollUp, *btnScrollDown;
    QScrollArea *scrollArea;
    QString currentSceneId;
    std::map<QString, Scene> scenes;

    void setupVoice() {
#ifndef __EMSCRIPTEN__
        QLocale locale(QLocale::Portuguese, QLocale::Brazil);
        speech->setLocale(locale); speech->setRate(-0.1);
#endif
    }

    void setupUI() {
        auto *central = new QWidget(this); setCentralWidget(central);
        bgLabel = new QLabel(central);
        bgLabel->setStyleSheet("background: transparent;");
        spriteLeft = new QLabel(central); spriteLeft->setAlignment(Qt::AlignLeft | Qt::AlignBottom); spriteLeft->setStyleSheet("background: transparent; border: none;");
        spriteRight = new QLabel(central); spriteRight->setAlignment(Qt::AlignRight | Qt::AlignBottom); spriteRight->setStyleSheet("background: transparent; border: none;");
        spriteCenter = new QLabel(central); spriteCenter->setAlignment(Qt::AlignHCenter | Qt::AlignBottom); spriteCenter->setStyleSheet("background: transparent; border: none;");
        // Text area com scroll
        textFrame = new QFrame(central);
        textFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        textFrame->setStyleSheet("background-color: rgba(255, 255, 255, 120); border-radius: 15px; border: 2px solid rgba(255, 183, 197, 100);");
        auto *textFrameLayout = new QVBoxLayout(textFrame);
        textFrameLayout->setContentsMargins(0, 0, 0, 0);
        textFrameLayout->setSpacing(0);

        // Botão subir
        btnScrollUp = new QPushButton("▲", textFrame);
        btnScrollUp->setFixedHeight(28);
        btnScrollUp->setCursor(Qt::PointingHandCursor);
        btnScrollUp->setStyleSheet("QPushButton { background: rgba(201,160,220,180); color: white; font: bold 14pt; border: none; border-radius: 10px 10px 0 0; } QPushButton:hover { background: rgba(201,160,220,230); }");
        textFrameLayout->addWidget(btnScrollUp);

        // Scroll area com o texto
        scrollArea = new QScrollArea(textFrame);
        scrollArea->setWidgetResizable(true);
        scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; } QScrollBar:vertical { background: rgba(255,255,255,80); width: 10px; border-radius: 5px; } QScrollBar::handle:vertical { background: rgba(201,160,220,180); border-radius: 5px; min-height: 30px; } QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }");
        textLabel = new QLabel(); textLabel->setWordWrap(true); textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        textLabel->setContentsMargins(15, 10, 15, 10);
        textLabel->setStyleSheet("background: transparent;");
        scrollArea->setWidget(textLabel);
        textFrameLayout->addWidget(scrollArea);

        // Botão descer
        btnScrollDown = new QPushButton("▼", textFrame);
        btnScrollDown->setFixedHeight(28);
        btnScrollDown->setCursor(Qt::PointingHandCursor);
        btnScrollDown->setStyleSheet("QPushButton { background: rgba(201,160,220,180); color: white; font: bold 14pt; border: none; border-radius: 0 0 10px 10px; } QPushButton:hover { background: rgba(201,160,220,230); }");
        textFrameLayout->addWidget(btnScrollDown);

        // Conectar botões de scroll
        connect(btnScrollUp, &QPushButton::clicked, this, [this]() {
            auto *sb = scrollArea->verticalScrollBar();
            sb->setValue(sb->value() - sb->pageStep() / 2);
        });
        connect(btnScrollDown, &QPushButton::clicked, this, [this]() {
            auto *sb = scrollArea->verticalScrollBar();
            sb->setValue(sb->value() + sb->pageStep() / 2);
        });

        choicesFrame = new QFrame(central); choicesLayout = new QVBoxLayout(choicesFrame); choicesLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter); choicesLayout->setSpacing(10);
        choicesFrame->setStyleSheet("background: transparent; border: none;");
        btnA11y = new QPushButton("⚙ Configs", central); btnA11y->setCursor(Qt::PointingHandCursor);
        connect(btnA11y, &QPushButton::clicked, this, &VisualNovel::openA11yMenu);
    }

    void setupScenes() {
        QString s = QCoreApplication::applicationDirPath() + "/assets/sprites/";
        QString b = QCoreApplication::applicationDirPath() + "/assets/backgrounds/";
        // === ATO 1 — O DESPERTAR ===
        scenes["inicio"] = {
            "A luz fraca da manhã entra pela fresta da cortina. Aurudinha acorda devagar, com a mente ainda confusa, como se o mundo ao redor fosse um borrão. Ela coça os olhos e se senta na cama. Tudo parece pesado demais para uma manhã comum.\n\nEntão, um barulho. Vozes vindas da sala. Sua mãe e seu pai conversam — ou melhor, discutem. As palavras chegam abafadas, mas o tom é cortante.\n\nAurudinha fica parada, sem saber o que fazer.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Ir ver o que aconteceu", "ir_ver"}, {"Permanecer no quarto", "ficar_quarto"}}
        };
        // === IR VER — PARTE 1 ===
        scenes["ir_ver"] = {
            "Aurudinha se levanta da cama, o coração acelerado. Ela caminha até a porta do quarto, abre devagar e segue pelo corredor. A cada passo, as vozes ficam mais claras. Ela se esconde atrás da parede da sala, sem conseguir se mover.\n\nSua mãe fala com uma voz carregada de frustração:\n\n— Você sabe o quão difícil é cuidar de crianças com essa 'doença'... Eu não queria um filho assim. Teria sido melhor se a gente não tivesse ido atrás de um diagnóstico. Será que esse diagnóstico é mesmo verdadeiro?",
            b+"saladacasa.png", "",
            s+"mae/maeseria_no_bg_t4bqtyzu.png",
            s+"pai/paitriste-removebg-preview.png",
            {{"Continuar ouvindo", "ir_ver_2"}}
        };
        // === IR VER — PARTE 2 ===
        scenes["ir_ver_2"] = {
            "Seu pai responde, a voz angustiada, como se cada palavra custasse:\n\n— Devíamos ter prestado mais atenção... Mas não, não podemos fazer nada agora. É aceitar esse diagnóstico. Não quero passar vergonha. Não quero ter um filho que tem essa 'doença'.\n\nAurudinha sente o mundo desabar. Cada palavra é como um golpe. Ela se segura para não fazer barulho, as lágrimas já escorrendo sem permissão.\n\nSe eles têm vergonha... de mim?",
            b+"saladacasa.png", "",
            s+"mae/maeseria_no_bg_t4bqtyzu.png",
            s+"pai/paitriste-removebg-preview.png",
            {{"Voltar para o quarto", "choro_quarto"}}
        };
        // === CHORO NO QUARTO ===
        scenes["choro_quarto"] = {
            "Aurudinha volta para o quarto em silêncio, fechando a porta com cuidado. Se joga na cama e o choro que estava preso finalmente escapa — mas ela tenta abafar, cobrindo o rosto com o travesseiro.\n\nNa sua cabeça, uma voz insiste:\n\n— Se eu não tivesse nascido autista... meus pais não estariam brigando. A culpa é minha. Eles têm vergonha de mim.\n\nEla seca as lágrimas ao ouvir seu pai chamando:\n\n— Aurudinha! Vamos, você vai se atrasar para a escola!\n\nEla respira fundo, se arruma e tenta compor o rosto. Ninguém pode saber.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "", "",
            {{"Ir para a escola", "escola"}}
        };
        // === FICAR NO QUARTO ===
        scenes["ficar_quarto"] = {
            "Aurudinha decide não ir ver. Talvez seja melhor assim. Ficar no quarto, no silêncio, longe daquelas vozes. Ela pega o celular e fica rolando a tela sem realmente ver nada, tentando ocupar a mente.\n\nOs minutos passam lentos. O barulho na sala diminui, mas o peso no peito não.\n\nEntão, a voz do pai:\n\n— Aurudinha! Vamos, você vai se atrasar!\n\nEla guarda o celular, se arruma e sai do quarto sem olhar para ninguém.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Ir para a escola", "escola"}}
        };
        // === ATO 2 — A ESCOLA ===
        scenes["escola"] = {
            "A escola. O portão se abre e Aurudinha entra junto com outras crianças. O barulho já começa aqui — vozes, risadas, passos no corredor. Tudo parece alto demais.\n\nLucas, um colega, se aproxima:\n\n— Oi, Aurudinha! Bom dia!\n\nAurudinha sai do seu transe:\n\n— Oi, Lucas...\n\n— Vamos para a aula? A professora mandou te chamar.\n\nEla acena com a cabeça e segue em direção à sala. Cada passo no corredor parece amplificado.",
            b+"frentedaescola.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Entrar na sala", "sala_aula"}}
        };
        // === SALA DE AULA ===
        scenes["sala_aula"] = {
            "A sala de aula está cheia. A aula já começou há um tempo, mas o barulho das crianças conversando não diminui. Vozes se misturam, carteiras arranham no chão, alguém ri alto no fundo.\n\nAurudinha senta na sua carteira e tenta se concentrar. Mas o barulho cresce. E cresce. Seu corpo começa a tremer. Suas mãos ficam frias. O coração dispara.\n\nEla sente que o mundo ao redor está se fechando, como se as paredes estivessem se aproximando. Cada som é uma agressão. Ela coloca as mãos sobre os olhos, tentando se proteger.\n\n— Eu não consigo... eu não consigo ficar aqui...",
            b+"saladeaula.png",
            s+"aurudinha/aurudinhamaonacabeça_no_bg_lwjhxt3z.png", "", "",
            {{"Pedir ajuda à professora", "pedir_ajuda"}, {"Sair correndo", "correr"}}
        };
        // === PEDIR AJUDA ===
        scenes["pedir_ajuda"] = {
            "Aurudinha se levanta, as pernas tremendo, e vai até a professora. Sua voz sai fraca:\n\n— Professora... eu não estou me sentindo bem...\n\nA professora percebe o sofrimento no rosto dela e responde com gentileza:\n\n— Calma, querida. Vá sentar, eu vou pedir alguém para trazer água.\n\n— Bell, por favor, traga água para a Aurudinha.\n\nBell não responde. A professora repete, agora com mais firmeza:\n\n— Bell! Eu pedi para você pegar uma água para ela!\n\nAurudinha volta para a carteira e coloca as mãos nos ouvidos, tentando abafar o barulho. Então, ouve risadas. Alguns colegas estão rindo dela. O desespero aumenta e as lágrimas começam a cair.\n\n— Por que estão rindo...?",
            b+"saladeaula2.png",
            s+"aurudinha/aurudinhamaonacabeça_no_bg_lwjhxt3z.png",
            s+"professora/professora normal ou aprenseiva_no_bg_i2annqee.png", "",
            {{"Permanecer na sala", "casa"}, {"Sair correndo", "correr"}}
        };
        // === CORRER / BULLYING ===
        scenes["correr"] = {
            "Aurudinha não aguenta mais. Seu coração está disparado, a respiração descontrolada. Ela se levanta e sai da sala correndo, sem olhar para trás.\n\nNo corredor, encontra um canto e se senta no chão, abraçando os joelhos, tentando se acalmar. Mas então... passos. Vozes se aproximando.\n\n— Olha só, é aquela menina 'doente' da sala...\n\n— Que pena. Os pais dela devem ter vergonha de uma filha que parece um monstro...\n\nO grupo começa a rir e debochar. A mente de Aurudinha entra em colapso. Tudo ao redor parece consumir seu espaço. Ela treme. Não consegue respirar.\n\n— Eu não sou um monstro... eu não sou...",
            b+"correndopelaescola.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "", "",
            {{"Fugir da escola", "atropelamento"}, {"Ficar e voltar para casa", "casa"}}
        };
        // === ATO 3 — CASA / ESCOLHA FINAL ===
        scenes["casa"] = {
            "Quebra de tempo.\n\nAurudinha chega em casa e vai direto para o seu quarto. Se joga na cama e o choro que segurou o dia inteiro finalmente vem, sem filtro, sem controle.\n\nDo outro lado da porta, seu pai ouve. Seu coração dói ao ver a expressão triste no rosto da filha. Ele não sabe a forma certa de agir — a mãe deles não aceitou que a própria filha nasceu no espectro autista, e agora a família está desmoronando.\n\nEle fica parado no corredor, em silêncio. Talvez... talvez seja melhor falar tudo para ela.\n\nAurudinha percebe uma presença. Seu pai está ali, na porta, com olhar preocupado.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "",
            s+"pai/paiolhandodelado-removebg-preview.png",
            {{"Contar tudo ao pai", "final_bom"}, {"Não contar nada", "final_ruim"}}
        };
        // === FINAL BOM ===
        scenes["final_bom"] = {
            "O pai se aproxima e gentilmente abre a porta. Ele se agacha ao lado da cama, os olhos marejados.\n\n— Você está bem, filha?\n\nAurudinha não aguenta. Corre para os braços do pai e desabafa, contando tudo — a escola, o bullying, as palavras dos pais, a dor de se sentir um fardo.\n\nO pai a abraça forte, e pela primeira vez, diz o que ela precisava ouvir:\n\n— Eu sinto muito, Aurudinha. Você não é um fardo. Você é minha filha e eu vou te apoiar. Sempre.\n\nOs dois se abraçam. A música é triste, mas há esperança. Pela primeira vez em muito tempo, Aurudinha sente que talvez... talvez tudo possa ficar bem.\n\n[ FINAL BOM — O Abraço ]",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "",
            s+"pai/paisorrindo-removebg-preview.png",
            {{"Reiniciar", "inicio"}}
        };
        // === FINAL RUIM ===
        scenes["final_ruim"] = {
            "Aurudinha decide não contar nada. Ela engole as lágrimas, limpa o rosto e segue o pai até a cozinha. Senta na cadeira à frente dele, nervosa.\n\nO pai tenta conversar, mas as palavras não saem. O silêncio entre eles é pesado, carregado de tudo o que não é dito.\n\n— Aurudinha... eu...\n\nEle não consegue terminar. Ela não consegue esperar.\n\nQuebra de tempo.\n\nO pai acorda e percebe a casa silenciosa. Estragamente silenciosa. Ele caminha até o quarto da filha e abre a porta devagar.\n\nSilêncio.\n\n[ FINAL RUIM — O Silêncio ]",
            b+"quartodaauruda.png",
            s+"aurudinha/auridnhacomolharmorto_no_bg_myl6f8jd.png", "",
            s+"pai/paitriste-removebg-preview.png",
            {{"Fim", "inicio"}}
        };
        // === FINAL TRÁGICO ===
        scenes["atropelamento"] = {
            "Aurudinha levanta do chão com a poupa força que ainda tem. Uma ideia passa pela sua mente — e se eu for embora? Talvez se eu fugir, meus pais fiquem melhor...\n\nEla sai correndo. Não olha para trás. Corre pela rua, pelo portão da escola, pela avenida. Não vê o sinal vermelho. Não vê o carro.\n\nO barulho do freio. O impacto. Tudo escurece.\n\n[ FINAL TRÁGICO ]",
            b+"frentedaescola.png",
            s+"aurudinha/auridnhacomolharmorto_no_bg_myl6f8jd.png", "", "",
            {{"Anos depois...", "nova_filha"}}
        };
        // === NOVA FILHA ===
        scenes["nova_filha"] = {
            "Quebra de tempo.\n\nAlguns anos depois.\n\nA casa que antes era silenciosa agora tem novos sons. Uma criança pequena brinca na sala. A mãe sorri, radiante, segurando sua nova filha.\n\n— Egosinha, vem cá, meu amor!\n\nO pai observa da porta, sem expressão. A nova filha se chama Egosinha. E eles vivem... 'felizes'.\n\nMas o quarto de Aurudinha continua ali. Intocado. Como um segredo que ninguém quer lembrar.\n\n[ FIM ]",
            b+"saladacasa.png", "",
            s+"mae/maefeliz_no_bg_r2s96e1i.png", "",
            {{"Reiniciar", "inicio"}}
        };
    }

    void goToScene(QString id) {
#ifndef __EMSCRIPTEN__
        speech->stop();
#endif
        currentSceneId = id; Scene &s = scenes[id]; textLabel->setText(s.text);
        scrollArea->verticalScrollBar()->setValue(0);
#ifndef __EMSCRIPTEN__
        if (prefs.text_to_speech) speech->say(s.text);
#endif
        updateSprites();
        // Limpar botões antigos
        QLayoutItem *item; while ((item = choicesLayout->takeAt(0)) != nullptr) { if (item->widget()) delete item->widget(); delete item; }
        // Criar botões de escolha
        int fontSize = qMax(12, (int)(14 * prefs.font_scale / 100));
        for (const auto &c : s.choices) {
            auto *btn = new QPushButton(c.text);
            btn->setMinimumWidth(width() * 0.4);
            btn->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
            btn->setStyleSheet(QString("QPushButton { background-color: rgba(255, 240, 245, 180); color: %1; font: bold %2pt; border: 2px solid rgba(201, 160, 220, 150); border-radius: 12px; padding: 12px 20px; } QPushButton:hover { background-color: rgba(201, 160, 220, 200); }")
                .arg(TEXTO_ESCURO).arg(fontSize));
            connect(btn, &QPushButton::clicked, this, [=](){ goToScene(c.nextScene); });
            choicesLayout->addWidget(btn);
        }
        // Auto-ajuste: calcular tamanho do texto e ajustar frames
        QTimer::singleShot(10, this, [this]() { adjustTextLayout(); });
    }

    void adjustTextLayout() {
        int w = width(), h = height();
        if (w == 0 || h == 0) return;

        // Zona dos personagens: de spriteTop até o fundo da tela
        int spriteTop = h * 0.45;
        // Espaço disponível para texto + escolhas (entre topo e personagens)
        int availableH = spriteTop - h * 0.02;
        int topMargin = h * 0.02;

        // Medir o tamanho real do texto
        int textAreaW = w * 0.84;
        QFontMetrics fm(textLabel->font());
        int textWidth = textAreaW - 40; // padding interno
        int textHeight = fm.boundingRect(QRect(0, 0, textWidth, 0), Qt::TextWordWrap, textLabel->text()).height();

        // Altura do texto: entre 15% e 50% do espaço disponível
        int minTextH = availableH * 0.15;
        int maxTextH = availableH * 0.50;
        int padding = 40;
        int targetTextH = qBound(minTextH, textHeight + padding, maxTextH);

        // Posicionar textFrame no topo da área disponível
        if (prefs.reading_mode) {
            // Centralizado na metade superior (acima dos personagens)
            int textY = topMargin + (availableH - targetTextH) / 2;
            textFrame->setGeometry(w * 0.15, textY, w * 0.7, targetTextH);
        } else {
            textFrame->setGeometry(w * 0.08, topMargin, textAreaW, targetTextH);
        }

        // Mostrar/esconder botões de scroll conforme necessidade
        QTimer::singleShot(50, this, [this]() {
            bool needsScroll = scrollArea->verticalScrollBar()->maximum() > 0;
            btnScrollUp->setVisible(needsScroll);
            btnScrollDown->setVisible(needsScroll);
        });

        // Posicionar choicesFrame abaixo do textFrame, mas acima dos personagens
        int remainingH = spriteTop - (textFrame->y() + textFrame->height()) - 10;
        int choicesH = qMax(60, remainingH);
        int choicesY = textFrame->y() + textFrame->height() + 10;
        choicesFrame->setGeometry(w * 0.1, choicesY, w * 0.8, choicesH);
    }

    void updateSprites() {
        if (currentSceneId.isEmpty()) return;
        Scene &s = scenes[currentSceneId];
        int w = width() > 0 ? width() : 1280;
        int h = height() > 0 ? height() : 720;
        bgLabel->setGeometry(0, 0, w, h);
        QPixmap bg(s.background);
        if (!bg.isNull()) {
            bgLabel->setPixmap(bg.scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        }
        int spriteH = (int)(h * 0.55);
        int spriteW = (int)(w * 0.4);
        auto load = [this, spriteW, spriteH](QLabel *l, QString p) {
            if (p.isEmpty()) { l->clear(); return; }
            QPixmap pix(p);
            if (!pix.isNull()) {
                l->setPixmap(pix.scaled(spriteW, spriteH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        };
        load(spriteCenter, s.centerSprite); load(spriteLeft, s.leftSprite); load(spriteRight, s.rightSprite);
        bgLabel->lower(); spriteLeft->raise(); spriteRight->raise(); spriteCenter->raise(); textFrame->raise(); choicesFrame->raise(); btnA11y->raise();
    }

    void openA11yMenu() { 
#ifndef __EMSCRIPTEN__
        speech->stop(); 
#endif
        AccessibilityMenu menu(this, prefs, [=](const Prefs &p){ applyPrefs(p); savePrefs(); }); 
        if (menu.exec() == QDialog::Accepted) goToScene(currentSceneId); 
    }

    void applyPrefs(const Prefs &p) {
        prefs = p;
        int w = width(), h = height();
        bgLabel->setGeometry(0, 0, w, h);
        spriteLeft->setGeometry(0, 0, w/2, h); spriteRight->setGeometry(w/2, 0, w/2, h); spriteCenter->setGeometry(0, 0, w, h);
        struct Palette { QString bg, panel, accent, text; };
        std::map<QString, Palette> pals = {{"none", {ROSA, BRANCO, LILAS, TEXTO_ESCURO}}, {"deuteranopia", {"#005b96", "#b3cde0", "#ffc100", "#000000"}}, {"protanopia", {"#005b96", "#b3cde0", "#ffc100", "#000000"}}, {"tritanopia", {"#c81d25", "#fbc4ab", "#0081a7", "#000000"}}};
        auto pal = pals[prefs.colorblind];
        int fontSize = qMax(14, (int)(20 * prefs.font_scale / 100));
        if (prefs.high_contrast) {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(HC_BG));
            textFrame->setStyleSheet(QString("background-color: rgba(26, 26, 26, 200); border: 4px solid %1; border-radius: 15px;").arg(HC_BORDER));
            textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; padding: 15px; background: transparent;").arg(HC_TEXT).arg(fontSize));
        } else {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(pal.bg));
            textFrame->setStyleSheet(QString("background-color: rgba(255, 255, 255, 120); border: 3px solid %1; border-radius: 15px;").arg(pal.accent));
            textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; padding: 15px; background: transparent;").arg(pal.text).arg(fontSize));
        }
        btnA11y->setGeometry(w - 180, 20, 160, 45);
        updateSprites();
        adjustTextLayout();
    }
    void loadPrefs() { QFile f(QCoreApplication::applicationDirPath() + "/config/a11y_prefs.json"); if (f.open(QIODevice::ReadOnly)) prefs = Prefs::fromJson(QJsonDocument::fromJson(f.readAll()).object()); }
    void savePrefs() { QFile f(QCoreApplication::applicationDirPath() + "/config/a11y_prefs.json"); if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(prefs.toJson()).toJson()); }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Prefs prefs;
    // Load prefs for the menu (petal animation, etc.)
    QFile f(QCoreApplication::applicationDirPath() + "/config/a11y_prefs.json");
    if (f.open(QIODevice::ReadOnly)) prefs = Prefs::fromJson(QJsonDocument::fromJson(f.readAll()).object());

    bool playing = false;
    bool running = true;
    while (running) {
        MainMenu menu(prefs,
            [&playing]() { playing = true; },   // Jogar
            [&prefs]() {                          // Configurações
                AccessibilityMenu a11y(nullptr, prefs, [](const Prefs&){});
                a11y.exec();
                // Save prefs after closing a11y menu
                QFile f(QCoreApplication::applicationDirPath() + "/config/a11y_prefs.json");
                if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(prefs.toJson()).toJson());
            }
        );
        if (menu.exec() == QDialog::Rejected) break; // Sair
        if (playing) {
            VisualNovel v;
            v.show();
            a.exec();
            running = false;
        }
    }
    return 0;
}
