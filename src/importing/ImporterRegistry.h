#pragma once

#include <memory>

#include <QString>
#include <QVector>

class ILogImporter;

class ImporterRegistry
{
public:
    void registerImporter(
        std::shared_ptr<ILogImporter> importer
        );

    std::shared_ptr<ILogImporter> importerById(
        const QString &id
        ) const;

    QVector<std::shared_ptr<ILogImporter>> importers() const;

private:
    QVector<std::shared_ptr<ILogImporter>> registeredImporters;
};