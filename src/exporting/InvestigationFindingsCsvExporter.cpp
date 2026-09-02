#include "InvestigationFindingsCsvExporter.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QTextStream>

#include <algorithm>
#include <utility>

#include "../domain/RecordSeverity.h"

namespace
{
bool isCanonicalKey(
    const QString &key
    )
{
    return key == QStringLiteral("timestamp")
    || key == QStringLiteral("level")
        || key == QStringLiteral("severity")
        || key == QStringLiteral("subsystem")
        || key == QStringLiteral("eventCode")
        || key == QStringLiteral("entityId")
        || key == QStringLiteral("message");
}
}

bool InvestigationFindingsCsvExporter::exportToFile(
    const QVector<InvestigationFindingExport> &findings,
    const QString &filePath
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Text
            )) {
        return false;
    }

    QTextStream stream(&file);

    const QStringList customKeys =
        customAttributeKeys(findings);

    QStringList headers {
        QStringLiteral("Finding Status"),
        QStringLiteral("Analyst Note"),
        QStringLiteral("Bookmarked"),
        QStringLiteral("Record ID"),
        QStringLiteral("Timestamp"),
        QStringLiteral("Severity"),
        QStringLiteral("Subsystem"),
        QStringLiteral("Event Code"),
        QStringLiteral("Entity ID"),
        QStringLiteral("Message"),
        QStringLiteral("Source Name"),
        QStringLiteral("Source Path"),
        QStringLiteral("Source Record")
    };

    headers.append(customKeys);

    headers.append(
        QStringLiteral("Raw Source")
        );

    QStringList escapedHeaders;

    escapedHeaders.reserve(
        headers.size()
        );

    for (
        const QString &header :
        std::as_const(headers)
        ) {
        escapedHeaders.append(
            escapeCsvField(header)
            );
    }

    stream
        << escapedHeaders.join(
               QLatin1Char(',')
               )
        << QLatin1Char('\n');

    for (
        const InvestigationFindingExport &finding :
        findings
        ) {
        const InvestigationRecord &record =
            finding.record;

        QStringList fields;

        fields.reserve(
            headers.size()
            );

        fields.append(
            findingStatusToString(
                finding.status
                )
            );

        fields.append(
            finding.note
            );

        fields.append(
            finding.bookmarked
                ? QStringLiteral("true")
                : QStringLiteral("false")
            );

        fields.append(
            record.recordId
            );

        fields.append(
            record.timestamp.has_value()
                ? record.timestamp->toString(
                      Qt::ISODateWithMs
                      )
                : QString()
            );

        fields.append(
            record.severity.has_value()
                ? recordSeverityToString(
                      record.severity.value()
                      )
                : QString()
            );

        fields.append(
            record.subsystem.value_or(
                QString()
                )
            );

        fields.append(
            record.eventCode.value_or(
                QString()
                )
            );

        fields.append(
            record.entityId.value_or(
                QString()
                )
            );

        fields.append(
            record.message.value_or(
                QString()
                )
            );

        fields.append(
            record.source.sourceName
            );

        fields.append(
            record.source.sourcePath
            );

        fields.append(
            record.source.recordNumber > 0
                ? QString::number(
                      record.source.recordNumber
                      )
                : QString()
            );

        for (
            const QString &key :
            customKeys
            ) {
            fields.append(
                variantToString(
                    record.customAttributes.value(
                        key
                        )
                    )
                );
        }

        fields.append(
            record.rawSource
            );

        QStringList escapedFields;

        escapedFields.reserve(
            fields.size()
            );

        for (
            const QString &field :
            std::as_const(fields)
            ) {
            escapedFields.append(
                escapeCsvField(field)
                );
        }

        stream
            << escapedFields.join(
                   QLatin1Char(',')
                   )
            << QLatin1Char('\n');
    }

    return true;
}

QStringList
    InvestigationFindingsCsvExporter::
    customAttributeKeys(
        const QVector<InvestigationFindingExport> &findings
        ) const
{
    QSet<QString> keys;

    for (
        const InvestigationFindingExport &finding :
        findings
        ) {
        for (
            auto iterator =
            finding
                .record
                .customAttributes
                .constBegin();
            iterator
            != finding
                   .record
                   .customAttributes
                   .constEnd();
            ++iterator
            ) {
            if (!isCanonicalKey(
                    iterator.key()
                    )) {
                keys.insert(
                    iterator.key()
                    );
            }
        }
    }

    QStringList sortedKeys =
        keys.values();

    std::sort(
        sortedKeys.begin(),
        sortedKeys.end(),
        [](const QString &left,
           const QString &right) {
            const int insensitiveComparison =
                left.compare(
                    right,
                    Qt::CaseInsensitive
                    );

            if (insensitiveComparison != 0) {
                return insensitiveComparison < 0;
            }

            return left.compare(
                       right,
                       Qt::CaseSensitive
                       ) < 0;
        }
        );

    return sortedKeys;
}

QString
    InvestigationFindingsCsvExporter::
    findingStatusToString(
        FindingStatus status
        ) const
{
    switch (status) {
    case FindingStatus::Open:
        return QStringLiteral("OPEN");

    case FindingStatus::Resolved:
        return QStringLiteral("RESOLVED");

    case FindingStatus::Dismissed:
        return QStringLiteral("DISMISSED");

    case FindingStatus::None:
        return QStringLiteral("NONE");
    }

    return QStringLiteral("NONE");
}

QString
    InvestigationFindingsCsvExporter::
    variantToString(
        const QVariant &value
        ) const
{
    if (!value.isValid()
        || value.isNull()) {
        return {};
    }

    const QJsonValue jsonValue =
        QJsonValue::fromVariant(
            value
            );

    if (jsonValue.isObject()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toObject()
                ).toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    if (jsonValue.isArray()) {
        return QString::fromUtf8(
            QJsonDocument(
                jsonValue.toArray()
                ).toJson(
                    QJsonDocument::Compact
                    )
            );
    }

    if (jsonValue.isBool()) {
        return jsonValue.toBool()
        ? QStringLiteral("true")
        : QStringLiteral("false");
    }

    return value.toString();
}

QString
    InvestigationFindingsCsvExporter::
    escapeCsvField(
        const QString &value
        ) const
{
    QString escaped = value;

    escaped.replace(
        "\"",
        "\"\""
        );

    if (escaped.contains(",")
        || escaped.contains("\"")
        || escaped.contains("\n")
        || escaped.contains("\r")) {
        return "\""
               + escaped
               + "\"";
    }

    return escaped;
}