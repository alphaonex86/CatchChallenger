#include "CustomButton.h"
#include <QFont>
#include <QPainter>
#include <QWidget>

CustomButton::CustomButton(QString pix, QGraphicsItem *item) :
    QGraphicsItem(item)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(35);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);

    pressed=false;
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

void CustomButton::paint(QPainter * paint, const QStyleOptionGraphicsItem *, QWidget * widget)
{
    if(pressed) {
            if(scaledBackgroundPressed.width()!=widget->width() || scaledBackgroundPressed.height()!=widget->height())
            {
                QPixmap temp(background);
                scaledBackgroundPressed=temp.copy(0,temp.height()/3,temp.width(),temp.height()/3);
                scaledBackgroundPressed=scaledBackgroundPressed.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
            }
            paint->drawPixmap(0,0,widget->width(),widget->height(),scaledBackgroundPressed);
    } else if(over) {
        if(scaledBackgroundOver.width()!=widget->width() || scaledBackgroundOver.height()!=widget->height())
        {
            QPixmap temp(background);
            scaledBackgroundOver=temp.copy(0,temp.height()/3*2,temp.width(),temp.height()/3);
            scaledBackgroundOver=scaledBackgroundOver.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        paint->drawPixmap(0,0,widget->width(),widget->height(),scaledBackgroundOver);
    } else {
        if(scaledBackground.width()!=widget->width() || scaledBackground.height()!=widget->height())
        {
            QPixmap temp(background);
            scaledBackground=temp.copy(0,0,temp.width(),temp.height()/3);
            scaledBackground=scaledBackground.scaled(widget->width(),widget->height(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        }
        paint->drawPixmap(0,0,widget->width(),widget->height(),scaledBackground);
    }

    if(textPath!=nullptr)
    {
        paint->setRenderHint(QPainter::Antialiasing);
        paint->setPen(QPen(QColor(217,145,0), 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        paint->setBrush(Qt::white);
        paint->drawPath(*textPath);
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
    //const int h=height();
    //const int w=width();
    const int h=222;
    const int w=93;
    const int newHeight=(h*80/100);
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath->addText(w/2-rect.width()/2, tempHeight, *font, text);
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

void CustomButton::mousePressEvent(QMouseEvent *e)
{
    pressed=true;
    //QGraphicsItem::mousePressEvent(e);
}

void CustomButton::mouseReleaseEvent(QMouseEvent *e)
{
    pressed=false;
    //QGraphicsItem::mouseReleaseEvent(e);
}

QRectF CustomButton::boundingRect() const
{
    return QRectF();
}
