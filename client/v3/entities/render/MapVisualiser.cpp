// Copyright 2021 CatchChallenger
#include "MapVisualiser.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QGLFormat>
#include <QGLWidget>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLabel>
#include <QMessageBox>
#include <QPixmapCache>
#include <QPointer>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <iostream>

#include "../../../general/base/MoveOnTheMap.hpp"

MapVisualiser::MapVisualiser(const bool &debugTags) : mark(NULL) {
  qRegisterMetaType<Map_full *>("Map_full *");

  if (!connect(this, &MapVisualiser::loadOtherMapAsync, &mapVisualiserThread,
               &MapVisualiserThread::loadOtherMapAsync, Qt::QueuedConnection))
    abort();
  if (!connect(&mapVisualiserThread, &MapVisualiserThread::asyncMapLoaded, this,
               &MapVisualiser::asyncMapLoaded, Qt::QueuedConnection))
    abort();

  /*    setCacheMode(QGraphicsView::CacheBackground);
      QPixmapCache::setCacheLimit(102400);*/

  mapVisualiserThread.debugTags = debugTags;
  this->debugTags = debugTags;

  mapItem = MapItem::Create();
  if (!connect(mapItem, &MapItem::eventOnMap, this, &MapVisualiser::eventOnMap))
    abort();

  grass = NULL;
  grassOver = NULL;

  tagTilesetIndex = 0;
  markPathFinding =
      new Tiled::Tileset(QStringLiteral("mark path finding"), 16, 24);
  {
    QImage image(QStringLiteral(":/CC/images/map/marker.png"));
    if (image.isNull())
      qDebug() << "Unable to load the image for marker because is null";
    else if (!markPathFinding->loadFromImage(
                 image, QStringLiteral(":/CC/images/map/marker.png")))
      qDebug() << "Unable to load the image for marker";
  }
}

MapVisualiser::~MapVisualiser() {
  if (mark != NULL) delete mark;
  // remove the not used map
  /// \todo re-enable this
  /*std::unordered_map<QString,Map_full *>::const_iterator i =
  all_map.constBegin(); while (i != all_map.constEnd()) { destroyMap(*i); i =
  all_map.constBegin();//needed
  }*/

  // delete mapItem;
  // delete playerMapObject;
}

Map_full *MapVisualiser::getMap(const std::string &map) const {
  if (all_map.find(map) != all_map.cend()) return all_map.at(map);
  abort();
  return NULL;
}

void MapVisualiser::eventOnMap(CatchChallenger::MapEvent event,
                               Map_full *tempMapObject, uint8_t x, uint8_t y) {
  if (event == CatchChallenger::MapEvent_SimpleClick) {
    if (mark != NULL) delete mark;
    /*        if(mark!=NULL)
    {
        Tiled::MapObject *mapObject=mark->mapObject();
        if(mapObject!=NULL)
            ObjectGroupItem::objectGroupLink.value(mapObject->objectGroup())->removeObject(mapObject);
    }*/
    Tiled::MapObject *mapObject = new Tiled::MapObject();
    mapObject->setName("Mark for path finding");
    Tiled::Cell cell = mapObject->cell();
    cell.tile = markPathFinding->tileAt(0);
    if (cell.tile == NULL) qDebug() << "Tile NULL before map mark contructor";
    mapObject->setCell(cell);
    mapObject->setPosition(QPointF(x, y + 1));
    mark = new MapMark(mapObject);
    ObjectGroupItem::objectGroupLink.at(tempMapObject->objectGroup)
        ->addObject(mapObject);
    if (MapObjectItem::objectLink.find(mapObject) !=
        MapObjectItem::objectLink.cend())
      MapObjectItem::objectLink.at(mapObject)->SetZValue(9999);
  }
}
