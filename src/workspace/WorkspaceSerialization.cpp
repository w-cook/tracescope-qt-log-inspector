#include "WorkspaceSerialization.h"

#include <cmath>
#include <limits>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "../importing/ImportProfileSerialization.h"

namespace
{
WorkspaceDeserializationResult failure(
    const QString &code,
    const QString &message
    )
{
    WorkspaceDeserializationResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

bool readRequiredString(
    const QJsonObject &object,
    const QString &key,
    QString &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isString()) {
        return false;
    }

    result = value.toString();

    return true;
}

std::optional<int> readSchemaVersion(
    const QJsonObject &root
    )
{
    const QJsonValue value =
        root.value(
            QStringLiteral("schemaVersion")
            );

    if (!value.isDouble()) {
        return std::nullopt;
    }

    const double number =
        value.toDouble();

    if (number <
            std::numeric_limits<int>::min()
        ||
        number >
            std::numeric_limits<int>::max()
        ||
        std::floor(number) != number) {
        return std::nullopt;
    }

    return static_cast<int>(number);
}
}

QByteArray WorkspaceSerializer::serialize(
    const WorkspacePersistenceState &workspace
    ) const
{
    QJsonObject root;

    root.insert(
        QStringLiteral("schemaVersion"),
        workspace.schemaVersion
        );

    QJsonArray sessions;

    const ImportProfileSerializer
        profileSerializer;

    for (const PersistedInvestigationSession &session
         : workspace.sessions) {
        QJsonObject sessionObject;

        sessionObject.insert(
            QStringLiteral("sessionId"),
            session.sessionId
            );

        sessionObject.insert(
            QStringLiteral("sourcePath"),
            session.sourcePath
            );

        const QByteArray profileJson =
            profileSerializer.serialize(
                session.importProfile
                );

        const QJsonDocument profileDocument =
            QJsonDocument::fromJson(
                profileJson
                );

        sessionObject.insert(
            QStringLiteral("importProfile"),
            profileDocument.object()
            );

        sessions.append(
            sessionObject
            );
    }

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    return QJsonDocument(root).toJson(
        QJsonDocument::Indented
        );
}

WorkspaceDeserializationResult
WorkspaceSerializer::deserialize(
    const QByteArray &json
    ) const
{
    QJsonParseError parseError;

    const QJsonDocument document =
        QJsonDocument::fromJson(
            json,
            &parseError
            );

    if (parseError.error !=
        QJsonParseError::NoError) {
        return failure(
            QStringLiteral("INVALID_JSON"),
            QStringLiteral(
                "The workspace is not valid JSON: %1"
                ).arg(
                    parseError.errorString()
                    )
            );
    }

    if (!document.isObject()) {
        return failure(
            QStringLiteral(
                "WORKSPACE_ROOT_NOT_OBJECT"
                ),
            QStringLiteral(
                "The workspace root must be a JSON object."
                )
            );
    }

    const QJsonObject root =
        document.object();

    const std::optional<int> schemaVersion =
        readSchemaVersion(root);

    if (!schemaVersion.has_value()) {
        return failure(
            QStringLiteral(
                "INVALID_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "The workspace schemaVersion must be an integer."
                )
            );
    }

    if (*schemaVersion !=
        WorkspacePersistenceState::
        CurrentSchemaVersion) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "Workspace schema version %1 is not supported."
                ).arg(*schemaVersion)
            );
    }

    const QJsonValue sessionsValue =
        root.value(
            QStringLiteral("sessions")
            );

    if (!sessionsValue.isArray()) {
        return failure(
            QStringLiteral(
                "INVALID_SESSIONS"
                ),
            QStringLiteral(
                "The workspace sessions field must be an array."
                )
            );
    }

    WorkspacePersistenceState workspace;

    workspace.schemaVersion =
        *schemaVersion;

    const ImportProfileSerializer
        profileSerializer;

    const QJsonArray sessions =
        sessionsValue.toArray();

    for (const QJsonValue &sessionValue
         : sessions) {
        if (!sessionValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_SESSION"
                    ),
                QStringLiteral(
                    "Each workspace session must be a JSON object."
                    )
                );
        }

        const QJsonObject sessionObject =
            sessionValue.toObject();

        PersistedInvestigationSession
            session;

        if (!readRequiredString(
                sessionObject,
                QStringLiteral("sessionId"),
                session.sessionId
                )
            ||
            session.sessionId.isEmpty()) {
            return failure(
                QStringLiteral(
                    "INVALID_SESSION_ID"
                    ),
                QStringLiteral(
                    "Each workspace session must have a non-empty sessionId."
                    )
                );
        }

        if (!readRequiredString(
                sessionObject,
                QStringLiteral("sourcePath"),
                session.sourcePath
                )
            ||
            session.sourcePath.isEmpty()) {
            return failure(
                QStringLiteral(
                    "INVALID_SOURCE_PATH"
                    ),
                QStringLiteral(
                    "Each workspace session must have a non-empty sourcePath."
                    )
                );
        }

        const QJsonValue profileValue =
            sessionObject.value(
                QStringLiteral("importProfile")
                );

        if (!profileValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_IMPORT_PROFILE"
                    ),
                QStringLiteral(
                    "Each workspace session must contain an importProfile object."
                    )
                );
        }

        const QByteArray profileJson =
            QJsonDocument(
                profileValue.toObject()
                ).toJson(
                    QJsonDocument::Compact
                    );

        const ProfileDeserializationResult
            profileResult =
            profileSerializer.deserialize(
                profileJson
                );

        if (!profileResult.isSuccess()) {
            return failure(
                QStringLiteral(
                    "INVALID_IMPORT_PROFILE"
                    ),
                QStringLiteral(
                    "A workspace session contains an invalid import profile: %1"
                    ).arg(
                        profileResult.errorMessage
                        )
                );
        }

        session.importProfile =
            *profileResult.profile;

        workspace.sessions.append(
            std::move(session)
            );
    }

    WorkspaceDeserializationResult result;

    result.workspace =
        std::move(workspace);

    return result;
}