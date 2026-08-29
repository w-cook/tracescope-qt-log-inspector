#include "WorkspaceDocumentLayoutSerialization.h"

#include <cmath>
#include <limits>

#include <QJsonArray>
#include <QJsonValue>

namespace
{

WorkspaceDocumentLayoutDeserializationResult
failure(
    const QString &message
    )
{
    WorkspaceDocumentLayoutDeserializationResult
        result;

    result.errorCode =
        QStringLiteral(
            "INVALID_DOCUMENT_LAYOUT"
            );

    result.errorMessage =
        message;

    return result;
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

std::optional<QStringList>
stringListFromJson(
    const QJsonValue &value
    )
{
    if (!value.isArray()) {
        return std::nullopt;
    }

    QStringList result;

    for (const QJsonValue &item
         : value.toArray()) {
        if (!item.isString()) {
            return std::nullopt;
        }

        const QString documentId =
            item.toString();

        if (documentId.trimmed().isEmpty()) {
            return std::nullopt;
        }

        result.append(
            documentId
            );
    }

    return result;
}

QJsonObject groupToJson(
    const WorkspaceDocumentGroupLayoutState
        &state
    )
{
    QJsonObject object;

    object.insert(
        QStringLiteral("documentIds"),
        stringListToJson(
            state.documentIds
            )
        );

    object.insert(
        QStringLiteral("currentDocumentId"),
        state.currentDocumentId
        );

    return object;
}

bool groupFromJson(
    const QJsonValue &value,
    WorkspaceDocumentGroupLayoutState
        &state
    )
{
    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    const auto documentIds =
        stringListFromJson(
            object.value(
                QStringLiteral("documentIds")
                )
            );

    const QJsonValue currentValue =
        object.value(
            QStringLiteral(
                "currentDocumentId"
                )
            );

    if (!documentIds.has_value()
        || !currentValue.isString()) {
        return false;
    }

    state.documentIds =
        *documentIds;

    state.currentDocumentId =
        currentValue.toString();

    /*
     * Empty is valid for a group with no current
     * document. Otherwise it must refer to a member
     * of the group.
     */
    if (!state.currentDocumentId.isEmpty()
        && !state.documentIds.contains(
            state.currentDocumentId
            )) {
        return false;
    }

    return true;
}

QJsonValue geometryToJson(
    const QRect &geometry
    )
{
    if (!geometry.isValid()) {
        return QJsonValue(
            QJsonValue::Null
            );
    }

    QJsonObject object;

    object.insert(
        QStringLiteral("x"),
        geometry.x()
        );

    object.insert(
        QStringLiteral("y"),
        geometry.y()
        );

    object.insert(
        QStringLiteral("width"),
        geometry.width()
        );

    object.insert(
        QStringLiteral("height"),
        geometry.height()
        );

    return object;
}

bool readInteger(
    const QJsonObject &object,
    const QString &key,
    int &result
    )
{
    const QJsonValue value =
        object.value(key);

    if (!value.isDouble()) {
        return false;
    }

    const double number =
        value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number
               < std::numeric_limits<int>::min()
        || number
               > std::numeric_limits<int>::max()) {
        return false;
    }

    result =
        static_cast<int>(number);

    return true;
}

bool geometryFromJson(
    const QJsonValue &value,
    QRect &geometry
    )
{
    if (value.isNull()
        || value.isUndefined()) {
        geometry =
            QRect();

        return true;
    }

    if (!value.isObject()) {
        return false;
    }

    const QJsonObject object =
        value.toObject();

    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    if (!readInteger(
            object,
            QStringLiteral("x"),
            x
            )
        || !readInteger(
            object,
            QStringLiteral("y"),
            y
            )
        || !readInteger(
            object,
            QStringLiteral("width"),
            width
            )
        || !readInteger(
            object,
            QStringLiteral("height"),
            height
            )
        || width <= 0
        || height <= 0) {
        return false;
    }

    geometry =
        QRect(
            x,
            y,
            width,
            height
            );

    return true;
}

}

QJsonObject
    WorkspaceDocumentLayoutSerializer::
    serialize(
        const WorkspaceDocumentLayoutState &state
        ) const
{
    QJsonObject object;

    object.insert(
        QStringLiteral("dockedGroup"),
        groupToJson(
            state.dockedGroup
            )
        );

    object.insert(
        QStringLiteral("activeDocumentId"),
        state.activeDocumentId
        );

    QJsonArray detachedWindows;

    for (
        const DetachedWorkspaceWindowLayoutState
            &windowState
        : state.detachedWindows
        ) {
        QJsonObject windowObject;

        windowObject.insert(
            QStringLiteral("group"),
            groupToJson(
                windowState.group
                )
            );

        windowObject.insert(
            QStringLiteral("geometry"),
            geometryToJson(
                windowState.geometry
                )
            );

        windowObject.insert(
            QStringLiteral("maximized"),
            windowState.maximized
            );

        detachedWindows.append(
            windowObject
            );
    }

    object.insert(
        QStringLiteral("detachedWindows"),
        detachedWindows
        );

    return object;
}

WorkspaceDocumentLayoutDeserializationResult
    WorkspaceDocumentLayoutSerializer::
    deserialize(
        const QJsonObject &object
        ) const
{
    WorkspaceDocumentLayoutState state;

    if (!groupFromJson(
            object.value(
                QStringLiteral("dockedGroup")
                ),
            state.dockedGroup
            )) {
        return failure(
            QStringLiteral(
                "dockedGroup is invalid."
                )
            );
    }

    const QJsonValue activeValue =
        object.value(
            QStringLiteral("activeDocumentId")
            );

    if (!activeValue.isString()) {
        return failure(
            QStringLiteral(
                "activeDocumentId must be a string."
                )
            );
    }

    state.activeDocumentId =
        activeValue.toString();

    const QJsonValue windowsValue =
        object.value(
            QStringLiteral("detachedWindows")
            );

    if (!windowsValue.isArray()) {
        return failure(
            QStringLiteral(
                "detachedWindows must be an array."
                )
            );
    }

    for (const QJsonValue &windowValue
         : windowsValue.toArray()) {
        if (!windowValue.isObject()) {
            return failure(
                QStringLiteral(
                    "Each detached window must be "
                    "an object."
                    )
                );
        }

        const QJsonObject windowObject =
            windowValue.toObject();

        DetachedWorkspaceWindowLayoutState
            windowState;

        if (!groupFromJson(
                windowObject.value(
                    QStringLiteral("group")
                    ),
                windowState.group
                )) {
            return failure(
                QStringLiteral(
                    "A detached window group "
                    "is invalid."
                    )
                );
        }

        if (!geometryFromJson(
                windowObject.value(
                    QStringLiteral("geometry")
                    ),
                windowState.geometry
                )) {
            return failure(
                QStringLiteral(
                    "A detached window geometry "
                    "is invalid."
                    )
                );
        }

        const QJsonValue maximizedValue =
            windowObject.value(
                QStringLiteral("maximized")
                );

        if (!maximizedValue.isBool()) {
            return failure(
                QStringLiteral(
                    "A detached window maximized "
                    "field must be boolean."
                    )
                );
        }

        windowState.maximized =
            maximizedValue.toBool();

        state.detachedWindows.append(
            windowState
            );
    }

    WorkspaceDocumentLayoutDeserializationResult
        result;

    result.layout =
        std::move(state);

    return result;
}