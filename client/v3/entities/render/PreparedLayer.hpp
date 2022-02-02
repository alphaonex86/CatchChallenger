// Copyright 2021 CatchChallenger
#ifndef CLIENT_V3_ENTITIES_RENDER_PREPAREDLAYER_HPP_
#define CLIENT_V3_ENTITIES_RENDER_PREPAREDLAYER_HPP_

#include <QGraphicsSceneMouseEvent>
#include <QObject>
#include <QPointF>
#include <QTime>

#include "../../core/Node.hpp"
#include "../ClientStructures.hpp"
#include "MapVisualiserThread.hpp"

class PreparedLayer : public QObject, public Node {
  Q_OBJECT
 public:
  static PreparedLayer *Create(Map_full *tempMapObject, Node *parent = 0);
  static PreparedLayer *Create(Map_full *tempMapObject, const QPixmap &pixmap,
                               Node *parent = 0);
  void hoverMoveEvent(QGraphicsSceneHoverEvent *event);
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
  void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);
  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
  void Draw(QPainter *painter) override;

 private:
  Map_full *tempMapObject;
  QTime clickDuration;
  QPixmap cache;
  explicit PreparedLayer(Map_full *tempMapObject, Node *parent = 0);
  explicit PreparedLayer(Map_full *tempMapObject, const QPixmap &pixmap,
                         Node *parent = 0);
 signals:
  void eventOnMap(CatchChallenger::MapEvent event, Map_full *tempMapObject,
                  uint8_t x, uint8_t y);
};

#endif  // CLIENT_V3_ENTITIES_RENDER_PREPAREDLAYER_HPP_
