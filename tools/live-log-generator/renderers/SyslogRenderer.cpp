#include "SyslogRenderer.h"

#include <algorithm>

#include <QMetaType>
#include <QLocale>
#include <QStringList>
#include <QVariant>

namespace
{
constexpr int local0Facility = 16;

int severityCode(
    const QString &severity
    )
{
    const QString normalized =
        severity.trimmed().toUpper();

    if (normalized
        == QStringLiteral("CRITICAL")) {
        return 2;
    }

    if (normalized
        == QStringLiteral("ERROR")) {
        return 3;
    }

    if (normalized
            == QStringLiteral("WARN")
        || normalized
               == QStringLiteral("WARNING")) {
        return 4;
    }

    if (normalized
            == QStringLiteral("DEBUG")
        || normalized
               == QStringLiteral("TRACE")) {
        return 7;
    }

    /*
     * INFO and unrecognized scenario severities
     * degrade to Informational rather than
     * manufacturing a stronger severity.
     */
    return 6;
}

int priority(
    const QString &severity
    )
{
    return local0Facility * 8
           + severityCode(
               severity
               );
}

QString scalarText(
    const QVariant &value
    )
{
    if (!value.isValid()
        || value.isNull()) {
        return QStringLiteral("null");
    }

    const int typeId =
        value.metaType().id();

    if (typeId
        == QMetaType::Bool) {
        return value.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    if (typeId == QMetaType::Double
        || typeId == QMetaType::Float) {
        return QString::number(
            value.toDouble(),
            'g',
            15
            );
    }

    return value.toString();
}

QString escapeStructuredDataValue(
    const QString &value
    )
{
    QString escaped;

    escaped.reserve(
        value.size()
        );

    for (const QChar character : value) {
        if (character
                == QLatin1Char('\\')
            || character
                   == QLatin1Char('"')
            || character
                   == QLatin1Char(']')) {
            escaped.append(
                QLatin1Char('\\')
                );
        }

        escaped.append(
            character
            );
    }

    return escaped;
}

QStringList sortedAttributeKeys(
    const QHash<QString, QVariant> &attributes
    )
{
    QStringList keys =
        attributes.keys();

    std::sort(
        keys.begin(),
        keys.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            const int insensitive =
                QString::compare(
                    left,
                    right,
                    Qt::CaseInsensitive
                    );

            if (insensitive != 0) {
                return insensitive < 0;
            }

            return left < right;
        }
        );

    return keys;
}

bool containsNonPrintUsAscii(
    const QString &value
    )
{
    for (const QChar character : value) {
        const ushort code =
            character.unicode();

        if (code < 33
            || code > 126) {
            return true;
        }
    }

    return false;
}

QString rfc3164Timestamp(
    const QDateTime &timestamp
    )
{
    const QDateTime local =
        timestamp.toUTC();

    const QString month =
        QLocale::c().toString(
            local.date(),
            QStringLiteral("MMM")
            );

    const QString day =
        QString::number(
            local.date().day()
            )
            .rightJustified(
                2,
                QLatin1Char(' ')
                );

    const QString time =
        local.time().toString(
            QStringLiteral("HH:mm:ss")
            );

    return QStringLiteral("%1 %2 %3")
        .arg(
            month,
            day,
            time
            );
}
}

SyslogRenderer::SyslogRenderer(
    SyslogFormat format
    )
    : m_format(format)
{
}

QByteArray
SyslogRenderer::initialContent() const
{
    return {};
}

QByteArray
SyslogRenderer::renderRecord(
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

    if (m_format
        == SyslogFormat::Rfc3164) {
        return renderRfc3164(
            record,
            timestamp
            );
    }

    return renderRfc5424(
        record,
        timestamp
        );
}

bool SyslogRenderer::
    isValidRfc5424AppName(
        const QString &value
        )
{
    if (value.isEmpty()
        || value.size() > 48) {
        return false;
    }

    return !containsNonPrintUsAscii(
        value
        );
}

bool SyslogRenderer::
    isValidRfc5424MessageId(
        const QString &value
        )
{
    if (value.isEmpty()
        || value.size() > 32) {
        return false;
    }

    return !containsNonPrintUsAscii(
        value
        );
}

bool SyslogRenderer::
    isValidRfc5424StructuredDataName(
        const QString &value
        )
{
    if (value.isEmpty()
        || value.size() > 32) {
        return false;
    }

    for (const QChar character : value) {
        const ushort code =
            character.unicode();

        if (code < 33
            || code > 126
            || character
                   == QLatin1Char('=')
            || character
                   == QLatin1Char('"')
            || character
                   == QLatin1Char(']')) {
            return false;
        }
    }

    return true;
}

bool SyslogRenderer::
    isValidRfc3164Tag(
        const QString &value
        )
{
    if (value.isEmpty()) {
        return false;
    }

    for (const QChar character : value) {
        const bool asciiLetter =
            (character
                 >= QLatin1Char('A')
             && character
                    <= QLatin1Char('Z'))
            || (character
                    >= QLatin1Char('a')
                && character
                       <= QLatin1Char('z'));

        const bool asciiDigit =
            character
                >= QLatin1Char('0')
            && character
                   <= QLatin1Char('9');

        if (asciiLetter
            || asciiDigit
            || character
                   == QLatin1Char('_')
            || character
                   == QLatin1Char('.')
            || character
                   == QLatin1Char('-')) {
            continue;
        }

        return false;
    }

    return true;
}

QByteArray
SyslogRenderer::renderRfc5424(
    const LiveLogRecord &record,
    const QDateTime &timestamp
    ) const
{
    const QString appName =
        record.subsystem.isEmpty()
            ? QStringLiteral(
                  "tracescope-live"
                  )
            : record.subsystem;

    const QString messageId =
        record.eventCode.isEmpty()
            ? QStringLiteral("-")
            : record.eventCode;

    QString structuredData =
        QStringLiteral(
            "[tracescope@32473"
            );

    if (!record.entityId.isEmpty()) {
        structuredData +=
            QStringLiteral(
                " entityId=\"%1\""
                )
                .arg(
                    escapeStructuredDataValue(
                        record.entityId
                        )
                    );
    }

    const QStringList keys =
        sortedAttributeKeys(
            record.attributes
            );

    for (const QString &key : keys) {
        /*
         * The factory will reject keys that cannot
         * legally be represented as SD-PARAM names.
         */
        structuredData +=
            QStringLiteral(
                " %1=\"%2\""
                )
                .arg(
                    key,
                    escapeStructuredDataValue(
                        scalarText(
                            record.attributes.value(
                                key
                                )
                            )
                        )
                    );
    }

    structuredData +=
        QLatin1Char(']');

    const QString line =
        QStringLiteral(
            "<%1>1 %2 tracescope-live "
            "%3 - %4 %5 %6\n"
            )
            .arg(
                QString::number(
                    priority(
                        record.severity
                        )
                    ),
                timestamp.toString(
                    Qt::ISODateWithMs
                    ),
                appName,
                messageId,
                structuredData,
                record.message
                );

    return line.toUtf8();
}

QByteArray
SyslogRenderer::renderRfc3164(
    const LiveLogRecord &record,
    const QDateTime &timestamp
    ) const
{
    const QString tag =
        record.subsystem.isEmpty()
            ? QStringLiteral(
                  "tracescope-live"
                  )
            : record.subsystem;

    /*
     * RFC 3164 has no native MSGID or structured
     * data equivalent. Do not smuggle scenario
     * event codes or custom attributes into the
     * message: this renderer should exercise the
     * source format as it actually behaves.
     */
    const QString line =
        QStringLiteral(
            "<%1>%2 tracescope-live %3: %4\n"
            )
            .arg(
                QString::number(
                    priority(
                        record.severity
                        )
                    ),
                rfc3164Timestamp(
                    timestamp
                    ),
                tag,
                record.message
                );

    return line.toUtf8();
}