#ifndef SCREENINPUT_H
#define SCREENINPUT_H

#include <QGraphicsItem>
#include <QPointF>

class ScreenInput : public QGraphicsItem
{
public:
    virtual void mousePressEventXY(const QPointF &p);
    virtual void mouseReleaseEventXY(const QPointF &p);
};

#endif // SCREENINPUT_H
