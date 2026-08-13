#pragma once

#include <QJsonObject>
#include <QString>

#include "../domain/InvestigationRecord.h"
#include "../domain/RecordSourceMetadata.h"
#include "ImportProfile.h"
#include "ImportResult.h"

class JsonObjectRecordMapper
{
public:
    static InvestigationRecord mapRecord(
        const QJsonObject &object,
        const QString &rawSource,
        const RecordSourceMetadata &source,
        const ImportProfile &profile,
        ImportResult &result
        );
};