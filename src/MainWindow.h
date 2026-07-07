#pragma once

#include <QMainWindow>

class QLabel;
class QTableWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QLabel *summaryLabel;
    QTableWidget *eventTable;

    void buildLayout();
};