#pragma once

#include <QDateTime>
#include <QString>
#include <QVector>

#include "../controllers/InvestigationController.h"
#include "../importing/ImportDiagnostic.h"
#include "../importing/ImportProfile.h"
#include "../importing/ImportResult.h"

struct InvestigationSessionSourceMetadata
{
    QString sourcePath;
    QString sourceName;

    qint64 sourceSizeBytes = 0;

    QDateTime sourceLastModified;
    QDateTime importedAtUtc;
};

class InvestigationSession
{
public:
    InvestigationSession(
        const QString &filePath,
        ImportProfile profile,
        ImportResult result
        );

    const QString &id() const;

    const InvestigationSessionSourceMetadata &
    sourceMetadata() const;

    const ImportProfile &importProfile() const;

    const QVector<ImportDiagnostic> &
    diagnostics() const;

    qint64 processedRecordCount() const;
    qint64 importedRecordCount() const;
    qint64 skippedRecordCount() const;

    bool sourceTruncated() const;

    InvestigationController *
    investigationController();

private:
    QString m_id;

    InvestigationSessionSourceMetadata
        m_sourceMetadata;

    ImportProfile m_importProfile;

    QVector<ImportDiagnostic>
        m_diagnostics;

    qint64 m_processedRecordCount = 0;

    bool m_sourceTruncated = false;

    InvestigationController
        m_investigationController;
};