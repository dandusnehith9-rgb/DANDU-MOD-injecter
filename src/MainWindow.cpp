#include "MainWindow.h"

#include "DropCard.h"
#include "ModAnalyzer.h"
#include "VersionMatcher.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("DANDU MOD injecter");
    setMinimumSize(980, 700);
    buildUi();
    refreshProfileList();
}

void MainWindow::buildUi()
{
    QWidget *root = new QWidget(this);
    setCentralWidget(root);

    QVBoxLayout *mainLayout = new QVBoxLayout(root);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(14);

    QLabel *title = new QLabel("DANDU MOD injecter", this);
    title->setObjectName("titleLabel");
    QLabel *subtitle = new QLabel("Drop one mod JAR. The app detects NeoForge / Fabric / Quilt, checks version, and installs it.", this);
    subtitle->setObjectName("subTitleLabel");
    subtitle->setWordWrap(true);
    mainLayout->addWidget(title);
    mainLayout->addWidget(subtitle);

    QHBoxLayout *profileRow = new QHBoxLayout();
    QLabel *profileLabel = new QLabel("Minecraft Profile:", this);
    m_profileCombo = new QComboBox(this);
    QPushButton *refreshButton = new QPushButton("Refresh Profiles", this);
    profileRow->addWidget(profileLabel);
    profileRow->addWidget(m_profileCombo, 1);
    profileRow->addWidget(refreshButton);
    mainLayout->addLayout(profileRow);

    m_dropCard = new DropCard(this);
    mainLayout->addWidget(m_dropCard);

    QWidget *infoCard = new QWidget(this);
    infoCard->setObjectName("infoCard");
    QGridLayout *infoGrid = new QGridLayout(infoCard);
    infoGrid->setContentsMargins(16, 16, 16, 16);
    infoGrid->setHorizontalSpacing(10);
    infoGrid->setVerticalSpacing(8);

    auto addInfoRow = [&](int row, const QString &labelText, QLabel **valueOut) {
        QLabel *label = new QLabel(labelText, infoCard);
        label->setObjectName("infoKey");
        QLabel *value = new QLabel("-", infoCard);
        value->setObjectName("infoValue");
        value->setWordWrap(true);
        infoGrid->addWidget(label, row, 0);
        infoGrid->addWidget(value, row, 1);
        *valueOut = value;
    };

    addInfoRow(0, "Mod Name", &m_modNameValue);
    addInfoRow(1, "Mod ID", &m_modIdValue);
    addInfoRow(2, "Mod Version", &m_modVersionValue);
    addInfoRow(3, "Loader Type", &m_loaderValue);
    addInfoRow(4, "Minecraft Version", &m_mcConstraintValue);
    addInfoRow(5, "Target Profile", &m_targetValue);
    mainLayout->addWidget(infoCard);

    m_infoOpacity = new QGraphicsOpacityEffect(infoCard);
    m_infoOpacity->setOpacity(1.0);
    infoCard->setGraphicsEffect(m_infoOpacity);
    m_infoAnim = new QPropertyAnimation(m_infoOpacity, "opacity", this);
    m_infoAnim->setDuration(260);
    m_infoAnim->setEasingCurve(QEasingCurve::OutCubic);

    QHBoxLayout *actionsRow = new QHBoxLayout();
    m_autoInstallCheck = new QCheckBox("Auto install after drop", this);
    m_autoCreateProfileCheck = new QCheckBox("Auto create profile if needed", this);
    m_autoInstallCheck->setChecked(true);
    m_autoCreateProfileCheck->setChecked(true);
    actionsRow->addWidget(m_autoInstallCheck);
    actionsRow->addWidget(m_autoCreateProfileCheck);
    actionsRow->addStretch(1);
    mainLayout->addLayout(actionsRow);

    QHBoxLayout *buttonsRow = new QHBoxLayout();
    m_installButton = new QPushButton("Install Mod", this);
    m_openModsButton = new QPushButton("Open Mods Folder", this);
    m_installButton->setEnabled(false);
    buttonsRow->addWidget(m_installButton);
    buttonsRow->addWidget(m_openModsButton);
    buttonsRow->addStretch(1);
    mainLayout->addLayout(buttonsRow);

    m_log = new QPlainTextEdit(this);
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(400);
    m_log->setObjectName("logBox");
    mainLayout->addWidget(m_log, 1);

    connect(m_dropCard, &DropCard::fileDropped, this, &MainWindow::onFileDropped);
    connect(m_installButton, &QPushButton::clicked, this, &MainWindow::onInstallClicked);
    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshProfiles);
    connect(m_openModsButton, &QPushButton::clicked, this, &MainWindow::onOpenModsFolder);

    setStyleSheet(R"(
        QWidget {
            background: #0d1420;
            color: #e8edf6;
            font-family: "Segoe UI";
            font-size: 10pt;
        }
        #titleLabel {
            font-size: 22pt;
            font-weight: 700;
            color: #f8fbff;
        }
        #subTitleLabel {
            color: #b4c4dd;
        }
        QPushButton {
            background: #1d2b3f;
            border: 1px solid #3b557c;
            border-radius: 10px;
            padding: 8px 14px;
            color: #f5f8ff;
        }
        QPushButton:hover {
            background: #244068;
            border-color: #5f8fd0;
        }
        QPushButton:disabled {
            color: #8fa4c2;
            border-color: #3a4352;
            background: #1a212c;
        }
        QComboBox {
            background: #111c2d;
            border: 1px solid #385179;
            border-radius: 8px;
            padding: 6px 8px;
            min-height: 28px;
        }
        QComboBox QAbstractItemView {
            background: #0f1725;
            border: 1px solid #335180;
            selection-background-color: #2b4572;
            color: #e7ecf7;
        }
        #infoCard {
            background: #111d2d;
            border: 1px solid #2f4b73;
            border-radius: 14px;
        }
        #infoKey {
            color: #9db6d7;
            font-weight: 600;
            min-width: 150px;
        }
        #infoValue {
            color: #ecf3ff;
            font-weight: 500;
        }
        #logBox {
            background: #0d1726;
            border: 1px solid #2b4670;
            border-radius: 10px;
            color: #c5d5ee;
        }
        QCheckBox {
            color: #c8d7ee;
        }
    )");
}

void MainWindow::refreshProfileList()
{
    m_profiles = m_manager.loadProfiles();
    m_profileCombo->clear();

    for (const ProfileInfo &profile : m_profiles)
    {
        const QString versionPart = profile.lastVersionId.isEmpty() ? QStringLiteral("no version id") : profile.lastVersionId;
        m_profileCombo->addItem(QString("%1 (%2)").arg(profile.name, versionPart));
    }

    logLine(QString("Loaded %1 profile(s) from %2").arg(m_profiles.size()).arg(m_manager.minecraftRoot()));
}

void MainWindow::populateModInfo(const ModInfo &mod)
{
    m_modNameValue->setText(mod.displayName.isEmpty() ? "-" : mod.displayName);
    m_modIdValue->setText(mod.modId.isEmpty() ? "-" : mod.modId);
    m_modVersionValue->setText(mod.modVersion.isEmpty() ? "-" : mod.modVersion);
    m_loaderValue->setText(loaderTypeToString(mod.loader));
    m_mcConstraintValue->setText(mod.minecraftConstraint.isEmpty() ? "Any / not specified" : mod.minecraftConstraint);
}

void MainWindow::logLine(const QString &line, bool error)
{
    if (error)
    {
        m_log->appendPlainText("[ERROR] " + line);
    }
    else
    {
        m_log->appendPlainText(line);
    }
}

ProfileInfo MainWindow::chooseTargetProfile(QStringList *notes)
{
    ProfileInfo selected;
    if (m_profiles.isEmpty())
    {
        if (notes)
        {
            notes->append("No profile list found, using default .minecraft/mods path.");
        }
        return selected;
    }

    const int currentIndex = m_profileCombo->currentIndex();
    if (currentIndex >= 0 && currentIndex < m_profiles.size())
    {
        selected = m_profiles[currentIndex];
    }
    else
    {
        selected = m_profiles.first();
    }

    if (m_manager.isProfileCompatible(selected, m_currentMod))
    {
        return selected;
    }

    const int bestIndex = m_manager.bestProfileIndexForMod(m_currentMod);
    if (bestIndex >= 0 && bestIndex < m_profiles.size())
    {
        const ProfileInfo best = m_profiles[bestIndex];
        if (m_manager.isProfileCompatible(best, m_currentMod))
        {
            if (notes)
            {
                notes->append(QString("Auto-switched to compatible profile: %1").arg(best.name));
            }
            m_profileCombo->setCurrentIndex(bestIndex);
            return best;
        }
    }

    if (m_autoCreateProfileCheck->isChecked())
    {
        QString createError;
        const ProfileInfo created = m_manager.createCompatibleProfile(m_currentMod, &createError);
        if (!created.key.isEmpty())
        {
            if (notes)
            {
                notes->append(QString("Created profile automatically: %1").arg(created.name));
            }
            refreshProfileList();
            for (int i = 0; i < m_profiles.size(); ++i)
            {
                if (m_profiles[i].key == created.key)
                {
                    m_profileCombo->setCurrentIndex(i);
                    return m_profiles[i];
                }
            }
            return created;
        }

        if (!createError.isEmpty() && notes)
        {
            notes->append(createError);
        }
    }

    if (notes)
    {
        notes->append("No exact compatible profile found, using selected profile.");
    }
    return selected;
}

void MainWindow::playInfoAnimation()
{
    m_infoAnim->stop();
    m_infoOpacity->setOpacity(0.1);
    m_infoAnim->setStartValue(0.1);
    m_infoAnim->setEndValue(1.0);
    m_infoAnim->start();
}

void MainWindow::onFileDropped(const QString &path)
{
    m_currentMod = ModAnalyzer::analyze(path);
    populateModInfo(m_currentMod);
    playInfoAnimation();

    if (!m_currentMod.valid)
    {
        m_installButton->setEnabled(false);
        logLine(QString("Could not parse mod file: %1").arg(path), true);
        for (const QString &warning : m_currentMod.warnings)
        {
            logLine(warning, true);
        }
        return;
    }

    m_installButton->setEnabled(true);
    logLine(QString("Detected %1 mod '%2' (%3).")
                .arg(loaderTypeToString(m_currentMod.loader), m_currentMod.displayName, m_currentMod.modVersion));
    if (!m_currentMod.minecraftConstraint.isEmpty())
    {
        logLine(QString("Minecraft version constraint: %1").arg(m_currentMod.minecraftConstraint));
    }
    for (const QString &warning : m_currentMod.warnings)
    {
        logLine(warning, true);
    }

    QStringList notes;
    const ProfileInfo target = chooseTargetProfile(&notes);
    m_targetValue->setText(target.name.isEmpty() ? "Default .minecraft/mods" : target.name);
    for (const QString &note : notes)
    {
        logLine(note);
    }

    if (m_autoInstallCheck->isChecked())
    {
        onInstallClicked();
    }
}

void MainWindow::onInstallClicked()
{
    if (!m_currentMod.valid)
    {
        logLine("No valid mod loaded. Drop a mod JAR first.", true);
        return;
    }

    QStringList notes;
    ProfileInfo target = chooseTargetProfile(&notes);

    for (const QString &note : notes)
    {
        logLine(note);
    }

    if (target.name.isEmpty())
    {
        target.name = "Default";
        target.gameDir = "";
        target.lastVersionId = "";
    }

    const InstallResult install = m_manager.installToProfile(m_currentMod.filePath, target);
    m_targetValue->setText(target.name);

    if (!install.ok)
    {
        logLine(install.message, true);
        return;
    }

    logLine(install.message);
    logLine(QString("Profile used: %1").arg(install.profileName));
}

void MainWindow::onRefreshProfiles()
{
    refreshProfileList();
}

void MainWindow::onOpenModsFolder()
{
    ProfileInfo target;
    if (!m_profiles.isEmpty())
    {
        const int idx = m_profileCombo->currentIndex();
        if (idx >= 0 && idx < m_profiles.size())
        {
            target = m_profiles[idx];
        }
    }

    const QString folderPath = m_manager.modsDirectory(target);
    QDesktopServices::openUrl(QUrl::fromLocalFile(folderPath));
    logLine(QString("Opened mods folder: %1").arg(folderPath));
}
