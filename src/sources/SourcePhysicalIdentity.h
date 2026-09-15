#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QString>
#include <QtTypes>

struct SourcePhysicalIdentity
{
    QDateTime birthTime;

    qint64 fingerprintLength = 0;

    QByteArray prefixFingerprint;

    /*
     * Size observed when the identity was captured.
     * This gives persisted Hybrid bindings a simple,
     * conservative truncation signal after reopen.
     */
    qint64 observedSizeBytes = 0;

    bool isValid() const
    {
        return observedSizeBytes >= 0
               && (
                   birthTime.isValid()
                   || fingerprintLength > 0
                   );
    }
};

struct SourcePhysicalIdentityCaptureResult
{
    SourcePhysicalIdentity identity;

    bool succeeded = false;

    QString errorMessage;
};

SourcePhysicalIdentityCaptureResult
captureSourcePhysicalIdentity(
    const QString &sourcePath
    );

bool sourcePhysicalIdentityChanged(
    const QString &sourcePath,
    const SourcePhysicalIdentity &identity
    );