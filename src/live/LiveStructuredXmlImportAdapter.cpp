#include "LiveStructuredXmlImportAdapter.h"

#include <utility>

#include <QFileInfo>

#include "../importing/JsonObjectRecordMapper.h"

namespace
{
RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber,
    quint64 sourceGeneration
    )
{
    RecordSourceMetadata source;

    source.sourcePath =
        sourcePath;

    source.recordNumber =
        recordNumber;

    source.sourceGeneration =
        sourceGeneration;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(
                sourcePath
                ).fileName();
    }

    return source;
}
}

LiveStructuredXmlImportAdapter::
    LiveStructuredXmlImportAdapter(
        ImportProfile profile
        )
    : m_profile(
          std::move(profile)
          )
{
}

bool LiveStructuredXmlImportAdapter::
    isSupported() const
{
    return m_profile
                   .importerId
                   .trimmed()
                   .compare(
                       QStringLiteral("xml"),
                       Qt::CaseInsensitive
                       )
               == 0
           && !m_profile
                   .recordPath
                   .trimmed()
                   .isEmpty();
}

ImportResult
    LiveStructuredXmlImportAdapter::
    importBatch(
        const LiveStructuredXmlRecordBatch &batch,
        const QString &sourcePath
        ) const
{
    ImportResult result;

    if (!isSupported()) {
        return result;
    }

    for (qsizetype index = 0;
         index < batch.records.size();
         ++index) {
        ++result.processedRecordCount;

        const qint64 recordNumber =
            batch.firstRecordNumber
            + index;

        const RecordSourceMetadata source =
            createSourceMetadata(
                sourcePath,
                recordNumber,
                batch.sourceGeneration
                );

        const StructuredXmlRecord &record =
            batch.records.at(index);

        result.records.append(
            JsonObjectRecordMapper::mapRecord(
                record.object,
                record.rawSource,
                source,
                m_profile,
                result
                )
            );
    }

    return result;
}