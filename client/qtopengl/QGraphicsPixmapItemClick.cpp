#include "QGraphicsPixmapItemClick.hpp"

QGraphicsPixmapItemClick::QGraphicsPixmapItemClick(const QPixmap &pixmap, QGraphicsItem *parent) :
    QGraphicsPixmapItem(pixmap,parent)
{
    pressed=false;
}

void QGraphicsPixmapItemClick::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    if(this->pressed)
        return;
    if(pressValidated)
        return;
    if(!isVisible())
        return;
    if(!isEnabled())
        return;
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(t.contains(p))
    {
        pressValidated=true;
        setPressed(true);
    }
}

void QGraphicsPixmapItemClick::mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated)
{
    if(previousPressValidated)
    {
        setPressed(false);
        return;
    }
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(!this->pressed)
        return;
    if(!isEnabled())
        return;
    if(!previousPressValidated && isVisible())
    {
        if(t.contains(p))
        {
            emit clicked();
            previousPressValidated=true;
        }
    }
    setPressed(false);
}

void QGraphicsPixmapItemClick::setPressed(const bool &pressed)
{
    this->pressed=pressed;
    //regenCache();
}
