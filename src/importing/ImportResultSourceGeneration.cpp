#include "ImportResultSourceGeneration.h"

#include <QDir>
#include <QFileInfo>

#include "../domain/RecordIdentity.h"

namespace
{
QString normalizedSourcePath(
    const QString &sourcePath
    )
{
    return QDir::cleanPath(
        QFileInfo(sourcePath)
            .absoluteFilePath()
        );
}

bool sourcePathsMatch(
    const QString &left,
    const QString &right
    )
{
#ifdef Q_OS_WIN
    constexpr Qt::CaseSensitivity
        caseSensitivity =
        Qt::CaseInsensitive;
#else
    constexpr Qt::CaseSensitivity
        caseSensitivity =
        Qt::CaseSensitive;
#endif

    return QString::compare(
               normalizedSourcePath(left),
               normalizedSourcePath(right),
               caseSensitivity
               ) == 0;
}

void rebaseRecordGeneration(
    InvestigationRecord &record,
    quint64 sourceGeneration
    )
{
    record.source.sourceGeneration =
        sourceGeneration;

    record.recordId =
        createStableRecordIdentity(
            record.source,
            record.rawSource
            );
}

void rebaseDiagnosticGeneration(
    ImportDiagnostic &diagnostic,
    quint64 sourceGeneration
    )
{
    if (!diagnostic.source.has_value()) {
        return;
    }

    diagnostic
        .source
        ->sourceGeneration =
        sourceGeneration;
}
}

void rebaseImportResultSourceGeneration(
    ImportResult &result,
    quint64 sourceGeneration
    )
{
    for (InvestigationRecord &record
         : result.records) {
        rebaseRecordGeneration(
            record,
            sourceGeneration
            );
    }

    for (ImportDiagnostic &diagnostic
         : result.diagnostics) {
        rebaseDiagnosticGeneration(
            diagnostic,
            sourceGeneration
            );
    }
}

void rebaseImportResultSourceGenerationForPath(
    ImportResult &result,
    const QString &sourcePath,
    quint64 sourceGeneration
    )
{
    for (InvestigationRecord &record
         : result.records) {
        if (!sourcePathsMatch(
                record.source.sourcePath,
                sourcePath
                )) {
            continue;
        }

        rebaseRecordGeneration(
            record,
            sourceGeneration
            );
    }

    for (ImportDiagnostic &diagnostic
         : result.diagnostics) {
        if (!diagnostic.source.has_value()
            || !sourcePathsMatch(
                diagnostic
                    .source
                    ->sourcePath,
                sourcePath
                )) {
            continue;
        }

        rebaseDiagnosticGeneration(
            diagnostic,
            sourceGeneration
            );
    }
}