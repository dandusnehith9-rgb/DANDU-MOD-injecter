#pragma once

#include "ModTypes.h"

class ModAnalyzer
{
public:
    static ModInfo analyze(const QString &jarPath);

private:
    static ModInfo parseFabric(const QString &jarPath, const QByteArray &jsonBytes);
    static ModInfo parseQuilt(const QString &jarPath, const QByteArray &jsonBytes);
    static ModInfo parseNeoForge(const QString &jarPath, const QByteArray &tomlBytes, const QByteArray &manifestBytes);
};
