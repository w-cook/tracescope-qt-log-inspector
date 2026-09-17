#pragma once

#include <QtTypes>
#include <QVector>

#include "../importing/StructuredXmlRecordStreamParser.h"

struct LiveStructuredXmlRecordBatch
{
    QVector<StructuredXmlRecord> records;

    qint64 firstRecordNumber = 1;

    quint64 sourceGeneration = 0;
};