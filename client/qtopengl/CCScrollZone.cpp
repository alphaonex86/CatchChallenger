#include "CCScrollZone.hpp"
#include <QPainter>

CCScrollZone::CCScrollZone(QGraphicsItem *parent) :
    QGraphicsObject(parent)
{
    cache=nullptr;
    lastwidth=0;
    lastheight=0;
}

CCScrollZone::~CCScrollZone()
{
    if(cache!=nullptr)
        delete cache;
    cache=nullptr;
}

void CCScrollZone::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(cache!=nullptr && !cache->isNull() && cache->width()==m_boundingRect.width() && cache->height()==m_boundingRect.height())
    {
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
        return;
    }
    if(m_boundingRect.isEmpty())
        return;
    if(cache!=nullptr)
        delete cache;
    cache=new QPixmap();

    QImage image(m_boundingRect.width(),m_boundingRect.height(),QImage::Format_ARGB32);
    //image.fill(Qt::transparent);
    if(isSelected())
        image.fill(QColor(100,150,135,150));
    else
        image.fill(QColor(100,150,100,50));
    QPainter paint;
    if(image.isNull())
        abort();
    paint.begin(&image);
    if(lastwidth!=m_boundingRect.width() || lastheight!=m_boundingRect.height())
    {
        lastwidth=m_boundingRect.width();
        lastheight=m_boundingRect.height();
    }
    if(!paint.isActive())
        return;

    //pain the child here

    *cache=QPixmap::fromImage(image);

    painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
}

QRectF CCScrollZone::boundingRect() const
{
    return m_boundingRect;
}

void CCScrollZone::setWidth(const int &w)
{
    this->m_boundingRect.setWidth(w);
}

void CCScrollZone::setHeight(const int &h)
{
    this->m_boundingRect.setHeight(h);
}

void CCScrollZone::setSize(const int &w,const int &h)
{
    this->m_boundingRect.setSize(QSizeF(w,h));
}

int CCScrollZone::width() const
{
    return this->m_boundingRect.width();
}

int CCScrollZone::height() const
{
    return this->m_boundingRect.height();
}
