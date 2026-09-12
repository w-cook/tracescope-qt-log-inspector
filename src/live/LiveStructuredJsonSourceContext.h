#pragma once

#include <QByteArray>
#include <QString>
#include <QtTypes>

#include "../importing/StructuredJsonRecordStreamFramer.h"

#include "LiveStructuredJsonRecordBatch.h"

struct LiveStructuredJsonSourceInitializationResult
{
    bool succeeded = true;

    QString errorMessage;

    LiveStructuredJsonRecordBatch
        existingRecordBatch;
};

struct LiveStructuredJsonSourceAppendResult
{
    bool succeeded = true;

    QString errorMessage;

    LiveStructuredJsonRecordBatch batch;
};

class LiveStructuredJsonSourceContext
{
public:
    explicit LiveStructuredJsonSourceContext(
        QString recordPath = {}
        );

    LiveStructuredJsonSourceInitializationResult
    initializeFromExistingFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        quint64 sourceGeneration
        );

    LiveStructuredJsonSourceAppendResult
    appendBytes(
        QByteArray bytes
        );

    void beginSourceGeneration(
        quint64 sourceGeneration
        );

    qint64 nextRecordNumber() const;

    quint64 sourceGeneration() const;

private:
    qint64 m_nextRecordNumber = 1;

    quint64 m_sourceGeneration = 0;

    StructuredJsonRecordStreamFramer
        m_framer;
};