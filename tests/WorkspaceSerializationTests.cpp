#include <QtTest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../src/importing/ImportProfileSerialization.h"
#include "../src/workspace/InvestigationPresentationStateSerialization.h"
#include "../src/workspace/WorkspaceSerialization.h"

class WorkspaceSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void serializationProducesVersionedJson();
    void sessionContextRoundTripsThroughJson();
    void sourceFamilyConfigurationRoundTripsThroughJson();
    void missingSourceFamilyConfigurationUsesDefaults();
    void invalidSourceFamilyConfigurationIsRejected();
    void comparisonRoundTripsThroughWorkspaceJson();
    void malformedJsonIsRejected();
    void unsupportedSchemaVersionIsRejected();
    void invalidSessionIsRejected();
    void invalidFindingStatusIsRejected();
    void invalidComparisonsCollectionIsRejected();
    void presentationStateRoundTripsThroughJson();
    void missingPresentationStateUsesDefaults();
    void documentLayoutRoundTripsThroughJson();
    void missingDocumentLayoutUsesDefaults();
    void missingMainWindowStateUsesDefaults();
    void missingNumericOrderDirectionUsesDescendingDefault();
    void schemaOneSessionMigratesToSourceBackedV2();
    void defaultPresentationStateOmitsUnsetColumnWidthScaleFactor();
};

void WorkspaceSerializationTests::
    serializationProducesVersionedJson()
{
    WorkspacePersistenceState workspace;

    const QByteArray json =
        WorkspaceSerializer().serialize(
            workspace
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(json);

    QVERIFY(document.isObject());

    const QJsonObject root =
        document.object();

    QCOMPARE(
        root.value(
                QStringLiteral("schemaVersion")
                ).toInt(),
        WorkspacePersistenceState::
        CurrentSchemaVersion
        );

    QVERIFY(
        root.value(
                QStringLiteral("sessions")
                ).isArray()
        );
}

void WorkspaceSerializationTests::
    sessionContextRoundTripsThroughJson()
{
    WorkspacePersistenceState original;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral("session-123");

    session.sourcePath =
        QStringLiteral(
            "/logs/session.jsonl"
            );

    session.importProfile.name =
        QStringLiteral("Saved Profile");

    session.importProfile.importerId =
        QStringLiteral("json-lines");

    session.importProfile
        .canonicalFields.messagePath =
        QStringLiteral("message");

    session.importProfile
        .canonicalFields.severityPath =
        QStringLiteral("level");

    session.importProfile
        .customFields.append({
            QStringLiteral("Request ID"),
            QStringLiteral("requestId")
        });

    PersistedInvestigationRecordState
        recordState;

    recordState.recordId =
        QStringLiteral("record-123");

    recordState.bookmarked = true;

    recordState.note =
        QStringLiteral(
            "Investigate this event"
            );

    recordState.findingStatus =
        FindingStatus::Open;

    session.recordStates.append(
        recordState
        );

    session.filterState.severities = {
        QStringLiteral("WARNING"),
        QStringLiteral("ERROR")
    };

    session.filterState.subsystems = {
        QStringLiteral("Backend")
    };

    session.filterState.searchText =
        QStringLiteral("timeout");

    session.filterState.eventCodes = {
        QStringLiteral("EVT-100")
    };

    session.filterState.entityIds = {
        QStringLiteral("node-4")
    };

    session.filterState.startTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    session.filterState.endTime =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T13:00:00.000Z"
                ),
            Qt::ISODateWithMs
            );

    session.filterState
        .customFieldFilters.insert(
            QStringLiteral("requestId"),
            {
                QStringLiteral("req-123"),
                QStringLiteral("req-456")
            }
            );

    session.filterState.findingStatuses = {
        QStringLiteral("OPEN")
    };

    session.filterState.bookmarkedOnly = true;

    original.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(original);

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(json);

    QVERIFY(result.isSuccess());
    QVERIFY(result.workspace.has_value());

    const WorkspacePersistenceState &restored =
        *result.workspace;

    QCOMPARE(
        restored.schemaVersion,
        WorkspacePersistenceState::
        CurrentSchemaVersion
        );

    QCOMPARE(
        restored.sessions.size(),
        1
        );

    const PersistedInvestigationSession
        &restoredSession =
        restored.sessions.first();

    QCOMPARE(
        restoredSession.sessionId,
        session.sessionId
        );

    QCOMPARE(
        restoredSession.sourcePath,
        session.sourcePath
        );

    QCOMPARE(
        restoredSession.importProfile.name,
        session.importProfile.name
        );

    QCOMPARE(
        restoredSession.importProfile.importerId,
        session.importProfile.importerId
        );

    QCOMPARE(
        restoredSession.importProfile
            .canonicalFields.messagePath,
        session.importProfile
            .canonicalFields.messagePath
        );

    QCOMPARE(
        restoredSession.importProfile
            .canonicalFields.severityPath,
        session.importProfile
            .canonicalFields.severityPath
        );

    QCOMPARE(
        restoredSession.importProfile
            .customFields.size(),
        1
        );

    QCOMPARE(
        restoredSession.importProfile
            .customFields.first().name,
        QStringLiteral("Request ID")
        );

    QCOMPARE(
        restoredSession.importProfile
            .customFields.first().sourcePath,
        QStringLiteral("requestId")
        );

    QCOMPARE(
        restoredSession.recordStates.size(),
        1
        );

    const PersistedInvestigationRecordState
        &restoredState =
        restoredSession.recordStates.first();

    QCOMPARE(
        restoredState.recordId,
        QStringLiteral("record-123")
        );

    QVERIFY(restoredState.bookmarked);

    QCOMPARE(
        restoredState.note,
        QStringLiteral(
            "Investigate this event"
            )
        );

    QVERIFY(
        restoredState.findingStatus
        == FindingStatus::Open
        );

    QCOMPARE(
        restoredSession.filterState.severities,
        QStringList({
            QStringLiteral("WARNING"),
            QStringLiteral("ERROR")
        })
        );

    QCOMPARE(
        restoredSession.filterState.searchText,
        QStringLiteral("timeout")
        );

    QVERIFY(
        restoredSession.filterState.startTime
        == session.filterState.startTime
        );

    QVERIFY(
        restoredSession.filterState.endTime
        == session.filterState.endTime
        );

    QCOMPARE(
        restoredSession
            .filterState
            .customFieldFilters,
        session
            .filterState
            .customFieldFilters
        );

    QCOMPARE(
        restoredSession
            .filterState
            .findingStatuses,
        QStringList({
            QStringLiteral("OPEN")
        })
        );

    QVERIFY(
        restoredSession
            .filterState
            .bookmarkedOnly
        );
}

void WorkspaceSerializationTests::
    sourceFamilyConfigurationRoundTripsThroughJson()
{
    WorkspacePersistenceState workspace;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral(
            "source-family-session"
            );

    /*
     * Populate the temporary compatibility mirrors.
     *
     * WorkspaceSerializer should normalize these into
     * a schema-v2 SourceBacked backing object.
     */
    session.sourcePath =
        QStringLiteral(
            "/logs/current.log"
            );

    session.importProfile.name =
        QStringLiteral(
            "Source Family Profile"
            );

    session.importProfile.importerId =
        QStringLiteral(
            "regex-text"
            );

    session
        .sourceFamilyConfiguration
        .includeRotatedSources =
        true;

    RotatedSourceRule &rotationRule =
        session
            .sourceFamilyConfiguration
            .rotationRule;

    rotationRule.namingScheme =
        RotatedSourceNamingScheme::
        CustomRegex;

    rotationRule.numericOrderDirection =
        RotatedSourceOrderDirection::
        Ascending;

    rotationRule.customRegularExpression =
        QStringLiteral(
            R"(archive-(\d+)\.log)"
            );

    rotationRule.customOrderCaptureGroup =
        1;

    rotationRule.customOrderValueType =
        RotatedSourceOrderValueType::
        Numeric;

    rotationRule.customOrderDirection =
        RotatedSourceOrderDirection::
        Ascending;

    rotationRule.customDateTimeFormat =
        QStringLiteral(
            "yyyy-MM-dd-HH-mm-ss"
            );

    workspace.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(
            workspace
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json
            );

    QVERIFY(
        document.isObject()
        );

    const QJsonObject root =
        document.object();

    QCOMPARE(
        root.value(
                QStringLiteral(
                    "schemaVersion"
                    )
                )
            .toInt(),
        WorkspacePersistenceState::
        CurrentSchemaVersion
        );

    const QJsonArray sessions =
        root.value(
                QStringLiteral(
                    "sessions"
                    )
                )
            .toArray();

    QCOMPARE(
        sessions.size(),
        1
        );

    const QJsonObject sessionObject =
        sessions
            .first()
            .toObject();

    /*
     * Schema 2 no longer writes the schema-v1 source
     * fields directly on the session object.
     */
    QVERIFY(
        !sessionObject.contains(
            QStringLiteral(
                "sourcePath"
                )
            )
        );

    QVERIFY(
        !sessionObject.contains(
            QStringLiteral(
                "importProfile"
                )
            )
        );

    QVERIFY(
        !sessionObject.contains(
            QStringLiteral(
                "sourceFamilyConfiguration"
                )
            )
        );

    QVERIFY(
        sessionObject
            .value(
                QStringLiteral(
                    "backing"
                    )
                )
            .isObject()
        );

    const QJsonObject backingObject =
        sessionObject
            .value(
                QStringLiteral(
                    "backing"
                    )
                )
            .toObject();

    QCOMPARE(
        backingObject
            .value(
                QStringLiteral(
                    "mode"
                    )
                )
            .toString(),
        QStringLiteral(
            "sourceBacked"
            )
        );

    QVERIFY(
        backingObject
            .value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .isObject()
        );

    const QJsonObject externalSourceObject =
        backingObject
            .value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .toObject();

    QCOMPARE(
        externalSourceObject
            .value(
                QStringLiteral(
                    "sourcePath"
                    )
                )
            .toString(),
        QStringLiteral(
            "/logs/current.log"
            )
        );

    QVERIFY(
        externalSourceObject
            .value(
                QStringLiteral(
                    "sourceFamilyConfiguration"
                    )
                )
            .isObject()
        );

    const QJsonObject sourceFamilyObject =
        externalSourceObject
            .value(
                QStringLiteral(
                    "sourceFamilyConfiguration"
                    )
                )
            .toObject();

    QVERIFY(
        sourceFamilyObject
            .value(
                QStringLiteral(
                    "includeRotatedSources"
                    )
                )
            .toBool()
        );

    /*
     * Physical rotated source members are runtime
     * discovery results and must never be persisted.
     */
    QVERIFY(
        !sourceFamilyObject.contains(
            QStringLiteral(
                "rotatedSourcePaths"
                )
            )
        );

    QVERIFY(
        sourceFamilyObject
            .value(
                QStringLiteral(
                    "rotationRule"
                    )
                )
            .isObject()
        );

    const QJsonObject rotationRuleObject =
        sourceFamilyObject
            .value(
                QStringLiteral(
                    "rotationRule"
                    )
                )
            .toObject();

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "namingScheme"
                    )
                )
            .toString(),
        QStringLiteral(
            "customRegex"
            )
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "numericOrderDirection"
                    )
                )
            .toString(),
        QStringLiteral(
            "ascending"
            )
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "customRegularExpression"
                    )
                )
            .toString(),
        rotationRule.customRegularExpression
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "customOrderCaptureGroup"
                    )
                )
            .toInt(),
        1
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "customOrderValueType"
                    )
                )
            .toString(),
        QStringLiteral(
            "numeric"
            )
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "customOrderDirection"
                    )
                )
            .toString(),
        QStringLiteral(
            "ascending"
            )
        );

    QCOMPARE(
        rotationRuleObject
            .value(
                QStringLiteral(
                    "customDateTimeFormat"
                    )
                )
            .toString(),
        QStringLiteral(
            "yyyy-MM-dd-HH-mm-ss"
            )
        );

    /*
     * Round-trip the schema-v2 document and verify the
     * normalized backing model.
     */
    const WorkspaceDeserializationResult
        result =
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
        result.workspace.has_value()
        );

    QCOMPARE(
        result.workspace->schemaVersion,
        WorkspacePersistenceState::
        CurrentSchemaVersion
        );

    QCOMPARE(
        result.workspace->sessions.size(),
        1
        );

    const PersistedInvestigationSession
        &restoredSession =
        result
            .workspace
            ->sessions
            .first();

    QVERIFY(
        restoredSession.backing.mode
        == PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        restoredSession
            .backing
            .externalSourceBinding
            .has_value()
        );

    QVERIFY(
        restoredSession
            .backing
            .sourceImportProfile
            .has_value()
        );

    const PersistedInvestigationExternalSourceBinding
        &restoredExternalSource =
        *restoredSession
             .backing
             .externalSourceBinding;

    QCOMPARE(
        restoredExternalSource.sourcePath,
        QStringLiteral(
            "/logs/current.log"
            )
        );

    QCOMPARE(
        restoredExternalSource.sourceGeneration,
        quint64(0)
        );

    QVERIFY(
        !restoredExternalSource
             .sourceIdentity
             .has_value()
        );

    const SourceFamilyConfiguration
        &restored =
        restoredExternalSource
            .sourceFamilyConfiguration;

    QVERIFY(
        restored.includeRotatedSources
        );

    QVERIFY(
        restored.rotatedSourcePaths.isEmpty()
        );

    QVERIFY(
        restored.rotationRule.namingScheme
        ==
        RotatedSourceNamingScheme::
        CustomRegex
        );

    QVERIFY(
        restored
            .rotationRule
            .numericOrderDirection
        ==
        RotatedSourceOrderDirection::
        Ascending
        );

    QCOMPARE(
        restored
            .rotationRule
            .customRegularExpression,
        rotationRule
            .customRegularExpression
        );

    QCOMPARE(
        restored
            .rotationRule
            .customOrderCaptureGroup,
        1
        );

    QVERIFY(
        restored
            .rotationRule
            .customOrderValueType
        ==
        RotatedSourceOrderValueType::
        Numeric
        );

    QVERIFY(
        restored
            .rotationRule
            .customOrderDirection
        ==
        RotatedSourceOrderDirection::
        Ascending
        );

    QCOMPARE(
        restored
            .rotationRule
            .customDateTimeFormat,
        QStringLiteral(
            "yyyy-MM-dd-HH-mm-ss"
            )
        );

    QCOMPARE(
        restoredSession
            .backing
            .sourceImportProfile
            ->importerId,
        QStringLiteral(
            "regex-text"
            )
        );

    /*
     * Transitional compatibility mirrors are also
     * populated for the current workspace-open path.
     */
    QCOMPARE(
        restoredSession.sourcePath,
        QStringLiteral(
            "/logs/current.log"
            )
        );

    QCOMPARE(
        restoredSession.importProfile.importerId,
        QStringLiteral(
            "regex-text"
            )
        );

    QVERIFY(
        restoredSession
            .sourceFamilyConfiguration
            .includeRotatedSources
        );
}

void WorkspaceSerializationTests::
    missingSourceFamilyConfigurationUsesDefaults()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Legacy Profile");

    profile.importerId =
        QStringLiteral("json-lines");

    const ImportProfileSerializer
        profileSerializer;

    const QJsonDocument profileDocument =
        QJsonDocument::fromJson(
            profileSerializer.serialize(
                profile
                )
            );

    QJsonObject sessionObject;

    sessionObject.insert(
        QStringLiteral("sessionId"),
        QStringLiteral(
            "legacy-source-family-session"
            )
        );

    sessionObject.insert(
        QStringLiteral("sourcePath"),
        QStringLiteral("legacy.jsonl")
        );

    sessionObject.insert(
        QStringLiteral("importProfile"),
        profileDocument.object()
        );

    QJsonArray sessions;

    sessions.append(
        sessionObject
        );

    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        1
        );

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const WorkspaceDeserializationResult
        result =
        WorkspaceSerializer().deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.workspace.has_value()
        );

    QCOMPARE(
        result.workspace->schemaVersion,
        WorkspacePersistenceState::
        CurrentSchemaVersion
        );

    const PersistedInvestigationSession
        &restored =
        result
            .workspace
            ->sessions
            .first();

    QVERIFY(
        restored.backing.mode
        == PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            .has_value()
        );

    QCOMPARE(
        restored
            .backing
            .externalSourceBinding
            ->sourceGeneration,
        quint64(0)
        );

    QVERIFY(
        !restored
             .backing
             .externalSourceBinding
             ->sourceIdentity
             .has_value()
        );

    QVERIFY(
        !restored
             .backing
             .externalSourceBinding
             ->sourceFamilyConfiguration
             .includeRotatedSources
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            ->sourceFamilyConfiguration
            .rotationRule
            .namingScheme
        == RotatedSourceNamingScheme::
        Disabled
        );

    QVERIFY(
        restored
            .backing
            .sourceImportProfile
            .has_value()
        );

    /*
     * Compatibility mirrors are populated too.
     */
    QCOMPARE(
        restored.sourcePath,
        QStringLiteral("legacy.jsonl")
        );

    QCOMPARE(
        restored.importProfile.importerId,
        QStringLiteral("json-lines")
        );
}

void WorkspaceSerializationTests::
    invalidSourceFamilyConfigurationIsRejected()
{
    WorkspacePersistenceState workspace;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral(
            "invalid-source-family-session"
            );

    session.sourcePath =
        QStringLiteral("current.log");

    session.importProfile.name =
        QStringLiteral("Test Profile");

    session.importProfile.importerId =
        QStringLiteral("regex-text");

    workspace.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray validJson =
        serializer.serialize(
            workspace
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            validJson
            );

    QJsonObject root =
        document.object();

    QJsonArray sessions =
        root.value(
                QStringLiteral("sessions")
                )
            .toArray();

    QJsonObject sessionObject =
        sessions.first().toObject();

    QJsonObject backingObject =
        sessionObject
            .value(
                QStringLiteral("backing")
                )
            .toObject();

    QJsonObject externalSourceObject =
        backingObject
            .value(
                QStringLiteral(
                    "externalSourceBinding"
                    )
                )
            .toObject();

    QJsonObject sourceFamilyObject =
        externalSourceObject
            .value(
                QStringLiteral(
                    "sourceFamilyConfiguration"
                    )
                )
            .toObject();

    QJsonObject rotationRuleObject =
        sourceFamilyObject
            .value(
                QStringLiteral("rotationRule")
                )
            .toObject();

    rotationRuleObject.insert(
        QStringLiteral("namingScheme"),
        QStringLiteral(
            "definitely-not-a-scheme"
            )
        );

    sourceFamilyObject.insert(
        QStringLiteral("rotationRule"),
        rotationRuleObject
        );

    externalSourceObject.insert(
        QStringLiteral(
            "sourceFamilyConfiguration"
            ),
        sourceFamilyObject
        );

    backingObject.insert(
        QStringLiteral(
            "externalSourceBinding"
            ),
        externalSourceObject
        );

    sessionObject.insert(
        QStringLiteral("backing"),
        backingObject
        );

    sessions[0] =
        sessionObject;

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SESSION_BACKING"
            )
        );
}

void WorkspaceSerializationTests::
    comparisonRoundTripsThroughWorkspaceJson()
{
    WorkspacePersistenceState original;

    PersistedInvestigationComparison
        comparison;

    comparison.comparisonId =
        QStringLiteral("comparison-123");

    comparison.baselineSource.sessionId =
        QStringLiteral("baseline-session");

    comparison.baselineSource.sourcePath =
        QStringLiteral(
            "/logs/baseline.jsonl"
            );

    comparison.baselineSource.sourceName =
        QStringLiteral(
            "baseline.jsonl"
            );

    comparison.baselineSource.sourceSizeBytes =
        12345;

    comparison.baselineSource
        .sourceLastModified =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:00:00Z"
                ),
            Qt::ISODate
            );

    comparison.baselineSource.importedAtUtc =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-27T12:05:00Z"
                ),
            Qt::ISODate
            );

    comparison.comparisonSource.sessionId =
        QStringLiteral(
            "comparison-session"
            );

    comparison.comparisonSource.sourcePath =
        QStringLiteral(
            "/logs/comparison.jsonl"
            );

    comparison.comparisonSource.sourceName =
        QStringLiteral(
            "comparison.jsonl"
            );

    comparison.comparisonSource
        .sourceSizeBytes =
        23456;

    comparison.comparisonSource
        .sourceLastModified =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:00:00Z"
                ),
            Qt::ISODate
            );

    comparison.comparisonSource.importedAtUtc =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-28T12:05:00Z"
                ),
            Qt::ISODate
            );

    comparison.analysis.totalRecords
        .baselineCount =
        100;

    comparison.analysis.totalRecords
        .comparisonCount =
        135;

    InvestigationSeverityDifference
        warningDifference;

    warningDifference.severity =
        RecordSeverity::Warning;

    warningDifference.baselineCount =
        4;

    warningDifference.comparisonCount =
        11;

    comparison.analysis.severity
        .differences
        .append(
            warningDifference
            );

    original.comparisons.append(
        comparison
        );

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(
            original
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json
            );

    QVERIFY(document.isObject());

    const QJsonObject root =
        document.object();

    QVERIFY(
        root.value(
                QStringLiteral(
                    "comparisons"
                    )
                ).isArray()
        );

    QCOMPARE(
        root.value(
                QStringLiteral(
                    "comparisons"
                    )
                )
            .toArray()
            .size(),
        1
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.workspace.has_value());

    const WorkspacePersistenceState
        &restored =
        *result.workspace;

    QCOMPARE(
        restored.comparisons.size(),
        1
        );

    const PersistedInvestigationComparison
        &restoredComparison =
        restored.comparisons.first();

    QCOMPARE(
        restoredComparison.comparisonId,
        QStringLiteral(
            "comparison-123"
            )
        );

    /*
     * Baseline -> Comparison orientation must
     * survive workspace persistence unchanged.
     */
    QCOMPARE(
        restoredComparison
            .baselineSource
            .sessionId,
        QStringLiteral(
            "baseline-session"
            )
        );

    QCOMPARE(
        restoredComparison
            .comparisonSource
            .sessionId,
        QStringLiteral(
            "comparison-session"
            )
        );

    QCOMPARE(
        restoredComparison
            .baselineSource
            .sourcePath,
        QStringLiteral(
            "/logs/baseline.jsonl"
            )
        );

    QCOMPARE(
        restoredComparison
            .comparisonSource
            .sourcePath,
        QStringLiteral(
            "/logs/comparison.jsonl"
            )
        );

    QCOMPARE(
        restoredComparison
            .analysis
            .totalRecords
            .baselineCount,
        100
        );

    QCOMPARE(
        restoredComparison
            .analysis
            .totalRecords
            .comparisonCount,
        135
        );

    QCOMPARE(
        restoredComparison
            .analysis
            .severity
            .differences
            .size(),
        1
        );

    QVERIFY(
        restoredComparison
            .analysis
            .severity
            .differences
            .first()
            .severity
        == RecordSeverity::Warning
        );

    QCOMPARE(
        restoredComparison
            .analysis
            .severity
            .differences
            .first()
            .comparisonCount,
        11
        );
}

void WorkspaceSerializationTests::
    malformedJsonIsRejected()
{
    const WorkspaceDeserializationResult
        result =
        WorkspaceSerializer().deserialize(
            QByteArrayLiteral(
                "{ definitely not json"
                )
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral("INVALID_JSON")
        );
}

void WorkspaceSerializationTests::
    unsupportedSchemaVersionIsRejected()
{
    const QByteArray json =
        QByteArrayLiteral(
            R"({
                "schemaVersion": 999,
                "sessions": []
            })"
            );

    const WorkspaceDeserializationResult
        result =
        WorkspaceSerializer().deserialize(
            json
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "UNSUPPORTED_SCHEMA_VERSION"
            )
        );
}

void WorkspaceSerializationTests::
    invalidSessionIsRejected()
{
    const QByteArray json =
        QByteArrayLiteral(
            R"({
                "schemaVersion": 1,
                "sessions": [
                    {
                        "sourcePath": "session.jsonl",
                        "importProfile": {}
                    }
                ]
            })"
            );

    const WorkspaceDeserializationResult
        result =
        WorkspaceSerializer().deserialize(
            json
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_SESSION_ID"
            )
        );
}

void WorkspaceSerializationTests::
    invalidFindingStatusIsRejected()
{
    WorkspacePersistenceState workspace;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral("session-1");

    session.sourcePath =
        QStringLiteral("source.jsonl");

    session.importProfile.name =
        QStringLiteral("Test Profile");

    session.importProfile.importerId =
        QStringLiteral("json-lines");

    PersistedInvestigationRecordState
        recordState;

    recordState.recordId =
        QStringLiteral("record-1");

    recordState.findingStatus =
        FindingStatus::Open;

    session.recordStates.append(
        recordState
        );

    workspace.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray validJson =
        serializer.serialize(
            workspace
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            validJson
            );

    QJsonObject root =
        document.object();

    QJsonArray sessions =
        root.value(
                QStringLiteral("sessions")
                ).toArray();

    QJsonObject sessionObject =
        sessions.at(0).toObject();

    QJsonArray recordStates =
        sessionObject.value(
                         QStringLiteral("recordStates")
                         ).toArray();

    QJsonObject stateObject =
        recordStates.at(0).toObject();

    stateObject.insert(
        QStringLiteral("findingStatus"),
        QStringLiteral("banana")
        );

    recordStates[0] =
        stateObject;

    sessionObject.insert(
        QStringLiteral("recordStates"),
        recordStates
        );

    sessions[0] =
        sessionObject;

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const QByteArray corruptedJson =
        QJsonDocument(root).toJson(
            QJsonDocument::Compact
            );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            corruptedJson
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_FINDING_STATUS"
            )
        );
}

void WorkspaceSerializationTests::
    invalidComparisonsCollectionIsRejected()
{
    WorkspacePersistenceState workspace;

    const WorkspaceSerializer serializer;

    const QByteArray validJson =
        serializer.serialize(
            workspace
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            validJson
            );

    QJsonObject root =
        document.object();

    root.insert(
        QStringLiteral("comparisons"),
        QStringLiteral(
            "not-an-array"
            )
        );

    const QByteArray invalidJson =
        QJsonDocument(root).toJson(
            QJsonDocument::Compact
            );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            invalidJson
            );

    QVERIFY(!result.isSuccess());

    QCOMPARE(
        result.errorCode,
        QStringLiteral(
            "INVALID_COMPARISONS"
            )
        );
}

void WorkspaceSerializationTests::
    presentationStateRoundTripsThroughJson()
{
    WorkspacePersistenceState original;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral("presentation-session");

    session.sourcePath =
        QStringLiteral("/logs/presentation.jsonl");

    session.importProfile.name =
        QStringLiteral("Presentation Profile");

    session.importProfile.importerId =
        QStringLiteral("json-lines");

    InvestigationSessionPresentationState
        &state =
        session.presentationState;

    state.eventTable.selectedRecordId =
        QStringLiteral("record-42");

    state.eventTable.columnWidths = {
        120,
        180,
        240
    };

    state.eventTable.manuallyResizedColumns = {
        1
    };

    state.eventTable.columnWidthScaleFactor = 1.25;

    state.eventTable.sortColumn =
        2;

    state.eventTable.sortOrder =
        Qt::DescendingOrder;

    state.eventTable.scroll.horizontalValue =
        17;

    state.eventTable.scroll.verticalValue =
        31;

    state.eventDetailScroll.verticalValue =
        9;

    state.timeline.intervalMilliseconds =
        1000;

    state.timeline.breakdown =
        InvestigationTimelineBreakdown::Subsystem;

    state.timeline.subsystemTrendLimit =
        7;

    state.timeline.horizontalScrollValue =
        24;

    state.review.selectedTab =
        InvestigationReviewTab::Analytics;

    state.review.issueSummaryTable.currentRow =
        3;

    state.review.issueSummaryTable.currentColumn =
        2;

    state.review.findingsTable.currentRow =
        8;

    state.review.findingsTable.currentColumn =
        3;

    state.review.analytics.selectedTab =
        InvestigationAnalyticsTab::Bursts;

    state.review.analytics.overviewSplitterSizes = {
        250,
        350
    };

    state.review.analytics.eventCodeTable.currentRow =
        6;

    state.review.analytics.entityTable.currentRow =
        4;

    state.review.analytics.burstSplitterSizes = {
        400,
        300
    };

    state.review.analytics
        .selectedBurstStartTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:00:50.000Z"
                ),
            Qt::ISODateWithMs
            );

    state.review.analytics
        .selectedBurstEndTimestamp =
        QDateTime::fromString(
            QStringLiteral(
                "2026-08-29T12:01:09.000Z"
                ),
            Qt::ISODateWithMs
            );

    state.review.analytics.burstTable.currentRow =
        1;

    state.review.analytics.burstTable.currentColumn =
        2;

    state.review.analytics
        .burstDetailScroll.verticalValue =
        14;

    state.mainSplitterSizes = {
        180,
        360,
        260
    };

    state.bottomSplitterSizes = {
        520,
        420
    };

    state.burstTimingMode =
        InvestigationBurstTimingMode::Manual;

    state.burstDetectionSettings
        .windowMilliseconds =
        4000;

    state.burstDetectionSettings
        .elevatedEventThreshold =
        3;

    state.burstDetectionSettings
        .errorCriticalThreshold =
        100;

    state.burstDetectionSettings
        .mergeGapMilliseconds =
        0;

    original.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(
            original
            );

    /*
     * Verify that presentation state is an explicit
     * part of the human-readable workspace format,
     * not merely reconstructed from defaults.
     */
    const QJsonDocument document =
        QJsonDocument::fromJson(
            json
            );

    QVERIFY(document.isObject());

    const QJsonArray sessions =
        document
            .object()
            .value(
                QStringLiteral("sessions")
                )
            .toArray();

    QCOMPARE(
        sessions.size(),
        1
        );

    QVERIFY(
        sessions
            .first()
            .toObject()
            .value(
                QStringLiteral(
                    "presentationState"
                    )
                )
            .isObject()
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(result.isSuccess());

    const InvestigationSessionPresentationState
        &restored =
        result
            .workspace
            ->sessions
            .first()
            .presentationState;

    QCOMPARE(
        restored.eventTable.selectedRecordId,
        state.eventTable.selectedRecordId
        );

    QCOMPARE(
        restored.eventTable.columnWidths,
        state.eventTable.columnWidths
        );

    QCOMPARE(
        restored.eventTable.manuallyResizedColumns,
        state.eventTable.manuallyResizedColumns
        );

    QCOMPARE(
        restored.eventTable.columnWidthScaleFactor,
        state.eventTable.columnWidthScaleFactor
        );

    QCOMPARE(
        restored.eventTable.sortColumn,
        state.eventTable.sortColumn
        );

    QCOMPARE(
        restored.eventTable.sortOrder,
        state.eventTable.sortOrder
        );

    QCOMPARE(
        restored.eventTable.scroll.verticalValue,
        state.eventTable.scroll.verticalValue
        );

    QCOMPARE(
        restored.eventDetailScroll.verticalValue,
        state.eventDetailScroll.verticalValue
        );

    QCOMPARE(
        restored.timeline.intervalMilliseconds,
        state.timeline.intervalMilliseconds
        );

    QCOMPARE(
        restored.timeline.breakdown,
        state.timeline.breakdown
        );

    QCOMPARE(
        restored.timeline.subsystemTrendLimit,
        state.timeline.subsystemTrendLimit
        );

    QCOMPARE(
        restored.timeline.horizontalScrollValue,
        state.timeline.horizontalScrollValue
        );

    QCOMPARE(
        restored.review.selectedTab,
        state.review.selectedTab
        );

    QCOMPARE(
        restored.review.issueSummaryTable.currentRow,
        state.review.issueSummaryTable.currentRow
        );

    QCOMPARE(
        restored.review.findingsTable.currentRow,
        state.review.findingsTable.currentRow
        );

    QCOMPARE(
        restored.review.analytics.selectedTab,
        state.review.analytics.selectedTab
        );

    QCOMPARE(
        restored.review.analytics
            .overviewSplitterSizes,
        state.review.analytics
            .overviewSplitterSizes
        );

    QCOMPARE(
        restored.review.analytics
            .burstSplitterSizes,
        state.review.analytics
            .burstSplitterSizes
        );

    QCOMPARE(
        restored.review.analytics
            .selectedBurstStartTimestamp,
        state.review.analytics
            .selectedBurstStartTimestamp
        );

    QCOMPARE(
        restored.review.analytics
            .selectedBurstEndTimestamp,
        state.review.analytics
            .selectedBurstEndTimestamp
        );

    QCOMPARE(
        restored.review.analytics
            .burstDetailScroll.verticalValue,
        state.review.analytics
            .burstDetailScroll.verticalValue
        );

    QCOMPARE(
        restored.mainSplitterSizes,
        state.mainSplitterSizes
        );

    QCOMPARE(
        restored.bottomSplitterSizes,
        state.bottomSplitterSizes
        );

    QCOMPARE(
        restored.burstTimingMode,
        state.burstTimingMode
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .windowMilliseconds,
        state.burstDetectionSettings
            .windowMilliseconds
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .elevatedEventThreshold,
        state.burstDetectionSettings
            .elevatedEventThreshold
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .errorCriticalThreshold,
        state.burstDetectionSettings
            .errorCriticalThreshold
        );

    QCOMPARE(
        restored.burstDetectionSettings
            .mergeGapMilliseconds,
        state.burstDetectionSettings
            .mergeGapMilliseconds
        );
}

void WorkspaceSerializationTests::
    missingPresentationStateUsesDefaults()
{
    WorkspacePersistenceState workspace;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral("legacy-session");

    session.sourcePath =
        QStringLiteral("legacy.jsonl");

    session.importProfile.name =
        QStringLiteral("Legacy Profile");

    session.importProfile.importerId =
        QStringLiteral("json-lines");

    workspace.sessions.append(
        session
        );

    const WorkspaceSerializer serializer;

    const QByteArray serialized =
        serializer.serialize(
            workspace
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            serialized
            );

    QJsonObject root =
        document.object();

    QJsonArray sessions =
        root.value(
                QStringLiteral("sessions")
                )
            .toArray();

    QJsonObject sessionObject =
        sessions.first().toObject();

    sessionObject.remove(
        QStringLiteral("presentationState")
        );

    sessions[0] =
        sessionObject;

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(result.isSuccess());

    const InvestigationSessionPresentationState
        &restored =
        result
            .workspace
            ->sessions
            .first()
            .presentationState;

    QCOMPARE(
        restored.eventTable.sortColumn,
        -1
        );

    QCOMPARE(
        restored.timeline.intervalMilliseconds,
        qint64(0)
        );

    QCOMPARE(
        restored.review.selectedTab,
        InvestigationReviewTab::IssueSummary
        );

    QCOMPARE(
        restored.burstTimingMode,
        InvestigationBurstTimingMode::Auto
        );

    QVERIFY(
        restored.mainSplitterSizes.isEmpty()
        );

    QVERIFY(
        restored.bottomSplitterSizes.isEmpty()
        );
}

void WorkspaceSerializationTests::
    documentLayoutRoundTripsThroughJson()
{
    WorkspacePersistenceState original;

    original.documentLayout
        .dockedGroup
        .documentIds = {
        QStringLiteral("session-1"),
        QStringLiteral("session-2")
    };

    original.documentLayout
        .dockedGroup
        .currentDocumentId =
        QStringLiteral("session-2");

    original.documentLayout
        .mainWindowGeometry =
        QRect(
            80,
            60,
            1280,
            820
            );

    original.documentLayout
        .mainWindowMaximized =
        true;

    DetachedWorkspaceWindowLayoutState detached;

    detached.group.documentIds = {
        QStringLiteral("session-3"),
        QStringLiteral("comparison-1")
    };

    detached.group.currentDocumentId =
        QStringLiteral("comparison-1");

    detached.geometry =
        QRect(
            120,
            140,
            720,
            480
            );

    detached.maximized = false;

    original.documentLayout
        .detachedWindows
        .append(
            detached
            );

    original.documentLayout.activeDocumentId =
        QStringLiteral("comparison-1");

    const WorkspaceSerializer serializer;

    const QByteArray json =
        serializer.serialize(
            original
            );

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json
            );

    QVERIFY(
        document.isObject()
        );

    const QJsonObject root =
        document.object();

    QVERIFY(
        root.value(
                QStringLiteral(
                    "documentLayout"
                    )
                )
            .isObject()
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            json
            );

    QVERIFY(
        result.isSuccess()
        );

    QVERIFY(
        result.workspace.has_value()
        );

    const WorkspaceDocumentLayoutState
        &restored =
        result.workspace
            ->documentLayout;

    QCOMPARE(
        restored.mainWindowGeometry,
        original
            .documentLayout
            .mainWindowGeometry
        );

    QCOMPARE(
        restored.mainWindowMaximized,
        original
            .documentLayout
            .mainWindowMaximized
        );

    QCOMPARE(
        restored
            .dockedGroup
            .documentIds,
        original
            .documentLayout
            .dockedGroup
            .documentIds
        );

    QCOMPARE(
        restored
            .dockedGroup
            .currentDocumentId,
        original
            .documentLayout
            .dockedGroup
            .currentDocumentId
        );

    QCOMPARE(
        restored.detachedWindows.size(),
        1
        );

    const DetachedWorkspaceWindowLayoutState
        &restoredDetached =
        restored
            .detachedWindows
            .first();

    QCOMPARE(
        restoredDetached
            .group
            .documentIds,
        detached
            .group
            .documentIds
        );

    QCOMPARE(
        restoredDetached
            .group
            .currentDocumentId,
        detached
            .group
            .currentDocumentId
        );

    QCOMPARE(
        restoredDetached.geometry,
        detached.geometry
        );

    QCOMPARE(
        restoredDetached.maximized,
        detached.maximized
        );

    QCOMPARE(
        restored.activeDocumentId,
        original
            .documentLayout
            .activeDocumentId
        );
}

void WorkspaceSerializationTests::
    missingDocumentLayoutUsesDefaults()
{
    WorkspacePersistenceState original;

    original.documentLayout
        .dockedGroup
        .documentIds = {
            QStringLiteral("session-1")
        };

    original.documentLayout
        .dockedGroup
        .currentDocumentId =
        QStringLiteral("session-1");

    DetachedWorkspaceWindowLayoutState detached;

    detached.group.documentIds = {
        QStringLiteral("comparison-1")
    };

    detached.group.currentDocumentId =
        QStringLiteral("comparison-1");

    detached.geometry =
        QRect(
            100,
            120,
            640,
            480
            );

    original.documentLayout
        .detachedWindows
        .append(
            detached
            );

    original.documentLayout.activeDocumentId =
        QStringLiteral("comparison-1");

    const WorkspaceSerializer serializer;

    const QByteArray serialized =
        serializer.serialize(
            original
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            serialized
            );

    QVERIFY(
        document.isObject()
        );

    QJsonObject root =
        document.object();

    /*
     * Simulate a schema-version-1 workspace written
     * before document-layout persistence existed.
     */
    root.remove(
        QStringLiteral("documentLayout")
        );

    const WorkspaceDeserializationResult
        result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(
        result.isSuccess()
        );

    QVERIFY(
        result.workspace.has_value()
        );

    const WorkspacePersistenceState
        &restored =
        *result.workspace;

    QVERIFY(
        restored.documentLayout
            .dockedGroup
            .documentIds
            .isEmpty()
        );

    QVERIFY(
        restored.documentLayout
            .dockedGroup
            .currentDocumentId
            .isEmpty()
        );

    QVERIFY(
        restored.documentLayout
            .detachedWindows
            .isEmpty()
        );

    QVERIFY(
        restored.documentLayout
            .activeDocumentId
            .isEmpty()
        );
}

void WorkspaceSerializationTests::
    missingMainWindowStateUsesDefaults()
{
    WorkspacePersistenceState original;

    original.documentLayout
        .mainWindowGeometry =
        QRect(
            80,
            60,
            1280,
            820
            );

    original.documentLayout
        .mainWindowMaximized =
        true;

    const WorkspaceSerializer serializer;

    const QByteArray serialized =
        serializer.serialize(
            original
            );

    QJsonDocument document =
        QJsonDocument::fromJson(
            serialized
            );

    QVERIFY(
        document.isObject()
        );

    QJsonObject root =
        document.object();

    QJsonObject documentLayout =
        root.value(
                QStringLiteral(
                    "documentLayout"
                    )
                )
            .toObject();

    /*
     * Simulate a schema-version-1 workspace written
     * after document-layout persistence existed but
     * before primary-window geometry was added.
     */
    documentLayout.remove(
        QStringLiteral(
            "mainWindowGeometry"
            )
        );

    documentLayout.remove(
        QStringLiteral(
            "mainWindowMaximized"
            )
        );

    root.insert(
        QStringLiteral("documentLayout"),
        documentLayout
        );

    const WorkspaceDeserializationResult result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(
        result.isSuccess()
        );

    QVERIFY(
        result.workspace.has_value()
        );

    QVERIFY(
        !result.workspace
             ->documentLayout
             .mainWindowGeometry
             .isValid()
        );

    QVERIFY(
        !result.workspace
             ->documentLayout
             .mainWindowMaximized
        );
}

void WorkspaceSerializationTests::
    missingNumericOrderDirectionUsesDescendingDefault()
{
    WorkspacePersistenceState workspace;

    PersistedInvestigationSession session;

    session.sessionId =
        QStringLiteral("legacy-numeric-order");

    session.sourcePath =
        QStringLiteral("/logs/service.log");

    session.importProfile.name =
        QStringLiteral("Legacy Profile");

    session.importProfile.importerId =
        QStringLiteral("json-lines");

    session
        .sourceFamilyConfiguration
        .includeRotatedSources =
        true;

    session
        .sourceFamilyConfiguration
        .rotationRule
        .namingScheme =
        RotatedSourceNamingScheme::
        NumericSuffix;

    workspace.sessions.append(session);

    const WorkspaceSerializer serializer;

    QJsonDocument document =
        QJsonDocument::fromJson(
            serializer.serialize(workspace)
            );

    QJsonObject root =
        document.object();

    QJsonArray sessions =
        root.value(
                QStringLiteral("sessions")
                )
            .toArray();

    QJsonObject sessionObject =
        sessions.at(0).toObject();

    QJsonObject familyObject =
        sessionObject
            .value(
                QStringLiteral(
                    "sourceFamilyConfiguration"
                    )
                )
            .toObject();

    QJsonObject ruleObject =
        familyObject
            .value(
                QStringLiteral("rotationRule")
                )
            .toObject();

    ruleObject.remove(
        QStringLiteral(
            "numericOrderDirection"
            )
        );

    familyObject.insert(
        QStringLiteral("rotationRule"),
        ruleObject
        );

    sessionObject.insert(
        QStringLiteral(
            "sourceFamilyConfiguration"
            ),
        familyObject
        );

    sessions[0] =
        sessionObject;

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const WorkspaceDeserializationResult result =
        serializer.deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY(result.isSuccess());
    QVERIFY(result.workspace.has_value());

    QCOMPARE(
        result
            .workspace
            ->sessions
            .first()
            .sourceFamilyConfiguration
            .rotationRule
            .numericOrderDirection,
        RotatedSourceOrderDirection::
        Descending
        );
}

void WorkspaceSerializationTests::
    schemaOneSessionMigratesToSourceBackedV2()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral("Legacy Profile");

    profile.importerId =
        QStringLiteral("json-lines");

    const ImportProfileSerializer
        profileSerializer;

    const QJsonDocument profileDocument =
        QJsonDocument::fromJson(
            profileSerializer.serialize(
                profile
                )
            );

    QJsonObject rotationRule;

    rotationRule.insert(
        QStringLiteral("namingScheme"),
        QStringLiteral("numericSuffix")
        );

    rotationRule.insert(
        QStringLiteral(
            "numericOrderDirection"
            ),
        QStringLiteral("descending")
        );

    rotationRule.insert(
        QStringLiteral(
            "customRegularExpression"
            ),
        QString()
        );

    rotationRule.insert(
        QStringLiteral(
            "customOrderCaptureGroup"
            ),
        1
        );

    rotationRule.insert(
        QStringLiteral(
            "customOrderValueType"
            ),
        QStringLiteral("lexicographic")
        );

    rotationRule.insert(
        QStringLiteral(
            "customOrderDirection"
            ),
        QStringLiteral("ascending")
        );

    rotationRule.insert(
        QStringLiteral(
            "customDateTimeFormat"
            ),
        QString()
        );

    QJsonObject sourceFamily;

    sourceFamily.insert(
        QStringLiteral(
            "includeRotatedSources"
            ),
        true
        );

    sourceFamily.insert(
        QStringLiteral("rotationRule"),
        rotationRule
        );

    QJsonObject session;

    session.insert(
        QStringLiteral("sessionId"),
        QStringLiteral("legacy-session")
        );

    session.insert(
        QStringLiteral("sourcePath"),
        QStringLiteral(
            "C:/logs/current.jsonl"
            )
        );

    session.insert(
        QStringLiteral(
            "sourceFamilyConfiguration"
            ),
        sourceFamily
        );

    session.insert(
        QStringLiteral("importProfile"),
        profileDocument.object()
        );

    QJsonArray sessions;
    sessions.append(session);

    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        1
        );

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    const auto result =
        WorkspaceSerializer().deserialize(
            QJsonDocument(root).toJson(
                QJsonDocument::Compact
                )
            );

    QVERIFY2(
        result.isSuccess(),
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.workspace->schemaVersion,
        2
        );

    const PersistedInvestigationSession
        &restored =
        result.workspace
            ->sessions
            .first();

    QVERIFY(
        restored.backing.mode
        == PersistedInvestigationSessionBackingMode::
        SourceBacked
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            .has_value()
        );

    QVERIFY(
        restored
            .backing
            .sourceImportProfile
            .has_value()
        );

    QCOMPARE(
        restored
            .backing
            .externalSourceBinding
            ->sourcePath,
        QStringLiteral(
            "C:/logs/current.jsonl"
            )
        );

    QCOMPARE(
        restored
            .backing
            .externalSourceBinding
            ->sourceGeneration,
        quint64(0)
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            ->sourceFamilyConfiguration
            .includeRotatedSources
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            ->sourceFamilyConfiguration
            .rotationRule
            .namingScheme
        == RotatedSourceNamingScheme::
        NumericSuffix
        );

    QVERIFY(
        restored
            .backing
            .externalSourceBinding
            ->sourceFamilyConfiguration
            .rotatedSourcePaths
            .isEmpty()
        );

    QCOMPARE(
        restored
            .backing
            .sourceImportProfile
            ->importerId,
        QStringLiteral("json-lines")
        );
}


void WorkspaceSerializationTests::
    defaultPresentationStateOmitsUnsetColumnWidthScaleFactor()
{
    /*
     * A default presentation state has no recorded
     * column-width scale. The serializer must omit
     * the field rather than writing an invalid zero.
     */
    InvestigationSessionPresentationState defaultState;

    QCOMPARE(
        defaultState.eventTable.columnWidthScaleFactor,
        0.0
        );

    const InvestigationPresentationStateSerializer serializer;

    const QJsonObject json =
        serializer.serialize(defaultState);

    const QJsonObject eventTable =
        json.value(
                QStringLiteral("eventTable")
                ).toObject();

    QVERIFY(!eventTable.isEmpty());

    QVERIFY(
        !eventTable.contains(
            QStringLiteral("columnWidthScaleFactor")
            )
        );

    /*
     * Verify that the resulting presentation state
     * can also be deserialized successfully.
     */
    const PresentationStateDeserializationResult result =
        serializer.deserialize(json);

    QVERIFY2(
        result.isSuccess(),
        qPrintable(result.errorMessage)
        );

    QCOMPARE(
        result.state->eventTable.columnWidthScaleFactor,
        0.0
        );
}

QTEST_MAIN(WorkspaceSerializationTests)

#include "WorkspaceSerializationTests.moc"