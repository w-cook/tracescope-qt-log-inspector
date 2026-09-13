#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/sources/RotatedSourceDiscoveryService.h"
#include "../src/sources/RotatedSourceRule.h"

namespace
{
bool writeFile(
    const QString &filePath
    )
{
    QFile file(
        filePath
        );

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )) {
        return false;
    }

    return file.write(
               QByteArrayLiteral(
                   "test"
                   )
               )
           >= 0;
}
}

class RotatedSourceDiscoveryServiceTests
    : public QObject
{
    Q_OBJECT

private slots:
    void disabledDiscoveryReturnsNoMatches();
    void discoversNumericSuffixRotationsOldestFirst();
    void discoversNumericBeforeExtensionRotationsOldestFirst();
    void ignoresUnrelatedAndInvalidNumericSuffixes();
    void missingActiveSourceFails();
    void discoversIsoDateTimeRotationsOldestFirst();
    void discoversCustomNumericPattern();
    void discoversCustomDatePatternChronologically();
    void invalidCustomRegexFails();
    void invalidCustomCaptureGroupFails();
    void suggestsLikelyBuiltInRules();
};

void RotatedSourceDiscoveryServiceTests::
    disabledDiscoveryReturnsNoMatches()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "test.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            activePath
            + QStringLiteral(
                ".1"
                )
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::Disabled;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY(
        result.succeeded
        );

    QVERIFY(
        result.rotatedSources.isEmpty()
        );
}

void RotatedSourceDiscoveryServiceTests::
    discoversNumericSuffixRotationsOldestFirst()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "test.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            activePath
            + QStringLiteral(".1")
            )
        );

    QVERIFY(
        writeFile(
            activePath
            + QStringLiteral(".2")
            )
        );

    QVERIFY(
        writeFile(
            activePath
            + QStringLiteral(".10")
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.rotatedSources.size(),
        3
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .fileName,
        QStringLiteral(
            "test.log.10"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .rotationIndex,
        qint64(10)
        );

    QCOMPARE(
        result.rotatedSources.at(1)
            .fileName,
        QStringLiteral(
            "test.log.2"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(2)
            .fileName,
        QStringLiteral(
            "test.log.1"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    discoversNumericBeforeExtensionRotationsOldestFirst()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service.1.log"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service.4.log"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service.12.log"
                    )
                )
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericBeforeExtension;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.rotatedSources.size(),
        3
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .fileName,
        QStringLiteral(
            "service.12.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(1)
            .fileName,
        QStringLiteral(
            "service.4.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(2)
            .fileName,
        QStringLiteral(
            "service.1.log"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    ignoresUnrelatedAndInvalidNumericSuffixes()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "test.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "test.log.1"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "test.log.0"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "test.log.bak"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "other.log.2"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "test.2.log"
                    )
                )
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY(
        result.succeeded
        );

    QCOMPARE(
        result.rotatedSources.size(),
        1
        );

    QCOMPARE(
        result.rotatedSources.first()
            .fileName,
        QStringLiteral(
            "test.log.1"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    missingActiveSourceFails()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            directory.filePath(
                QStringLiteral(
                    "missing.log"
                    )
                ),
            rule
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );
}

void RotatedSourceDiscoveryServiceTests::
    discoversIsoDateTimeRotationsOldestFirst()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service-2026-09-10.log"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service-2026-09-12.log"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service-2026-09-12T14-30-05.log"
                    )
                )
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        IsoDateTimeBeforeExtension;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.rotatedSources.size(),
        3
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .fileName,
        QStringLiteral(
            "service-2026-09-10.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(1)
            .fileName,
        QStringLiteral(
            "service-2026-09-12.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(2)
            .fileName,
        QStringLiteral(
            "service-2026-09-12T14-30-05.log"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    discoversCustomNumericPattern()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    for (const QString &fileName
         : {
             QStringLiteral(
                 "service.archive.1.log"
                 ),
             QStringLiteral(
                 "service.archive.3.log"
                 ),
             QStringLiteral(
                 "service.archive.12.log"
                 )
         }) {
        QVERIFY(
            writeFile(
                directory.filePath(
                    fileName
                    )
                )
            );
    }

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    rule.customRegularExpression =
        QStringLiteral(
            "^service\\.archive\\.(\\d+)\\.log$"
            );

    rule.customOrderCaptureGroup = 1;

    rule.customOrderValueType =
        RotatedSourceOrderValueType::
        Numeric;

    rule.customOrderDirection =
        RotatedSourceOrderDirection::
        Descending;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.rotatedSources.size(),
        3
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .fileName,
        QStringLiteral(
            "service.archive.12.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(1)
            .fileName,
        QStringLiteral(
            "service.archive.3.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(2)
            .fileName,
        QStringLiteral(
            "service.archive.1.log"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    discoversCustomDatePatternChronologically()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    for (const QString &fileName
         : {
             QStringLiteral(
                 "service_12-31-2025.log"
                 ),
             QStringLiteral(
                 "service_01-02-2026.log"
                 ),
             QStringLiteral(
                 "service_09-12-2026.log"
                 )
         }) {
        QVERIFY(
            writeFile(
                directory.filePath(
                    fileName
                    )
                )
            );
    }

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    rule.customRegularExpression =
        QStringLiteral(
            "^service_(\\d{2}-\\d{2}-\\d{4})\\.log$"
            );

    rule.customOrderCaptureGroup = 1;

    rule.customOrderValueType =
        RotatedSourceOrderValueType::
        DateTime;

    rule.customOrderDirection =
        RotatedSourceOrderDirection::
        Ascending;

    rule.customDateTimeFormat =
        QStringLiteral(
            "MM-dd-yyyy"
            );

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.rotatedSources.size(),
        3
        );

    QCOMPARE(
        result.rotatedSources.at(0)
            .fileName,
        QStringLiteral(
            "service_12-31-2025.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(1)
            .fileName,
        QStringLiteral(
            "service_01-02-2026.log"
            )
        );

    QCOMPARE(
        result.rotatedSources.at(2)
            .fileName,
        QStringLiteral(
            "service_09-12-2026.log"
            )
        );
}

void RotatedSourceDiscoveryServiceTests::
    invalidCustomRegexFails()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    rule.customRegularExpression =
        QStringLiteral(
            "[invalid"
            );

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY(
        !result.succeeded
        );

    QVERIFY(
        !result.errorMessage.isEmpty()
        );
}

void RotatedSourceDiscoveryServiceTests::
    invalidCustomCaptureGroupFails()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    RotatedSourceRule rule;

    rule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    rule.customRegularExpression =
        QStringLiteral(
            "^service\\.(\\d+)\\.log$"
            );

    rule.customOrderCaptureGroup = 2;

    const RotatedSourceDiscoveryResult result =
        RotatedSourceDiscoveryService::discover(
            activePath,
            rule
            );

    QVERIFY(
        !result.succeeded
        );
}

void RotatedSourceDiscoveryServiceTests::
    suggestsLikelyBuiltInRules()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString activePath =
        directory.filePath(
            QStringLiteral(
                "service.log"
                )
            );

    QVERIFY(
        writeFile(
            activePath
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service.log.1"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service.log.2"
                    )
                )
            )
        );

    QVERIFY(
        writeFile(
            directory.filePath(
                QStringLiteral(
                    "service-2026-09-12.log"
                    )
                )
            )
        );

    const RotatedSourceRuleSuggestionResult
        result =
        RotatedSourceDiscoveryService::
        suggestRules(
            activePath
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.suggestions.size(),
        2
        );

    QCOMPARE(
        result.suggestions.at(0)
            .rule
            .namingScheme,
        RotatedSourceNamingScheme::
        NumericSuffix
        );

    QCOMPARE(
        result.suggestions.at(0)
            .matchCount,
        2
        );

    QCOMPARE(
        result.suggestions.at(1)
            .rule
            .namingScheme,
        RotatedSourceNamingScheme::
        IsoDateTimeBeforeExtension
        );

    QCOMPARE(
        result.suggestions.at(1)
            .matchCount,
        1
        );
}

QTEST_MAIN(
    RotatedSourceDiscoveryServiceTests
    )

#include "RotatedSourceDiscoveryServiceTests.moc"