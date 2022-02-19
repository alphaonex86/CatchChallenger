#include <QObject>
#include <iostream>

#include "../../../general/base/CommonDatapack.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "MapController.hpp"

std::string MapController::mapIdToString(const uint32_t &mapId) const {
  if (mapId >= (uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size()) {
    qDebug() << "MapController::insert_plant() mapId greater than "
                "QtDatapackClientLoader::datapackLoader->maps.size()";
    return std::string();
  }
  return QFileInfo(QString::fromStdString(
                       datapackMapPathSpec +
                       QtDatapackClientLoader::datapackLoader->get_maps().at(mapId)))
      .absoluteFilePath()
      .toStdString();
}

// plant
void MapController::insert_plant(const uint32_t &mapId, const uint8_t &x,
                                 const uint8_t &y, const uint8_t &plant_id,
                                 const uint16_t &seconds_to_mature) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
    DelayedPlantInsert tempItem;
    tempItem.mapId = mapId;
    tempItem.x = x;
    tempItem.y = y;
    tempItem.plant_id = plant_id;
    tempItem.seconds_to_mature = seconds_to_mature;
    delayedPlantInsert.push_back(tempItem);
    return;
  }
  if (mapId >= (uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size()) {
    qDebug() << "MapController::insert_plant() mapId greater than "
                "QtDatapackClientLoader::datapackLoader->maps.size()";
    return;
  }
  const std::string &map =
      QFileInfo(QString::fromStdString(
                    datapackMapPathSpec +
                    QtDatapackClientLoader::datapackLoader->get_maps().at(mapId)))
          .absoluteFilePath()
          .toStdString();
  if (!haveMapInMemory(map) || !mapItem->haveMap(all_map.at(map)->tiledMap)) {
    qDebug() << QString("map (%1) not show or not loaded, delay it")
                    .arg(QString::fromStdString(map));
    DelayedPlantInsert tempItem;
    tempItem.mapId = mapId;
    tempItem.x = x;
    tempItem.y = y;
    tempItem.plant_id = plant_id;
    tempItem.seconds_to_mature = seconds_to_mature;
    delayedPlantInsertOnMap[map].push_back(tempItem);
    return;
  }
  insert_plant_full(map, x, y, plant_id, seconds_to_mature);
}

void MapController::insert_plant_full(const std::string &map, const uint8_t &x,
                                      const uint8_t &y, const uint8_t &plant_id,
                                      const uint16_t &seconds_to_mature) {
  if (CatchChallenger::CommonDatapack::commonDatapack.get_plants().find(plant_id) ==
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().cend()) {
    qDebug() << "plant_id don't exists";
    return;
  }
  Map_full *map_full = all_map[map];
  unsigned int index = 0;
  while (index < map_full->logicalMap.plantList.size()) {
    if (map_full->logicalMap.plantList.at(index)->x == x &&
        map_full->logicalMap.plantList.at(index)->y == y) {
      qDebug() << "map have already an item at this point, remove it";
      remove_plant_full(map, x, y);
    } else
      index++;
  }
  uint64_t current_time = QDateTime::currentMSecsSinceEpoch() / 1000;
  CatchChallenger::ClientPlantWithTimer *plant =
      new CatchChallenger::ClientPlantWithTimer();
  plant->setSingleShot(true);
  plant->mapObject = new Tiled::MapObject();
  plant->x = x;
  plant->y = y;
  plant->plant_id = plant_id;
  plant->mature_at = current_time + seconds_to_mature;
  if (updatePlantGrowing(plant))
    if (!QObject::connect(plant, &QTimer::timeout, this,
                          &MapController::getPlantTimerEvent))
      abort();
  // move to the final position (integer), y+1 because the tile lib start y to
  // 1, not 0
  plant->mapObject->setPosition(QPoint(x, y + 1));

  map_full->logicalMap.plantList.push_back(plant);
#ifdef DEBUG_CLIENT_PLANTS
  qDebug() << std::stringLiteral("insert_plant(), map: %1 at: %2,%3")
                  .arg(map)
                  .arg(x)
                  .arg(y);
#endif
  if (ObjectGroupItem::objectGroupLink.find(all_map[map]->objectGroup) !=
      ObjectGroupItem::objectGroupLink.cend())
    ObjectGroupItem::objectGroupLink[all_map[map]->objectGroup]->addObject(
        plant->mapObject);
  else
    qDebug() << "insert_plant(), all_map[map]->objectGroup not contains "
                "current_map->objectGroup";
  MapObjectItem::objectLink[plant->mapObject]->SetZValue(y);
}

void MapController::getPlantTimerEvent() {
  QTimer *clientPlantTimer = qobject_cast<QTimer *>(QObject::sender());
  if (clientPlantTimer == NULL) return;
  if (!mHaveTheDatapack || mapVisualiserThread.stopIt) return;
  updatePlantGrowing(
      static_cast<CatchChallenger::ClientPlantWithTimer *>(clientPlantTimer));
}

// return true if is growing
bool MapController::updatePlantGrowing(
    CatchChallenger::ClientPlantWithTimer *plant) {
  uint64_t currentTime = QDateTime::currentMSecsSinceEpoch() / 1000;
  Tiled::Cell cell = plant->mapObject->cell();
  if (plant->mature_at <= currentTime) {
    cell.tile =
        QtDatapackClientLoader::datapackLoader->getPlantExtra(plant->plant_id)
            .tileset->tileAt(4);
    plant->mapObject->setCell(cell);
    return false;
  }
  int seconds_to_mature = static_cast<uint32_t>(plant->mature_at - currentTime);
  int floweringDiff =
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .fruits_seconds -
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .flowering_seconds;
  int tallerDiff =
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .fruits_seconds -
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .taller_seconds;
  int sproutedDiff =
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .fruits_seconds -
      CatchChallenger::CommonDatapack::commonDatapack.get_plants().at(plant->plant_id)
          .sprouted_seconds;
  if (seconds_to_mature < floweringDiff) {
    plant->start(seconds_to_mature * 1000);
    cell.tile =
        QtDatapackClientLoader::datapackLoader->getPlantExtra(plant->plant_id)
            .tileset->tileAt(3);
  } else if (seconds_to_mature < tallerDiff) {
    plant->start((seconds_to_mature - floweringDiff) * 1000);
    cell.tile =
        QtDatapackClientLoader::datapackLoader->getPlantExtra(plant->plant_id)
            .tileset->tileAt(2);
  } else if (seconds_to_mature < sproutedDiff) {
    plant->start((seconds_to_mature - tallerDiff) * 1000);
    cell.tile =
        QtDatapackClientLoader::datapackLoader->getPlantExtra(plant->plant_id)
            .tileset->tileAt(1);
  } else {
    plant->start((seconds_to_mature - sproutedDiff) * 1000);
    cell.tile =
        QtDatapackClientLoader::datapackLoader->getPlantExtra(plant->plant_id)
            .tileset->tileAt(0);
  }
  plant->mapObject->setCell(cell);
  return true;
}

void MapController::remove_plant(const uint32_t &mapId, const uint8_t &x,
                                 const uint8_t &y) {
  if (!mHaveTheDatapack) {
    unsigned int index = 0;
    while (index < delayedPlantInsert.size()) {
      if (delayedPlantInsert.at(index).mapId == mapId &&
          delayedPlantInsert.at(index).x == x &&
          delayedPlantInsert.at(index).y == y) {
        delayedPlantInsert.erase(delayedPlantInsert.cbegin() + index);
        return;
      }
      index++;
    }
    qDebug() << "MapController::remove_plant() remove item not found into the "
                "insert";
    return;
  }
  if (mapId >= (uint32_t)QtDatapackClientLoader::datapackLoader->get_maps().size()) {
    qDebug() << "MapController::remove_plant() mapId greater than "
                "QtDatapackClientLoader::datapackLoader->maps.size()";
    return;
  }
  remove_plant_full(
      QFileInfo(QString::fromStdString(
                    datapackMapPathSpec +
                    QtDatapackClientLoader::datapackLoader->get_maps().at(mapId)))
          .absoluteFilePath()
          .toStdString(),
      x, y);
}

void MapController::remove_plant_full(const std::string &map, const uint8_t &x,
                                      const uint8_t &y) {
#ifdef DEBUG_CLIENT_PLANTS
  qDebug() << std::stringLiteral("remove_plant(%1,%2,%3)")
                  .arg(QtDatapackClientLoader::datapackLoader->maps[mapId])
                  .arg(x)
                  .arg(y);
#endif
  if (all_map.find(map) == all_map.cend()) {
    std::vector<std::string> map_list;
    for (const auto &n : all_map) map_list.push_back(n.first);
    qDebug() << QString("map (%1) is not into map list: %2, ignore it")
                    .arg(QString::fromStdString(map))
                    .arg(QString::fromStdString(stringimplode(map_list, ";")));
    return;
  }
  if (!mapItem->haveMap(all_map[map]->tiledMap)) {
    qDebug() << QString("map (%1) not show, ignore it")
                    .arg(QString::fromStdString(map));
    return;
  }
  Map_full *map_full = all_map[map];
  unsigned int index = 0;
  while (index < map_full->logicalMap.plantList.size()) {
    if (map_full->logicalMap.plantList.at(index)->x == x &&
        map_full->logicalMap.plantList.at(index)->y == y) {
      // unload the player sprite
      if (ObjectGroupItem::objectGroupLink.find(
              map_full->logicalMap.plantList.at(index)
                  ->mapObject->objectGroup()) !=
          ObjectGroupItem::objectGroupLink.cend())
        ObjectGroupItem::objectGroupLink
            [map_full->logicalMap.plantList.at(index)->mapObject->objectGroup()]
                ->removeObject(
                    map_full->logicalMap.plantList.at(index)->mapObject);
      else
        qDebug()
            << "remove_plant(), ObjectGroupItem::objectGroupLink not contains "
               "map_full->logicalMap.plantList.at(index).mapObject->"
               "objectGroup()";
      delete map_full->logicalMap.plantList.at(index)->mapObject;
      delete map_full->logicalMap.plantList.at(index);
      map_full->logicalMap.plantList.erase(
          map_full->logicalMap.plantList.cbegin() + index);
    } else
      index++;
  }
}

void MapController::updateGrowing() {}

void MapController::seed_planted(const bool &ok) {
  if (!mHaveTheDatapack) {
    qDebug() << "Prevent seed_planted before datapack is loaded";
    return;
  }
  Q_UNUSED(ok);
}

void MapController::plant_collected(
    const CatchChallenger::Plant_collect &stat) {
  if (!mHaveTheDatapack) {
    qDebug() << "Prevent seed_planted before datapack is loaded";
    return;
  }
  Q_UNUSED(stat);
}

void MapController::reinject_signals() {
  MapControllerMP::reinject_signals();
  unsigned int index;
  if (mHaveTheDatapack && player_informations_is_set) {
    index = 0;
    while (index < delayedPlantInsert.size()) {
      insert_plant(
          delayedPlantInsert.at(index).mapId, delayedPlantInsert.at(index).x,
          delayedPlantInsert.at(index).y, delayedPlantInsert.at(index).plant_id,
          delayedPlantInsert.at(index).seconds_to_mature);
      index++;
    }
    delayedPlantInsert.clear();
  }
}

void MapController::tryLoadPlantOnMapDisplayed(const std::string &fileName) {
  if (delayedPlantInsertOnMap.find(fileName) == delayedPlantInsertOnMap.cend())
    return;
  unsigned int index = 0;
  std::vector<DelayedPlantInsert> values = delayedPlantInsertOnMap.at(fileName);
  while (index < values.size()) {
    insert_plant(values.at(index).mapId, values.at(index).x, values.at(index).y,
                 values.at(index).plant_id, values.at(index).seconds_to_mature);
    index++;
  }
  delayedPlantInsertOnMap.erase(fileName);
}

