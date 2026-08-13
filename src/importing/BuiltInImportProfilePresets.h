#pragma once

#include <optional>

#include <QString>

#include "ImportProfile.h"

namespace BuiltInImportProfilePresetIds
{
inline const QString ApacheCommon =
    QStringLiteral("apache-common");

inline const QString ApacheNginxCombined =
    QStringLiteral("apache-nginx-combined");

inline const QString IisW3c =
    QStringLiteral("iis-w3c");

inline const QString WindowsEventXml =
    QStringLiteral(
        "windows-event-xml"
        );

inline const QString WindowsEventXmlCollection =
    QStringLiteral(
        "windows-event-xml-collection"
        );
}

std::optional<ImportProfile>
builtInImportProfilePreset(
    const QString &presetId
    );