#include "FilterPresetStore.h"

#include <QSettings>
#include <QVariantMap>

namespace
{
const QString PresetsKey =
    QStringLiteral("filters/presets");

const QString VersionKey =
    QStringLiteral("version");

const QString NameKey =
    QStringLiteral("name");

const QString SeveritiesKey =
    QStringLiteral("severities");

const QString SubsystemsKey =
    QStringLiteral("subsystems");

const QString SearchTextKey =
    QStringLiteral("searchText");

const QString BookmarkedOnlyKey =
    QStringLiteral("bookmarkedOnly");

const QString EventCodesKey =
    QStringLiteral("eventCodes");

const QString EntityIdsKey =
    QStringLiteral("entityIds");

const QString TimeRangeStartKey =
    QStringLiteral("timeRangeStart");

const QString TimeRangeEndKey =
    QStringLiteral("timeRangeEnd");

const QString CustomFieldFiltersKey =
    QStringLiteral("customFieldFilters");

constexpr int CurrentPresetVersion = 1;

QVariantMap customFieldFiltersToVariantMap(
    const QMap<QString, QStringList> &filters
    )
{
    QVariantMap result;

    for (auto it = filters.cbegin();
         it != filters.cend();
         ++it) {
        result.insert(
            it.key(),
            it.value()
            );
    }

    return result;
}

QMap<QString, QStringList>
customFieldFiltersFromVariantMap(
    const QVariantMap &values
    )
{
    QMap<QString, QStringList> result;

    for (auto it = values.cbegin();
         it != values.cend();
         ++it) {
        result.insert(
            it.key(),
            it.value().toStringList()
            );
    }

    return result;
}
}

FilterPresetStore::FilterPresetStore(
    QSettings &settings
    )
    : m_settings(settings)
{
}

QVector<InvestigationFilterPreset>
FilterPresetStore::presets() const
{
    QVector<InvestigationFilterPreset> result;

    const int size =
        m_settings.beginReadArray(
            PresetsKey
            );

    result.reserve(size);

    for (int index = 0;
         index < size;
         ++index) {
        m_settings.setArrayIndex(index);

        const int version =
            m_settings
                .value(
                    VersionKey,
                    CurrentPresetVersion
                    )
                .toInt();

        if (version
            != CurrentPresetVersion) {
            continue;
        }

        InvestigationFilterPreset preset;

        preset.name =
            m_settings
                .value(NameKey)
                .toString();

        if (preset.name.trimmed().isEmpty()) {
            continue;
        }

        preset.severities =
            m_settings
                .value(SeveritiesKey)
                .toStringList();

        preset.subsystems =
            m_settings
                .value(SubsystemsKey)
                .toStringList();

        preset.searchText =
            m_settings
                .value(SearchTextKey)
                .toString();

        preset.bookmarkedOnly =
            m_settings
                .value(
                    BookmarkedOnlyKey,
                    false
                    )
                .toBool();

        preset.eventCodes =
            m_settings
                .value(EventCodesKey)
                .toStringList();

        preset.entityIds =
            m_settings
                .value(EntityIdsKey)
                .toStringList();

        const QDateTime startTime =
            m_settings
                .value(TimeRangeStartKey)
                .toDateTime();

        if (startTime.isValid()) {
            preset.timeRangeStart =
                startTime;
        }

        const QDateTime endTime =
            m_settings
                .value(TimeRangeEndKey)
                .toDateTime();

        if (endTime.isValid()) {
            preset.timeRangeEnd =
                endTime;
        }

        preset.customFieldFilters =
            customFieldFiltersFromVariantMap(
                m_settings
                    .value(
                        CustomFieldFiltersKey
                        )
                    .toMap()
                );

        result.append(
            std::move(preset)
            );
    }

    m_settings.endArray();

    return result;
}

bool FilterPresetStore::savePreset(
    InvestigationFilterPreset preset
    )
{
    preset.name =
        preset.name.trimmed();

    if (preset.name.isEmpty()) {
        return false;
    }

    QVector<InvestigationFilterPreset>
        storedPresets =
        presets();

    for (InvestigationFilterPreset &existing
         : storedPresets) {
        if (existing.name.compare(
                preset.name,
                Qt::CaseInsensitive
                )
            == 0) {
            existing =
                std::move(preset);

            writePresets(
                storedPresets
                );

            return true;
        }
    }

    storedPresets.append(
        std::move(preset)
        );

    writePresets(
        storedPresets
        );

    return true;
}

bool FilterPresetStore::removePreset(
    const QString &name
    )
{
    const QString trimmedName =
        name.trimmed();

    if (trimmedName.isEmpty()) {
        return false;
    }

    QVector<InvestigationFilterPreset>
        storedPresets =
        presets();

    for (qsizetype index = 0;
         index < storedPresets.size();
         ++index) {
        if (storedPresets[index]
                .name
                .compare(
                    trimmedName,
                    Qt::CaseInsensitive
                    )
            == 0) {
            storedPresets.removeAt(
                index
                );

            writePresets(
                storedPresets
                );

            return true;
        }
    }

    return false;
}

void FilterPresetStore::writePresets(
    const QVector<InvestigationFilterPreset>
        &presets
    )
{
    /*
     * Clear the previous array first so removed
     * presets or optional fields cannot leave stale
     * QSettings entries behind.
     */
    m_settings.remove(
        PresetsKey
        );

    m_settings.beginWriteArray(
        PresetsKey,
        presets.size()
        );

    for (qsizetype index = 0;
         index < presets.size();
         ++index) {
        m_settings.setArrayIndex(
            index
            );

        const InvestigationFilterPreset &preset =
            presets[index];

        m_settings.setValue(
            VersionKey,
            CurrentPresetVersion
            );

        m_settings.setValue(
            NameKey,
            preset.name
            );

        m_settings.setValue(
            SeveritiesKey,
            preset.severities
            );

        m_settings.setValue(
            SubsystemsKey,
            preset.subsystems
            );

        m_settings.setValue(
            SearchTextKey,
            preset.searchText
            );

        m_settings.setValue(
            BookmarkedOnlyKey,
            preset.bookmarkedOnly
            );

        m_settings.setValue(
            EventCodesKey,
            preset.eventCodes
            );

        m_settings.setValue(
            EntityIdsKey,
            preset.entityIds
            );

        if (preset.timeRangeStart.has_value()) {
            m_settings.setValue(
                TimeRangeStartKey,
                preset.timeRangeStart.value()
                );
        }

        if (preset.timeRangeEnd.has_value()) {
            m_settings.setValue(
                TimeRangeEndKey,
                preset.timeRangeEnd.value()
                );
        }

        m_settings.setValue(
            CustomFieldFiltersKey,
            customFieldFiltersToVariantMap(
                preset.customFieldFilters
                )
            );
    }

    m_settings.endArray();

    m_settings.sync();
}