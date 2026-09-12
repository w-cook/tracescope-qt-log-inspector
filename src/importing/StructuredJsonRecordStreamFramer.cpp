#include "StructuredJsonRecordStreamFramer.h"

#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QStringList>

namespace
{
enum class PrefixParseStatus
{
    Complete,
    NeedMore,
    Found,
    Invalid
};

struct RecordArrayLocationResult
{
    PrefixParseStatus status =
        PrefixParseStatus::NeedMore;

    qsizetype arrayStart = -1;

    QString errorMessage;
};

void skipWhitespace(
    const QByteArray &bytes,
    qsizetype &index
    )
{
    while (index < bytes.size()) {
        const char character =
            bytes.at(index);

        if (character == ' '
            || character == '\t'
            || character == '\r'
            || character == '\n') {
            ++index;
            continue;
        }

        break;
    }
}

PrefixParseStatus parseJsonString(
    const QByteArray &bytes,
    qsizetype &index,
    QString &value,
    QString &errorMessage
    )
{
    if (index >= bytes.size()) {
        return PrefixParseStatus::NeedMore;
    }

    if (bytes.at(index) != '"') {
        errorMessage =
            QStringLiteral(
                "Expected a JSON string."
                );

        return PrefixParseStatus::Invalid;
    }

    const qsizetype startIndex =
        index;

    bool escaped = false;

    ++index;

    while (index < bytes.size()) {
        const char character =
            bytes.at(index);

        if (escaped) {
            escaped = false;
            ++index;
            continue;
        }

        if (character == '\\') {
            escaped = true;
            ++index;
            continue;
        }

        if (character == '"') {
            const qsizetype endIndex =
                index;

            ++index;

            QByteArray wrapped;

            wrapped.reserve(
                endIndex
                - startIndex
                + 3
                );

            wrapped.append('[');

            wrapped.append(
                bytes.mid(
                    startIndex,
                    endIndex
                        - startIndex
                        + 1
                    )
                );

            wrapped.append(']');

            QJsonParseError parseError;

            const QJsonDocument document =
                QJsonDocument::fromJson(
                    wrapped,
                    &parseError
                    );

            if (parseError.error
                    != QJsonParseError::NoError
                || !document.isArray()
                || document.array().size() != 1
                || !document.array()
                        .at(0)
                        .isString()) {
                errorMessage =
                    QStringLiteral(
                        "A JSON string in the "
                        "structured source is malformed."
                        );

                return PrefixParseStatus::Invalid;
            }

            value =
                document.array()
                    .at(0)
                    .toString();

            return PrefixParseStatus::Complete;
        }

        ++index;
    }

    return PrefixParseStatus::NeedMore;
}

PrefixParseStatus parseValue(
    const QByteArray &bytes,
    qsizetype &index,
    const QStringList &targetPath,
    int matchedPathDepth,
    qsizetype &arrayStart,
    QString &errorMessage
    );

PrefixParseStatus parseObject(
    const QByteArray &bytes,
    qsizetype &index,
    const QStringList &targetPath,
    int matchedPathDepth,
    qsizetype &arrayStart,
    QString &errorMessage
    )
{
    ++index;

    while (true) {
        skipWhitespace(
            bytes,
            index
            );

        if (index >= bytes.size()) {
            return PrefixParseStatus::NeedMore;
        }

        if (bytes.at(index) == '}') {
            ++index;

            return PrefixParseStatus::Complete;
        }

        QString key;

        const PrefixParseStatus keyStatus =
            parseJsonString(
                bytes,
                index,
                key,
                errorMessage
                );

        if (keyStatus
            != PrefixParseStatus::Complete) {
            return keyStatus;
        }

        skipWhitespace(
            bytes,
            index
            );

        if (index >= bytes.size()) {
            return PrefixParseStatus::NeedMore;
        }

        if (bytes.at(index) != ':') {
            errorMessage =
                QStringLiteral(
                    "Expected ':' after a JSON "
                    "object key."
                    );

            return PrefixParseStatus::Invalid;
        }

        ++index;

        int childMatchedDepth = -1;

        if (matchedPathDepth >= 0
            && matchedPathDepth
                   < targetPath.size()
            && key
                   == targetPath.at(
                       matchedPathDepth
                       )) {
            childMatchedDepth =
                matchedPathDepth + 1;
        }

        const PrefixParseStatus valueStatus =
            parseValue(
                bytes,
                index,
                targetPath,
                childMatchedDepth,
                arrayStart,
                errorMessage
                );

        if (valueStatus
                == PrefixParseStatus::Found
            || valueStatus
                   == PrefixParseStatus::NeedMore
            || valueStatus
                   == PrefixParseStatus::Invalid) {
            return valueStatus;
        }

        skipWhitespace(
            bytes,
            index
            );

        if (index >= bytes.size()) {
            return PrefixParseStatus::NeedMore;
        }

        if (bytes.at(index) == ',') {
            ++index;
            continue;
        }

        if (bytes.at(index) == '}') {
            ++index;

            return PrefixParseStatus::Complete;
        }

        errorMessage =
            QStringLiteral(
                "Expected ',' or '}' after a JSON "
                "object value."
                );

        return PrefixParseStatus::Invalid;
    }
}

PrefixParseStatus parseArray(
    const QByteArray &bytes,
    qsizetype &index,
    const QStringList &targetPath,
    qsizetype &arrayStart,
    QString &errorMessage
    )
{
    ++index;

    while (true) {
        skipWhitespace(
            bytes,
            index
            );

        if (index >= bytes.size()) {
            return PrefixParseStatus::NeedMore;
        }

        if (bytes.at(index) == ']') {
            ++index;

            return PrefixParseStatus::Complete;
        }

        /*
         * A dot-delimited record path cannot
         * traverse through an array. Once we enter
         * an unrelated array, path matching is
         * intentionally disabled for its values.
         */
        const PrefixParseStatus valueStatus =
            parseValue(
                bytes,
                index,
                targetPath,
                -1,
                arrayStart,
                errorMessage
                );

        if (valueStatus
                == PrefixParseStatus::Found
            || valueStatus
                   == PrefixParseStatus::NeedMore
            || valueStatus
                   == PrefixParseStatus::Invalid) {
            return valueStatus;
        }

        skipWhitespace(
            bytes,
            index
            );

        if (index >= bytes.size()) {
            return PrefixParseStatus::NeedMore;
        }

        if (bytes.at(index) == ',') {
            ++index;
            continue;
        }

        if (bytes.at(index) == ']') {
            ++index;

            return PrefixParseStatus::Complete;
        }

        errorMessage =
            QStringLiteral(
                "Expected ',' or ']' after a JSON "
                "array value."
                );

        return PrefixParseStatus::Invalid;
    }
}

PrefixParseStatus parsePrimitive(
    const QByteArray &bytes,
    qsizetype &index
    )
{
    while (index < bytes.size()) {
        const char character =
            bytes.at(index);

        if (character == ','
            || character == ']'
            || character == '}'
            || character == ' '
            || character == '\t'
            || character == '\r'
            || character == '\n') {
            return PrefixParseStatus::Complete;
        }

        ++index;
    }

    /*
     * Do not decide that a primitive ending exactly
     * at the current byte boundary is complete.
     * More bytes may still belong to the token.
     */
    return PrefixParseStatus::NeedMore;
}

PrefixParseStatus parseValue(
    const QByteArray &bytes,
    qsizetype &index,
    const QStringList &targetPath,
    int matchedPathDepth,
    qsizetype &arrayStart,
    QString &errorMessage
    )
{
    skipWhitespace(
        bytes,
        index
        );

    if (index >= bytes.size()) {
        return PrefixParseStatus::NeedMore;
    }

    /*
     * Reaching the complete configured path means
     * this value itself must be the live record
     * array.
     */
    if (matchedPathDepth >= 0
        && matchedPathDepth
               == targetPath.size()) {
        if (bytes.at(index) != '[') {
            errorMessage =
                QStringLiteral(
                    "The configured structured JSON "
                    "record path must resolve "
                    "to an array."
                    );

            return PrefixParseStatus::Invalid;
        }

        arrayStart = index;

        return PrefixParseStatus::Found;
    }

    switch (bytes.at(index)) {
    case '{':
        return parseObject(
            bytes,
            index,
            targetPath,
            matchedPathDepth,
            arrayStart,
            errorMessage
            );

    case '[':
        return parseArray(
            bytes,
            index,
            targetPath,
            arrayStart,
            errorMessage
            );

    case '"': {
        QString ignored;

        return parseJsonString(
            bytes,
            index,
            ignored,
            errorMessage
            );
    }

    default:
        return parsePrimitive(
            bytes,
            index
            );
    }
}

RecordArrayLocationResult locateRecordArray(
    const QByteArray &bytes,
    const QString &recordPath
    )
{
    RecordArrayLocationResult result;

    QStringList targetPath;

    const QString normalizedPath =
        recordPath.trimmed();

    if (!normalizedPath.isEmpty()) {
        targetPath =
            normalizedPath.split(
                QLatin1Char('.'),
                Qt::KeepEmptyParts
                );

        for (const QString &segment
             : targetPath) {
            if (segment.isEmpty()) {
                result.status =
                    PrefixParseStatus::Invalid;

                result.errorMessage =
                    QStringLiteral(
                        "The structured JSON "
                        "record path contains an "
                        "empty segment."
                        );

                return result;
            }
        }
    }

    qsizetype index = 0;

    result.status =
        parseValue(
            bytes,
            index,
            targetPath,
            0,
            result.arrayStart,
            result.errorMessage
            );

    if (result.status
        == PrefixParseStatus::Complete) {
        skipWhitespace(
            bytes,
            index
            );

        /*
         * A complete document was parsed without
         * locating the requested array.
         */
        if (index == bytes.size()) {
            result.status =
                PrefixParseStatus::Invalid;

            result.errorMessage =
                normalizedPath.isEmpty()
                    ? QStringLiteral(
                          "The structured JSON "
                          "document root is not a "
                          "record array."
                          )
                    : QStringLiteral(
                          "The configured structured "
                          "JSON record path '%1' "
                          "was not found."
                          )
                          .arg(
                              normalizedPath
                              );
        }
    }

    return result;
}
}

StructuredJsonRecordStreamFramer::
    StructuredJsonRecordStreamFramer(
        QString recordPath
        )
    : m_recordPath(
          std::move(recordPath)
          )
{
}

StructuredJsonRecordFrameResult
    StructuredJsonRecordStreamFramer::
    appendBytes(
        QByteArray bytes
        )
{
    StructuredJsonRecordFrameResult result;

    if (bytes.isEmpty()
        || m_recordArrayClosed) {
        return result;
    }

    if (!m_recordArrayLocated) {
        m_prefixBytes.append(
            std::move(bytes)
            );

        const RecordArrayLocationResult
            location =
            locateRecordArray(
                m_prefixBytes,
                m_recordPath
                );

        if (location.status
            == PrefixParseStatus::NeedMore) {
            return result;
        }

        if (location.status
            == PrefixParseStatus::Invalid) {
            result.succeeded = false;
            result.errorMessage =
                location.errorMessage;

            return result;
        }

        if (location.status
            != PrefixParseStatus::Found) {
            return result;
        }

        m_recordArrayLocated = true;

        const QByteArray remainingBytes =
            m_prefixBytes.mid(
                location.arrayStart + 1
                );

        m_prefixBytes.clear();

        return appendRecordArrayBytes(
            remainingBytes
            );
    }

    return appendRecordArrayBytes(
        bytes
        );
}

StructuredJsonRecordFrameResult
    StructuredJsonRecordStreamFramer::
    appendRecordArrayBytes(
        const QByteArray &bytes
        )
{
    StructuredJsonRecordFrameResult result;

    if (m_recordArrayClosed) {
        return result;
    }

    for (const char character : bytes) {
        if (!m_insideRecord) {
            if (character == ' '
                || character == '\t'
                || character == '\r'
                || character == '\n'
                || character == ',') {
                continue;
            }

            if (character == ']') {
                m_recordArrayClosed = true;
                break;
            }

            if (character != '{') {
                result.succeeded = false;

                result.errorMessage =
                    QStringLiteral(
                        "Structured JSON records "
                        "must be JSON objects."
                        );

                return result;
            }

            m_insideRecord = true;

            m_recordNestingDepth = 1;

            m_insideString = false;
            m_escapeNextCharacter = false;

            m_recordBytes.clear();

            m_recordBytes.append(
                character
                );

            continue;
        }

        m_recordBytes.append(
            character
            );

        if (m_insideString) {
            if (m_escapeNextCharacter) {
                m_escapeNextCharacter = false;
                continue;
            }

            if (character == '\\') {
                m_escapeNextCharacter = true;
                continue;
            }

            if (character == '"') {
                m_insideString = false;
            }

            continue;
        }

        if (character == '"') {
            m_insideString = true;
            continue;
        }

        if (character == '{'
            || character == '[') {
            ++m_recordNestingDepth;
            continue;
        }

        if (character == '}'
            || character == ']') {
            --m_recordNestingDepth;

            if (m_recordNestingDepth < 0) {
                result.succeeded = false;

                result.errorMessage =
                    QStringLiteral(
                        "Structured JSON record "
                        "nesting became invalid."
                        );

                return result;
            }

            if (m_recordNestingDepth == 0) {
                if (character != '}') {
                    result.succeeded = false;

                    result.errorMessage =
                        QStringLiteral(
                            "Structured JSON records "
                            "must close as JSON "
                            "objects."
                            );

                    return result;
                }

                result.records.append(
                    std::move(
                        m_recordBytes
                        )
                    );

                m_recordBytes.clear();

                m_insideRecord = false;
                m_insideString = false;
                m_escapeNextCharacter = false;
            }
        }
    }

    return result;
}

void StructuredJsonRecordStreamFramer::
    reset()
{
    m_prefixBytes.clear();
    m_recordBytes.clear();

    m_recordArrayLocated = false;
    m_recordArrayClosed = false;
    m_insideRecord = false;

    m_recordNestingDepth = 0;

    m_insideString = false;
    m_escapeNextCharacter = false;
}

bool StructuredJsonRecordStreamFramer::
    recordArrayLocated() const
{
    return m_recordArrayLocated;
}

bool StructuredJsonRecordStreamFramer::
    recordArrayClosed() const
{
    return m_recordArrayClosed;
}