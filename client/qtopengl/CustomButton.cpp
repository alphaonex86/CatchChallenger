#include "CustomButton.h"
#include "../qt/GameLoader.h"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>

CustomButton::CustomButton(QString pix,QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPixelSize(35);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);

    pressed=false;
    background=pix;
    outlineColor=QColor(217,145,0);
    percent=75;
    cache=nullptr;

    m_boundingRect=QRectF(0.0,0.0,223.0,92.0);
}

void CustomButton::setSize(uint16_t w,uint16_t h)
{
    if(m_boundingRect.width()==w && m_boundingRect.height()==h)
        return;
    m_boundingRect=QRectF(m_boundingRect.x(),m_boundingRect.y(),w,h);
    if(cache!=nullptr)
        delete cache;
    cache=nullptr;
    updateTextPath();
}

void CustomButton::setPos(qreal ax, qreal ay)
{
    /*QGraphicsItem::setPos(ax,ay);*/
    if(m_boundingRect.x()==ax && m_boundingRect.y()==ay)
        return;
    m_boundingRect=QRectF(ax,ay,m_boundingRect.width(),m_boundingRect.height());
}

int CustomButton::width()
{
    return m_boundingRect.width();
}

int CustomButton::height()
{
    return m_boundingRect.height();
}

CustomButton::~CustomButton()
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

void CustomButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
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
    QPixmap scaledBackground;
    QPixmap temp(*GameLoader::gameLoader->getImage(background));
    if(temp.isNull())
        abort();
    if(pressed)
        scaledBackground=temp.copy(0,temp.height()/2,temp.width(),temp.height()/2);
    else
        scaledBackground=temp.copy(0,0,temp.width(),temp.height()/2);
    scaledBackground=scaledBackground.scaled(m_boundingRect.width(),m_boundingRect.height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    paint.drawPixmap(0,0,m_boundingRect.width(),m_boundingRect.height(),scaledBackground);
    updateTextPath();

    if(textPath!=nullptr && paint.isActive())
    {
        paint.setRenderHint(QPainter::Antialiasing);
        qreal penWidth=2.0;
        paint.setPen(QPen(outlineColor/*penColor*/, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint.setBrush(Qt::white);
        paint.drawPath(*textPath);
    }
    *cache=QPixmap::fromImage(image);

    {
        painter->drawPixmap(m_boundingRect.x(),m_boundingRect.y(),m_boundingRect.width(),m_boundingRect.height(),*cache);
        return;
    }
}

void CustomButton::setText(const QString &text)
{
    if(this->m_text==text)
        return;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
    this->m_text=text;
    updateTextPath();
}

bool CustomButton::setPointSize(uint8_t size)
{
    int tempsize=size*1.5;
    if(font->pixelSize()==tempsize)
        return true;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
    font->setPixelSize(tempsize);
    return true;
}

void CustomButton::updateTextPath()
{
    const QString &text=this->m_text;
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    const int h=m_boundingRect.height();
    int newHeight=0;
    if(pressed)
        newHeight=(h*(percent+20)/100);
    else
        newHeight=(h*percent/100);
    const int p=font->pixelSize();
    const int tempHeight=newHeight/2+p/2;
    textPath->addText(m_boundingRect.width()/2-rect.width()/2, tempHeight, *font, text);
}

bool CustomButton::updateTextPercent(uint8_t percent)
{
    if(this->percent==percent)
        return true;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
    if(percent>100)
        return false;
    this->percent=percent;
    updateTextPath();
    update();
    return true;
}

void CustomButton::setFont(const QFont &font)
{
    *this->font=font;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

QFont CustomButton::getFont() const
{
    return *font;
}

void CustomButton::setOutlineColor(const QColor &color)
{
    this->outlineColor=color;
}

void CustomButton::setPressed(const bool &pressed)
{
    this->pressed=pressed;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
}

QRectF CustomButton::boundingRect() const
{
    return m_boundingRect;
}

void CustomButton::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    if(this->pressed)
        return;
    if(!isVisible())
        return;
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(t.contains(p))
    {
        pressValidated=true;
        setPressed(true);
    }
}

void CustomButton::mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated)
{
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(!this->pressed)
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
