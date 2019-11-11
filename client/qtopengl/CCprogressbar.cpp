#include "CCprogressbar.h"
#include <QPainter>

CCprogressbar::CCprogressbar(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(16);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);
    m_boundingRect=QRectF(0.0,0.0,200.0,82.0);

    m_value=0;
    m_min=0;
    m_max=0;
}

CCprogressbar::~CCprogressbar()
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

QRectF CCprogressbar::boundingRect() const
{
    return m_boundingRect;
}

void CCprogressbar::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if(backgroundLeft.isNull() || backgroundLeft.height()!=m_boundingRect.height())
    {
        QPixmap background(":/CC/images/interface/Pbarbackground.png");
        if(background.isNull())
            abort();
        QPixmap bar(":/CC/images/interface/Pbarforeground.png");
        if(bar.isNull())
            abort();
        if(m_boundingRect.height()==background.height())
        {
            backgroundLeft=background.copy(0,0,41,82);
            backgroundMiddle=background.copy(41,0,824,82);
            backgroundRight=background.copy(865,0,41,82);
            barLeft=bar.copy(0,0,26,82);
            barMiddle=bar.copy(26,0,620,82);
            barRight=bar.copy(646,0,26,82);
        }
        else
        {
            backgroundLeft=background.copy(0,0,41,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
            backgroundMiddle=background.copy(41,0,824,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
            backgroundRight=background.copy(865,0,41,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
            barLeft=bar.copy(0,0,26,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
            barMiddle=bar.copy(26,0,620,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
            barRight=bar.copy(646,0,26,82).scaledToHeight(m_boundingRect.height(),Qt::SmoothTransformation);
        }
    }
    painter->drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    painter->drawPixmap(backgroundLeft.width(),        0,
                     m_boundingRect.width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    painter->drawPixmap(m_boundingRect.width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);

    int startX=18*m_boundingRect.height()/82;
    int size=m_boundingRect.width()-startX-startX;
    int inpixel=value()*size/maximum();
    if(inpixel<(barLeft.width()+barRight.width()))
    {
        if(inpixel>0)
        {
            QPixmap barLeftC=barLeft.copy(0,0,inpixel/2,barLeft.height());
            painter->drawPixmap(startX,0,barLeftC.width(),           barLeftC.height(),           barLeftC);
            const unsigned int pixelremaining=inpixel-barLeftC.width();
            if(pixelremaining>0)
            {

                QPixmap barRightC=barRight.copy(barRight.width()-pixelremaining,0,pixelremaining,barRight.height());
                painter->drawPixmap(startX+barLeftC.width(),               0,
                             barRightC.width(),                  barRightC.height(),barRightC);
            }
        }
    }
    else {
        painter->drawPixmap(startX,0,barLeft.width(),           barLeft.height(),           barLeft);
        const unsigned int pixelremaining=inpixel-(barLeft.width()+barRight.width());
        if(pixelremaining>0)
            painter->drawPixmap(startX+barLeft.width(),          0,                          pixelremaining,           barMiddle.height(),barMiddle);
        painter->drawPixmap(startX+barLeft.width()+pixelremaining,  0,barRight.width(),                  barRight.height(),barRight);
    }

    if(false)
    {
        const unsigned int fontheight=m_boundingRect.height()/4;
        if(fontheight>=11)
        {
            QString text="%p";
            text.replace("%p",QString::number(value()*100/maximum()));

            if(oldText!=text)
            {
                font->setPointSize(fontheight);
                if(textPath!=nullptr)
                    delete textPath;
                QPainterPath tempPath;
                tempPath.addText(0, 0, *font, text);
                QRectF rect=tempPath.boundingRect();
                textPath=new QPainterPath();
                const int h=m_boundingRect.height();
                const int newHeight=h*95/100;
                const int p=font->pointSize();
                const int tempHeight=newHeight/2+p/2;
                textPath->addText(m_boundingRect.width()/2-rect.width()/2, tempHeight, *font, text);

                oldText=text;
            }

            painter->setRenderHint(QPainter::Antialiasing);
            int penWidth=1;
            if(fontheight>16)
                penWidth=2;
            painter->setPen(QPen(QColor(14,102,0)/*penColor*/, penWidth/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter->setBrush(Qt::white);
            painter->drawPath(*textPath);
        }
    }
}

void CCprogressbar::setMaximum(const int &value)
{
    this->m_max=value;
}

void CCprogressbar::setMinimum(const int &value)
{
    this->m_min=value;
}

void CCprogressbar::setValue(const int &value)
{
    this->m_value=value;
}

int CCprogressbar::maximum()
{
    return this->m_max;
}

int CCprogressbar::minimum()
{
    return this->m_min;
}

int CCprogressbar::value()
{
    return this->m_value;
}


