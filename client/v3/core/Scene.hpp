// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_CORE_SCENE_HPP_
#define CLIENT_V3_CORE_SCENE_HPP_

#include <QCoreApplication>
#include <QGraphicsWidget>
#include <string>
#include <vector>

#include "Node.hpp"

class Scene : public Node {
  Q_DECLARE_TR_FUNCTIONS(Scene)

 public:
  ~Scene();

  void RegisterMouseListener(Node *node);
  void RegisterKeyboardListerner(Node *node);
  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *) override;
  QRectF boundingRect() const override;

  void Draw(QPainter *painter) override;
  void MousePressEvent(const QPointF &point, bool &press_validated) override;
  void MouseReleaseEvent(const QPointF &point, bool &prev_validated) override;
  void MouseMoveEvent(const QPointF &point) override;
  void KeyPressEvent(QKeyEvent *event, bool &event_trigger) override;
  void KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) override;
  void OnResize() override;
  void OnEnter() override;

  void PrintError(const char *file, int line, const QString &message);
  void PrintError(const QString &message);
  void PrintError(const std::string &message);

 protected:
  explicit Scene(Node *parent = nullptr);

  virtual void OnScreenSD();
  virtual void OnScreenHD();
  virtual void OnScreenHDR();
  virtual void OnScreenResize();
};
#endif  // CLIENT_V3_CORE_SCENE_HPP_
