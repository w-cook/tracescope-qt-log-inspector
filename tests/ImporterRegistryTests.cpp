#include <QtTest/QtTest>

#include "../src/importing/ILogImporter.h"
#include "../src/importing/ImporterRegistry.h"

class FakeImporter final : public ILogImporter
{
public:
    FakeImporter(
        QString importerId,
        QString importerDisplayName
        )
        : importerId(std::move(importerId)),
        importerDisplayName(std::move(importerDisplayName))
    {
    }

    QString id() const override
    {
        return importerId;
    }

    QString displayName() const override
    {
        return importerDisplayName;
    }

    ImportResult importFile(
        const QString &,
        qint64
        ) const override
    {
        return {};
    }

private:
    QString importerId;
    QString importerDisplayName;
};

class ImporterRegistryTests : public QObject
{
    Q_OBJECT

private slots:
    void registersImporter();
    void findsImporterById();
    void returnsEmptyForUnknownId();
    void ignoresDuplicateImporterId();
    void ignoresNullImporter();
};

void ImporterRegistryTests::registersImporter()
{
    ImporterRegistry registry;

    registry.registerImporter(
        std::make_shared<FakeImporter>(
            QStringLiteral("json-lines"),
            QStringLiteral("JSON Lines")
            )
        );

    QCOMPARE(registry.importers().size(), 1);
}

void ImporterRegistryTests::findsImporterById()
{
    ImporterRegistry registry;

    auto importer = std::make_shared<FakeImporter>(
        QStringLiteral("json-lines"),
        QStringLiteral("JSON Lines")
        );

    registry.registerImporter(importer);

    QCOMPARE(
        registry.importerById(
            QStringLiteral("json-lines")
            ),
        importer
        );
}

void ImporterRegistryTests::returnsEmptyForUnknownId()
{
    ImporterRegistry registry;

    QVERIFY(
        !registry.importerById(
            QStringLiteral("missing")
            )
        );
}

void ImporterRegistryTests::ignoresDuplicateImporterId()
{
    ImporterRegistry registry;

    registry.registerImporter(
        std::make_shared<FakeImporter>(
            QStringLiteral("json-lines"),
            QStringLiteral("JSON Lines")
            )
        );

    registry.registerImporter(
        std::make_shared<FakeImporter>(
            QStringLiteral("json-lines"),
            QStringLiteral("Duplicate")
            )
        );

    QCOMPARE(registry.importers().size(), 1);
    QCOMPARE(
        registry.importerById(
                    QStringLiteral("json-lines")
                    )->displayName(),
        QStringLiteral("JSON Lines")
        );
}

void ImporterRegistryTests::ignoresNullImporter()
{
    ImporterRegistry registry;

    registry.registerImporter({});

    QVERIFY(registry.importers().isEmpty());
}

QTEST_MAIN(ImporterRegistryTests)

#include "ImporterRegistryTests.moc"