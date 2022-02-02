// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_SLIDER_HPP_
#define CLIENT_V3_UI_SLIDER_HPP_

#include "../core/Node.hpp"
#include "../core/Sprite.hpp"

namespace UI {
class Slider : public Node {
 public:
  static Slider *Create(Node *parent = nullptr);
  ~Slider();

  void SetValue(qreal value);
  void SetMaximum(qreal value);
  qreal Value();
  qreal Maximum();

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void SetOnValueChange(std::function<void(qreal)> callback);

 private:
  bool is_slider_pressed_;
  Sprite *slider_;
  std::function<void(qreal)> on_value_change_;
  qreal value_;
  qreal maximum_;

  explicit Slider(Node *parent = nullptr);
  void ReCalculateSliderPosition();

 protected:
  void OnResize() override;
};
}  // namespace UI
#endif  // CLIENT_V3_UI_SLIDER_HPP_
