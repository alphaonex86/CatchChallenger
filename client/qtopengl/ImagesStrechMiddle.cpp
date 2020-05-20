#include "ImagesStrechMiddle.hpp"
#include "../qt/GameLoader.hpp"
#include <QPainter>

ImagesStrechMiddle::ImagesStrechMiddle(unsigned int borderSize,QString path,QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    oldratio=0;
    m_w=0;
    m_h=0;

    m_borderSize=borderSize;
    m_path=path;
}

unsigned int ImagesStrechMiddle::imageBorderSize() const
{
    const int w=width();
    int tw=m_borderSize;
    if(w<50)
        tw=m_borderSize/4;
    else if(w>250)
        tw=m_borderSize;
    else
        tw=(w-50)*m_borderSize*3/4/200+m_borderSize/4;
    int th=m_borderSize;
    const int h=height();
    if(h<50)
        th=m_borderSize/4;
    else if(h>250)
        th=m_borderSize;
    else
        /* (h-50) -> 0 to 200
         * (h-50)*m_borderSize*3/4/200 -> 0 to m_borderSize*3/4
         * (h-50)*m_borderSize*3/4/200+m_borderSize/4 -> m_borderSize/4 to m_borderSize */
        th=(h-50)*m_borderSize*3/4/200+m_borderSize/4;
    int min=tw;
    if(min>th)
        min=th;

    return min;
}

QRectF ImagesStrechMiddle::boundingRect() const
{
    return QRectF();
}

void ImagesStrechMiddle::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const int min=imageBorderSize();

    if(tl.isNull() || min!=tl.height())
    {
        QPixmap background;
        if(GameLoader::gameLoader!=nullptr)
            background=*GameLoader::gameLoader->getImage(m_path);
        else
            background=QPixmap(m_path);
        if(background.isNull())
            abort();
        tl=background.copy(0,0,m_borderSize,m_borderSize);
        tm=background.copy(m_borderSize,0,background.width()-m_borderSize-m_borderSize,m_borderSize);
        tr=background.copy(background.width()-m_borderSize,0,m_borderSize,m_borderSize);
        ml=background.copy(0,m_borderSize,m_borderSize,background.height()-m_borderSize-m_borderSize);
        mm=background.copy(m_borderSize,m_borderSize,background.width()-m_borderSize-m_borderSize,background.height()-m_borderSize-m_borderSize);
        mr=background.copy(background.width()-m_borderSize,m_borderSize,m_borderSize,background.height()-m_borderSize-m_borderSize);
        bl=background.copy(0,background.height()-m_borderSize,m_borderSize,m_borderSize);
        bm=background.copy(m_borderSize,background.height()-m_borderSize,background.width()-m_borderSize-m_borderSize,m_borderSize);
        br=background.copy(background.width()-m_borderSize,background.height()-m_borderSize,m_borderSize,m_borderSize);

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

unsigned int ImagesStrechMiddle::currentBorderSize() const
{
    return imageBorderSize()/2;
}

void ImagesStrechMiddle::setWidth(const int &w)
{
    this->m_w=w;
}

void ImagesStrechMiddle::setHeight(const int &h)
{
    this->m_h=h;
}

void ImagesStrechMiddle::setSize(const int &w,const int &h)
{
    this->m_w=w;
    this->m_h=h;
}

int ImagesStrechMiddle::width() const
{
    return m_w;
}

int ImagesStrechMiddle::height() const
{
    return m_h;
}
