#ifndef SCREENINPUT_H
#define SCREENINPUT_H

#include <QGraphicsItem>
#include <QPointF>

class ScreenInput : public QGraphicsItem
{
public:
    virtual void mousePressEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void mouseReleaseEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void keyPressEvent(QKeyEvent * event);
    virtual void keyReleaseEvent(QKeyEvent *event);
};

#endif // SCREENINPUT_H
