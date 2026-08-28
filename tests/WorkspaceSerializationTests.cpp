#include <QtTest>

#include <QJsonDocument>
#include <QJsonObject>

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

QTEST_MAIN(WorkspaceSerializationTests)

#include "WorkspaceSerializationTests.moc"