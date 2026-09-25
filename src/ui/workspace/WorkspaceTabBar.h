#pragma once

#include <QColor>
#include <QHash>
#include <QPoint>
#include <QPointer>
#include <QTabBar>
#include <QVector>

class QDrag;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMouseEvent;
class QPainter;
class QPaintEvent;

class WorkspaceTabBar
    : public QTabBar
{
    Q_OBJECT

public:
    explicit WorkspaceTabBar(
        QWidget *parent = nullptr
        );

    void refreshTabAccessoryLayout(
        QWidget *accessory
        );

    void setDocumentTint(
        const QString &documentId,
        const QColor &color
        );

    void clearDocumentTint(
        const QString &documentId
        );

    void setEmptyDropTargetActive(
        bool active
        );

    int normalTabRowHeight() const;

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

    void paintDocumentTint(
        QPainter &painter,
        int index,
        const QRect &rectangle
        ) const;

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

    bool m_emptyDropTargetActive =
        false;

    QHash<QString, QColor>
        m_documentTints;
};