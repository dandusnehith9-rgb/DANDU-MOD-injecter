#include "ModAnalyzer.h"

#include "SimpleZipReader.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStringList>

namespace
{
QString jsonValueToConstraint(const QJsonValue &value)
{
    if (value.isString())
    {
        return value.toString();
    }

    if (value.isArray())
    {
        QStringList values;
        const QJsonArray arr = value.toArray();
        for (const QJsonValue &v : arr)
        {
            if (v.isString())
            {
                values.append(v.toString());
            }
            else if (v.isObject())
            {
                const QJsonObject obj = v.toObject();
                if (obj.contains("version"))
                {
                    values.append(obj.value("version").toString());
                }
                else if (obj.contains("versions") && obj.value("versions").isArray())
                {
                    const QJsonArray versions = obj.value("versions").toArray();
                    for (const QJsonValue &vv : versions)
                    {
                        if (vv.isString())
                        {
                            values.append(vv.toString());
                        }
                    }
                }
            }
        }
        return values.join(" || ");
    }

    if (value.isObject())
    {
        const QJsonObject obj = value.toObject();
        if (obj.contains("version"))
        {
            return obj.value("version").toString();
        }
        if (obj.contains("versions") && obj.value("versions").isArray())
        {
            QStringList versionsOut;
            const QJsonArray versions = obj.value("versions").toArray();
            for (const QJsonValue &v : versions)
            {
                if (v.isString())
                {
                    versionsOut.append(v.toString());
                }
            }
            return versionsOut.join(" || ");
        }
    }

    return {};
}

QString readManifestValue(const QByteArray &manifestBytes, const QString &key)
{
    if (manifestBytes.isEmpty())
    {
        return {};
    }

    const QString targetPrefix = key + ":";
    const QStringList lines = QString::fromUtf8(manifestBytes).split('\n');
    for (const QString &rawLine : lines)
    {
        const QString line = rawLine.trimmed();
        if (line.startsWith(targetPrefix, Qt::CaseInsensitive))
        {
            return line.mid(targetPrefix.size()).trimmed();
        }
    }

    return {};
}
}

ModInfo ModAnalyzer::analyze(const QString &jarPath)
{
    ModInfo info;
    info.filePath = jarPath;

    if (!jarPath.endsWith(".jar", Qt::CaseInsensitive))
    {
        info.warnings << QStringLiteral("File is not a .jar mod archive.");
        return info;
    }

    SimpleZipReader zip(jarPath);
    if (!zip.isOpen())
    {
        info.warnings << QStringLiteral("Could not read JAR: %1").arg(zip.errorString());
        return info;
    }

    const bool hasQuilt = zip.contains(QStringLiteral("quilt.mod.json"));
    const bool hasFabric = zip.contains(QStringLiteral("fabric.mod.json"));
    const bool hasNeoForge = zip.contains(QStringLiteral("META-INF/neoforge.mods.toml"));
    const bool hasModsToml = zip.contains(QStringLiteral("META-INF/mods.toml"));

    if (hasQuilt)
    {
        return parseQuilt(jarPath, zip.readFile(QStringLiteral("quilt.mod.json")));
    }

    if (hasFabric)
    {
        return parseFabric(jarPath, zip.readFile(QStringLiteral("fabric.mod.json")));
    }

    if (hasNeoForge || hasModsToml)
    {
        const QString tomlPath = hasNeoForge ? QStringLiteral("META-INF/neoforge.mods.toml") : QStringLiteral("META-INF/mods.toml");
        return parseNeoForge(jarPath, zip.readFile(tomlPath), zip.readFile(QStringLiteral("META-INF/MANIFEST.MF")));
    }

    info.warnings << QStringLiteral("Unknown mod type: missing quilt.mod.json, fabric.mod.json, or mods.toml.");
    return info;
}

ModInfo ModAnalyzer::parseFabric(const QString &jarPath, const QByteArray &jsonBytes)
{
    ModInfo info;
    info.filePath = jarPath;
    info.loader = LoaderType::Fabric;

    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject())
    {
        info.warnings << QStringLiteral("Invalid fabric.mod.json.");
        return info;
    }

    const QJsonObject root = doc.object();
    info.modId = root.value("id").toString();
    info.displayName = root.value("name").toString(info.modId);
    info.modVersion = root.value("version").toString();

    if (root.contains("depends") && root.value("depends").isObject())
    {
        const QJsonObject depends = root.value("depends").toObject();
        info.minecraftConstraint = jsonValueToConstraint(depends.value("minecraft"));
    }

    info.valid = !info.modId.isEmpty();
    if (!info.valid)
    {
        info.warnings << QStringLiteral("Fabric mod metadata has no id.");
    }

    return info;
}

ModInfo ModAnalyzer::parseQuilt(const QString &jarPath, const QByteArray &jsonBytes)
{
    ModInfo info;
    info.filePath = jarPath;
    info.loader = LoaderType::Quilt;

    const QJsonDocument doc = QJsonDocument::fromJson(jsonBytes);
    if (!doc.isObject())
    {
        info.warnings << QStringLiteral("Invalid quilt.mod.json.");
        return info;
    }

    const QJsonObject root = doc.object();
    const QJsonObject qLoader = root.value("quilt_loader").toObject();

    info.modId = qLoader.value("id").toString(root.value("id").toString());
    info.displayName = qLoader.value("metadata").toObject().value("name").toString(info.modId);
    if (info.displayName.isEmpty())
    {
        info.displayName = qLoader.value("name").toString(info.modId);
    }
    info.modVersion = qLoader.value("version").toString(root.value("version").toString());

    if (qLoader.contains("depends"))
    {
        const QJsonValue dependsValue = qLoader.value("depends");
        if (dependsValue.isObject())
        {
            const QJsonObject depends = dependsValue.toObject();
            info.minecraftConstraint = jsonValueToConstraint(depends.value("minecraft"));
        }
        else if (dependsValue.isArray())
        {
            const QJsonArray dependsArray = dependsValue.toArray();
            for (const QJsonValue &entry : dependsArray)
            {
                if (!entry.isObject())
                {
                    continue;
                }
                const QJsonObject obj = entry.toObject();
                if (obj.value("id").toString() == "minecraft")
                {
                    if (obj.contains("versions"))
                    {
                        info.minecraftConstraint = jsonValueToConstraint(obj.value("versions"));
                    }
                    else
                    {
                        info.minecraftConstraint = jsonValueToConstraint(obj.value("version"));
                    }
                    break;
                }
            }
        }
    }

    info.valid = !info.modId.isEmpty();
    if (!info.valid)
    {
        info.warnings << QStringLiteral("Quilt mod metadata has no id.");
    }

    return info;
}

ModInfo ModAnalyzer::parseNeoForge(const QString &jarPath, const QByteArray &tomlBytes, const QByteArray &manifestBytes)
{
    ModInfo info;
    info.filePath = jarPath;
    info.loader = LoaderType::NeoForge;

    const QString text = QString::fromUtf8(tomlBytes);
    if (text.trimmed().isEmpty())
    {
        info.warnings << QStringLiteral("mods.toml metadata is empty.");
        return info;
    }

    const QRegularExpression modIdRegex(R"re(\[\[mods\]\][\s\S]*?modId\s*=\s*"([^"]+)")re");
    const QRegularExpression nameRegex(R"re(\[\[mods\]\][\s\S]*?displayName\s*=\s*"([^"]+)")re");
    const QRegularExpression versionRegex(R"re(\[\[mods\]\][\s\S]*?version\s*=\s*"([^"]+)")re");

    auto captureFirst = [&](const QRegularExpression &regex) -> QString {
        const auto match = regex.match(text);
        return match.hasMatch() ? match.captured(1).trimmed() : QString();
    };

    info.modId = captureFirst(modIdRegex);
    info.displayName = captureFirst(nameRegex);
    info.modVersion = captureFirst(versionRegex);

    if (info.displayName.isEmpty())
    {
        info.displayName = info.modId;
    }

    if (info.modVersion == "${file.jarVersion}")
    {
        const QString implVersion = readManifestValue(manifestBytes, QStringLiteral("Implementation-Version"));
        if (!implVersion.isEmpty())
        {
            info.modVersion = implVersion;
        }
    }

    bool inDependencyBlock = false;
    bool isMinecraftDependency = false;

    const QStringList lines = text.split('\n');
    for (const QString &rawLine : lines)
    {
        QString line = rawLine.trimmed();
        const int commentIndex = line.indexOf('#');
        if (commentIndex >= 0)
        {
            line = line.left(commentIndex).trimmed();
        }
        if (line.isEmpty())
        {
            continue;
        }

        if (line.startsWith("[[dependencies.", Qt::CaseInsensitive))
        {
            inDependencyBlock = true;
            isMinecraftDependency = false;
            continue;
        }

        if (!inDependencyBlock)
        {
            continue;
        }

        if (line.startsWith("modId", Qt::CaseInsensitive))
        {
            const int idx = line.indexOf('=');
            if (idx >= 0)
            {
                const QString value = line.mid(idx + 1).trimmed().remove('"');
                isMinecraftDependency = (value.compare(QStringLiteral("minecraft"), Qt::CaseInsensitive) == 0);
            }
        }

        if (isMinecraftDependency && line.startsWith("versionRange", Qt::CaseInsensitive))
        {
            const int idx = line.indexOf('=');
            if (idx >= 0)
            {
                info.minecraftConstraint = line.mid(idx + 1).trimmed().remove('"');
                break;
            }
        }
    }

    info.valid = !info.modId.isEmpty();
    if (!info.valid)
    {
        info.warnings << QStringLiteral("NeoForge metadata has no modId in [[mods]] block.");
    }

    return info;
}
