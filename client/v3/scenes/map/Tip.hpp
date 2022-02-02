// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_MAP_TIP_HPP_
#define CLIENT_V3_SCENES_MAP_TIP_HPP_

#include "../../core/Node.hpp"
#include "../../core/Sprite.hpp"
#include "../../ui/ThemedLabel.hpp"

namespace Scenes {
class Tip : public Node {
 public:
  static Tip *Create(Node *parent);
  ~Tip();
  void SetText(const std::string &text);
  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 private:
  Sprite *background_;
  UI::Label *label_;
  std::string text_;

  Tip(Node *parent);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_MAP_TIP_HPP_
