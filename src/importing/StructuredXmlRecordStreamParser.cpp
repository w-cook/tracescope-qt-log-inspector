#include "StructuredXmlRecordStreamParser.h"

#include <utility>

#include <QJsonArray>
#include <QXmlStreamAttributes>
#include <QXmlStreamWriter>

namespace
{
QStringList splitRecordPath(
    const QString &recordPath
    )
{
    return recordPath
        .trimmed()
        .split(
            QLatin1Char('.'),
            Qt::SkipEmptyParts
            );
}

bool appendNamedDataValue(
    QJsonObject &object,
    const QString &elementName,
    const QJsonValue &value
    )
{
    /*
     * Preserve XmlImporter's existing treatment of
     * collections such as:
     *
     * <Data Name="ProcessName">worker.exe</Data>
     *
     * This is required by Windows Event XML and is
     * also useful for other structured XML sources.
     */
    if (elementName
            != QStringLiteral("Data")
        || !value.isObject()) {
        return false;
    }

    const QJsonObject dataObject =
        value.toObject();

    const QJsonValue nameValue =
        dataObject.value(
            QStringLiteral("@Name")
            );

    if (!nameValue.isString()) {
        return false;
    }

    const QString fieldName =
        nameValue
            .toString()
            .trimmed();

    if (fieldName.isEmpty()) {
        return false;
    }

    const QJsonValue fieldValue =
        dataObject.contains(
            QStringLiteral("#text")
            )
            ? dataObject.value(
                  QStringLiteral("#text")
                  )
            : QJsonValue(
                  QString()
                  );

    QJsonObject namedValues =
        object.value(
                  QStringLiteral(
                      "NamedData"
                      )
                  )
            .toObject();

    if (!namedValues.contains(
            fieldName
            )) {
        namedValues.insert(
            fieldName,
            fieldValue
            );
    } else {
        QJsonArray repeatedValues;

        const QJsonValue existing =
            namedValues.value(
                fieldName
                );

        if (existing.isArray()) {
            repeatedValues =
                existing.toArray();
        } else {
            repeatedValues.append(
                existing
                );
        }

        repeatedValues.append(
            fieldValue
            );

        namedValues.insert(
            fieldName,
            repeatedValues
            );
    }

    object.insert(
        QStringLiteral("NamedData"),
        namedValues
        );

    return true;
}

void appendChildValue(
    QJsonObject &object,
    const QString &name,
    const QJsonValue &value
    )
{
    if (appendNamedDataValue(
            object,
            name,
            value
            )) {
        return;
    }

    if (!object.contains(
            name
            )) {
        object.insert(
            name,
            value
            );

        return;
    }

    QJsonArray values;

    const QJsonValue existing =
        object.value(
            name
            );

    if (existing.isArray()) {
        values =
            existing.toArray();
    } else {
        values.append(
            existing
            );
    }

    values.append(
        value
        );

    object.insert(
        name,
        values
        );
}

QJsonObject recordObject(
    const QJsonValue &value
    )
{
    if (value.isObject()) {
        return value.toObject();
    }

    QJsonObject object;

    object.insert(
        QStringLiteral("#text"),
        value
        );

    return object;
}
}

StructuredXmlRecordStreamParser::
    StructuredXmlRecordStreamParser(
        QString recordPath
        )
    : m_recordPath(
          splitRecordPath(
              recordPath
              )
          ),
    m_useDocumentRoot(
        m_recordPath.isEmpty()
        )
{
}

StructuredXmlRecordStreamParser::
    ~StructuredXmlRecordStreamParser() =
    default;

StructuredXmlRecordStreamParseResult
    StructuredXmlRecordStreamParser::
    appendBytes(
        QByteArray bytes
        )
{
    StructuredXmlRecordStreamParseResult
        result;

    if (m_failed) {
        result.succeeded = false;
        result.errorMessage =
            m_errorMessage;

        return result;
    }

    /*
     * Once a complete XML document has ended,
     * additional non-whitespace content cannot
     * belong to that same physical generation.
     */
    if (m_documentClosed) {
        if (!bytes.trimmed().isEmpty()) {
            fail(
                QStringLiteral(
                    "Additional content was appended "
                    "after the XML document had "
                    "already closed."
                    )
                );

            result.succeeded = false;
            result.errorMessage =
                m_errorMessage;
        }

        return result;
    }

    if (bytes.isEmpty()) {
        return result;
    }

    /*
     * QXmlStreamReader supports incremental input.
     *
     * In particular, addData() allows parsing to
     * resume after PrematureEndOfDocumentError when
     * a later physical chunk completes the current
     * XML structure.
     */
    m_reader.addData(
        bytes
        );

    while (!m_reader.atEnd()) {
        m_reader.readNext();

        /*
         * Do not send an invalid/error token to the
         * raw-source writer.
         */
        if (m_reader.hasError()) {
            break;
        }

        if (m_reader.isEndDocument()) {
            m_documentClosed = true;
            break;
        }

        /*
         * Once a selected record has begun, its own
         * explicit element stack owns nested parsing.
         * The outer document path remains parked on
         * the selected record until it completes.
         */
        if (m_currentWriter) {
            processRecordToken(
                result
                );

            continue;
        }

        if (m_reader.isStartElement()) {
            m_currentPath.append(
                m_reader
                    .name()
                    .toString()
                );

            updateRecordContainerLocation();

            if (!currentPathIsRecord()) {
                continue;
            }

            m_recordPathLocated = true;

            beginRecord();

            /*
             * The current reader token is the
             * selected record's StartElement, so it
             * belongs to the newly created record
             * parser state.
             */
            processRecordToken(
                result
                );

            continue;
        }

        if (m_reader.isEndElement()
            && !m_currentPath.isEmpty()) {
            m_currentPath.removeLast();
        }
    }

    if (!m_reader.hasError()) {
        return result;
    }

    /*
     * EOF while the outer container or a selected
     * record is still open is expected for an
     * actively written structured source.
     *
     * QXmlStreamReader retains its parsing state and
     * can resume after addData() supplies the next
     * chunk.
     */
    if (m_reader.error()
        == QXmlStreamReader::
        PrematureEndOfDocumentError) {
        return result;
    }

    /*
     * All other QXmlStreamReader errors represent an
     * actually malformed XML stream rather than an
     * ordinary incremental boundary.
     */
    failFromReader();

    result.succeeded = false;
    result.errorMessage =
        m_errorMessage;

    return result;
}

void StructuredXmlRecordStreamParser::
    reset()
{
    /*
     * Destroy the writer before clearing the backing
     * QString and parser state.
     */
    m_currentWriter.reset();

    m_currentRawSource.clear();

    m_elementStack.clear();
    m_currentPath.clear();

    m_reader.clear();

    m_recordPathLocated = false;
    m_recordContainerLocated = false;
    m_documentClosed = false;

    m_failed = false;
    m_errorMessage.clear();
}

bool StructuredXmlRecordStreamParser::
    recordPathLocated() const
{
    return m_recordPathLocated;
}

bool StructuredXmlRecordStreamParser::
    recordContainerLocated() const
{
    return m_recordContainerLocated;
}

bool StructuredXmlRecordStreamParser::
    documentClosed() const
{
    return m_documentClosed;
}

bool StructuredXmlRecordStreamParser::
    hasIncompleteRecord() const
{
    return m_currentWriter
           != nullptr;
}

bool StructuredXmlRecordStreamParser::
    currentPathIsRecord() const
{
    if (m_useDocumentRoot) {
        return m_currentPath.size()
        == 1;
    }

    return m_currentPath
           == m_recordPath;
}

void StructuredXmlRecordStreamParser::
    updateRecordContainerLocation()
{
    /*
     * An open-stream record container only exists
     * when the configured record path has an outer
     * parent.
     *
     * Examples:
     *
     * session.events.event
     *     container = session.events
     *
     * Events.Event
     *     container = Events
     *
     * A document-root record does not have a
     * repeatable outer record container.
     */
    if (m_recordContainerLocated
        || m_useDocumentRoot
        || m_recordPath.size() < 2) {
        return;
    }

    const qsizetype containerDepth =
        m_recordPath.size() - 1;

    if (m_currentPath.size()
        != containerDepth) {
        return;
    }

    for (qsizetype index = 0;
         index < containerDepth;
         ++index) {
        if (m_currentPath.at(index)
            != m_recordPath.at(index)) {
            return;
        }
    }

    m_recordContainerLocated = true;
}

void StructuredXmlRecordStreamParser::
    beginRecord()
{
    m_currentRawSource.clear();
    m_elementStack.clear();

    m_currentWriter =
        std::make_unique<
            QXmlStreamWriter
            >(
            &m_currentRawSource
            );
}

void StructuredXmlRecordStreamParser::
    processRecordToken(
        StructuredXmlRecordStreamParseResult
            &result
        )
{
    if (!m_currentWriter) {
        return;
    }

    if (m_reader.isStartElement()) {
        m_currentWriter
            ->writeCurrentToken(
                m_reader
                );

        ElementFrame frame;

        frame.name =
            m_reader
                .name()
                .toString();

        const QXmlStreamAttributes attributes =
            m_reader.attributes();

        for (const QXmlStreamAttribute &attribute
             : attributes) {
            frame.object.insert(
                QStringLiteral("@%1")
                    .arg(
                        attribute
                            .name()
                            .toString()
                        ),
                attribute
                    .value()
                    .toString()
                );
        }

        if (!m_elementStack.isEmpty()) {
            m_elementStack
                .last()
                .hasChildElements =
                true;
        }

        m_elementStack.append(
            std::move(
                frame
                )
            );

        return;
    }

    if (m_reader.isCharacters()) {
        m_currentWriter
            ->writeCurrentToken(
                m_reader
                );

        if (!m_elementStack.isEmpty()) {
            m_elementStack
                .last()
                .text
                .append(
                    m_reader
                        .text()
                        .toString()
                    );
        }

        return;
    }

    if (m_reader.isEndElement()) {
        m_currentWriter
            ->writeCurrentToken(
                m_reader
                );

        if (m_elementStack.isEmpty()) {
            return;
        }

        ElementFrame completedFrame =
            m_elementStack.takeLast();

        const QString completedName =
            completedFrame.name;

        const QJsonValue completedValue =
            finalizeElementFrame(
                std::move(
                    completedFrame
                    )
                );

        if (!m_elementStack.isEmpty()) {
            appendChildValue(
                m_elementStack
                    .last()
                    .object,
                completedName,
                completedValue
                );

            return;
        }

        /*
         * Empty element stack after finalization means
         * the selected record element itself has just
         * completed.
         */
        StructuredXmlRecord record;

        record.object =
            recordObject(
                completedValue
                );

        record.rawSource =
            m_currentRawSource;

        result.records.append(
            std::move(
                record
                )
            );

        m_currentWriter.reset();
        m_currentRawSource.clear();

        /*
         * The selected record's EndElement was
         * consumed by the record parser rather than
         * the outer path parser.
         */
        if (!m_currentPath.isEmpty()) {
            m_currentPath.removeLast();
        }

        return;
    }

    /*
     * Preserve comments, entity references,
     * processing instructions, CDATA-related tokens,
     * and other valid XML tokens in reconstructed
     * rawSource just as XmlImporter's existing
     * QXmlStreamWriter path does.
     */
    m_currentWriter
        ->writeCurrentToken(
            m_reader
            );
}

QJsonValue StructuredXmlRecordStreamParser::
    finalizeElementFrame(
        ElementFrame frame
        ) const
{
    const QString normalizedText =
        frame.text.trimmed();

    /*
     * Match XmlImporter's existing scalar behavior:
     *
     * <message>Hello</message>
     *
     * becomes the JSON string "Hello" rather than
     * an object containing #text.
     */
    if (!frame.hasChildElements
        && frame.object.isEmpty()) {
        return normalizedText;
    }

    if (!normalizedText.isEmpty()) {
        frame.object.insert(
            QStringLiteral("#text"),
            normalizedText
            );
    }

    return frame.object;
}

void StructuredXmlRecordStreamParser::
    failFromReader()
{
    fail(
        QStringLiteral(
            "The XML stream could not be parsed "
            "at line %1, column %2: %3"
            )
            .arg(
                m_reader.lineNumber()
                )
            .arg(
                m_reader.columnNumber()
                )
            .arg(
                m_reader.errorString()
                )
        );
}

void StructuredXmlRecordStreamParser::
    fail(
        QString errorMessage
        )
{
    m_failed = true;
    m_errorMessage =
        std::move(
            errorMessage
            );
}