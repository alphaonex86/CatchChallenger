#include "ListEntryEnvolued.h"

ListEntryEnvolued::ListEntryEnvolued()
{
    haveFirstClick=false;
}

void ListEntryEnvolued::mousePressEvent(QMouseEvent *)
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

void ListEntryEnvolued::mouseReleaseEvent(QMouseEvent *)
{
}
