#include "ProgressBarPixel.hpp"
#include <QPainter>

ProgressBarPixel::ProgressBarPixel(QGraphicsItem *parent,const bool onlygreen) :
    QGraphicsItem(parent),
    onlygreen(onlygreen)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPixelSize(16);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);
    m_boundingRect=QRectF(0.0,0.0,50.0,8.0);
    cache=nullptr;

    m_value=0;
    m_min=0;
    m_max=0;
}

ProgressBarPixel::~ProgressBarPixel()
{
    if(textPath!=nullptr)
    {
        delete textPath;
        textPath=nullptr;
    }
    if(font!=nullptr)
    {
        delete font;
        font=nullptr;
    }
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

QRectF ProgressBarPixel::boundingRect() const
{
    return m_boundingRect;
}

void ProgressBarPixel::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(cache!=nullptr)
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
    image.fill(Qt::transparent);
    QPainter paint;
    if(image.isNull())
        abort();
    paint.begin(&image);

    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
    if(backgroundLeft.isNull() || backgroundLeft.height()!=m_boundingRect.height())
    {
        QPixmap background(":/CC/images/interface/mbb.png");
        if(background.isNull())
            abort();
        QPixmap bar;
        int val=m_value;
        int max=m_max;
        if(val>(max/2) || onlygreen)
            bar=QPixmap(":/CC/images/interface/mbgreen.png");
        else if(value()>(maximum()/4))
            bar=QPixmap(":/CC/images/interface/mborange.png");
        else
            bar=QPixmap(":/CC/images/interface/mbred.png");
        if(bar.isNull())
            abort();
        if(m_boundingRect.height()==background.height())
        {
            backgroundLeft=background.copy(0,0,3,8);
            backgroundMiddle=background.copy(3,0,44,8);
            backgroundRight=background.copy(47,0,3,8);
            barLeft=bar.copy(0,0,2,8);
            barMiddle=bar.copy(2,0,46,8);
            barRight=bar.copy(48,0,2,8);
        }
        else
        {
            backgroundLeft=background.copy(0,0,3,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
            backgroundMiddle=background.copy(3,0,44,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
            backgroundRight=background.copy(47,0,3,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
            barLeft=bar.copy(0,0,2,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
            barMiddle=bar.copy(2,0,46,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
            barRight=bar.copy(48,0,2,8).scaledToHeight(m_boundingRect.height(),Qt::FastTransformation);
        }
    }
    paint.drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    paint.drawPixmap(backgroundLeft.width(),        0,
                     m_boundingRect.width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    paint.drawPixmap(m_boundingRect.width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);

    int startX=0;//18*m_boundingRect.height()/8;
    int size=m_boundingRect.width()-startX-startX;
    int inpixel=value()*size/maximum();
    if(inpixel<(barLeft.width()+barRight.width()))
    {
        if(inpixel>0)
        {
            const int tempW=inpixel/2;
            if(tempW>0)
            {
                QPixmap barLeftC=barLeft.copy(0,0,tempW,barLeft.height());
                paint.drawPixmap(startX,0,barLeftC.width(),           barLeftC.height(),           barLeftC);
                const unsigned int pixelremaining=inpixel-barLeftC.width();
                if(pixelremaining>0)
                {

                    QPixmap barRightC=barRight.copy(barRight.width()-pixelremaining,0,pixelremaining,barRight.height());
                    paint.drawPixmap(startX+barLeftC.width(),               0,
                                 barRightC.width(),                  barRightC.height(),barRightC);
                }
            }
        }
    }
    else {
        paint.drawPixmap(startX,0,barLeft.width(),           barLeft.height(),           barLeft);
        const unsigned int pixelremaining=inpixel-(barLeft.width()+barRight.width());
        if(pixelremaining>0)
            paint.drawPixmap(startX+barLeft.width(),          0,                          pixelremaining,           barMiddle.height(),barMiddle);
        paint.drawPixmap(startX+barLeft.width()+pixelremaining,  0,barRight.width(),                  barRight.height(),barRight);
    }

    *cache=QPixmap::fromImage(image);

    {
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
        return;
    }
}

void ProgressBarPixel::setMaximum(const int &value)
{
    if(this->m_max==value)
        return;
    this->m_max=value;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

void ProgressBarPixel::setMinimum(const int &value)
{
    if(this->m_min==value)
        return;
    this->m_min=value;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

void ProgressBarPixel::setValue(const int &value)
{
    if(this->m_value==value)
        return;
    this->m_value=value;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

int ProgressBarPixel::maximum()
{
    return this->m_max;
}

int ProgressBarPixel::minimum()
{
    return this->m_min;
}

int ProgressBarPixel::value()
{
    return this->m_value;
}

void ProgressBarPixel::setPos(qreal ax, qreal ay)
{
    if(m_boundingRect.x()==ax && m_boundingRect.y()==ay)
        return;
    qreal width=m_boundingRect.width();
    qreal height=m_boundingRect.height();
    m_boundingRect.setX(ax);
    m_boundingRect.setY(ay);
    m_boundingRect.setWidth(width);
    m_boundingRect.setHeight(height);

    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

void ProgressBarPixel::setSize(qreal width,qreal height)
{
    if(m_boundingRect.width()==width && m_boundingRect.height()==height)
        return;
    m_boundingRect.setWidth(width);
    m_boundingRect.setHeight(height);
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}
