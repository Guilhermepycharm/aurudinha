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

// --- Cores do jogo ---
const QString ROSA = "#FFB7C5";
const QString CREME = "#FFF0F5";
const QString LILAS = "#C9A0DC";
const QString TEXTO_ESCURO = "#3D2B3D";
const QString ROSA_BORDA = "#ffccd5";
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

struct Choice {
    QString text;
    QString nextScene;
};

struct Scene {
    QString text;
    QString background;
    QString centerSprite;
    QString leftSprite;
    QString rightSprite;
    std::vector<Choice> choices;
};

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
        for(int i=0; i<30; ++i) {
            Petal p; p.reset(1920, 1080);
            petals.push_back(p);
        }
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
            painter.setPen(QPen(QColor(ROSA_BORDA), 1));
            for(auto &p : petals) { painter.drawEllipse(p.x, p.y, p.size, p.size); }
        }
    }
    void updateAnimation() {
        for(auto &p : petals) {
            p.y += p.speed; p.x += p.drift;
            if (p.y > height()) p.reset(width(), height());
        }
        update();
    }
private:
    std::vector<Petal> petals;
    QTimer *timer;
};

class AccessibilityMenu : public QDialog {
public:
    AccessibilityMenu(QWidget *parent, Prefs &currentPrefs, std::function<void(const Prefs&)> onApply) 
        : QDialog(parent), prefs(currentPrefs), applyCallback(onApply) {
        setModal(true);
        setStyleSheet(QString("background-color: %1;").arg(ROSA));
        petalCanvas = new PetalCanvas(this);
        petalCanvas->lower();
        auto *mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(50, 50, 50, 50);
        auto *header = new QLabel("✦ CONFIGURAÇÕES ✦", this);
        header->setAlignment(Qt::AlignCenter);
        header->setStyleSheet(QString("font: bold 35pt 'Helvetica'; color: %1; background: transparent;").arg(TEXTO_ESCURO));
        mainLayout->addWidget(header);
        auto *scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scroll->setStyleSheet("QScrollArea { background-color: transparent; }");
        auto *scrollContent = new QFrame();
        scrollContent->setObjectName("scrollContent");
        scrollContent->setStyleSheet(QString("QFrame#scrollContent { background-color: %1; border: 4px solid %2; border-radius: 20px; }").arg(CREME).arg(ROSA_BORDA));
        auto *scrollLayout = new QVBoxLayout(scrollContent);
        scrollLayout->setContentsMargins(30, 30, 30, 30);
        addToggle(scrollLayout, "📖 Modo Leitura", "Foca o texto no centro.", &prefs.reading_mode);
        addColorblindSelect(scrollLayout);
        addToggle(scrollLayout, "🚫 Reduzir Movimento", "Para as pétalas.", &prefs.reduce_motion);
        addToggle(scrollLayout, "◑ Alto Contraste", "Cores vibrantes.", &prefs.high_contrast);
        addSlider(scrollLayout, "🔤 Tamanho do Texto", &prefs.font_scale);
        addToggle(scrollLayout, "🖱 Cursor Personalizado", "Aumenta o mouse.", &prefs.custom_cursor);
        addToggle(scrollLayout, "🔊 Falar Texto", "Narrador PT-BR.", &prefs.text_to_speech);
        scroll->setWidget(scrollContent);
        mainLayout->addWidget(scroll);
        auto *btnClose = new QPushButton("✅ Sair e Salvar", this);
        btnClose->setMinimumHeight(60);
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
        cb->setStyleSheet("QCheckBox::indicator { width: 30px; height: 30px; border: 2px solid #C9A0DC; border-radius: 5px; } QCheckBox::indicator:checked { background-color: #C9A0DC; }");
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
        combo->setCurrentText(im[prefs.colorblind]); combo->setStyleSheet("QComboBox { border: 2px solid #C9A0DC; border-radius: 5px; padding: 5px; font-size: 12pt; color: black; background: white; }");
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

class VisualNovel : public QMainWindow {
public:
    VisualNovel() {
        setWindowTitle("Aurudinha");
        loadPrefs();
        speech = new QTextToSpeech(this);
        setupVoice();
        setupUI();
        setupScenes();
        applyPrefs(prefs);
        goToScene("inicio");
        showFullScreen();
    }

protected:
    void resizeEvent(QResizeEvent *event) override {
        applyPrefs(prefs);
        QMainWindow::resizeEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (prefs.keyboard_shortcuts) {
            int key = event->key();
            if (key >= Qt::Key_1 && key <= Qt::Key_9) {
                int idx = key - Qt::Key_1;
                auto it = scenes.find(currentSceneId);
                if (it != scenes.end() && idx < (int)it->second.choices.size()) goToScene(it->second.choices[idx].nextScene);
            }
        }
        QMainWindow::keyPressEvent(event);
    }

private:
    Prefs prefs; QTextToSpeech *speech;
    QLabel *bgLabel, *spriteCenter, *spriteLeft, *spriteRight, *textLabel, *colorOverlay;
    QFrame *textFrame, *choicesFrame;
    QVBoxLayout *choicesLayout;
    QPushButton *btnA11y;
    QString currentSceneId;
    std::map<QString, Scene> scenes;

    void setupVoice() {
        QLocale locale(QLocale::Portuguese, QLocale::Brazil);
        speech->setLocale(locale);
        speech->setRate(-0.1);
    }

    void setupUI() {
        auto *central = new QWidget(this);
        setCentralWidget(central);
        bgLabel = new QLabel(central);
        bgLabel->setScaledContents(true);
        
        spriteLeft = new QLabel(central); spriteLeft->setAlignment(Qt::AlignLeft | Qt::AlignBottom); spriteLeft->setStyleSheet("background: transparent; border: none;");
        spriteRight = new QLabel(central); spriteRight->setAlignment(Qt::AlignRight | Qt::AlignBottom); spriteRight->setStyleSheet("background: transparent; border: none;");
        spriteCenter = new QLabel(central); spriteCenter->setAlignment(Qt::AlignHCenter | Qt::AlignBottom); spriteCenter->setStyleSheet("background: transparent; border: none;");

        colorOverlay = new QLabel(central);
        colorOverlay->setAttribute(Qt::WA_TransparentForMouseEvents);
        colorOverlay->hide();

        textFrame = new QFrame(central);
        textLabel = new QLabel(textFrame); textLabel->setWordWrap(true); textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        choicesFrame = new QFrame(central); choicesLayout = new QVBoxLayout(choicesFrame); choicesLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
        
        choicesFrame->setStyleSheet("background: transparent; border: none;");
        textFrame->setStyleSheet("background-color: rgba(255, 255, 255, 120); border-radius: 15px; border: 2px solid rgba(255, 183, 197, 100);");

        btnA11y = new QPushButton("⚙ Configs", central); btnA11y->setCursor(Qt::PointingHandCursor);
        connect(btnA11y, &QPushButton::clicked, this, &VisualNovel::openA11yMenu);
    }

    void setupScenes() {
        QString s = QDir::currentPath() + "/assets/sprites/";
        QString b = QDir::currentPath() + "/assets/backgrounds/";

        QString a_triste = s + "aurudinhacomolhartriste_no_bg_ya4gzsbv.png";
        QString a_chorando = s + "aurudinhachorando_no_bg_z9jgkih3.png";
        QString a_agonia = s + "aurudinhamaonacabeça_no_bg_lwjhxt3z.png";
        QString a_morta = s + "auridnhacomolharmorto_no_bg_myl6f8jd.png";
        QString mae_seria = s + "maeseria_no_bg_t4bqtyzu.png";
        QString mae_feliz = s + "maefeliz_no_bg_r2s96e1i.png";
        QString pai_triste = s + "paitriste-removebg-preview.png";
        QString pai_olhando = s + "paiolhandodelado-removebg-preview.png";
        QString pai_sorrindo = s + "paisorrindo-removebg-preview.png";
        QString prof_apreensiva = s + "professora normal ou aprenseiva_no_bg_i2annqee.png";

        QString bg_quarto = b + "quartodaauruda.jpeg";
        QString bg_sala = b + "saladacasa.jpeg";
        QString bg_escola_frente = b + "frentedaescola.jpeg";
        QString bg_escola_corredor = b + "correndopelaescola.jpeg";
        QString bg_sala_aula = b + "saladeaula.jpeg";
        QString bg_funeral = b + "cenadoveloriodaaurda.jpeg";

        scenes["inicio"] = {"Aurudinha acorda em seu quarto com a mente um pouco confusa. Escuta um barulho na sala.", bg_quarto, a_triste, "", "", {{"Ir ver o que aconteceu", "ir_ver"}, {"Permanecer no quarto", "ficar_quarto"}}};
        scenes["ir_ver"] = {"Ela se aproxima e ouve os pais no corredor...\nMãe: 'Eu não queria um filho assim...'", bg_sala, "", mae_seria, pai_triste, {{"Continuar ouvindo", "ir_ver_2"}}};
        scenes["ir_ver_2"] = {"Pai: 'Não quero passar vergonha por ter um filho autista...'", bg_sala, "", mae_seria, pai_triste, {{"Voltar para o quarto", "choro_quarto"}}};
        scenes["choro_quarto"] = {"Aurudinha: 'Se eu não fosse autista, meus pais não teriam vergonha de mim.'", bg_quarto, a_chorando, "", "", {{"Escola", "escola"}}};
        scenes["ficar_quarto"] = {"Ela decide não ver nada. Fica no celular até ser chamada.", bg_quarto, a_triste, "", "", {{"Escola", "escola"}}};
        scenes["escola"] = {"Na escola, o barulho das crianças começa a incomodar muito.", bg_escola_frente, a_triste, "", "", {{"Entrar na sala", "sala_aula"}}};
        scenes["sala_aula"] = {"O barulho aumenta. Aurudinha começa a tremer e se sentir mal.", bg_sala_aula, a_agonia, "", "", {{"Pedir ajuda", "pedir_ajuda"}, {"Não pedir ajuda", "correr"}}};
        scenes["pedir_ajuda"] = {"A professora percebe e tenta ajudar, mas o barulho continua.", bg_sala_aula, a_agonia, prof_apreensiva, "", {{"Ficar na sala", "casa"}, {"Sair correndo", "correr"}}};
        scenes["correr"] = {"Ela sai correndo. No corredor, o grupinho começa a rir dela.\n'Olha essa menina doente!'", bg_escola_corredor, a_chorando, "", "", {{"Fugir da escola", "atropelamento"}, {"Ficar no chão", "casa"}}};
        scenes["casa"] = {"De volta em casa, Aurudinha vai direto para o quarto. O pai está triste.", bg_quarto, a_triste, "", pai_triste, {{"Contar para ele", "final_bom"}, {"Não contar", "final_ruim"}}};
        scenes["final_bom"] = {"O pai decide dar todo o apoio a Aurudinha. [ FINAL BOM ]", bg_quarto, a_chorando, "", pai_sorrindo, {{"Reiniciar", "inicio"}}};
        scenes["final_ruim"] = {"Silêncio total na casa. O pai entra no quarto...", bg_quarto, a_morta, "", pai_triste, {{"Ver o que houve", "funeral"}}};
        scenes["funeral"] = {"Davy se encontra perdido em pensamentos em frente ao caixão.\n'E se eu tivesse feito diferente?'", bg_funeral, "", "", pai_triste, {{"Fim", "nova_filha"}}};
        scenes["atropelamento"] = {"Aurudinha corre sem olhar... [ FINAL TRÁGICO ]", bg_escola_frente, a_morta, "", "", {{"Anos depois", "nova_filha"}}};
        scenes["nova_filha"] = {"A mãe vive 'feliz' com uma nova filha. [ FIM ]", bg_sala, "", mae_feliz, "", {{"Reiniciar", "inicio"}}};
    }

    void goToScene(QString id) {
        speech->stop(); currentSceneId = id; Scene &s = scenes[id];
        textLabel->setText(s.text);
        if (prefs.text_to_speech) speech->say(s.text);
        updateSprites();
        
        QLayoutItem *item; while ((item = choicesLayout->takeAt(0)) != nullptr) { if (item->widget()) delete item->widget(); delete item; }
        for (const auto &c : s.choices) {
            auto *btn = new QPushButton(c.text); btn->setFixedWidth(width() * 0.4); btn->setMinimumHeight(55);
            QString btnStyle;
            if (prefs.high_contrast) {
                btnStyle = QString("QPushButton { background-color: black; color: yellow; font: bold %1pt; border: 3px solid yellow; border-radius: 12px; }").arg(qMax(10, (int)(12 * prefs.font_scale / 100)));
            } else {
                btnStyle = QString("QPushButton { background-color: rgba(255, 240, 245, 200); color: #3D2B3D; font: bold %1pt; border: 2px solid rgba(201, 160, 220, 150); border-radius: 12px; padding: 10px; } QPushButton:hover { background-color: rgba(201, 160, 220, 230); color: white; }").arg(qMax(10, (int)(12 * prefs.font_scale / 100)));
            }
            btn->setStyleSheet(btnStyle);
            connect(btn, &QPushButton::clicked, this, [=](){ goToScene(c.nextScene); });
            choicesLayout->addWidget(btn);
        }
    }

    void updateSprites() {
        if (currentSceneId.isEmpty()) return;
        Scene &s = scenes[currentSceneId];
        
        QPixmap bg(s.background);
        if (!bg.isNull()) bgLabel->setPixmap(bg);

        auto load = [this](QLabel *l, QString p) {
            if (p.isEmpty()) { l->clear(); return; }
            QPixmap pix(p); if (!pix.isNull()) l->setPixmap(pix.scaled(l->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        };
        load(spriteCenter, s.centerSprite); load(spriteLeft, s.leftSprite); load(spriteRight, s.rightSprite);
        bgLabel->lower(); spriteLeft->raise(); spriteRight->raise(); spriteCenter->raise(); 
        colorOverlay->raise(); // Filtro em cima de tudo
        textFrame->raise(); choicesFrame->raise(); btnA11y->raise();
    }

    void openA11yMenu() { speech->stop(); AccessibilityMenu menu(this, prefs, [=](const Prefs &p){ applyPrefs(p); savePrefs(); }); if (menu.exec() == QDialog::Accepted) goToScene(currentSceneId); }

    void applyPrefs(const Prefs &p) {
        prefs = p; int w = width(), h = height();
        bgLabel->setGeometry(0, 0, w, h);
        colorOverlay->setGeometry(0, 0, w, h);
        
        int sH = h * 0.8;
        int sY = h - sH;
        spriteLeft->setGeometry(0, sY, w/2.5, sH);
        spriteRight->setGeometry(w - (w/2.5), sY, w/2.5, sH);
        spriteCenter->setGeometry(0, sY, w, sH);
        
        struct Palette { QString bg, panel, accent, text, overlay; };
        std::map<QString, Palette> pals = {
            {"none", {ROSA, BRANCO, LILAS, TEXTO_ESCURO, "transparent"}}, 
            {"deuteranopia", {"#005b96", "#b3cde0", "#ffc100", "#000000", "rgba(0, 100, 255, 40)"}}, 
            {"protanopia", {"#005b96", "#b3cde0", "#ffc100", "#000000", "rgba(255, 100, 0, 40)"}}, 
            {"tritanopia", {"#c81d25", "#fbc4ab", "#0081a7", "#000000", "rgba(0, 255, 255, 40)"}}
        };
        auto pal = pals[prefs.colorblind];
        
        if (pal.overlay != "transparent") {
            colorOverlay->setStyleSheet(QString("background-color: %1;").arg(pal.overlay));
            colorOverlay->show();
        } else {
            colorOverlay->hide();
        }
        
        if (prefs.high_contrast) {
            centralWidget()->setStyleSheet(QString("background-color: black;"));
            textFrame->setStyleSheet(QString("background-color: rgba(0, 0, 0, 230); border: 4px solid yellow; border-radius: 15px;"));
            textLabel->setStyleSheet(QString("color: white; font: bold %1pt; background: transparent;").arg((int)(20 * prefs.font_scale/100)));
        } else {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(pal.bg));
            textFrame->setStyleSheet(QString("background-color: rgba(255, 255, 255, 150); border: 3px solid %1; border-radius: 15px;").arg(pal.accent));
            textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; background: transparent;").arg(pal.text).arg((int)(20 * prefs.font_scale/100)));
        }

        if (prefs.reading_mode) { textFrame->setGeometry(w*0.2, h*0.2, w*0.6, h*0.6); }
        else { textFrame->setGeometry(w*0.1, 40, w*0.8, h*0.25); }
        
        textLabel->setGeometry(25, 25, textFrame->width()-50, textFrame->height()-50);
        choicesFrame->setGeometry(0, h*0.6, w, h*0.35);
        btnA11y->setGeometry(w - 180, 20, 160, 45);
        updateSprites();
    }
    void loadPrefs() { QFile f(QDir::currentPath() + "/a11y_prefs.json"); if (f.open(QIODevice::ReadOnly)) prefs = Prefs::fromJson(QJsonDocument::fromJson(f.readAll()).object()); }
    void savePrefs() { QFile f(QDir::currentPath() + "/a11y_prefs.json"); if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(prefs.toJson()).toJson()); }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    VisualNovel v;
    return a.exec();
}
