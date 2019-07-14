#include "View.h"

View::View(QWidget *parent) :
    QGraphicsView(parent)
{
    /*zoom=0;
    setMinimumHeight(120);
    setMinimumWidth(120);
    grassMove=0;
    treebackMove=0;
    treefrontMove=0;
    connect(&grassTimer,&QTimer::timeout,this,&View::grassSlot);
    connect(&treebackTimer,&QTimer::timeout,this,&View::treebackSlot);
    connect(&treefrontTimer,&QTimer::timeout,this,&View::treefrontSlot);
    unsigned int baseTime=20;
    grassTimer.start(baseTime);
    treefrontTimer.start(baseTime*3);
    treebackTimer.start(baseTime*9);
    connect(&updateTimer,&QTimer::timeout,this,&View::updateCustom);
    updateTimer.start(40);*/
}

void View::resizeEvent(QResizeEvent *event)
{
    //scene.setSceneRect(0,0,width(),height());
    QGraphicsView::resizeEvent(event);
}

void View::updateCustom()
{
    viewport()->update();
}

void View::scalePix(QPixmap &pix,unsigned int zoom)
{
    pix=pix.scaled(pix.width()*zoom,pix.height()*zoom);
}

void View::grassSlot()
{
    /*if(grass.isNull())
        return;
    grassMove++;
    unsigned int targetZoom=getTargetZoom();
    if(grassMove>=grass.width()*targetZoom)
        grassMove=0;*/
}

void View::treebackSlot()
{
    /*if(treeback.isNull())
        return;
    treebackMove++;
    unsigned int targetZoom=getTargetZoom();
    if(treebackMove>=treeback.width()*targetZoom)
        treebackMove=0;*/
}

void View::treefrontSlot()
{
    /*if(treefront.isNull())
        return;
    treefrontMove++;
    unsigned int targetZoom=getTargetZoom();
    if(treefrontMove>=treefront.width()*targetZoom)
        treefrontMove=0;*/
}

unsigned int View::getTargetZoom()
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
    return targetZoom;
}

/*void View::paintEvent(QPaintEvent *)
{
    unsigned int targetZoom=getTargetZoom();
    if(zoom!=targetZoom)
    {
        //std::cout << "new zoom: " << targetZoom << " with " << width() << "*" << height() << std::endl;
        cloud=QGraphicsPixmapItem(QPixmap(":/cloud.png"));
        if(cloud.isNull())
            abort();
        grass=QGraphicsPixmapItem(QPixmap(":/grass.png"));
        if(grass.isNull())
            abort();
        sun=QGraphicsPixmapItem(QPixmap(":/sun.png"));
        if(sun.isNull())
            abort();
        treeback=QGraphicsPixmapItem(QPixmap(":/treeback.png"));
        if(treeback.isNull())
            abort();
        treefront=QGraphicsPixmapItem(QPixmap(":/treefront.png"));
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
        skyOffset=(height()-120*targetZoom)*2/3;
        if(skyOffset>height()/4)
            skyOffset=height()/4;
        endOfGrass=120*targetZoom+(height()-120*targetZoom)*2/3;
        paint.fillRect(0,endOfGrass,width(),height()-endOfGrass,QColor(131,203,83));
        paint.fillRect(0,0,width(),skyOffset,QColor(115,225,255));
    }
    sunOffset=endOfGrass/6;

    QRect gradientRect(0,skyOffset,width(),120*targetZoom);
    QLinearGradient gradient(gradientRect.topLeft(), gradientRect.bottomLeft()); // diagonal gradient from top-left to bottom-right
    gradient.setColorAt(0, QColor(115,225,255));
    gradient.setColorAt(1, QColor(183,241,255));
    paint.fillRect(0,skyOffset,width(),endOfGrass-skyOffset,gradient);

    grassOffset=endOfGrass-grass.height();
    unsigned int treeFrontOffset=grassOffset+4*targetZoom-treefront.height();
    unsigned int treebackOffset=grassOffset+4*targetZoom-treeback.height();

    int lastTreebackX=-treebackMove;
    while(width()>lastTreebackX)
    {
        paint.drawPixmap(lastTreebackX,treebackOffset,treeback.width(),    treeback.height(),    treeback);
        lastTreebackX+=treeback.width();
    }
    int lastTreefrontX=-treefrontMove;
    while(width()>lastTreefrontX)
    {
        paint.drawPixmap(lastTreefrontX,treeFrontOffset,treefront.width(),    treefront.height(),    treefront);
        lastTreefrontX+=treefront.width();
    }
    int lastGrassX=-grassMove;
    while(width()>lastGrassX)
    {
        paint.drawPixmap(lastGrassX,grassOffset,grass.width(),    grass.height(),    grass);
        lastGrassX+=grass.width();
    }

    paint.drawPixmap(width()*2/3-sun.width()/2,sunOffset,sun.width(),    sun.height(),    sun);
    paint.drawPixmap(width()/3-cloud.width()/2,skyOffset+(endOfGrass-skyOffset)/4,cloud.width(),    cloud.height(),    cloud);
}*/
