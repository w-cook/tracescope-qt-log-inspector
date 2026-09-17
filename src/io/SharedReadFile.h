#pragma once

#include <QFile>
#include <QString>

struct SharedReadFileOpenResult
{
    bool succeeded = true;
    QString errorMessage;
};

/*
 * Opens an externally owned investigation source for
 * passive read access.
 *
 * TraceScope must not prevent the source-producing
 * application from continuing to read, write, append,
 * rename, rotate, delete, or replace the source while
 * TraceScope has it open.
 *
 * On Windows this requires explicit read/write/delete
 * sharing. On platforms whose normal read handles
 * already permit those operations, ordinary QFile
 * semantics are used.
 */
SharedReadFileOpenResult openSharedReadFile(
    QFile &file,
    const QString &filePath,
    QIODeviceBase::OpenMode mode =
    QIODevice::ReadOnly
    );