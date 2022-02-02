// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_CCMAP_HPP_
#define CLIENT_V3_SCENES_MAP_CCMAP_HPP_

#include <QObject>
#include <vector>

#include "../../core/Scene.hpp"
#include "../../entities/render/MapController.hpp"

namespace Scenes {
class CCMap : public QObject, public Scene {
  Q_OBJECT

 public:
  static CCMap *Create();
  MapController mapController;

  void PaintChildItems(std::vector<Node *> childItems, qreal parentX,
                       qreal parentY, QPainter *painter,
                       const QStyleOptionGraphicsItem *option,
                       QWidget *widget = nullptr);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;
  void OnEnter() override;
  void OnExit() override;

 private:
  QPointF lastClickedPos;
  bool clicked;

  qreal scale;
  qreal x;
  qreal y;
  CCMap();

 signals:
  void pathFindingNotFound();
  void repelEffectIsOver() const;
  void teleportConditionNotRespected(const std::string &text) const;
  void send_player_direction(const CatchChallenger::Direction &the_direction);
  void stopped_in_front_of(CatchChallenger::Map_client *map, const uint8_t &x,
                           const uint8_t &y);
  void actionOn(CatchChallenger::Map_client *map, const uint8_t &x,
                const uint8_t &y);
  void actionOnNothing();
  void blockedOn(const MapVisualiserPlayer::BlockedOn &blockOnVar);
  void wildFightCollision(CatchChallenger::Map_client *map, const uint8_t &x,
                          const uint8_t &y);
  void botFightCollision(CatchChallenger::Map_client *map, const uint8_t &x,
                         const uint8_t &y, const uint16_t &fightId);
  void currentMapLoaded();
  void errorWithTheCurrentMap();
  void error(const std::string &error);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_CCMAP_HPP_
