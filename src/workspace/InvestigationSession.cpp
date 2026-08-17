#include "InvestigationSession.h"

#include <QFileInfo>
#include <QUuid>

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
        ),
    m_diagnostics(
        std::move(result.diagnostics)
        ),
    m_processedRecordCount(
        result.processedRecordCount
        ),
    m_sourceTruncated(
        result.sourceTruncated
        )
{
    const QFileInfo fileInfo(filePath);

    m_sourceMetadata.sourcePath =
        fileInfo.absoluteFilePath();

    m_sourceMetadata.sourceName =
        fileInfo.fileName();

    if (fileInfo.exists()) {
        m_sourceMetadata.sourceSizeBytes =
            fileInfo.size();

        m_sourceMetadata.sourceLastModified =
            fileInfo.lastModified();
    }

    m_sourceMetadata.importedAtUtc =
        QDateTime::currentDateTimeUtc();

    m_investigationController.setRecords(
        result.records
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