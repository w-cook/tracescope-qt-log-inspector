#include "LiveStructuredJsonImportAdapter.h"

#include <optional>
#include <utility>

#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "../importing/ImportDiagnostic.h"
#include "../importing/JsonObjectRecordMapper.h"

namespace
{
RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber,
    quint64 sourceGeneration
    )
{
    RecordSourceMetadata source;

    source.sourcePath =
        sourcePath;

    source.recordNumber =
        recordNumber;

    source.sourceGeneration =
        sourceGeneration;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(
                sourcePath
                ).fileName();
    }

    return source;
}

void appendDiagnostic(
    ImportResult &result,
    const QString &code,
    const QString &message,
    ImportDiagnosticSeverity severity,
    const std::optional<RecordSourceMetadata>
        &source = std::nullopt
    )
{
    ImportDiagnostic diagnostic;

    diagnostic.code =
        code;

    diagnostic.message =
        message;

    diagnostic.severity =
        severity;

    diagnostic.source =
        source;

    result.diagnostics.append(
        std::move(diagnostic)
        );
}

QString compactJson(
    const QJsonObject &object
    )
{
    return QString::fromUtf8(
        QJsonDocument(
            object
            ).toJson(
                QJsonDocument::Compact
                )
        );
}
}

LiveStructuredJsonImportAdapter::
    LiveStructuredJsonImportAdapter(
        ImportProfile profile
        )
    : m_profile(
          std::move(profile)
          )
{
}

bool LiveStructuredJsonImportAdapter::
    isSupported() const
{
    return m_profile
               .importerId
               .trimmed()
               .compare(
                   QStringLiteral(
                       "structured-json"
                       ),
                   Qt::CaseInsensitive
                   )
           == 0;
}

ImportResult
    LiveStructuredJsonImportAdapter::
    importBatch(
        const LiveStructuredJsonRecordBatch &batch,
        const QString &sourcePath
        ) const
{
    ImportResult result;

    if (!isSupported()) {
        return result;
    }

    for (qsizetype index = 0;
         index < batch.records.size();
         ++index) {
        ++result.processedRecordCount;

        const qint64 recordNumber =
            batch.firstRecordNumber
            + index;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                recordNumber,
                batch.sourceGeneration
                );

        QJsonParseError parseError;

        const QJsonDocument document =
            QJsonDocument::fromJson(
                batch.records.at(index),
                &parseError
                );

        if (parseError.error
            != QJsonParseError::NoError) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "MALFORMED_STRUCTURED_JSON_LIVE_RECORD"
                    ),
                QStringLiteral(
                    "The live structured JSON "
                    "record could not be parsed: %1"
                    )
                    .arg(
                        parseError.errorString()
                        ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        if (!document.isObject()) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "STRUCTURED_JSON_RECORD_NOT_OBJECT"
                    ),
                QStringLiteral(
                    "The live structured JSON "
                    "record is not an object."
                    ),
                ImportDiagnosticSeverity::Error,
                source
                );

            continue;
        }

        const QJsonObject object =
            document.object();

        /*
         * Match StructuredJsonImporter's existing
         * raw-source behavior rather than retaining
         * incidental source whitespace.
         *
         * This also keeps stable identity semantics
         * consistent between static and live
         * structured JSON imports.
         */
        const QString rawSource =
            compactJson(
                object
                );

        result.records.append(
            JsonObjectRecordMapper::mapRecord(
                object,
                rawSource,
                source,
                m_profile,
                result
                )
            );
    }

    return result;
}