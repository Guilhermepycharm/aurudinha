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
#include <QVoice>
#endif
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QSoundEffect>
#include <QPixmap>
#include <QPalette>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QTimer>
#include <QRandomGenerator>
#include <QKeyEvent>
#include <QDialog>
#include <QPainter>
#include <QGraphicsColorizeEffect>
#include <QStyle>
#include <QScreen>
#include <QStringList>
#include <QUrl>
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
const QString HC_BTN_BG = "#2a2a2a";
const QString HC_BTN_TEXT = "#FFFF00";
const QString HC_BTN_BORDER = "#FFD700";

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
    bool ending_good_unlocked = false;
    bool ending_silence_unlocked = false;
    bool ending_tragic_unlocked = false;

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
        obj["ending_good_unlocked"] = ending_good_unlocked;
        obj["ending_silence_unlocked"] = ending_silence_unlocked;
        obj["ending_tragic_unlocked"] = ending_tragic_unlocked;
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
        if (obj.contains("ending_good_unlocked")) p.ending_good_unlocked = obj["ending_good_unlocked"].toBool();
        if (obj.contains("ending_silence_unlocked")) p.ending_silence_unlocked = obj["ending_silence_unlocked"].toBool();
        if (obj.contains("ending_tragic_unlocked")) p.ending_tragic_unlocked = obj["ending_tragic_unlocked"].toBool();
        return p;
    }

    int unlockedEndingsCount() const {
        return (ending_good_unlocked ? 1 : 0) + (ending_silence_unlocked ? 1 : 0) + (ending_tragic_unlocked ? 1 : 0);
    }
};

struct Choice { QString text; QString nextScene; };
struct Scene { QString text; QString background; QString centerSprite; QString leftSprite; QString rightSprite; std::vector<Choice> choices; };

static QString cleanPath(const QString &path) {
    return QDir::cleanPath(QDir::fromNativeSeparators(path));
}

static QStringList uniquePaths(const QStringList &paths) {
    QStringList result;
    for (const QString &path : paths) {
        QString cleaned = cleanPath(path);
        if (!result.contains(cleaned)) result.append(cleaned);
    }
    return result;
}

static QStringList assetRootCandidates() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cwd = QDir::currentPath();
    return uniquePaths({
        QDir(appDir).filePath("assets"),
        QDir(cwd).filePath("assets"),
        QDir(appDir).filePath("../assets"),
        QDir(appDir).filePath("../../assets")
    });
}

static QString assetsRoot() {
    const QStringList candidates = assetRootCandidates();
    for (const QString &path : candidates) {
        if (QDir(path).exists()) return path;
    }
    qWarning() << "Diretorio de assets nao encontrado. Caminhos testados:" << candidates;
    return candidates.isEmpty() ? QString("assets") : candidates.first();
}

static QString assetPath(const QString &relativePath) {
    const QString relative = cleanPath(relativePath);
    QStringList tried;
    for (const QString &root : assetRootCandidates()) {
        const QString candidate = cleanPath(QDir(root).filePath(relative));
        tried.append(candidate);
        if (QFileInfo::exists(candidate)) return candidate;
    }
    qWarning() << "Asset nao encontrado:" << relative << "Caminhos testados:" << tried;
    return cleanPath(QDir(assetsRoot()).filePath(relative));
}

static QStringList configDirCandidates() {
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString cwd = QDir::currentPath();
    return uniquePaths({
        QDir(appDir).filePath("config"),
        QDir(cwd).filePath("config"),
        QDir(appDir).filePath("../config"),
        QDir(appDir).filePath("../../config")
    });
}

static QString prefsPathForRead() {
    for (const QString &dir : configDirCandidates()) {
        const QString path = cleanPath(QDir(dir).filePath("a11y_prefs.json"));
        if (QFileInfo::exists(path)) return path;
    }
    return QString();
}

static QString prefsPathForWrite() {
    const QString existing = prefsPathForRead();
    if (!existing.isEmpty()) return existing;

    for (const QString &dirPath : configDirCandidates()) {
        QDir dir(dirPath);
        if (dir.exists() || dir.mkpath(".")) {
            return cleanPath(dir.filePath("a11y_prefs.json"));
        }
    }

    return cleanPath(QDir(QCoreApplication::applicationDirPath()).filePath("config/a11y_prefs.json"));
}

static void applyApplicationPalette(const Prefs &prefs) {
    QPalette palette;
    if (prefs.high_contrast) {
        palette.setColor(QPalette::Window, QColor(HC_BG));
        palette.setColor(QPalette::WindowText, QColor(HC_TEXT));
        palette.setColor(QPalette::Base, QColor(HC_PANEL));
        palette.setColor(QPalette::AlternateBase, QColor(HC_BTN_BG));
        palette.setColor(QPalette::Text, QColor(HC_TEXT));
        palette.setColor(QPalette::Button, QColor(HC_BTN_BG));
        palette.setColor(QPalette::ButtonText, QColor(HC_BTN_TEXT));
        palette.setColor(QPalette::Highlight, QColor(HC_ACCENT));
        palette.setColor(QPalette::HighlightedText, QColor(HC_BG));
    } else {
        palette.setColor(QPalette::Window, QColor(ROSA));
        palette.setColor(QPalette::WindowText, QColor(TEXTO_ESCURO));
        palette.setColor(QPalette::Base, QColor(CREME));
        palette.setColor(QPalette::AlternateBase, QColor(BRANCO));
        palette.setColor(QPalette::Text, QColor(TEXTO_ESCURO));
        palette.setColor(QPalette::Button, QColor(LILAS));
        palette.setColor(QPalette::ButtonText, QColor(BRANCO));
        palette.setColor(QPalette::Highlight, QColor(LILAS));
        palette.setColor(QPalette::HighlightedText, QColor(BRANCO));
    }
    qApp->setPalette(palette);

    static const int basePointSize = qMax(10, qApp->font().pointSize());
    QFont appFont = qApp->font();
    appFont.setPointSize(qMax(9, basePointSize * prefs.font_scale / 100));
    qApp->setFont(appFont);
}

static Prefs loadPrefsFromDisk() {
    Prefs prefs;
    const QString path = prefsPathForRead();
    if (path.isEmpty()) {
        qDebug() << "Preferencias de acessibilidade nao encontradas; usando padroes.";
        return prefs;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Falha ao abrir preferencias:" << path << file.errorString();
        return prefs;
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        qWarning() << "Preferencias invalidas em" << path << error.errorString();
        return prefs;
    }

    qDebug() << "Preferencias carregadas de" << path;
    return Prefs::fromJson(doc.object());
}

static bool savePrefsToDisk(const Prefs &prefs) {
    const QString path = prefsPathForWrite();
    QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning() << "Falha ao salvar preferencias:" << path << file.errorString();
        return false;
    }

    file.write(QJsonDocument(prefs.toJson()).toJson(QJsonDocument::Indented));
    qDebug() << "Preferencias salvas em" << path;
    return true;
}

class AudioManager {
public:
    AudioManager() {
        musicOutput = new QAudioOutput(qApp);
        musicOutput->setVolume(normalMusicVolume);

        musicPlayer = new QMediaPlayer(qApp);
        musicPlayer->setAudioOutput(musicOutput);
        musicPlayer->setLoops(QMediaPlayer::Infinite);

        typewriterEffect = new QSoundEffect(qApp);
        typewriterEffect->setSource(QUrl::fromLocalFile(assetPath("audio/typewriter-click.wav")));
        typewriterEffect->setVolume(0.16f);
    }

    void startMusic() {
        const QString musicPath = assetPath("audio/the-storyteller.mp3");
        if (!QFileInfo::exists(musicPath)) {
            qWarning() << "Trilha sonora nao encontrada:" << musicPath;
            return;
        }

        if (musicPlayer->source() != QUrl::fromLocalFile(musicPath)) {
            musicPlayer->setSource(QUrl::fromLocalFile(musicPath));
        }

        if (musicPlayer->playbackState() != QMediaPlayer::PlayingState) {
            musicPlayer->play();
        }
    }

    void playTypewriterClick(bool muted) {
        if (muted || !typewriterEffect || !typewriterEffect->isLoaded()) return;
        typewriterEffect->play();
    }

    void duckMusic(bool duck) {
        if (!musicOutput) return;
        musicOutput->setVolume(duck ? duckedMusicVolume : normalMusicVolume);
    }

private:
    QMediaPlayer *musicPlayer = nullptr;
    QAudioOutput *musicOutput = nullptr;
    QSoundEffect *typewriterEffect = nullptr;
    const float normalMusicVolume = 0.22f;
    const float duckedMusicVolume = 0.08f;
};

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
        setModal(true);
        setWindowModality(Qt::WindowModal);
        setWindowTitle("Configurações");
        petalCanvas = new PetalCanvas(this); petalCanvas->lower();
        auto *mainLayout = new QVBoxLayout(this); mainLayout->setContentsMargins(24, 24, 24, 24);
        mainLayout->setSpacing(14);
        header = new QLabel("✦ CONFIGURAÇÕES ✦", this);
        header->setAlignment(Qt::AlignCenter);
        mainLayout->addWidget(header);

        scroll = new QScrollArea(this);
        scroll->setWidgetResizable(true);
        scroll->setFrameShape(QFrame::NoFrame);
        scrollContent = new QFrame();
        scrollContent->setObjectName("scrollContent");
        auto *scrollLayout = new QVBoxLayout(scrollContent); scrollLayout->setContentsMargins(30, 30, 30, 30);
        scrollLayout->setSpacing(14);
        addToggle(scrollLayout, "📖 Modo Leitura", "Centraliza o texto na tela.", &prefs.reading_mode);
        addColorblindSelect(scrollLayout);
        addToggle(scrollLayout, "🚫 Reduzir Movimento", "Desativa animações de pétalas.", &prefs.reduce_motion);
        addToggle(scrollLayout, "◑ Alto Contraste", "Fundo escuro com texto branco e bordas amarelas.", &prefs.high_contrast);
        addSlider(scrollLayout, "🔤 Tamanho do Texto", &prefs.font_scale);
        addToggle(scrollLayout, "⌨ Atalhos de Teclado", "Teclas 1-9 para escolher.", &prefs.keyboard_shortcuts);
#ifndef __EMSCRIPTEN__
        addToggle(scrollLayout, "🔊 Leitor de Texto", "Narra o texto em voz alta (PT-BR).", &prefs.text_to_speech);
#endif
        scroll->setWidget(scrollContent); mainLayout->addWidget(scroll);
        btnClose = new QPushButton("✅ Sair e Salvar", this);
        btnClose->setObjectName("btnClose");
        btnClose->setMinimumHeight(56);
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        mainLayout->addWidget(btnClose);

        applyPrefs(prefs);
        QTimer::singleShot(0, this, [this](){ positionDialog(); });
    }
protected:
    void resizeEvent(QResizeEvent *e) override { if (petalCanvas) petalCanvas->setGeometry(0, 0, width(), height()); QDialog::resizeEvent(e); }
private:
    Prefs &prefs;
    std::function<void(const Prefs&)> applyCallback;
    PetalCanvas *petalCanvas = nullptr;
    QLabel *header;
    QScrollArea *scroll;
    QFrame *scrollContent;
    QPushButton *btnClose;

    void positionDialog() {
        QRect area;
        if (parentWidget()) {
            area = parentWidget()->window()->geometry();
        } else if (QApplication::primaryScreen()) {
            area = QApplication::primaryScreen()->availableGeometry();
        } else {
            area = QRect(0, 0, 1366, 768);
        }

        const int dialogW = qBound(620, (int)(area.width() * 0.72), 920);
        const int dialogH = qBound(500, (int)(area.height() * 0.86), 680);
        resize(dialogW, dialogH);
        move(area.center() - rect().center());
    }

    void applyPrefs(const Prefs &p) {
        prefs = p;
        applyApplicationPalette(prefs);

        const int bodyFont = qMax(13, 16 * prefs.font_scale / 100);
        const int smallFont = qMax(11, 13 * prefs.font_scale / 100);
        const int headerFont = qMax(24, 35 * prefs.font_scale / 100);
        const QString bg = prefs.high_contrast ? HC_BG : ROSA;
        const QString panel = prefs.high_contrast ? HC_PANEL : CREME;
        const QString text = prefs.high_contrast ? HC_TEXT : TEXTO_ESCURO;
        const QString accent = prefs.high_contrast ? HC_ACCENT : LILAS;
        const QString border = prefs.high_contrast ? HC_BORDER : LILAS;
        const QString buttonText = prefs.high_contrast ? HC_BG : BRANCO;

        setStyleSheet(QString(
            "QDialog { background-color: %1; color: %3; }"
            "QLabel { color: %3; background: transparent; font-size: %4pt; }"
            "QFrame#scrollContent { background-color: %2; border: 4px solid %5; border-radius: 16px; }"
            "QScrollArea { background-color: transparent; border: none; }"
            "QScrollBar:vertical { background: rgba(255,255,255,70); width: 12px; border-radius: 6px; }"
            "QScrollBar::handle:vertical { background: %6; border-radius: 6px; min-height: 32px; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0px; }"
            "QCheckBox::indicator { width: 32px; height: 32px; border: 3px solid %5; border-radius: 8px; background: %2; }"
            "QCheckBox::indicator:checked { background-color: %6; }"
            "QComboBox { background: %2; color: %3; border: 2px solid %5; border-radius: 6px; padding: 7px; font-size: %7pt; min-width: 190px; }"
            "QComboBox QAbstractItemView { background: %2; color: %3; selection-background-color: %6; }"
            "QSlider::groove:horizontal { height: 8px; background: %5; border-radius: 4px; }"
            "QSlider::handle:horizontal { background: %6; width: 24px; margin: -8px 0; border-radius: 12px; }"
            "QPushButton#btnClose { background-color: %6; color: %8; font: bold %9pt 'Helvetica'; border: 2px solid %5; border-radius: 12px; padding: 10px; }"
            "QPushButton#btnClose:hover { background-color: %5; }"
        ).arg(bg).arg(panel).arg(text).arg(bodyFont).arg(border).arg(accent).arg(smallFont).arg(buttonText).arg(qMax(15, 18 * prefs.font_scale / 100)));

        header->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent;").arg(headerFont).arg(text));
        if (prefs.reduce_motion) petalCanvas->stop(); else petalCanvas->start();
    }

    void addToggle(QVBoxLayout *layout, QString title, QString desc, bool *val) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel(QString("<b>%1</b><br><small>%2</small>").arg(title).arg(desc));
        lTitle->setWordWrap(true);
        auto *cb = new QCheckBox(); cb->setChecked(*val); cb->setFixedSize(40, 40);
        connect(cb, &QCheckBox::toggled, this, [=](bool checked){
            *val = checked;
            // TTS: se ativou, fala o texto imediatamente
#ifndef __EMSCRIPTEN__
            if (val == &prefs.text_to_speech && checked) {
                // O callback vai recarregar a cena e o TTS vai falar
            }
#endif
            applyPrefs(prefs);
            applyCallback(prefs);
        });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(cb); layout->addWidget(row);
    }
    void addColorblindSelect(QVBoxLayout *layout) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel("<b>🎨 Modo Daltônico</b><br><small>Aplica filtro de cor em toda a tela.</small>");
        lTitle->setWordWrap(true);
        auto *combo = new QComboBox(); combo->addItems({"Nenhum", "Deuteranopia", "Protanopia", "Tritanopia"});
        std::map<QString, QString> m = {{"Nenhum", "none"}, {"Deuteranopia", "deuteranopia"}, {"Protanopia", "protanopia"}, {"Tritanopia", "tritanopia"}};
        std::map<QString, QString> im; for(auto const& [k, v] : m) im[v] = k;
        combo->setCurrentText(im[prefs.colorblind]);
        connect(combo, &QComboBox::currentTextChanged, this, [=, m=m](QString t){ prefs.colorblind = m.at(t); applyPrefs(prefs); applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(combo); layout->addWidget(row);
    }
    void addSlider(QVBoxLayout *layout, QString title, int *val) {
        auto *row = new QFrame(); auto *rowLayout = new QHBoxLayout(row);
        auto *lTitle = new QLabel(QString("<b>%1</b>").arg(title));
        auto *slider = new QSlider(Qt::Horizontal); slider->setRange(80, 160); slider->setValue(*val);
        auto *lblVal = new QLabel(QString("%1%").arg(*val));
        lblVal->setMinimumWidth(64);
        connect(slider, &QSlider::valueChanged, this, [=](int v){ *val = v; lblVal->setText(QString("%1%").arg(v)); applyPrefs(prefs); applyCallback(prefs); });
        rowLayout->addWidget(lTitle, 1); rowLayout->addWidget(slider); rowLayout->addWidget(lblVal); layout->addWidget(row);
    }
};

// --- Menu de Finais ---
class EndingsMenu : public QDialog {
public:
    EndingsMenu(QWidget *parent, Prefs &prefsRef) : QDialog(parent), prefs(prefsRef) {
        setModal(true);
        setWindowModality(Qt::WindowModal);
        setWindowTitle("Finais");

        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(28, 28, 28, 28);
        layout->setSpacing(16);

        header = new QLabel("✦ FINAIS ✦", this);
        header->setAlignment(Qt::AlignCenter);
        layout->addWidget(header);

        progressLabel = new QLabel(this);
        progressLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(progressLabel);

        endingsFrame = new QFrame(this);
        endingsFrame->setObjectName("endingsFrame");
        auto *endingsLayout = new QVBoxLayout(endingsFrame);
        endingsLayout->setContentsMargins(24, 22, 24, 22);
        endingsLayout->setSpacing(12);

        addEndingRow(endingsLayout, prefs.ending_good_unlocked, "Final 1", "FINAL BOM — O Abraço");
        addEndingRow(endingsLayout, prefs.ending_silence_unlocked, "Final 2", "FINAL RUIM — O Silêncio");
        addEndingRow(endingsLayout, prefs.ending_tragic_unlocked, "Final 3", "FINAL TRÁGICO");
        layout->addWidget(endingsFrame);

        auto *btnClose = new QPushButton("Voltar", this);
        btnClose->setObjectName("btnClose");
        btnClose->setMinimumHeight(54);
        connect(btnClose, &QPushButton::clicked, this, &QDialog::accept);
        layout->addWidget(btnClose);

        applyPrefs(prefs);
        QTimer::singleShot(0, this, [this]() { positionDialog(); });
    }

private:
    Prefs &prefs;
    QLabel *header;
    QLabel *progressLabel;
    QFrame *endingsFrame;
    std::vector<QLabel*> endingLabels;

    void addEndingRow(QVBoxLayout *layout, bool unlocked, const QString &slot, const QString &title) {
        auto *label = new QLabel(unlocked ? QString("✓ %1").arg(title) : QString("□ %1 — ???").arg(slot), this);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
        label->setProperty("unlocked", unlocked);
        endingLabels.push_back(label);
        layout->addWidget(label);
    }

    void positionDialog() {
        QRect area;
        if (parentWidget()) {
            area = parentWidget()->window()->geometry();
        } else if (QApplication::primaryScreen()) {
            area = QApplication::primaryScreen()->availableGeometry();
        } else {
            area = QRect(0, 0, 1366, 768);
        }

        const int dialogW = qBound(520, (int)(area.width() * 0.46), 760);
        const int dialogH = qBound(390, (int)(area.height() * 0.58), 560);
        resize(dialogW, dialogH);
        move(area.center() - rect().center());
    }

    void applyPrefs(const Prefs &p) {
        applyApplicationPalette(p);
        const int headerFont = qMax(24, 34 * p.font_scale / 100);
        const int bodyFont = qMax(14, 18 * p.font_scale / 100);
        const int smallFont = qMax(12, 14 * p.font_scale / 100);
        const QString bg = p.high_contrast ? HC_BG : ROSA;
        const QString panel = p.high_contrast ? HC_PANEL : CREME;
        const QString text = p.high_contrast ? HC_TEXT : TEXTO_ESCURO;
        const QString accent = p.high_contrast ? HC_ACCENT : LILAS;
        const QString muted = p.high_contrast ? "#9a9a9a" : "#7a607a";
        const QString buttonText = p.high_contrast ? HC_BG : BRANCO;

        setStyleSheet(QString(
            "QDialog { background-color: %1; color: %3; }"
            "QFrame#endingsFrame { background-color: %2; border: 4px solid %4; border-radius: 14px; }"
            "QPushButton#btnClose { background-color: %4; color: %5; font: bold %6pt 'Helvetica'; border: 2px solid %4; border-radius: 12px; padding: 10px; }"
            "QPushButton#btnClose:hover { border-color: white; }"
        ).arg(bg).arg(panel).arg(text).arg(accent).arg(buttonText).arg(smallFont));

        header->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent;").arg(headerFont).arg(text));
        progressLabel->setText(QString("%1/3 alcançados").arg(p.unlockedEndingsCount()));
        progressLabel->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent;").arg(smallFont).arg(text));

        for (auto *label : endingLabels) {
            const bool unlocked = label->property("unlocked").toBool();
            label->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent;")
                .arg(bodyFont).arg(unlocked ? text : muted));
        }
    }
};

// --- Menu Principal ---
class MainMenu : public QDialog {
public:
    MainMenu(Prefs &prefsRef, std::function<void()> onPlay)
        : QDialog(nullptr), prefs(prefsRef), playCallback(onPlay) {
        setWindowTitle("Aurudinha");
        setModal(false);
        auto *layout = new QVBoxLayout(this);
        layout->setAlignment(Qt::AlignCenter);
        layout->setSpacing(20);
        layout->setContentsMargins(80, 80, 80, 80);

        titleLabel = new QLabel("✦ AURUDINHA ✦", this);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        auto makeBtn = [this](QString text, QString icon) -> QPushButton* {
            auto *btn = new QPushButton(icon + "  " + text, this);
            menuButtons.push_back(btn);
            return btn;
        };

        auto *btnPlay = makeBtn("Jogar", "🎮");
        connect(btnPlay, &QPushButton::clicked, this, [this](){ accept(); playCallback(); });
        layout->addWidget(btnPlay);

        auto *btnA11y = makeBtn("Configurações", "⚙");
        connect(btnA11y, &QPushButton::clicked, this, [this](){ openA11yMenu(); });
        layout->addWidget(btnA11y);

        auto *btnEndings = makeBtn("Finais", "★");
        connect(btnEndings, &QPushButton::clicked, this, [this](){ openEndingsMenu(); });
        layout->addWidget(btnEndings);

        auto *btnExit = makeBtn("Sair", "🚪");
        connect(btnExit, &QPushButton::clicked, this, &QDialog::reject);
        layout->addWidget(btnExit);

        petalCanvas = new PetalCanvas(this);
        petalCanvas->lower();
        applyPrefs(prefs);

        resize(800, 600);
        showFullScreen();
    }
    void applyPrefs(const Prefs &p) {
        prefs = p;
        applyApplicationPalette(prefs);

        const int titleSize = qBound(36, 48 * prefs.font_scale / 100, 72);
        const int buttonFont = qMax(18, 22 * prefs.font_scale / 100);
        const int buttonHeight = qMax(70, 70 * prefs.font_scale / 100);

        if (prefs.high_contrast) {
            setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(HC_BG).arg(HC_TEXT));
            titleLabel->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent; margin-bottom: 30px;").arg(titleSize).arg(HC_TEXT));
            for (auto *btn : menuButtons) {
                btn->setMinimumSize(400, buttonHeight);
                btn->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; font: bold %3pt 'Helvetica'; border: 3px solid %4; border-radius: 12px; padding: 14px; } QPushButton:hover { background-color: %4; color: %1; }")
                    .arg(HC_BTN_BG).arg(HC_BTN_TEXT).arg(buttonFont).arg(HC_BTN_BORDER));
            }
        } else {
            setStyleSheet(QString("QDialog { background-color: %1; color: %2; }").arg(ROSA).arg(TEXTO_ESCURO));
            titleLabel->setStyleSheet(QString("font: bold %1pt 'Helvetica'; color: %2; background: transparent; margin-bottom: 30px;").arg(titleSize).arg(TEXTO_ESCURO));
            for (auto *btn : menuButtons) {
                btn->setMinimumSize(400, buttonHeight);
                btn->setStyleSheet(QString("QPushButton { background-color: %1; color: white; font: bold %2pt 'Helvetica'; border-radius: 15px; padding: 15px; } QPushButton:hover { background-color: #E8A0B0; }")
                    .arg(LILAS).arg(buttonFont));
            }
        }

        if (petalCanvas) {
            if (prefs.reduce_motion) petalCanvas->stop(); else petalCanvas->start();
        }
    }
protected:
    void resizeEvent(QResizeEvent *e) override { if (petalCanvas) petalCanvas->setGeometry(0, 0, width(), height()); QDialog::resizeEvent(e); }
private:
    Prefs &prefs;
    std::function<void()> playCallback;
    PetalCanvas *petalCanvas = nullptr;
    QLabel *titleLabel;
    std::vector<QPushButton*> menuButtons;

    void openA11yMenu() {
        AccessibilityMenu menu(this, prefs, [this](const Prefs &p){
            applyPrefs(p);
            savePrefsToDisk(prefs);
        });
        menu.exec();
        applyPrefs(prefs);
    }

    void openEndingsMenu() {
        EndingsMenu menu(this, prefs);
        menu.exec();
        applyPrefs(prefs);
    }
};

// --- Overlay de Daltonismo ---
// Widget transparente que cobre toda a tela com uma cor semi-transparente
// para simular o filtro de daltonismo sobre backgrounds e sprites
class ColorblindOverlay : public QWidget {
public:
    ColorblindOverlay(QWidget *parent) : QWidget(parent) {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_TranslucentBackground);
        hide();
    }
    void setMode(const QString &mode) {
        if (mode == "none") { hide(); return; }
        show();
        // Filtro de cor sutil (alpha baixo) que simula daltonismo sobre toda a tela
        if (mode == "deuteranopia") {
            filterColor = QColor(0, 40, 120, 20);
        } else if (mode == "protanopia") {
            filterColor = QColor(0, 60, 140, 22);
        } else if (mode == "tritanopia") {
            filterColor = QColor(160, 40, 0, 18);
        }
        update();
    }
protected:
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setCompositionMode(QPainter::CompositionMode_Multiply);
        p.fillRect(rect(), filterColor);
    }
private:
    QColor filterColor;
};

class VisualNovel : public QMainWindow {
public:
    VisualNovel(Prefs &prefsRef, AudioManager &audioRef, std::function<void()> onReturnToMenu)
        : prefs(prefsRef), audio(audioRef), returnToMenuCallback(onReturnToMenu) {
        setWindowTitle("Aurudinha");
#ifndef __EMSCRIPTEN__
        speech = new QTextToSpeech(this); setupVoice();
#endif
        setupUI(); setupScenes(); applyPrefs(prefs); goToScene("inicio"); showFullScreen();
    }

protected:
    void resizeEvent(QResizeEvent *e) override { applyPrefs(prefs); QMainWindow::resizeEvent(e); }
    void keyPressEvent(QKeyEvent *e) override {
        if (e->key() == Qt::Key_C) {
            openA11yMenu();
            return;
        }
        if (e->key() == Qt::Key_Space || e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter || e->key() == Qt::Key_Right) {
            advanceDialoguePage();
            return;
        }
        if (e->key() == Qt::Key_Left || e->key() == Qt::Key_Backspace) {
            previousDialoguePage();
            return;
        }
        if (choicesFrame->isVisible() && prefs.keyboard_shortcuts && e->key() >= Qt::Key_1 && e->key() <= Qt::Key_9) {
            int idx = e->key() - Qt::Key_1;
            selectChoice(idx);
        }
        QMainWindow::keyPressEvent(e);
    }

private:
    Prefs &prefs;
    AudioManager &audio;
    std::function<void()> returnToMenuCallback;
#ifndef __EMSCRIPTEN__
    QTextToSpeech *speech;
#endif
    QLabel *bgLabel, *spriteCenter, *spriteLeft, *spriteRight, *textLabel;
    QLabel *pageIndicator;
    QFrame *textFrame, *choicesFrame, *dialogueNavFrame;
    QVBoxLayout *choicesLayout;
    QPushButton *btnA11y, *btnPageBack, *btnPageNext;
    QTimer *typewriterTimer;
    ColorblindOverlay *colorOverlay;
    QString currentSceneId;
    QString fullDialogueText;
    QStringList dialoguePages;
    int currentDialoguePage = 0;
    int visibleDialogueChars = 0;
    int typeSoundCounter = 0;
    bool dialogueTyping = false;
    std::map<QString, Scene> scenes;

    void setupVoice() {
#ifndef __EMSCRIPTEN__
        const QStringList engines = QTextToSpeech::availableEngines();
        if (engines.isEmpty()) {
            qWarning() << "Nenhum backend de texto-para-voz disponivel no sistema.";
        } else {
            qDebug() << "Backends de texto-para-voz disponiveis:" << engines;
        }

        QLocale locale(QLocale::Portuguese, QLocale::Brazil);
        const QList<QLocale> locales = speech->availableLocales();
        bool hasPortuguese = false;
        for (const QLocale &candidate : locales) {
            if (candidate.language() == QLocale::Portuguese) {
                locale = candidate;
                hasPortuguese = true;
                if (candidate.territory() == QLocale::Brazil) break;
            }
        }
        if (!hasPortuguese) {
            qWarning() << "Voz em portugues nao encontrada. Usando locale padrao do backend TTS:" << speech->locale();
        } else {
            speech->setLocale(locale);
        }

        const QList<QVoice> voices = speech->availableVoices();
        for (const QVoice &voice : voices) {
            if (voice.locale().language() == QLocale::Portuguese) {
                speech->setVoice(voice);
                break;
            }
        }

        speech->setRate(-0.05);
        speech->setPitch(0.0);
        speech->setVolume(1.0);

        connect(speech, &QTextToSpeech::errorOccurred, this, [](QTextToSpeech::ErrorReason reason, const QString &errorString) {
            qWarning() << "Erro no leitor de texto:" << static_cast<int>(reason) << errorString;
        });
        connect(speech, &QTextToSpeech::stateChanged, this, [this](QTextToSpeech::State state) {
            if (state == QTextToSpeech::Ready || state == QTextToSpeech::Error) {
                audio.duckMusic(false);
            }
        });
#endif
    }

    void setupUI() {
        auto *central = new QWidget(this); setCentralWidget(central);
        bgLabel = new QLabel(central);
        bgLabel->setStyleSheet("background: transparent;");
        spriteLeft = new QLabel(central); spriteLeft->setAlignment(Qt::AlignLeft | Qt::AlignBottom); spriteLeft->setStyleSheet("background: transparent; border: none;");
        spriteRight = new QLabel(central); spriteRight->setAlignment(Qt::AlignRight | Qt::AlignBottom); spriteRight->setStyleSheet("background: transparent; border: none;");
        spriteCenter = new QLabel(central); spriteCenter->setAlignment(Qt::AlignHCenter | Qt::AlignBottom); spriteCenter->setStyleSheet("background: transparent; border: none;");

        // Overlay de daltonismo (cobre tudo: bg + sprites)
        colorOverlay = new ColorblindOverlay(central);

        // Caixa de diálogo paginada, no estilo visual novel/portátil.
        textFrame = new QFrame(central);
        textFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        textFrame->setStyleSheet("background-color: rgba(255, 255, 255, 120); border-radius: 15px; border: 2px solid rgba(255, 183, 197, 100);");
        auto *textFrameLayout = new QVBoxLayout(textFrame);
        textFrameLayout->setContentsMargins(22, 18, 22, 12);
        textFrameLayout->setSpacing(8);

        textLabel = new QLabel(textFrame);
        textLabel->setWordWrap(true);
        textLabel->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        textLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        textLabel->setContentsMargins(0, 0, 0, 0);
        textLabel->setStyleSheet("background: transparent;");
        textFrameLayout->addWidget(textLabel, 1);

        dialogueNavFrame = new QFrame(textFrame);
        dialogueNavFrame->setStyleSheet("background: transparent; border: none;");
        dialogueNavFrame->setFixedHeight(40);
        auto *dialogueNavLayout = new QHBoxLayout(dialogueNavFrame);
        dialogueNavLayout->setContentsMargins(0, 0, 0, 0);
        dialogueNavLayout->setSpacing(8);

        btnPageBack = new QPushButton("◀", dialogueNavFrame);
        btnPageBack->setFixedSize(44, 34);
        btnPageBack->setCursor(Qt::PointingHandCursor);
        pageIndicator = new QLabel(dialogueNavFrame);
        pageIndicator->setAlignment(Qt::AlignCenter);
        pageIndicator->setMinimumWidth(72);
        btnPageNext = new QPushButton("▶", dialogueNavFrame);
        btnPageNext->setFixedSize(44, 34);
        btnPageNext->setCursor(Qt::PointingHandCursor);

        dialogueNavLayout->addStretch(1);
        dialogueNavLayout->addWidget(btnPageBack);
        dialogueNavLayout->addWidget(pageIndicator);
        dialogueNavLayout->addWidget(btnPageNext);
        textFrameLayout->addWidget(dialogueNavFrame, 0);

        connect(btnPageBack, &QPushButton::clicked, this, [this]() { previousDialoguePage(); });
        connect(btnPageNext, &QPushButton::clicked, this, [this]() { advanceDialoguePage(); });

        typewriterTimer = new QTimer(this);
        typewriterTimer->setInterval(18);
        connect(typewriterTimer, &QTimer::timeout, this, [this]() { typeNextDialogueChar(); });

        choicesFrame = new QFrame(central); choicesLayout = new QVBoxLayout(choicesFrame); choicesLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter); choicesLayout->setSpacing(10);
        choicesFrame->setStyleSheet("background: transparent; border: none;");
        choicesFrame->hide();
        btnA11y = new QPushButton("⚙", central); btnA11y->setCursor(Qt::PointingHandCursor);
        connect(btnA11y, &QPushButton::clicked, this, &VisualNovel::openA11yMenu);
    }

    void setupScenes() {
        QString s = assetPath("sprites") + "/";
        QString b = assetPath("backgrounds") + "/";
        scenes["inicio"] = {
            "A luz fraca da manhã entra pela fresta da cortina. Aurudinha acorda devagar, com a mente ainda confusa, como se o mundo ao redor fosse um borrão. Ela coça os olhos e se senta na cama. Tudo parece pesado demais para uma manhã comum.\n\nEntão, um barulho. Vozes vindas da sala. Sua mãe e seu pai conversam — ou melhor, discutem. As palavras chegam abafadas, mas o tom é cortante.\n\nAurudinha fica parada, sem saber o que fazer.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Ir ver o que aconteceu", "ir_ver"}, {"Permanecer no quarto", "ficar_quarto"}}
        };
        scenes["ir_ver"] = {
            "Aurudinha se levanta da cama, o coração acelerado. Ela caminha até a porta do quarto, abre devagar e segue pelo corredor. A cada passo, as vozes ficam mais claras. Ela se esconde atrás da parede da sala, sem conseguir se mover.\n\nSua mãe fala com uma voz carregada de frustração:\n\n— Você sabe o quão difícil é cuidar de crianças com essa 'doença'... Eu não queria um filho assim. Teria sido melhor se a gente não tivesse ido atrás de um diagnóstico. Será que esse diagnóstico é mesmo verdadeiro?",
            b+"saladacasa.png", "",
            s+"mae/maeseria_no_bg_t4bqtyzu.png",
            s+"pai/paitriste-removebg-preview.png",
            {{"Continuar ouvindo", "ir_ver_2"}}
        };
        scenes["ir_ver_2"] = {
            "Seu pai responde, a voz angustiada, como se cada palavra custasse:\n\n— Devíamos ter prestado mais atenção... Mas não, não podemos fazer nada agora. É aceitar esse diagnóstico. Não quero passar vergonha. Não quero ter um filho que tem essa 'doença'.\n\nAurudinha sente o mundo desabar. Cada palavra é como um golpe. Ela se segura para não fazer barulho, as lágrimas já escorrendo sem permissão.\n\nSe eles têm vergonha... de mim?",
            b+"saladacasa.png", "",
            s+"mae/maeseria_no_bg_t4bqtyzu.png",
            s+"pai/paitriste-removebg-preview.png",
            {{"Voltar para o quarto", "choro_quarto"}}
        };
        scenes["choro_quarto"] = {
            "Aurudinha volta para o quarto em silêncio, fechando a porta com cuidado. Se joga na cama e o choro que estava preso finalmente escapa — mas ela tenta abafar, cobrindo o rosto com o travesseiro.\n\nNa sua cabeça, uma voz insiste:\n\n— Se eu não tivesse nascido autista... meus pais não estariam brigando. A culpa é minha. Eles têm vergonha de mim.\n\nEla seca as lágrimas ao ouvir seu pai chamando:\n\n— Aurudinha! Vamos, você vai se atrasar para a escola!\n\nEla respira fundo, se arruma e tenta compor o rosto. Ninguém pode saber.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "", "",
            {{"Ir para a escola", "escola"}}
        };
        scenes["ficar_quarto"] = {
            "Aurudinha decide não ir ver. Talvez seja melhor assim. Ficar no quarto, no silêncio, longe daquelas vozes. Ela pega o celular e fica rolando a tela sem realmente ver nada, tentando ocupar a mente.\n\nOs minutos passam lentos. O barulho na sala diminui, mas o peso no peito não.\n\nEntão, a voz do pai:\n\n— Aurudinha! Vamos, você vai se atrasar!\n\nEla guarda o celular, se arruma e sai do quarto sem olhar para ninguém.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Ir para a escola", "escola"}}
        };
        scenes["escola"] = {
            "A escola. O portão se abre e Aurudinha entra junto com outras crianças. O barulho já começa aqui — vozes, risadas, passos no corredor. Tudo parece alto demais.\n\nLucas, um colega, se aproxima:\n\n— Oi, Aurudinha! Bom dia!\n\nAurudinha sai do seu transe:\n\n— Oi, Lucas...\n\n— Vamos para a aula? A professora mandou te chamar.\n\nEla acena com a cabeça e segue em direção à sala. Cada passo no corredor parece amplificado.",
            b+"frentedaescola.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "", "",
            {{"Entrar na sala", "sala_aula"}}
        };
        scenes["sala_aula"] = {
            "A sala de aula está cheia. A aula já começou há um tempo, mas o barulho das crianças conversando não diminui. Vozes se misturam, carteiras arranham no chão, alguém ri alto no fundo.\n\nAurudinha senta na sua carteira e tenta se concentrar. Mas o barulho cresce. E cresce. Seu corpo começa a tremer. Suas mãos ficam frias. O coração dispara.\n\nEla sente que o mundo ao redor está se fechando, como se as paredes estivessem se aproximando. Cada som é uma agressão. Ela coloca as mãos sobre os olhos, tentando se proteger.\n\n— Eu não consigo... eu não consigo ficar aqui...",
            b+"saladeaula.png",
            s+"aurudinha/aurudinhamaonacabeça_no_bg_lwjhxt3z.png", "", "",
            {{"Pedir ajuda à professora", "pedir_ajuda"}, {"Sair correndo", "correr"}}
        };
        scenes["pedir_ajuda"] = {
            "Aurudinha se levanta, as pernas tremendo, e vai até a professora. Sua voz sai fraca:\n\n— Professora... eu não estou me sentindo bem...\n\nA professora percebe o sofrimento no rosto dela e responde com gentileza:\n\n— Calma, querida. Vá sentar, eu vou pedir alguém para trazer água.\n\n— Bell, por favor, traga água para a Aurudinha.\n\nBell não responde. A professora repete, agora com mais firmeza:\n\n— Bell! Eu pedi para você pegar uma água para ela!\n\nAurudinha volta para a carteira e coloca as mãos nos ouvidos, tentando abafar o barulho. Então, ouve risadas. Alguns colegas estão rindo dela. O desespero aumenta e as lágrimas começam a cair.\n\n— Por que estão rindo...?",
            b+"saladeaula2.png",
            s+"aurudinha/aurudinhamaonacabeça_no_bg_lwjhxt3z.png",
            s+"professora/professora normal ou aprenseiva_no_bg_i2annqee.png", "",
            {{"Permanecer na sala", "casa"}, {"Sair correndo", "correr"}}
        };
        scenes["correr"] = {
            "Aurudinha não aguenta mais. Seu coração está disparado, a respiração descontrolada. Ela se levanta e sai da sala correndo, sem olhar para trás.\n\nNo corredor, encontra um canto e se senta no chão, abraçando os joelhos, tentando se acalmar. Mas então... passos. Vozes se aproximando.\n\n— Olha só, é aquela menina 'doente' da sala...\n\n— Que pena. Os pais dela devem ter vergonha de uma filha que parece um monstro...\n\nO grupo começa a rir e debochar. A mente de Aurudinha entra em colapso. Tudo ao redor parece consumir seu espaço. Ela treme. Não consegue respirar.\n\n— Eu não sou um monstro... eu não sou...",
            b+"correndopelaescola.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "", "",
            {{"Fugir da escola", "atropelamento"}, {"Ficar e voltar para casa", "casa"}}
        };
        scenes["casa"] = {
            "Quebra de tempo.\n\nAurudinha chega em casa e vai direto para o seu quarto. Se joga na cama e o choro que segurou o dia inteiro finalmente vem, sem filtro, sem controle.\n\nDo outro lado da porta, seu pai ouve. Seu coração dói ao ver a expressão triste no rosto da filha. Ele não sabe a forma certa de agir — a mãe deles não aceitou que a própria filha nasceu no espectro autista, e agora a família está desmoronando.\n\nEle fica parado no corredor, em silêncio. Talvez... talvez seja melhor falar tudo para ela.\n\nAurudinha percebe uma presença. Seu pai está ali, na porta, com olhar preocupado.",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhacomolhartriste_no_bg_ya4gzsbv.png", "",
            s+"pai/paiolhandodelado-removebg-preview.png",
            {{"Contar tudo ao pai", "final_bom"}, {"Não contar nada", "final_ruim"}}
        };
        scenes["final_bom"] = {
            "O pai se aproxima e gentilmente abre a porta. Ele se agacha ao lado da cama, os olhos marejados.\n\n— Você está bem, filha?\n\nAurudinha não aguenta. Corre para os braços do pai e desabafa, contando tudo — a escola, o bullying, as palavras dos pais, a dor de se sentir um fardo.\n\nO pai a abraça forte, e pela primeira vez, diz o que ela precisava ouvir:\n\n— Eu sinto muito, Aurudinha. Você não é um fardo. Você é minha filha e eu vou te apoiar. Sempre.\n\nOs dois se abraçam. A música é triste, mas há esperança. Pela primeira vez em muito tempo, Aurudinha sente que talvez... talvez tudo possa ficar bem.\n\n[ FINAL BOM — O Abraço ]",
            b+"quartodaauruda.png",
            s+"aurudinha/aurudinhachorando_no_bg_z9jgkih3.png", "",
            s+"pai/paisorrindo-removebg-preview.png",
            {{"Reiniciar", "inicio"}}
        };
        scenes["final_ruim"] = {
            "Aurudinha decide não contar nada. Ela engole as lágrimas, limpa o rosto e segue o pai até a cozinha. Senta na cadeira à frente dele, nervosa.\n\nO pai tenta conversar, mas as palavras não saem. O silêncio entre eles é pesado, carregado de tudo o que não é dito.\n\n— Aurudinha... eu...\n\nEle não consegue terminar. Ela não consegue esperar.\n\nQuebra de tempo.\n\nO pai acorda e percebe a casa silenciosa. Estragamente silenciosa. Ele caminha até o quarto da filha e abre a porta devagar.\n\nSilêncio.\n\n[ FINAL RUIM — O Silêncio ]",
            b+"quartodaauruda.png",
            s+"aurudinha/auridnhacomolharmorto_no_bg_myl6f8jd.png", "",
            s+"pai/paitriste-removebg-preview.png",
            {{"Fim", "inicio"}}
        };
        scenes["atropelamento"] = {
            "Aurudinha levanta do chão com a pouca força que ainda tem. Uma ideia passa pela sua mente — e se eu for embora? Talvez se eu fugir, meus pais fiquem melhor...\n\nEla sai correndo. Não olha para trás. Corre pela rua, pelo portão da escola, pela avenida. Não vê o sinal vermelho. Não vê o carro.\n\nO barulho do freio. O impacto. Tudo escurece.\n\n[ FINAL TRÁGICO ]",
            b+"frentedaescola.png",
            s+"aurudinha/auridnhacomolharmorto_no_bg_myl6f8jd.png", "", "",
            {{"Anos depois...", "nova_filha"}}
        };
        scenes["nova_filha"] = {
            "Quebra de tempo.\n\nAlguns anos depois.\n\nA casa que antes era silenciosa agora tem novos sons. Uma criança pequena brinca na sala. A mãe sorri, radiante, segurando sua nova filha.\n\n— Egosinha, vem cá, meu amor!\n\nO pai observa da porta, sem expressão. A nova filha se chama Egosinha. E eles vivem... 'felizes'.\n\nMas o quarto de Aurudinha continua ali. Intocado. Como um segredo que ninguém quer lembrar.\n\n[ FIM ]",
            b+"saladacasa.png", "",
            s+"mae/maefeliz_no_bg_r2s96e1i.png", "",
            {{"Reiniciar", "inicio"}}
        };
    }

    // --- Estilo dos botões de escolha (considera high contrast e colorblind) ---
    QString choiceButtonStyle(int fontSize) {
        if (prefs.high_contrast) {
            return QString("QPushButton { background-color: %1; color: %2; font: bold %3pt; border: 3px solid %4; border-radius: 12px; padding: 12px 20px; } QPushButton:hover { background-color: %4; color: %1; }")
                .arg(HC_BTN_BG).arg(HC_BTN_TEXT).arg(fontSize).arg(HC_BTN_BORDER);
        }
        // Normal: usa cor de accent do palette de daltonismo
        struct Pal { QString accent; };
        std::map<QString, Pal> pals = {{"none", {LILAS}}, {"deuteranopia", {"#ffc100"}}, {"protanopia", {"#ffc100"}}, {"tritanopia", {"#0081a7"}}};
        auto pal = pals[prefs.colorblind];
        return QString("QPushButton { background-color: rgba(255, 240, 245, 180); color: %1; font: bold %2pt; border: 2px solid %3; border-radius: 12px; padding: 12px 20px; } QPushButton:hover { background-color: %3; color: white; }")
            .arg(TEXTO_ESCURO).arg(fontSize).arg(pal.accent);
    }

    void updateChoiceButtonStyles() {
        int fontSize = qMax(12, (int)(14 * prefs.font_scale / 100));
        int minHeight = qMax(50, (int)(52 * prefs.font_scale / 100));
        for (int i = 0; i < choicesLayout->count(); ++i) {
            auto *btn = qobject_cast<QPushButton*>(choicesLayout->itemAt(i)->widget());
            if (!btn) continue;
            btn->setMinimumWidth(qMax(320, (int)(width() * 0.4)));
            btn->setMinimumHeight(minHeight);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            btn->setStyleSheet(choiceButtonStyle(fontSize));
        }
    }

    bool shouldReturnToMenu(const QString &sceneId, const Choice &choice) const {
        if (choice.nextScene != "inicio") return false;
        return sceneId == "final_bom" || sceneId == "final_ruim" || sceneId == "nova_filha";
    }

    void unlockEndingForScene(const QString &sceneId) {
        bool changed = false;
        if (sceneId == "final_bom" && !prefs.ending_good_unlocked) {
            prefs.ending_good_unlocked = true;
            changed = true;
        } else if (sceneId == "final_ruim" && !prefs.ending_silence_unlocked) {
            prefs.ending_silence_unlocked = true;
            changed = true;
        } else if ((sceneId == "atropelamento" || sceneId == "nova_filha") && !prefs.ending_tragic_unlocked) {
            prefs.ending_tragic_unlocked = true;
            changed = true;
        }

        if (changed) savePrefs();
    }

    void returnToMenu() {
#ifndef __EMSCRIPTEN__
        speech->stop();
#endif
        typewriterTimer->stop();
        savePrefs();
        close();
        if (returnToMenuCallback) returnToMenuCallback();
    }

    void selectChoice(int idx) {
        auto it = scenes.find(currentSceneId);
        if (it == scenes.end() || idx < 0 || idx >= (int)it->second.choices.size()) return;

        const Choice choice = it->second.choices[idx];
        if (shouldReturnToMenu(currentSceneId, choice)) {
            returnToMenu();
        } else {
            goToScene(choice.nextScene);
        }
    }

#ifndef __EMSCRIPTEN__
    void speakText(const QString &text) {
        if (!prefs.text_to_speech || text.trimmed().isEmpty()) {
            audio.duckMusic(false);
            return;
        }

        if (QTextToSpeech::availableEngines().isEmpty()) {
            qWarning() << "Leitor de texto ativado, mas nenhum backend TTS foi encontrado.";
            return;
        }

        audio.duckMusic(true);
        speech->stop();
        speech->say(text.trimmed());
    }

    void speakCurrentDialoguePage() {
        if (dialoguePages.isEmpty() || currentDialoguePage < 0 || currentDialoguePage >= dialoguePages.size()) return;
        speakText(dialoguePages.at(currentDialoguePage));
    }
#endif

    bool dialogueTextFits(const QString &text, int textWidth, int textHeight) const {
        if (text.trimmed().isEmpty()) return true;
        QFontMetrics fm(textLabel->font());
        QRect bounds = fm.boundingRect(QRect(0, 0, textWidth, 10000), Qt::TextWordWrap, text.trimmed());
        return bounds.height() <= textHeight;
    }

    bool isSentenceBoundary(QChar c) const {
        return c == '.' || c == '!' || c == '?' || c == ':' || c == ';' || c == QChar(0x2026) || c == ']';
    }

    QStringList splitDialogueUnits(const QString &text) const {
        QStringList units;
        int start = 0;
        for (int i = 0; i < text.size(); ++i) {
            bool boundary = false;
            int end = i + 1;

            if (i + 1 < text.size() && text.mid(i, 2) == "\n\n") {
                boundary = true;
                end = i + 2;
                ++i;
            } else if (isSentenceBoundary(text.at(i))) {
                boundary = true;
                while (end < text.size() && (text.at(end).isSpace() || text.at(end) == '"' || text.at(end) == '\'')) ++end;
            }

            if (boundary && end > start) {
                units.append(text.mid(start, end - start));
                start = end;
            }
        }

        if (start < text.size()) units.append(text.mid(start));
        return units;
    }

    void appendSplitUnit(QString unit, QStringList &pages, QString &current, int textWidth, int textHeight) const {
        const QStringList words = unit.simplified().split(' ', Qt::SkipEmptyParts);
        for (const QString &word : words) {
            QString candidate = current.trimmed().isEmpty() ? word : current.trimmed() + " " + word;
            if (dialogueTextFits(candidate, textWidth, textHeight)) {
                current = candidate;
                continue;
            }

            if (!current.trimmed().isEmpty()) {
                pages.append(current.trimmed());
                current.clear();
            }

            current = word;
        }
    }

    QStringList paginateDialogue(const QString &text, int textWidth, int textHeight) const {
        QStringList pages;
        QString current;
        for (const QString &unit : splitDialogueUnits(text)) {
            QString candidate = current + unit;
            if (dialogueTextFits(candidate, textWidth, textHeight)) {
                current = candidate;
                continue;
            }

            if (!current.trimmed().isEmpty()) {
                pages.append(current.trimmed());
                current.clear();
            }

            if (dialogueTextFits(unit, textWidth, textHeight)) {
                current = unit;
            } else {
                appendSplitUnit(unit, pages, current, textWidth, textHeight);
            }
        }

        if (!current.trimmed().isEmpty()) pages.append(current.trimmed());
        if (pages.isEmpty()) pages.append(text.trimmed());
        return pages;
    }

    int dialogueTextWidth() const {
        return qMax(260, textFrame->width() - 48);
    }

    int dialogueTextHeight() const {
        return qMax(90, textFrame->height() - dialogueNavFrame->height() - 56);
    }

    void rebuildDialoguePages(bool keepPage, bool animate) {
        if (fullDialogueText.isEmpty() || textFrame->width() <= 0 || textFrame->height() <= 0) return;

        int oldPage = currentDialoguePage;
        dialoguePages = paginateDialogue(fullDialogueText, dialogueTextWidth(), dialogueTextHeight());
        currentDialoguePage = keepPage ? qBound(0, oldPage, dialoguePages.size() - 1) : 0;
        showDialoguePage(animate && !keepPage);
    }

    void showDialoguePage(bool animate) {
        if (dialoguePages.isEmpty()) {
            textLabel->clear();
            return;
        }

        typewriterTimer->stop();
        const QString page = dialoguePages.at(currentDialoguePage);
        if (animate && !prefs.reduce_motion) {
            visibleDialogueChars = 0;
            typeSoundCounter = 0;
            dialogueTyping = true;
            textLabel->clear();
            typewriterTimer->start();
        } else {
            visibleDialogueChars = page.size();
            dialogueTyping = false;
            textLabel->setText(page);
#ifndef __EMSCRIPTEN__
            speakCurrentDialoguePage();
#endif
        }
        updateDialogueControls();
    }

    void typeNextDialogueChar() {
        if (dialoguePages.isEmpty()) return;

        const QString page = dialoguePages.at(currentDialoguePage);
        visibleDialogueChars = qMin(visibleDialogueChars + 2, page.size());
        textLabel->setText(page.left(visibleDialogueChars));
        if (++typeSoundCounter % 2 == 0) {
            audio.playTypewriterClick(prefs.reduce_motion);
        }

        if (visibleDialogueChars >= page.size()) {
            typewriterTimer->stop();
            dialogueTyping = false;
#ifndef __EMSCRIPTEN__
            speakCurrentDialoguePage();
#endif
        }
        updateDialogueControls();
    }

    void revealDialoguePage() {
        if (dialoguePages.isEmpty()) return;

        typewriterTimer->stop();
        const QString page = dialoguePages.at(currentDialoguePage);
        visibleDialogueChars = page.size();
        dialogueTyping = false;
        textLabel->setText(page);
#ifndef __EMSCRIPTEN__
        speakCurrentDialoguePage();
#endif
        updateDialogueControls();
    }

    void advanceDialoguePage() {
        if (dialogueTyping) {
            revealDialoguePage();
            return;
        }

        if (currentDialoguePage + 1 < dialoguePages.size()) {
            ++currentDialoguePage;
            showDialoguePage(true);
        }
    }

    void previousDialoguePage() {
        if (currentDialoguePage <= 0) return;
        --currentDialoguePage;
        showDialoguePage(false);
    }

    void updateDialogueControls() {
        const bool hasPages = !dialoguePages.isEmpty();
        const bool onLastPage = hasPages && currentDialoguePage == dialoguePages.size() - 1;
        const bool pageComplete = hasPages && !dialogueTyping;

        btnPageBack->setEnabled(hasPages && currentDialoguePage > 0);
        btnPageNext->setEnabled(hasPages && (dialogueTyping || currentDialoguePage + 1 < dialoguePages.size()));
        pageIndicator->setText(hasPages ? QString("%1/%2").arg(currentDialoguePage + 1).arg(dialoguePages.size()) : QString());
        choicesFrame->setVisible(onLastPage && pageComplete);
    }

    void goToScene(QString id) {
#ifndef __EMSCRIPTEN__
        speech->stop();
#endif
        typewriterTimer->stop();
        currentSceneId = id; Scene &s = scenes[id]; fullDialogueText = s.text; textLabel->clear();
        unlockEndingForScene(id);
        dialoguePages.clear();
        currentDialoguePage = 0;
        visibleDialogueChars = 0;
        dialogueTyping = false;
#ifndef __EMSCRIPTEN__
        if (!prefs.text_to_speech) audio.duckMusic(false);
#endif
        updateSprites();
        QLayoutItem *item; while ((item = choicesLayout->takeAt(0)) != nullptr) { if (item->widget()) delete item->widget(); delete item; }
        int fontSize = qMax(12, (int)(14 * prefs.font_scale / 100));
        for (int i = 0; i < (int)s.choices.size(); ++i) {
            const auto &c = s.choices[i];
            auto *btn = new QPushButton(c.text);
            btn->setMinimumWidth(qMax(320, (int)(width() * 0.4)));
            btn->setMinimumHeight(qMax(50, (int)(52 * prefs.font_scale / 100)));
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            btn->setStyleSheet(choiceButtonStyle(fontSize));
            connect(btn, &QPushButton::clicked, this, [this, i](){ selectChoice(i); });
            choicesLayout->addWidget(btn);
        }
        choicesFrame->hide();
        QTimer::singleShot(10, this, [this]() {
            adjustTextLayout();
            rebuildDialoguePages(false, true);
        });
    }

    void adjustTextLayout() {
        int w = width(), h = height();
        if (w == 0 || h == 0) return;

        int gap = qMax(12, (int)(h * 0.018));
        int choiceCount = choicesLayout->count();
        int choiceButtonH = qMax(50, (int)(52 * prefs.font_scale / 100));
        int desiredChoicesH = choiceCount > 0 ? choiceCount * choiceButtonH + qMax(0, choiceCount - 1) * gap + 12 : 0;
        int choicesH = choiceCount > 0 ? qMin(desiredChoicesH, qMax(90, (int)(h * 0.22))) : 0;
        choicesLayout->setSpacing(gap);

        int textAreaW = prefs.reading_mode ? (int)(w * 0.72) : (int)(w * 0.86);
        int minTextH = qMax(170, (int)(h * 0.24));
        int targetTextH = qBound(minTextH, (int)(h * 0.28), qMax(minTextH, (int)(h * 0.34)));
        int textY = qMax(24, (int)(h * 0.035));

        if (prefs.reading_mode) {
            textFrame->setGeometry((w - textAreaW) / 2, textY, textAreaW, targetTextH);
        } else {
            textFrame->setGeometry((w - textAreaW) / 2, textY, textAreaW, targetTextH);
        }

        textLabel->setMinimumWidth(dialogueTextWidth());
        textLabel->setMaximumWidth(dialogueTextWidth());

        int choicesY = textFrame->y() + textFrame->height() + gap;
        choicesFrame->setGeometry((int)(w * 0.12), choicesY, (int)(w * 0.76), qMax(70, choicesH));
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
        } else {
            bgLabel->clear();
            qWarning() << "Falha ao carregar background:" << s.background;
        }
        int spriteH = (int)(h * 0.55);
        int spriteW = (int)(w * 0.4);
        auto load = [this, spriteW, spriteH](QLabel *l, QString p) {
            if (p.isEmpty()) { l->clear(); return; }
            QPixmap pix(p);
            if (!pix.isNull()) {
                l->setPixmap(pix.scaled(spriteW, spriteH, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                l->clear();
                qWarning() << "Falha ao carregar sprite:" << p;
            }
        };
        load(spriteCenter, s.centerSprite); load(spriteLeft, s.leftSprite); load(spriteRight, s.rightSprite);
        // Z-order: bg < sprites < overlay < text < choices < a11y
        bgLabel->lower();
        spriteLeft->raise(); spriteRight->raise(); spriteCenter->raise();
        colorOverlay->raise();
        textFrame->raise(); choicesFrame->raise(); btnA11y->raise();
    }

    void openA11yMenu() {
#ifndef __EMSCRIPTEN__
        speech->stop();
#endif
        AccessibilityMenu menu(this, prefs, [=](const Prefs &p){ applyPrefs(p); savePrefs(); });
        if (menu.exec() == QDialog::Accepted) {
            applyPrefs(prefs);
            revealDialoguePage();
        }
    }

    void applyPrefs(const Prefs &p) {
        prefs = p;
        applyApplicationPalette(prefs);
#ifndef __EMSCRIPTEN__
        if (!prefs.text_to_speech) {
            speech->stop();
            audio.duckMusic(false);
        }
#endif
        int w = width(), h = height();
        bgLabel->setGeometry(0, 0, w, h);
        spriteLeft->setGeometry(0, 0, w/2, h); spriteRight->setGeometry(w/2, 0, w/2, h); spriteCenter->setGeometry(0, 0, w, h);

        // Atualizar overlay de daltonismo
        colorOverlay->setGeometry(0, 0, w, h);
        colorOverlay->setMode(prefs.colorblind);

        int fontSize = qMax(14, (int)(20 * prefs.font_scale / 100));
        int iconButtonFont = qMax(18, (int)(22 * prefs.font_scale / 100));
        int pageButtonFont = qMax(12, (int)(14 * prefs.font_scale / 100));
        int indicatorFont = qMax(10, (int)(11 * prefs.font_scale / 100));
        if (prefs.high_contrast) {
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(HC_BG));
            textFrame->setStyleSheet(QString("background-color: rgba(26, 26, 26, 230); border: 4px solid %1; border-radius: 8px;").arg(HC_BORDER));
            textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; background: transparent;").arg(HC_TEXT).arg(fontSize));
            pageIndicator->setStyleSheet(QString("color: %1; font: bold %2pt; background: transparent;").arg(HC_TEXT).arg(indicatorFont));
            btnA11y->setStyleSheet(QString("QPushButton { background-color: %1; color: %2; font: bold %3pt; border: 2px solid %4; border-radius: 8px; padding: 0px; } QPushButton:hover { background-color: %4; color: %1; }").arg(HC_BTN_BG).arg(HC_BTN_TEXT).arg(iconButtonFont).arg(HC_BTN_BORDER));
            QString pageButtonStyle = QString("QPushButton { background-color: %1; color: %2; font: bold %3pt; border: 2px solid %4; border-radius: 6px; padding: 0px; } QPushButton:hover { background-color: %4; color: %1; } QPushButton:disabled { color: #777777; border-color: #777777; }")
                .arg(HC_BTN_BG).arg(HC_BTN_TEXT).arg(pageButtonFont).arg(HC_BTN_BORDER);
            btnPageBack->setStyleSheet(pageButtonStyle);
            btnPageNext->setStyleSheet(pageButtonStyle);
        } else {
            // Paleta normal com suporte a daltonismo
            struct Pal { QString bg, accent, text; };
            std::map<QString, Pal> pals = {
                {"none", {ROSA, LILAS, TEXTO_ESCURO}},
                {"deuteranopia", {"#005b96", "#ffc100", "#000000"}},
                {"protanopia", {"#004d80", "#ff9500", "#000000"}},
                {"tritanopia", {"#c81d25", "#0081a7", "#000000"}}
            };
            auto pal = pals[prefs.colorblind];
            centralWidget()->setStyleSheet(QString("background-color: %1;").arg(pal.bg));
            textFrame->setStyleSheet(QString("background-color: rgba(255, 240, 245, 235); border: 4px solid %1; border-radius: 8px;").arg(pal.accent));
            textLabel->setStyleSheet(QString("color: %1; font: bold %2pt; background: transparent;").arg(pal.text).arg(fontSize));
            pageIndicator->setStyleSheet(QString("color: %1; font: bold %2pt; background: transparent;").arg(pal.text).arg(indicatorFont));
            btnA11y->setStyleSheet(QString("QPushButton { background-color: rgba(201,160,220,220); color: white; font: bold %1pt; border: 2px solid white; border-radius: 8px; padding: 0px; } QPushButton:hover { background-color: %2; }").arg(iconButtonFont).arg(pal.accent));
            QString pageButtonStyle = QString("QPushButton { background-color: %1; color: white; font: bold %2pt; border: 2px solid white; border-radius: 6px; padding: 0px; } QPushButton:hover { background-color: #E8A0B0; } QPushButton:disabled { background-color: rgba(201,160,220,90); color: rgba(255,255,255,120); }")
                .arg(pal.accent).arg(pageButtonFont);
            btnPageBack->setStyleSheet(pageButtonStyle);
            btnPageNext->setStyleSheet(pageButtonStyle);
        }
        int configSize = qMax(48, (int)(52 * prefs.font_scale / 100));
        int margin = qMax(18, (int)(w * 0.015));
        btnA11y->setGeometry(w - configSize - margin, margin, configSize, configSize);
        updateChoiceButtonStyles();
        updateSprites();
        adjustTextLayout();
        rebuildDialoguePages(true, false);
    }
    void savePrefs() { savePrefsToDisk(prefs); }
};

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Prefs prefs = loadPrefsFromDisk();
    applyApplicationPalette(prefs);
    AudioManager audio;
    audio.startMusic();

    bool playing = false;
    bool running = true;
    while (running) {
        playing = false;
        MainMenu menu(prefs, [&playing, &prefs]() { playing = true; savePrefsToDisk(prefs); });
        if (menu.exec() == QDialog::Rejected) break;
        if (playing) {
            bool backToMenu = false;
            VisualNovel v(prefs, audio, [&]() {
                backToMenu = true;
                qApp->quit();
            });
            v.show();
            int result = a.exec();
            savePrefsToDisk(prefs);
            if (backToMenu) {
                audio.startMusic();
                continue;
            }
            return result;
        }
    }
    return 0;
}
