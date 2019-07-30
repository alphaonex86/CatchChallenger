#include "CCWidget.h"
#include <QPainter>

CCWidget::CCWidget(QWidget *parent) :
    QProgressBar(parent)
{
    oldratio=0;
}

void CCWidget::paintEvent(QPaintEvent *)
{
    const int w=width();
    int tw=46;
    if(w<50)
        tw=46/4;
    else if(w>250)
        tw=46;
    else
        tw=(w-50)*46*3/4/200+46/4;
    int th=46;
    const int h=height();
    if(h<50)
        th=46/4;
    else if(h>250)
        th=46;
    else
        /* (h-50) -> 0 to 200
         * (h-50)*46*3/4/200 -> 0 to 46*3/4
         * (h-50)*46*3/4/200+46/4 -> 46/4 to 46 */
        th=(h-50)*46*3/4/200+46/4;
    int min=tw;
    if(min>th)
        min=th;

    if(tl.isNull() || min!=tl.height())
    {
        QPixmap background(":/images/interface/message.png");
        if(background.isNull())
            abort();
        tl=background.copy(0,0,46,46);
        tm=background.copy(46,0,401,46);
        tr=background.copy(447,0,46,46);
        ml=background.copy(0,46,46,179);
        mm=background.copy(46,46,401,179);
        mr=background.copy(447,46,46,179);
        bl=background.copy(0,225,46,46);
        bm=background.copy(46,225,401,46);
        br=background.copy(447,225,46,46);

        if(min!=tl.height())
        {
            tl=tl.scaledToHeight(min,Qt::SmoothTransformation);
            tm=tm.scaledToHeight(min,Qt::SmoothTransformation);
            tr=tr.scaledToHeight(min,Qt::SmoothTransformation);
            ml=ml.scaledToWidth(min,Qt::SmoothTransformation);
            //mm=mm.scaledToHeight(minsize,Qt::SmoothTransformation);
            mr=mr.scaledToWidth(min,Qt::SmoothTransformation);
            bl=bl.scaledToHeight(min,Qt::SmoothTransformation);
            bm=bm.scaledToHeight(min,Qt::SmoothTransformation);
            br=br.scaledToHeight(min,Qt::SmoothTransformation);
        }
    }
    QPainter paint;
    paint.begin(this);
    paint.drawPixmap(0,             0,min,          min,tl);
    paint.drawPixmap(min,           0,width()-min*2,min,tm);
    paint.drawPixmap(width()-min,   0,min,          min,tr);

    paint.drawPixmap(0,             min,min,          height()-min*2,ml);
    paint.drawPixmap(min,           min,width()-min*2,height()-min*2,mm);
    paint.drawPixmap(width()-min,   min,min,          height()-min*2,mr);

    paint.drawPixmap(0,             height()-min,min,           min,bl);
    paint.drawPixmap(min,           height()-min,width()-min*2, min,bm);
    paint.drawPixmap(width()-min,   height()-min,min,           min,br);
}
