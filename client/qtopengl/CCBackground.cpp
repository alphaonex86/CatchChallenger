#include "CCBackground.h"
#include <QPainter>
#include <QTime>
#include <chrono>
#include <iostream>

/*unsigned int CCBackground::timeToDraw;
unsigned int CCBackground::timeDraw;
std::chrono::time_point<std::chrono::steady_clock> CCBackground::timeFromStart;*/

CCBackground::CCBackground(QGraphicsItem *parent) :
    QGraphicsItem(parent)
{
    zoom=0;
    grassMove=0;
    treebackMove=0;
    treefrontMove=0;
    if(!QObject::connect(&grassTimer,&QTimer::timeout,this,&CCBackground::grassSlot))
        abort();
    if(!QObject::connect(&treebackTimer,&QTimer::timeout,this,&CCBackground::treebackSlot))
        abort();
    if(!QObject::connect(&treefrontTimer,&QTimer::timeout,this,&CCBackground::treefrontSlot))
        abort();
    if(!QObject::connect(&updateTimer,&QTimer::timeout,this,&CCBackground::updateSlot))
        abort();
    startAnimation();
    //timeFromStart=std::chrono::steady_clock::now();
}

QRectF CCBackground::boundingRect() const
{
    return QRectF();
}

void CCBackground::startAnimation()
{
    unsigned int baseTime=20;
    grassTimer.start(baseTime);
    treefrontTimer.start(baseTime*3);
    treebackTimer.start(baseTime*9);
    updateTimer.start(40);
}

void CCBackground::stopAnimation()
{
    grassTimer.stop();
    treefrontTimer.stop();
    treebackTimer.stop();
    updateTimer.stop();
}

void CCBackground::scalePix(QPixmap &pix,unsigned int zoom)
{
    pix=pix.scaled(pix.width()*zoom,pix.height()*zoom);
}

void CCBackground::updateSlot()
{
    update();
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

unsigned int CCBackground::getTargetZoom(QWidget *widget)
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

void CCBackground::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *widget)
{
    QTime time;
    time.restart();
    unsigned int targetZoom=getTargetZoom(widget);
    if(zoom!=targetZoom)
    {
        //std::cout << "new zoom: " << targetZoom << " with " << width() << "*" << height() << std::endl;
        cloud=QPixmap(":/CC/images/animatedbackground/cloud.png");
        if(cloud.isNull())
            abort();
        grass=QPixmap(":/CC/images/animatedbackground/grass.png");
        if(grass.isNull())
            abort();
        sun=QPixmap(":/CC/images/animatedbackground/sun.png");
        if(sun.isNull())
            abort();
        treeback=QPixmap(":/CC/images/animatedbackground/treeback.png");
        if(treeback.isNull())
            abort();
        treefront=QPixmap(":/CC/images/animatedbackground/treefront.png");
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
    treefrontMove%=treefront.width()*targetZoom;
    treebackMove%=treeback.width()*targetZoom;
    grassMove%=grass.width()*targetZoom;
    //paint.fillRect(rect(),QColor(116,225,255));

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
        painter->fillRect(0,endOfGrass,widget->width(),widget->height()-endOfGrass,QColor(131,203,83));
        painter->fillRect(0,0,widget->width(),skyOffset,QColor(115,225,255));
    }
    sunOffset=endOfGrass/6/*16%*/;

    QRect gradientRect(0,skyOffset,widget->width(),120*targetZoom);
    QLinearGradient gradient(gradientRect.topLeft(), gradientRect.bottomLeft()); // diagonal gradient from top-left to bottom-right
    gradient.setColorAt(0, QColor(115,225,255));
    gradient.setColorAt(1, QColor(183,241,255));
    painter->fillRect(0,skyOffset,widget->width(),endOfGrass-skyOffset,gradient);

    grassOffset=endOfGrass-grass.height();
    unsigned int treeFrontOffset=grassOffset+4*targetZoom-treefront.height();
    unsigned int treebackOffset=grassOffset+4*targetZoom-treeback.height();

    int lastTreebackX=-treebackMove;
    while(widget->width()>lastTreebackX)
    {
        painter->drawPixmap(lastTreebackX,treebackOffset,treeback.width(),    treeback.height(),    treeback);
        lastTreebackX+=treeback.width();
    }
    int lastTreefrontX=-treefrontMove;
    while(widget->width()>lastTreefrontX)
    {
        painter->drawPixmap(lastTreefrontX,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
        lastTreefrontX+=treefront.width();
    }
    int lastGrassX=-grassMove;
    while(widget->width()>lastGrassX)
    {
        painter->drawPixmap(lastGrassX,grassOffset,grass.width(),    grass.height(),    grass);
        lastGrassX+=grass.width();
    }

    painter->drawPixmap(widget->width()*2/3/*66%*/-sun.width()/2,sunOffset,sun.width(),    sun.height(),    sun);
    painter->drawPixmap(widget->width()/3/*33%*/-cloud.width()/2,skyOffset+(endOfGrass-skyOffset)/4/*16%*/,cloud.width(),    cloud.height(),    cloud);
}
