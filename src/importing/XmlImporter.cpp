#include "XmlImporter.h"

#include <optional>
#include <utility>

#include <QBuffer>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "ImportDiagnostic.h"
#include "JsonObjectRecordMapper.h"

namespace
{
RecordSourceMetadata createSourceMetadata(
    const QString &sourcePath,
    qint64 recordNumber
    )
{
    RecordSourceMetadata source;

    source.sourcePath = sourcePath;
    source.recordNumber = recordNumber;

    if (!sourcePath.isEmpty()) {
        source.sourceName =
            QFileInfo(sourcePath).fileName();
    }

    return source;
}

void appendDiagnostic(
    ImportResult &result,
    const QString &code,
    const QString &message,
    ImportDiagnosticSeverity severity,
    const std::optional<RecordSourceMetadata>
        &source = std::nullopt
    )
{
    ImportDiagnostic diagnostic;

    diagnostic.code = code;
    diagnostic.message = message;
    diagnostic.severity = severity;
    diagnostic.source = source;

    result.diagnostics.append(
        diagnostic
        );
}

bool appendNamedDataValue(
    QJsonObject &object,
    const QString &elementName,
    const QJsonValue &value
    )
{
    /*
     * Some structured XML formats represent a
     * dynamic field collection with elements such
     * as:
     *
     * <Data Name="ProcessName">worker.exe</Data>
     *
     * Keep those fields individually addressable
     * instead of collapsing them into a positional
     * array.
     *
     * The original XML remains preserved separately
     * as rawSource.
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

    if (!object.contains(name)) {
        object.insert(
            name,
            value
            );

        return;
    }

    QJsonArray values;

    const QJsonValue existing =
        object.value(name);

    if (existing.isArray()) {
        values = existing.toArray();
    } else {
        values.append(existing);
    }

    values.append(value);

    object.insert(
        name,
        values
        );
}

constexpr qint64 progressReportByteInterval =
    256 * 1024;

void reportProgressIfNeeded(
    QIODevice &device,
    qint64 totalBytes,
    qint64 processedRecordCount,
    qint64 &lastReportedBytes,
    const ImportExecutionContext &executionContext
    )
{
    const qint64 bytesProcessed =
        device.pos();

    if (bytesProcessed
            - lastReportedBytes
        < progressReportByteInterval) {
        return;
    }

    executionContext.report({
        bytesProcessed,
        totalBytes,
        processedRecordCount
    });

    lastReportedBytes =
        bytesProcessed;
}

QJsonValue readElementValue(
    QXmlStreamReader &reader,
    QXmlStreamWriter &writer,
    QIODevice &device,
    qint64 totalBytes,
    qint64 processedRecordCount,
    qint64 &lastReportedBytes,
    const ImportExecutionContext &executionContext,
    bool &cancelled
    )
{
    /*
     * The reader must be positioned on the
     * element's StartElement token.
     */
    writer.writeCurrentToken(reader);

    QJsonObject object;

    const QXmlStreamAttributes attributes =
        reader.attributes();

    for (const QXmlStreamAttribute &attribute
         : attributes) {
        object.insert(
            QStringLiteral("@%1")
                .arg(
                    attribute.name()
                        .toString()
                    ),
            attribute.value()
                .toString()
            );
    }

    QString text;
    bool hasChildElements = false;

    while (!reader.atEnd()) {
        if (executionContext
                .cancellationRequested()) {
            cancelled = true;
            return {};
        }

        reader.readNext();

        reportProgressIfNeeded(
            device,
            totalBytes,
            processedRecordCount,
            lastReportedBytes,
            executionContext
            );

        /*
     * A parse error leaves QXmlStreamReader on an
     * invalid token. Do not pass that token to
     * QXmlStreamWriter::writeCurrentToken(), which
     * would itself emit a warning while we are already
     * handling the malformed source as an import
     * diagnostic.
     */
        if (reader.hasError()) {
            break;
        }

        if (reader.isStartElement()) {
            hasChildElements = true;

            const QString childName =
                reader.name().toString();

            const QJsonValue childValue =
                readElementValue(
                    reader,
                    writer,
                    device,
                    totalBytes,
                    processedRecordCount,
                    lastReportedBytes,
                    executionContext,
                    cancelled
                    );

            if (cancelled) {
                return {};
            }

            appendChildValue(
                object,
                childName,
                childValue
                );

            continue;
        }

        if (reader.isCharacters()) {
            writer.writeCurrentToken(reader);

            text.append(
                reader.text().toString()
                );

            continue;
        }

        if (reader.isEndElement()) {
            writer.writeCurrentToken(reader);
            break;
        }

        /*
         * Preserve comments, entity references,
         * processing instructions, and other
         * valid tokens in the reconstructed
         * raw-source XML.
         */
        writer.writeCurrentToken(reader);
    }

    const QString normalizedText =
        text.trimmed();

    if (!hasChildElements
        && object.isEmpty()) {
        return normalizedText;
    }

    if (!normalizedText.isEmpty()) {
        object.insert(
            QStringLiteral("#text"),
            normalizedText
            );
    }

    return object;
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
}

XmlImporter::XmlImporter(
    ImportProfile profile
    )
    : profile(std::move(profile))
{
    this->profile.importerId =
        QStringLiteral("xml");
}

QString XmlImporter::id() const
{
    return QStringLiteral("xml");
}

QString XmlImporter::displayName() const
{
    return QStringLiteral(
        "Structured XML"
        );
}

ImportResult XmlImporter::importContent(
    const QByteArray &xml,
    const QString &sourcePath,
    qint64 maxProcessedRecords
    ) const
{
    QBuffer buffer;

    buffer.setData(xml);
    buffer.open(QIODevice::ReadOnly);

    return importDevice(
        buffer,
        sourcePath,
        maxProcessedRecords,
        {}
        );
}

ImportResult XmlImporter::importFile(
    const QString &filePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    QFile file(filePath);

    if (!file.open(
            QIODevice::ReadOnly
            )) {
        ImportResult result;

        appendDiagnostic(
            result,
            QStringLiteral(
                "FILE_OPEN_FAILED"
                ),
            QStringLiteral(
                "The source file could not be opened: %1"
                )
                .arg(
                    file.errorString()
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                filePath,
                0
                )
            );

        return result;
    }

    return importDevice(
        file,
        filePath,
        maxProcessedRecords,
        executionContext
        );
}

ImportResult XmlImporter::importDevice(
    QIODevice &device,
    const QString &sourcePath,
    qint64 maxProcessedRecords,
    const ImportExecutionContext &executionContext
    ) const
{
    ImportResult result;

    const qint64 totalBytes =
        device.size();

    qint64 lastReportedBytes = 0;

    executionContext.report({
        0,
        totalBytes,
        0
    });

    QXmlStreamReader reader(&device);

    const QStringList recordPath =
        splitRecordPath(
            profile.recordPath
            );

    const bool useDocumentRoot =
        recordPath.isEmpty();

    QStringList currentPath;

    while (!reader.atEnd()) {
        if (executionContext
                .cancellationRequested()) {
            result.cancelled = true;
            break;
        }

        reader.readNext();

        reportProgressIfNeeded(
            device,
            totalBytes,
            result.processedRecordCount,
            lastReportedBytes,
            executionContext
            );

        if (reader.isStartElement()) {
            currentPath.append(
                reader.name().toString()
                );

            const bool isRecord =
                useDocumentRoot
                    ? currentPath.size() == 1
                    : currentPath == recordPath;

            if (!isRecord) {
                continue;
            }

            if (maxProcessedRecords > 0
                && result.processedRecordCount
                       >= maxProcessedRecords) {
                result.sourceTruncated = true;
                break;
            }

            ++result.processedRecordCount;

            QString rawSource;

            QXmlStreamWriter writer(
                &rawSource
                );

            const QJsonValue parsedValue =
                readElementValue(
                    reader,
                    writer,
                    device,
                    totalBytes,
                    result.processedRecordCount,
                    lastReportedBytes,
                    executionContext,
                    result.cancelled
                    );

            if (result.cancelled) {
                break;
            }

            /*
             * A record whose element could not be parsed to
             * completion is not a valid imported record.
             *
             * Keep any records that were fully parsed before
             * the malformed portion of the document, but do
             * not manufacture a partial record from the
             * element in which parsing failed.
             */
            if (reader.hasError()) {
                break;
            }

            const RecordSourceMetadata source =
                createSourceMetadata(
                    sourcePath,
                    result.processedRecordCount
                    );

            result.records.append(
                JsonObjectRecordMapper::mapRecord(
                    recordObject(parsedValue),
                    rawSource,
                    source,
                    profile,
                    result
                    )
                );

            /*
             * readElementValue() consumed the
             * matched element's EndElement token,
             * so remove that path component here.
             */
            currentPath.removeLast();

            continue;
        }

        if (reader.isEndElement()
            && !currentPath.isEmpty()) {
            currentPath.removeLast();
        }
    }

    if (reader.hasError()
        && !result.sourceTruncated
        && !result.cancelled) {
        appendDiagnostic(
            result,
            QStringLiteral(
                "XML_PARSE_ERROR"
                ),
            QStringLiteral(
                "The XML document could not be parsed "
                "at line %1, column %2: %3"
                )
                .arg(
                    reader.lineNumber()
                    )
                .arg(
                    reader.columnNumber()
                    )
                .arg(
                    reader.errorString()
                    ),
            ImportDiagnosticSeverity::Error,
            createSourceMetadata(
                sourcePath,
                0
                )
            );

        return result;
    }

    if (!result.cancelled
        && result.processedRecordCount == 0) {
        if (useDocumentRoot) {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "XML_DOCUMENT_EMPTY"
                    ),
                QStringLiteral(
                    "The XML document does not contain "
                    "a root record element."
                    ),
                ImportDiagnosticSeverity::Error,
                createSourceMetadata(
                    sourcePath,
                    0
                    )
                );
        } else {
            appendDiagnostic(
                result,
                QStringLiteral(
                    "XML_RECORD_PATH_NOT_FOUND"
                    ),
                QStringLiteral(
                    "No XML records were found at "
                    "record path '%1'."
                    )
                    .arg(
                        profile.recordPath
                        ),
                ImportDiagnosticSeverity::Error,
                createSourceMetadata(
                    sourcePath,
                    0
                    )
                );
        }
    }

    executionContext.report({
        device.pos(),
        totalBytes,
        result.processedRecordCount
    });

    return result;
}