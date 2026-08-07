#pragma once

#include <QString>
#include <QtTypes>

struct RecordSourceMetadata
{
    QString sourcePath;
    QString sourceName;
    qint64 recordNumber = 0;
};