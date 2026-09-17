#pragma once

#include <QByteArray>
#include <QVector>

class LiveLineRecordFramer
{
public:
    const QByteArray &pendingBytes() const;

    QVector<QByteArray> appendBytes(
        QByteArray bytes
        );

    void reset();

private:
    QByteArray m_pendingBytes;
};