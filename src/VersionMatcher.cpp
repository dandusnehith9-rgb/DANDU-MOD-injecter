#include "VersionMatcher.h"

#include <QRegularExpression>
#include <QStringList>
#include <QtGlobal>

namespace
{
QString normalizeConstraint(QString value)
{
    value = value.trimmed();
    if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2)
    {
        value = value.mid(1, value.size() - 2);
    }
    return value.trimmed();
}

QList<int> parseVersionParts(const QString &version)
{
    QList<int> parts;
    const QStringList tokens = version.split('.', Qt::SkipEmptyParts);
    for (const QString &token : tokens)
    {
        bool ok = false;
        const int part = token.toInt(&ok);
        if (!ok)
        {
            const QRegularExpression digitsOnlyRegex(R"((\d+))");
            const auto match = digitsOnlyRegex.match(token);
            if (match.hasMatch())
            {
                parts.append(match.captured(1).toInt());
            }
            else
            {
                parts.append(0);
            }
        }
        else
        {
            parts.append(part);
        }
    }
    return parts;
}
}

QString VersionMatcher::extractMinecraftVersion(const QString &text)
{
    const QRegularExpression regex(R"((\d+\.\d+(?:\.\d+)?))");
    const auto match = regex.match(text);
    return match.hasMatch() ? match.captured(1) : QString();
}

bool VersionMatcher::profileMatchesConstraint(const QString &profileVersionId, const QString &constraint)
{
    const QString version = extractMinecraftVersion(profileVersionId);
    if (version.isEmpty())
    {
        return constraint.trimmed().isEmpty();
    }
    return versionMatchesConstraint(version, constraint);
}

bool VersionMatcher::versionMatchesConstraint(const QString &mcVersion, const QString &constraint)
{
    const QString normalized = normalizeConstraint(constraint);
    if (normalized.isEmpty() || normalized == "*" || normalized == "any")
    {
        return true;
    }

    const QStringList orParts = normalized.split("||", Qt::SkipEmptyParts);
    if (orParts.size() > 1)
    {
        for (const QString &part : orParts)
        {
            if (matchSingleConstraint(mcVersion, part.trimmed()))
            {
                return true;
            }
        }
        return false;
    }

    return matchSingleConstraint(mcVersion, normalized);
}

int VersionMatcher::compareVersions(const QString &left, const QString &right)
{
    const QList<int> a = parseVersionParts(left);
    const QList<int> b = parseVersionParts(right);
    const int maxSize = qMax(a.size(), b.size());

    for (int i = 0; i < maxSize; ++i)
    {
        const int av = (i < a.size()) ? a[i] : 0;
        const int bv = (i < b.size()) ? b[i] : 0;
        if (av < bv)
        {
            return -1;
        }
        if (av > bv)
        {
            return 1;
        }
    }

    return 0;
}

bool VersionMatcher::matchSingleConstraint(const QString &version, const QString &constraint)
{
    const QString c = normalizeConstraint(constraint);
    if (c.isEmpty() || c == "*")
    {
        return true;
    }

    if ((c.startsWith('[') || c.startsWith('(')) && (c.endsWith(']') || c.endsWith(')')) && c.contains(','))
    {
        return matchRangeConstraint(version, c);
    }

    if (c.contains(' '))
    {
        const QStringList andParts = c.split(' ', Qt::SkipEmptyParts);
        bool allMatch = true;
        for (const QString &part : andParts)
        {
            if (!matchSingleConstraint(version, part))
            {
                allMatch = false;
                break;
            }
        }
        return allMatch;
    }

    if (c.endsWith(".x", Qt::CaseInsensitive) || c.endsWith(".*"))
    {
        QString prefix = c;
        prefix.chop(2);
        return version.startsWith(prefix + ".");
    }

    if (c.startsWith(">") || c.startsWith("<") || c.startsWith("="))
    {
        return matchComparatorConstraint(version, c);
    }

    return compareVersions(version, c) == 0 || version.startsWith(c + ".");
}

bool VersionMatcher::matchRangeConstraint(const QString &version, const QString &constraint)
{
    const bool includeMin = constraint.startsWith('[');
    const bool includeMax = constraint.endsWith(']');
    const QString inside = constraint.mid(1, constraint.size() - 2);
    const QStringList bounds = inside.split(',', Qt::KeepEmptyParts);
    if (bounds.size() != 2)
    {
        return false;
    }

    const QString min = bounds[0].trimmed();
    const QString max = bounds[1].trimmed();

    if (!min.isEmpty())
    {
        const int cmp = compareVersions(version, min);
        if (cmp < 0 || (!includeMin && cmp == 0))
        {
            return false;
        }
    }

    if (!max.isEmpty())
    {
        const int cmp = compareVersions(version, max);
        if (cmp > 0 || (!includeMax && cmp == 0))
        {
            return false;
        }
    }

    return true;
}

bool VersionMatcher::matchComparatorConstraint(const QString &version, const QString &constraint)
{
    QString op;
    QString target;

    if (constraint.startsWith(">="))
    {
        op = ">=";
        target = constraint.mid(2).trimmed();
    }
    else if (constraint.startsWith("<="))
    {
        op = "<=";
        target = constraint.mid(2).trimmed();
    }
    else if (constraint.startsWith(">"))
    {
        op = ">";
        target = constraint.mid(1).trimmed();
    }
    else if (constraint.startsWith("<"))
    {
        op = "<";
        target = constraint.mid(1).trimmed();
    }
    else if (constraint.startsWith("="))
    {
        op = "=";
        target = constraint.mid(1).trimmed();
    }
    else
    {
        return false;
    }

    const int cmp = compareVersions(version, target);
    if (op == ">=")
    {
        return cmp >= 0;
    }
    if (op == "<=")
    {
        return cmp <= 0;
    }
    if (op == ">")
    {
        return cmp > 0;
    }
    if (op == "<")
    {
        return cmp < 0;
    }
    return cmp == 0;
}
