#include "MapVisualiser.hpp"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QMessageBox>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QGLFormat>
#include <QGLWidget>
#endif
#include <QLabel>
#include <QPixmapCache>

#include "../../general/base/MoveOnTheMap.hpp"

MapVisualiser::MapVisualiser(const bool &debugTags,const bool &useCache,const bool &openGL) :
    mScene(new QGraphicsScene(this)),
    mark(NULL)
{
    qRegisterMetaType<Map_full *>("Map_full *");

    if(!connect(this,&MapVisualiser::loadOtherMapAsync,&mapVisualiserThread,&MapVisualiserThread::loadOtherMapAsync,Qt::QueuedConnection))
        abort();
    if(!connect(&mapVisualiserThread,&MapVisualiserThread::asyncMapLoaded,this,&MapVisualiser::asyncMapLoaded,Qt::QueuedConnection))
        abort();

    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    if(openGL)
        setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    #endif
    setRenderHint(QPainter::Antialiasing,false);
    setRenderHint(QPainter::TextAntialiasing,false);
    #if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    setRenderHint(QPainter::NonCosmeticDefaultPen,true);
    setRenderHint(QPainter::HighQualityAntialiasing,false);
    #endif
    setRenderHint(QPainter::SmoothPixmapTransform,false);
    setCacheMode(QGraphicsView::CacheBackground);
    QPixmapCache::setCacheLimit(102400);

    mapVisualiserThread.debugTags=debugTags;
    this->debugTags=debugTags;

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=40;
    timerRender.setSingleShot(true);
    if(!connect(&timerRender,&QTimer::timeout,this,&MapVisualiser::render))
        abort();
    if(!connect(&timerUpdateFPS,&QTimer::timeout,this,&MapVisualiser::updateFPS))
        abort();
    timerUpdateFPS.start();

    mapItem=new MapItem(NULL,useCache);
    if(!connect(mapItem,&MapItem::eventOnMap,this,&MapVisualiser::eventOnMap))
        abort();

    grass=NULL;
    grassOver=NULL;

    setScene(mScene);
/*    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);*/
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing | QGraphicsView::DontSavePainterState | QGraphicsView::IndirectPainting);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);

    viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);

    tagTilesetIndex=0;
    markPathFinding=Tiled::Tileset::create(QStringLiteral("mark path finding"),16,24);
    {
        QImage image(QStringLiteral(":/CC/images/map/marker.png"));
        if(image.isNull())
            qDebug() << "Unable to load the image for marker because is null";
        else if(!markPathFinding->loadFromImage(image,QStringLiteral(":/CC/images/map/marker.png")))
            qDebug() << "Unable to load the image for marker";
    }

    mScene->addItem(mapItem);
    //mScene->setSceneRect(QRectF(xPerso*TILE_SIZE,yPerso*TILE_SIZE,64,32));
    render();
}

MapVisualiser::~MapVisualiser()
{
    if(mark!=NULL)
        delete mark;
    //remove the not used map
    /// \todo re-enable this
    /*std::unordered_map<QString,Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        destroyMap(*i);
        i = all_map.constBegin();//needed
    }*/

    //delete mapItem;
    //delete playerMapObject;
}

Map_full * MapVisualiser::getMap(const std::string &map) const
{
    if(all_map.find(map)!=all_map.cend())
        return all_map.at(map);
    abort();
    return NULL;
}

void MapVisualiser::render()
{
    //mScene->update();
    viewport()->update();
}

void MapVisualiser::paintEvent(QPaintEvent * event)
{
    timeRender.restart();

    QGraphicsView::paintEvent(event);

    uint32_t elapsed=timeRender.elapsed();
    if(waitRenderTime<=elapsed)
        timerRender.start(1);
    else
        timerRender.start(waitRenderTime-elapsed);

    if(frameCounter<65535)
        frameCounter++;
}

void MapVisualiser::updateFPS()
{
    const unsigned int FPS=(int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed();
    emit newFPSvalue(FPS);

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

void MapVisualiser::setTargetFPS(int targetFPS)
{
    if(targetFPS==0)
        waitRenderTime=0;
    else
    {
        waitRenderTime=static_cast<uint8_t>(static_cast<float>(1000.0)/(float)targetFPS);
        if(waitRenderTime<1)
            waitRenderTime=1;
    }
}

void MapVisualiser::eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y)
{
    if(event==CatchChallenger::MapEvent_SimpleClick)
    {
        if(mark!=NULL)
            delete mark;
        /*        if(mark!=NULL)
        {
            Tiled::MapObject *mapObject=mark->mapObject();
            if(mapObject!=NULL)
                ObjectGroupItem::objectGroupLink.value(mapObject->objectGroup())->removeObject(mapObject);
        }*/
        Tiled::MapObject *mapObject=new Tiled::MapObject();
        mapObject->setName("Mark for path finding");
        Tiled::Cell cell=mapObject->cell();
        cell.setTile(markPathFinding->tileAt(0));
        if(cell.tile()==NULL)
            qDebug() << "Tile NULL before map mark contructor";
        mapObject->setCell(cell);
        mapObject->setPosition(QPointF(x,y+1));
        mark=new MapMark(mapObject);
        ObjectGroupItem::objectGroupLink.at(tempMapObject->objectGroup)->addObject(mapObject);
        if(MapObjectItem::objectLink.find(mapObject)!=MapObjectItem::objectLink.cend())
            MapObjectItem::objectLink.at(mapObject)->setZValue(9999);
    }
}
