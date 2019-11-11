#include "CCWidget.h"
#include <QPainter>

CCWidget::CCWidget(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    oldratio=0;
    m_w=0;
    m_h=0;
}

unsigned int CCWidget::imageBorderSize() const
{
    const int w=width();
    int tw=46;
    if(w<50)
        tw=46/4;
    else if(w>250)
        tw=46;
    else
        tw=(w-50)*46*3/4/200+46/4;
    int th=46;
    const int h=height();
    if(h<50)
        th=46/4;
    else if(h>250)
        th=46;
    else
        /* (h-50) -> 0 to 200
         * (h-50)*46*3/4/200 -> 0 to 46*3/4
         * (h-50)*46*3/4/200+46/4 -> 46/4 to 46 */
        th=(h-50)*46*3/4/200+46/4;
    int min=tw;
    if(min>th)
        min=th;

    return min;
}

QRectF CCWidget::boundingRect() const
{
    return QRectF();
}

void CCWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const int min=imageBorderSize();

    if(tl.isNull() || min!=tl.height())
    {
        QPixmap background(":/CC/images/interface/message.png");
        if(background.isNull())
            abort();
        tl=background.copy(0,0,46,46);
        tm=background.copy(46,0,401,46);
        tr=background.copy(447,0,46,46);
        ml=background.copy(0,46,46,179);
        mm=background.copy(46,46,401,179);
        mr=background.copy(447,46,46,179);
        bl=background.copy(0,225,46,46);
        bm=background.copy(46,225,401,46);
        br=background.copy(447,225,46,46);

        if(min!=tl.height())
        {
            tl=tl.scaledToHeight(min,Qt::SmoothTransformation);
            tm=tm.scaledToHeight(min,Qt::SmoothTransformation);
            tr=tr.scaledToHeight(min,Qt::SmoothTransformation);
            ml=ml.scaledToWidth(min,Qt::SmoothTransformation);
            //mm=mm.scaledToHeight(minsize,Qt::SmoothTransformation);
            mr=mr.scaledToWidth(min,Qt::SmoothTransformation);
            bl=bl.scaledToHeight(min,Qt::SmoothTransformation);
            bm=bm.scaledToHeight(min,Qt::SmoothTransformation);
            br=br.scaledToHeight(min,Qt::SmoothTransformation);
        }
    }

    painter->drawPixmap(0,             0,min,          min,tl);
    painter->drawPixmap(min,           0,width()-min*2,min,tm);
    painter->drawPixmap(width()-min,   0,min,          min,tr);

    painter->drawPixmap(0,             min,min,          height()-min*2,ml);
    painter->drawPixmap(min,           min,width()-min*2,height()-min*2,mm);
    painter->drawPixmap(width()-min,   min,min,          height()-min*2,mr);

    painter->drawPixmap(0,             height()-min,min,           min,bl);
    painter->drawPixmap(min,           height()-min,width()-min*2, min,bm);
    painter->drawPixmap(width()-min,   height()-min,min,           min,br);
}

unsigned int CCWidget::currentBorderSize() const
{
    return imageBorderSize()/2;
}

void CCWidget::setWidth(const int &w)
{
    this->m_w=w;
}

void CCWidget::setHeight(const int &h)
{
    this->m_h=h;
}

int CCWidget::width() const
{
    return m_w;
}

int CCWidget::height() const
{
    return m_h;
}

