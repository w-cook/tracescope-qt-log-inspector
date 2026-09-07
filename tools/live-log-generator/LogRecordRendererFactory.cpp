#include "LogRecordRendererFactory.h"

#include <variant>

#include "renderers/DelimitedTextRenderer.h"
#include "renderers/JsonLinesRenderer.h"
#include "renderers/KeyValueTextRenderer.h"
#include "renderers/StructuredJsonRenderer.h"
#include "renderers/StructuredXmlRenderer.h"
#include "renderers/WindowsEventXmlRenderer.h"

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
        QStringLiteral("tsv"),
        QStringLiteral("logfmt"),
        QStringLiteral("structured-json"),
        QStringLiteral("structured-xml"),
        QStringLiteral("windows-event-xml")
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

    if (normalized
        == QStringLiteral("logfmt")) {
        for (qsizetype stepIndex = 0;
             stepIndex < scenario.steps.size();
             ++stepIndex) {
            const auto *recordStep =
                std::get_if<LiveLogRecordStep>(
                    &scenario.steps.at(
                        stepIndex
                        )
                    );

            if (!recordStep) {
                continue;
            }

            for (auto iterator =
                 recordStep
                     ->record
                     .attributes
                     .constBegin();
                 iterator !=
                 recordStep
                     ->record
                     .attributes
                     .constEnd();
                 ++iterator) {
                if (KeyValueTextRenderer::
                    isValidAttributeKey(
                        iterator.key()
                        )) {
                    continue;
                }

                return failure(
                    QStringLiteral(
                        "INVALID_LOGFMT_ATTRIBUTE_KEY"
                        ),
                    QStringLiteral(
                        "Scenario step %1 contains "
                        "attribute key '%2', which "
                        "cannot be rendered as logfmt. "
                        "Keys must be non-empty and "
                        "cannot contain whitespace or '='."
                        )
                        .arg(stepIndex + 1)
                        .arg(iterator.key())
                    );
            }
        }

        result.renderer =
            std::make_unique<
                KeyValueTextRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("structured-json")) {
        result.renderer =
            std::make_unique<
                StructuredJsonRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("structured-xml")) {
        result.renderer =
            std::make_unique<
                StructuredXmlRenderer
                >();

        return result;
    }

    if (normalized
        == QStringLiteral("windows-event-xml")) {
        result.renderer =
            std::make_unique<
                WindowsEventXmlRenderer
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