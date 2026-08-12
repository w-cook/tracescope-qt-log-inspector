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

ImportProfile createIisW3cProfile()
{
    ImportProfile profile;

    profile.name =
        QStringLiteral(
            "IIS W3C Extended Log"
            );

    profile.importerId =
        QStringLiteral(
            "iis-w3c"
            );

    profile.canonicalFields.timestampPath =
        QStringLiteral("timestamp");

    profile.canonicalFields.severityPath.clear();
    profile.canonicalFields.subsystemPath.clear();
    profile.canonicalFields.eventCodePath.clear();
    profile.canonicalFields.entityIdPath.clear();

    profile.canonicalFields.messagePath =
        QStringLiteral("message");

    profile.customFields = {
        {
            QStringLiteral("Server IP"),
            QStringLiteral("s-ip")
        },
        {
            QStringLiteral("HTTP Method"),
            QStringLiteral("cs-method")
        },
        {
            QStringLiteral("URI Stem"),
            QStringLiteral("cs-uri-stem")
        },
        {
            QStringLiteral("URI Query"),
            QStringLiteral("cs-uri-query")
        },
        {
            QStringLiteral("Server Port"),
            QStringLiteral("s-port")
        },
        {
            QStringLiteral("Username"),
            QStringLiteral("cs-username")
        },
        {
            QStringLiteral("Client IP"),
            QStringLiteral("c-ip")
        },
        {
            QStringLiteral("User Agent"),
            QStringLiteral("cs(User-Agent)")
        },
        {
            QStringLiteral("Referrer"),
            QStringLiteral("cs(Referer)")
        },
        {
            QStringLiteral("HTTP Status"),
            QStringLiteral("sc-status")
        },
        {
            QStringLiteral("HTTP Substatus"),
            QStringLiteral("sc-substatus")
        },
        {
            QStringLiteral("Win32 Status"),
            QStringLiteral("sc-win32-status")
        },
        {
            QStringLiteral("Time Taken (ms)"),
            QStringLiteral("time-taken")
        }
    };

    profile.timestampRules = {
        TimestampRule {}
    };

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

    if (presetId
        == BuiltInImportProfilePresetIds::
        IisW3c) {
        return createIisW3cProfile();
    }

    return std::nullopt;
}