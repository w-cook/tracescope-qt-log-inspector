#include "NativeStyleAnimationSuppressor.h"

#include <QEvent>
#include <QWidget>

NativeStyleAnimationSuppressor::
    NativeStyleAnimationSuppressor(
        QObject *parent
        )
    : QObject(parent)
{
}

bool NativeStyleAnimationSuppressor::
    eventFilter(
        QObject *watched,
        QEvent *event
        )
{
    if (
        event != nullptr
        && event->type()
               == QEvent::Polish
        ) {
        suppressNativeAnimation(
            watched
            );
    }

    return QObject::eventFilter(
        watched,
        event
        );
}

void NativeStyleAnimationSuppressor::
    suppressNativeAnimation(
        QObject *object
        ) const
{
    QWidget *widget =
        qobject_cast<QWidget *>(
            object
            );

    if (widget == nullptr) {
        return;
    }

    /*
     * QWindowsVistaStyle uses this dynamic property
     * when deciding whether native state-transition
     * animations may run for a widget.
     *
     * At fractional Windows device-pixel ratios,
     * those transition buffers can produce transient
     * frame distortion during hover/focus/pressed
     * state changes.
     *
     * Apply the suppression when widgets are polished
     * so controls created later by dialogs and other
     * workflows receive the same application-wide
     * behavior.
     */
    widget->setProperty(
        "_q_no_animation",
        true
        );
}