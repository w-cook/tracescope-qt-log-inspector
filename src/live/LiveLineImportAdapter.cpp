#include "LiveLineImportAdapter.h"

#include <utility>

#include "../importing/JsonLinesImporter.h"
#include "../importing/KeyValueTextImporter.h"
#include "../importing/RegexTextImporter.h"
#include "../importing/SyslogImporter.h"

namespace
{
QStringList decodeLines(
    const QVector<QByteArray> &lines
    )
{
    QStringList decoded;

    decoded.reserve(
        lines.size()
        );

    for (const QByteArray &line : lines) {
        decoded.append(
            QString::fromUtf8(
                line
                )
            );
    }

    return decoded;
}

IncrementalImportInitializationResult
unsupportedInitializationResult(
    const QString &importerId
    )
{
    IncrementalImportInitializationResult
        result;

    result.succeeded = false;

    result.errorMessage =
        QStringLiteral(
            "Importer '%1' does not support "
            "line-oriented live ingestion."
            )
            .arg(
                importerId
                );

    return result;
}
}

LiveLineImportAdapter::
    LiveLineImportAdapter(
        ImportProfile profile
        )
    : m_profile(
          std::move(profile)
          ),
    m_kind(
        kindForImporterId(
            m_profile.importerId
            )
        ),
    m_syslogLegacyReferenceDate(
        QDate::currentDate()
        )
{
}

bool LiveLineImportAdapter::
    isSupported() const
{
    return m_kind
           != LiveLineImporterKind::
           Unsupported;
}

LiveLineImporterKind
LiveLineImportAdapter::kind() const
{
    return m_kind;
}

IncrementalImportInitializationResult
    LiveLineImportAdapter::
    initializeFromExistingFile(
        const QString &sourcePath
        )
{
    switch (m_kind) {
    case LiveLineImporterKind::Csv: {
        DelimitedTextImporter importer(
            QStringLiteral("csv"),
            QStringLiteral("CSV"),
            QLatin1Char(','),
            m_profile
            );

        return importer
            .initializeIncrementalStateFromFile(
                sourcePath,
                m_delimitedState
                );
    }

    case LiveLineImporterKind::Tsv: {
        DelimitedTextImporter importer(
            QStringLiteral("tsv"),
            QStringLiteral("TSV"),
            QLatin1Char('\t'),
            m_profile
            );

        return importer
            .initializeIncrementalStateFromFile(
                sourcePath,
                m_delimitedState
                );
    }

    case LiveLineImporterKind::IisW3c: {
        IisW3cImporter importer(
            m_profile
            );

        return importer
            .initializeIncrementalStateFromFile(
                sourcePath,
                m_iisState
                );
    }

    case LiveLineImporterKind::JsonLines:
    case LiveLineImporterKind::RegexText:
    case LiveLineImporterKind::KeyValue:
    case LiveLineImporterKind::Syslog:
        /*
         * These formats have no parser state that
         * must be reconstructed from existing data.
         */
        return {};

    case LiveLineImporterKind::Unsupported:
        return unsupportedInitializationResult(
            m_profile.importerId
            );
    }

    return unsupportedInitializationResult(
        m_profile.importerId
        );
}

ImportResult
LiveLineImportAdapter::importBatch(
    const LiveFramedLineBatch &batch,
    const QString &sourcePath
    )
{
    const QStringList lines =
        decodeLines(
            batch.lines
            );

    switch (m_kind) {
    case LiveLineImporterKind::JsonLines: {
        JsonLinesImporter importer(
            m_profile
            );

        return importer.importLines(
            lines,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::RegexText: {
        RegexTextImporter importer(
            m_profile
            );

        return importer.importLines(
            lines,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::KeyValue: {
        KeyValueTextImporter importer(
            m_profile
            );

        return importer.importLines(
            lines,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::Syslog: {
        SyslogImporter importer(
            m_profile,
            m_syslogLegacyReferenceDate
            );

        return importer.importLines(
            lines,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::Csv: {
        DelimitedTextImporter importer(
            QStringLiteral("csv"),
            QStringLiteral("CSV"),
            QLatin1Char(','),
            m_profile
            );

        return importer.importIncrementalLines(
            lines,
            m_delimitedState,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::Tsv: {
        DelimitedTextImporter importer(
            QStringLiteral("tsv"),
            QStringLiteral("TSV"),
            QLatin1Char('\t'),
            m_profile
            );

        return importer.importIncrementalLines(
            lines,
            m_delimitedState,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::IisW3c: {
        IisW3cImporter importer(
            m_profile
            );

        return importer.importIncrementalLines(
            lines,
            m_iisState,
            sourcePath,
            batch.firstPhysicalLineNumber,
            batch.sourceGeneration
            );
    }

    case LiveLineImporterKind::Unsupported:
        return {};
    }

    return {};
}

void LiveLineImportAdapter::
    beginSourceGeneration()
{
    switch (m_kind) {
    case LiveLineImporterKind::Csv:
    case LiveLineImporterKind::Tsv:
        /*
         * A new delimited physical source must
         * establish its own header.
         */
        m_delimitedState.reset();
        break;

    case LiveLineImporterKind::IisW3c:
        /*
         * A new IIS physical source must establish
         * a fresh active #Fields directive.
         */
        m_iisState.reset();
        break;

    case LiveLineImporterKind::Unsupported:
    case LiveLineImporterKind::JsonLines:
    case LiveLineImporterKind::RegexText:
    case LiveLineImporterKind::KeyValue:
    case LiveLineImporterKind::Syslog:
        break;
    }
}

LiveLineImporterKind
LiveLineImportAdapter::kindForImporterId(
    const QString &importerId
    )
{
    const QString normalized =
        importerId.trimmed()
            .toLower();

    if (normalized
        == QStringLiteral("json-lines")) {
        return LiveLineImporterKind::
            JsonLines;
    }

    if (normalized
        == QStringLiteral("regex-text")) {
        return LiveLineImporterKind::
            RegexText;
    }

    if (normalized
        == QStringLiteral("key-value")) {
        return LiveLineImporterKind::
            KeyValue;
    }

    if (normalized
        == QStringLiteral("syslog")) {
        return LiveLineImporterKind::
            Syslog;
    }

    if (normalized
        == QStringLiteral("csv")) {
        return LiveLineImporterKind::Csv;
    }

    if (normalized
        == QStringLiteral("tsv")) {
        return LiveLineImporterKind::Tsv;
    }

    if (normalized
        == QStringLiteral("iis-w3c")) {
        return LiveLineImporterKind::
            IisW3c;
    }

    return LiveLineImporterKind::
        Unsupported;
}