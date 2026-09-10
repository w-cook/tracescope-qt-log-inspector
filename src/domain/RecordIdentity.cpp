#include "RecordIdentity.h"

#include <QByteArray>
#include <QCryptographicHash>
#include <QDataStream>
#include <QDir>
#include <QIODevice>

QString createStableRecordIdentity(
    const RecordSourceMetadata &source,
    const QString &rawSource
    )
{
    QString sourceKey = source.sourcePath.trimmed();

    if (sourceKey.isEmpty()) {
        sourceKey = source.sourceName.trimmed();
    }

    sourceKey.replace('\\', '/');
    sourceKey = QDir::cleanPath(sourceKey);

    QByteArray identityMaterial;
    QDataStream stream(&identityMaterial, QIODevice::WriteOnly);
    stream.setVersion(QDataStream::Qt_6_0);

    stream << sourceKey;

    /*
     * Preserve the original identity material exactly
     * for ordinary/static records. This keeps existing
     * generation-zero record IDs stable across the
     * Phase 15 change.
     */
    if (source.sourceGeneration > 0) {
        stream << QStringLiteral(
            "source-generation"
            );

        stream << source.sourceGeneration;
    }

    stream << source.recordNumber;
    stream << rawSource;

    const QByteArray hash = QCryptographicHash::hash(
        identityMaterial,
        QCryptographicHash::Sha256
        );

    return QString::fromLatin1(hash.toHex());
}