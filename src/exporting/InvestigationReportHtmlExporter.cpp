#include "InvestigationReportHtmlExporter.h"

#include <QByteArray>
#include <QSaveFile>

#include "InvestigationReportHtmlRenderer.h"

bool InvestigationReportHtmlExporter::
    exportToFile(
        const InvestigationReportSnapshot &snapshot,
        const QString &filePath
        ) const
{
    if (filePath.trimmed().isEmpty()) {
        return false;
    }

    /*
     * Render exclusively from the immutable report
     * snapshot captured before filesystem interaction.
     */
    const InvestigationReportHtmlRenderer renderer;

    const QByteArray html =
        renderer
            .render(snapshot)
            .toUtf8();

    QSaveFile file(
        filePath
        );

    if (!file.open(
            QIODevice::WriteOnly
            )) {
        return false;
    }

    const qint64 bytesWritten =
        file.write(
            html
            );

    if (
        bytesWritten
        != html.size()
        ) {
        file.cancelWriting();

        return false;
    }

    /*
     * QSaveFile replaces the destination only after
     * the complete temporary output has been written
     * successfully.
     */
    return file.commit();
}