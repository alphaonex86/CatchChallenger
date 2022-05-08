#include "CCprogressbar.h"
#include <QPainter>
#include <QPainterPath>

CCprogressbar::CCprogressbar(QWidget *parent) :
    QProgressBar(parent)
{
    textPath=nullptr;

    font=new QFont();
    font->setFamily("Comic Sans MS");
    font->setPointSize(16);
    font->setStyleHint(QFont::Monospace);
    font->setBold(true);
    font->setStyleStrategy(QFont::ForceOutline);
    setMaximumHeight(82);
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

void CCprogressbar::paintEvent(QPaintEvent *)
{
    if(backgroundLeft.isNull() || backgroundLeft.height()!=height())
    {
        QPixmap background(":/Pbarbackground.png");
        if(background.isNull())
            abort();
        QPixmap bar(":/Pbarforeground.png");
        if(bar.isNull())
            abort();
        if(height()==background.height())
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
            backgroundLeft=background.copy(0,0,41,82).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundMiddle=background.copy(41,0,824,82).scaledToHeight(height(),Qt::SmoothTransformation);
            backgroundRight=background.copy(865,0,41,82).scaledToHeight(height(),Qt::SmoothTransformation);
            barLeft=bar.copy(0,0,26,82).scaledToHeight(height(),Qt::SmoothTransformation);
            barMiddle=bar.copy(26,0,620,82).scaledToHeight(height(),Qt::SmoothTransformation);
            barRight=bar.copy(646,0,26,82).scaledToHeight(height(),Qt::SmoothTransformation);
        }
    }
    QPainter paint;
    paint.begin(this);
    paint.drawPixmap(0,0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    paint.drawPixmap(backgroundLeft.width(),        0,
                     width()-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    paint.drawPixmap(width()-backgroundRight.width(),0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);

    int startX=18*height()/82;
    int size=width()-startX-startX;
    int inpixel=value()*size/maximum();
    if(inpixel<(barLeft.width()+barRight.width()))
    {
        if(inpixel>0)
        {
            QPixmap barLeftC=barLeft.copy(0,0,inpixel/2,barLeft.height());
            paint.drawPixmap(startX,0,barLeftC.width(),           barLeftC.height(),           barLeftC);
            const unsigned int pixelremaining=inpixel-barLeftC.width();
            if(pixelremaining>0)
            {

                QPixmap barRightC=barRight.copy(barRight.width()-pixelremaining,0,pixelremaining,barRight.height());
                paint.drawPixmap(startX+barLeftC.width(),               0,
                             barRightC.width(),                  barRightC.height(),barRightC);
            }
        }
    }
    else {
        paint.drawPixmap(startX,0,barLeft.width(),           barLeft.height(),           barLeft);
        const unsigned int pixelremaining=inpixel-(barLeft.width()+barRight.width());
        if(pixelremaining>0)
            paint.drawPixmap(startX+barLeft.width(),          0,                          pixelremaining,           barMiddle.height(),barMiddle);
        paint.drawPixmap(startX+barLeft.width()+pixelremaining,  0,barRight.width(),                  barRight.height(),barRight);
    }

    if(isTextVisible())
    {
        const unsigned int fontheight=height()/4;
        if(fontheight>=11)
        {
            QString text=format();
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
                const int h=height();
                const int newHeight=h*95/100;
                const int p=font->pointSize();
                const int tempHeight=newHeight/2+p/2;
                textPath->addText(width()/2-rect.width()/2, tempHeight, *font, text);

                oldText=text;
            }

            paint.setRenderHint(QPainter::Antialiasing);
            int penWidth=1;
            if(fontheight>16)
                penWidth=2;
            paint.setPen(QPen(QColor(14,102,0)/*penColor*/, penWidth/*penWidth*/, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            paint.setBrush(Qt::white);
            paint.drawPath(*textPath);
        }
    }
}

void CCprogressbar::resizeEvent(QResizeEvent *)
{
    oldText.clear();
}
