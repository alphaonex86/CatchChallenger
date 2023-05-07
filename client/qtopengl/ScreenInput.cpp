#include "ScreenInput.hpp"

void ScreenInput::mousePressEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
    Q_UNUSED(callParentClass);
}

void ScreenInput::mouseReleaseEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
    Q_UNUSED(callParentClass);
}

void ScreenInput::mouseMoveEventXY(const QPointF &p,bool &pressValidated/*if true then don't do action*/,bool &callParentClass)
{
    Q_UNUSED(p);
    Q_UNUSED(pressValidated);
    Q_UNUSED(callParentClass);
}

void ScreenInput::keyPressEvent(QKeyEvent * event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    Q_UNUSED(eventTriggerGeneral);
}

void ScreenInput::keyReleaseEvent(QKeyEvent *event, bool &eventTriggerGeneral)
{
    Q_UNUSED(event);
    Q_UNUSED(eventTriggerGeneral);
}

void ScreenInput::removeAbove()
{
    emit setAbove(nullptr);
}
