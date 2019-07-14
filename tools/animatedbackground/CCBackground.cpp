#include "CCBackground.h"
#include <QPainter>
#include <iostream>

CCBackground::CCBackground(QWidget *parent) :
    QWidget(parent)
{
    zoom=0;
    setMinimumHeight(120);
    setMinimumWidth(120);
}

void CCBackground::scalePix(QPixmap &pix,unsigned int zoom)
{
    pix=pix.scaled(pix.width()*zoom,pix.height()*zoom);
}

void CCBackground::resizeEvent(QResizeEvent*)
{
}

void CCBackground::paintEvent(QPaintEvent *)
{
    unsigned int targetZoomHeight=(unsigned int)height()/120;
    if(targetZoomHeight<1)
        targetZoomHeight=1;
    if(targetZoomHeight>8)
        targetZoomHeight=8;
    unsigned int targetZoomWidth=(unsigned int)width()/120;
    if(targetZoomWidth<1)
        targetZoomWidth=1;
    if(targetZoomWidth>8)
        targetZoomWidth=8;
    unsigned int targetZoom=targetZoomHeight;
    if(targetZoomWidth<targetZoomHeight)
        targetZoom=targetZoomWidth;
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
        zoom=targetZoom;
    }
    QPainter paint;
    paint.begin(this);
    //paint.fillRect(rect(),QColor(116,225,255));

    int grassOffset=0;
    int skyOffset=0;
    int sunOffset=0;
    unsigned int endOfGrass=0;
    if((unsigned int)height()<=120*targetZoom)
        endOfGrass=height();
    else
    {
        skyOffset=(height()-120*targetZoom)*2/3/*66%*/;
        if(skyOffset>height()/4)
            skyOffset=height()/4;
        endOfGrass=120*targetZoom+(height()-120*targetZoom)*2/3/*66%*/;
        paint.fillRect(0,endOfGrass,width(),height()-endOfGrass,QColor(131,203,83));
        paint.fillRect(0,0,width(),skyOffset,QColor(115,225,255));
    }
    sunOffset=endOfGrass/6/*16%*/;

    QRect gradientRect(0,skyOffset,width(),120*targetZoom);
    QLinearGradient gradient(gradientRect.topLeft(), gradientRect.bottomLeft()); // diagonal gradient from top-left to bottom-right
    gradient.setColorAt(0, QColor(115,225,255));
    gradient.setColorAt(1, QColor(183,241,255));
    paint.fillRect(0,skyOffset,width(),endOfGrass-skyOffset,gradient);

    grassOffset=endOfGrass-grass.height();
    unsigned int treeFrontOffset=grassOffset+4*targetZoom-treefront.height();
    unsigned int treebackOffset=grassOffset+4*targetZoom-treeback.height();

    paint.drawPixmap(0,treebackOffset,treeback.width(),    treeback.height(),    treeback);
    int lastTreebackX=treeback.width();
    while(width()>lastTreebackX)
    {
        paint.drawPixmap(lastTreebackX,treebackOffset,treeback.width(),    treeback.height(),    treeback);
        lastTreebackX+=treeback.width();
    }

    paint.drawPixmap(0,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
    int lastTreefrontX=treefront.width();
    while(width()>lastTreefrontX)
    {
        paint.drawPixmap(lastTreefrontX,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
        lastTreefrontX+=treefront.width();
    }

    paint.drawPixmap(0,grassOffset,grass.width(),    grass.height(),    grass);
    int lastGrassX=grass.width();
    while(width()>lastGrassX)
    {
        paint.drawPixmap(lastGrassX,grassOffset,grass.width(),    grass.height(),    grass);
        lastGrassX+=grass.width();
    }

    paint.drawPixmap(width()*2/3/*66%*/-sun.width()/2,sunOffset,sun.width(),    sun.height(),    sun);
    paint.drawPixmap(width()/3/*33%*/-cloud.width()/2,skyOffset+(endOfGrass-skyOffset)/4/*16%*/,cloud.width(),    cloud.height(),    cloud);
}
