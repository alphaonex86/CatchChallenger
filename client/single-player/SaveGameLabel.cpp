#include "SaveGameLabel.h"
#include <QDebug>

SaveGameLabel::SaveGameLabel()
{
    haveFirstClick=false;
}

void SaveGameLabel::mousePressEvent(QMouseEvent *)
{
    //double click
    if(haveFirstClick && lastClick.elapsed()<400)
    {
        emit doubleClicked();
        haveFirstClick=false;
        return;
    }
    //simple click
    else
    {
        haveFirstClick=true;
        lastClick.restart();
        emit clicked();
    }
}

void SaveGameLabel::mouseReleaseEvent(QMouseEvent *)
{
}
