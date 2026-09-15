#include "InvestigationSessionSnapshotFile.h"

#include <QFile>
#include <QSaveFile>

#include "InvestigationSessionSnapshotSerialization.h"
#include "InvestigationSnapshotContainer.h"

namespace
{
InvestigationSessionSnapshotSaveResult
saveFailure(
    const QString &code,
    const QString &message
    )
{
    InvestigationSessionSnapshotSaveResult
        result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}

InvestigationSessionSnapshotLoadResult
loadFailure(
    const QString &code,
    const QString &message
    )
{
    InvestigationSessionSnapshotLoadResult
        result;

    result.errorCode = code;
    result.errorMessage = message;

    return result;
}
}

InvestigationSessionSnapshotSaveResult
InvestigationSessionSnapshotFile::save(
    const QString &filePath,
    const InvestigationSessionSnapshot &snapshot
    ) const
{
    const InvestigationSessionSnapshotSerializer
        serializer;

    const QByteArray payload =
        serializer.serialize(
            snapshot
            );

    const InvestigationSnapshotContainer
        containerCodec;

    const QByteArray container =
        containerCodec.encode(
            payload
            );

    QSaveFile file(
        filePath
        );

    if (!file.open(
            QIODevice::WriteOnly
            )) {
        return saveFailure(
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "could not be opened for writing: %1"
                )
                .arg(
                    file.errorString()
                    )
            );
    }

    const qint64 written =
        file.write(
            container
            );

    if (written
        != container.size()) {
        const QString errorMessage =
            file.errorString();

        file.cancelWriting();

        return saveFailure(
            QStringLiteral(
                "FILE_WRITE_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "could not be written completely: %1"
                )
                .arg(
                    errorMessage
                    )
            );
    }

    /*
     * QSaveFile writes to a temporary file and only
     * replaces the destination during commit(), so a
     * failed save does not overwrite a previously
     * valid investigation snapshot.
     */
    if (!file.commit()) {
        return saveFailure(
            QStringLiteral(
                "FILE_COMMIT_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "could not be committed atomically: %1"
                )
                .arg(
                    file.errorString()
                    )
            );
    }

    InvestigationSessionSnapshotSaveResult
        result;

    result.succeeded = true;

    return result;
}

InvestigationSessionSnapshotLoadResult
InvestigationSessionSnapshotFile::load(
    const QString &filePath
    ) const
{
    QFile file(
        filePath
        );

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        return loadFailure(
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "could not be opened for reading: %1"
                )
                .arg(
                    file.errorString()
                    )
            );
    }

    const QByteArray container =
        file.readAll();

    if (file.error()
        != QFileDevice::NoError) {
        return loadFailure(
            QStringLiteral(
                "FILE_READ_FAILED"
                ),
            QStringLiteral(
                "The investigation snapshot "
                "could not be read completely: %1"
                )
                .arg(
                    file.errorString()
                    )
            );
    }

    const InvestigationSnapshotContainer
        containerCodec;

    const InvestigationSnapshotContainerDecodeResult
        containerResult =
        containerCodec.decode(
            container
            );

    if (!containerResult.isSuccess()) {
        return loadFailure(
            containerResult.errorCode,
            containerResult.errorMessage
            );
    }

    const InvestigationSessionSnapshotSerializer
        serializer;

    const InvestigationSessionSnapshotDeserializationResult
        snapshotResult =
        serializer.deserialize(
            *containerResult.payload
            );

    if (!snapshotResult.isSuccess()) {
        return loadFailure(
            snapshotResult.errorCode,
            snapshotResult.errorMessage
            );
    }

    InvestigationSessionSnapshotLoadResult
        result;

    result.snapshot =
        std::move(
            *snapshotResult.snapshot
            );

    return result;
}