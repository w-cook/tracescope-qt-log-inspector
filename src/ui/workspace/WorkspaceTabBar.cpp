#include "WorkspaceTabBar.h"

#include <QApplication>
#include <QCursor>
#include <QDrag>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QStyle>
#include <QStyleOptionTab>
#include <QPointer>
#include <QLabel>
#include <QSizePolicy>
#include <QTimer>

#include <utility>

namespace
{
constexpr auto WorkspaceDocumentMimeType =
    "application/x-tracescope-workspace-document";

constexpr auto WorkspaceDocumentWidthMimeType =
    "application/x-tracescope-workspace-document-width";
}

WorkspaceTabBar::WorkspaceTabBar(
    QWidget *parent
    )
    : QTabBar(parent)
{
    setMovable(
        true
        );

    setAcceptDrops(
        true
        );

    setAutoHide(
        false
        );

    setSizePolicy(
        QSizePolicy::Expanding,
        QSizePolicy::Fixed
        );

    setMinimumHeight(
        emptyDropTargetHeight()
        );
}

void WorkspaceTabBar::mousePressEvent(
    QMouseEvent *event
    )
{
    m_pressedDocumentId.clear();

    if (event != nullptr
        && event->button()
               == Qt::LeftButton) {
        const int index =
            tabAt(
                event->position()
                    .toPoint()
                );

        if (index >= 0) {
            m_pressedDocumentId =
                tabData(index)
                    .toString();

            m_pressGlobalPosition =
                event->globalPosition()
                    .toPoint();
        }
    }

    QTabBar::mousePressEvent(
        event
        );
}

void WorkspaceTabBar::mouseMoveEvent(
    QMouseEvent *event
    )
{
    if (event == nullptr
        || m_externalDragActive
        || m_pressedDocumentId.isEmpty()
        || !(event->buttons()
             & Qt::LeftButton)) {
        QTabBar::mouseMoveEvent(
            event
            );

        return;
    }

    const QPoint globalPosition =
        event->globalPosition()
            .toPoint();

    const int distance =
        (globalPosition
         - m_pressGlobalPosition)
            .manhattanLength();

    if (distance
        < QApplication::startDragDistance()) {
        QTabBar::mouseMoveEvent(
            event
            );

        return;
    }

    /*
     * Keep normal QTabBar reordering in control
     * while the pointer remains in or immediately
     * around this tab strip.
     */
    QRect safeRectangle =
        rect();

    const int margin =
        QApplication::startDragDistance();

    safeRectangle.adjust(
        -margin,
        -margin,
        margin,
        margin
        );

    const QPoint localPosition =
        mapFromGlobal(
            globalPosition
            );

    if (safeRectangle.contains(
            localPosition
            )) {
        QTabBar::mouseMoveEvent(
            event
            );

        return;
    }

    startExternalDrag(
        event
        );
}

void WorkspaceTabBar::mouseReleaseEvent(
    QMouseEvent *event
    )
{
    QTabBar::mouseReleaseEvent(
        event
        );

    if (!m_externalDragActive) {
        m_pressedDocumentId.clear();
    }
}

void WorkspaceTabBar::startExternalDrag(
    QMouseEvent *event
    )
{
    if (event == nullptr
        || m_pressedDocumentId.isEmpty()) {
        return;
    }

    const QString documentId =
        m_pressedDocumentId;

    const int index =
        indexForDocumentId(
            documentId
            );

    if (index < 0) {
        m_pressedDocumentId.clear();
        return;
    }

    const QRect sourceRectangle =
        tabRect(index);

    const QString title =
        tabText(index);

    /*
     * Render a stable representation of the tab
     * before the document is extracted from its
     * source tab group.
     */
    QPixmap dragPixmap(
        sourceRectangle.size()
        );

    dragPixmap.fill(
        Qt::transparent
        );

    QStyleOptionTab dragOption;

    initStyleOption(
        &dragOption,
        index
        );

    dragOption.rect =
        QRect(
            QPoint(0, 0),
            sourceRectangle.size()
            );

    dragOption.text =
        title;

    QPainter dragPainter(
        &dragPixmap
        );

    style()->drawControl(
        QStyle::CE_TabBarTab,
        &dragOption,
        &dragPainter,
        this
        );

    dragPainter.end();

    /*
     * Preserve the pointer's location within the
     * tab so the floating representation tracks the
     * mouse naturally while crossing empty space.
     */
    QPoint hotSpot =
        mapFromGlobal(
            event->globalPosition()
                .toPoint()
            )
        - sourceRectangle.topLeft();

    hotSpot.setX(
        qBound(
            0,
            hotSpot.x(),
            qMax(
                0,
                sourceRectangle.width() - 1
                )
            )
        );

    hotSpot.setY(
        qBound(
            0,
            hotSpot.y(),
            qMax(
                0,
                sourceRectangle.height() - 1
                )
            )
        );

    /*
     * QDrag still owns the actual platform drag/drop
     * operation, but its native pixmap is kept
     * transparent. A separate lightweight window
     * provides the visible floating tab instead.
     *
     * That allows destination tab bars to hide the
     * floating image while showing their insertion
     * preview.
     */
    QDrag drag(this);

    auto *mimeData =
        new QMimeData;

    mimeData->setData(
        WorkspaceDocumentMimeType,
        documentId.toUtf8()
        );

    mimeData->setText(
        title
        );

    mimeData->setData(
        WorkspaceDocumentWidthMimeType,
        QByteArray::number(
            sourceRectangle.width()
            )
        );

    drag.setMimeData(
        mimeData
        );

    QPixmap transparentDragPixmap(
        1,
        1
        );

    transparentDragPixmap.fill(
        Qt::transparent
        );

    drag.setPixmap(
        transparentDragPixmap
        );

    drag.setHotSpot(
        QPoint(0, 0)
        );

    /*
     * Visible floating tab used while the pointer is
     * not over a valid workspace insertion target.
     */
    QLabel dragPreview(
        nullptr,
        Qt::ToolTip
            | Qt::FramelessWindowHint
            | Qt::WindowTransparentForInput
        );

    dragPreview.setAttribute(
        Qt::WA_TransparentForMouseEvents
        );

    dragPreview.setAttribute(
        Qt::WA_ShowWithoutActivating
        );

    dragPreview.setAttribute(
        Qt::WA_TranslucentBackground
        );

    dragPreview.setPixmap(
        dragPixmap
        );

    dragPreview.resize(
        dragPixmap.size()
        );

    dragPreview.move(
        QCursor::pos()
        - hotSpot
        );

    m_externalDragPreview =
        &dragPreview;

    dragPreview.show();
    dragPreview.raise();

    /*
     * QDrag::exec() runs a nested event loop, so a
     * timer can continue following the pointer while
     * the native drag operation is active.
     */
    QTimer dragPreviewTimer;

    dragPreviewTimer.setInterval(
        16
        );

    connect(
        &dragPreviewTimer,
        &QTimer::timeout,
        this,
        [this, hotSpot]() {
            if (m_externalDragPreview.isNull()
                || !m_externalDragPreview
                        ->isVisible()) {
                return;
            }

            m_externalDragPreview->move(
                QCursor::pos()
                - hotSpot
                );
        }
        );

    dragPreviewTimer.start();

    /*
     * QTabBar has already started its own movable-tab
     * gesture. Finish that cleanly BEFORE disabling
     * movable behavior or removing the real tab.
     */
    finishBuiltInMoveBeforeExternalDrag(
        event,
        index
        );

    m_externalDragActive =
        true;

    m_pressedDocumentId.clear();

    /*
     * The built-in gesture is now fully finished, so
     * QDrag owns the interaction from here onward.
     */
    setMovable(
        false
        );

    /*
     * The root workspace can now safely extract the
     * document without leaving QTabBar in a stale
     * pressed/moving visual state.
     */
    emit documentDragStarted(
        documentId
        );

    const Qt::DropAction result =
        drag.exec(
            Qt::MoveAction
            );

    /*
     * The external proxy is no longer needed once
     * the platform drag operation has ended.
     */
    dragPreviewTimer.stop();

    dragPreview.hide();

    m_externalDragPreview.clear();

    m_externalDragActive =
        false;

    setMovable(
        true
        );

    QObject *dropTarget =
        drag.target();

    auto *targetTabBar =
        qobject_cast<
            WorkspaceTabBar *>(
            dropTarget
            );

    /*
     * If no TraceScope tab bar accepted the drop,
     * turn the pending document into a new detached
     * workspace window at the release position.
     */
    if (result != Qt::MoveAction
        || targetTabBar == nullptr) {
        emit documentTearOutRequested(
            documentId,
            QCursor::pos()
            );
    }

    /*
     * An empty detached source window must survive
     * until QDrag::exec() returns. It is safe for
     * the root host to clean it up now.
     */
    emit externalDragCompleted();
}

void WorkspaceTabBar::dragEnterEvent(
    QDragEnterEvent *event
    )
{
    if (event == nullptr
        || !event->mimeData()
                ->hasFormat(
                    WorkspaceDocumentMimeType
                    )) {
        QTabBar::dragEnterEvent(
            event
            );

        return;
    }

    m_workspaceDragOver =
        true;

    m_previewSourceTabBar =
        qobject_cast<
            WorkspaceTabBar *>(
            event->source()
            );

    if (!m_previewSourceTabBar.isNull()) {
        m_previewSourceTabBar
            ->setExternalDragPreviewVisible(
                false
                );
    }

    m_dropPreviewTitle =
        event->mimeData()
            ->text();

    bool widthOk = false;

    const int requestedWidth =
        event->mimeData()
            ->data(
                WorkspaceDocumentWidthMimeType
                )
            .toInt(
                &widthOk
                );

    m_dropPreviewWidth =
        widthOk
            ? qMax(
                  80,
                  requestedWidth
                  )
            : 120;

    m_dropPreviewIndex =
        insertionIndexAt(
            event->position()
                .toPoint()
            );

    hideTabButtonsForPreview();

    update();

    event->setDropAction(
        Qt::MoveAction
        );

    event->accept();
}

void WorkspaceTabBar::dragMoveEvent(
    QDragMoveEvent *event
    )
{
    if (event == nullptr
        || !event->mimeData()
                ->hasFormat(
                    WorkspaceDocumentMimeType
                    )) {
        QTabBar::dragMoveEvent(
            event
            );

        return;
    }

    const int index =
        insertionIndexAt(
            event->position()
                .toPoint()
            );

    if (m_dropPreviewIndex
        != index) {
        m_dropPreviewIndex =
            index;

        update();
    }

    event->setDropAction(
        Qt::MoveAction
        );

    event->accept();
}

void WorkspaceTabBar::dragLeaveEvent(
    QDragLeaveEvent *event
    )
{
    m_workspaceDragOver =
        false;

    if (!m_previewSourceTabBar.isNull()) {
        m_previewSourceTabBar
            ->setExternalDragPreviewVisible(
                true
                );
    }

    m_previewSourceTabBar.clear();

    clearDropPreview();

    QTabBar::dragLeaveEvent(
        event
        );
}

void WorkspaceTabBar::dropEvent(
    QDropEvent *event
    )
{
    if (event == nullptr
        || !event->mimeData()
                ->hasFormat(
                    WorkspaceDocumentMimeType
                    )) {
        QTabBar::dropEvent(
            event
            );

        return;
    }

    const QString documentId =
        QString::fromUtf8(
            event->mimeData()
                ->data(
                    WorkspaceDocumentMimeType
                    )
            );

    const int targetIndex =
        insertionIndexAt(
            event->position()
                .toPoint()
            );

    m_workspaceDragOver =
        false;

    /*
     * The native drag is ending here, so the
     * floating external preview does not need to
     * be restored.
     */
    m_previewSourceTabBar.clear();

    /*
     * Remove all temporary insertion-preview state
     * before the real document is inserted.
     */
    clearDropPreview();

    if (documentId.isEmpty()) {
        event->ignore();
        return;
    }

    /*
     * This signal is delivered synchronously in the
     * GUI thread. By the time emit returns, the host
     * has moved the real WorkspaceDocument into this
     * tab group.
     */
    emit documentDropRequested(
        documentId,
        targetIndex
        );

    event->setDropAction(
        Qt::MoveAction
        );

    event->accept();
}

void WorkspaceTabBar::paintEvent(
    QPaintEvent *event
    )
{
    /*
     * Normal tab-bar painting when there is no
     * insertion preview.
     */
    if (m_dropPreviewIndex < 0
        || m_dropPreviewTitle.isEmpty()) {
        QTabBar::paintEvent(
            event
            );

        /*
         * An empty tab group should advertise itself
         * only while it is relevant to an active
         * workspace drag.
         *
         * This prevents "Drop tab here" from showing
         * on normal application startup or after all
         * documents have simply been closed.
         */
        if (count() == 0
            && (m_externalDragActive
                || m_workspaceDragOver)) {
            QPainter painter(this);

            painter.setPen(
                palette().color(
                    QPalette::PlaceholderText
                    )
                );

            painter.drawText(
                rect().adjusted(
                    12,
                    0,
                    -12,
                    0
                    ),
                Qt::AlignLeft
                    | Qt::AlignVCenter,
                tr("Drop tab here")
                );
        }

        return;
    }

    /*
     * During an insertion preview, paint a temporary
     * tab layout in which tabs after the insertion
     * point are shifted aside to make room.
     */
    QPainter painter(this);

    painter.fillRect(
        rect(),
        palette().window()
        );

    for (int index = 0;
         index < count();
         ++index) {
        QStyleOptionTab option;

        initStyleOption(
            &option,
            index
            );

        option.rect =
            displayedTabRect(
                index
                );

        style()->drawControl(
            QStyle::CE_TabBarTab,
            &option,
            &painter,
            this
            );
    }

    /*
     * Paint the dragged document as a disabled-style
     * temporary tab in the proposed insertion gap.
     */
    QStyleOptionTab previewOption;

    previewOption.initFrom(
        this
        );

    previewOption.rect =
        dropPreviewRect();

    previewOption.text =
        m_dropPreviewTitle;

    previewOption.shape =
        shape();

    previewOption.state &=
        ~QStyle::State_Enabled;

    style()->drawControl(
        QStyle::CE_TabBarTab,
        &previewOption,
        &painter,
        this
        );
}

int WorkspaceTabBar::indexForDocumentId(
    const QString &documentId
    ) const
{
    for (int index = 0;
         index < count();
         ++index) {
        if (tabData(index)
                .toString()
            == documentId) {
            return index;
        }
    }

    return -1;
}

int WorkspaceTabBar::insertionIndexAt(
    const QPoint &position
    ) const
{
    if (count() == 0) {
        return 0;
    }

    /*
     * If a preview is already active, its gap is
     * part of the visual layout the user is
     * interacting with.
     */
    if (m_dropPreviewIndex >= 0) {
        const QRect previewRectangle =
            dropPreviewRect();

        if (previewRectangle.contains(
                position
                )) {
            return m_dropPreviewIndex;
        }
    }

    /*
     * Choose an insertion position from the visual
     * centers of the shifted tabs rather than their
     * underlying QTabBar geometry.
     */
    for (int index = 0;
         index < count();
         ++index) {
        const QRect rectangle =
            displayedTabRect(
                index
                );

        if (position.x()
            < rectangle
                  .center()
                  .x()) {
            return index;
        }
    }

    return count();
}

void WorkspaceTabBar::clearDropPreview()
{
    const bool hadPreview =
        m_dropPreviewIndex >= 0
        || !m_dropPreviewTitle.isEmpty();

    m_dropPreviewIndex =
        -1;

    m_dropPreviewTitle.clear();

    restoreTabButtonsAfterPreview();

    if (hadPreview) {
        update();
    }
}

QRect WorkspaceTabBar::displayedTabRect(
    int index
    ) const
{
    QRect rectangle =
        tabRect(index);

    if (m_dropPreviewIndex >= 0
        && index >= m_dropPreviewIndex) {
        rectangle.translate(
            m_dropPreviewWidth,
            0
            );
    }

    return rectangle;
}

QRect WorkspaceTabBar::dropPreviewRect()
    const
{
    if (m_dropPreviewIndex < 0) {
        return QRect();
    }

    int x = 0;

    if (count() == 0) {
        x = 0;
    } else if (
        m_dropPreviewIndex <= 0
        ) {
        x =
            tabRect(0)
                .left();
    } else if (
        m_dropPreviewIndex >= count()
        ) {
        x =
            tabRect(
                count() - 1
                )
                .right()
            + 1;
    } else {
        x =
            tabRect(
                m_dropPreviewIndex
                )
                .left();
    }

    return QRect(
        x,
        0,
        m_dropPreviewWidth,
        height()
        );
}

void WorkspaceTabBar::
    hideTabButtonsForPreview()
{
    if (!m_hiddenTabButtons.isEmpty()) {
        return;
    }

    for (int index = 0;
         index < count();
         ++index) {
        QWidget *leftButton =
            tabButton(
                index,
                QTabBar::LeftSide
                );

        if (leftButton != nullptr
            && leftButton->isVisible()) {
            m_hiddenTabButtons.push_back(
                leftButton
                );

            leftButton->hide();
        }

        QWidget *rightButton =
            tabButton(
                index,
                QTabBar::RightSide
                );

        if (rightButton != nullptr
            && rightButton->isVisible()) {
            m_hiddenTabButtons.push_back(
                rightButton
                );

            rightButton->hide();
        }
    }
}

void WorkspaceTabBar::
    restoreTabButtonsAfterPreview()
{
    for (const QPointer<QWidget> &button
         : std::as_const(
             m_hiddenTabButtons
             )) {
        if (!button.isNull()) {
            button->show();
        }
    }

    m_hiddenTabButtons.clear();
}

int WorkspaceTabBar::
    emptyDropTargetHeight() const
{
    QStyleOptionTab option;

    option.initFrom(
        this
        );

    const QSize styledSize =
        style()->sizeFromContents(
            QStyle::CT_TabBarTab,
            &option,
            QSize(
                120,
                fontMetrics().height()
                ),
            this
            );

    return qMax(
        28,
        styledSize.height()
        );
}

QSize WorkspaceTabBar::sizeHint()
    const
{
    QSize result =
        QTabBar::sizeHint();

    if (count() == 0) {
        result.setWidth(
            qMax(
                180,
                result.width()
                )
            );

        result.setHeight(
            qMax(
                emptyDropTargetHeight(),
                result.height()
                )
            );
    }

    return result;
}

QSize WorkspaceTabBar::minimumSizeHint()
    const
{
    QSize result =
        QTabBar::minimumSizeHint();

    if (count() == 0) {
        result.setWidth(
            qMax(
                180,
                result.width()
                )
            );

        result.setHeight(
            qMax(
                emptyDropTargetHeight(),
                result.height()
                )
            );
    }

    return result;
}

void WorkspaceTabBar::
    setExternalDragPreviewVisible(
        bool visible
        )
{
    if (m_externalDragPreview.isNull()) {
        return;
    }

    if (!visible) {
        m_externalDragPreview->hide();
        return;
    }

    m_externalDragPreview->move(
        QCursor::pos()
        );

    m_externalDragPreview->show();
    m_externalDragPreview->raise();
}

void WorkspaceTabBar::
    finishBuiltInMoveBeforeExternalDrag(
        QMouseEvent *event,
        int sourceIndex
        )
{
    if (event == nullptr
        || sourceIndex < 0
        || sourceIndex >= count()) {
        return;
    }

    /*
     * QTabBar has already entered its own movable-tab
     * drag state by the time we decide that the user
     * has left the tab strip.
     *
     * Finish that gesture through QTabBar's normal
     * mouse-release path before physically extracting
     * the document. This clears Qt's private
     * pressedIndex, dragInProgress, drag offsets, and
     * moving-tab presentation.
     *
     * Use the center of the real source tab rather
     * than the actual off-bar pointer position so a
     * platform style that selects on mouse release
     * does not temporarily select index -1.
     */
    const QPoint localReleasePosition =
        tabRect(
            sourceIndex
            )
            .center();

    const QPoint globalReleasePosition =
        mapToGlobal(
            localReleasePosition
            );

    QMouseEvent releaseEvent(
        QEvent::MouseButtonRelease,
        QPointF(
            localReleasePosition
            ),
        QPointF(
            globalReleasePosition
            ),
        Qt::LeftButton,
        Qt::NoButton,
        event->modifiers()
        );

    QTabBar::mouseReleaseEvent(
        &releaseEvent
        );

    update();
}