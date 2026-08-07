#pragma once

#include <QString>

#include "RecordSourceMetadata.h"

QString createStableRecordIdentity(
    const RecordSourceMetadata &source,
    const QString &rawSource
    );