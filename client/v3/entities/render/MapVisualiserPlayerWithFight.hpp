#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include <QObject>

#include "MapVisualiserPlayer.hpp"

class MapVisualiserPlayerWithFight : public MapVisualiserPlayer {
  Q_OBJECT
 public:
  ~MapVisualiserPlayerWithFight();
  void addRepelStep(const uint32_t &repel_step);
 protected slots:
  virtual void keyPressParse();
  virtual bool haveStopTileAction();
  virtual bool canGoTo(const CatchChallenger::Direction &direction,
                       CatchChallenger::CommonMap map, COORD_TYPE x,
                       COORD_TYPE y, const bool &checkCollision);
  virtual void resetAll();

 protected:
  uint32_t repel_step;
  Tiled::Tileset *fightCollisionBot;
  explicit MapVisualiserPlayerWithFight(const bool &debugTags = false);

 signals:
  void repelEffectIsOver() const;
  void teleportConditionNotRespected(const std::string &text) const;
};

#endif
