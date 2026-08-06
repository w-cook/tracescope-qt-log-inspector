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
    stream << source.recordNumber;
    stream << rawSource;

    const QByteArray hash = QCryptographicHash::hash(
        identityMaterial,
        QCryptographicHash::Sha256
        );

    return QString::fromLatin1(hash.toHex());
}