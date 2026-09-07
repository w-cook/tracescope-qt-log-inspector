#include "KeyValueTextRenderer.h"

#include <algorithm>

#include <QMetaType>
#include <QStringList>

QByteArray KeyValueTextRenderer::initialContent() const
{
    return {};
}

QByteArray KeyValueTextRenderer::renderRecord(
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

    QStringList assignments;

    assignments.append(
        QStringLiteral("timestamp=%1")
            .arg(
                renderStringValue(
                    timestamp.toString(
                        Qt::ISODateWithMs
                        )
                    )
                )
        );

    assignments.append(
        QStringLiteral("level=%1")
            .arg(
                renderStringValue(
                    record.severity
                    )
                )
        );

    assignments.append(
        QStringLiteral("subsystem=%1")
            .arg(
                renderStringValue(
                    record.subsystem
                    )
                )
        );

    assignments.append(
        QStringLiteral("eventCode=%1")
            .arg(
                renderStringValue(
                    record.eventCode
                    )
                )
        );

    assignments.append(
        QStringLiteral("entityId=%1")
            .arg(
                renderStringValue(
                    record.entityId
                    )
                )
        );

    const QStringList customKeys =
        customKeysForRecord(
            record
            );

    for (const QString &key
         : customKeys) {
        assignments.append(
            QStringLiteral("%1=%2")
                .arg(
                    key,
                    renderValue(
                        record.attributes.value(
                            key
                            )
                        )
                    )
            );
    }

    assignments.append(
        QStringLiteral("message=%1")
            .arg(
                renderStringValue(
                    record.message
                    )
                )
        );

    QString rendered =
        assignments.join(
            QLatin1Char(' ')
            );

    rendered.append(
        QLatin1Char('\n')
        );

    return rendered.toUtf8();
}

bool KeyValueTextRenderer::isValidAttributeKey(
    const QString &key
    )
{
    if (key.isEmpty()) {
        return false;
    }

    for (const QChar character : key) {
        if (character.isSpace()
            || character == QLatin1Char('=')) {
            return false;
        }
    }

    return true;
}

bool KeyValueTextRenderer::isCanonicalKey(
    const QString &key
    )
{
    static const QStringList canonicalKeys = {
        QStringLiteral("timestamp"),
        QStringLiteral("level"),
        QStringLiteral("subsystem"),
        QStringLiteral("eventCode"),
        QStringLiteral("entityId"),
        QStringLiteral("message")
    };

    for (const QString &canonicalKey
         : canonicalKeys) {
        if (key.compare(
                canonicalKey,
                Qt::CaseInsensitive
                ) == 0) {
            return true;
        }
    }

    return false;
}

QStringList KeyValueTextRenderer::customKeysForRecord(
    const LiveLogRecord &record
    )
{
    QStringList keys =
        record.attributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](
            const QString &left,
            const QString &right
            )
        {
            const int caseInsensitive =
                QString::compare(
                    left,
                    right,
                    Qt::CaseInsensitive
                    );

            if (caseInsensitive != 0) {
                return caseInsensitive < 0;
            }

            return left < right;
        }
        );

    QStringList result;

    for (const QString &key : keys) {
        if (isCanonicalKey(key)) {
            continue;
        }

        bool duplicate = false;

        for (const QString &existing
             : result) {
            if (existing.compare(
                    key,
                    Qt::CaseInsensitive
                    ) == 0) {
                duplicate = true;
                break;
            }
        }

        if (!duplicate) {
            result.append(key);
        }
    }

    return result;
}

QString KeyValueTextRenderer::renderValue(
    const QVariant &value
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return QStringLiteral("null");
    }

    if (value.metaType().id()
        == QMetaType::Bool) {
        return value.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    if (value.metaType().id()
            != QMetaType::QString
        && value.canConvert<double>()) {
        return QString::number(
            value.toDouble(),
            'g',
            15
            );
    }

    return renderStringValue(
        value.toString()
        );
}

QString KeyValueTextRenderer::renderStringValue(
    const QString &value
    )
{
    bool requiresQuotes =
        value.isEmpty();

    for (const QChar character : value) {
        if (character.isSpace()
            || character == QLatin1Char('"')
            || character == QLatin1Char('\\')) {
            requiresQuotes = true;
            break;
        }
    }

    if (!requiresQuotes) {
        return value;
    }

    QString escaped;

    for (const QChar character : value) {
        switch (character.unicode()) {
        case '"':
            escaped.append(
                QStringLiteral("\\\"")
                );
            break;

        case '\\':
            escaped.append(
                QStringLiteral("\\\\")
                );
            break;

        case '\n':
            escaped.append(
                QStringLiteral("\\n")
                );
            break;

        case '\r':
            escaped.append(
                QStringLiteral("\\r")
                );
            break;

        case '\t':
            escaped.append(
                QStringLiteral("\\t")
                );
            break;

        default:
            escaped.append(character);
            break;
        }
    }

    return QStringLiteral("\"%1\"")
        .arg(escaped);
}