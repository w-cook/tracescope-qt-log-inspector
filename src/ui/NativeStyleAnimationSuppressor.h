#pragma once

#include <QObject>

class QEvent;

class NativeStyleAnimationSuppressor
    : public QObject
{
public:
    explicit NativeStyleAnimationSuppressor(
        QObject *parent = nullptr
        );

protected:
    bool eventFilter(
        QObject *watched,
        QEvent *event
        ) override;

private:
    void suppressNativeAnimation(
        QObject *object
        ) const;
};