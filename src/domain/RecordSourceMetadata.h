#pragma once

#include <QString>
#include <QtTypes>

struct RecordSourceMetadata
{
    QString sourcePath;
    QString sourceName;
    qint64 recordNumber = 0;

    /*
     * Zero represents the original/static source
     * generation. Live following increments this
     * when the observed source is replaced or reset
     * and later produces a new logical record stream.
     */
    quint64 sourceGeneration = 0;
};