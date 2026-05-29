#pragma once

#include <QString>

class VersionMatcher
{
public:
    static QString extractMinecraftVersion(const QString &text);
    static bool profileMatchesConstraint(const QString &profileVersionId, const QString &constraint);
    static bool versionMatchesConstraint(const QString &mcVersion, const QString &constraint);

private:
    static int compareVersions(const QString &left, const QString &right);
    static bool matchSingleConstraint(const QString &version, const QString &constraint);
    static bool matchRangeConstraint(const QString &version, const QString &constraint);
    static bool matchComparatorConstraint(const QString &version, const QString &constraint);
};
