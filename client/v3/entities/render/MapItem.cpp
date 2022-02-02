// Copyright 2021 CatchChallenger
#include "MapItem.hpp"

#include <QDebug>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <iostream>

#include "../ClientVariable.hpp"
#include "PreparedLayer.hpp"

MapItem::MapItem(Node *parent) : Node(parent) {
  setFlag(QGraphicsItem::ItemHasNoContents);
  setCacheMode(QGraphicsItem::ItemCoordinateCache); // just change direction
  // without move do bug
}

MapItem *MapItem::Create(Node *parent) {
  return new (std::nothrow) MapItem(parent);
}

void MapItem::addMap(Map_full *tempMapObject, Tiled::Map *map,
                     Tiled::MapRenderer *renderer,
                     const int &playerLayerIndex) {
  if (displayed_layer.find(map) != displayed_layer.cend()) {
    qDebug() << "Map already displayed";
    return;
  }
  // align zIndex to "Dyna management" Layer
  int index = -playerLayerIndex;
  const QList<Tiled::Layer *> &layers = map->layers();

  QImage image;
  Node *graphicsItem = nullptr;
  QStringList mapNameList;
  // Create a child item for each layer
  const int &loopSize = layers.size();
  int index2 = 0;
  while (index2 < loopSize) {
    mapNameList << layers.at(index2)->name();
    if (Tiled::TileLayer *tileLayer = layers.at(index2)->asTileLayer()) {
      graphicsItem = TileLayerItem::Create(tileLayer, renderer);
      if (image.size().isNull()) {
        image = QImage(QSize(graphicsItem->BoundingRect().size().width(),
                             graphicsItem->BoundingRect().size().height()),
                       QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
      }

      QPainter painter(&image);
      if (painter.paintEngine() == nullptr)
        std::cerr
            << "painter.paintEngine()==nullptr before renderer->drawTileLayer"
            << std::endl;
      renderer->drawTileLayer(&painter, tileLayer,
                              graphicsItem->BoundingRect());
      delete graphicsItem;
      graphicsItem = nullptr;
    } else if (Tiled::ObjectGroup *objectGroup =
                   layers.at(index2)->asObjectGroup()) {
      if (!image.size().isNull()) {
        QPixmap tempPixmap;
        if (tempPixmap.convertFromImage(image)) {
#ifdef DEBUG_RENDER_DISPLAY_INDIVIDUAL_LAYER
          QLabel *temp = new QLabel();
          temp->setPixmap(tempPixmap);
          temp->show();
#endif
          graphicsItem = PreparedLayer::Create(tempMapObject, tempPixmap, this);
          if (!connect(static_cast<PreparedLayer *>(graphicsItem),
                       &PreparedLayer::eventOnMap, this, &MapItem::eventOnMap))
            abort();
          graphicsItem->SetZValue(index - 1);
          displayed_layer[map].insert(graphicsItem);
          image = QImage();
        }
      }

      ObjectGroupItem *newObject = ObjectGroupItem::Create(objectGroup, this);
      graphicsItem = newObject;

      ObjectGroupItem::objectGroupLink[objectGroup] = newObject;
      MapObjectItem::mRendererList[objectGroup] = renderer;
    }
    if (graphicsItem != nullptr) {
      graphicsItem->SetZValue(index);
      displayed_layer[map].insert(graphicsItem);
    }
    index++;
    index2++;
  }
  if (!image.size().isNull()) {
    QPixmap tempPixmap;
    if (tempPixmap.convertFromImage(image)) {
#ifdef DEBUG_RENDER_DISPLAY_INDIVIDUAL_LAYER
      QLabel *temp = new QLabel();
      temp->setPixmap(tempPixmap);
      temp->show();
#endif
      graphicsItem = PreparedLayer::Create(tempMapObject, tempPixmap, this);
      if (!connect(static_cast<PreparedLayer *>(graphicsItem),
                   &PreparedLayer::eventOnMap, this, &MapItem::eventOnMap))
        abort();
      graphicsItem->SetZValue(index);
      displayed_layer[map].insert(graphicsItem);
    }
  }

#ifdef DEBUG_RENDER_CACHE
  if (cache)
    qDebug() << "Map: " << layers.size() << " layers (" << mapNameList.join(";")
             << "), but only " << displayed_layer.count(map) << " displayed";
#endif
}

bool MapItem::haveMap(Tiled::Map *map) {
  return displayed_layer.find(map) != displayed_layer.cend();
}

void MapItem::removeMap(Tiled::Map *map) {
  std::cerr << "map: " << map << std::endl;
  if (displayed_layer.find(map) == displayed_layer.cend()) return;
  const std::unordered_set<Node *> &values = displayed_layer.at(map);
  for (const auto &value : values) {
    std::cerr << "values.at(index): " << value << std::endl;
    delete value;
  }
  displayed_layer.erase(map);
}

void MapItem::setMapPosition(Tiled::Map *map,
                             int16_t x /*pixel, need be 16Bits*/,
                             int16_t y /*pixel, need be 16Bits*/) {
  if (displayed_layer.find(map) == displayed_layer.cend()) return;
  const std::unordered_set<Node *> &values = displayed_layer.at(map);
  for (const auto &value : values)
    value->SetPos(static_cast<qreal>(static_cast<double>(x)),
                  static_cast<qreal>(static_cast<double>(y)));
}

void MapItem::Draw(QPainter *) {}
