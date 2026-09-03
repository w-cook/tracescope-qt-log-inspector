#pragma once

#include <QString>
#include <QStringList>

struct InvestigationReportConfiguration
{
    /*
     * Human-facing report identification.
     *
     * title should normally be pre-populated by the
     * export dialog from the document that initiated
     * the export, but remains editable.
     *
     * context is optional investigator-provided text
     * explaining the run, failure, test, incident,
     * or other purpose of the investigation.
     */
    QString title;
    QString context;

    /*
     * Selected logical workspace documents.
     *
     * These IDs deliberately represent workspace
     * documents rather than windows or presentation
     * state. Detached and docked documents therefore
     * behave identically during report generation.
     *
     * Order is retained so report generation can
     * preserve the user's selection/display order
     * deterministically.
     */
    QStringList sessionIds;
    QStringList comparisonIds;

    /*
     * Most investigation data is part of the report
     * whenever supported. These two options represent
     * the intentionally high-volume/detail-heavy
     * portions that are reasonable for the user to
     * control explicitly.
     */
    bool includeSupportingEvidence = true;

    bool includeTechnicalAppendix = true;

    bool hasSelectedDocuments() const
    {
        return !sessionIds.isEmpty()
        || !comparisonIds.isEmpty();
    }
};