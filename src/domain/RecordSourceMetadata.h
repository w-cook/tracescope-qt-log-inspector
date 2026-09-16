#pragma once

#include <QString>
#include <QtTypes>

struct RecordSourceMetadata
{
    /*
     * Physical source location associated with this
     * record when it was observed.
     */
    QString sourcePath;

    /*
     * Stable logical source identity.
     *
     * Normally this is the same as sourcePath.
     * After a verified source relocation, sourcePath
     * may change while logicalSourceKey remains stable.
     *
     * Empty means "fall back to sourcePath" for
     * backward compatibility.
     */
    QString logicalSourceKey;

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