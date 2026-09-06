#include "LogRecordRendererFactory.h"

#include "renderers/DelimitedTextRenderer.h"
#include "renderers/JsonLinesRenderer.h"

namespace
{
LogRecordRendererCreateResult failure(
    const QString &code,
    const QString &message
    )
{
    LogRecordRendererCreateResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}
}

QStringList
LogRecordRendererFactory::supportedFormats()
{
    return {
        QStringLiteral("jsonl"),
        QStringLiteral("csv"),
        QStringLiteral("tsv")
    };
}

LogRecordRendererCreateResult
LogRecordRendererFactory::create(
    const QString &formatId,
    const LiveLogScenario &scenario
    )
{
    const QString normalized =
        formatId.trimmed().toLower();

    LogRecordRendererCreateResult result;

    if (normalized
        == QStringLiteral("jsonl")) {
        result.renderer =
            std::make_unique<
                JsonLinesRenderer
                >();

        return result;
    }

    const QStringList attributeKeys =
        DelimitedTextRenderer::
        attributeKeysForScenario(
            scenario
            );

    if (normalized
        == QStringLiteral("csv")) {
        result.renderer =
            std::make_unique<
                DelimitedTextRenderer
                >(
                QLatin1Char(','),
                attributeKeys
                );

        return result;
    }

    if (normalized
        == QStringLiteral("tsv")) {
        result.renderer =
            std::make_unique<
                DelimitedTextRenderer
                >(
                QLatin1Char('\t'),
                attributeKeys
                );

        return result;
    }

    return failure(
        QStringLiteral(
            "UNSUPPORTED_OUTPUT_FORMAT"
            ),
        QStringLiteral(
            "Unsupported format '%1'. "
            "Supported formats: %2."
            )
            .arg(
                normalized,
                supportedFormats().join(
                    QStringLiteral(", ")
                    )
                )
        );
}