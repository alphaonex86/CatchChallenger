#include "CCBackground.h"
#include <QTime>
#include <QPainter>
#include <QRect>
#include <QLinearGradient>
#include <QWidget>

CCBackground::CCBackground(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    zoom=0;
    benchmark=true;
    grassMove=0;
    treebackMove=0;
    treefrontMove=0;
    if(!QObject::connect(&grassTimer,&QTimer::timeout,this,&CCBackground::grassSlot))
        abort();
    if(!QObject::connect(&treebackTimer,&QTimer::timeout,this,&CCBackground::treebackSlot))
        abort();
    if(!QObject::connect(&treefrontTimer,&QTimer::timeout,this,&CCBackground::treefrontSlot))
        abort();
    if(!QObject::connect(&updateTimer,&QTimer::timeout,this,&CCBackground::update))
        abort();

    unsigned int baseTime=20;
    grassTimer.start(baseTime);
    treefrontTimer.start(baseTime*3);
    treebackTimer.start(baseTime*9);
    updateTimer.start(40);
}

void CCBackground::scalePix(QPixmap &pix,unsigned int zoom)
{
    pix=pix.scaled(pix.width()*zoom,pix.height()*zoom);
}

void CCBackground::update()
{
    //QGraphicsItem::update();
}

void CCBackground::grassSlot()
{
    if(grass.isNull())
        return;
    grassMove++;
}

void CCBackground::treebackSlot()
{
    if(treeback.isNull())
        return;
    treebackMove++;
}

void CCBackground::treefrontSlot()
{
    if(treefront.isNull())
        return;
    treefrontMove++;
}

unsigned int CCBackground::getTargetZoom(const QWidget * const widget)
{
    unsigned int targetZoomHeight=(unsigned int)widget->height()/120;
    if(targetZoomHeight<1)
        targetZoomHeight=1;
    if(targetZoomHeight>8)
        targetZoomHeight=8;
    unsigned int targetZoomWidth=(unsigned int)widget->width()/120;
    if(targetZoomWidth<1)
        targetZoomWidth=1;
    if(targetZoomWidth>8)
        targetZoomWidth=8;
    unsigned int targetZoom=targetZoomHeight;
    if(targetZoomWidth<targetZoomHeight)
        targetZoom=targetZoomWidth;

    return targetZoom;
}

void CCBackground::paint(QPainter * paint, const QStyleOptionGraphicsItem *, QWidget * widget)
{
    QTime time;
    time.restart();
    unsigned int targetZoom=getTargetZoom(widget);
    if(zoom!=targetZoom)
    {
        //std::cout << "new zoom: " << targetZoom << " with " << width() << "*" << height() << std::endl;
        cloud=QPixmap(":/cloud.png");
        if(cloud.isNull())
            abort();
        grass=QPixmap(":/grass.png");
        if(grass.isNull())
            abort();
        sun=QPixmap(":/sun.png");
        if(sun.isNull())
            abort();
        treeback=QPixmap(":/treeback.png");
        if(treeback.isNull())
            abort();
        treefront=QPixmap(":/treefront.png");
        if(treefront.isNull())
            abort();
        if(targetZoom>1)
        {
            scalePix(cloud,targetZoom);
            scalePix(grass,targetZoom);
            scalePix(sun,targetZoom);
            scalePix(treeback,targetZoom);
            scalePix(treefront,targetZoom);
        }
        grassMove=rand()%grass.width()*targetZoom;
        treebackMove=rand()%treeback.width()*targetZoom;
        treefrontMove=rand()%treefront.width()*targetZoom;
        zoom=targetZoom;
    }

    const unsigned int grasswidth=grass.width();
    if((unsigned int)grassMove>=grasswidth*targetZoom && grasswidth>0)
        grassMove=grassMove%grasswidth*targetZoom;
    const unsigned int treebackwidth=treeback.width();
    if((unsigned int)treebackMove>=treebackwidth*targetZoom && treebackwidth>0)
        treebackMove=treebackMove%treebackwidth*targetZoom;
    const unsigned int treefrontwidth=treefront.width();
    if((unsigned int)treefrontMove>=treefrontwidth*targetZoom && treefrontwidth>0)
        treefrontMove=treefrontMove%treefrontwidth*targetZoom;
    //paint->fillRect(rect(),QColor(116,225,255));

    int grassOffset=0;
    int skyOffset=0;
    int sunOffset=0;
    unsigned int endOfGrass=0;
    if((unsigned int)widget->height()<=120*targetZoom)
        endOfGrass=widget->height();
    else
    {
        skyOffset=(widget->height()-120*targetZoom)*2/3/*66%*/;
        if(skyOffset>widget->height()/4)
            skyOffset=widget->height()/4;
        endOfGrass=120*targetZoom+(widget->height()-120*targetZoom)*2/3/*66%*/;
        paint->fillRect(0,endOfGrass,widget->width(),widget->height()-endOfGrass,QColor(131,203,83));
        paint->fillRect(0,0,widget->width(),skyOffset,QColor(115,225,255));
    }
    sunOffset=endOfGrass/6/*16%*/;

    QRect gradientRect(0,skyOffset,widget->width(),120*targetZoom);
    QLinearGradient gradient(gradientRect.topLeft(), gradientRect.bottomLeft()); // diagonal gradient from top-left to bottom-right
    gradient.setColorAt(0, QColor(115,225,255));
    gradient.setColorAt(1, QColor(183,241,255));
    paint->fillRect(0,skyOffset,widget->width(),endOfGrass-skyOffset,gradient);

    grassOffset=endOfGrass-grass.height();
    unsigned int treeFrontOffset=grassOffset+4*targetZoom-treefront.height();
    unsigned int treebackOffset=grassOffset+4*targetZoom-treeback.height();

    int lastTreebackX=-treebackMove;
    while(widget->width()>lastTreebackX)
    {
        paint->drawPixmap(lastTreebackX,treebackOffset,treeback.width(),    treeback.height(),    treeback);
        lastTreebackX+=treeback.width();
    }
    int lastTreefrontX=-treefrontMove;
    while(widget->width()>lastTreefrontX)
    {
        paint->drawPixmap(lastTreefrontX,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
        lastTreefrontX+=treefront.width();
    }
    int lastGrassX=-grassMove;
    while(widget->width()>lastGrassX)
    {
        paint->drawPixmap(lastGrassX,grassOffset,grass.width(),    grass.height(),    grass);
        lastGrassX+=grass.width();
    }

    paint->drawPixmap(widget->width()*2/3/*66%*/-sun.width()/2,sunOffset,sun.width(),    sun.height(),    sun);
    paint->drawPixmap(widget->width()/3/*33%*/-cloud.width()/2,skyOffset+(endOfGrass-skyOffset)/4/*16%*/,cloud.width(),    cloud.height(),    cloud);

    if(benchmark)
    {
        results.push_back(time.elapsed());
        if(results.size()*updateTimer.interval()>1000)/*benchmark on 1s*/
        {
            benchmark=false;
            unsigned int index=0;
            unsigned int minimalTime=(unsigned int)updateTimer.interval()/2;
            while(index<results.size())
            {
                if(results.at(index)<minimalTime)
                    break;
                index++;
            }
            if(index>=results.size())
            {
                grassTimer.stop();
                treebackTimer.stop();
                treefrontTimer.stop();
                updateTimer.stop();
            }
            results.clear();
        }
    }
}

QRectF CCBackground::boundingRect() const
{
    return QRectF();
}
