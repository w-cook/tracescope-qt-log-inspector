#include "InvestigationSession.h"

#include <QFileInfo>
#include <QUuid>
#include <QSet>

#include <algorithm>
#include <utility>

InvestigationSession::InvestigationSession(
    const QString &filePath,
    ImportProfile profile,
    ImportResult result
    )
    : m_id(
          QUuid::createUuid().toString(
              QUuid::WithoutBraces
              )
          ),
    m_importProfile(
        std::move(profile)
        )
{
    const QFileInfo fileInfo(filePath);

    m_sourceMetadata.sourcePath =
        fileInfo.absoluteFilePath();

    refreshSourceMetadata();

    installImportResult(
        std::move(result)
        );
}

const QString &
InvestigationSession::id() const
{
    return m_id;
}

const InvestigationSessionSourceMetadata &
InvestigationSession::sourceMetadata() const
{
    return m_sourceMetadata;
}

const ImportProfile &
InvestigationSession::importProfile() const
{
    return m_importProfile;
}

const QVector<ImportDiagnostic> &
InvestigationSession::diagnostics() const
{
    return m_diagnostics;
}

qint64 InvestigationSession::
    processedRecordCount() const
{
    return m_processedRecordCount;
}

qint64 InvestigationSession::
    importedRecordCount() const
{
    return m_investigationController
        .totalRecordCount();
}

qint64 InvestigationSession::
    skippedRecordCount() const
{
    return std::max<qint64>(
        0,
        m_processedRecordCount
            - importedRecordCount()
        );
}

bool InvestigationSession::
    sourceTruncated() const
{
    return m_sourceTruncated;
}

InvestigationController *
    InvestigationSession::
    investigationController()
{
    return &m_investigationController;
}

void InvestigationSession::reload(
    ImportResult result
    )
{
    refreshSourceMetadata();

    installImportResult(
        std::move(result)
        );
}

bool InvestigationSession::
    hasSeverityData() const
{
    return m_hasSeverityData;
}

bool InvestigationSession::
    hasSubsystemData() const
{
    return m_hasSubsystemData;
}

const QStringList &
    InvestigationSession::
    availableSubsystems() const
{
    return m_availableSubsystems;
}

bool InvestigationSession::
    hasCustomFieldData() const
{
    return m_hasCustomFieldData;
}

const QStringList &
    InvestigationSession::
    availableCustomFields() const
{
    return m_availableCustomFields;
}

bool InvestigationSession::
    hasEventCodeData() const
{
    return m_hasEventCodeData;
}

bool InvestigationSession::
    hasEntityData() const
{
    return m_hasEntityData;
}

const QStringList &
    InvestigationSession::
    availableEventCodes() const
{
    return m_availableEventCodes;
}

const QStringList &
    InvestigationSession::
    availableEntities() const
{
    return m_availableEntities;
}

const std::optional<QDateTime> &
    InvestigationSession::
    firstTimestamp() const
{
    return m_firstTimestamp;
}

const std::optional<QDateTime> &
    InvestigationSession::
    lastTimestamp() const
{
    return m_lastTimestamp;
}

const QVector<int> &
    InvestigationSession::
    columnWidths() const
{
    return m_columnWidths;
}

void InvestigationSession::setColumnWidths(
    QVector<int> widths
    )
{
    m_columnWidths =
        std::move(widths);
}

void InvestigationSession::
    refreshSourceMetadata()
{
    const QFileInfo fileInfo(
        m_sourceMetadata.sourcePath
        );

    m_sourceMetadata.sourceName =
        fileInfo.fileName();

    m_sourceMetadata.sourceSizeBytes =
        fileInfo.exists()
            ? fileInfo.size()
            : 0;

    m_sourceMetadata.sourceLastModified =
        fileInfo.exists()
            ? fileInfo.lastModified()
            : QDateTime();

    m_sourceMetadata.importedAtUtc =
        QDateTime::currentDateTimeUtc();
}

void InvestigationSession::installImportResult(
    ImportResult result
    )
{
    m_diagnostics =
        std::move(result.diagnostics);

    m_processedRecordCount =
        result.processedRecordCount;

    m_sourceTruncated =
        result.sourceTruncated;

    rebuildDerivedData(
        result.records
        );

    m_investigationController.setRecords(
        result.records
        );

    /*
     * A reload may change the available columns
     * or their representative content.
     */
    m_columnWidths.clear();
}

void InvestigationSession::rebuildDerivedData(
    const QVector<InvestigationRecord> &records
    )
{
    m_hasSeverityData = false;
    m_hasSubsystemData = false;

    m_availableSubsystems.clear();

    m_hasEventCodeData = false;
    m_hasEntityData = false;

    m_hasCustomFieldData = false;
    m_availableCustomFields.clear();

    m_availableEventCodes.clear();
    m_availableEntities.clear();

    QSet<QString> eventCodes;
    QSet<QString> entities;

    m_firstTimestamp.reset();
    m_lastTimestamp.reset();

    QSet<QString> subsystems;
    QSet<QString> customFields;

    for (const InvestigationRecord &record
         : records) {
        m_hasSeverityData =
            m_hasSeverityData
            || record.severity.has_value();

        if (record.subsystem.has_value()
            && !record.subsystem->isEmpty()) {
            m_hasSubsystemData = true;

            subsystems.insert(
                record.subsystem.value()
                );
        }

        if (record.eventCode.has_value()
            && !record.eventCode
                    ->trimmed()
                    .isEmpty()) {
            m_hasEventCodeData = true;

            eventCodes.insert(
                record.eventCode.value()
                );
        }

        if (record.entityId.has_value()
            && !record.entityId
                    ->trimmed()
                    .isEmpty()) {
            m_hasEntityData = true;

            entities.insert(
                record.entityId.value()
                );
        }

        for (
            auto iterator =
            record.customAttributes.constBegin();
            iterator !=
            record.customAttributes.constEnd();
            ++iterator
            ) {
            if (iterator.key().isEmpty()) {
                continue;
            }

            m_hasCustomFieldData = true;

            customFields.insert(
                iterator.key()
                );
        }

        if (!record.timestamp.has_value()) {
            continue;
        }

        const QDateTime timestamp =
            record.timestamp->toUTC();

        if (!timestamp.isValid()) {
            continue;
        }

        if (!m_firstTimestamp.has_value()
            || timestamp < *m_firstTimestamp) {
            m_firstTimestamp = timestamp;
        }

        if (!m_lastTimestamp.has_value()
            || timestamp > *m_lastTimestamp) {
            m_lastTimestamp = timestamp;
        }
    }

    m_availableSubsystems =
        subsystems.values();

    m_availableEventCodes =
        eventCodes.values();

    m_availableCustomFields =
        customFields.values();

    std::sort(
        m_availableCustomFields.begin(),
        m_availableCustomFields.end(),
        [](
            const QString &left,
            const QString &right
            ) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );

    m_availableEntities =
        entities.values();

    std::sort(
        m_availableSubsystems.begin(),
        m_availableSubsystems.end(),
        [](const QString &left,
           const QString &right) {
            return left.compare(
                       right,
                       Qt::CaseInsensitive
                       ) < 0;
        }
        );
}