#include "CustomButton.h"
#include "CCBackground.h"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>

CustomButton::CustomButton(QString pix)
{
    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(35);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);
    setMaximumSize(QSize(223,92));

    pressed=false;
    over=false;
    background=pix;
}

CustomButton::~CustomButton()
{
    if(font!=nullptr)
    {
        delete font;
        font=nullptr;
    }
}

void CustomButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    auto end = std::chrono::steady_clock::now();
    unsigned int time=std::chrono::duration_cast<std::chrono::milliseconds>(end - CCBackground::timeFromStart).count();
    setText(QString::number(CCBackground::timeToDraw)+"-"+QString::number(CCBackground::timeDraw*1000/time));
    if(pressed) {
            if(scaledBackgroundPressed.width()!=widget->width() || scaledBackgroundPressed.height()!=widget->height())
            {
                QPixmap temp(background);
                scaledBackgroundPressed=temp.copy(0,temp.height()/3,temp.width(),temp.height()/3);
                scaledBackgroundPressed=scaledBackgroundPressed.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            }
            painter->drawPixmap(0,0,widget->width(),widget->height(),scaledBackgroundPressed);
    } else if(over) {
        if(scaledBackgroundOver.width()!=widget->width() || scaledBackgroundOver.height()!=widget->height())
        {
            QPixmap temp(background);
            scaledBackgroundOver=temp.copy(0,temp.height()/3*2,temp.width(),temp.height()/3);
            scaledBackgroundOver=scaledBackgroundOver.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        painter->drawPixmap(0,0,widget->width(),widget->height(),scaledBackgroundOver);
    } else {
        if(scaledBackground.width()!=widget->width() || scaledBackground.height()!=widget->height())
        {
            QPixmap temp(background);
            scaledBackground=temp.copy(0,0,temp.width(),temp.height()/3);
            scaledBackground=scaledBackground.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        painter->drawPixmap(0,0,widget->width(),widget->height(),scaledBackground);
    }

    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    QPainterPath textPath;
    const int h=widget->height();
    const int newHeight=(h*80/100);
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath.addText(widget->width()/2-rect.width()/2, tempHeight, *font, text);

    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(QPen(QColor(217,145,0)/*penColor*/, 2/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter->setBrush(Qt::white);
    painter->drawPath(textPath);
}

void CustomButton::setText(const QString &text)
{
    this->text=text;
}

void CustomButton::setFont(const QFont &font)
{
    *this->font=font;
}

/*void CustomButton::enterEvent(QEvent *e)
{
    over=true;
    QPushButton::enterEvent(e);
}
void CustomButton::leaveEvent(QEvent *e)
{
    over=false;
    pressed=false;
    QPushButton::leaveEvent(e);
}*/

void CustomButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    pressed=true;
    QGraphicsWidget::mousePressEvent(event);
}

void CustomButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    pressed=false;
    QGraphicsWidget::mouseReleaseEvent(event);
    emit clicked();
}
