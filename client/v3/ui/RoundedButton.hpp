// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_ROUNDEDBUTTON_HPP_
#define CLIENT_V3_UI_ROUNDEDBUTTON_HPP_

#include <QGraphicsItem>
#include <QPointF>

#include "../core/Node.hpp"
#include "Label.hpp"

namespace UI {
class RoundedButton : public Node {
 public:
  ~RoundedButton();
  void SetPressed(const bool &pressed);
  void SetEnabled(bool enabled);
  void SetCheckable(bool checkable);
  bool IsChecked() const;
  void SetSize(qreal width, qreal height);
  void SetWidth(qreal width);
  void SetPos(qreal x, qreal y);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

  void Draw(QPainter *painter) override;
  void ReDrawContent();
  virtual void DrawContent(QPainter *painter, QColor color, QRectF boundary) = 0;

  void SetStart(QColor inactive, QColor active);
  void SetEnd(qreal percent, QColor inactive, QColor active);
  void SetBorder(qreal width, QColor color);

 protected:
  explicit RoundedButton(Node *parent = nullptr);

 private:
  QColor start_inactive_;
  QColor start_active_;
  QColor end_inactive_;
  QColor end_active_;
  qreal end_percent_;

  bool has_border_;
  qreal border_width_;
  QColor border_color_;

  bool pressed_;
  bool checkable_;
  bool checked_;
  QPixmap *content_cache_;
};
}  // namespace UI

#endif  // CLIENT_V3_UI_ROUNDEDBUTTON_HPP_
