// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_CORE_SPRITE_HPP_
#define CLIENT_V3_CORE_SPRITE_HPP_

#include <QGraphicsWidget>

#include "Node.hpp"
#include "Stateful.hpp"

class Sprite : public Node, public Stateful {
 public:
  static Sprite *Create(Node *parent = nullptr);
  static Sprite *Create(QString pix, Node *parent = nullptr);
  ~Sprite();

  struct State {
    int x;
    int y;
  };

  void SetPixmap(const QString &file_name);
  void SetPixmap(const QString &file_name, bool preserve_size);
  void SetPixmap(const QPixmap &pixmap);
  void SetPixmap(const QPixmap &pixmap, bool preserve_size);
  QPixmap Pixmap() const;
  void SetSize(qreal width, qreal height) override;
  void Strech(unsigned int border_size, int width, int height);
  /** Scale the sprite to specified size, returns padding scaled */
  QPointF Strech(unsigned int scaled_x, unsigned int scaled_y,
                 unsigned int border_size, int width, int height);
  void SetTransformationMode(Qt::TransformationMode mode);
  void ScaledToWidth(qreal width);
  void ScaledToHeight(qreal height);

  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;

  void Draw(QPainter *painter) override;
  void RegisterEvents() override;
  void UnRegisterEvents() override;

 protected:
  virtual void *StoreInternal();
  virtual void UseInternal(void *data);
  explicit Sprite(Node *parent = nullptr);

 private:
  QPixmap cache_;
  QPixmap original_pix_;
  Qt::TransformationMode mode_;
};
#endif  // CLIENT_V3_CORE_SPRITE_HPP_
