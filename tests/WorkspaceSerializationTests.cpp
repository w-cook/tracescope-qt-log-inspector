#include <QtTest>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include "../src/workspace/WorkspaceSerialization.h"

class WorkspaceSerializationTests
    : public QObject
{
    Q_OBJECT

private slots:
    void serializationProducesVersionedJson();
    void sessionContextRoundTripsThroughJson();
    void malformedJsonIsRejected();
    void unsupportedSchemaVersionIsRejected();
    void invalidSessionIsRejected();
    void invalidFindingStatusIsRejected();
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

QTEST_MAIN(WorkspaceSerializationTests)

#include "WorkspaceSerializationTests.moc"