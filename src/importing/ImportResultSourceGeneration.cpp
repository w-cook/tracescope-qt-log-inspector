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

void rebaseRecordLogicalSourceKey(
    InvestigationRecord &record,
    const QString &logicalSourceKey
    )
{
    record.source.logicalSourceKey =
        logicalSourceKey;

    record.recordId =
        createStableRecordIdentity(
            record.source,
            record.rawSource
            );
}

void rebaseDiagnosticLogicalSourceKey(
    ImportDiagnostic &diagnostic,
    const QString &logicalSourceKey
    )
{
    if (!diagnostic.source.has_value()) {
        return;
    }

    diagnostic
        .source
        ->logicalSourceKey =
        logicalSourceKey;
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

void rebaseImportResultLogicalSourceKeyForPath(
    ImportResult &result,
    const QString &sourcePath,
    const QString &logicalSourceKey
    )
{
    const QString effectiveLogicalSourceKey =
        logicalSourceKey.trimmed().isEmpty()
            ? sourcePath
            : logicalSourceKey;

    for (InvestigationRecord &record
         : result.records) {
        if (!sourcePathsMatch(
                record.source.sourcePath,
                sourcePath
                )) {
            continue;
        }

        rebaseRecordLogicalSourceKey(
            record,
            effectiveLogicalSourceKey
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

        rebaseDiagnosticLogicalSourceKey(
            diagnostic,
            effectiveLogicalSourceKey
            );
    }
}