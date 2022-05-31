#include <iostream>

#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "MapController.hpp"

void MapControllerMP::insert_player(
    const CatchChallenger::Player_public_informations &player,
    const uint32_t &mapId, const uint16_t &x, const uint16_t &y,
    const CatchChallenger::Direction &direction) {
  insert_player_final(player, mapId, x, y, direction, false);
}

void MapControllerMP::move_player(
    const uint16_t &id,
    const std::vector<std::pair<uint8_t, CatchChallenger::Direction> >
        &movement) {
  move_player_final(id, movement, false);
}

void MapControllerMP::remove_player(const uint16_t &id) {
  remove_player_final(id, false);
}

void MapControllerMP::reinsert_player(
    const uint16_t &id, const uint8_t &x, const uint8_t &y,
    const CatchChallenger::Direction &direction) {
  reinsert_player_final(id, x, y, direction, false);
}

void MapControllerMP::full_reinsert_player(
    const uint16_t &id, const uint32_t &mapId, const uint8_t &x,
    const uint8_t &y, const CatchChallenger::Direction &direction) {
  full_reinsert_player_final(id, mapId, x, y, direction, false);
}

void MapControllerMP::dropAllPlayerOnTheMap() {
  dropAllPlayerOnTheMap_final(false);
}

// map move
bool MapControllerMP::insert_player_final(
    const CatchChallenger::Player_public_informations &player,
    const uint32_t &mapId, const uint16_t &x, const uint16_t &y,
    const CatchChallenger::Direction &direction, bool inReplayMode) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
    if (!inReplayMode) {
      DelayedInsert tempItem;
      tempItem.player = player;
      tempItem.mapId = mapId;
      tempItem.x = x;
      tempItem.y = y;
      tempItem.direction = direction;

      DelayedMultiplex multiplex;
      multiplex.insert = tempItem;
      multiplex.type = DelayedType_Insert;
      delayedActions.push_back(multiplex);
    }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
    qDebug() << QStringLiteral("delayed: insert_player(%1->%2,%3,%4,%5,%6)")
                    .arg(player.pseudo)
                    .arg(player.simplifiedId)
                    .arg(mapId)
                    .arg(x)
                    .arg(y)
                    .arg(CatchChallenger::MoveOnTheMap::directionToString(
                        direction));
#endif
    return false;
  }
  if (mapId >= (uint32_t)QtDatapackClientLoader::GetInstance()->get_maps().size()) {
    /// \bug here pass after delete a party, create a new
    emit error(
        "mapId greater than "
        "QtDatapackClientLoader::GetInstance()->maps.size(): " +
        std::to_string(QtDatapackClientLoader::GetInstance()->get_maps().size()));
    return true;
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  qDebug()
      << QStringLiteral("insert_player(%1->%2,%3,%4,%5,%6)")
             .arg(player.pseudo)
             .arg(player.simplifiedId)
             .arg(QtDatapackClientLoader::GetInstance()->maps.value(mapId))
             .arg(x)
             .arg(y)
             .arg(CatchChallenger::MoveOnTheMap::directionToString(direction));
#endif
  // current player
  if (player.simplifiedId ==
      client->get_player_informations_ro().public_informations.simplifiedId)
    MapVisualiserPlayer::insert_player_internal(player, mapId, x, y, direction,
                                                skinFolderList);
  // other player
  else {
    if (otherPlayerList.find(player.simplifiedId) != otherPlayerList.cend()) {
      qDebug() << QStringLiteral("Other player (%1) already loaded on the map")
                      .arg(player.simplifiedId);
      // return true;-> ignored to fix temporally, but need remove at map unload

      // clean other player
      remove_player_final(player.simplifiedId, false);
    }

    OtherPlayer otherPlayer;
    otherPlayer.playerMapObject = nullptr;
    otherPlayer.playerTileset = nullptr;
    otherPlayer.informations.monsterId = 0;
    otherPlayer.informations.simplifiedId = 0;
    otherPlayer.informations.skinId = 0;
    otherPlayer.informations.speed = 0;
    otherPlayer.informations.type = CatchChallenger::Player_type_normal;
    otherPlayer.labelMapObject = nullptr;
    otherPlayer.labelTileset = nullptr;
    otherPlayer.playerSpeed = 0;
    otherPlayer.animationDisplayed = false;
    otherPlayer.monsterMapObject = nullptr;
    otherPlayer.monsterTileset = nullptr;
    otherPlayer.monster_x = 0;
    otherPlayer.monster_y = 0;
    otherPlayer.presumed_map = nullptr;
    otherPlayer.presumed_x = 0;
    otherPlayer.presumed_y = 0;

    otherPlayer.x = static_cast<uint8_t>(x);
    otherPlayer.y = static_cast<uint8_t>(y);
    otherPlayer.presumed_x = static_cast<uint8_t>(x);
    otherPlayer.presumed_y = static_cast<uint8_t>(y);
    otherPlayer.monster_x = static_cast<uint8_t>(x);
    otherPlayer.monster_y = static_cast<uint8_t>(y);
    otherPlayer.direction = direction;
    otherPlayer.moveStep = 0;
    otherPlayer.inMove = false;
    otherPlayer.pendingMonsterMoves.clear();
    otherPlayer.stepAlternance = false;

    const std::string &mapPath =
        QFileInfo(QString::fromStdString(
                      datapackMapPathSpec +
                      QtDatapackClientLoader::GetInstance()->get_maps().at(mapId)))
            .absoluteFilePath()
            .toStdString();
    if (all_map.find(mapPath) == all_map.cend()) {
      qDebug() << "MapControllerMP::insert_player(): current map "
               << QString::fromStdString(mapPath) << " not loaded, delayed: ";
      for (const auto &n : all_map) std::cout << n.first << std::endl;
      qDebug() << "List end";
      if (!inReplayMode) {
        DelayedInsert tempItem;
        tempItem.player = player;
        tempItem.mapId = mapId;
        tempItem.x = x;
        tempItem.y = y;
        tempItem.direction = direction;

        DelayedMultiplex multiplex;
        multiplex.insert = tempItem;
        multiplex.type = DelayedType_Insert;
        delayedActions.push_back(multiplex);
      }
      return false;
    }
    /// \todo do a player cache here
    // the player skin
    if (player.skinId < skinFolderList.size()) {
      QImage image(QString::fromStdString(
          datapackPath + DATAPACK_BASE_PATH_SKIN +
          skinFolderList.at(player.skinId) + "/trainer.png"));
      if (!image.isNull()) {
        otherPlayer.playerMapObject = new Tiled::MapObject();
        otherPlayer.playerMapObject->setName("Other player");
        otherPlayer.playerTileset = new Tiled::Tileset(
            QString::fromStdString(skinFolderList.at(player.skinId)), 16, 24);
        if (!otherPlayer.playerTileset->loadFromImage(
                image, QString::fromStdString(
                           datapackPath + DATAPACK_BASE_PATH_SKIN +
                           skinFolderList.at(player.skinId) + "/trainer.png")))
          abort();
      } else {
        qDebug() << "Unable to load the player tilset: " +
                        QString::fromStdString(
                            datapackPath + DATAPACK_BASE_PATH_SKIN +
                            skinFolderList.at(player.skinId) + "/trainer.png");
        return true;
      }
    } else {
      qDebug() << QStringLiteral("The skin id: ") +
                      QString::number(player.skinId) +
                      QStringLiteral(", into a list of: ") +
                      QString::number(skinFolderList.size()) +
                      QStringLiteral(
                          " item(s) info MapControllerMP::insert_player()");
      return true;
    }
    {
      QPixmap pix;
      if (!player.pseudo.empty()) {
        QRectF destRect;
        {
          QPixmap pix(50, 10);
          QPainter painter(&pix);
          painter.setFont(playerpseudofont);
          QRectF sourceRec(0.0, 0.0, 50.0, 10.0);
          destRect =
              painter.boundingRect(sourceRec, Qt::TextSingleLine,
                                   QString::fromStdString(player.pseudo));
        }
        int more = 0;
        if (player.type != CatchChallenger::Player_type_normal) more = 40;
        pix = QPixmap(destRect.width() + more, destRect.height());
        // draw the text & image
        {
          pix.fill(Qt::transparent);
          QPainter painter(&pix);
          painter.setFont(playerpseudofont);
          painter.drawText(
              QRectF(0.0, 0.0, destRect.width(), destRect.height()),
              Qt::TextSingleLine, QString::fromStdString(player.pseudo));
          if (player.type == CatchChallenger::Player_type_gm) {
            if (imgForPseudoAdmin == NULL)
              imgForPseudoAdmin = new QPixmap(":/CC/images/chat/admin.png");
            painter.drawPixmap(destRect.width(), (destRect.height() - 14) / 2,
                               40, 14, *imgForPseudoAdmin);
          }
          if (player.type == CatchChallenger::Player_type_dev) {
            if (imgForPseudoDev == NULL)
              imgForPseudoDev = new QPixmap(":/CC/images/chat/developer.png");
            painter.drawPixmap(destRect.width(), (destRect.height() - 14) / 2,
                               40, 14, *imgForPseudoDev);
          }
          if (player.type == CatchChallenger::Player_type_premium) {
            if (imgForPseudoPremium == NULL)
              imgForPseudoPremium = new QPixmap(":/CC/images/chat/premium.png");
            painter.drawPixmap(destRect.width(), (destRect.height() - 14) / 2,
                               40, 14, *imgForPseudoPremium);
          }
        }
      } else {
        if (player.type == CatchChallenger::Player_type_gm) {
          if (imgForPseudoAdmin == NULL)
            imgForPseudoAdmin = new QPixmap(":/CC/images/chat/admin.png");
          pix = *imgForPseudoAdmin;
        }
        if (player.type == CatchChallenger::Player_type_dev) {
          if (imgForPseudoDev == NULL)
            imgForPseudoDev = new QPixmap(":/CC/images/chat/developer.png");
          pix = *imgForPseudoDev;
        }
        if (player.type == CatchChallenger::Player_type_premium) {
          if (imgForPseudoPremium == NULL)
            imgForPseudoPremium = new QPixmap(":/CC/images/chat/premium.png");
          pix = *imgForPseudoPremium;
        }
      }
      if (!pix.isNull()) {
        otherPlayer.labelMapObject = new Tiled::MapObject();
        otherPlayer.labelMapObject->setName("labelMapObject");
        otherPlayer.labelTileset =
            new Tiled::Tileset(QString(), pix.width(), pix.height());
        otherPlayer.labelTileset->addTile(pix);
        Tiled::Cell cell = otherPlayer.labelMapObject->cell();
        cell.tile = otherPlayer.labelTileset->tileAt(0);
        otherPlayer.labelMapObject->setCell(cell);
      } else {
        otherPlayer.labelMapObject = NULL;
        otherPlayer.labelTileset = NULL;
      }
    }

    otherPlayer.current_map = mapPath;
    otherPlayer.presumed_map = all_map.at(mapPath);
    otherPlayer.current_monster_map = mapPath;

    switch (direction) {
      case CatchChallenger::Direction_look_at_top:
      case CatchChallenger::Direction_move_at_top: {
        Tiled::Cell cell = otherPlayer.playerMapObject->cell();
        cell.tile = otherPlayer.playerTileset->tileAt(1);
        otherPlayer.playerMapObject->setCell(cell);
      } break;
      case CatchChallenger::Direction_look_at_right:
      case CatchChallenger::Direction_move_at_right: {
        Tiled::Cell cell = otherPlayer.playerMapObject->cell();
        cell.tile = otherPlayer.playerTileset->tileAt(4);
        otherPlayer.playerMapObject->setCell(cell);
      } break;
      case CatchChallenger::Direction_look_at_bottom:
      case CatchChallenger::Direction_move_at_bottom: {
        Tiled::Cell cell = otherPlayer.playerMapObject->cell();
        cell.tile = otherPlayer.playerTileset->tileAt(7);
        otherPlayer.playerMapObject->setCell(cell);
      } break;
      case CatchChallenger::Direction_look_at_left:
      case CatchChallenger::Direction_move_at_left: {
        Tiled::Cell cell = otherPlayer.playerMapObject->cell();
        cell.tile = otherPlayer.playerTileset->tileAt(10);
        otherPlayer.playerMapObject->setCell(cell);
      } break;
      default:
        delete otherPlayer.playerMapObject;
        delete otherPlayer.playerTileset;
        qDebug() << QStringLiteral("The direction send by the server is wrong");
        return true;
    }

    updateOtherPlayerMonsterTile(otherPlayer, player.monsterId);
    loadOtherPlayerFromMap(otherPlayer, false);

    otherPlayer.animationDisplayed = false;
    otherPlayer.informations = player;
    otherPlayer.oneStepMore = new QTimer();
    otherPlayer.oneStepMore->setSingleShot(true);
    otherPlayer.moveAnimationTimer = new QTimer();
    otherPlayer.moveAnimationTimer->setSingleShot(true);
    otherPlayer.playerSpeed = player.speed;
    otherPlayerListByTimer[otherPlayer.oneStepMore] = player.simplifiedId;
    otherPlayerListByAnimationTimer[otherPlayer.moveAnimationTimer] =
        player.simplifiedId;
    if (!connect(otherPlayer.oneStepMore, &QTimer::timeout, this,
                 &MapControllerMP::moveOtherPlayerStepSlot))
      abort();
    if (!connect(otherPlayer.moveAnimationTimer, &QTimer::timeout, this,
                 &MapControllerMP::doMoveOtherAnimation))
      abort();
    otherPlayerList[player.simplifiedId] = otherPlayer;

    switch (direction) {
      case CatchChallenger::Direction_move_at_top:
      case CatchChallenger::Direction_move_at_right:
      case CatchChallenger::Direction_move_at_bottom:
      case CatchChallenger::Direction_move_at_left: {
        std::vector<std::pair<uint8_t, CatchChallenger::Direction> > movement;
        std::pair<uint8_t, CatchChallenger::Direction> move;
        move.first = 0;
        move.second = direction;
        movement.push_back(move);
        move_player_final(player.simplifiedId, movement, inReplayMode);
      } break;
      default:
        break;
    }
    finalOtherPlayerStep(otherPlayer);
    return true;
  }
  return true;
}

bool MapControllerMP::move_otherMonster(
    MapControllerMP::OtherPlayer &otherPlayer, const bool &haveMoved,
    const uint8_t &previous_different_x, const uint8_t &previous_different_y,
    const CatchChallenger::CommonMap *previous_different_map,
    CatchChallenger::Direction &previous_different_move,
    const std::vector<CatchChallenger::Direction> &lastMovedDirection) {
  if (otherPlayer.monsterMapObject == NULL)
    return false;
  else if (!otherPlayer.monsterMapObject->isVisible()) {
    otherPlayer.pendingMonsterMoves.clear();
    return false;
  } else if (otherPlayer.pendingMonsterMoves.size() > 1) {
    if (haveMoved) {
      otherPlayer.pendingMonsterMoves.clear();

      // detect last monster pos (player -1 pos)
      const bool mapChange =
          otherPlayer.current_monster_map != previous_different_map->map_file;
      if (mapChange)
        unloadOtherMonsterFromCurrentMap(otherPlayer);
      else {
        otherPlayer.monsterMapObject->setPosition(
            QPointF(otherPlayer.monster_x - 0.5, otherPlayer.monster_y + 1));
        MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)
            ->SetZValue(otherPlayer.monster_y);
      }
      otherPlayer.current_monster_map = previous_different_map->map_file;
      otherPlayer.monster_x = previous_different_x;
      otherPlayer.monster_y = previous_different_y;
      if (mapChange) loadOtherMonsterFromCurrentMap(otherPlayer);
      // detect last monster orientation (player -1 move)
      if (lastMovedDirection.size() > 1)
        previous_different_move =
            lastMovedDirection.at(lastMovedDirection.size() - 1 /*last item*/ -
                                  1 /*before the last item*/);
      // do the next move
      if (lastMovedDirection.size() > 1)
        otherPlayer.pendingMonsterMoves.push_back(
            lastMovedDirection.at(lastMovedDirection.size() - 1 /*last item*/));

      return true;
    } else {
      while (otherPlayer.pendingMonsterMoves.size() > 1)
        otherPlayer.pendingMonsterMoves.erase(
            otherPlayer.pendingMonsterMoves.cbegin());
      return false;
    }
  }
  return false;
}

bool MapControllerMP::move_player_final(
    const uint16_t &id,
    const std::vector<std::pair<uint8_t, CatchChallenger::Direction> >
        &movement,
    bool inReplayMode) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
    if (!inReplayMode) {
      DelayedMove tempItem;
      tempItem.id = id;
      tempItem.movement = movement;

      DelayedMultiplex multiplex;
      multiplex.move = tempItem;
      multiplex.type = DelayedType_Move;
      multiplex.insert.direction =
          CatchChallenger::Direction::Direction_look_at_top;
      multiplex.insert.mapId = 0;
      multiplex.insert.x = 0;
      multiplex.insert.y = 0;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
  if (id ==
      client->get_player_informations_ro().public_informations.simplifiedId) {
    qDebug() << "The current player can't be moved (only teleported)";
    return true;
  }
  if (otherPlayerList.find(id) == otherPlayerList.cend()) {
    qDebug()
        << QStringLiteral("Other player (%1) not loaded on the map").arg(id);
    return true;
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  QStringList moveString;
  int index_temp = 0;
  while (index_temp < movement.size()) {
    std::pair<uint8_t, CatchChallenger::Direction> move =
        movement.at(index_temp);
    moveString << QStringLiteral("{%1,%2}")
                      .arg(move.first)
                      .arg(CatchChallenger::MoveOnTheMap::directionToString(
                          move.second));
    index_temp++;
  }

  qDebug() << QStringLiteral("move_player(%1,%2), previous direction: %3")
                  .arg(id)
                  .arg(moveString.join(";"))
                  .arg(CatchChallenger::MoveOnTheMap::directionToString(
                      otherPlayerList.value(id).direction));
#endif

  // qDebug() << "MapControllerMP::move_player_final";

  // reset to the good position
  OtherPlayer &otherPlayer = otherPlayerList[id];
  otherPlayer.oneStepMore->stop();
  otherPlayer.inMove = false;
  otherPlayer.moveStep = 0;
  while (otherPlayer.pendingMonsterMoves.size() > 1)
    otherPlayer.pendingMonsterMoves.erase(
        otherPlayer.pendingMonsterMoves.cbegin());

  if (otherPlayer.current_map !=
      otherPlayer.presumed_map->logicalMap.map_file) {
    unloadOtherPlayerFromMap(otherPlayer);
    std::string mapPath = otherPlayer.current_map;
    if (!haveMapInMemory(mapPath)) {
      qDebug() << QStringLiteral("move_player(%1), map not already loaded")
                      .arg(id)
                      .arg(QString::fromStdString(otherPlayer.current_map));
      if (!inReplayMode) {
        DelayedMove tempItem;
        tempItem.id = id;
        tempItem.movement = movement;

        DelayedMultiplex multiplex;
        multiplex.move = tempItem;
        multiplex.type = DelayedType_Move;
        multiplex.insert.direction =
            CatchChallenger::Direction::Direction_look_at_top;
        multiplex.insert.mapId = 0;
        multiplex.insert.x = 0;
        multiplex.insert.y = 0;
        delayedActions.push_back(multiplex);
      }
      return false;
    }
    loadOtherMap(mapPath);
    otherPlayer.presumed_map = all_map.at(mapPath);
    loadOtherPlayerFromMap(otherPlayer);
  }
  uint8_t x = otherPlayer.x;
  uint8_t y = otherPlayer.y;
  otherPlayer.presumed_x = x;
  otherPlayer.presumed_y = y;
  CatchChallenger::CommonMap *map = &otherPlayer.presumed_map->logicalMap;
  CatchChallenger::CommonMap *old_map;

  bool haveMoved = false;
  uint8_t previous_different_x = x;
  uint8_t previous_different_y = y;
  CatchChallenger::CommonMap *previous_different_map = map;
  CatchChallenger::Direction previous_different_move =
      otherPlayer.presumed_direction;
  std::vector<CatchChallenger::Direction> lastMovedDirection;
  // move to have the new position if needed
  unsigned int index = 0;
  while (index < movement.size()) {
    // var tho use
    std::pair<uint8_t, CatchChallenger::Direction> move = movement.at(index);
    // qDebug() << "MapControllerMP::move_player_final" << move.first << ", " <<
    // move.second;

    // set the direction
    otherPlayer.direction = move.second;
    otherPlayer.presumed_direction = move.second;

    // do the moves
    int index2 = 0;
    while (index2 < move.first) {
      haveMoved = true;
      old_map = map;
      // set the final value (direction, position, ...)
      switch (otherPlayer.presumed_direction) {
        case CatchChallenger::Direction_move_at_left:
        case CatchChallenger::Direction_move_at_right:
        case CatchChallenger::Direction_move_at_top:
        case CatchChallenger::Direction_move_at_bottom:
          if (CatchChallenger::MoveOnTheMap::canGoTo(
                  otherPlayer.presumed_direction, *map, x, y, true)) {
            previous_different_x = x;
            previous_different_y = y;
            previous_different_map = map;
            lastMovedDirection.push_back(otherPlayer.presumed_direction);

            CatchChallenger::MoveOnTheMap::move(otherPlayer.presumed_direction,
                                                &map, &x, &y);
          } else {
            qDebug()
                << QStringLiteral("move_player(): at %1(%2,%3) can't go to %4")
                       .arg(QString::fromStdString(map->map_file))
                       .arg(x)
                       .arg(y)
                       .arg(QString::fromStdString(
                           CatchChallenger::MoveOnTheMap::directionToString(
                               otherPlayer.presumed_direction)));
            return true;
          }
          break;
        default:
          qDebug() << QStringLiteral(
                          "move_player(): moveStep: %1, wrong direction: %2")
                          .arg(move.first)
                          .arg(QString::fromStdString(
                              CatchChallenger::MoveOnTheMap::directionToString(
                                  otherPlayer.presumed_direction)));
          move_otherMonster(otherPlayer, haveMoved, previous_different_x,
                            previous_different_y, previous_different_map,
                            previous_different_move, lastMovedDirection);
          break;
      }
      // if the map have changed
      if (old_map != map) {
        loadOtherMap(map->map_file);
        if (all_map.find(map->map_file) == all_map.cend())
          qDebug() << QStringLiteral("map changed not located: %1")
                          .arg(QString::fromStdString(map->map_file));
        else {
          unloadOtherPlayerFromMap(otherPlayer);
          otherPlayer.presumed_map = all_map.at(map->map_file);
          loadOtherPlayerFromMap(otherPlayer);
        }
      }
      index2++;
    }
    index++;
  }

  move_otherMonster(otherPlayer, haveMoved, previous_different_x,
                    previous_different_y, previous_different_map,
                    previous_different_move, lastMovedDirection);

  // set the new variables
  otherPlayer.current_map = map->map_file;
  otherPlayer.x = x;
  otherPlayer.y = y;

  otherPlayer.presumed_map = all_map.at(otherPlayer.current_map);
  otherPlayer.presumed_x = otherPlayer.x;
  otherPlayer.presumed_y = otherPlayer.y;
  otherPlayer.presumed_direction = otherPlayer.direction;

  // move to the final position (integer), y+1 because the tile lib start y to
  // 1, not 0
  otherPlayer.playerMapObject->setPosition(
      QPointF(otherPlayer.presumed_x, otherPlayer.presumed_y + 1));
  if (otherPlayer.labelMapObject != NULL) {
    otherPlayerList.at(id).labelMapObject->setPosition(QPointF(
        static_cast<qreal>(otherPlayer.presumed_x) -
            static_cast<qreal>(otherPlayer.labelTileset->tileWidth()) / 2 / 16 +
            0.5,
        static_cast<qreal>(otherPlayer.presumed_y) + 1 - 1.4));
    MapObjectItem::objectLink.at(otherPlayer.labelMapObject)
        ->SetZValue(otherPlayer.presumed_y);
  }
  if (MapObjectItem::objectLink.find(otherPlayer.playerMapObject) !=
      MapObjectItem::objectLink.cend())
    MapObjectItem::objectLink.at(otherPlayer.playerMapObject)
        ->SetZValue(otherPlayer.presumed_y);

  // start moving into the right direction
  switch (otherPlayer.presumed_direction) {
    case CatchChallenger::Direction_look_at_top:
    case CatchChallenger::Direction_move_at_top: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(1);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_right:
    case CatchChallenger::Direction_move_at_right: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(4);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_bottom:
    case CatchChallenger::Direction_move_at_bottom: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(7);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_left:
    case CatchChallenger::Direction_move_at_left: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(10);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    default:
      qDebug() << QStringLiteral(
                      "move_player(): player: %1 (%2), wrong direction: %3")
                      .arg(QString::fromStdString(
                          otherPlayer.informations.pseudo))
                      .arg(id)
                      .arg(otherPlayer.presumed_direction);
      return true;
  }
  switch (otherPlayer.presumed_direction) {
    case CatchChallenger::Direction_move_at_top:
    case CatchChallenger::Direction_move_at_right:
    case CatchChallenger::Direction_move_at_bottom:
    case CatchChallenger::Direction_move_at_left:
      otherPlayer.oneStepMore->start(otherPlayer.informations.speed / 5);
      break;
    default:
      break;
  }
  finalOtherPlayerStep(otherPlayer);
  return true;
}

bool MapControllerMP::remove_player_final(const uint16_t &id,
                                          bool inReplayMode) {
  if (id ==
      client->get_player_informations_ro().public_informations.simplifiedId) {
    qDebug() << "The current player can't be removed";
    return true;
  }
  if (!mHaveTheDatapack || !player_informations_is_set) {
#ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("delayed: MapControllerMP::remove_player(%1)")
                    .arg(id);
#endif
    if (!inReplayMode) {
      DelayedMultiplex multiplex;
      multiplex.remove = id;
      multiplex.type = DelayedType_Remove;
      multiplex.insert.direction =
          CatchChallenger::Direction::Direction_look_at_top;
      multiplex.insert.mapId = 0;
      multiplex.insert.x = 0;
      multiplex.insert.y = 0;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
  if (otherPlayerList.find(id) == otherPlayerList.cend()) {
    qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
    return true;
  }
  {
    unsigned int index = 0;
    while (index < delayedActions.size()) {
      switch (delayedActions.at(index).type) {
        case DelayedType_Insert:
          if (delayedActions.at(index).insert.player.simplifiedId == id)
            delayedActions.erase(delayedActions.cbegin() + index);
          else
            index++;
          break;
        case DelayedType_Move:
          if (delayedActions.at(index).move.id == id)
            delayedActions.erase(delayedActions.cbegin() + index);
          else
            index++;
          break;
        default:
          index++;
          break;
      }
    }
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  qDebug() << QStringLiteral("remove_player(%1)").arg(id);
#endif
  OtherPlayer &otherPlayer = otherPlayerList[id];
  unloadOtherPlayerFromMap(otherPlayer);
  if (otherPlayer.monsterMapObject != NULL)
    unloadOtherMonsterFromCurrentMap(otherPlayer);

  otherPlayerListByTimer.erase(otherPlayer.oneStepMore);
  otherPlayerListByAnimationTimer.erase(otherPlayer.moveAnimationTimer);

  Tiled::ObjectGroup *currentGroup = otherPlayer.playerMapObject->objectGroup();
  if (currentGroup != NULL) {
    if (ObjectGroupItem::objectGroupLink.find(currentGroup) !=
        ObjectGroupItem::objectGroupLink.cend()) {
      ObjectGroupItem::objectGroupLink.at(currentGroup)
          ->removeObject(otherPlayer.playerMapObject);
      if (otherPlayer.labelMapObject != NULL)
        ObjectGroupItem::objectGroupLink.at(currentGroup)
            ->removeObject(otherPlayer.labelMapObject);
    }
    if (currentGroup != otherPlayer.presumed_map->objectGroup)
      qDebug() << QStringLiteral(
                      "loadOtherPlayerFromMap(), the playerMapObject group is "
                      "wrong: %1")
                      .arg(currentGroup->name());
    currentGroup->removeObject(otherPlayer.playerMapObject);
    if (otherPlayer.labelMapObject != NULL)
      currentGroup->removeObject(otherPlayer.labelMapObject);
  }

  /*delete otherPlayer.playerMapObject;
  delete otherPlayer.playerTileset;*/
  if (otherPlayer.monsterMapObject != NULL) delete otherPlayer.monsterMapObject;
  if (otherPlayer.oneStepMore != NULL) delete otherPlayer.oneStepMore;
  otherPlayer.oneStepMore = NULL;
  if (otherPlayer.moveAnimationTimer != NULL)
    /*delete */ otherPlayer.moveAnimationTimer->deleteLater();
  otherPlayer.moveAnimationTimer = NULL;
  if (otherPlayer.labelMapObject != NULL) delete otherPlayer.labelMapObject;
  if (otherPlayer.labelTileset != NULL) delete otherPlayer.labelTileset;

  otherPlayerList.erase(id);
  return true;
}

bool MapControllerMP::reinsert_player_final(
    const uint16_t &id, const uint8_t &x, const uint8_t &y,
    const CatchChallenger::Direction &direction, bool inReplayMode) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
#ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("delayed: MapControllerMP::reinsert_player(%1)")
                    .arg(id);
#endif
    if (!inReplayMode) {
      DelayedReinsertSingle tempItem;
      tempItem.id = id;
      tempItem.x = x;
      tempItem.y = y;
      tempItem.direction = direction;

      DelayedMultiplex multiplex;
      multiplex.reinsert_single = tempItem;
      multiplex.type = DelayedType_Reinsert_single;
      multiplex.insert.direction =
          CatchChallenger::Direction::Direction_look_at_top;
      multiplex.insert.mapId = 0;
      multiplex.insert.x = 0;
      multiplex.insert.y = 0;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
  if (id ==
      client->get_player_informations_ro().public_informations.simplifiedId) {
    qDebug() << "The current player can't be removed";
    return true;
  }
  if (otherPlayerList.find(id) == otherPlayerList.cend()) {
    qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
    return true;
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
#endif

  /// \warning search by loop because otherPlayerList.value(id).current_map is
  /// the full path, QtDatapackClientLoader::GetInstance()->maps relative path
  std::string tempCurrentMap = otherPlayerList.at(id).current_map;
  // if not found, search into sub
  if (all_map.find(tempCurrentMap) == all_map.cend() &&
      !client->subDatapackCode().empty()) {
    std::string tempCurrentMapSub = tempCurrentMap;
    stringreplaceOne(tempCurrentMapSub, client->datapackPathSub(), "");
    if (all_map.find(tempCurrentMapSub) != all_map.cend())
      tempCurrentMap = tempCurrentMapSub;
  }
  // if not found, search into main
  if (all_map.find(tempCurrentMap) == all_map.cend()) {
    std::string tempCurrentMapMain = tempCurrentMap;
    stringreplaceOne(tempCurrentMapMain, client->datapackPathMain(), "");
    if (all_map.find(tempCurrentMapMain) != all_map.cend())
      tempCurrentMap = tempCurrentMapMain;
  }
  // if remain not found
  if (all_map.find(tempCurrentMap) == all_map.cend()) {
    qDebug() << "internal problem, revert map ("
             << QString::fromStdString(otherPlayerList.at(id).current_map)
             << ") index is wrong ("
             << QString::fromStdString(stringimplode(
                    QtDatapackClientLoader::GetInstance()->get_maps(), ";"))
             << ")";
    if (!inReplayMode) {
      DelayedReinsertSingle tempItem;
      tempItem.id = id;
      tempItem.x = x;
      tempItem.y = y;
      tempItem.direction = direction;

      DelayedMultiplex multiplex;
      multiplex.reinsert_single = tempItem;
      multiplex.type = DelayedType_Reinsert_single;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
  const uint32_t mapId = (uint32_t)all_map.at(tempCurrentMap)->logicalMap.id;
  if (mapId == 0) qDebug() << QStringLiteral("supected NULL map then error");

  OtherPlayer &otherPlayer = otherPlayerList[id];
  if (otherPlayer.x == x && otherPlayer.y == y &&
      otherPlayer.direction == direction) {
    /*error("supected bug at insert: tempPlayer.x
    "+std::to_string(otherPlayer.x)+" == x  "+std::to_string(x)+ "  &&
    tempPlayer.y "+std::to_string(otherPlayer.y)+" == y "+std::to_string(y)+" &&
    tempPlayer.direction "+ std::to_string(otherPlayer.direction)+" == direction
    "+std::to_string(direction)); return false;*/
    return true;
  }

  // set the player coord
  uint8_t old_tempPlayer_x = otherPlayer.x;
  uint8_t old_tempPlayer_y = otherPlayer.y;
  otherPlayer.presumed_x = static_cast<uint8_t>(x);
  otherPlayer.presumed_y = static_cast<uint8_t>(y);
  otherPlayer.x = static_cast<uint8_t>(x);
  otherPlayer.y = static_cast<uint8_t>(y);
  otherPlayer.presumed_direction = direction;
  otherPlayer.direction = direction;
  otherPlayer.inMove = false;
  otherPlayer.moveStep = 0;
  while (otherPlayer.pendingMonsterMoves.size() > 1)
    otherPlayer.pendingMonsterMoves.erase(
        otherPlayer.pendingMonsterMoves.cbegin());

  switch (direction) {
    case CatchChallenger::Direction_look_at_top:
    case CatchChallenger::Direction_move_at_top: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(1);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_right:
    case CatchChallenger::Direction_move_at_right: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(4);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_bottom:
    case CatchChallenger::Direction_move_at_bottom: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(7);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    case CatchChallenger::Direction_look_at_left:
    case CatchChallenger::Direction_move_at_left: {
      Tiled::Cell cell = otherPlayer.playerMapObject->cell();
      cell.tile = otherPlayer.playerTileset->tileAt(10);
      otherPlayer.playerMapObject->setCell(cell);
    } break;
    default:
      delete otherPlayer.playerMapObject;
      delete otherPlayer.playerTileset;
      qDebug() << QStringLiteral("The direction send by the server is wrong");
      return true;
  }

  if (otherPlayer.playerMapObject != NULL) {
    otherPlayer.playerMapObject->setPosition(
        QPointF(otherPlayer.x, otherPlayer.y + 1));
    if (MapObjectItem::objectLink.find(otherPlayer.playerMapObject) !=
        MapObjectItem::objectLink.cend())
      MapObjectItem::objectLink.at(otherPlayer.playerMapObject)
          ->SetZValue(otherPlayer.y);
    if (otherPlayer.labelMapObject != NULL)
      otherPlayer.labelMapObject->setPosition(QPointF(
          static_cast<qreal>(x) -
              static_cast<qreal>(otherPlayer.labelTileset->tileWidth()) / 2 /
                  16 +
              0.5,
          y + 1 - 1.4));
  }

  if (old_tempPlayer_x == x || old_tempPlayer_y == y) {
    /// \warning no path finding because too many player update can overflow the
    /// cpu
    int moveOffset = 0;
    if (old_tempPlayer_x == x) {
      if (y < old_tempPlayer_y) {
        otherPlayer.pendingMonsterMoves.clear();
        otherPlayer.pendingMonsterMoves.push_back(
            CatchChallenger::Direction_move_at_top);
        uint8_t temp_x = x;
        uint8_t temp_y = y;
        CatchChallenger::CommonMap *map =
            &all_map.at(tempCurrentMap)->logicalMap;
        CatchChallenger::CommonMap *new_map = map;
        if (CatchChallenger::MoveOnTheMap::move(
                CatchChallenger::Direction_move_at_bottom, &new_map, &temp_x,
                &temp_y, true, false)) {
          otherPlayer.current_monster_map = new_map->map_file;
          otherPlayer.monster_x = temp_x;
          otherPlayer.monster_y = temp_y;
          if (otherPlayer.monsterMapObject != NULL) {
            Tiled::Cell cell = otherPlayer.monsterMapObject->cell();
            cell.tile = otherPlayer.monsterTileset->tileAt(2);
            otherPlayer.monsterMapObject->setCell(cell);
            otherPlayer.monsterMapObject->setVisible(true);
            if (map != new_map) unloadOtherMonsterFromCurrentMap(otherPlayer);
            otherPlayer.monsterMapObject->setPosition(
                QPointF((float)otherPlayer.monster_x - 0.5,
                        (float)otherPlayer.monster_y + 1));
            MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)
                ->SetZValue(otherPlayer.monster_y);
            if (map != new_map) loadOtherMonsterFromCurrentMap(otherPlayer);
          }
        }
        moveOffset = old_tempPlayer_y - y;
      } else if (old_tempPlayer_y < y) {
        otherPlayer.pendingMonsterMoves.clear();
        otherPlayer.pendingMonsterMoves.push_back(
            CatchChallenger::Direction_move_at_bottom);
        uint8_t temp_x = x;
        uint8_t temp_y = y;
        CatchChallenger::CommonMap *map =
            &all_map.at(tempCurrentMap)->logicalMap;
        CatchChallenger::CommonMap *new_map = map;
        if (CatchChallenger::MoveOnTheMap::move(
                CatchChallenger::Direction_move_at_top, &new_map, &temp_x,
                &temp_y, true, false)) {
          otherPlayer.current_monster_map = new_map->map_file;
          otherPlayer.monster_x = temp_x;
          otherPlayer.monster_y = temp_y;
          if (otherPlayer.monsterMapObject != NULL) {
            Tiled::Cell cell = otherPlayer.monsterMapObject->cell();
            cell.tile = otherPlayer.monsterTileset->tileAt(6);
            otherPlayer.monsterMapObject->setCell(cell);
            otherPlayer.monsterMapObject->setVisible(true);
            if (map != new_map) unloadOtherMonsterFromCurrentMap(otherPlayer);
            otherPlayer.monsterMapObject->setPosition(
                QPointF((float)otherPlayer.monster_x - 0.5,
                        (float)otherPlayer.monster_y + 1));
            MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)
                ->SetZValue(otherPlayer.monster_y);
            if (map != new_map) loadOtherMonsterFromCurrentMap(otherPlayer);
          }
        }
        moveOffset = y - old_tempPlayer_y;
      }
    } else if (old_tempPlayer_y == y) {
      if (x < old_tempPlayer_x) {
        otherPlayer.pendingMonsterMoves.clear();
        otherPlayer.pendingMonsterMoves.push_back(
            CatchChallenger::Direction_move_at_left);
        uint8_t temp_x = x;
        uint8_t temp_y = y;
        CatchChallenger::CommonMap *map =
            &all_map.at(tempCurrentMap)->logicalMap;
        CatchChallenger::CommonMap *new_map = map;
        if (CatchChallenger::MoveOnTheMap::move(
                CatchChallenger::Direction_move_at_right, &new_map, &temp_x,
                &temp_y, true, false)) {
          otherPlayer.current_monster_map = new_map->map_file;
          otherPlayer.monster_x = temp_x;
          otherPlayer.monster_y = temp_y;
          if (otherPlayer.monsterMapObject != NULL) {
            Tiled::Cell cell = otherPlayer.monsterMapObject->cell();
            cell.tile = otherPlayer.monsterTileset->tileAt(3);
            otherPlayer.monsterMapObject->setCell(cell);
            otherPlayer.monsterMapObject->setVisible(true);
            if (map != new_map) unloadOtherMonsterFromCurrentMap(otherPlayer);
            otherPlayer.monsterMapObject->setPosition(
                QPointF((float)otherPlayer.monster_x - 0.5,
                        (float)otherPlayer.monster_y + 1));
            MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)
                ->SetZValue(otherPlayer.monster_y);
            if (map != new_map) loadOtherMonsterFromCurrentMap(otherPlayer);
          }
        }
        moveOffset = old_tempPlayer_x - x;
      } else if (old_tempPlayer_x < x) {
        otherPlayer.pendingMonsterMoves.clear();
        otherPlayer.pendingMonsterMoves.push_back(
            CatchChallenger::Direction_move_at_right);
        uint8_t temp_x = x;
        uint8_t temp_y = y;
        CatchChallenger::CommonMap *map =
            &all_map.at(tempCurrentMap)->logicalMap;
        CatchChallenger::CommonMap *new_map = map;
        if (CatchChallenger::MoveOnTheMap::move(
                CatchChallenger::Direction_move_at_left, &new_map, &temp_x,
                &temp_y, true, false)) {
          otherPlayer.current_monster_map = new_map->map_file;
          otherPlayer.monster_x = temp_x;
          otherPlayer.monster_y = temp_y;
          if (otherPlayer.monsterMapObject != NULL) {
            Tiled::Cell cell = otherPlayer.monsterMapObject->cell();
            cell.tile = otherPlayer.monsterTileset->tileAt(7);
            otherPlayer.monsterMapObject->setCell(cell);
            otherPlayer.monsterMapObject->setVisible(true);
            if (map != new_map) unloadOtherMonsterFromCurrentMap(otherPlayer);
            otherPlayer.monsterMapObject->setPosition(
                QPointF((float)otherPlayer.monster_x - 0.5,
                        (float)otherPlayer.monster_y + 1));
            MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)
                ->SetZValue(otherPlayer.monster_y);
            if (map != new_map) loadOtherMonsterFromCurrentMap(otherPlayer);
          }
        }
        moveOffset = x - old_tempPlayer_x;
      }
    } else
      abort();

    switch (direction) {
      case CatchChallenger::Direction_move_at_left:
      case CatchChallenger::Direction_move_at_right:
      case CatchChallenger::Direction_move_at_top:
      case CatchChallenger::Direction_move_at_bottom: {
        if (!otherPlayer.pendingMonsterMoves.empty()) {
          std::vector<std::pair<uint8_t, CatchChallenger::Direction> > movement;
          std::pair<uint8_t, CatchChallenger::Direction> t(moveOffset,
                                                           direction);
          movement.push_back(t);
          // apply the x,y change: if(move_player_final(id,movement,false))
          {
            finalOtherPlayerStep(otherPlayer);
            otherPlayer.oneStepMore->start(otherPlayer.informations.speed / 5);
            return true;
          }
        }
      } break;
      case CatchChallenger::Direction_look_at_left:
      case CatchChallenger::Direction_look_at_right:
      case CatchChallenger::Direction_look_at_top:
      case CatchChallenger::Direction_look_at_bottom:
        finalOtherPlayerStep(otherPlayer);
        return true;
        break;
      default:
        break;
    }
  } else
    qDebug() << "move: !x && !y";

  finalOtherPlayerStep(otherPlayer);
  switch (direction) {
    case CatchChallenger::Direction_move_at_top:
    case CatchChallenger::Direction_move_at_right:
    case CatchChallenger::Direction_move_at_bottom:
    case CatchChallenger::Direction_move_at_left:
      otherPlayer.oneStepMore->start(otherPlayer.informations.speed / 5);
      break;
    default:
      break;
  }

  // monster
  otherPlayer.monster_x = static_cast<uint8_t>(x);
  otherPlayer.monster_y = static_cast<uint8_t>(y);
  if (otherPlayer.monsterMapObject != NULL) {
    otherPlayer.monsterMapObject->setPosition(
        QPointF((float)x - 0.5, (float)y + 1));
    MapObjectItem::objectLink.at(otherPlayer.monsterMapObject)->SetZValue(y);
    otherPlayer.monsterMapObject->setVisible(false);
  }

  return true;
}

bool MapControllerMP::full_reinsert_player_final(
    const uint16_t &id, const uint32_t &mapId, const uint8_t &x,
    const uint8_t &y, const CatchChallenger::Direction &direction,
    bool inReplayMode) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
#ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral("delayed: MapControllerMP::reinsert_player(%1)")
                    .arg(id);
#endif
    if (!inReplayMode) {
      DelayedReinsertFull tempItem;
      tempItem.id = id;
      tempItem.mapId = mapId;
      tempItem.x = x;
      tempItem.y = y;
      tempItem.direction = direction;

      DelayedMultiplex multiplex;
      multiplex.reinsert_full = tempItem;
      multiplex.insert.direction =
          CatchChallenger::Direction::Direction_look_at_top;
      multiplex.insert.mapId = 0;
      multiplex.insert.x = 0;
      multiplex.insert.y = 0;
      multiplex.type = DelayedType_Reinsert_full;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
  if (id ==
      client->get_player_informations_ro().public_informations.simplifiedId) {
    qDebug() << "The current player can't be removed";
    return true;
  }
  if (otherPlayerList.find(id) == otherPlayerList.cend()) {
    qDebug() << QStringLiteral("Other player (%1) not exists").arg(id);
    return true;
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  qDebug() << QStringLiteral("reinsert_player(%1)").arg(id);
#endif

  CatchChallenger::Player_public_informations informations =
      otherPlayerList.at(id).informations;
  remove_player_final(id, inReplayMode);
  insert_player_final(informations, mapId, x, y, direction, inReplayMode);
  return true;
}

bool MapControllerMP::dropAllPlayerOnTheMap_final(bool inReplayMode) {
  if (!mHaveTheDatapack || !player_informations_is_set) {
#ifdef DEBUG_CLIENT_LOAD_ORDER
    qDebug() << QStringLiteral(
        "delayed: MapControllerMP::dropAllPlayerOnTheMap()");
#endif
    if (!inReplayMode) {
      DelayedMultiplex multiplex;
      multiplex.type = DelayedType_Drop_all;
      multiplex.insert.direction =
          CatchChallenger::Direction::Direction_look_at_top;
      multiplex.insert.mapId = 0;
      multiplex.insert.x = 0;
      multiplex.insert.y = 0;
      delayedActions.push_back(multiplex);
    }
    return false;
  }
#ifdef DEBUG_CLIENT_PLAYER_ON_MAP
  qDebug() << QStringLiteral("dropAllPlayerOnTheMap()");
#endif
  std::vector<uint16_t> temIdList;
  for (const auto &n : otherPlayerList) temIdList.push_back(n.first);
  unsigned int index = 0;
  while (index < temIdList.size()) {
    remove_player_final(temIdList.at(index), inReplayMode);
    index++;
  }
  return true;
}
