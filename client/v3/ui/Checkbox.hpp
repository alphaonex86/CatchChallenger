// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_UI_CHECKBOX_HPP_
#define CLIENT_QTOPENGL_UI_CHECKBOX_HPP_

#include <QGraphicsItem>
#include <QPointF>

#include "../core/Node.hpp"
#include "Label.hpp"

namespace UI {
class Checkbox : public Node {
 public:
  enum CheckSize { kSmall, kMedium, kLarge };
  static Checkbox* Create(Node *parent = nullptr);
  ~Checkbox();
  bool IsChecked() const;
  void SetChecked(bool check);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;
  void SetSize(const CheckSize &size);

  void Draw(QPainter *painter) override;

 protected:
  explicit Checkbox(Node *parent = nullptr);

 private:
  bool checked_;
};
}  // namespace UI

#endif  // CLIENT_QTOPENGL_UI_CHECKBOX_HPP_
