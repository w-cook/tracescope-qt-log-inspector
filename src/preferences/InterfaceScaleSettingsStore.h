#pragma once

#include <QtGlobal>

class QSettings;

class InterfaceScaleSettingsStore
{
public:
    explicit InterfaceScaleSettingsStore(
        QSettings &settings
        );

    qreal userFactor() const;

    void setUserFactor(
        qreal factor
        );

private:
    QSettings &m_settings;
};