#include "BuiltInImportProfilePresets.h"

namespace
{
ImportProfile createWebAccessProfile(
    const QString &name,
    const QString &regexPattern,
    bool combined
    )
{
    ImportProfile profile;

    profile.name = name;

    profile.importerId =
        QStringLiteral("regex-text");

    profile.regexPattern =
        regexPattern;

    profile.canonicalFields.timestampPath =
        QStringLiteral("timestamp");

    profile.canonicalFields.severityPath.clear();
    profile.canonicalFields.subsystemPath.clear();
    profile.canonicalFields.eventCodePath.clear();
    profile.canonicalFields.entityIdPath.clear();

    profile.canonicalFields.messagePath =
        QStringLiteral("request");

    profile.customFields = {
        {
            QStringLiteral("Client Address"),
            QStringLiteral("remoteAddress")
        },
        {
            QStringLiteral("Remote Logname"),
            QStringLiteral("remoteLogname")
        },
        {
            QStringLiteral("Remote User"),
            QStringLiteral("remoteUser")
        },
        {
            QStringLiteral("HTTP Status"),
            QStringLiteral("status")
        },
        {
            QStringLiteral("Response Bytes"),
            QStringLiteral("bytesSent")
        }
    };

    if (combined) {
        profile.customFields.append({
            QStringLiteral("Referrer"),
            QStringLiteral("referrer")
        });

        profile.customFields.append({
            QStringLiteral("User Agent"),
            QStringLiteral("userAgent")
        });
    }

    profile.severityAliases.clear();

    profile.timestampRules.clear();

    TimestampRule timestampRule;

    timestampRule.type =
        TimestampRuleType::QtFormat;

    timestampRule.format =
        QStringLiteral(
            "dd/MMM/yyyy:HH:mm:ss tt"
            );

    profile.timestampRules.append(
        timestampRule
        );

    profile.preserveUnmappedFields = true;

    return profile;
}
}

std::optional<ImportProfile>
builtInImportProfilePreset(
    const QString &presetId
    )
{
    if (presetId
        == BuiltInImportProfilePresetIds::
        ApacheCommon) {
        return createWebAccessProfile(
            QStringLiteral(
                "Apache Common Access Log"
                ),
            QString::fromLatin1(
                R"regex(^(?<remoteAddress>\S+)\s+(?<remoteLogname>\S+)\s+(?<remoteUser>\S+)\s+\[(?<timestamp>[^\]]+)\]\s+"(?<request>[^"]*)"\s+(?<status>\d{3})\s+(?<bytesSent>\d+|-)$)regex"
                ),
            false
            );
    }

    if (presetId
        == BuiltInImportProfilePresetIds::
        ApacheNginxCombined) {
        return createWebAccessProfile(
            QStringLiteral(
                "Apache/Nginx Combined Access Log"
                ),
            QString::fromLatin1(
                R"regex(^(?<remoteAddress>\S+)\s+(?<remoteLogname>\S+)\s+(?<remoteUser>\S+)\s+\[(?<timestamp>[^\]]+)\]\s+"(?<request>[^"]*)"\s+(?<status>\d{3})\s+(?<bytesSent>\d+|-)\s+"(?<referrer>[^"]*)"\s+"(?<userAgent>[^"]*)"$)regex"
                ),
            true
            );
    }

    return std::nullopt;
}