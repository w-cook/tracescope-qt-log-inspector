#pragma once

#include <QDate>
#include <QString>

#include "../importing/DelimitedTextImporter.h"
#include "../importing/IisW3cImporter.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"
#include "../importing/IncrementalImportInitializationResult.h"

#include "LiveLineSourceContext.h"

enum class LiveLineImporterKind
{
    Unsupported,
    JsonLines,
    RegexText,
    KeyValue,
    Syslog,
    Csv,
    Tsv,
    IisW3c
};

class LiveLineImportAdapter
{
public:
    explicit LiveLineImportAdapter(
        ImportProfile profile
        );

    bool isSupported() const;

    LiveLineImporterKind kind() const;

    IncrementalImportInitializationResult
    initializeFromExistingFile(
        const QString &sourcePath
        );

    ImportResult importBatch(
        const LiveFramedLineBatch &batch,
        const QString &sourcePath
        );

    void beginSourceGeneration();

private:
    ImportProfile m_profile;

    LiveLineImporterKind m_kind =
        LiveLineImporterKind::Unsupported;

    DelimitedTextImportState
        m_delimitedState;

    IisW3cImportState
        m_iisState;

    /*
     * Preserve one RFC 3164 inference reference
     * throughout this adapter's lifetime.
     */
    QDate m_syslogLegacyReferenceDate;

    static LiveLineImporterKind
    kindForImporterId(
        const QString &importerId
        );
};