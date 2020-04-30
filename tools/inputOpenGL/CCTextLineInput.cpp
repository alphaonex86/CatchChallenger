#include "CCTextLineInput.h"
#include <QPainter>
#include <QWidget>

CCTextLineInput::CCTextLineInput(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    m_textCursor=0;
}

QRectF CCTextLineInput::boundingRect() const
{
    return QRectF();
}

CCTextLineInput::~CCTextLineInput()
{
}

void CCTextLineInput::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPixmap background(":/inputText.png");
    if(background.isNull())
        abort();
    QPixmap bar(":/inputText.png");
    if(bar.isNull())
        abort();
    if(widget->height()==background.height())
    {
        backgroundLeft=background.copy(0,0,41,82);
        backgroundMiddle=background.copy(41,0,824,82);
        backgroundRight=background.copy(865,0,41,82);
    }
    else
    {
        backgroundLeft=background.copy(0,0,41,82).scaledToHeight(widget->height(),Qt::SmoothTransformation);
        backgroundMiddle=background.copy(41,0,824,82).scaledToHeight(widget->height(),Qt::SmoothTransformation);
        backgroundRight=background.copy(865,0,41,82).scaledToHeight(widget->height(),Qt::SmoothTransformation);
    }
    painter->drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    painter->drawPixmap(backgroundLeft.width(),        0,
                     widget->width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    painter->drawPixmap(widget->width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);

    /*QString text=format();
    text.replace("%p",QString::number(value()*100/maximum()));

    font->setPointSize(fontheight);
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    const int h=height();
    const int newHeight=h*95/100;
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    textPath->addText(width()/2-rect.width()/2, tempHeight, *font, text);

    paint.setRenderHint(QPainter::Antialiasing);
    int penWidth=1;
    if(fontheight>16)
        penWidth=2;
    paint.setPen(QPen(QColor(14,102,0), penWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    paint.setBrush(Qt::white);
    paint.drawPath(*textPath);*/
}
