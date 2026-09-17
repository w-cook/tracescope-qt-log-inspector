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

    void verifiesCopiedSourceAsRelocationCandidate();
    void verifiesGrownSourceAsRelocationCandidate();
    void rejectsShorterRelocationCandidate();
    void rejectsChangedRelocationCandidatePrefix();
    void rejectsMissingRelocationCandidate();
    void rejectsRelocationWithoutHistoricalEvidence();
    void verifiesBirthTimeOnlyRelocationEvidence();
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

void SourcePhysicalIdentityTests::
    verifiesCopiedSourceAsRelocationCandidate()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral("original.log")
            );

    const QString candidatePath =
        directory.filePath(
            QStringLiteral("relocated.log")
            );

    const QByteArray content(
        6000,
        'A'
        );

    writeFile(
        originalPath,
        content
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            originalPath
            );

    QVERIFY2(
        captureResult.succeeded,
        qPrintable(
            captureResult.errorMessage
            )
        );

    writeFile(
        candidatePath,
        content
        );

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            candidatePath,
            captureResult.identity
            );

    QVERIFY2(
        result.verified,
        qPrintable(
            result.errorMessage
            )
        );

    QCOMPARE(
        result.candidateIdentity.observedSizeBytes,
        qint64(content.size())
        );
}

void SourcePhysicalIdentityTests::
    verifiesGrownSourceAsRelocationCandidate()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral("original.log")
            );

    const QString candidatePath =
        directory.filePath(
            QStringLiteral("relocated.log")
            );

    const QByteArray original(
        "historical source content\n"
        );

    writeFile(
        originalPath,
        original
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            originalPath
            );

    QVERIFY2(
        captureResult.succeeded,
        qPrintable(
            captureResult.errorMessage
            )
        );

    writeFile(
        candidatePath,
        original
        );

    appendFile(
        candidatePath,
        QByteArray(
            "newer content\n"
            )
        );

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            candidatePath,
            captureResult.identity
            );

    QVERIFY2(
        result.verified,
        qPrintable(
            result.errorMessage
            )
        );

    QVERIFY(
        result.candidateIdentity.observedSizeBytes
        > captureResult.identity.observedSizeBytes
        );
}

void SourcePhysicalIdentityTests::
    rejectsShorterRelocationCandidate()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral("original.log")
            );

    const QString candidatePath =
        directory.filePath(
            QStringLiteral("shorter.log")
            );

    const QByteArray original(
        6000,
        'A'
        );

    writeFile(
        originalPath,
        original
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            originalPath
            );

    QVERIFY(captureResult.succeeded);

    /*
     * This still contains the complete historical
     * 4096-byte fingerprint prefix but is shorter
     * than the previously observed source.
     */
    writeFile(
        candidatePath,
        original.left(5000)
        );

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            candidatePath,
            captureResult.identity
            );

    QVERIFY(
        !result.verified
        );
}

void SourcePhysicalIdentityTests::
    rejectsChangedRelocationCandidatePrefix()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString originalPath =
        directory.filePath(
            QStringLiteral("original.log")
            );

    const QString candidatePath =
        directory.filePath(
            QStringLiteral("different.log")
            );

    writeFile(
        originalPath,
        QByteArray(
            5000,
            'A'
            )
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            originalPath
            );

    QVERIFY(captureResult.succeeded);

    writeFile(
        candidatePath,
        QByteArray(
            5000,
            'B'
            )
        );

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            candidatePath,
            captureResult.identity
            );

    QVERIFY(
        !result.verified
        );
}

void SourcePhysicalIdentityTests::
    rejectsMissingRelocationCandidate()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    SourcePhysicalIdentity identity;

    identity.fingerprintLength = 1;
    identity.prefixFingerprint =
        QByteArray(
            32,
            'A'
            );

    identity.observedSizeBytes = 1;

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            directory.filePath(
                QStringLiteral(
                    "missing.log"
                    )
                ),
            identity
            );

    QVERIFY(
        !result.verified
        );
}

void SourcePhysicalIdentityTests::
    rejectsRelocationWithoutHistoricalEvidence()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString candidatePath =
        directory.filePath(
            QStringLiteral("candidate.log")
            );

    writeFile(
        candidatePath,
        QByteArray()
        );

    SourcePhysicalIdentity identity;

    identity.observedSizeBytes = 0;

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            candidatePath,
            identity
            );

    QVERIFY(
        !result.verified
        );
}

void SourcePhysicalIdentityTests::
    verifiesBirthTimeOnlyRelocationEvidence()
{
    QTemporaryDir directory;

    QVERIFY(directory.isValid());

    const QString sourcePath =
        directory.filePath(
            QStringLiteral("empty.log")
            );

    writeFile(
        sourcePath,
        QByteArray()
        );

    const SourcePhysicalIdentityCaptureResult
        captureResult =
        captureSourcePhysicalIdentity(
            sourcePath
            );

    QVERIFY(captureResult.succeeded);

    if (!captureResult
             .identity
             .birthTime
             .isValid()) {
        QSKIP(
            "Filesystem does not expose a usable birth time."
            );
    }

    SourcePhysicalIdentity birthTimeOnly =
        captureResult.identity;

    birthTimeOnly.fingerprintLength = 0;
    birthTimeOnly.prefixFingerprint.clear();

    const SourceRelocationVerificationResult
        result =
        verifySourceRelocationCandidate(
            sourcePath,
            birthTimeOnly
            );

    QVERIFY2(
        result.verified,
        qPrintable(
            result.errorMessage
            )
        );
}

QTEST_MAIN(
    SourcePhysicalIdentityTests
    )

#include "SourcePhysicalIdentityTests.moc"