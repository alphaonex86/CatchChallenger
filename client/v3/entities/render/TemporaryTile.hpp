// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_RENDER_TEMPORARYTILE_HPP_
#define CLIENT_V3_ENTITIES_RENDER_TEMPORARYTILE_HPP_

#include <QObject>
#include <QTimer>

#include "../../libraries/tiled/tiled_mapobject.hpp"
#include "../../libraries/tiled/tiled_tile.hpp"
#include "../../libraries/tiled/tiled_tileset.hpp"

class TemporaryTile : public QObject {
  Q_OBJECT

 public:
  explicit TemporaryTile(Tiled::MapObject *object);
  ~TemporaryTile();
  void startAnimation(Tiled::Tile *tile, const uint32_t &ms,
                      const uint8_t &count);
  static Tiled::Tile *empty;

 private:
  Tiled::MapObject *object;
  int index;
  uint8_t count;
  QTimer timer;

 private:
  void updateTheTile();
};

#endif  // CLIENT_V3_ENTITIES_RENDER_TEMPORARYTILE_HPP_
