#include "LiveLogScenarioLoader.h"

#include <cmath>
#include <limits>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>

namespace
{
constexpr int SupportedSchemaVersion = 1;

LiveLogScenarioLoadResult failure(
    const QString &code,
    const QString &message
    )
{
    LiveLogScenarioLoadResult result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

bool readInteger(
    const QJsonValue &value,
    qint64 &result
    )
{
    if (!value.isDouble()) {
        return false;
    }

    const double number = value.toDouble();

    if (!std::isfinite(number)
        || std::floor(number) != number
        || number <
               static_cast<double>(
                   std::numeric_limits<qint64>::min()
                   )
        || number >
               static_cast<double>(
                   std::numeric_limits<qint64>::max()
                   )) {
        return false;
    }

    result = static_cast<qint64>(number);

    return true;
}

bool readRequiredString(
    const QJsonObject &object,
    const QString &key,
    QString &result
    )
{
    const QJsonValue value = object.value(key);

    if (!value.isString()) {
        return false;
    }

    result = value.toString();

    return true;
}

bool isSupportedAttributeValue(
    const QJsonValue &value
    )
{
    return value.isNull()
    || value.isString()
        || value.isDouble()
        || value.isBool();
}

LiveLogScenarioLoadResult parseRecord(
    const QJsonObject &stepObject,
    LiveLogRecordStep &step,
    int stepIndex
    )
{
    const QJsonValue recordValue =
        stepObject.value(
            QStringLiteral("record")
            );

    if (!recordValue.isObject()) {
        return failure(
            QStringLiteral("INVALID_RECORD"),
            QStringLiteral(
                "Scenario step %1 requires a record object."
                ).arg(stepIndex)
            );
    }

    const QJsonObject recordObject =
        recordValue.toObject();

    qint64 timestampOffsetMs = 0;

    if (!readInteger(
            recordObject.value(
                QStringLiteral("timestampOffsetMs")
                ),
            timestampOffsetMs
            )
        || timestampOffsetMs < 0) {
        return failure(
            QStringLiteral(
                "INVALID_TIMESTAMP_OFFSET"
                ),
            QStringLiteral(
                "Scenario step %1 timestampOffsetMs "
                "must be a non-negative integer."
                ).arg(stepIndex)
            );
    }

    step.record.timestampOffsetMs =
        timestampOffsetMs;

    if (!readRequiredString(
            recordObject,
            QStringLiteral("severity"),
            step.record.severity
            )) {
        return failure(
            QStringLiteral("INVALID_SEVERITY"),
            QStringLiteral(
                "Scenario step %1 severity "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (!readRequiredString(
            recordObject,
            QStringLiteral("subsystem"),
            step.record.subsystem
            )) {
        return failure(
            QStringLiteral("INVALID_SUBSYSTEM"),
            QStringLiteral(
                "Scenario step %1 subsystem "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (!readRequiredString(
            recordObject,
            QStringLiteral("eventCode"),
            step.record.eventCode
            )) {
        return failure(
            QStringLiteral("INVALID_EVENT_CODE"),
            QStringLiteral(
                "Scenario step %1 eventCode "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (!readRequiredString(
            recordObject,
            QStringLiteral("entityId"),
            step.record.entityId
            )) {
        return failure(
            QStringLiteral("INVALID_ENTITY_ID"),
            QStringLiteral(
                "Scenario step %1 entityId "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (!readRequiredString(
            recordObject,
            QStringLiteral("message"),
            step.record.message
            )) {
        return failure(
            QStringLiteral("INVALID_MESSAGE"),
            QStringLiteral(
                "Scenario step %1 message "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (recordObject.contains(
            QStringLiteral("attributes")
            )) {
        const QJsonValue attributesValue =
            recordObject.value(
                QStringLiteral("attributes")
                );

        if (!attributesValue.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_ATTRIBUTES"
                    ),
                QStringLiteral(
                    "Scenario step %1 attributes "
                    "must be an object."
                    ).arg(stepIndex)
                );
        }

        const QJsonObject attributes =
            attributesValue.toObject();

        for (auto iterator =
             attributes.constBegin();
             iterator != attributes.constEnd();
             ++iterator) {
            if (!isSupportedAttributeValue(
                    iterator.value()
                    )) {
                return failure(
                    QStringLiteral(
                        "INVALID_ATTRIBUTE_VALUE"
                        ),
                    QStringLiteral(
                        "Scenario step %1 attribute "
                        "'%2' must be a string, "
                        "number, boolean, or null."
                        )
                        .arg(stepIndex)
                        .arg(iterator.key())
                    );
            }

            step.record.attributes.insert(
                iterator.key(),
                iterator.value().toVariant()
                );
        }
    }

    if (!stepObject.contains(
            QStringLiteral("write")
            )) {
        return {};
    }

    const QJsonValue writeValue =
        stepObject.value(
            QStringLiteral("write")
            );

    if (!writeValue.isObject()) {
        return failure(
            QStringLiteral(
                "INVALID_WRITE_OPTIONS"
                ),
            QStringLiteral(
                "Scenario step %1 write "
                "must be an object."
                ).arg(stepIndex)
            );
    }

    const QJsonObject writeObject =
        writeValue.toObject();

    QString mode;

    if (!readRequiredString(
            writeObject,
            QStringLiteral("mode"),
            mode
            )) {
        return failure(
            QStringLiteral(
                "INVALID_WRITE_MODE"
                ),
            QStringLiteral(
                "Scenario step %1 write mode "
                "must be a string."
                ).arg(stepIndex)
            );
    }

    if (mode == QStringLiteral("normal")) {
        step.write.mode =
            LiveLogWriteMode::Normal;

        return {};
    }

    if (mode != QStringLiteral("partial")) {
        return failure(
            QStringLiteral(
                "INVALID_WRITE_MODE"
                ),
            QStringLiteral(
                "Scenario step %1 has unsupported "
                "write mode '%2'."
                )
                .arg(stepIndex)
                .arg(mode)
            );
    }

    step.write.mode =
        LiveLogWriteMode::Partial;

    const QJsonValue splitValue =
        writeObject.value(
            QStringLiteral("splitFraction")
            );

    if (!splitValue.isDouble()) {
        return failure(
            QStringLiteral(
                "INVALID_SPLIT_FRACTION"
                ),
            QStringLiteral(
                "Scenario step %1 partial write "
                "requires numeric splitFraction."
                ).arg(stepIndex)
            );
    }

    step.write.splitFraction =
        splitValue.toDouble();

    if (!std::isfinite(
            step.write.splitFraction
            )
        || step.write.splitFraction <= 0.0
        || step.write.splitFraction >= 1.0) {
        return failure(
            QStringLiteral(
                "INVALID_SPLIT_FRACTION"
                ),
            QStringLiteral(
                "Scenario step %1 splitFraction "
                "must be greater than 0 and less than 1."
                ).arg(stepIndex)
            );
    }

    qint64 holdMs = 0;

    if (!readInteger(
            writeObject.value(
                QStringLiteral("holdMs")
                ),
            holdMs
            )
        || holdMs < 0) {
        return failure(
            QStringLiteral(
                "INVALID_PARTIAL_HOLD"
                ),
            QStringLiteral(
                "Scenario step %1 holdMs "
                "must be a non-negative integer."
                ).arg(stepIndex)
            );
    }

    step.write.holdMs = holdMs;

    return {};
}
}

LiveLogScenarioLoadResult
LiveLogScenarioLoader::loadFile(
    const QString &filePath
    ) const
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        return failure(
            QStringLiteral(
                "SCENARIO_FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "Could not open scenario file '%1': %2"
                )
                .arg(
                    filePath,
                    file.errorString()
                    )
            );
    }

    return deserialize(
        file.readAll()
        );
}

LiveLogScenarioLoadResult
LiveLogScenarioLoader::deserialize(
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
                "The live-log scenario is not "
                "valid JSON: %1"
                ).arg(
                    parseError.errorString()
                    )
            );
    }

    if (!document.isObject()) {
        return failure(
            QStringLiteral(
                "SCENARIO_ROOT_NOT_OBJECT"
                ),
            QStringLiteral(
                "The live-log scenario root "
                "must be a JSON object."
                )
            );
    }

    const QJsonObject root =
        document.object();

    LiveLogScenario scenario;

    qint64 schemaVersion = 0;

    if (!readInteger(
            root.value(
                QStringLiteral("schemaVersion")
                ),
            schemaVersion
            )) {
        return failure(
            QStringLiteral(
                "INVALID_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "The live-log scenario "
                "schemaVersion must be an integer."
                )
            );
    }

    if (schemaVersion !=
        SupportedSchemaVersion) {
        return failure(
            QStringLiteral(
                "UNSUPPORTED_SCHEMA_VERSION"
                ),
            QStringLiteral(
                "Unsupported live-log scenario "
                "schemaVersion %1. Supported version is %2."
                )
                .arg(schemaVersion)
                .arg(SupportedSchemaVersion)
            );
    }

    scenario.schemaVersion =
        static_cast<int>(schemaVersion);

    if (!readRequiredString(
            root,
            QStringLiteral("name"),
            scenario.name
            )
        || scenario.name.trimmed().isEmpty()) {
        return failure(
            QStringLiteral(
                "INVALID_SCENARIO_NAME"
                ),
            QStringLiteral(
                "The live-log scenario name "
                "must be a non-empty string."
                )
            );
    }

    if (!readRequiredString(
            root,
            QStringLiteral("description"),
            scenario.description
            )) {
        return failure(
            QStringLiteral(
                "INVALID_SCENARIO_DESCRIPTION"
                ),
            QStringLiteral(
                "The live-log scenario description "
                "must be a string."
                )
            );
    }

    const QJsonValue stepsValue =
        root.value(
            QStringLiteral("steps")
            );

    if (!stepsValue.isArray()) {
        return failure(
            QStringLiteral("INVALID_STEPS"),
            QStringLiteral(
                "The live-log scenario steps "
                "value must be an array."
                )
            );
    }

    const QJsonArray steps =
        stepsValue.toArray();

    if (steps.isEmpty()) {
        return failure(
            QStringLiteral("EMPTY_SCENARIO"),
            QStringLiteral(
                "The live-log scenario must "
                "contain at least one step."
                )
            );
    }

    for (qsizetype index = 0;
         index < steps.size();
         ++index) {
        const QJsonValue value =
            steps.at(index);

        const int displayIndex =
            static_cast<int>(index) + 1;

        if (!value.isObject()) {
            return failure(
                QStringLiteral(
                    "INVALID_SCENARIO_STEP"
                    ),
                QStringLiteral(
                    "Scenario step %1 must "
                    "be an object."
                    ).arg(displayIndex)
                );
        }

        const QJsonObject stepObject =
            value.toObject();

        QString type;

        if (!readRequiredString(
                stepObject,
                QStringLiteral("type"),
                type
                )) {
            return failure(
                QStringLiteral(
                    "INVALID_STEP_TYPE"
                    ),
                QStringLiteral(
                    "Scenario step %1 type "
                    "must be a string."
                    ).arg(displayIndex)
                );
        }

        if (type == QStringLiteral("record")) {
            LiveLogRecordStep step;

            const LiveLogScenarioLoadResult
                recordResult =
                parseRecord(
                    stepObject,
                    step,
                    displayIndex
                    );

            if (!recordResult.errorCode.isEmpty()) {
                return recordResult;
            }

            scenario.steps.append(step);
            continue;
        }

        if (type == QStringLiteral("wait")) {
            qint64 durationMs = 0;

            if (!readInteger(
                    stepObject.value(
                        QStringLiteral(
                            "durationMs"
                            )
                        ),
                    durationMs
                    )
                || durationMs < 0) {
                return failure(
                    QStringLiteral(
                        "INVALID_WAIT_DURATION"
                        ),
                    QStringLiteral(
                        "Scenario step %1 durationMs "
                        "must be a non-negative integer."
                        ).arg(displayIndex)
                    );
            }

            scenario.steps.append(
                LiveLogWaitStep{
                    durationMs
                }
                );

            continue;
        }

        if (type ==
            QStringLiteral("truncate")) {
            scenario.steps.append(
                LiveLogTruncateStep{}
                );

            continue;
        }

        if (type ==
            QStringLiteral("replace")) {
            scenario.steps.append(
                LiveLogReplaceStep{}
                );

            continue;
        }

        if (type ==
            QStringLiteral("rotate")) {
            scenario.steps.append(
                LiveLogRotateStep{}
                );

            continue;
        }

        return failure(
            QStringLiteral(
                "UNKNOWN_STEP_TYPE"
                ),
            QStringLiteral(
                "Scenario step %1 has unknown "
                "type '%2'."
                )
                .arg(displayIndex)
                .arg(type)
            );
    }

    LiveLogScenarioLoadResult result;
    result.scenario = std::move(scenario);

    return result;
}