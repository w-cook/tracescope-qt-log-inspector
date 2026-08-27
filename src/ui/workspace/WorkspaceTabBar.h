#pragma once

#include <QPoint>
#include <QTabBar>
#include <QPointer>
#include <QVector>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;
class QPaintEvent;
class QDrag;

class WorkspaceTabBar
    : public QTabBar
{
    Q_OBJECT

public:
    explicit WorkspaceTabBar(
        QWidget *parent = nullptr
        );

signals:
    void documentDragStarted(
        const QString &documentId
        );

    void documentDropRequested(
        const QString &documentId,
        int targetIndex
        );

    void documentTearOutRequested(
        const QString &documentId,
        const QPoint &globalPosition
        );

    void externalDragCompleted();

protected:
    void mousePressEvent(
        QMouseEvent *event
        ) override;

    void mouseMoveEvent(
        QMouseEvent *event
        ) override;

    void mouseReleaseEvent(
        QMouseEvent *event
        ) override;

    void dragEnterEvent(
        QDragEnterEvent *event
        ) override;

    void dragMoveEvent(
        QDragMoveEvent *event
        ) override;

    void dragLeaveEvent(
        QDragLeaveEvent *event
        ) override;

    void dropEvent(
        QDropEvent *event
        ) override;

    void paintEvent(
        QPaintEvent *event
        ) override;

    QSize sizeHint() const override;

    QSize minimumSizeHint() const override;

private:
    int indexForDocumentId(
        const QString &documentId
        ) const;

    int insertionIndexAt(
        const QPoint &position
        ) const;

    void clearDropPreview();

    void startExternalDrag(
        QMouseEvent *event
        );

    QRect displayedTabRect(
        int index
        ) const;

    QRect dropPreviewRect() const;

    void hideTabButtonsForPreview();

    void restoreTabButtonsAfterPreview();

    void setExternalDragPreviewVisible(
        bool visible
        );

    int emptyDropTargetHeight() const;

    void finishBuiltInMoveBeforeExternalDrag(
        QMouseEvent *event,
        int sourceIndex
        );

    QString m_pressedDocumentId;

    QPoint m_pressGlobalPosition;

    bool m_externalDragActive =
        false;

    int m_dropPreviewIndex =
        -1;

    QString m_dropPreviewTitle;

    int m_dropPreviewWidth =
        120;

    QVector<QPointer<QWidget>>
        m_hiddenTabButtons;

    QPointer<WorkspaceTabBar>
        m_previewSourceTabBar;

    QPointer<QWidget>
        m_externalDragPreview;

    bool m_workspaceDragOver =
        false;
};