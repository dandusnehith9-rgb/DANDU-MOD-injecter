#pragma once

#include <QString>
#include <QStringList>

enum class LoaderType
{
    Unknown,
    NeoForge,
    Fabric,
    Quilt
};

inline QString loaderTypeToString(LoaderType loader)
{
    switch (loader)
    {
    case LoaderType::NeoForge:
        return QStringLiteral("NeoForge");
    case LoaderType::Fabric:
        return QStringLiteral("Fabric");
    case LoaderType::Quilt:
        return QStringLiteral("Quilt");
    default:
        return QStringLiteral("Unknown");
    }
}

struct ModInfo
{
    bool valid = false;
    QString filePath;
    QString modId;
    QString displayName;
    QString modVersion;
    LoaderType loader = LoaderType::Unknown;
    QString minecraftConstraint;
    QStringList warnings;
    QString rawLoaderHint;
};

struct ProfileInfo
{
    QString key;
    QString name;
    QString lastVersionId;
    QString gameDir;
    QString sourceFile;
    LoaderType loaderHint = LoaderType::Unknown;
};

struct InstallResult
{
    bool ok = false;
    QString message;
    QString destination;
    QString profileName;
};
