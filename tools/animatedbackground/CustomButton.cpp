#include "CustomButton.h"
#include <QPainter>
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

    over=false;
    background=pix;
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
    if(scaledBackground.width()!=width() || scaledBackground.height()!=height())
    {
        scaledBackground=background;
        scaledBackground=scaledBackground.scaled(width(),height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    }
    paint.drawPixmap(0,0,width(),height(),scaledBackground);

    if(textPath!=nullptr)
    {
        paint.setRenderHint(QPainter::Antialiasing);

        paint.setPen(QPen(QColor(217,145,0)/*penColor*/, 2/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint.setBrush(Qt::white);
        paint.drawPath(*textPath);
    }
}

void CustomButton::setText(const QString &text)
{
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    const int h=height();
    const int newHeight=(h*80/100);
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath->addText(width()/2-rect.width()/2, tempHeight, *font, text);
    QPushButton::setText(text);
}

void CustomButton::setFont(const QFont &font)
{
    *this->font=font;
}

void CustomButton::enterEvent(QEvent *e)
{
    over=true;
    QWidget::enterEvent(e);
    update();
}
void CustomButton::leaveEvent(QEvent *e)
{
    over=false;
    QWidget::leaveEvent(e);
    update();
}
