#include "BuiltInImporterRegistry.h"

#include <memory>

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

    return registry;
}