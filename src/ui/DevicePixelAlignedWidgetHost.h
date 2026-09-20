#pragma once

#include <QWidget>
#include <Qt>

class QEvent;
class QMoveEvent;
class QResizeEvent;

class DevicePixelAlignedWidgetHost
    : public QWidget
{
public:
    explicit DevicePixelAlignedWidgetHost(
        QWidget *contentWidget,
        Qt::Edges alignedEdges = Qt::RightEdge,
        QWidget *parent = nullptr
        );

    QWidget *contentWidget() const;

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

protected:
    bool event(
        QEvent *event
        ) override;

    void moveEvent(
        QMoveEvent *event
        ) override;

    void resizeEvent(
        QResizeEvent *event
        ) override;

private:
    static constexpr int MaxAlignmentInset =
        3;

    int alignmentInset(
        int logicalCoordinate,
        int direction
        ) const;

    int horizontalAllowance() const;

    int verticalAllowance() const;

    void updateContentGeometry();

    QWidget *m_contentWidget =
        nullptr;

    Qt::Edges m_alignedEdges;
};