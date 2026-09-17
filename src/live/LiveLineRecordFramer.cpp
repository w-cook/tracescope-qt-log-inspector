#include "LiveLineRecordFramer.h"

#include <utility>

const QByteArray &
LiveLineRecordFramer::pendingBytes() const
{
    return m_pendingBytes;
}

QVector<QByteArray>
LiveLineRecordFramer::appendBytes(
    QByteArray bytes
    )
{
    QVector<QByteArray> lines;

    if (bytes.isEmpty()) {
        return lines;
    }

    QByteArray buffer =
        std::move(m_pendingBytes);

    buffer.append(
        bytes
        );

    qsizetype lineStart = 0;

    while (true) {
        const qsizetype newlineIndex =
            buffer.indexOf(
                '\n',
                lineStart
                );

        if (newlineIndex < 0) {
            break;
        }

        QByteArray line =
            buffer.mid(
                lineStart,
                newlineIndex - lineStart
                );

        /*
         * Match the existing file importers'
         * treatment of CRLF while keeping framing
         * byte-oriented until a complete physical
         * line exists.
         */
        if (line.endsWith('\r')) {
            line.chop(1);
        }

        lines.append(
            std::move(line)
            );

        lineStart =
            newlineIndex + 1;
    }

    if (lineStart < buffer.size()) {
        m_pendingBytes =
            buffer.mid(
                lineStart
                );
    } else {
        m_pendingBytes.clear();
    }

    return lines;
}

void LiveLineRecordFramer::reset()
{
    m_pendingBytes.clear();
}