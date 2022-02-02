// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_MONSTERTHUMB_HPP_
#define CLIENT_V3_SCENES_MAP_MONSTERTHUMB_HPP_

#include "../../core/Node.hpp"
#include "../../entities/PlayerInfo.hpp"
#include "MapMonsterPreview.hpp"
#include "MonsterDetails.hpp"

namespace Scenes {
class MonsterThumb : public QObject, public Node {
  Q_OBJECT
 public:
  static MonsterThumb *Create(Node *parent);
  ~MonsterThumb();

  void LoadMonsters(
      const CatchChallenger::Player_private_and_public_informations
          &informations);
  void OpenMonster(Node *item);
  void OnDropMonster(Node *item);
  void Reset();

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void OnResize() override;
  void OnEnter() override;
  void OnExit() override;

 private:
  MonsterThumb(Node *parent);
  void OnUpdateInfo(PlayerInfo *player_info);

  std::vector<MapMonsterPreview *> monsters_;
  QPointF m_startPress;
  bool was_dragged_;
  MapMonsterPreview *monsters_dragged_;
  MonsterDetails *monster_details_;
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_MONSTERTHUMB_HPP_
