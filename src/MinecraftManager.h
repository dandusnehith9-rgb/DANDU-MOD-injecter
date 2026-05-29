#pragma once

#include "ModTypes.h"

#include <QList>
#include <QStringList>

class MinecraftManager
{
public:
    MinecraftManager();

    QList<ProfileInfo> loadProfiles();
    QList<ProfileInfo> profiles() const;
    QString minecraftRoot() const;

    int bestProfileIndexForMod(const ModInfo &mod) const;
    bool isProfileCompatible(const ProfileInfo &profile, const ModInfo &mod) const;
    ProfileInfo createCompatibleProfile(const ModInfo &mod, QString *errorOut);
    InstallResult installToProfile(const QString &jarPath, const ProfileInfo &profile) const;
    QString modsDirectory(const ProfileInfo &profile) const;

private:
    static LoaderType inferProfileLoader(const QString &versionId);
    static bool loaderCompatible(LoaderType modLoader, LoaderType profileLoader, const QString &versionId);
    static bool versionCompatible(const QString &profileVersionId, const QString &constraint);

    QStringList profileFiles() const;
    QStringList installedVersionIds() const;
    QString pickVersionIdForMod(const ModInfo &mod) const;

    QString m_mcRoot;
    QList<ProfileInfo> m_profiles;
};
