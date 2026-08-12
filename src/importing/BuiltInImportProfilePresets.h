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
}

std::optional<ImportProfile>
builtInImportProfilePreset(
    const QString &presetId
    );