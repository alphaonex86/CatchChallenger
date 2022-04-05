// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_UI_PLAINLABEL_HPP_
#define CLIENT_V3_UI_PLAINLABEL_HPP_

#include "../core/Node.hpp"

namespace UI {
class PlainLabel : public Node {
 public:
  static PlainLabel *Create(Node *parent = nullptr);
  static PlainLabel *Create(QColor text, Node *parent = nullptr);
  ~PlainLabel();

  void SetText(const QString &text);
  void SetText(const std::string &text);
  QString Text() const;
  void SetFont(const QFont &font);
  QFont GetFont() const;
  void SetPixelSize(uint8_t size);
  void UpdateTextPath();
  void SetTextColor(QColor color);
  void SetAlignment(Qt::Alignment alignment);
  void SetSize(qreal w, qreal h);
  void SetWidth(qreal w);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;

  void Draw(QPainter *painter) override;

 private:
  QFont *font_;
  QString text_;
  qreal font_height_;
  Qt::Alignment alignment_;
  bool auto_size_;
  QColor text_color_;

  explicit PlainLabel(QColor color, Node *parent = nullptr);
  qreal GetPenWidth();
};
}  // namespace UI
#endif  // CLIENT_V3_UI_PLAINLABEL_HPP_
