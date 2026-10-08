#include "InterfaceScaleSettingsStore.h"

#include <cmath>

#include <QSettings>
#include <QString>

namespace
{
const QString UserFactorKey =
    QStringLiteral(
        "interface/userScaleFactor"
        );

constexpr qreal DefaultUserFactor =
    1.0;
}

InterfaceScaleSettingsStore::
    InterfaceScaleSettingsStore(
        QSettings &settings
        )
    : m_settings(settings)
{
}

qreal InterfaceScaleSettingsStore::
    userFactor() const
{
    bool ok = false;

    const qreal factor =
        m_settings
            .value(
                UserFactorKey,
                DefaultUserFactor
                )
            .toDouble(
                &ok
                );

    if (
        !ok
        || !std::isfinite(
            factor
            )
        ) {
        return DefaultUserFactor;
    }

    return factor;
}

void InterfaceScaleSettingsStore::
    setUserFactor(
        qreal factor
        )
{
    if (!std::isfinite(
            factor
            )) {
        factor =
            DefaultUserFactor;
    }

    m_settings.setValue(
        UserFactorKey,
        factor
        );

    m_settings.sync();
}