#include "LiveFollowTabControl.h"

#include <QColor>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QSignalBlocker>
#include <QToolButton>

#include "../InterfaceScale.h"
#include "../../live/LiveSessionFollowCoordinator.h"
#include "../../workspace/InvestigationSession.h"

#include "WorkspaceTabBar.h"

namespace
{

constexpr int CompactBadgeWidth = 30;
constexpr int ActiveBadgeWidth = 38;

constexpr int ControlButtonSize = 18;
constexpr int ControlIconCanvasSize = 16;
constexpr int ControlIconDisplaySize = 14;
constexpr int ControlSpacing = 1;

enum class ControlIconKind
{
    Play,
    Pause,
    Stop,
    FollowNewest
};

struct LiveControlColors
{
    QColor following;
    QColor paused;
    QColor stopped;
    QColor error;
};

LiveControlColors liveControlColors(
    const QPalette &palette
    )
{
    const bool dark =
        palette
            .color(
                QPalette::Window
                )
            .lightness()
        < 128;

    return {
        dark
            ? QColor(
                  QStringLiteral(
                      "#66BB6A"
                      )
                  )
            : QColor(
                  QStringLiteral(
                      "#2E7D32"
                      )
                  ),

        dark
            ? QColor(
                  QStringLiteral(
                      "#FFB74D"
                      )
                  )
            : QColor(
                  QStringLiteral(
                      "#B26A00"
                      )
                  ),

        QColor(
            QStringLiteral(
                "#8A8A8A"
                )
            ),

        dark
            ? QColor(
                  QStringLiteral(
                      "#EF5350"
                      )
                  )
            : QColor(
                  QStringLiteral(
                      "#C62828"
                      )
                  )
    };
}

QIcon controlIcon(
    ControlIconKind kind,
    const QColor &color,
    const QWidget *widget
    )
{
    const int logicalSize =
        InterfaceScale::pixels(
            ControlIconCanvasSize,
            widget
            );

    const qreal devicePixelRatio =
        widget != nullptr
            ? widget->devicePixelRatioF()
            : 1.0;

    const int physicalSize =
        qMax(
            1,
            qRound(
                logicalSize
                * devicePixelRatio
                )
            );

    QPixmap pixmap(
        physicalSize,
        physicalSize
        );

    pixmap.setDevicePixelRatio(
        devicePixelRatio
        );

    pixmap.fill(
        Qt::transparent
        );

    QPainter painter(
        &pixmap
        );

    painter.setRenderHint(
        QPainter::Antialiasing,
        true
        );

    /*
     * Keep the icon artwork authored in the original
     * 16 x 16 coordinate system while scaling the
     * logical canvas through InterfaceScale.
     */
    painter.scale(
        static_cast<qreal>(
            logicalSize
            )
            / ControlIconCanvasSize,
        static_cast<qreal>(
            logicalSize
            )
            / ControlIconCanvasSize
        );

    painter.setPen(
        Qt::NoPen
        );

    painter.setBrush(
        color
        );

    switch (kind) {
    case ControlIconKind::Play:
        painter.drawPolygon(
            QPolygonF{
                QPointF(5.0, 3.0),
                QPointF(12.5, 8.0),
                QPointF(5.0, 13.0)
            }
            );
        break;

    case ControlIconKind::Pause:
        painter.drawRoundedRect(
            QRectF(
                4.0,
                3.0,
                3.0,
                10.0
                ),
            0.8,
            0.8
            );

        painter.drawRoundedRect(
            QRectF(
                9.0,
                3.0,
                3.0,
                10.0
                ),
            0.8,
            0.8
            );
        break;

    case ControlIconKind::Stop:
        painter.drawRoundedRect(
            QRectF(
                4.0,
                4.0,
                8.0,
                8.0
                ),
            0.8,
            0.8
            );
        break;

    case ControlIconKind::FollowNewest:
        painter.setPen(
            QPen(
                color,
                1.8,
                Qt::SolidLine,
                Qt::RoundCap,
                Qt::RoundJoin
                )
            );

        painter.drawLine(
            QPointF(8.0, 3.0),
            QPointF(8.0, 10.0)
            );

        painter.drawLine(
            QPointF(4.5, 7.0),
            QPointF(8.0, 10.5)
            );

        painter.drawLine(
            QPointF(11.5, 7.0),
            QPointF(8.0, 10.5)
            );

        painter.drawLine(
            QPointF(4.0, 13.0),
            QPointF(12.0, 13.0)
            );
        break;
    }

    return QIcon(
        pixmap
        );
}

QString rgbaStyleValue(
    const QColor &color,
    int alpha
    )
{
    return QStringLiteral(
               "rgba(%1, %2, %3, %4)"
               )
        .arg(color.red())
        .arg(color.green())
        .arg(color.blue())
        .arg(alpha);
}

}

LiveFollowTabControl::
    LiveFollowTabControl(
        InvestigationSession *session,
        QWidget *parent
        )
    : QWidget(parent),
    m_session(session),
    m_statusBadge(
        new QLabel(
            tr("LIVE"),
            this
            )
        ),
    m_primaryButton(
        new QToolButton(this)
        ),
    m_stopButton(
        new QToolButton(this)
        ),
    m_followNewestButton(
        new QToolButton(this)
        )
{
    auto *layout =
        new QHBoxLayout(this);

    layout->setContentsMargins(
        0,
        0,
        0,
        0
        );

    layout->setSpacing(
        InterfaceScale::pixels(
            ControlSpacing,
            this
            )
        );

    setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );

    /*
     * The badge itself changes width based on state.
     * It must not expand automatically into unused
     * space when one of the control buttons is hidden.
     */
    m_statusBadge->setFixedHeight(
        InterfaceScale::pixels(
            ControlButtonSize,
            this
            )
        );

    m_statusBadge->setFixedWidth(
        InterfaceScale::pixels(
            CompactBadgeWidth,
            this
            )
        );

    m_statusBadge->setSizePolicy(
        QSizePolicy::Fixed,
        QSizePolicy::Fixed
        );

    m_statusBadge->setAlignment(
        Qt::AlignCenter
        );

    m_statusBadge->setObjectName(
        QStringLiteral(
            "liveFollowStatusBadge"
            )
        );

    m_statusBadge->setAccessibleName(
        tr(
            "Live following status"
            )
        );

    const int controlButtonSize =
        InterfaceScale::pixels(
            ControlButtonSize,
            this
            );

    const int controlIconDisplaySize =
        InterfaceScale::pixels(
            ControlIconDisplaySize,
            this
            );

    /*
     * Both action buttons use the same compact
     * geometry so the accessory remains visually
     * consistent as controls appear and disappear.
     */
    for (QToolButton *button
         : {
             m_primaryButton,
             m_stopButton,
             m_followNewestButton
         }) {
        button->setFixedSize(
            controlButtonSize,
            controlButtonSize
            );

        button->setIconSize(
            QSize(
                controlIconDisplaySize,
                controlIconDisplaySize
                )
            );

        button->setAutoRaise(
            true
            );

        button->setSizePolicy(
            QSizePolicy::Fixed,
            QSizePolicy::Fixed
            );
    }

    m_primaryButton->setObjectName(
        QStringLiteral(
            "liveFollowPrimaryButton"
            )
        );

    m_stopButton->setObjectName(
        QStringLiteral(
            "liveFollowStopButton"
            )
        );

    m_followNewestButton->setObjectName(
        QStringLiteral(
            "liveFollowNewestButton"
            )
        );

    m_followNewestButton->setCheckable(
        true
        );

    layout->addWidget(
        m_statusBadge
        );

    layout->addWidget(
        m_primaryButton
        );

    layout->addWidget(
        m_stopButton
        );

    layout->addWidget(
        m_followNewestButton
        );

    layout->setAlignment(
        Qt::AlignLeft
        | Qt::AlignVCenter
        );

    connect(
        m_primaryButton,
        &QToolButton::clicked,
        this,
        &LiveFollowTabControl::
        handlePrimaryAction
        );

    connect(
        m_stopButton,
        &QToolButton::clicked,
        this,
        &LiveFollowTabControl::
        handleStop
        );

    connect(
        m_followNewestButton,
        &QToolButton::toggled,
        this,
        [this](
            bool enabled
            ) {
            refreshPresentation();

            emit followNewestChanged(
                enabled
                );
        }
        );

    /*
     * If this tab accessory is being recreated after
     * a tear-out or redock, reconnect to the existing
     * session-owned coordinator rather than creating
     * a new one.
     */
    if (m_session != nullptr) {
        attachCoordinator(
            m_session
                ->liveFollowCoordinator()
            );
    }

    refreshPresentation();
}

void LiveFollowTabControl::
    attachCoordinator(
        LiveSessionFollowCoordinator *coordinator
        )
{
    if (m_coordinator
        == coordinator) {
        return;
    }

    if (m_coordinator != nullptr) {
        disconnect(
            m_coordinator,
            nullptr,
            this,
            nullptr
            );
    }

    m_coordinator =
        coordinator;

    if (m_coordinator == nullptr) {
        refreshPresentation();
        return;
    }

    connect(
        m_coordinator,
        &LiveSessionFollowCoordinator::
        stateChanged,
        this,
        [this]() {
            clearError();
            refreshPresentation();

            emit liveFollowStateChanged();
        }
        );

    connect(
        m_coordinator,
        &LiveSessionFollowCoordinator::
        sessionUpdated,
        this,
        [this]() {
            clearError();
            refreshPresentation();

            emit liveSessionUpdated();
        }
        );

    connect(
        m_coordinator,
        &LiveSessionFollowCoordinator::
        pollError,
        this,
        [this](
            const QString &errorMessage
            ) {
            setError(
                errorMessage
                );
        }
        );

    connect(
        m_coordinator,
        &QObject::destroyed,
        this,
        [this]() {
            m_coordinator =
                nullptr;

            clearError();
            refreshPresentation();
        }
        );

    refreshPresentation();
}

void LiveFollowTabControl::
    handlePrimaryAction()
{
    if (m_session == nullptr) {
        return;
    }

    LiveSessionFollowCoordinator *coordinator =
        m_session
            ->ensureLiveFollowCoordinator();

    if (coordinator == nullptr) {
        setError(
            tr(
                "Live following is unavailable "
                "for this session."
                )
            );

        return;
    }

    attachCoordinator(
        coordinator
        );

    if (coordinator
            ->state()
            .isFollowing()) {
        if (!coordinator->pause()) {
            setError(
                tr(
                    "Live following could not "
                    "be paused."
                    )
                );
        }

        return;
    }

    if (coordinator
            ->state()
            .isPaused()) {
        if (!coordinator->resume()) {
            setError(
                tr(
                    "Live following could not "
                    "be resumed."
                    )
                );
        }

        return;
    }

    const LiveSessionFollowStartResult
        result =
        coordinator->start();

    if (!result.succeeded) {
        setError(
            result.errorMessage
            );

        return;
    }

    clearError();
    refreshPresentation();
}

void LiveFollowTabControl::
    handleStop()
{
    if (m_coordinator == nullptr
        || m_coordinator
               ->state()
               .isStopped()) {
        return;
    }

    if (!m_coordinator->stop()) {
        setError(
            tr(
                "Live following could not "
                "be stopped."
                )
            );

        return;
    }

    clearError();
    refreshPresentation();
}

void LiveFollowTabControl::
    setFollowNewestEnabled(
        bool enabled
        )
{
    if (m_followNewestButton == nullptr) {
        return;
    }

    {
        const QSignalBlocker blocker(
            m_followNewestButton
            );

        m_followNewestButton->setChecked(
            enabled
            );
    }

    refreshPresentation();
}

void LiveFollowTabControl::
    setError(
        const QString &errorMessage
        )
{
    m_errorMessage =
        errorMessage.trimmed();

    if (m_errorMessage.isEmpty()) {
        m_errorMessage =
            tr(
                "An unknown live-follow error "
                "occurred."
                );
    }

    refreshPresentation();
}

void LiveFollowTabControl::
    clearError()
{
    m_errorMessage.clear();
}

void LiveFollowTabControl::
    refreshPresentation()
{
    const LiveControlColors colors =
        liveControlColors(
            palette()
            );

    const bool hasError =
        !m_errorMessage.isEmpty();

    LiveFileFollowStatus status =
        LiveFileFollowStatus::Stopped;

    if (m_coordinator != nullptr) {
        status =
            m_coordinator
                ->state()
                .status();
    }

    const bool activelyFollowing =
        status
            == LiveFileFollowStatus::Following
        && !hasError;

    bool followNewestWasCleared =
        false;

    if (!activelyFollowing
        && m_followNewestButton->isChecked()) {
        {
            const QSignalBlocker blocker(
                m_followNewestButton
                );

            m_followNewestButton->setChecked(
                false
                );
        }

        followNewestWasCleared =
            true;
    }

    m_followNewestButton->setEnabled(
        activelyFollowing
        );

    if (followNewestWasCleared) {
        emit followNewestChanged(
            false
            );
    }

    /*
     * Following receives the strongest visual
     * treatment. Stopped, paused, and error states
     * use the compact badge.
     */
    m_statusBadge->setFixedWidth(
        InterfaceScale::pixels(
            activelyFollowing
                ? ActiveBadgeWidth
                : CompactBadgeWidth,
            this
            )
        );

    QColor badgeColor =
        colors.stopped;

    QString statusText =
        tr("Stopped");

    if (hasError) {
        badgeColor =
            colors.error;

        statusText =
            tr("Error");
    } else {
        switch (status) {
        case LiveFileFollowStatus::Stopped:
            badgeColor =
                colors.stopped;

            statusText =
                tr("Stopped");
            break;

        case LiveFileFollowStatus::Following:
            badgeColor =
                colors.following;

            statusText =
                tr("Following");
            break;

        case LiveFileFollowStatus::Paused:
            badgeColor =
                colors.paused;

            statusText =
                tr("Paused");
            break;
        }
    }

    const int horizontalPadding =
        InterfaceScale::pixels(
            activelyFollowing
                ? 4
                : 2,
            this
            );

    const int borderRadius =
        InterfaceScale::pixels(
            3,
            this
            );

    const int fontWeight =
        activelyFollowing
            ? 700
            : 600;

    m_statusBadge->setStyleSheet(
        QStringLiteral(
            "QLabel {"
            " color: %1;"
            " border: 1px solid %1;"
            " border-radius: %2px;"
            " padding: 0px %3px;"
            " font-weight: %4;"
            " background-color: %5;"
            "}"
            )
            .arg(
                badgeColor.name()
                )
            .arg(
                borderRadius
                )
            .arg(
                horizontalPadding
                )
            .arg(
                fontWeight
                )
            .arg(
                rgbaStyleValue(
                    badgeColor,
                    24
                    )
                )
        );

    QString statusToolTip =
        tr(
            "Live following: %1"
            )
            .arg(
                statusText
                );

    if (hasError) {
        statusToolTip +=
            QStringLiteral("\n")
            + m_errorMessage;
    }

    m_statusBadge->setToolTip(
        statusToolTip
        );

    /*
     * Primary action:
     *
     * Following -> Pause
     * Paused    -> Resume
     * Stopped   -> Start
     */
    if (status
        == LiveFileFollowStatus::Following) {
        m_primaryButton->setIcon(
            controlIcon(
                ControlIconKind::Pause,
                colors.paused,
                m_primaryButton
                )
            );

        m_primaryButton->setToolTip(
            tr(
                "Pause live following"
                )
            );

        m_primaryButton->setAccessibleName(
            tr(
                "Pause live following"
                )
            );
    } else {
        m_primaryButton->setIcon(
            controlIcon(
                ControlIconKind::Play,
                colors.following,
                m_primaryButton
                )
            );

        const QString actionText =
            status
                    == LiveFileFollowStatus::Paused
                ? tr(
                      "Resume live following"
                      )
                : tr(
                      "Start live following"
                      );

        m_primaryButton->setToolTip(
            actionText
            );

        m_primaryButton->setAccessibleName(
            actionText
            );
    }

    m_stopButton->setIcon(
        controlIcon(
            ControlIconKind::Stop,
            colors.error,
            m_stopButton
            )
        );

    m_stopButton->setToolTip(
        tr(
            "Stop live following"
            )
        );

    m_stopButton->setAccessibleName(
        tr(
            "Stop live following"
            )
        );

    const bool followNewest =
        m_followNewestButton->isChecked();

    const QColor followNewestColor =
        followNewest
            ? colors.following
            : colors.stopped;

    m_followNewestButton->setIcon(
        controlIcon(
            ControlIconKind::FollowNewest,
            followNewestColor,
            m_followNewestButton
            )
        );

    m_followNewestButton->setToolTip(
        followNewest
            ? tr(
                  "Stop following newest events"
                  )
            : tr(
                  "Follow newest events"
                  )
        );

    m_followNewestButton->setAccessibleName(
        followNewest
            ? tr(
                  "Stop following newest events"
                  )
            : tr(
                  "Follow newest events"
                  )
        );

    if (followNewest) {
        m_followNewestButton->setStyleSheet(
            QStringLiteral(
                "QToolButton {"
                " border: 1px solid %1;"
                " border-radius: %2px;"
                " background-color: %3;"
                " padding: 0px;"
                " margin: 0px;"
                "}"
                )
                .arg(
                    colors.following.name()
                    )
                .arg(
                    borderRadius
                    )
                .arg(
                    rgbaStyleValue(
                        colors.following,
                        40
                        )
                    )
            );
    } else {
        m_followNewestButton->setStyleSheet(
            QStringLiteral(
                "QToolButton {"
                " border: 1px solid transparent;"
                " padding: 0px;"
                " margin: 0px;"
                "}"
                )
            );
    }

    m_followNewestButton->setVisible(
        true
        );

    m_primaryButton->setVisible(
        true
        );

    m_stopButton->setVisible(
        status
        != LiveFileFollowStatus::Stopped
        );

    setToolTip(
        statusToolTip
        );

    refreshTabGeometry();
}

void LiveFollowTabControl::
    refreshTabGeometry()
{
    if (layout() != nullptr) {
        layout()->activate();
    }

    adjustSize();

    auto *tabBar =
        qobject_cast<WorkspaceTabBar *>(
            parentWidget()
            );

    if (tabBar == nullptr) {
        updateGeometry();
        return;
    }

    tabBar->refreshTabAccessoryLayout(
        this
        );
}