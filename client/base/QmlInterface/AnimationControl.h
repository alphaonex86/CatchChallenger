#ifndef AnimationControl_H
#define AnimationControl_H

#include <QObject>

class AnimationControl : public QObject
{
    Q_OBJECT
signals:
    Q_INVOKABLE void finished();
};

#endif // AnimationControl_H
