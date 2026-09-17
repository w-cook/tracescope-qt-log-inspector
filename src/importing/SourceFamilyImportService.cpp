#include "SourceFamilyImportService.h"

#include <QFileInfo>

#include <algorithm>
#include <utility>

namespace
{
qint64 totalSourceBytes(
    const QStringList &filePaths
    )
{
    qint64 total = 0;

    for (const QString &filePath
         : filePaths) {
        const qint64 size =
            QFileInfo(filePath).size();

        if (size > 0) {
            total += size;
        }
    }

    return total;
}

void appendImportResult(
    ImportResult &combined,
    ImportResult member
    )
{
    combined.processedRecordCount +=
        member.processedRecordCount;

    combined.sourceTruncated =
        combined.sourceTruncated
        || member.sourceTruncated;

    combined.cancelled =
        combined.cancelled
        || member.cancelled;

    for (InvestigationRecord &record
         : member.records) {
        combined.records.append(
            std::move(record)
            );
    }

    for (ImportDiagnostic &diagnostic
         : member.diagnostics) {
        combined.diagnostics.append(
            std::move(diagnostic)
            );
    }
}
}

ImportResult
SourceFamilyImportService::importFiles(
    const QStringList &orderedFilePaths,
    const ILogImporter &importer,
    const SourceFamilyImportOptions &options,
    const ImportExecutionContext &executionContext
    ) const
{
    ImportResult result;

    if (orderedFilePaths.isEmpty()) {
        return result;
    }

    const bool limited =
        options.maxProcessedRecords
        != ILogImporter::UnlimitedRecordLimit;

    if (limited
        && options.maxProcessedRecords <= 0) {
        return result;
    }

    qint64 remainingRecordBudget =
        options.maxProcessedRecords;

    const qint64 familyTotalBytes =
        totalSourceBytes(
            orderedFilePaths
            );

    qint64 completedSourceBytes = 0;
    qint64 completedRecordCount = 0;

    for (qsizetype index = 0;
         index < orderedFilePaths.size();
         ++index) {
        if (executionContext
                .cancellationRequested()) {
            result.cancelled = true;
            return result;
        }

        if (limited
            && remainingRecordBudget <= 0) {
            result.sourceTruncated = true;
            break;
        }

        const QString &filePath =
            orderedFilePaths.at(index);

        const qint64 sourceSize =
            std::max<qint64>(
                0,
                QFileInfo(filePath).size()
                );

        qint64 memberRecordLimit =
            ILogImporter::UnlimitedRecordLimit;

        if (limited) {
            if (options.recordLimitMode
                == SourceFamilyRecordLimitMode::
                DistributedAcrossSources) {
                const qint64 remainingSources =
                    orderedFilePaths.size()
                    - index;

                /*
                 * Divide the remaining family preview
                 * budget as evenly as possible over the
                 * sources still to be sampled.
                 *
                 * Unused budget from a short source
                 * naturally carries forward to later
                 * sources.
                 */
                memberRecordLimit =
                    (
                        remainingRecordBudget
                        + remainingSources
                        - 1
                        )
                    / remainingSources;
            } else {
                memberRecordLimit =
                    remainingRecordBudget;
            }
        }

        ImportExecutionContext
            memberExecutionContext;

        memberExecutionContext
            .isCancellationRequested =
            executionContext
                .isCancellationRequested;

        memberExecutionContext
            .reportProgress =
            [
                executionContext,
                completedSourceBytes,
                completedRecordCount,
                familyTotalBytes
        ](
                const ImportProgress &progress
                ) {
                if (!executionContext
                         .reportProgress) {
                    return;
                }

                ImportProgress familyProgress;

                familyProgress.bytesProcessed =
                    completedSourceBytes
                    + progress.bytesProcessed;

                familyProgress.totalBytes =
                    familyTotalBytes;

                familyProgress.processedRecordCount =
                    completedRecordCount
                    + progress.processedRecordCount;

                executionContext.report(
                    familyProgress
                    );
            };

        ImportResult memberResult =
            importer.importFile(
                filePath,
                memberRecordLimit,
                memberExecutionContext
                );

        const qint64 memberProcessedCount =
            memberResult.processedRecordCount;

        const bool memberCancelled =
            memberResult.cancelled;

        appendImportResult(
            result,
            std::move(memberResult)
            );

        if (limited) {
            remainingRecordBudget -=
                std::min(
                    remainingRecordBudget,
                    memberProcessedCount
                    );
        }

        completedRecordCount =
            result.processedRecordCount;

        /*
         * Once this member has completed, the family
         * operation advances to the next physical
         * source. For an unlimited real import that
         * means the complete file was consumed.
         *
         * A limited preview may intentionally sample
         * only part of the file, but the next progress
         * event still represents advancement into the
         * next member of the family.
         */
        completedSourceBytes +=
            sourceSize;

        if (memberCancelled) {
            return result;
        }
    }

    return result;
}