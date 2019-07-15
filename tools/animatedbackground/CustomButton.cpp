#include "CustomButton.h"
#include <QPainter>
#include <iostream>

CustomButton::CustomButton(QPixmap pix,QWidget *parent) :
    QPushButton(parent)
{
    setMinimumWidth(223);
    setMinimumHeight(93);
    setMaximumWidth(223);
    setMaximumHeight(93);
    over=false;
    background=pix;

    /*QFont font("Comic Sans MS");
    font.setStyleHint(QFont::Monospace);
    font.setBold(true);
    font.setPixelSize(30);
    p->setFont( font );*/
}

void CustomButton::paintEvent(QPaintEvent *)
{
    QPainter paint;
    paint.begin(this);
    paint.drawPixmap(0,0,background.width(),    background.height(),    background);

    paint.setRenderHint(QPainter::Antialiasing);

    paint.setPen(QPen(QColor(217,145,0)/*penColor*/, 2/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    paint.setBrush(Qt::white);
    paint.drawPath(textPath);
}

void CustomButton::setText(const QString &text)
{
    QFont timesFont("Comic Sans MS",35);
    timesFont.setStyleHint(QFont::Monospace);
    timesFont.setBold(true);
    timesFont.setStyleStrategy(QFont::ForceOutline);
    textPath=QPainterPath();
    textPath.addText(0, 56, timesFont, text);
    QRectF rect=textPath.boundingRect();
    textPath=QPainterPath();
    textPath.addText(width()/2-rect.width()/2, 56, timesFont, text);
    QPushButton::setText(text);
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
