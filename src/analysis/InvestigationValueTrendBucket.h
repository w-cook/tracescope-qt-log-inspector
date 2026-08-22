#pragma once

#include <QDateTime>
#include <QMap>
#include <QString>

struct InvestigationValueTrendBucket
{
    QDateTime startTimestamp;
    QString label;
    QMap<QString, int> countsByValue;

    int countFor(
        const QString &value
        ) const
    {
        return countsByValue.value(
            value,
            0
            );
    }

    int totalCount() const
    {
        int total = 0;

        for (auto iterator =
             countsByValue.cbegin();
             iterator !=
             countsByValue.cend();
             ++iterator) {
            total += iterator.value();
        }

        return total;
    }
};