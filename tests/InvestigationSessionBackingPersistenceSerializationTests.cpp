#include <QtTest>

#include <QJsonObject>

#include "../src/workspace/InvestigationSessionBackingPersistenceSerialization.h"

namespace
{
ImportProfile jsonLinesProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("JSON Lines");

    profile.importerId =
        QStringLiteral("json-lines");

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    return profile;
}

PersistedInvestigationExternalSourceBinding
externalBinding()
{
    PersistedInvestigationExternalSourceBinding
        binding;

    binding.sourcePath =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    binding.logicalSourceKey =
        QStringLiteral(
            "C:/logs/live.jsonl"
            );

    binding.sourceGeneration = 7;

    binding
        .sourceFamilyConfiguration
        .includeRotatedSources = true;

    binding
        .sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    binding
        .sourceFamilyConfiguration
        .rotationRule
        .numericOrderDirection =
        RotatedSourceOrderDirection::
        Descending;

    /*
     * This must never survive persistence. Physical
     * family members are rediscovered.
     */
    binding
        .sourceFamilyConfiguration
        .rotatedSourcePaths = {
        QStringLiteral(
            "C:/logs/live.jsonl.2"
            ),
        QStringLiteral(
            "C:/logs/live.jsonl.1"
            )
    };

    SourcePhysicalIdentity identity;

    identity.birthTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-09-15T12:34:56.000Z"
                ),
            Qt::ISODateWithMs
            );

    identity.fingerprintLength = 128;

    identity.prefixFingerprint =
        QByteArray(
            32,
            '\x42'
            );

    identity.observedSizeBytes = 4096;

    binding.sourceIdentity =
        identity;

    return binding;
}
}

class InvestigationSessionBackingPersistenceSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void sourceBackedRoundTripsWithOptionalFallback();
    void snapshotBackedRoundTrips();
    void hybridRoundTrips();
    void invalidBackingCombinationIsRejected();
    void invalidSourceIdentityIsRejected();
    void missingLogicalSourceKeyFallsBackToSourcePath();
};

void
    InvestigationSessionBackingPersistenceSerializationTests::
    sourceBackedRoundTripsWithOptionalFallback()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    backing.snapshotReference =
        QStringLiteral(
            "sessions/session-1.tsinv"
            );

    backing.externalSourceBinding =
        externalBinding();

    backing.sourceImportProfile =
        jsonLinesProfile();

    InvestigationSessionBackingPersistenceSerializer
        serializer;

    const QJsonObject json =
        serializer.serialize(
            backing
            );

    QCOMPARE(
        json.value(
                QStringLiteral("mode")
                ).toString(),
        QStringLiteral("sourceBacked")
        );

    QVERIFY(
        json.contains(
            QStringLiteral(
                "snapshotReference"
                )
            )
        );

    const QJsonObject external =
        json.value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .toObject();

    QCOMPARE(
        external.value(
                    QStringLiteral(
                        "logicalSourceKey"
                        )
                    )
            .toString(),
        QStringLiteral(
            "C:/logs/live.jsonl"
            )
        );

    QCOMPARE(
        external.value(
                    QStringLiteral(
                        "sourceGeneration"
                        )
                    )
            .toString(),
        QStringLiteral("7")
        );

    const QJsonObject family =
        external.value(
                    QStringLiteral(
                        "sourceFamilyConfiguration"
                        )
                    )
            .toObject();

    QVERIFY(
        !family.contains(
            QStringLiteral(
                "rotatedSourcePaths"
                )
            )
        );

    const auto result =
        serializer.deserialize(
            json
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    const auto &restored =
        *result.backing;

    QVERIFY(
        restored.mode
        == PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        restored.snapshotReference.has_value()
        );

    QCOMPARE(
        *restored.snapshotReference,
        QStringLiteral(
            "sessions/session-1.tsinv"
            )
        );

    QCOMPARE(
        restored
            .externalSourceBinding
            ->logicalSourceKey,
        QStringLiteral(
            "C:/logs/live.jsonl"
            )
        );

    QVERIFY(
        restored
            .externalSourceBinding
            .has_value()
        );

    QCOMPARE(
        restored
            .externalSourceBinding
            ->sourceGeneration,
        quint64(7)
        );

    QVERIFY(
        restored
            .externalSourceBinding
            ->sourceIdentity
            .has_value()
        );

    QCOMPARE(
        restored
            .externalSourceBinding
            ->sourceIdentity
            ->fingerprintLength,
        qint64(128)
        );

    QCOMPARE(
        restored
            .externalSourceBinding
            ->sourceIdentity
            ->observedSizeBytes,
        qint64(4096)
        );

    QCOMPARE(
        restored
            .externalSourceBinding
            ->sourceIdentity
            ->prefixFingerprint,
        QByteArray(
            32,
            '\x42'
            )
        );

    QVERIFY(
        restored
            .externalSourceBinding
            ->sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QVERIFY(
        restored.sourceImportProfile.has_value()
        );

    QCOMPARE(
        restored
            .sourceImportProfile
            ->importerId,
        QStringLiteral("json-lines")
        );
}

void
    InvestigationSessionBackingPersistenceSerializationTests::
    snapshotBackedRoundTrips()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        SnapshotBacked;

    backing.snapshotReference =
        QStringLiteral(
            "sessions/snapshot-only.tsinv"
            );

    InvestigationSessionBackingPersistenceSerializer
        serializer;

    const auto result =
        serializer.deserialize(
            serializer.serialize(
                backing
                )
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.backing->mode
        == PersistedInvestigationSessionBackingMode::
        SnapshotBacked
        );

    QVERIFY(
        result
            .backing
            ->snapshotReference
            .has_value()
        );

    QVERIFY(
        !result
             .backing
             ->externalSourceBinding
             .has_value()
        );

    QVERIFY(
        !result
             .backing
             ->sourceImportProfile
             .has_value()
        );
}

void
    InvestigationSessionBackingPersistenceSerializationTests::
    hybridRoundTrips()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        Hybrid;

    backing.snapshotReference =
        QStringLiteral(
            "sessions/hybrid.tsinv"
            );

    backing.externalSourceBinding =
        externalBinding();

    InvestigationSessionBackingPersistenceSerializer
        serializer;

    const QJsonObject json =
        serializer.serialize(
            backing
            );

    QVERIFY(
        !json.contains(
            QStringLiteral(
                "sourceImportProfile"
                )
            )
        );

    const auto result =
        serializer.deserialize(
            json
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.backing->mode
        == PersistedInvestigationSessionBackingMode::
        Hybrid
        );

    QVERIFY(
        result
            .backing
            ->snapshotReference
            .has_value()
        );

    QVERIFY(
        result
            .backing
            ->externalSourceBinding
            .has_value()
        );

    QVERIFY(
        !result
             .backing
             ->sourceImportProfile
             .has_value()
        );
}

void
    InvestigationSessionBackingPersistenceSerializationTests::
    invalidBackingCombinationIsRejected()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        Hybrid;

    backing.externalSourceBinding =
        externalBinding();

    /*
     * Hybrid requires a snapshot reference.
     */
    InvestigationSessionBackingPersistenceSerializer
        serializer;

    const auto result =
        serializer.deserialize(
            serializer.serialize(
                backing
                )
            );

    QVERIFY(
        !result.isSuccess()
        );

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_BACKING_CONFIGURATION"
            )
        );
}

void
    InvestigationSessionBackingPersistenceSerializationTests::
    invalidSourceIdentityIsRejected()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    backing.externalSourceBinding =
        externalBinding();

    backing.sourceImportProfile =
        jsonLinesProfile();

    InvestigationSessionBackingPersistenceSerializer
        serializer;

    QJsonObject json =
        serializer.serialize(
            backing
            );

    QJsonObject external =
        json.value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .toObject();

    QJsonObject identity =
        external.value(
                    QStringLiteral(
                        "sourceIdentity"
                        )
                    )
            .toObject();

    identity.insert(
        QStringLiteral(
            "fingerprintLength"
            ),
        QStringLiteral("-1")
        );

    external.insert(
        QStringLiteral("sourceIdentity"),
        identity
        );

    json.insert(
        QStringLiteral(
            "externalSourceBinding"
            ),
        external
        );

    const auto result =
        serializer.deserialize(
            json
            );

    QVERIFY(
        !result.isSuccess()
        );

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_EXTERNAL_SOURCE_BINDING"
            )
        );
}

void
    InvestigationSessionBackingPersistenceSerializationTests::
    missingLogicalSourceKeyFallsBackToSourcePath()
{
    PersistedInvestigationSessionBacking
        backing;

    backing.mode =
        PersistedInvestigationSessionBackingMode::
        SourceBacked;

    backing.externalSourceBinding =
        externalBinding();

    backing.sourceImportProfile =
        jsonLinesProfile();

    InvestigationSessionBackingPersistenceSerializer
        serializer;

    QJsonObject json =
        serializer.serialize(
            backing
            );

    QJsonObject external =
        json.value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .toObject();

    /*
     * Simulate a workspace written before logical
     * source identity was persisted explicitly.
     */
    external.remove(
        QStringLiteral(
            "logicalSourceKey"
            )
        );

    json.insert(
        QStringLiteral(
            "externalSourceBinding"
            ),
        external
        );

    const auto result =
        serializer.deserialize(
            json
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result
            .backing
            ->externalSourceBinding
            .has_value()
        );

    QCOMPARE(
        result
            .backing
            ->externalSourceBinding
            ->logicalSourceKey,
        QStringLiteral(
            "C:/logs/live.jsonl"
            )
        );
}

QTEST_MAIN(
    InvestigationSessionBackingPersistenceSerializationTests
    )

#include "InvestigationSessionBackingPersistenceSerializationTests.moc"