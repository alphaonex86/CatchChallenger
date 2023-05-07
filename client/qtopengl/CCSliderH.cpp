#include "CCSliderH.hpp"
#include <QPainter>

CCSliderH::CCSliderH(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    m_width=10;
    m_maximum=100;
    m_value=0;
    m_value_temp=0;
    m_pressed=false;
}

QRectF CCSliderH::boundingRect() const
{
    return QRectF(0,0,width(),26);
}

void CCSliderH::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    if(l.isNull())
    {
        const QPixmap &background=QPixmap(":/CC/images/interface/sliderBarH.png");
        l=background.copy(0,0,5,26);
        m=background.copy(5,0,33,26);
        r=background.copy(38,0,5,26);
    }

    painter->drawPixmap(0,              0,5,            26,l);
    if(width()>10)
        painter->drawPixmap(5,              0,width()-5*2,  26,m);
    painter->drawPixmap(width()-5,      0,5,            26,r);

    const QPixmap &silderH=QPixmap(":/CC/images/interface/sliderH.png");
    painter->drawPixmap((width()-silderH.width())*m_value_temp/m_maximum,0,silderH.width(),silderH.height(),silderH);
}

void CCSliderH::setWidth(const int &w)
{
    if(w>=10)
        m_width=w;
}

int CCSliderH::width() const
{
    return m_width;
}

void CCSliderH::setMaximum(const unsigned int &maximum)
{
    m_maximum=maximum;
    if(m_value>m_maximum)
    {
        m_value=m_maximum;
        m_value_temp=m_maximum;
    }
}

unsigned int CCSliderH::maximum()
{
    return m_maximum;
}

void CCSliderH::setValue(const unsigned int &value)
{
    if(value<=m_maximum)
    {
        m_value=value;
        m_value_temp=value;
    }
}

unsigned int CCSliderH::value()
{
    return m_value;
}

void CCSliderH::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    if(this->m_pressed)
        return;
    if(!isVisible())
        return;
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(t.contains(p))
    {
        pressValidated=true;
        m_pressed=true;

        const qreal px=p.x();
        const qreal tx=t.x();
        const int twidth=width();
        if(px<=tx)
            m_value_temp=0;
        else if(px>=(tx+twidth))
            m_value_temp=m_maximum;
        else
        {
            m_value_temp=(px-tx)*m_maximum/twidth;
        }
        emit sliderPressed();
    }
}

void CCSliderH::mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated)
{
    if(!this->m_pressed)
        return;
    if(!previousPressValidated && isVisible())
        previousPressValidated=true;
    m_pressed=false;
    m_value=m_value_temp;
    emit sliderReleased();
    emit valueChanged(m_value_temp);
}

void CCSliderH::mouseMoveEventXY(const QPointF &p, bool &previousPressValidated)
{
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(!this->m_pressed)
        return;
    if(!previousPressValidated && isVisible())
        previousPressValidated=true;
    const qreal px=p.x();
    const qreal tx=t.x();
    const int twidth=width();
    if(px<=tx)
        m_value_temp=0;
    else if(px>=(tx+twidth))
        m_value_temp=m_maximum;
    else
    {
        m_value_temp=(px-tx)*m_maximum/twidth;
    }

    emit sliderMoved(m_value_temp);
}

