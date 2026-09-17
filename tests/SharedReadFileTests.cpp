#include <QtTest>

#include <QFile>
#include <QTemporaryDir>

#include "../src/io/SharedReadFile.h"

namespace
{
bool writeFile(
    const QString &path,
    const QByteArray &contents
    )
{
    QFile file(
        path
        );

    if (!file.open(
            QIODevice::WriteOnly
            | QIODevice::Truncate
            )) {
        return false;
    }

    return file.write(
               contents
               )
           == contents.size();
}
}

class SharedReadFileTests : public QObject
{
    Q_OBJECT

private slots:
    void readerAllowsConcurrentAppend();
    void readerAllowsReplacement();
    void readerAllowsRotation();
};

void SharedReadFileTests::
    readerAllowsConcurrentAppend()
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

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArray(
                "first\n"
                )
            )
        );

    QFile reader;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            reader,
            sourcePath
            );

    QVERIFY2(
        openResult.succeeded,
        qPrintable(
            openResult.errorMessage
            )
        );

    QFile writer(
        sourcePath
        );

    QVERIFY2(
        writer.open(
            QIODevice::WriteOnly
            | QIODevice::Append
            ),
        qPrintable(
            writer.errorString()
            )
        );

    QCOMPARE(
        writer.write(
            QByteArray(
                "second\n"
                )
            ),
        qint64(7)
        );
}

void SharedReadFileTests::
    readerAllowsReplacement()
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

    const QByteArray originalContents(
        "original\n"
        );

    QVERIFY(
        writeFile(
            sourcePath,
            originalContents
            )
        );

    QFile reader;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            reader,
            sourcePath
            );

    QVERIFY2(
        openResult.succeeded,
        qPrintable(
            openResult.errorMessage
            )
        );

    /*
     * Simulate an external producer replacing the
     * active pathname while TraceScope still owns a
     * read handle to the previous generation.
     */
    QVERIFY2(
        QFile::remove(
            sourcePath
            ),
        "The active source could not be removed "
        "while the TraceScope reader was open."
        );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArray(
                "replacement\n"
                )
            )
        );

    /*
     * The existing reader should continue referring
     * to the old physical generation.
     */
    QVERIFY(
        reader.seek(
            0
            )
        );

    QCOMPARE(
        reader.readAll(),
        originalContents
        );
}

void SharedReadFileTests::
    readerAllowsRotation()
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
                "live.log.1"
                )
            );

    const QByteArray originalContents(
        "original\n"
        );

    QVERIFY(
        writeFile(
            sourcePath,
            originalContents
            )
        );

    QFile reader;

    const SharedReadFileOpenResult
        openResult =
        openSharedReadFile(
            reader,
            sourcePath
            );

    QVERIFY2(
        openResult.succeeded,
        qPrintable(
            openResult.errorMessage
            )
        );

    QVERIFY2(
        QFile::rename(
            sourcePath,
            rotatedPath
            ),
        "The active source could not be rotated "
        "while the TraceScope reader was open."
        );

    QVERIFY(
        writeFile(
            sourcePath,
            QByteArray(
                "new active\n"
                )
            )
        );

    QVERIFY(
        reader.seek(
            0
            )
        );

    QCOMPARE(
        reader.readAll(),
        originalContents
        );
}

QTEST_MAIN(
    SharedReadFileTests
    )

#include "SharedReadFileTests.moc"