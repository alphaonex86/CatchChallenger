// Copyright 2021 CatchChallenger
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QPointer>
#include <QStyleOptionGraphicsItem>
#include <QTime>
#include <iostream>

#include "../../../../general/base/CommonMap.hpp"
#include "../../../../general/base/FacilityLib.hpp"
#include "../../../../general/base/FacilityLibGeneral.hpp"
#include "../../../../general/base/GeneralVariable.hpp"
#include "../../../../general/base/MoveOnTheMap.hpp"
#include "../../../libcatchchallenger/ClientStructures.hpp"
#include "../../../libcatchchallenger/ClientVariable.hpp"
#include "../../FacilityLibClient.hpp"
#include "../../libraries/tiled/tiled_tile.hpp"
#include "MapVisualiser.hpp"

/// \warning all ObjectGroupItem destroyed into removeMap()
void MapVisualiser::destroyMap(Map_full *map) {
  // logicalMap.plantList, delete plants useless, destroyed into removeMap()
  // logicalMap.botsDisplay, delete bot useless, destroyed into removeMap()
  // remove from the list
  for (const auto /*& crash with ref*/ n : map->doors) {
    n.second->deleteLater();
  }
  map->doors.clear();
  if (map->tiledMap != NULL) {
    mapItem->removeMap(map->tiledMap);
  }
  if (all_map.find(map->logicalMap.map_file) != all_map.cend()) {
    all_map[map->logicalMap.map_file] = NULL;
    all_map.erase(map->logicalMap.map_file);
  }
  if (old_all_map.find(map->logicalMap.map_file) != old_all_map.cend()) {
    old_all_map[map->logicalMap.map_file];
    old_all_map.erase(map->logicalMap.map_file);
  }
  // delete common variables
  CatchChallenger::CommonMap::removeParsedLayer(map->logicalMap.parsed_layer);
  map->logicalMap.parsed_layer.simplifiedMap = NULL;
  qDeleteAll(map->tiledMap->tilesets());
  if (map->tiledMap != NULL) delete map->tiledMap;
  map->tiledMap = NULL;
  if (map->tiledRender != NULL) delete map->tiledRender;
  map->tiledRender = NULL;
  delete map;
}

void MapVisualiser::resetAll() {
  /// remove the not used map, then where no player is susceptible to switch (by
  /// border or teleporter)
  std::vector<Map_full *> mapListToDelete;
  for (auto /*& crash with ref*/ n : old_all_map)
    mapListToDelete.push_back(n.second);
  for (auto /*& crash with ref*/ n : all_map)
    mapListToDelete.push_back(n.second);
  unsigned int index = 0;
  while (index < mapListToDelete.size()) {
    destroyMap(mapListToDelete.at(index));
    index++;
  }
  old_all_map.clear();
  old_all_map_time.clear();
  all_map.clear();
  mapVisualiserThread.resetAll();
}

// open the file, and load it into the variables
void MapVisualiser::loadOtherMap(const std::string &resolvedFileName) {
  if (current_map.empty()) {
    std::cerr << "MapVisualiser::loadOtherMap() map empty" << std::endl;
    return;
  }
  // already loaded
  if (all_map.find(resolvedFileName) != all_map.cend()) return;
  // already in progress
  if (vectorcontainsAtLeastOne(asyncMap, resolvedFileName)) return;
  // previously loaded
  if (old_all_map.find(resolvedFileName) != old_all_map.cend()) {
    Map_full *tempMapObject = old_all_map.at(resolvedFileName);
    tempMapObject->displayed = false;
    old_all_map.erase(resolvedFileName);
    old_all_map_time.erase(resolvedFileName);
    tempMapObject->logicalMap.border.bottom.map = NULL;
    tempMapObject->logicalMap.border.top.map = NULL;
    tempMapObject->logicalMap.border.left.map = NULL;
    tempMapObject->logicalMap.border.right.map = NULL;
    tempMapObject->logicalMap.teleporter = NULL;
    tempMapObject->logicalMap.teleporter_list_size = 0;
    asyncMap.push_back(resolvedFileName);
    asyncMapLoaded(resolvedFileName, tempMapObject);
    return;
  }
#ifdef CATCHCHALLENGER_EXTRA_CHECK
  if (!QFileInfo(QString::fromStdString(resolvedFileName)).exists())
    qDebug() << (QStringLiteral("file not found to async: %1")
                     .arg(QString::fromStdString(resolvedFileName)));
#endif
  asyncMap.push_back(resolvedFileName);
  emit loadOtherMapAsync(resolvedFileName);
}

void MapVisualiser::asyncDetectBorder(Map_full *tempMapObject) {
  if (tempMapObject == NULL) {
    qDebug()
        << "Map is NULL, can't load more at MapVisualiser::asyncDetectBorder()";
    return;
  }
  QRect current_map_rect;
  if (!current_map.empty() && all_map.find(current_map) != all_map.cend())
    current_map_rect = QRect(0, 0, all_map.at(current_map)->logicalMap.width,
                             all_map.at(current_map)->logicalMap.height);
  else {
    qDebug() << "The current map is not set, crash prevented";
    return;
  }
  QRect map_rect(tempMapObject->relative_x, tempMapObject->relative_y,
                 tempMapObject->logicalMap.width,
                 tempMapObject->logicalMap.height);
  // if the new map touch the current map
  if (CatchChallenger::FacilityLibClient::rectTouch(current_map_rect,
                                                    map_rect)) {
    // display a new map now visible
    if (!mapItem->haveMap(tempMapObject->tiledMap)) {
      mapItem->addMap(tempMapObject, tempMapObject->tiledMap,
                      tempMapObject->tiledRender,
                      tempMapObject->objectGroupIndex);
    }
    mapItem->setMapPosition(
        tempMapObject->tiledMap,
        static_cast<uint16_t>(tempMapObject->relative_x_pixel),
        static_cast<uint16_t>(tempMapObject->relative_y_pixel));
    emit mapDisplayed(tempMapObject->logicalMap.map_file);
    // display the bot
    for (const auto &n : tempMapObject->logicalMap.bots) {
      std::string skin;
      if (n.second.properties.find("skin") != n.second.properties.cend())
        skin = n.second.properties.at("skin");
      std::string direction;
      if (n.second.properties.find("lookAt") != n.second.properties.cend())
        direction = n.second.properties.at("lookAt");
      else {
        if (!skin.empty())
          qDebug() << QStringLiteral(
                          "asyncDetectBorder(): lookAt: missing, fixed to "
                          "bottom: %1")
                          .arg(QString::fromStdString(
                              tempMapObject->logicalMap.map_file));
        direction = "bottom";
      }
      loadBotOnTheMap(tempMapObject, n.second.botId, n.first.first,
                      n.first.second, direction, skin);
    }
    if (!tempMapObject->logicalMap.border_semi.bottom.fileName.empty())
      if (!vectorcontainsAtLeastOne(
              asyncMap,
              tempMapObject->logicalMap.border_semi.bottom.fileName) &&
          all_map.find(tempMapObject->logicalMap.border_semi.bottom.fileName) ==
              all_map.cend())
        loadOtherMap(tempMapObject->logicalMap.border_semi.bottom.fileName);
    if (!tempMapObject->logicalMap.border_semi.top.fileName.empty())
      if (!vectorcontainsAtLeastOne(
              asyncMap, tempMapObject->logicalMap.border_semi.top.fileName) &&
          all_map.find(tempMapObject->logicalMap.border_semi.top.fileName) ==
              all_map.cend())
        loadOtherMap(tempMapObject->logicalMap.border_semi.top.fileName);
    if (!tempMapObject->logicalMap.border_semi.left.fileName.empty())
      if (!vectorcontainsAtLeastOne(
              asyncMap, tempMapObject->logicalMap.border_semi.left.fileName) &&
          all_map.find(tempMapObject->logicalMap.border_semi.left.fileName) ==
              all_map.cend())
        loadOtherMap(tempMapObject->logicalMap.border_semi.left.fileName);
    if (!tempMapObject->logicalMap.border_semi.right.fileName.empty())
      if (!vectorcontainsAtLeastOne(
              asyncMap, tempMapObject->logicalMap.border_semi.right.fileName) &&
          all_map.find(tempMapObject->logicalMap.border_semi.right.fileName) ==
              all_map.cend())
        loadOtherMap(tempMapObject->logicalMap.border_semi.right.fileName);
  }
}

bool MapVisualiser::asyncMapLoaded(const std::string &fileName,
                                   Map_full *tempMapObject) {
  if (current_map.empty()) return false;
  if (all_map.find(fileName) != all_map.cend()) {
    vectorremoveOne(asyncMap, fileName);
    qDebug() << (QStringLiteral(
                     "seam already loaded by sync call, internal bug on: %1")
                     .arg(QString::fromStdString(fileName)));
    return false;
  }
  if (tempMapObject != NULL) {
    tempMapObject->displayed = false;
    all_map[tempMapObject->logicalMap.map_file] = tempMapObject;
    for (auto &n : tempMapObject->animatedObject) {
      // MapVisualiserOrder::Map_animation &map_animation=n.second;
      const uint16_t &interval = static_cast<uint16_t>(n.first);
      if (animationTimer.find(interval) == animationTimer.cend()) {
        QTimer *newTimer = new QTimer();
        newTimer->setInterval(interval);
        animationTimer[interval] = newTimer;
        if (!connect(newTimer, &QTimer::timeout, this,
                     &MapVisualiser::applyTheAnimationTimer))
          abort();
        newTimer->start();
      }
      /*do at another place unsigned int index=0;
      while(index<map_animation.animatedObjectList.size())
      {
          MapVisualiserOrder::Map_animation_object
      &map_animation_object=map_animation.animatedObjectList.at(index);
          Tiled::MapObject * mapObject=map_animation_object.animatedObject;
          Tiled::Cell cell=mapObject->cell();
          Tiled::Tile *tile=mapObject->cell().tile;
          Tiled::Tile *newTile=NULL;
          newTile=tile->tileset()->tileAt(tile->id()+1);
          if(newTile->id()>=map_animation.maxId)
              newTile=tile->tileset()->tileAt(map_animation.minId);
          cell.tile=newTile;
          mapObject->setCell(cell);
          index++;
      }*/
    }
    // try locate and place it
    if (tempMapObject->logicalMap.map_file == current_map) {
      tempMapObject->displayed = true;
      tempMapObject->relative_x = 0;
      tempMapObject->relative_y = 0;
      tempMapObject->relative_x_pixel = 0;
      tempMapObject->relative_y_pixel = 0;
      loadTeleporter(tempMapObject);
      asyncDetectBorder(tempMapObject);
    } else {
      QRect current_map_rect;
      if (!current_map.empty() && all_map.find(current_map) != all_map.cend()) {
        current_map_rect =
            QRect(0, 0, all_map.at(current_map)->logicalMap.width,
                  all_map.at(current_map)->logicalMap.height);
        if (all_map.find(tempMapObject->logicalMap.border_semi.top.fileName) !=
            all_map.cend()) {
          if (all_map.at(tempMapObject->logicalMap.border_semi.top.fileName)
                  ->displayed) {
            Map_full *border_map =
                all_map.at(tempMapObject->logicalMap.border_semi.top.fileName);
            // if both border match
            if (tempMapObject->logicalMap.map_file ==
                border_map->logicalMap.border_semi.bottom.fileName) {
              const int &offset =
                  border_map->logicalMap.border_semi.bottom.x_offset -
                  tempMapObject->logicalMap.border_semi.top.x_offset;
              const int &offset_pixel =
                  border_map->logicalMap.border_semi.bottom.x_offset *
                      border_map->tiledMap->tileWidth() -
                  tempMapObject->logicalMap.border_semi.top.x_offset *
                      tempMapObject->tiledMap->tileWidth();
              tempMapObject->relative_x = border_map->relative_x + offset;
              tempMapObject->relative_y =
                  border_map->relative_y + border_map->logicalMap.height;
              tempMapObject->relative_x_pixel =
                  border_map->relative_x_pixel + offset_pixel;
              tempMapObject->relative_y_pixel =
                  border_map->relative_y_pixel +
                  border_map->logicalMap.height *
                      border_map->tiledMap->tileHeight();
              tempMapObject->logicalMap.border.top.map =
                  &border_map->logicalMap;
              tempMapObject->logicalMap.border.top.x_offset =
                  static_cast<int16_t>(offset);
              if (!vectorcontainsAtLeastOne(
                      tempMapObject->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &border_map->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &border_map->logicalMap);
              border_map->logicalMap.border.bottom.map =
                  &tempMapObject->logicalMap;
              border_map->logicalMap.border.bottom.x_offset =
                  static_cast<int16_t>(-offset);
              if (!vectorcontainsAtLeastOne(
                      border_map->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &tempMapObject->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &tempMapObject->logicalMap);
              QRect map_rect(tempMapObject->relative_x,
                             tempMapObject->relative_y,
                             tempMapObject->logicalMap.width,
                             tempMapObject->logicalMap.height);
              if (CatchChallenger::FacilityLibClient::rectTouch(
                      current_map_rect, map_rect)) {
                tempMapObject->displayed = true;
                asyncDetectBorder(tempMapObject);
              }
            } else
              qDebug()
                  << QStringLiteral(
                         "loadNearMap(): bottom: have not mutual border %1")
                         .arg(QString::fromStdString(
                             tempMapObject->logicalMap.map_file));
          }
        }
        if (all_map.find(
                tempMapObject->logicalMap.border_semi.bottom.fileName) !=
            all_map.cend()) {
          if (all_map.at(tempMapObject->logicalMap.border_semi.bottom.fileName)
                  ->displayed) {
            Map_full *border_map = all_map.at(
                tempMapObject->logicalMap.border_semi.bottom.fileName);
            // if both border match
            if (tempMapObject->logicalMap.map_file ==
                border_map->logicalMap.border_semi.top.fileName) {
              const int &offset =
                  border_map->logicalMap.border_semi.top.x_offset -
                  tempMapObject->logicalMap.border_semi.bottom.x_offset;
              const int &offset_pixel =
                  border_map->logicalMap.border_semi.top.x_offset *
                      border_map->tiledMap->tileWidth() -
                  tempMapObject->logicalMap.border_semi.bottom.x_offset *
                      tempMapObject->tiledMap->tileWidth();
              tempMapObject->relative_x = border_map->relative_x + offset;
              tempMapObject->relative_y =
                  border_map->relative_y - tempMapObject->logicalMap.height;
              tempMapObject->relative_x_pixel =
                  border_map->relative_x_pixel + offset_pixel;
              tempMapObject->relative_y_pixel =
                  border_map->relative_y_pixel -
                  tempMapObject->logicalMap.height *
                      tempMapObject->tiledMap->tileHeight();
              tempMapObject->logicalMap.border.bottom.map =
                  &border_map->logicalMap;
              tempMapObject->logicalMap.border.bottom.x_offset =
                  static_cast<int16_t>(offset);
              if (!vectorcontainsAtLeastOne(
                      tempMapObject->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &border_map->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &border_map->logicalMap);
              border_map->logicalMap.border.top.map =
                  &tempMapObject->logicalMap;
              border_map->logicalMap.border.top.x_offset =
                  static_cast<int16_t>(-offset);
              if (!vectorcontainsAtLeastOne(
                      border_map->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &tempMapObject->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &tempMapObject->logicalMap);
              QRect map_rect(tempMapObject->relative_x,
                             tempMapObject->relative_y,
                             tempMapObject->logicalMap.width,
                             tempMapObject->logicalMap.height);
              if (CatchChallenger::FacilityLibClient::rectTouch(
                      current_map_rect, map_rect)) {
                tempMapObject->displayed = true;
                asyncDetectBorder(tempMapObject);
              }
            } else
              qDebug()
                  << QStringLiteral(
                         "loadNearMap(): bottom: have not mutual border %1")
                         .arg(QString::fromStdString(
                             tempMapObject->logicalMap.map_file));
          }
        }
        if (all_map.find(
                tempMapObject->logicalMap.border_semi.right.fileName) !=
            all_map.cend()) {
          if (all_map.at(tempMapObject->logicalMap.border_semi.right.fileName)
                  ->displayed) {
            Map_full *border_map = all_map.at(
                tempMapObject->logicalMap.border_semi.right.fileName);
            // if both border match
            if (tempMapObject->logicalMap.map_file ==
                border_map->logicalMap.border_semi.left.fileName) {
              const int &offset =
                  border_map->logicalMap.border_semi.left.y_offset -
                  tempMapObject->logicalMap.border_semi.right.y_offset;
              const int &offset_pixel =
                  border_map->logicalMap.border_semi.left.y_offset *
                      border_map->tiledMap->tileHeight() -
                  tempMapObject->logicalMap.border_semi.right.y_offset *
                      tempMapObject->tiledMap->tileHeight();
              tempMapObject->relative_x =
                  border_map->relative_x - tempMapObject->logicalMap.width;
              tempMapObject->relative_y = border_map->relative_y + offset;
              tempMapObject->relative_x_pixel =
                  border_map->relative_x_pixel -
                  tempMapObject->logicalMap.width *
                      tempMapObject->tiledMap->tileWidth();
              tempMapObject->relative_y_pixel =
                  border_map->relative_y_pixel + offset_pixel;
              tempMapObject->logicalMap.border.right.map =
                  &border_map->logicalMap;
              tempMapObject->logicalMap.border.right.y_offset =
                  static_cast<int16_t>(offset);
              if (!vectorcontainsAtLeastOne(
                      tempMapObject->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &border_map->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &border_map->logicalMap);
              border_map->logicalMap.border.left.map =
                  &tempMapObject->logicalMap;
              border_map->logicalMap.border.left.y_offset =
                  static_cast<int16_t>(-offset);
              if (!vectorcontainsAtLeastOne(
                      border_map->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &tempMapObject->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &tempMapObject->logicalMap);
              QRect map_rect(tempMapObject->relative_x,
                             tempMapObject->relative_y,
                             tempMapObject->logicalMap.width,
                             tempMapObject->logicalMap.height);
              if (CatchChallenger::FacilityLibClient::rectTouch(
                      current_map_rect, map_rect)) {
                tempMapObject->displayed = true;
                asyncDetectBorder(tempMapObject);
              }
            } else
              qDebug()
                  << QStringLiteral(
                         "loadNearMap(): bottom: have not mutual border %1")
                         .arg(QString::fromStdString(
                             tempMapObject->logicalMap.map_file));
          }
        }
        if (all_map.find(tempMapObject->logicalMap.border_semi.left.fileName) !=
            all_map.cend()) {
          if (all_map.at(tempMapObject->logicalMap.border_semi.left.fileName)
                  ->displayed) {
            Map_full *border_map =
                all_map.at(tempMapObject->logicalMap.border_semi.left.fileName);
            // if both border match
            if (tempMapObject->logicalMap.map_file ==
                border_map->logicalMap.border_semi.right.fileName) {
              const int &offset =
                  border_map->logicalMap.border_semi.right.y_offset -
                  tempMapObject->logicalMap.border_semi.left.y_offset;
              const int &offset_pixel =
                  border_map->logicalMap.border_semi.right.y_offset *
                      border_map->tiledMap->tileHeight() -
                  tempMapObject->logicalMap.border_semi.left.y_offset *
                      tempMapObject->tiledMap->tileHeight();
              tempMapObject->relative_x =
                  border_map->relative_x + border_map->logicalMap.width;
              tempMapObject->relative_y = border_map->relative_y + offset;
              tempMapObject->relative_x_pixel =
                  border_map->relative_x_pixel +
                  border_map->logicalMap.width *
                      border_map->tiledMap->tileWidth();
              tempMapObject->relative_y_pixel =
                  border_map->relative_y_pixel + offset_pixel;
              tempMapObject->logicalMap.border.left.map =
                  &border_map->logicalMap;
              tempMapObject->logicalMap.border.left.y_offset =
                  static_cast<int16_t>(offset);
              if (!vectorcontainsAtLeastOne(
                      tempMapObject->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &border_map->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &border_map->logicalMap);
              border_map->logicalMap.border.right.map =
                  &tempMapObject->logicalMap;
              border_map->logicalMap.border.right.y_offset =
                  static_cast<int16_t>(-offset);
              if (!vectorcontainsAtLeastOne(
                      border_map->logicalMap.near_map,
                      static_cast<CatchChallenger::CommonMap *>(
                          &tempMapObject->logicalMap)))
                tempMapObject->logicalMap.near_map.push_back(
                    &tempMapObject->logicalMap);
              QRect map_rect(tempMapObject->relative_x,
                             tempMapObject->relative_y,
                             tempMapObject->logicalMap.width,
                             tempMapObject->logicalMap.height);
              if (CatchChallenger::FacilityLibClient::rectTouch(
                      current_map_rect, map_rect)) {
                tempMapObject->displayed = true;
                asyncDetectBorder(tempMapObject);
              }
            } else
              qDebug()
                  << QStringLiteral(
                         "loadNearMap(): bottom: have not mutual border %1")
                         .arg(QString::fromStdString(
                             tempMapObject->logicalMap.map_file));
          }
        }
      } else
        qDebug() << "The current map is not set, crash prevented";
    }
  }
  if (vectorremoveOne(asyncMap, fileName))
    if (asyncMap.empty()) {
      hideNotloadedMap();  // hide the mpa loaded by mistake
      removeUnusedMap();
    }
  if (tempMapObject != NULL)
    return true;
  else
    return false;
}

void MapVisualiser::applyTheAnimationTimer() {
  QTimer *timer = qobject_cast<QTimer *>(QObject::sender());
  const uint16_t &interval = static_cast<uint16_t>(timer->interval());
  bool isUsed = false;
  for (const auto &n : all_map) {
    Map_full *tempMap = n.second;
    if (tempMap->displayed) {
      if (tempMap->animatedObject.find(interval) !=
          tempMap->animatedObject.cend()) {
        for (auto &n : tempMap->animatedObject[interval]) {
          MapVisualiserOrder::Map_animation &map_animation = n.second;
          isUsed = true;
          std::vector<MapVisualiserThread::Map_animation_object>
              &animatedObject = map_animation.animatedObjectList;
          unsigned int index = 0;
          while (index < animatedObject.size()) {
            MapVisualiserThread::Map_animation_object &animation_object =
                animatedObject[index];

            Tiled::MapObject *mapObject = animation_object.animatedObject;
            Tiled::Cell cell = mapObject->cell();
            Tiled::Tile *tile = mapObject->cell().tile;
            Tiled::Tile *newTile = tile->tileset()->tileAt(tile->id() + 1);
            if (newTile->id() >= map_animation.maxId)
              newTile = tile->tileset()->tileAt(map_animation.minId);
            cell.tile = newTile;
            mapObject->setCell(cell);

            index++;
          }
        }
      }
    }
  }
  if (!isUsed) {
#ifdef CATCHCHALLENGER_EXTRA_CHECK
    std::cout << "MapVisualiser::applyTheAnimationTimer() : !isUsed for timer: "
              << std::to_string(interval) << std::endl;
#endif
    animationTimer.erase(interval);
    timer->stop();
    delete timer;
  }
}

void MapVisualiser::loadBotOnTheMap(Map_full *parsedMap, const uint32_t &botId,
                                    const uint8_t &x, const uint8_t &y,
                                    const std::string &lookAt,
                                    const std::string &skin) {
  Q_UNUSED(botId);
  Q_UNUSED(parsedMap);
  Q_UNUSED(x);
  Q_UNUSED(y);
  Q_UNUSED(lookAt);
  Q_UNUSED(skin);
}

void MapVisualiser::removeUnusedMap() {
  const QDateTime &currentTime = QDateTime::currentDateTime();
  // undisplay the unused map
  for (const auto &n : old_all_map) {
    if (old_all_map_time.find(n.first) == old_all_map_time.cend() ||
        (currentTime.toTime_t() - old_all_map_time.at(n.first).toTime_t()) >
            CATCHCHALLENGER_CLIENT_MAP_CACHE_TIMEOUT ||
        old_all_map.size() > CATCHCHALLENGER_CLIENT_MAP_CACHE_SIZE)
      destroyMap(n.second);
  }
}

void MapVisualiser::hideNotloadedMap() {
  // undisplay the map not in dirrect contact with the current_map
  for (const auto &n : all_map) {
    if (n.second->logicalMap.map_file != current_map) {
      if (!n.second->displayed) {
        mapItem->removeMap(n.second->tiledMap);
      }
    }
  }
  for (const auto &n : old_all_map) {
    mapItem->removeMap(n.second->tiledMap);
  }
}

void MapVisualiser::passMapIntoOld() {
  const QDateTime &currentTime = QDateTime::currentDateTime();
  for (const auto &n : all_map) {
    old_all_map[n.first] = n.second;
    old_all_map_time[n.first] = currentTime;
  }
  all_map.clear();
}

void MapVisualiser::loadTeleporter(Map_full *map) {
  // load the teleporter
  unsigned int index = 0;
  while (index < map->logicalMap.teleport_semi.size()) {
    loadOtherMap(map->logicalMap.teleport_semi.at(index).map);
    index++;
  }
}
