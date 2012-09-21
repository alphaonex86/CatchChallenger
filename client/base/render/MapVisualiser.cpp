#include "MapVisualiser.h"

#include <QCoreApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <QDebug>
#include <QFileInfo>
#include <QPointer>
#include <QMessageBox>

#include "../../general/base/MoveOnTheMap.h"

MapVisualiser::MapVisualiser(QWidget *parent,const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    QGraphicsView(parent),
    mScene(new QGraphicsScene(this))
{
    setRenderHint(QPainter::Antialiasing,false);
    setRenderHint(QPainter::TextAntialiasing,false);
    setCacheMode(QGraphicsView::CacheBackground);

    this->centerOnPlayer=centerOnPlayer;
    this->debugTags=debugTags;

    timerUpdateFPS.setSingleShot(true);
    timerUpdateFPS.setInterval(1000);
    timeUpdateFPS.restart();
    frameCounter=0;
    timeRender.restart();
    waitRenderTime=30;
    timerRender.setSingleShot(true);
    connect(&timerRender,SIGNAL(timeout()),this,SLOT(render()));
    connect(&timerUpdateFPS,SIGNAL(timeout()),this,SLOT(updateFPS()));
    timerUpdateFPS.start();
    FPSText=NULL;
    mShowFPS=false;

    current_map=NULL;
    mapItem=new MapItem(NULL,useCache);

    setWindowTitle(tr("map-visualiser-qt"));

    setScene(mScene);
/*    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setDragMode(QGraphicsView::ScrollHandDrag);*/
    setOptimizationFlags(QGraphicsView::DontAdjustForAntialiasing
                         | QGraphicsView::DontSavePainterState);
    setBackgroundBrush(Qt::black);
    setFrameStyle(0);

    //viewport()->setAttribute(Qt::WA_StaticContents);
    viewport()->setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAttribute(Qt::WA_NoSystemBackground);
    setViewportUpdateMode(QGraphicsView::NoViewportUpdate);
    if(OpenGL)
    {
        QGLFormat format(QGL::StencilBuffer | QGL::AlphaChannel | QGL::DoubleBuffer | QGL::Rgba);
        //setSampleBuffers
        format.setRgba(true);
        format.setAlpha(true);

        QGLWidget *widgetOpenGL=new QGLWidget(format);// | QGL::IndirectRendering -> do a crash
        if(widgetOpenGL==NULL)
            QMessageBox::critical(this,"No OpenGL","Sorry but OpenGL can't be enabled, be sure of support with your graphic drivers: create widget");
        else
            setViewport(widgetOpenGL);
    }

    tagTilesetIndex=0;
    tagTileset = new Tiled::Tileset("tags",16,16);
    tagTileset->loadFromImage(QImage(":/tags.png"),":/tags.png");

    mScene->addItem(mapItem);
    //mScene->setSceneRect(QRectF(xPerso*TILE_SIZE,yPerso*TILE_SIZE,64,32));

    FPSText=new QGraphicsSimpleTextItem(mapItem);
    FPSText->setVisible(mShowFPS);
    mScene->addItem(FPSText);
    FPSText->setPos(2,2);
    QFont bold;
    bold.setBold(true);
    FPSText->setFont(bold);
    FPSText->setPen(QPen(Qt::black));
    FPSText->setBrush(Qt::white);
    setShowFPS(mShowFPS);

    render();
}

MapVisualiser::~MapVisualiser()
{
    //remove the not used map
    QHash<QString,Map_full *>::const_iterator i = all_map.constBegin();
    while (i != all_map.constEnd()) {
        delete (*i)->logicalMap.parsed_layer.walkable;
        delete (*i)->logicalMap.parsed_layer.water;
        qDeleteAll((*i)->tiledMap->tilesets());
        delete (*i)->tiledMap;
        delete (*i)->tiledRender;
        delete (*i);
        all_map.remove((*i)->logicalMap.map_file);
        i = all_map.constBegin();//needed
    }

    //delete mapItem;
    //delete playerMapObject;
    delete tagTileset;
}

bool MapVisualiser::viewMap(const QString &fileName)
{
    current_map=NULL;

    QTime startTime;
    startTime.restart();

    //commented to not blink
    //blink_dyna_layer.start(200);
    connect(&blink_dyna_layer,SIGNAL(timeout()),this,SLOT(blinkDynaLayer()));

    QString current_map_fileName=loadOtherMap(fileName);
    if(current_map_fileName.isEmpty())
    {
        QMessageBox::critical(this,"Error",mLastError);
        return false;
    }
    current_map=all_map[current_map_fileName];

    render();
    return true;
}

bool MapVisualiser::RectTouch(QRect r1,QRect r2)
{
    if (r1.isNull() || r2.isNull())
        return false;

    if((r1.x()+r1.width())<r2.x())
        return false;
    if((r2.x()+r2.width())<r1.x())
        return false;

    if((r1.y()+r1.height())<r2.y())
        return false;
    if((r2.y()+r2.height())<r1.y())
        return false;

    return true;
}

MapVisualiser::Map_full * MapVisualiser::getMap(QString map)
{
    if(all_map.contains(map))
        return all_map[map];
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

    quint32 elapsed=timeRender.elapsed();
    if(waitRenderTime<=elapsed)
        timerRender.start(0);
    else
        timerRender.start(waitRenderTime-elapsed);

    if(frameCounter<65535)
        frameCounter++;
}

void MapVisualiser::updateFPS()
{
    if(FPSText!=NULL && mShowFPS)
        FPSText->setText(QString("%1 FPS").arg((int)(((float)frameCounter)*1000)/timeUpdateFPS.elapsed()));

    frameCounter=0;
    timeUpdateFPS.restart();
    timerUpdateFPS.start();
}

bool MapVisualiser::showFPS()
{
    return mShowFPS;
}

void MapVisualiser::setShowFPS(const bool &showFPS)
{
    mShowFPS=showFPS;
    if(FPSText!=NULL)
        FPSText->setVisible(showFPS);
}

void MapVisualiser::setTargetFPS(int targetFPS)
{
    if(targetFPS==0)
        waitRenderTime=0;
    else
    {
        waitRenderTime=1000.0/(float)targetFPS;
        if(waitRenderTime<1)
            waitRenderTime=1;
    }
}
