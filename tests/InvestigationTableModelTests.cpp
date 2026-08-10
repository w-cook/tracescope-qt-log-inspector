#include <QtTest>

#include "../src/models/InvestigationTableModel.h"

class InvestigationTableModelTests : public QObject
{
    Q_OBJECT

private slots:
    void emptyModelHasCanonicalColumns();
    void displaysCanonicalRecordValues();
    void displaysMissingCanonicalValuesAsEmpty();
    void addsDynamicCustomColumns();
    void customColumnsAreDeterministicallySorted();
    void recordAtReturnsSourceRecord();
    void exposesTypedSortValues();
};

void InvestigationTableModelTests::emptyModelHasCanonicalColumns()
{
    InvestigationTableModel model;

    QCOMPARE(model.rowCount(), 0);
    QCOMPARE(model.columnCount(), 6);

    const QStringList expectedHeaders {
        QStringLiteral("Timestamp"),
        QStringLiteral("Severity"),
        QStringLiteral("Subsystem"),
        QStringLiteral("Event Code"),
        QStringLiteral("Entity ID"),
        QStringLiteral("Message")
    };

    for (int column = 0;
         column < expectedHeaders.size();
         ++column) {
        QCOMPARE(
            model.headerData(
                     column,
                     Qt::Horizontal,
                     Qt::DisplayRole
                     ).toString(),
            expectedHeaders[column]
            );
    }
}

void InvestigationTableModelTests::displaysCanonicalRecordValues()
{
    InvestigationRecord record;

    record.recordId = QStringLiteral("record-1");

    record.timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-08T12:30:45.123Z"),
        Qt::ISODateWithMs
        );

    record.severity = RecordSeverity::Warning;
    record.subsystem = QStringLiteral("Tracking");
    record.eventCode = QStringLiteral("TRACK_LOST");
    record.entityId = QStringLiteral("TRK-402");
    record.message = QStringLiteral("Track lost");

    InvestigationTableModel model;
    model.setRecords({record});

    QCOMPARE(model.rowCount(), 1);

    QCOMPARE(
        model.data(model.index(0, 0)).toString(),
        QStringLiteral("2026-08-08T12:30:45.123Z")
        );

    QCOMPARE(
        model.data(model.index(0, 1)).toString(),
        QStringLiteral("WARN")
        );

    QCOMPARE(
        model.data(model.index(0, 2)).toString(),
        QStringLiteral("Tracking")
        );

    QCOMPARE(
        model.data(model.index(0, 3)).toString(),
        QStringLiteral("TRACK_LOST")
        );

    QCOMPARE(
        model.data(model.index(0, 4)).toString(),
        QStringLiteral("TRK-402")
        );

    QCOMPARE(
        model.data(model.index(0, 5)).toString(),
        QStringLiteral("Track lost")
        );
}

void InvestigationTableModelTests::
    displaysMissingCanonicalValuesAsEmpty()
{
    InvestigationRecord record;
    record.recordId = QStringLiteral("record-1");

    InvestigationTableModel model;
    model.setRecords({record});

    for (int column = 0; column < 6; ++column) {
        QVERIFY(
            !model.data(
                      model.index(0, column),
                      Qt::DisplayRole
                      ).isValid()
            );
    }
}

void InvestigationTableModelTests::addsDynamicCustomColumns()
{
    InvestigationRecord first;
    first.recordId = QStringLiteral("record-1");
    first.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("server-01")
        );

    InvestigationRecord second;
    second.recordId = QStringLiteral("record-2");
    second.customAttributes.insert(
        QStringLiteral("threadId"),
        42
        );

    InvestigationTableModel model;
    model.setRecords({first, second});

    QCOMPARE(model.columnCount(), 8);

    QCOMPARE(
        model.headerData(
                 6,
                 Qt::Horizontal
                 ).toString(),
        QStringLiteral("host")
        );

    QCOMPARE(
        model.headerData(
                 7,
                 Qt::Horizontal
                 ).toString(),
        QStringLiteral("threadId")
        );

    QCOMPARE(
        model.data(
                 model.index(0, 6)
                 ).toString(),
        QStringLiteral("server-01")
        );

    QCOMPARE(
        model.data(
                 model.index(1, 7)
                 ).toInt(),
        42
        );
}

void InvestigationTableModelTests::
    customColumnsAreDeterministicallySorted()
{
    InvestigationRecord record;
    record.recordId = QStringLiteral("record-1");

    record.customAttributes.insert(
        QStringLiteral("zone"),
        QStringLiteral("west")
        );

    record.customAttributes.insert(
        QStringLiteral("host"),
        QStringLiteral("server-01")
        );

    record.customAttributes.insert(
        QStringLiteral("processId"),
        1234
        );

    InvestigationTableModel model;
    model.setRecords({record});

    QCOMPARE(
        model.headerData(6, Qt::Horizontal).toString(),
        QStringLiteral("host")
        );

    QCOMPARE(
        model.headerData(7, Qt::Horizontal).toString(),
        QStringLiteral("processId")
        );

    QCOMPARE(
        model.headerData(8, Qt::Horizontal).toString(),
        QStringLiteral("zone")
        );
}

void InvestigationTableModelTests::recordAtReturnsSourceRecord()
{
    InvestigationRecord record;
    record.recordId = QStringLiteral("record-123");

    InvestigationTableModel model;
    model.setRecords({record});

    const InvestigationRecord *storedRecord =
        model.recordAt(0);

    QVERIFY(storedRecord != nullptr);
    QCOMPARE(
        storedRecord->recordId,
        QStringLiteral("record-123")
        );

    QVERIFY(model.recordAt(-1) == nullptr);
    QVERIFY(model.recordAt(1) == nullptr);
}

void InvestigationTableModelTests::exposesTypedSortValues()
{
    InvestigationRecord record;

    record.timestamp = QDateTime::fromString(
        QStringLiteral("2026-08-08T12:30:45.123Z"),
        Qt::ISODateWithMs
        );

    record.severity = RecordSeverity::Error;

    InvestigationTableModel model;
    model.setRecords({record});

    const QVariant timestampSortValue =
        model.data(
            model.index(0, 0),
            InvestigationTableModel::SortRole
            );

    QVERIFY(timestampSortValue.canConvert<QDateTime>());

    QCOMPARE(
        timestampSortValue.toDateTime(),
        record.timestamp.value()
        );

    QCOMPARE(
        model.data(
                 model.index(0, 1),
                 InvestigationTableModel::SortRole
                 ).toInt(),
        static_cast<int>(RecordSeverity::Error)
        );
}

QTEST_MAIN(InvestigationTableModelTests)

#include "InvestigationTableModelTests.moc"