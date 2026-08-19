#pragma once

#include <QString>
#include <QVector>

#include "../filtering/InvestigationFilterPreset.h"

class QSettings;

class FilterPresetStore
{
public:
    explicit FilterPresetStore(
        QSettings &settings
        );

    QVector<InvestigationFilterPreset>
    presets() const;

    bool savePreset(
        InvestigationFilterPreset preset
        );

    bool removePreset(
        const QString &name
        );

private:
    QSettings &m_settings;

    void writePresets(
        const QVector<InvestigationFilterPreset>
            &presets
        );
};