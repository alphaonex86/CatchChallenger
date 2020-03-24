#ifndef SCREENINPUT_H
#define SCREENINPUT_H

#include <QGraphicsItem>
#include <QPointF>

class ScreenInput : public QGraphicsItem
{
public:
    virtual void mousePressEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/);
    virtual void mouseReleaseEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/);
    virtual void mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/);
};

#endif // SCREENINPUT_H
