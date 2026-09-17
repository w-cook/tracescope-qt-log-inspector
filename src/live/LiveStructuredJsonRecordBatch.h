#pragma once

#include <QByteArray>
#include <QVector>
#include <QtTypes>

struct LiveStructuredJsonRecordBatch
{
    QVector<QByteArray> records;

    qint64 firstRecordNumber = 1;
    quint64 sourceGeneration = 0;
};