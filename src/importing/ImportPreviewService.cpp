#include "ImportPreviewService.h"

#include <QFile>
#include <QStringList>
#include <QTextStream>

#include "ImportProfileValidator.h"
#include "JsonLinesImporter.h"

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

    if (profile.importerId !=
        QStringLiteral("json-lines")) {
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

    JsonLinesImporter importer(profile);

    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text
            )) {
        result.importResult =
            importer.importFile(filePath);

        return result;
    }

    QTextStream stream(&file);

    QStringList lines;
    qint64 processedRecords = 0;

    while (!stream.atEnd()
           && processedRecords <
                  maxProcessedRecords) {
        const QString line =
            stream.readLine();

        lines.append(line);

        if (!line.trimmed().isEmpty()) {
            ++processedRecords;
        }
    }

    result.sourceTruncated =
        !stream.atEnd();

    result.importResult =
        importer.importLines(
            lines,
            filePath
            );

    return result;
}