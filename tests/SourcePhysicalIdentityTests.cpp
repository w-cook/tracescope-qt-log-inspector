#include <QtTest>

#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>

#include "../src/sources/SourcePhysicalIdentity.h"

namespace
{
void writeFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}

void appendFile(
    const QString &path,
    const QByteArray &content
    )
{
    QFile file(path);

    QVERIFY(
        file.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            )
        );

    QCOMPARE(
        file.write(content),
        qint64(content.size())
        );

    file.close();
}
}

class SourcePhysicalIdentityTests
    : public QObject
{
    Q_OBJECT

private slots:
    void capturesNonEmptySourceIdentity();
    void ordinaryAppendDoesNotChangeIdentity();
    void samePathReplacementChangesIdentity();
    void truncationBelowCapturedSizeChangesIdentity();
};

void SourcePhysicalIdentityTests::
    capturesNonEmptySourceIdentity()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    const QByteArray content(
        "first record\n"
        "second record\n"
        );

    writeFile(
        sourcePath,
        content
        );

    const SourcePhysicalIdentityCaptureResult
        result =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        result.succeeded,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.identity.isValid()
        );

    QCOMPARE(
        result.identity.observedSizeBytes,
        qint64(content.size())
        );

    QCOMPARE(
        result.identity.fingerprintLength,
        qint64(content.size())
        );

    QVERIFY(
        !result
             .identity
             .prefixFingerprint
             .isEmpty()
        );
}

void SourcePhysicalIdentityTests::
    ordinaryAppendDoesNotChangeIdentity()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    const QByteArray original(
        "first record\n"
        "second record\n"
        );

    writeFile(
        sourcePath,
        original
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        captureResult.succeeded,
        qPrintable(
            captureResult.errorMessage
            )
        );

    appendFile(
        sourcePath,
        QByteArray(
            "third record\n"
            )
        );

    QVERIFY(
        !sourcePhysicalIdentityChanged(
            sourcePath,
            captureResult.identity
            )
        );
}

void SourcePhysicalIdentityTests::
    samePathReplacementChangesIdentity()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    const QString rotatedPath =
        directory.filePath(
            QStringLiteral(
                "live.log.old"
                )
            );

    const QByteArray original(
        "original generation record one\n"
        "original generation record two\n"
        );

    writeFile(
        sourcePath,
        original
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        captureResult.succeeded,
        qPrintable(
            captureResult.errorMessage
            )
        );

    /*
     * Move the original physical file away and create
     * a new physical file at the same active path.
     *
     * This models producer rotation/replacement rather
     * than merely editing the existing file contents.
     */
    QVERIFY(
        QFile::rename(
            sourcePath,
            rotatedPath
            )
        );

    const QByteArray replacement(
        "replacement generation record\n"
        );

    writeFile(
        sourcePath,
        replacement
        );

    QVERIFY(
        sourcePhysicalIdentityChanged(
            sourcePath,
            captureResult.identity
            )
        );
}

void SourcePhysicalIdentityTests::
    truncationBelowCapturedSizeChangesIdentity()
{
    QTemporaryDir directory;

    QVERIFY(
        directory.isValid()
        );

    const QString sourcePath =
        directory.filePath(
            QStringLiteral(
                "live.log"
                )
            );

    const QByteArray original(
        "original generation record one\n"
        "original generation record two\n"
        );

    writeFile(
        sourcePath,
        original
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY2(
        captureResult.succeeded,
        qPrintable(
            captureResult.errorMessage
            )
        );

    const QByteArray truncated(
        "new\n"
        );

    QVERIFY(
        truncated.size()
        < original.size()
        );

    writeFile(
        sourcePath,
        truncated
        );

    QVERIFY(
        sourcePhysicalIdentityChanged(
            sourcePath,
            captureResult.identity
            )
        );
}

QTEST_MAIN(
    SourcePhysicalIdentityTests
    )

#include "SourcePhysicalIdentityTests.moc"