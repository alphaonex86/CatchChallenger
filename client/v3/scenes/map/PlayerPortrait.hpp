// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_PLAYERPORTRAIT_HPP_
#define CLIENT_V3_SCENES_MAP_PLAYERPORTRAIT_HPP_

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../core/Node.hpp"
#include "../../entities/PlayerInfo.hpp"

namespace Scenes {
class PlayerPortrait : public QObject, public Node {
  Q_OBJECT
 public:
  static PlayerPortrait *Create(Node *parent);
  void SetInformation(
      const CatchChallenger::Player_private_and_public_informations
          &informations);
  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 private:
  QString image_;
  QString name_;
  QString cash_;

  PlayerPortrait(Node *parent);
  void OnUpdateInfo(PlayerInfo *info);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_PLAYERPORTRAIT_HPP_
