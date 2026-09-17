#include "DelimitedTextRenderer.h"

#include <QSet>

#include <variant>

namespace
{
const QStringList CanonicalFields {
    QStringLiteral("timestamp"),
    QStringLiteral("level"),
    QStringLiteral("subsystem"),
    QStringLiteral("eventCode"),
    QStringLiteral("entityId"),
    QStringLiteral("message")
};

bool isCanonicalField(
    const QString &key
    )
{
    const QString normalized =
        key.trimmed().toCaseFolded();

    for (const QString &canonical
         : CanonicalFields) {
        if (canonical.toCaseFolded()
            == normalized) {
            return true;
        }
    }

    return false;
}

QStringList normalizedAttributeKeys(
    const QStringList &keys
    )
{
    QStringList candidates;

    for (const QString &key : keys) {
        const QString trimmed =
            key.trimmed();

        if (trimmed.isEmpty()
            || isCanonicalField(trimmed)) {
            continue;
        }

        candidates.append(trimmed);
    }

    /*
     * Sort before case-insensitive de-duplication
     * so the retained spelling is deterministic.
     */
    candidates.sort(Qt::CaseSensitive);

    QSet<QString> seen;
    QStringList result;

    for (const QString &key
         : candidates) {
        const QString normalized =
            key.toCaseFolded();

        if (seen.contains(normalized)) {
            continue;
        }

        seen.insert(normalized);
        result.append(key);
    }

    return result;
}
}

DelimitedTextRenderer::DelimitedTextRenderer(
    QChar delimiter,
    QStringList attributeKeys
    )
    : delimiter(delimiter),
    attributeKeys(
        normalizedAttributeKeys(
            attributeKeys
            )
        )
{
}

QStringList
DelimitedTextRenderer::attributeKeysForScenario(
    const LiveLogScenario &scenario
    )
{
    QStringList keys;

    for (const LiveLogScenarioStep &step
         : scenario.steps) {
        if (!std::holds_alternative<
                LiveLogRecordStep
                >(step)) {
            continue;
        }

        const LiveLogRecordStep &recordStep =
            std::get<LiveLogRecordStep>(
                step
                );

        for (auto iterator =
             recordStep.record
                 .attributes.constBegin();
             iterator !=
             recordStep.record
                 .attributes.constEnd();
             ++iterator) {
            keys.append(iterator.key());
        }
    }

    return normalizedAttributeKeys(keys);
}

QByteArray
DelimitedTextRenderer::initialContent() const
{
    QStringList headers {
        QStringLiteral("timestamp"),
        QStringLiteral("level"),
        QStringLiteral("subsystem"),
        QStringLiteral("eventCode"),
        QStringLiteral("entityId"),
        QStringLiteral("message")
    };

    headers.append(attributeKeys);

    return renderFields(headers);
}

QByteArray
DelimitedTextRenderer::renderRecord(
    const LiveLogRecord &record,
    const QDateTime &scenarioStart
    ) const
{
    const QDateTime timestamp =
        scenarioStart
            .addMSecs(
                record.timestampOffsetMs
                )
            .toUTC();

    QStringList fields {
        timestamp.toString(
            Qt::ISODateWithMs
            ),
        record.severity,
        record.subsystem,
        record.eventCode,
        record.entityId,
        record.message
    };

    for (const QString &key
         : attributeKeys) {
        fields.append(
            attributeValue(
                readAttribute(
                    record,
                    key
                    )
                )
            );
    }

    return renderFields(fields);
}

QString DelimitedTextRenderer::renderField(
    const QString &value
    ) const
{
    const bool requiresQuotes =
        value.contains(delimiter)
        || value.contains(
            QLatin1Char('"')
            )
        || value.contains(
            QLatin1Char('\n')
            )
        || value.contains(
            QLatin1Char('\r')
            );

    if (!requiresQuotes) {
        return value;
    }

    QString escaped = value;

    escaped.replace(
        QStringLiteral("\""),
        QStringLiteral("\"\"")
        );

    return QStringLiteral("\"%1\"")
        .arg(escaped);
}

QByteArray DelimitedTextRenderer::renderFields(
    const QStringList &fields
    ) const
{
    QStringList renderedFields;

    renderedFields.reserve(
        fields.size()
        );

    for (const QString &field
         : fields) {
        renderedFields.append(
            renderField(field)
            );
    }

    QString row =
        renderedFields.join(
            delimiter
            );

    row.append(
        QLatin1Char('\n')
        );

    return row.toUtf8();
}

QString DelimitedTextRenderer::attributeValue(
    const QVariant &value
    ) const
{
    if (!value.isValid()
        || value.isNull()) {
        return {};
    }

    if (value.metaType().id()
        == QMetaType::Bool) {
        return value.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}

QVariant DelimitedTextRenderer::readAttribute(
    const LiveLogRecord &record,
    const QString &key
    ) const
{
    const auto exact =
        record.attributes.constFind(key);

    if (exact !=
        record.attributes.constEnd()) {
        return exact.value();
    }

    const QString normalized =
        key.toCaseFolded();

    QStringList matches;

    for (auto iterator =
         record.attributes.constBegin();
         iterator !=
         record.attributes.constEnd();
         ++iterator) {
        if (iterator.key()
                .toCaseFolded()
            == normalized) {
            matches.append(
                iterator.key()
                );
        }
    }

    if (matches.isEmpty()) {
        return {};
    }

    matches.sort(Qt::CaseSensitive);

    return record.attributes.value(
        matches.first()
        );
}