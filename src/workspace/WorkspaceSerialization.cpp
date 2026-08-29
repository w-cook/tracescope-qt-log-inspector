#include "WorkspaceSerialization.h"

#include <cmath>
#include <limits>
#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

#include "InvestigationComparisonPersistenceSerialization.h"
#include "InvestigationPresentationStateSerialization.h"

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

QJsonArray stringListToJson(
    const QStringList &values
    )
{
    QJsonArray array;

    for (const QString &value : values) {
        array.append(value);
    }

    return array;
}

std::optional<QStringList> stringListFromJson(
    const QJsonValue &value
    )
{
    if (!value.isArray()) {
        return std::nullopt;
    }

    QStringList values;

    for (const QJsonValue &item
         : value.toArray()) {
        if (!item.isString()) {
            return std::nullopt;
        }

        values.append(
            item.toString()
            );
    }

    return values;
}

QString findingStatusToJson(
    FindingStatus status
    )
{
    switch (status) {
    case FindingStatus::None:
        return QStringLiteral("none");

    case FindingStatus::Open:
        return QStringLiteral("open");

    case FindingStatus::Resolved:
        return QStringLiteral("resolved");

    case FindingStatus::Dismissed:
        return QStringLiteral("dismissed");
    }

    return QStringLiteral("none");
}

std::optional<FindingStatus>
findingStatusFromJson(
    const QString &value
    )
{
    if (value == QStringLiteral("none")) {
        return FindingStatus::None;
    }

    if (value == QStringLiteral("open")) {
        return FindingStatus::Open;
    }

    if (value == QStringLiteral("resolved")) {
        return FindingStatus::Resolved;
    }

    if (value == QStringLiteral("dismissed")) {
        return FindingStatus::Dismissed;
    }

    return std::nullopt;
}

QJsonValue optionalDateTimeToJson(
    const std::optional<QDateTime> &value
    )
{
    if (!value.has_value()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    return value->toString(
        Qt::ISODateWithMs
        );
}

bool readOptionalDateTime(
    const QJsonObject &object,
    const QString &key,
    std::optional<QDateTime> &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (value.isUndefined()
        || value.isNull()) {
        result.reset();
        return true;
    }

    if (!value.isString()) {
        return false;
    }

    const QDateTime dateTime =
        QDateTime::fromString(
            value.toString(),
            Qt::ISODateWithMs
            );

    if (!dateTime.isValid()) {
        return false;
    }

    result = dateTime;

    return true;
}

QJsonObject customFieldFiltersToJson(
    const QMap<QString, QStringList>
        &filters
    )
{
    QJsonObject object;

    for (
        auto iterator = filters.constBegin();
        iterator != filters.constEnd();
        ++iterator
        ) {
        object.insert(
            iterator.key(),
            stringListToJson(
                iterator.value()
                )
            );
    }

    return object;
}

std::optional<
    QMap<QString, QStringList>>
customFieldFiltersFromJson(
    const QJsonValue &value
    )
{
    if (!value.isObject()) {
        return std::nullopt;
    }

    QMap<QString, QStringList> filters;

    const QJsonObject object =
        value.toObject();

    for (
        auto iterator = object.constBegin();
        iterator != object.constEnd();
        ++iterator
        ) {
        const std::optional<QStringList>
            values =
            stringListFromJson(
                iterator.value()
                );

        if (!values.has_value()) {
            return std::nullopt;
        }

        filters.insert(
            iterator.key(),
            *values
            );
    }

    return filters;
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

    const InvestigationPresentationStateSerializer
        presentationSerializer;

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

        QJsonArray recordStates;

        for (
            const PersistedInvestigationRecordState
                &recordState
            : session.recordStates
            ) {
            QJsonObject stateObject;

            stateObject.insert(
                QStringLiteral("recordId"),
                recordState.recordId
                );

            stateObject.insert(
                QStringLiteral("bookmarked"),
                recordState.bookmarked
                );

            stateObject.insert(
                QStringLiteral("note"),
                recordState.note
                );

            stateObject.insert(
                QStringLiteral("findingStatus"),
                findingStatusToJson(
                    recordState.findingStatus
                    )
                );

            recordStates.append(
                stateObject
                );
        }

        sessionObject.insert(
            QStringLiteral("recordStates"),
            recordStates
            );

        QJsonObject filterState;

        filterState.insert(
            QStringLiteral("severities"),
            stringListToJson(
                session.filterState.severities
                )
            );

        filterState.insert(
            QStringLiteral("subsystems"),
            stringListToJson(
                session.filterState.subsystems
                )
            );

        filterState.insert(
            QStringLiteral("searchText"),
            session.filterState.searchText
            );

        filterState.insert(
            QStringLiteral("eventCodes"),
            stringListToJson(
                session.filterState.eventCodes
                )
            );

        filterState.insert(
            QStringLiteral("entityIds"),
            stringListToJson(
                session.filterState.entityIds
                )
            );

        filterState.insert(
            QStringLiteral("startTime"),
            optionalDateTimeToJson(
                session.filterState.startTime
                )
            );

        filterState.insert(
            QStringLiteral("endTime"),
            optionalDateTimeToJson(
                session.filterState.endTime
                )
            );

        filterState.insert(
            QStringLiteral("customFieldFilters"),
            customFieldFiltersToJson(
                session.filterState.customFieldFilters
                )
            );

        filterState.insert(
            QStringLiteral("findingStatuses"),
            stringListToJson(
                session.filterState.findingStatuses
                )
            );

        filterState.insert(
            QStringLiteral("bookmarkedOnly"),
            session.filterState.bookmarkedOnly
            );

        sessionObject.insert(
            QStringLiteral("filterState"),
            filterState
            );

        sessionObject.insert(
            QStringLiteral("presentationState"),
            presentationSerializer.serialize(
                session.presentationState
                )
            );

        sessions.append(
            sessionObject
            );
    }

    root.insert(
        QStringLiteral("sessions"),
        sessions
        );

    QJsonArray comparisons;

    const InvestigationComparisonPersistenceSerializer
        comparisonSerializer;

    for (
        const PersistedInvestigationComparison
            &comparison
        : workspace.comparisons
        ) {
        comparisons.append(
            comparisonSerializer.serialize(
                comparison
                )
            );
    }

    root.insert(
        QStringLiteral("comparisons"),
        comparisons
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

        const QJsonValue recordStatesValue =
            sessionObject.value(
                QStringLiteral("recordStates")
                );

        if (!recordStatesValue.isUndefined()) {
            if (!recordStatesValue.isArray()) {
                return failure(
                    QStringLiteral(
                        "INVALID_RECORD_STATES"
                        ),
                    QStringLiteral(
                        "The session recordStates field must be an array."
                        )
                    );
            }

            for (
                const QJsonValue &stateValue
                : recordStatesValue.toArray()
                ) {
                if (!stateValue.isObject()) {
                    return failure(
                        QStringLiteral(
                            "INVALID_RECORD_STATE"
                            ),
                        QStringLiteral(
                            "Each persisted record state must be an object."
                            )
                        );
                }

                const QJsonObject stateObject =
                    stateValue.toObject();

                PersistedInvestigationRecordState
                    state;

                if (!readRequiredString(
                        stateObject,
                        QStringLiteral("recordId"),
                        state.recordId
                        )
                    ||
                    state.recordId.isEmpty()) {
                    return failure(
                        QStringLiteral(
                            "INVALID_RECORD_ID"
                            ),
                        QStringLiteral(
                            "Each persisted record state must have a non-empty recordId."
                            )
                        );
                }

                const QJsonValue bookmarkedValue =
                    stateObject.value(
                        QStringLiteral("bookmarked")
                        );

                if (!bookmarkedValue.isBool()) {
                    return failure(
                        QStringLiteral(
                            "INVALID_BOOKMARK_STATE"
                            ),
                        QStringLiteral(
                            "Persisted bookmarked state must be boolean."
                            )
                        );
                }

                state.bookmarked =
                    bookmarkedValue.toBool();

                const QJsonValue noteValue =
                    stateObject.value(
                        QStringLiteral("note")
                        );

                if (!noteValue.isString()) {
                    return failure(
                        QStringLiteral(
                            "INVALID_NOTE"
                            ),
                        QStringLiteral(
                            "Persisted record notes must be strings."
                            )
                        );
                }

                state.note =
                    noteValue.toString();

                QString findingStatusText;

                if (!readRequiredString(
                        stateObject,
                        QStringLiteral(
                            "findingStatus"
                            ),
                        findingStatusText
                        )) {
                    return failure(
                        QStringLiteral(
                            "INVALID_FINDING_STATUS"
                            ),
                        QStringLiteral(
                            "Persisted findingStatus must be a string."
                            )
                        );
                }

                const std::optional<FindingStatus>
                    findingStatus =
                    findingStatusFromJson(
                        findingStatusText
                        );

                if (!findingStatus.has_value()) {
                    return failure(
                        QStringLiteral(
                            "INVALID_FINDING_STATUS"
                            ),
                        QStringLiteral(
                            "Persisted findingStatus has an unsupported value."
                            )
                        );
                }

                state.findingStatus =
                    *findingStatus;

                session.recordStates.append(
                    std::move(state)
                    );
            }
        }

        const QJsonValue filterStateValue =
            sessionObject.value(
                QStringLiteral("filterState")
                );

        if (!filterStateValue.isUndefined()) {
            if (!filterStateValue.isObject()) {
                return failure(
                    QStringLiteral(
                        "INVALID_FILTER_STATE"
                        ),
                    QStringLiteral(
                        "The session filterState field must be an object."
                        )
                    );
            }

            const QJsonObject filterObject =
                filterStateValue.toObject();

            const auto severities =
                stringListFromJson(
                    filterObject.value(
                        QStringLiteral("severities")
                        )
                    );

            const auto subsystems =
                stringListFromJson(
                    filterObject.value(
                        QStringLiteral("subsystems")
                        )
                    );

            const auto eventCodes =
                stringListFromJson(
                    filterObject.value(
                        QStringLiteral("eventCodes")
                        )
                    );

            const auto entityIds =
                stringListFromJson(
                    filterObject.value(
                        QStringLiteral("entityIds")
                        )
                    );

            const auto findingStatuses =
                stringListFromJson(
                    filterObject.value(
                        QStringLiteral(
                            "findingStatuses"
                            )
                        )
                    );

            if (!severities.has_value()
                || !subsystems.has_value()
                || !eventCodes.has_value()
                || !entityIds.has_value()
                || !findingStatuses.has_value()) {
                return failure(
                    QStringLiteral(
                        "INVALID_FILTER_STATE"
                        ),
                    QStringLiteral(
                        "One or more persisted filter lists are invalid."
                        )
                    );
            }

            session.filterState.severities =
                *severities;

            session.filterState.subsystems =
                *subsystems;

            session.filterState.eventCodes =
                *eventCodes;

            session.filterState.entityIds =
                *entityIds;

            session.filterState.findingStatuses =
                *findingStatuses;

            const QJsonValue searchTextValue =
                filterObject.value(
                    QStringLiteral("searchText")
                    );

            if (!searchTextValue.isString()) {
                return failure(
                    QStringLiteral(
                        "INVALID_FILTER_STATE"
                        ),
                    QStringLiteral(
                        "Persisted searchText must be a string."
                        )
                    );
            }

            session.filterState.searchText =
                searchTextValue.toString();

            if (!readOptionalDateTime(
                    filterObject,
                    QStringLiteral("startTime"),
                    session.filterState.startTime
                    )
                ||
                !readOptionalDateTime(
                    filterObject,
                    QStringLiteral("endTime"),
                    session.filterState.endTime
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_FILTER_TIME_RANGE"
                        ),
                    QStringLiteral(
                        "Persisted filter time boundaries must be valid ISO 8601 date-times or null."
                        )
                    );
            }

            const auto customFilters =
                customFieldFiltersFromJson(
                    filterObject.value(
                        QStringLiteral(
                            "customFieldFilters"
                            )
                        )
                    );

            if (!customFilters.has_value()) {
                return failure(
                    QStringLiteral(
                        "INVALID_CUSTOM_FIELD_FILTERS"
                        ),
                    QStringLiteral(
                        "Persisted custom-field filters are invalid."
                        )
                    );
            }

            session.filterState.customFieldFilters =
                *customFilters;

            const QJsonValue bookmarkedOnlyValue =
                filterObject.value(
                    QStringLiteral("bookmarkedOnly")
                    );

            if (!bookmarkedOnlyValue.isBool()) {
                return failure(
                    QStringLiteral(
                        "INVALID_FILTER_STATE"
                        ),
                    QStringLiteral(
                        "Persisted bookmarkedOnly must be boolean."
                        )
                    );
            }

            session.filterState.bookmarkedOnly =
                bookmarkedOnlyValue.toBool();
        }

        const QJsonValue presentationStateValue =
            sessionObject.value(
                QStringLiteral("presentationState")
                );

        if (!presentationStateValue.isUndefined()) {
            if (!presentationStateValue.isObject()) {
                return failure(
                    QStringLiteral(
                        "INVALID_PRESENTATION_STATE"
                        ),
                    QStringLiteral(
                        "The session presentationState "
                        "field must be an object."
                        )
                    );
            }

            const InvestigationPresentationStateSerializer
                presentationSerializer;

            const PresentationStateDeserializationResult
                presentationResult =
                presentationSerializer.deserialize(
                    presentationStateValue.toObject()
                    );

            if (!presentationResult.isSuccess()) {
                return failure(
                    QStringLiteral(
                        "INVALID_PRESENTATION_STATE"
                        ),
                    QStringLiteral(
                        "A workspace session contains "
                        "invalid presentation state: %1"
                        )
                        .arg(
                            presentationResult.errorMessage
                            )
                    );
            }

            session.presentationState =
                *presentationResult.state;
        }

        workspace.sessions.append(
            std::move(session)
            );
    }

    const QJsonValue comparisonsValue =
        root.value(
            QStringLiteral("comparisons")
            );

    if (!comparisonsValue.isUndefined()) {
        if (!comparisonsValue.isArray()) {
            return failure(
                QStringLiteral(
                    "INVALID_COMPARISONS"
                    ),
                QStringLiteral(
                    "The workspace comparisons field must be an array."
                    )
                );
        }

        const InvestigationComparisonPersistenceSerializer
            comparisonSerializer;

        for (
            const QJsonValue &comparisonValue
            : comparisonsValue.toArray()
            ) {
            if (!comparisonValue.isObject()) {
                return failure(
                    QStringLiteral(
                        "INVALID_COMPARISON"
                        ),
                    QStringLiteral(
                        "Each workspace comparison must be a JSON object."
                        )
                    );
            }

            const ComparisonPersistenceDeserializationResult
                comparisonResult =
                comparisonSerializer.deserialize(
                    comparisonValue.toObject()
                    );

            if (!comparisonResult.isSuccess()) {
                return failure(
                    QStringLiteral(
                        "INVALID_COMPARISON"
                        ),
                    QStringLiteral(
                        "A persisted comparison is invalid: %1"
                        ).arg(
                            comparisonResult.errorMessage
                            )
                    );
            }

            workspace.comparisons.append(
                std::move(
                    *comparisonResult.comparison
                    )
                );
        }
    }

    WorkspaceDeserializationResult result;

    result.workspace =
        std::move(workspace);

    return result;
}