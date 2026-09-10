#pragma once

#include <QByteArray>
#include <QString>
#include <QVector>

#include "LiveLineRecordFramer.h"

struct LiveLineSourceInitializationResult
{
    bool succeeded = true;

    QString errorMessage;

    qint64 existingPhysicalLineCount = 0;

    /*
     * False means the source already contained an
     * unterminated final physical line when live
     * following began.
     *
     * We expose this explicitly rather than silently
     * guessing how a later append should relate to
     * the statically imported final record.
     */
    bool endsAtLineBoundary = true;
};

struct LiveFramedLineBatch
{
    QVector<QByteArray> lines;

    qint64 firstPhysicalLineNumber = 1;

    quint64 sourceGeneration = 0;
};

class LiveLineSourceContext
{
public:
    LiveLineSourceInitializationResult
    initializeFromExistingFile(
        const QString &sourcePath
        );

    LiveFramedLineBatch appendBytes(
        QByteArray bytes
        );

    void beginSourceGeneration(
        quint64 sourceGeneration
        );

    qint64 nextPhysicalLineNumber() const;

    quint64 sourceGeneration() const;

    const LiveLineRecordFramer &framer() const;

private:
    qint64 m_nextPhysicalLineNumber = 1;

    quint64 m_sourceGeneration = 0;

    LiveLineRecordFramer m_framer;
};