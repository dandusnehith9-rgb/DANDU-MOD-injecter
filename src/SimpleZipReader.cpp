#include "SimpleZipReader.h"

#include <QFile>
#include <QtGlobal>

#include <zlib.h>

namespace
{
constexpr quint32 LOCAL_FILE_HEADER_SIG = 0x04034b50;
constexpr quint32 CENTRAL_FILE_HEADER_SIG = 0x02014b50;
constexpr quint32 END_OF_CENTRAL_DIR_SIG = 0x06054b50;
}

SimpleZipReader::SimpleZipReader(const QString &zipPath)
    : m_path(zipPath)
{
    QFile file(m_path);
    if (!file.open(QIODevice::ReadOnly))
    {
        m_error = QStringLiteral("Could not open archive: %1").arg(file.errorString());
        return;
    }

    m_bytes = file.readAll();
    if (m_bytes.isEmpty())
    {
        m_error = QStringLiteral("Archive is empty.");
        return;
    }

    parse();
}

bool SimpleZipReader::isOpen() const
{
    return m_error.isEmpty() && !m_entries.isEmpty();
}

QString SimpleZipReader::errorString() const
{
    return m_error;
}

bool SimpleZipReader::contains(const QString &entryName) const
{
    return m_entries.contains(entryName);
}

QStringList SimpleZipReader::entries() const
{
    return m_entries.keys();
}

QByteArray SimpleZipReader::readFile(const QString &entryName) const
{
    if (!m_entries.contains(entryName))
    {
        return {};
    }

    const Entry entry = m_entries.value(entryName);
    const int headerOffset = static_cast<int>(entry.localHeaderOffset);
    if (headerOffset < 0 || headerOffset + 30 > m_bytes.size())
    {
        return {};
    }

    const quint32 sig = readLe32(m_bytes, headerOffset);
    if (sig != LOCAL_FILE_HEADER_SIG)
    {
        return {};
    }

    const quint16 nameLen = readLe16(m_bytes, headerOffset + 26);
    const quint16 extraLen = readLe16(m_bytes, headerOffset + 28);
    const int dataOffset = headerOffset + 30 + static_cast<int>(nameLen) + static_cast<int>(extraLen);
    if (dataOffset < 0 || dataOffset + static_cast<int>(entry.compressedSize) > m_bytes.size())
    {
        return {};
    }

    const QByteArray compressed = m_bytes.mid(dataOffset, static_cast<int>(entry.compressedSize));
    if (entry.compressionMethod == 0)
    {
        return compressed;
    }

    if (entry.compressionMethod == 8)
    {
        bool ok = false;
        QByteArray out = inflateRaw(compressed, entry.uncompressedSize, &ok);
        return ok ? out : QByteArray();
    }

    return {};
}

bool SimpleZipReader::parse()
{
    const int minimumEocdSize = 22;
    if (m_bytes.size() < minimumEocdSize)
    {
        m_error = QStringLiteral("Archive is too small.");
        return false;
    }

    const int maxComment = 0xFFFF;
    const int searchStart = qMax(0, m_bytes.size() - minimumEocdSize - maxComment);
    int eocdOffset = -1;

    for (int i = m_bytes.size() - minimumEocdSize; i >= searchStart; --i)
    {
        if (readLe32(m_bytes, i) == END_OF_CENTRAL_DIR_SIG)
        {
            eocdOffset = i;
            break;
        }
    }

    if (eocdOffset < 0)
    {
        m_error = QStringLiteral("Could not locate central directory.");
        return false;
    }

    const quint16 totalEntries = readLe16(m_bytes, eocdOffset + 10);
    const quint32 centralSize = readLe32(m_bytes, eocdOffset + 12);
    const quint32 centralOffset = readLe32(m_bytes, eocdOffset + 16);

    if (centralOffset + centralSize > static_cast<quint32>(m_bytes.size()))
    {
        m_error = QStringLiteral("Invalid central directory range.");
        return false;
    }

    int cursor = static_cast<int>(centralOffset);
    int parsedEntries = 0;

    while (cursor + 46 <= m_bytes.size())
    {
        if (readLe32(m_bytes, cursor) != CENTRAL_FILE_HEADER_SIG)
        {
            break;
        }

        Entry entry;
        entry.compressionMethod = readLe16(m_bytes, cursor + 10);
        entry.compressedSize = readLe32(m_bytes, cursor + 20);
        entry.uncompressedSize = readLe32(m_bytes, cursor + 24);
        const quint16 nameLen = readLe16(m_bytes, cursor + 28);
        const quint16 extraLen = readLe16(m_bytes, cursor + 30);
        const quint16 commentLen = readLe16(m_bytes, cursor + 32);
        entry.localHeaderOffset = readLe32(m_bytes, cursor + 42);

        const int nameOffset = cursor + 46;
        if (nameOffset + nameLen > m_bytes.size())
        {
            m_error = QStringLiteral("Invalid file name entry in archive.");
            return false;
        }

        const QByteArray nameBytes = m_bytes.mid(nameOffset, nameLen);
        entry.name = QString::fromUtf8(nameBytes);
        m_entries.insert(entry.name, entry);

        cursor = nameOffset + static_cast<int>(nameLen) + static_cast<int>(extraLen) + static_cast<int>(commentLen);
        ++parsedEntries;
    }

    if (parsedEntries == 0 || (totalEntries != 0 && parsedEntries < totalEntries))
    {
        if (m_error.isEmpty())
        {
            m_error = QStringLiteral("Failed to parse archive entries.");
        }
        return false;
    }

    return true;
}

quint16 SimpleZipReader::readLe16(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 2 > data.size())
    {
        return 0;
    }

    const uchar *ptr = reinterpret_cast<const uchar *>(data.constData() + offset);
    return static_cast<quint16>(ptr[0] | (ptr[1] << 8));
}

quint32 SimpleZipReader::readLe32(const QByteArray &data, int offset)
{
    if (offset < 0 || offset + 4 > data.size())
    {
        return 0;
    }

    const uchar *ptr = reinterpret_cast<const uchar *>(data.constData() + offset);
    return static_cast<quint32>(ptr[0] | (ptr[1] << 8) | (ptr[2] << 16) | (ptr[3] << 24));
}

QByteArray SimpleZipReader::inflateRaw(const QByteArray &compressed, quint32 uncompressedSizeHint, bool *okOut)
{
    if (okOut)
    {
        *okOut = false;
    }

    if (compressed.isEmpty())
    {
        return {};
    }

    z_stream stream{};
    stream.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.constData()));
    stream.avail_in = static_cast<uInt>(compressed.size());

    if (inflateInit2(&stream, -MAX_WBITS) != Z_OK)
    {
        return {};
    }

    QByteArray out;
    const int initialSize = (uncompressedSizeHint > 0 && uncompressedSizeHint < (32u * 1024u * 1024u))
                                ? static_cast<int>(uncompressedSizeHint)
                                : 64 * 1024;
    out.resize(initialSize);

    int status = Z_OK;
    while (status == Z_OK)
    {
        if (stream.total_out >= static_cast<uLong>(out.size()))
        {
            out.resize(out.size() + 64 * 1024);
        }

        stream.next_out = reinterpret_cast<Bytef *>(out.data() + stream.total_out);
        stream.avail_out = static_cast<uInt>(out.size() - stream.total_out);
        status = inflate(&stream, Z_NO_FLUSH);
    }

    const bool done = (status == Z_STREAM_END);
    const int outputSize = static_cast<int>(stream.total_out);
    inflateEnd(&stream);

    if (!done)
    {
        return {};
    }

    out.resize(outputSize);
    if (okOut)
    {
        *okOut = true;
    }
    return out;
}
