#include "MinecraftManager.h"

#include "VersionMatcher.h"

#include <algorithm>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

MinecraftManager::MinecraftManager()
{
    const QString appData = qEnvironmentVariable("APPDATA");
    if (!appData.isEmpty())
    {
        m_mcRoot = QDir(appData).filePath(".minecraft");
    }
    else
    {
        m_mcRoot = QDir::homePath() + "/.minecraft";
    }
}

QList<ProfileInfo> MinecraftManager::loadProfiles()
{
    m_profiles.clear();

    const QStringList files = profileFiles();
    for (const QString &profileFile : files)
    {
        QFile file(profileFile);
        if (!file.exists() || !file.open(QIODevice::ReadOnly))
        {
            continue;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (!doc.isObject())
        {
            continue;
        }

        const QJsonObject root = doc.object();
        const QJsonObject profilesObj = root.value("profiles").toObject();

        for (auto it = profilesObj.begin(); it != profilesObj.end(); ++it)
        {
            if (!it.value().isObject())
            {
                continue;
            }

            const QJsonObject profileObj = it.value().toObject();
            ProfileInfo profile;
            profile.key = it.key();
            profile.name = profileObj.value("name").toString(it.key());
            profile.lastVersionId = profileObj.value("lastVersionId").toString();
            profile.gameDir = profileObj.value("gameDir").toString();
            profile.sourceFile = profileFile;
            profile.loaderHint = inferProfileLoader(profile.lastVersionId);
            m_profiles.append(profile);
        }
    }

    if (m_profiles.isEmpty())
    {
        ProfileInfo fallback;
        fallback.key = "default";
        fallback.name = "Default";
        fallback.lastVersionId = "";
        fallback.gameDir = "";
        fallback.sourceFile = profileFiles().value(0);
        fallback.loaderHint = LoaderType::Unknown;
        m_profiles.append(fallback);
    }

    std::sort(m_profiles.begin(), m_profiles.end(), [](const ProfileInfo &a, const ProfileInfo &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    return m_profiles;
}

QList<ProfileInfo> MinecraftManager::profiles() const
{
    return m_profiles;
}

QString MinecraftManager::minecraftRoot() const
{
    return m_mcRoot;
}

int MinecraftManager::bestProfileIndexForMod(const ModInfo &mod) const
{
    int bestIndex = -1;
    int bestScore = -100000;

    for (int i = 0; i < m_profiles.size(); ++i)
    {
        const ProfileInfo &profile = m_profiles[i];
        int score = 0;

        const bool loaderOk = loaderCompatible(mod.loader, profile.loaderHint, profile.lastVersionId);
        const bool versionOk = versionCompatible(profile.lastVersionId, mod.minecraftConstraint);

        if (loaderOk)
        {
            score += 40;
        }
        else
        {
            score -= 50;
        }

        if (versionOk)
        {
            score += 25;
        }
        else if (!mod.minecraftConstraint.trimmed().isEmpty())
        {
            score -= 30;
        }

        if (profile.name.contains("mod", Qt::CaseInsensitive))
        {
            score += 5;
        }

        if (score > bestScore)
        {
            bestScore = score;
            bestIndex = i;
        }
    }

    return bestIndex;
}

bool MinecraftManager::isProfileCompatible(const ProfileInfo &profile, const ModInfo &mod) const
{
    return loaderCompatible(mod.loader, profile.loaderHint, profile.lastVersionId)
           && versionCompatible(profile.lastVersionId, mod.minecraftConstraint);
}

ProfileInfo MinecraftManager::createCompatibleProfile(const ModInfo &mod, QString *errorOut)
{
    ProfileInfo created;

    const QString versionId = pickVersionIdForMod(mod);
    if (versionId.isEmpty())
    {
        if (errorOut)
        {
            *errorOut = QStringLiteral("No installed %1 loader version was found for this mod.")
                            .arg(loaderTypeToString(mod.loader));
        }
        return created;
    }

    QString targetFile;
    const QStringList files = profileFiles();
    for (const QString &candidate : files)
    {
        if (QFileInfo::exists(candidate))
        {
            targetFile = candidate;
            break;
        }
    }
    if (targetFile.isEmpty())
    {
        targetFile = files.value(0);
    }

    QJsonObject root;
    QFile file(targetFile);
    if (file.exists() && file.open(QIODevice::ReadOnly))
    {
        const QJsonDocument existing = QJsonDocument::fromJson(file.readAll());
        if (existing.isObject())
        {
            root = existing.object();
        }
        file.close();
    }

    QJsonObject profilesObj = root.value("profiles").toObject();
    const QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);

    const QString mcVersion = VersionMatcher::extractMinecraftVersion(versionId);
    const QString labelVersion = mcVersion.isEmpty() ? versionId : mcVersion;

    QJsonObject profileObj;
    profileObj.insert("name", QStringLiteral("DANDU %1 %2").arg(loaderTypeToString(mod.loader), labelVersion));
    profileObj.insert("type", "custom");
    profileObj.insert("created", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    profileObj.insert("lastUsed", QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    profileObj.insert("lastVersionId", versionId);
    profilesObj.insert(key, profileObj);
    root.insert("profiles", profilesObj);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (errorOut)
        {
            *errorOut = QStringLiteral("Could not write profile file: %1").arg(targetFile);
        }
        return created;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    created.key = key;
    created.name = profileObj.value("name").toString();
    created.lastVersionId = versionId;
    created.gameDir = "";
    created.sourceFile = targetFile;
    created.loaderHint = inferProfileLoader(versionId);
    m_profiles.append(created);

    std::sort(m_profiles.begin(), m_profiles.end(), [](const ProfileInfo &a, const ProfileInfo &b) {
        return QString::localeAwareCompare(a.name, b.name) < 0;
    });

    return created;
}

InstallResult MinecraftManager::installToProfile(const QString &jarPath, const ProfileInfo &profile) const
{
    InstallResult result;
    result.profileName = profile.name;

    const QString modsDirPath = modsDirectory(profile);
    QDir modsDir(modsDirPath);
    if (!modsDir.exists() && !modsDir.mkpath("."))
    {
        result.message = QStringLiteral("Could not create mods folder: %1").arg(modsDirPath);
        return result;
    }

    const QFileInfo srcInfo(jarPath);
    const QString destinationFile = modsDir.filePath(srcInfo.fileName());
    const QString srcAbs = srcInfo.absoluteFilePath();
    const QString dstAbs = QFileInfo(destinationFile).absoluteFilePath();
    if (QString::compare(srcAbs, dstAbs, Qt::CaseInsensitive) == 0)
    {
        result.ok = true;
        result.destination = destinationFile;
        result.message = QStringLiteral("Mod already exists in target mods folder.");
        return result;
    }

    if (QFile::exists(destinationFile) && !QFile::remove(destinationFile))
    {
        result.message = QStringLiteral("Could not replace existing mod file: %1").arg(destinationFile);
        return result;
    }

    if (!QFile::copy(jarPath, destinationFile))
    {
        result.message = QStringLiteral("Could not copy mod into %1").arg(destinationFile);
        return result;
    }

    result.ok = true;
    result.destination = destinationFile;
    result.message = QStringLiteral("Installed successfully to %1").arg(destinationFile);
    return result;
}

QString MinecraftManager::modsDirectory(const ProfileInfo &profile) const
{
    const QString baseDir = profile.gameDir.trimmed().isEmpty() ? m_mcRoot : profile.gameDir;
    return QDir(baseDir).filePath("mods");
}

LoaderType MinecraftManager::inferProfileLoader(const QString &versionId)
{
    const QString lower = versionId.toLower();
    if (lower.contains("quilt"))
    {
        return LoaderType::Quilt;
    }
    if (lower.contains("fabric"))
    {
        return LoaderType::Fabric;
    }
    if (lower.contains("neoforge") || lower.contains("forge"))
    {
        return LoaderType::NeoForge;
    }
    return LoaderType::Unknown;
}

bool MinecraftManager::loaderCompatible(LoaderType modLoader, LoaderType profileLoader, const QString &versionId)
{
    if (modLoader == LoaderType::Unknown)
    {
        return true;
    }

    if (modLoader == LoaderType::Fabric)
    {
        return profileLoader == LoaderType::Fabric || profileLoader == LoaderType::Quilt
               || versionId.contains("fabric", Qt::CaseInsensitive)
               || versionId.contains("quilt", Qt::CaseInsensitive);
    }

    if (modLoader == LoaderType::Quilt)
    {
        return profileLoader == LoaderType::Quilt || versionId.contains("quilt", Qt::CaseInsensitive);
    }

    if (modLoader == LoaderType::NeoForge)
    {
        return profileLoader == LoaderType::NeoForge || versionId.contains("neoforge", Qt::CaseInsensitive)
               || versionId.contains("forge", Qt::CaseInsensitive);
    }

    return false;
}

bool MinecraftManager::versionCompatible(const QString &profileVersionId, const QString &constraint)
{
    return VersionMatcher::profileMatchesConstraint(profileVersionId, constraint);
}

QStringList MinecraftManager::profileFiles() const
{
    return {
        QDir(m_mcRoot).filePath("launcher_profiles_microsoft_store.json"),
        QDir(m_mcRoot).filePath("launcher_profiles.json"),
    };
}

QStringList MinecraftManager::installedVersionIds() const
{
    QStringList ids;
    const QDir versionsDir(QDir(m_mcRoot).filePath("versions"));
    if (!versionsDir.exists())
    {
        return ids;
    }

    const QFileInfoList entries = versionsDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &entry : entries)
    {
        ids << entry.fileName();
    }
    return ids;
}

QString MinecraftManager::pickVersionIdForMod(const ModInfo &mod) const
{
    const QStringList ids = installedVersionIds();
    QString best;

    for (const QString &id : ids)
    {
        if (!loaderCompatible(mod.loader, inferProfileLoader(id), id))
        {
            continue;
        }

        if (!versionCompatible(id, mod.minecraftConstraint))
        {
            continue;
        }

        best = id;
        break;
    }

    if (!best.isEmpty())
    {
        return best;
    }

    for (const QString &id : ids)
    {
        if (loaderCompatible(mod.loader, inferProfileLoader(id), id))
        {
            return id;
        }
    }

    return {};
}
