#include "ImporterRegistry.h"

#include "ILogImporter.h"

void ImporterRegistry::registerImporter(
    std::shared_ptr<ILogImporter> importer
    )
{
    if (!importer) {
        return;
    }

    for (const auto &registeredImporter : registeredImporters) {
        if (registeredImporter->id() == importer->id()) {
            return;
        }
    }

    registeredImporters.append(std::move(importer));
}

std::shared_ptr<ILogImporter> ImporterRegistry::importerById(
    const QString &id
    ) const
{
    for (const auto &importer : registeredImporters) {
        if (importer->id() == id) {
            return importer;
        }
    }

    return {};
}

QVector<std::shared_ptr<ILogImporter>>
ImporterRegistry::importers() const
{
    return registeredImporters;
}