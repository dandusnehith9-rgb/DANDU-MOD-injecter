#pragma once

#include "MinecraftManager.h"
#include "ModTypes.h"

#include <QMainWindow>

class DropCard;
class QCheckBox;
class QComboBox;
class QGraphicsOpacityEffect;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QPropertyAnimation;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onFileDropped(const QString &path);
    void onInstallClicked();
    void onRefreshProfiles();
    void onOpenModsFolder();

private:
    void buildUi();
    void refreshProfileList();
    void populateModInfo(const ModInfo &mod);
    void logLine(const QString &line, bool error = false);
    ProfileInfo chooseTargetProfile(QStringList *notes);
    void playInfoAnimation();

    MinecraftManager m_manager;
    QList<ProfileInfo> m_profiles;
    ModInfo m_currentMod;

    DropCard *m_dropCard = nullptr;
    QComboBox *m_profileCombo = nullptr;
    QLabel *m_modNameValue = nullptr;
    QLabel *m_modIdValue = nullptr;
    QLabel *m_modVersionValue = nullptr;
    QLabel *m_loaderValue = nullptr;
    QLabel *m_mcConstraintValue = nullptr;
    QLabel *m_targetValue = nullptr;
    QCheckBox *m_autoInstallCheck = nullptr;
    QCheckBox *m_autoCreateProfileCheck = nullptr;
    QPushButton *m_installButton = nullptr;
    QPushButton *m_openModsButton = nullptr;
    QPlainTextEdit *m_log = nullptr;
    QGraphicsOpacityEffect *m_infoOpacity = nullptr;
    QPropertyAnimation *m_infoAnim = nullptr;
};
