// Copyright 2021 CatchChallenger
#include "CCMap.hpp"

#include <math.h>

#include <QObject>
#include <QPainter>
#include <QTime>
#include <iostream>
#include <vector>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/MoveOnTheMap.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../core/EventManager.hpp"
#include "../../core/Logger.hpp"
#include "../../../qtmaprender/TileLayerItem.hpp"

using Scenes::CCMap;

CCMap::CCMap() {
  setCacheMode(QGraphicsItem::NoCache);
  if (!connect(&mapController, &MapController::pathFindingNotFound, this,
               &CCMap::pathFindingNotFound))
    abort();
  if (!connect(&mapController, &MapController::repelEffectIsOver, this,
               &CCMap::repelEffectIsOver))
    abort();
  if (!connect(&mapController, &MapController::teleportConditionNotRespected,
               this, &CCMap::teleportConditionNotRespected))
    abort();
  if (!connect(&mapController, &MapController::stopped_in_front_of, this,
               &CCMap::stopped_in_front_of))
    abort();
  if (!connect(&mapController, &MapController::actionOn, this,
               &CCMap::actionOn))
    abort();
  if (!connect(&mapController, &MapController::actionOnNothing, this,
               &CCMap::actionOnNothing))
    abort();
  if (!connect(&mapController, &MapController::blockedOn, this,
               &CCMap::blockedOn))
    abort();
  if (!connect(&mapController, &MapController::wildFightCollision, this,
               &CCMap::wildFightCollision))
    abort();
  if (!connect(&mapController, &MapController::botFightCollision, this,
               &CCMap::botFightCollision))
    abort();
  if (!connect(&mapController, &MapController::error, this, &CCMap::error))
    abort();
  if (!connect(&mapController, &MapController::errorWithTheCurrentMap, this,
               &CCMap::errorWithTheCurrentMap))
    abort();
  if (!connect(&mapController, &MapController::currentMapLoaded, this,
               &CCMap::currentMapLoaded))
    abort();
  clicked = false;
  scale = 1.0;
  x = 0.0;
  y = 0.0;

  mapController.resetAll();
  mapController.connectAllSignals(ConnectionManager::GetInstance()->client);
  mapController.datapackParsed();
  mapController.datapackParsedMainSub();
}

CCMap *CCMap::Create() { return new CCMap(); }

void CCMap::PaintChildItems(std::vector<Node *> childItems, qreal parentX,
                            qreal parentY, QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget *widget) {
  int index = 0;
  while (index < childItems.size()) {
    // take care of childs.at(index)->zValue()
    // take care of x,y
    auto child = childItems.at(index);
    qreal x = child->X() + parentX;
    qreal y = child->Y() + parentY;
    //MapObjectItem::x = x;
    //MapObjectItem::y = y;
    painter->translate(x, y);
    child->Draw(painter);
    painter->translate(-x, -y);
    if (!child->Children().empty()) {
      PaintChildItems(child->Children(), x, y, painter, option, widget);
    }
    index++;
  }
}

void CCMap::paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
                  QWidget *widget) {
  painter->save();
  qreal zoomW = (qreal)widget->width() / (32.0 * 16.0);
  qreal zoomH = (qreal)widget->height() / (32.0 * 16.0);
  qreal zoomFinal = zoomW;
  // keep the greatest value
  if (zoomFinal > zoomH) zoomFinal = zoomH;
  zoomFinal *=
      CatchChallenger::CommonDatapack::commonDatapack.get_layersOptions().zoom;
  scale = ceil(zoomFinal);
  //MapObjectItem::scale = scale;
  //MapObjectItem::playerObject = mapController.getPlayerMapObject();
  //painter->scale(scale, scale);

  //const Tiled::MapObject *p = mapController.getPlayerMapObject();
  //x = (widget->width() / 2 / scale - (p->x() * 16 + p->width() / 2));
  //y = (widget->height() / 2 / scale - ((p->y() - 1) * 16 + p->height() / 2));
  //PaintChildItems(mapController.mapItem->Children(), x, y, painter, option,
                  //widget);
  painter->restore();
}

void CCMap::MousePressEvent(const QPointF &p, bool &press_validated) {
  if (press_validated) return;
  if (clicked) return;
  clicked = true;
  lastClickedPos = p;
  press_validated = true;
}

void CCMap::MouseReleaseEvent(const QPointF &p, bool &press_validated) {
  if (!clicked) return;
  clicked = false;
  press_validated = true;
  //mapController.keyPressReset();

  // convert pixel scaled -> pixel -> tile
  qreal diffX = (p.x() / scale - x) / 16;
  qreal diffY = (p.y() / scale - y) / 16;
  // std::cout << "CCMap clicked: " << p.x() << "," << p.y() << " - " << diffX
  // << "," << diffY << " - " << x << "," << y << " * " << scale << std::endl;

  std::unordered_set<std::string> mapToScan;
  Map_full *current_map = mapController.currentMapFull();
  mapToScan.insert(current_map->logicalMap.map_file);
  if (current_map->logicalMap.border.left.map != nullptr)
    mapToScan.insert(current_map->logicalMap.border.left.map->map_file);
  if (current_map->logicalMap.border.right.map != nullptr)
    mapToScan.insert(current_map->logicalMap.border.right.map->map_file);
  if (current_map->logicalMap.border.top.map != nullptr)
    mapToScan.insert(current_map->logicalMap.border.top.map->map_file);
  if (current_map->logicalMap.border.bottom.map != nullptr)
    mapToScan.insert(current_map->logicalMap.border.bottom.map->map_file);

  std::unordered_map<std::string, Map_full *> all_map = mapController.all_map;
  // locate the right map
  for (const auto &n : all_map) {
    Map_full *map = n.second;
    if (mapToScan.find(map->logicalMap.map_file) != mapToScan.cend()) {
      /*std::cout << map->logicalMap.map_file << ": "
                 << std::to_string(map->relative_x) << ","
                 << std::to_string(map->relative_y) << ","
                 << std::to_string(map->logicalMap.width) << ","
                 << std::to_string(map->logicalMap.height) << std::endl;
      */
      if (diffX >= map->relative_x &&
          diffX <= (map->relative_x + map->logicalMap.width))
        if (diffY >= map->relative_y &&
            diffY <= (map->relative_y + map->logicalMap.height)) {
          std::cout << "click on " << map->logicalMap.map_file << " " << diffX
                    << "," << diffY << std::endl;
          //mapController.eventOnMap(CatchChallenger::MapEvent_SimpleClick, map,
                                   //(int)floor(diffX) - map->relative_x,
                                   //(int)floor(diffY) - map->relative_y);
          return;
        }
    }
  }
  Logger::Log(QString("CCMap not found: %1, %2").arg(p.x()).arg(p.y()));
}

void CCMap::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  mapController.keyPressEvent(event);
  event_trigger = false;
}

void CCMap::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  mapController.keyReleaseEvent(event);
  event_trigger = false;
}

void CCMap::OnEnter() {
  Scene::OnEnter();
  EventManager::GetInstance()->AddMouseListener(this);
  EventManager::GetInstance()->AddKeyboardListener(this);
  mapController.unblock();
}

void CCMap::OnExit() { 
  EventManager::GetInstance()->RemoveListener(this);
}
