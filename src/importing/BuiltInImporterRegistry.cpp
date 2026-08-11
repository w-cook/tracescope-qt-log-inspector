#include "BuiltInImporterRegistry.h"

#include <memory>

#include "DelimitedTextImporter.h"
#include "JsonLinesImporter.h"

ImporterRegistry createBuiltInImporterRegistry(
    const ImportProfile &profile
    )
{
    ImporterRegistry registry;

    registry.registerImporter(
        std::make_shared<JsonLinesImporter>(
            profile
            )
        );

    registry.registerImporter(
        std::make_shared<DelimitedTextImporter>(
            QStringLiteral("csv"),
            QStringLiteral("CSV"),
            QLatin1Char(','),
            profile
            )
        );

    registry.registerImporter(
        std::make_shared<DelimitedTextImporter>(
            QStringLiteral("tsv"),
            QStringLiteral("TSV"),
            QLatin1Char('\t'),
            profile
            )
        );

    return registry;
}