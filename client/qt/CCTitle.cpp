#include "CCTitle.h"
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <iostream>

CCTitle::CCTitle(QWidget *parent) :
    QLabel(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(25);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);

    outlineColor=QColor(77,64,44);
    lastwidth=0;
    lastheight=0;
    cache=nullptr;
}

CCTitle::~CCTitle()
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

void CCTitle::paintEvent(QPaintEvent *)
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
    if(lastwidth!=width() || lastheight!=height())
    {
        updateTextPath();
        lastwidth=width();
        lastheight=height();
    }

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

void CCTitle::setText(const QString &text)
{
    QLabel::setText(text);
    updateTextPath();
}

bool CCTitle::setPointSize(uint8_t size)
{
    font->setPointSize(size);
    return true;
}

void CCTitle::updateTextPath()
{
    const QString &text=QLabel::text();
    if(textPath!=nullptr)
        delete textPath;
    QPainterPath tempPath;
    tempPath.addText(0, 0, *font, text);
    QRectF rect=tempPath.boundingRect();
    textPath=new QPainterPath();
    int newHeight=height();
    const int p=font->pointSize();
    const int tempHeight=newHeight/2+p/2;
    const Qt::Alignment a=alignment();
    if(a.testFlag(Qt::AlignLeft))
        textPath->addText(0, tempHeight-1, *font, text);
    else if(a.testFlag(Qt::AlignRight))
        textPath->addText(width()-rect.width(), tempHeight-1, *font, text);
    else
        textPath->addText(width()/2-rect.width()/2, tempHeight-1, *font, text);
}

void CCTitle::setFont(const QFont &font)
{
    *this->font=font;
}

QFont CCTitle::getFont() const
{
    return *font;
}

void CCTitle::setOutlineColor(const QColor &color)
{
    this->outlineColor=color;
}
