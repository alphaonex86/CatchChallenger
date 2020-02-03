#ifndef SCREENINPUT_H
#define SCREENINPUT_H

#include <QGraphicsItem>
#include <QMouseEvent>

class ScreenInput : public QGraphicsItem
{
public:
    virtual void mousePressEventXY(QMouseEvent *event);
    virtual void mouseReleaseEventXY(QMouseEvent *event);
};

#endif // SCREENINPUT_H
