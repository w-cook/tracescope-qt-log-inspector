#pragma once

#include <QList>
#include <QMap>
#include <QString>

#include "../domain/RecordSeverity.h"
#include "CanonicalFieldMappings.h"

enum class TimestampRuleType
{
    Iso8601,
    QtFormat
};

struct TimestampRule
{
    TimestampRuleType type =
        TimestampRuleType::Iso8601;

    QString format;
};

struct CustomFieldMapping
{
    QString name;
    QString sourcePath;
};

struct ImportProfile
{
    inline static constexpr int
        CurrentSchemaVersion = 1;

    int schemaVersion =
        CurrentSchemaVersion;

    QString name;

    QString importerId =
        QStringLiteral("json-lines");

    QString recordPath;

    QString regexPattern;

    CanonicalFieldMappings canonicalFields;

    QList<CustomFieldMapping>
        customFields;

    QMap<QString, RecordSeverity>
        severityAliases;

    QList<TimestampRule>
        timestampRules {
            TimestampRule {}
        };

    bool preserveUnmappedFields = true;
};