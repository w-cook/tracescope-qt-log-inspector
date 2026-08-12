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
}

std::optional<ImportProfile>
builtInImportProfilePreset(
    const QString &presetId
    );