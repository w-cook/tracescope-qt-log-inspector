#include "ImportPreviewService.h"

#include "BuiltInImporterRegistry.h"
#include "ILogImporter.h"
#include "ImportProfileValidator.h"
#include "SourceFamilyImportService.h"

namespace
{
ImportPreviewResult serviceFailure(
    const ProfileValidationResult &validation,
    const QString &code,
    const QString &message
    )
{
    ImportPreviewResult result;

    result.profileValidation = validation;
    result.errorCode = code;
    result.errorMessage = message;

    return result;
}
}

ImportPreviewResult ImportPreviewService::previewFile(
    const QString &filePath,
    const ImportProfile &profile,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    return previewFiles(
        QStringList {
            filePath
        },
        profile,
        maxProcessedRecords,
        executionContext
        );
}

ImportPreviewResult ImportPreviewService::previewFiles(
    const QStringList &orderedFilePaths,
    const ImportProfile &profile,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    ImportPreviewResult result;

    result.profileValidation =
        ImportProfileValidator().validate(
            profile
            );

    if (!result.profileValidation.isValid()) {
        return result;
    }

    if (maxProcessedRecords <= 0) {
        return serviceFailure(
            result.profileValidation,
            QStringLiteral(
                "INVALID_PREVIEW_LIMIT"
                ),
            QStringLiteral(
                "The preview record limit must "
                "be greater than zero."
                )
            );
    }

    if (orderedFilePaths.isEmpty()) {
        return serviceFailure(
            result.profileValidation,
            QStringLiteral(
                "PREVIEW_SOURCE_MISSING"
                ),
            QStringLiteral(
                "At least one source file is "
                "required for preview."
                )
            );
    }

    const ImporterRegistry registry =
        createBuiltInImporterRegistry(
            profile
            );

    const std::shared_ptr<ILogImporter> importer =
        registry.importerById(
            profile.importerId
            );

    if (!importer) {
        return serviceFailure(
            result.profileValidation,
            QStringLiteral(
                "PREVIEW_IMPORTER_UNSUPPORTED"
                ),
            QStringLiteral(
                "Preview is not implemented for "
                "importer '%1'."
                )
                .arg(
                    profile.importerId
                    )
            );
    }

    SourceFamilyImportOptions options;

    options.maxProcessedRecords =
        maxProcessedRecords;

    options.recordLimitMode =
        SourceFamilyRecordLimitMode::
        DistributedAcrossSources;

    result.importResult =
        SourceFamilyImportService()
            .importFiles(
                orderedFilePaths,
                *importer,
                options,
                executionContext
                );

    result.sourceTruncated =
        result.importResult.sourceTruncated;

    return result;
}