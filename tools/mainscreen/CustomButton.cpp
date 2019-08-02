#include "CustomButton.h"
#include <QPainter>
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
}

void CustomButton::paintEvent(QPaintEvent *)
{
    QPainter paint;
    paint.begin(this);
    if(pressed) {
        if(scaledBackgroundPressed.width()!=width() || scaledBackgroundPressed.height()!=height())
        {
            QPixmap temp(background);
            if(temp.isNull())
                abort();
            scaledBackgroundPressed=temp.copy(0,temp.height()/2,temp.width(),temp.height()/2);
            scaledBackgroundPressed=scaledBackgroundPressed.scaled(width(),height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        paint.drawPixmap(0,0,width(),height(),scaledBackgroundPressed);
        updateTextPath();
    } else {
        if(scaledBackground.width()!=width() || scaledBackground.height()!=height())
        {
            QPixmap temp(background);
            if(temp.isNull())
                abort();
            scaledBackground=temp.copy(0,0,temp.width(),temp.height()/2);
            scaledBackground=scaledBackground.scaled(width(),height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        paint.drawPixmap(0,0,width(),height(),scaledBackground);
        updateTextPath();
    }

    if(textPath!=nullptr)
    {
        paint.setRenderHint(QPainter::Antialiasing);
        paint.setPen(QPen(outlineColor/*penColor*/, 2/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint.setBrush(Qt::white);
        paint.drawPath(*textPath);
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
    QPushButton::mousePressEvent(e);
    updateTextPath();
}

void CustomButton::mouseReleaseEvent(QMouseEvent *e)
{
    pressed=false;
    QPushButton::mouseReleaseEvent(e);
    updateTextPath();
}
