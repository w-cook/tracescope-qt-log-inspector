#include "DevicePixelAlignedWidgetHost.h"

#include <cmath>
#include <limits>

#include <QEvent>
#include <QMoveEvent>
#include <QResizeEvent>
#include <QtGlobal>

DevicePixelAlignedWidgetHost::
    DevicePixelAlignedWidgetHost(
        QWidget *contentWidget,
        Qt::Edges alignedEdges,
        QWidget *parent
        )
    : QWidget(parent),
    m_contentWidget(contentWidget),
    m_alignedEdges(alignedEdges)
{
    if (m_contentWidget == nullptr) {
        return;
    }

    setSizePolicy(
        m_contentWidget->sizePolicy()
        );

    setFocusProxy(
        m_contentWidget
        );

    m_contentWidget->setParent(
        this
        );

    m_contentWidget->show();

    updateContentGeometry();
}

QWidget *
    DevicePixelAlignedWidgetHost::
    contentWidget() const
{
    return m_contentWidget;
}

QSize DevicePixelAlignedWidgetHost::
    sizeHint() const
{
    if (m_contentWidget == nullptr) {
        return QWidget::sizeHint();
    }

    QSize result =
        m_contentWidget->sizeHint();

    if (!result.isValid()) {
        result =
            m_contentWidget
                ->minimumSizeHint();
    }

    result.setWidth(
        std::max(
            0,
            result.width()
            )
        + horizontalAllowance()
        );

    result.setHeight(
        std::max(
            0,
            result.height()
            )
        + verticalAllowance()
        );

    return result;
}

QSize DevicePixelAlignedWidgetHost::
    minimumSizeHint() const
{
    if (m_contentWidget == nullptr) {
        return QWidget::minimumSizeHint();
    }

    /*
     * Pixel-alignment space is optional presentation
     * space. It must not increase the layout's actual
     * minimum-size requirement.
     */
    return m_contentWidget
        ->minimumSizeHint()
        .expandedTo(
            m_contentWidget
                ->minimumSize()
            );
}

bool DevicePixelAlignedWidgetHost::
    event(
        QEvent *event
        )
{
    const bool handled =
        QWidget::event(
            event
            );

    if (event == nullptr) {
        return handled;
    }

    if (event->type()
            == QEvent::Show
#if QT_VERSION >= QT_VERSION_CHECK(6, 6, 0)
        || event->type()
               == QEvent::DevicePixelRatioChange
#endif
        ) {
        updateContentGeometry();
    }

    return handled;
}

void DevicePixelAlignedWidgetHost::
    moveEvent(
        QMoveEvent *event
        )
{
    QWidget::moveEvent(
        event
        );

    updateContentGeometry();
}

void DevicePixelAlignedWidgetHost::
    resizeEvent(
        QResizeEvent *event
        )
{
    QWidget::resizeEvent(
        event
        );

    updateContentGeometry();
}

int DevicePixelAlignedWidgetHost::
    alignmentInset(
        int logicalCoordinate,
        int direction
        ) const
{
    const qreal dpr =
        devicePixelRatioF();

    if (dpr <= 0.0) {
        return 0;
    }

    int bestInset =
        0;

    qreal bestError =
        std::numeric_limits<
            qreal
            >::max();

    constexpr qreal AlignmentTolerance =
        0.0001;

    for (
        int inset = 0;
        inset <= MaxAlignmentInset;
        ++inset
        ) {
        const int candidateCoordinate =
            logicalCoordinate
            + direction * inset;

        const qreal physicalCoordinate =
            candidateCoordinate
            * dpr;

        const qreal error =
            std::abs(
                physicalCoordinate
                - std::round(
                    physicalCoordinate
                    )
                );

        if (error < AlignmentTolerance) {
            return inset;
        }

        if (error < bestError) {
            bestError =
                error;

            bestInset =
                inset;
        }
    }

    return bestInset;
}

int DevicePixelAlignedWidgetHost::
    horizontalAllowance() const
{
    int allowance =
        0;

    if (m_alignedEdges.testFlag(
            Qt::LeftEdge
            )) {
        allowance +=
            MaxAlignmentInset;
    }

    if (m_alignedEdges.testFlag(
            Qt::RightEdge
            )) {
        allowance +=
            MaxAlignmentInset;
    }

    return allowance;
}

int DevicePixelAlignedWidgetHost::
    verticalAllowance() const
{
    int allowance =
        0;

    if (m_alignedEdges.testFlag(
            Qt::TopEdge
            )) {
        allowance +=
            MaxAlignmentInset;
    }

    if (m_alignedEdges.testFlag(
            Qt::BottomEdge
            )) {
        allowance +=
            MaxAlignmentInset;
    }

    return allowance;
}

void DevicePixelAlignedWidgetHost::
    updateContentGeometry()
{
    if (m_contentWidget == nullptr) {
        return;
    }

    QWidget *topLevel =
        window();

    const QPoint hostOrigin =
        topLevel != nullptr
            ? mapTo(
                  topLevel,
                  QPoint(0, 0)
                  )
            : pos();

    int desiredLeftInset =
        0;

    int desiredRightInset =
        0;

    int desiredTopInset =
        0;

    int desiredBottomInset =
        0;

    if (m_alignedEdges.testFlag(
            Qt::LeftEdge
            )) {
        desiredLeftInset =
            alignmentInset(
                hostOrigin.x(),
                1
                );
    }

    if (m_alignedEdges.testFlag(
            Qt::RightEdge
            )) {
        desiredRightInset =
            alignmentInset(
                hostOrigin.x()
                    + width(),
                -1
                );
    }

    if (m_alignedEdges.testFlag(
            Qt::TopEdge
            )) {
        desiredTopInset =
            alignmentInset(
                hostOrigin.y(),
                1
                );
    }

    if (m_alignedEdges.testFlag(
            Qt::BottomEdge
            )) {
        desiredBottomInset =
            alignmentInset(
                hostOrigin.y()
                    + height(),
                -1
                );
    }

    /*
     * Alignment must never make the wrapped widget
     * smaller than its own real minimum size.
     *
     * At constrained widths, presentation alignment
     * yields to usability.
     */
    const QSize minimumContentSize =
        m_contentWidget
            ->minimumSizeHint()
            .expandedTo(
                m_contentWidget
                    ->minimumSize()
                );

    int horizontalSpare =
        std::max(
            0,
            width()
                - minimumContentSize.width()
            );

    int verticalSpare =
        std::max(
            0,
            height()
                - minimumContentSize.height()
            );

    const int leftInset =
        std::min(
            desiredLeftInset,
            horizontalSpare
            );

    horizontalSpare -=
        leftInset;

    const int rightInset =
        std::min(
            desiredRightInset,
            horizontalSpare
            );

    const int topInset =
        std::min(
            desiredTopInset,
            verticalSpare
            );

    verticalSpare -=
        topInset;

    const int bottomInset =
        std::min(
            desiredBottomInset,
            verticalSpare
            );

    const QRect targetGeometry(
        leftInset,
        topInset,
        width()
            - leftInset
            - rightInset,
        height()
            - topInset
            - bottomInset
        );

    if (m_contentWidget->geometry()
        != targetGeometry) {
        m_contentWidget->setGeometry(
            targetGeometry
            );
    }
}