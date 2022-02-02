// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENE_MAP_MAPMONSTERPREVIEW_HPP_
#define CLIENT_V3_SCENE_MAP_MAPMONSTERPREVIEW_HPP_

#include "../../../../general/base/GeneralStructures.hpp"
#include "../../core/Node.hpp"

namespace Scenes {
class MapMonsterPreview : public Node {
 public:
  ~MapMonsterPreview();
  static MapMonsterPreview *Create(const CatchChallenger::PlayerMonster &m,
                                   int index, Node *parent = nullptr);
  bool IsPressed();
  const CatchChallenger::PlayerMonster &GetMonster() const;
  void DrawContent();
  int Index();
  void SetOnDrop(std::function<void(Node *)> callback);

  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 private:
  CatchChallenger::PlayerMonster monster_;
  bool pressed_;
  bool drag_;
  QPixmap cache_;
  int index_;
  std::function<void(Node *)> on_drop_;

  MapMonsterPreview(const CatchChallenger::PlayerMonster &m, int index,
                    Node *parent = nullptr);
  void SetPressed(const bool &pressed);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENE_MAP_MAPMONSTERPREVIEW_HPP_
