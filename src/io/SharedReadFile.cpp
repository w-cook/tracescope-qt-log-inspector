#include "SharedReadFile.h"

#ifdef Q_OS_WIN

#include <cerrno>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <io.h>

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include <QDir>

namespace
{
QString windowsErrorString(
    DWORD errorCode
    )
{
    wchar_t buffer[1024] {};

    const DWORD length =
        FormatMessageW(
            FORMAT_MESSAGE_FROM_SYSTEM
                | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr,
            errorCode,
            0,
            buffer,
            static_cast<DWORD>(
                sizeof(buffer)
                / sizeof(buffer[0])
                ),
            nullptr
            );

    if (length == 0) {
        return QStringLiteral(
                   "Windows error %1."
                   )
            .arg(
                errorCode
                );
    }

    return QString::fromWCharArray(
               buffer,
               static_cast<int>(
                   length
                   )
               )
        .trimmed();
}
}

#endif

SharedReadFileOpenResult openSharedReadFile(
    QFile &file,
    const QString &filePath,
    QIODeviceBase::OpenMode mode
    )
{
    SharedReadFileOpenResult result;

    /*
     * This helper is intentionally limited to passive
     * read access. TraceScope must never use this path
     * to modify an externally owned live source.
     */
    if (!(mode & QIODevice::ReadOnly)
        || (mode & QIODevice::WriteOnly)) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "Shared source access only supports "
                "read-only file modes."
                );

        return result;
    }

#ifndef Q_OS_WIN

    file.setFileName(
        filePath
        );

    if (!file.open(
            mode
            )) {
        result.succeeded = false;

        result.errorMessage =
            file.errorString();
    }

    return result;

#else

    /*
     * Windows file sharing is established when the
     * handle is opened.
     *
     * TraceScope is a passive observer, so its read
     * handle must permit an external producer to:
     *
     * - continue reading the same file
     * - continue writing/appending
     * - rename, rotate, delete, or replace the path
     */
    const QString nativePath =
        QDir::toNativeSeparators(
            filePath
            );

    const HANDLE handle =
        CreateFileW(
            reinterpret_cast<LPCWSTR>(
                nativePath.utf16()
                ),
            GENERIC_READ,
            FILE_SHARE_READ
                | FILE_SHARE_WRITE
                | FILE_SHARE_DELETE,
            nullptr,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
            );

    if (handle
        == INVALID_HANDLE_VALUE) {
        result.succeeded = false;

        result.errorMessage =
            windowsErrorString(
                GetLastError()
                );

        return result;
    }

    /*
     * QFile adopts CRT file descriptors rather than
     * native Windows HANDLE values.
     *
     * _open_osfhandle transfers ownership of the
     * HANDLE to the returned descriptor when it
     * succeeds.
     */
    const int descriptor =
        _open_osfhandle(
            reinterpret_cast<intptr_t>(
                handle
                ),
            _O_RDONLY
                | _O_BINARY
            );

    if (descriptor == -1) {
        result.succeeded = false;

        result.errorMessage =
            QStringLiteral(
                "Could not adopt the Windows "
                "shared-read handle: %1"
                )
                .arg(
                    QString::fromLocal8Bit(
                        std::strerror(
                            errno
                            )
                        )
                    );

        /*
         * Ownership was not transferred because
         * _open_osfhandle failed.
         */
        CloseHandle(
            handle
            );

        return result;
    }

    /*
     * QFile takes ownership of the CRT descriptor
     * only if this open succeeds.
     */
    if (!file.open(
            descriptor,
            mode,
            QFileDevice::AutoCloseHandle
            )) {
        result.succeeded = false;

        result.errorMessage =
            file.errorString();

        /*
         * QFile did not adopt the descriptor, so we
         * still own it here. Closing the descriptor
         * also closes the underlying HANDLE.
         */
        _close(
            descriptor
            );

        return result;
    }

    return result;

#endif
}