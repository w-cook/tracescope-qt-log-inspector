#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

struct StructuredJsonRecordFrameResult
{
    bool succeeded = true;
    QString errorMessage;

    QVector<QByteArray> records;
};

class StructuredJsonRecordStreamFramer
{
public:
    explicit StructuredJsonRecordStreamFramer(
        QString recordPath = {}
        );

    StructuredJsonRecordFrameResult appendBytes(
        QByteArray bytes
        );

    void reset();

    bool recordArrayLocated() const;
    bool recordArrayClosed() const;

private:
    StructuredJsonRecordFrameResult
    appendRecordArrayBytes(
        const QByteArray &bytes
        );

    QString m_recordPath;

    QByteArray m_prefixBytes;
    QByteArray m_recordBytes;

    bool m_recordArrayLocated = false;
    bool m_recordArrayClosed = false;
    bool m_insideRecord = false;

    int m_recordNestingDepth = 0;

    bool m_insideString = false;
    bool m_escapeNextCharacter = false;
};