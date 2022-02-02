// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_UI_LABEL_HPP_
#define CLIENT_QTOPENGL_UI_LABEL_HPP_

#include <QGraphicsWidget>

#include "../core/Node.hpp"

namespace UI {
class Label : public Node {
 public:
  static Label *Create(Node *parent = nullptr);
  static Label *Create(QColor text, QColor border, Node *parent = nullptr);
  static Label *Create(QColor text, QLinearGradient border,
                       Node *parent = nullptr);
  static Label *Create(QLinearGradient text, QLinearGradient border,
                       Node *parent = nullptr);
  ~Label();

  void SetText(const QString &text);
  void SetText(const std::string &text);
  QString Text() const;
  void SetFont(const QFont &font);
  QFont GetFont() const;
  void SetPixelSize(uint8_t size);
  void UpdateTextPath();
  void SetTextColor(QColor color);
  void SetTextColor(QLinearGradient gradient);
  void SetBorderColor(QColor color);
  void SetBorderColor(QLinearGradient gradient);
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
  QPainterPath *text_path_;
  QFont *font_;
  QString text_;
  QLinearGradient pen_gradient_;
  QLinearGradient brush_gradient_;
  qreal font_height_;
  Qt::Alignment alignment_;
  bool auto_size_;

  explicit Label(QLinearGradient pen, QLinearGradient brush,
                 Node *parent = nullptr);
  qreal GetPenWidth();
};
}  // namespace UI
#endif  // CLIENT_QTOPENGL_UI_LABEL_HPP_
