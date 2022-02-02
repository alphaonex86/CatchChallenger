// Copyright 2021 CatchChallenger
#ifndef CLIENT_QTOPENGL_GRAPHICRENDERER_HPP_
#define CLIENT_QTOPENGL_GRAPHICRENDERER_HPP_

#include <QGraphicsItem>
#include <QObject>

#include "Scene.hpp"

class GraphicRenderer : public QObject, public QGraphicsItem {
  Q_OBJECT

 public:
  GraphicRenderer();
  ~GraphicRenderer();

  void SetScene(Scene *scene);

  void paint(QPainter *painter, const QStyleOptionGraphicsItem *option,
             QWidget *widget) override;
  QRectF boundingRect() const override;

 private:
  Scene *scene_;
  QPixmap buffer_;
};

#endif  // CLIENT_QTOPENGL_GRAPHICRENDERER_HPP_
