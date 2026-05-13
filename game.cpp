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
    QPushButton *btnA11y;
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
        spriteLeft = new QLabel(central); spriteLeft->setAlignment(Qt::AlignLeft | Qt::AlignBottom); spriteLeft->setStyleSheet("background: transparent; border: none;");
        spriteRight = new QLabel(central); spriteRight->setAlignment(Qt::AlignRight | Qt::AlignBottom); spriteRight->setStyleSheet("background: transparent; border: none;");
        spriteCenter = new QLabel(central); spriteCenter->setAlignment(Qt::AlignHCenter | Qt::AlignBottom); spriteCenter->setStyleSheet("background: transparent; border: none;");
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
        scenes["inicio"] = {"Aurudinha acorda confusa. Escuta barulho na sala.", b+"quartodaauruda.jpeg", s+"aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "", {{"Ir ver", "ir_ver"}, {"Ficar", "ficar_quarto"}}};
        scenes["ir_ver"] = {"Ela ouve os pais...", b+"saladacasa.jpeg", "", s+"maeseria_no_bg_t4bqtyzu.png", s+"paitriste-removebg-preview.png", {{"Continuar", "ir_ver_2"}}};
        scenes["ir_ver_2"] = {"Pai: 'Não quero passar vergonha...'", b+"saladacasa.jpeg", "", s+"maeseria_no_bg_t4bqtyzu.png", s+"paitriste-removebg-preview.png", {{"Voltar", "choro_quarto"}}};
        scenes["choro_quarto"] = {"Aurudinha: 'Se eu não fosse autista...'", b+"quartodaauruda.jpeg", s+"aurudinhachorando_no_bg_z9jgkih3.png", "", "", {{"Escola", "escola"}}};
        scenes["ficar_quarto"] = {"Ela decide não ver nada.", b+"quartodaauruda.jpeg", s+"aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "", {{"Escola", "escola"}}};
        scenes["escola"] = {"Na escola, o barulho incomoda.", b+"frentedaescola.jpeg", s+"aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "", {{"Entrar", "sala_aula"}}};
        scenes["sala_aula"] = {"O barulho aumenta. Aurudinha treme.", b+"saladeaula.jpeg", s+"aurudinhamaonacabeça_no_bg_lwjhxt3z.png", "", "", {{"Ajuda", "pedir_ajuda"}, {"Correr", "correr"}}};
        scenes["pedir_ajuda"] = {"A professora percebe.", b+"saladeaula2.jpeg", s+"aurudinhamaonacabeça_no_bg_lwjhxt3z.png", s+"professora normal ou aprenseiva_no_bg_i2annqee.png", "", {{"Ficar", "casa"}, {"Sair", "correr"}}};
        scenes["correr"] = {"'Olha a doente!'", b+"correndopelaescola.jpeg", s+"aurudinhachorando_no_bg_z9jgkih3.png", "", "", {{"Fugir", "atropelamento"}, {"Ficar", "casa"}}};
        scenes["casa"] = {"Em casa, o pai está preocupado.", b+"quartodaauruda.jpeg", s+"aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", s+"paiolhandodelado-removebg-preview.png", {{"Contar", "final_bom"}, {"Não", "final_ruim"}}};
        scenes["final_bom"] = {"[ FINAL BOM ]", b+"quartodaauruda.jpeg", s+"aurudinhachorando_no_bg_z9jgkih3.png", "", s+"paisorrindo-removebg-preview.png", {{"Reiniciar", "inicio"}}};
        scenes["final_ruim"] = {"Silêncio total. [ FINAL RUIM ]", b+"quartodaauruda.jpeg", s+"auridnhacomolharmorto_no_bg_myl6f8jd.png", "", s+"paitriste-removebg-preview.png", {{"Fim", "inicio"}}};
        scenes["atropelamento"] = {"[ FINAL TRÁGICO ]", b+"frentedaescola.jpeg", s+"auridnhacomolharmorto_no_bg_myl6f8jd.png", "", "", {{"Anos depois", "nova_filha"}}};
        scenes["nova_filha"] = {"Egosinha vive 'feliz'. [ FIM ]", b+"saladacasa.jpeg", "", s+"maefeliz_no_bg_r2s96e1i.png", "", {{"Reiniciar", "inicio"}}};
    }

    void goToScene(QString id) {
#ifndef __EMSCRIPTEN__
        speech->stop();
#endif
        currentSceneId = id; Scene &s = scenes[id]; textLabel->setText(s.text);
#ifndef __EMSCRIPTEN__
        if (prefs.text_to_speech) speech->say(s.text);
#endif
        updateSprites();
        QLayoutItem *item; while ((item = choicesLayout->takeAt(0)) != nullptr) { if (item->widget()) delete item->widget(); delete item; }
        for (const auto &c : s.choices) {
            auto *btn = new QPushButton(c.text); btn->setFixedWidth(width() * 0.5);
            btn->setStyleSheet(QString("QPushButton { background-color: rgba(255, 240, 245, 180); color: %1; font: bold %2pt; border: 2px solid rgba(201, 160, 220, 150); border-radius: 12px; padding: 10px; }")
                .arg(TEXTO_ESCURO).arg(qMax(12, (int)(14 * prefs.font_scale / 100))));
            connect(btn, &QPushButton::clicked, this, [=](){ goToScene(c.nextScene); }); choicesLayout->addWidget(btn);
        }
    }

    void updateSprites() {
        if (currentSceneId.isEmpty()) return;
        Scene &s = scenes[currentSceneId];
        QPixmap bg(s.background);
        if (!bg.isNull()) bgLabel->setPixmap(bg.scaled(bgLabel->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
        auto load = [this](QLabel *l, QString p) {
            if (p.isEmpty()) { l->clear(); return; }
            QPixmap pix(p); if (!pix.isNull()) l->setPixmap(pix.scaled(l->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
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
        prefs = p; int w = width(), h = height(); bgLabel->setGeometry(0, 0, w, h);
        spriteLeft->setGeometry(0, 0, w/2, h); spriteRight->setGeometry(w/2, 0, w/2, h); spriteCenter->setGeometry(0, 0, w, h);
        struct Palette { QString bg, panel, accent, text; };
        std::map<QString, Palette> pals = {{"none", {ROSA, BRANCO, LILAS, TEXTO_ESCURO}}, {"deuteranopia", {"#005b96", "#b3cde0", "#ffc100", "#000000"}}, {"protanopia", {"#005b96", "#b3cde0", "#ffc100", "#000000"}}, {"tritanopia", {"#c81d25", "#fbc4ab", "#0081a7", "#000000"}}};
        auto pal = pals[prefs.colorblind];
        if (prefs.high_contrast) { centralWidget()->setStyleSheet(QString("background-color: %1;").arg(HC_BG)); textFrame->setStyleSheet(QString("background-color: rgba(26, 26, 26, 200); border: 4px solid %1; border-radius: 15px;").arg(HC_BORDER)); textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; padding: 15px; background: transparent;").arg(HC_TEXT).arg((int)(20 * prefs.font_scale/100))); }
        else { centralWidget()->setStyleSheet(QString("background-color: %1;").arg(pal.bg)); textFrame->setStyleSheet(QString("background-color: rgba(255, 255, 255, 120); border: 3px solid %1; border-radius: 15px;").arg(pal.accent)); textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; padding: 15px; background: transparent;").arg(pal.text).arg((int)(20 * prefs.font_scale/100))); }
        if (prefs.reading_mode) textFrame->setGeometry(w*0.2, h*0.3, w*0.6, h*0.5); else textFrame->setGeometry(w*0.1, 40, w*0.8, h*0.25);
        textLabel->setGeometry(20, 20, textFrame->width()-40, textFrame->height()-40); choicesFrame->setGeometry(0, h*0.6, w, h*0.35); btnA11y->setGeometry(w - 180, 20, 160, 45); updateSprites();
    }
    void loadPrefs() { QFile f(QDir::currentPath() + "/a11y_prefs.json"); if (f.open(QIODevice::ReadOnly)) prefs = Prefs::fromJson(QJsonDocument::fromJson(f.readAll()).object()); }
    void savePrefs() { QFile f(QDir::currentPath() + "/a11y_prefs.json"); if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(prefs.toJson()).toJson()); }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    VisualNovel v;
    return a.exec();
}
