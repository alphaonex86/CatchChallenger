#ifndef SCREENINPUT_H
#define SCREENINPUT_H

#include <QGraphicsItem>
#include <QPointF>
#include <QObject>

class ScreenInput : public QObject, public QGraphicsItem
{
    Q_OBJECT
public:
    virtual void mousePressEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void mouseReleaseEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass);
    virtual void keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral);
    virtual void keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral);
    void removeAbove();
signals:
    void setAbove(ScreenInput *widget);//first plan popup
};

#endif // SCREENINPUT_H
