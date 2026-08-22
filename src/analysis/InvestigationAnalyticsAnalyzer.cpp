#include "InvestigationAnalyticsAnalyzer.h"

#include <QMap>

#include <algorithm>

namespace
{
template<typename ValueSelector>
QVector<InvestigationValueFrequency>
frequenciesFor(
    const QVector<InvestigationRecord> &records,
    ValueSelector selector
    )
{
    QMap<QString, int> counts;

    for (const InvestigationRecord &record
         : records) {
        const std::optional<QString> &value =
            selector(record);

        if (!value.has_value()
            || value->trimmed().isEmpty()) {
            continue;
        }

        ++counts[value.value()];
    }

    QVector<InvestigationValueFrequency>
        frequencies;

    frequencies.reserve(counts.size());

    for (auto iterator = counts.cbegin();
         iterator != counts.cend();
         ++iterator) {
        InvestigationValueFrequency frequency;
        frequency.value = iterator.key();
        frequency.count = iterator.value();

        frequencies.append(frequency);
    }

    std::sort(
        frequencies.begin(),
        frequencies.end(),
        [](
            const InvestigationValueFrequency &left,
            const InvestigationValueFrequency &right
            ) {
            if (left.count == right.count) {
                return QString::compare(
                           left.value,
                           right.value,
                           Qt::CaseSensitive
                           )
                       < 0;
            }

            return left.count > right.count;
        }
        );

    return frequencies;
}
}

QVector<InvestigationValueFrequency>
    InvestigationAnalyticsAnalyzer::
    eventCodeFrequencies(
        const QVector<InvestigationRecord> &records
        ) const
{
    return frequenciesFor(
        records,
        [](
            const InvestigationRecord &record
            ) -> const std::optional<QString> & {
            return record.eventCode;
        }
        );
}

QVector<InvestigationValueFrequency>
    InvestigationAnalyticsAnalyzer::
    entityFrequencies(
        const QVector<InvestigationRecord> &records
        ) const
{
    return frequenciesFor(
        records,
        [](
            const InvestigationRecord &record
            ) -> const std::optional<QString> & {
            return record.entityId;
        }
        );
}