#include "CustomButton.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>

CustomButton::CustomButton(QString pix,QWidget *parent) :
    QPushButton(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(35);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);

    pressed=false;
    background=pix;
    outlineColor=QColor(217,145,0);
    percent=85;
    cache=nullptr;
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

void CustomButton::paintEvent(QPaintEvent *)
{
    if(cache!=nullptr && !cache->isNull() && cache->width()==width() && cache->height()==height())
    {
        QPainter paint;
        paint.begin(this);
        paint.drawPixmap(0,0,width(),height(),*cache);
        return;
    }
    if(cache!=nullptr)
        delete cache;
    cache=new QPixmap();
    QImage image(width(),height(),QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter paint;
    paint.begin(&image);
    QPixmap scaledBackground;
    QPixmap temp(background);
    if(temp.isNull())
        abort();
    if(pressed)
        scaledBackground=temp.copy(0,temp.height()/2,temp.width(),temp.height()/2);
    else
        scaledBackground=temp.copy(0,0,temp.width(),temp.height()/2);
    scaledBackground=scaledBackground.scaled(width(),height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    paint.drawPixmap(0,0,width(),height(),scaledBackground);
    updateTextPath();

    if(textPath!=nullptr)
    {
        paint.setRenderHint(QPainter::Antialiasing);
        qreal penWidth=2.0;
        if(font->pointSize()<=12)
            penWidth=0.7;
        else if(font->pointSize()<=18)
            penWidth=1;
        else if(font->pointSize()<=24)
            penWidth=1.5;
        paint.setPen(QPen(outlineColor/*penColor*/, penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint.setBrush(Qt::white);
        paint.drawPath(*textPath);
    }
    *cache=QPixmap::fromImage(image);

    {
        QPainter paint;
        paint.begin(this);
        paint.drawPixmap(0,0,width(),height(),*cache);
    }
}

void CustomButton::setText(const QString &text)
{
    QPushButton::setText(text);
    updateTextPath();
}

bool CustomButton::setPointSize(uint8_t size)
{
    font->setPointSize(size);
    return true;
}

void CustomButton::updateTextPath()
{
    const QString &text=QPushButton::text();
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    const int h=height();
    int newHeight=0;
    if(pressed)
        newHeight=(h*(percent+20)/100);
    else
        newHeight=(h*percent/100);
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath->addText(width()/2-rect.width()/2, tempHeight, *font, text);
}

bool CustomButton::updateTextPercent(uint8_t percent)
{
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
}

QFont CustomButton::getFont() const
{
    return *font;
}

void CustomButton::setOutlineColor(const QColor &color)
{
    this->outlineColor=color;
}

void CustomButton::mousePressEvent(QMouseEvent *e)
{
    pressed=true;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
    QPushButton::mousePressEvent(e);
}

void CustomButton::mouseReleaseEvent(QMouseEvent *e)
{
    pressed=false;
    if(cache!=nullptr)
    {
        delete cache;
        cache=nullptr;
    }
    QPushButton::mouseReleaseEvent(e);
}
