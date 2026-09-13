#include "RotatedSourceDiscoveryService.h"

#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>

#include <algorithm>
#include <utility>

namespace
{
RotatedSourceDiscoveryResult failure(
    const QString &message
    )
{
    RotatedSourceDiscoveryResult result;

    result.succeeded = false;
    result.errorMessage = message;

    return result;
}

QRegularExpression numericSuffixExpression(
    const QFileInfo &activeSourceInfo
    )
{
    const QString escapedFileName =
        QRegularExpression::escape(
            activeSourceInfo.fileName()
            );

    return QRegularExpression(
        QStringLiteral(
            "^%1\\.(\\d+)$"
            )
            .arg(
                escapedFileName
                )
        );
}

QRegularExpression numericBeforeExtensionExpression(
    const QFileInfo &activeSourceInfo
    )
{
    const QString suffix =
        activeSourceInfo.suffix();

    /*
     * A source without an extension has no meaningful
     * "before extension" boundary. In that case use
     * the same shape as numeric-suffix rotation.
     */
    if (suffix.isEmpty()) {
        return numericSuffixExpression(
            activeSourceInfo
            );
    }

    const QString escapedBaseName =
        QRegularExpression::escape(
            activeSourceInfo.completeBaseName()
            );

    const QString escapedSuffix =
        QRegularExpression::escape(
            suffix
            );

    return QRegularExpression(
        QStringLiteral(
            "^%1\\.(\\d+)\\.%2$"
            )
            .arg(
                escapedBaseName,
                escapedSuffix
                )
        );
}
}

RotatedSourceDiscoveryResult
RotatedSourceDiscoveryService::discover(
    const QString &activeSourcePath,
    const RotatedSourceRule &rule
    )
{
    RotatedSourceDiscoveryResult result;

    if (rule.namingScheme
        == RotatedSourceNamingScheme::Disabled) {
        return result;
    }

    const QFileInfo activeSourceInfo(
        activeSourcePath
        );

    if (!activeSourceInfo.exists()
        || !activeSourceInfo.isFile()) {
        return failure(
            QStringLiteral(
                "The active source file does not exist "
                "or is not a regular file."
                )
            );
    }

    QRegularExpression expression;

    switch (rule.namingScheme) {
    case RotatedSourceNamingScheme::Disabled:
        return result;

    case RotatedSourceNamingScheme::NumericSuffix:
        expression =
            numericSuffixExpression(
                activeSourceInfo
                );
        break;

    case RotatedSourceNamingScheme::
        NumericBeforeExtension:
        expression =
            numericBeforeExtensionExpression(
                activeSourceInfo
                );
        break;
    }

    if (!expression.isValid()) {
        return failure(
            QStringLiteral(
                "The rotated-source naming expression "
                "could not be constructed."
                )
            );
    }

    const QDir directory(
        activeSourceInfo.absolutePath()
        );

    const QFileInfoList entries =
        directory.entryInfoList(
            QDir::Files
                | QDir::NoDotAndDotDot,
            QDir::Name
            );

    for (const QFileInfo &entry
         : entries) {
        const QRegularExpressionMatch match =
            expression.match(
                entry.fileName()
                );

        if (!match.hasMatch()) {
            continue;
        }

        bool converted = false;

        const qint64 rotationIndex =
            match
                .captured(1)
                .toLongLong(
                    &converted
                    );

        /*
         * Rotation zero is the active-source concept,
         * not a rotated sibling. Extremely large
         * numeric suffixes that cannot fit qint64 are
         * likewise ignored rather than guessed.
         */
        if (!converted
            || rotationIndex <= 0) {
            continue;
        }

        RotatedSourceMatch sourceMatch;

        sourceMatch.filePath =
            entry.absoluteFilePath();

        sourceMatch.fileName =
            entry.fileName();

        sourceMatch.rotationIndex =
            rotationIndex;

        result.rotatedSources.append(
            std::move(
                sourceMatch
                )
            );
    }

    /*
     * Numbered rotation convention:
     *
     * test.log.1  = newest archived source
     * test.log.2  = older
     * test.log.10 = older still
     *
     * Import-family ordering therefore runs from the
     * largest rotation index toward one. The active
     * source can later be appended as the newest
     * physical source.
     */
    std::sort(
        result.rotatedSources.begin(),
        result.rotatedSources.end(),
        [](
            const RotatedSourceMatch &left,
            const RotatedSourceMatch &right
            ) {
            if (left.rotationIndex
                != right.rotationIndex) {
                return left.rotationIndex
                       > right.rotationIndex;
            }

            return left.fileName
                   < right.fileName;
        }
        );

    return result;
}