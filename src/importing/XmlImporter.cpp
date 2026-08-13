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

void appendChildValue(
    QJsonObject &object,
    const QString &name,
    const QJsonValue &value
    )
{
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

QJsonValue readElementValue(
    QXmlStreamReader &reader,
    QXmlStreamWriter &writer
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
        reader.readNext();

        if (reader.isStartElement()) {
            hasChildElements = true;

            const QString childName =
                reader.name().toString();

            const QJsonValue childValue =
                readElementValue(
                    reader,
                    writer
                    );

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
        maxProcessedRecords
        );
}

ImportResult XmlImporter::importFile(
    const QString &filePath,
    qint64 maxProcessedRecords
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
        maxProcessedRecords
        );
}

ImportResult XmlImporter::importDevice(
    QIODevice &device,
    const QString &sourcePath,
    qint64 maxProcessedRecords
    ) const
{
    ImportResult result;

    QXmlStreamReader reader(&device);

    const QStringList recordPath =
        splitRecordPath(
            profile.recordPath
            );

    const bool useDocumentRoot =
        recordPath.isEmpty();

    QStringList currentPath;

    while (!reader.atEnd()) {
        reader.readNext();

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
                    writer
                    );

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
        && !result.sourceTruncated) {
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

    if (result.processedRecordCount == 0) {
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

    return result;
}