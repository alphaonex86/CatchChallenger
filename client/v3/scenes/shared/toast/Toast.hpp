// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_SCENES_SHARED_TOAST_TOAST_HPP_
#define CLIENT_V3_SCENES_SHARED_TOAST_TOAST_HPP_

#include "../../../core/Node.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../ui/ThemedLabel.hpp"
#include "../../../ui/Column.hpp"

namespace Scenes {
class Toast : public Node {
 public:
  static Toast *Create(Node *parent);
  ~Toast();
  void SetContent(const std::vector<std::string> &content);
  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 private:
  Sprite *background_;
  UI::Column *content_;
  std::string prev_rendered_;

  Toast(Node *parent);
};
}  // namespace Scenes
#endif  // CLIENT_V3_SCENES_SHARED_TOAST_TOAST_HPP_
