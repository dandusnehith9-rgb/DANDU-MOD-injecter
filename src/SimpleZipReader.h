#pragma once

#include <QByteArray>
#include <QHash>
#include <QString>
#include <QStringList>

class SimpleZipReader
{
public:
    explicit SimpleZipReader(const QString &zipPath);

    bool isOpen() const;
    QString errorString() const;
    bool contains(const QString &entryName) const;
    QStringList entries() const;
    QByteArray readFile(const QString &entryName) const;

private:
    struct Entry
    {
        quint16 compressionMethod = 0;
        quint32 compressedSize = 0;
        quint32 uncompressedSize = 0;
        quint32 localHeaderOffset = 0;
        QString name;
    };

    bool parse();
    static quint16 readLe16(const QByteArray &data, int offset);
    static quint32 readLe32(const QByteArray &data, int offset);
    static QByteArray inflateRaw(const QByteArray &compressed, quint32 uncompressedSizeHint, bool *okOut);

    QString m_path;
    QString m_error;
    QByteArray m_bytes;
    QHash<QString, Entry> m_entries;
};
