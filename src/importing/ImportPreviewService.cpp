#include "ImportPreviewService.h"

#include "BuiltInImporterRegistry.h"
#include "ILogImporter.h"
#include "ImportProfileValidator.h"

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
    qint64 maxProcessedRecords
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
                "The preview record limit must be greater than zero."
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
                "Preview is not implemented for importer '%1'."
                ).arg(profile.importerId)
            );
    }

    result.importResult =
        importer->importFile(
            filePath,
            maxProcessedRecords
            );

    result.sourceTruncated =
        result.importResult.sourceTruncated;

    return result;
}