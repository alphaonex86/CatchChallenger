// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_UI_BUTTON_HPP_
#define CLIENT_QTOPENGL_UI_BUTTON_HPP_

#include <QGraphicsItem>
#include <QPointF>

#include "../core/Node.hpp"
#include "Label.hpp"

namespace UI {
class Button : public Node {
 public:
  enum ButtonSize {
    kRectSmall,
    kRectMedium,
    kRectLarge,
    kRoundSmall,
    kRoundMedium,
    kRoundLarge
  };
  static Button *Create(QString pix, Node *parent = nullptr);
  ~Button();
  void SetText(const QString &text);
  void SetIcon(const QString &icon);
  void SetIcon(const QPixmap &icon);
  void SetIcon(const QIcon &icon);
  void SetOutlineColor(const QColor &color);
  void SetFont(const QFont &font);
  QFont GetFont() const;
  bool SetPixelSize(uint8_t size);
  void SetPressed(const bool &pressed);
  void SetEnabled(bool enabled);
  void SetCheckable(bool checkable);
  bool IsChecked() const;
  void SetSize(const ButtonSize &size);
  void SetSize(const QSizeF &size);
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

 protected:
  explicit Button(QString pix, Node *parent = nullptr);

  virtual QPixmap ScaleBackground(QPixmap pixmap, qreal width, qreal height);

 private:
  QString background_;
  bool pressed_;
  QColor outline_color_;
  QString text_;
  bool checkable_;
  bool checked_;
  Label *label_;
};
}  // namespace UI

#endif  // CLIENT_QTOPENGL_UI_BUTTON_HPP_
