#pragma once

#include <memory>

#include <QByteArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QXmlStreamReader>

class QXmlStreamWriter;

struct StructuredXmlRecord
{
    QJsonObject object;
    QString rawSource;
};

struct StructuredXmlRecordStreamParseResult
{
    bool succeeded = true;
    QString errorMessage;

    QVector<StructuredXmlRecord> records;
};

class StructuredXmlRecordStreamParser
{
public:
    explicit StructuredXmlRecordStreamParser(
        QString recordPath = {}
        );

    ~StructuredXmlRecordStreamParser();

    StructuredXmlRecordStreamParseResult
    appendBytes(
        QByteArray bytes
        );

    void reset();

    bool recordPathLocated() const;
    bool recordContainerLocated() const;
    bool documentClosed() const;
    bool hasIncompleteRecord() const;

private:
    struct ElementFrame
    {
        QString name;
        QJsonObject object;
        QString text;
        bool hasChildElements = false;
    };

    bool currentPathIsRecord() const;

    void updateRecordContainerLocation();

    void beginRecord();

    void processRecordToken(
        StructuredXmlRecordStreamParseResult &result
        );

    QJsonValue finalizeElementFrame(
        ElementFrame frame
        ) const;

    void failFromReader();

    void fail(
        QString errorMessage
        );

    QStringList m_recordPath;
    bool m_useDocumentRoot = false;

    QXmlStreamReader m_reader;

    QStringList m_currentPath;

    QVector<ElementFrame> m_elementStack;

    QString m_currentRawSource;

    std::unique_ptr<QXmlStreamWriter>
        m_currentWriter;

    bool m_recordPathLocated = false;
    bool m_recordContainerLocated = false;
    bool m_documentClosed = false;

    bool m_failed = false;
    QString m_errorMessage;
};