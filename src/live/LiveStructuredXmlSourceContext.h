#pragma once

#include <QByteArray>
#include <QString>
#include <QtTypes>

#include "../importing/StructuredXmlRecordStreamParser.h"

#include "LiveStructuredXmlRecordBatch.h"

struct LiveStructuredXmlSourceInitializationResult
{
    bool succeeded = true;

    QString errorMessage;

    LiveStructuredXmlRecordBatch
        existingRecordBatch;
};

struct LiveStructuredXmlSourceAppendResult
{
    bool succeeded = true;

    QString errorMessage;

    LiveStructuredXmlRecordBatch batch;
};

class LiveStructuredXmlSourceContext
{
public:
    explicit LiveStructuredXmlSourceContext(
        QString recordPath = {}
        );

    LiveStructuredXmlSourceInitializationResult
    initializeFromExistingFile(
        const QString &sourcePath,
        qint64 existingByteCount,
        quint64 sourceGeneration
        );

    LiveStructuredXmlSourceAppendResult
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

    StructuredXmlRecordStreamParser
        m_parser;
};