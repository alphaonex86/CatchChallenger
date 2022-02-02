// Copyright 2021 CatchChallenger
#include "GraphicRenderer.hpp"

#include <QDebug>
#include <QGraphicsItem>
#include <QPainter>
#include <QWidget>
#include <iostream>

#include "Scene.hpp"

GraphicRenderer::GraphicRenderer() : QGraphicsItem() {
  scene_ = nullptr;
  buffer_ = QPixmap(0, 0);
}

GraphicRenderer::~GraphicRenderer() {}

void GraphicRenderer::SetScene(Scene *scene) { scene_ = scene; }

void GraphicRenderer::paint(QPainter *painter,
                            const QStyleOptionGraphicsItem *option,
                            QWidget *widget) {

  if (scene_ != nullptr) {
    if (widget->width() != buffer_.width() ||
        widget->height() != buffer_.height())
      buffer_ = QPixmap(widget->width(), widget->height());
    if (buffer_.width() == 0 || buffer_.height() == 0) return;
    buffer_.fill(Qt::transparent);
    QPainter tmp(&buffer_);
    scene_->Render(&tmp);
    painter->drawPixmap(0, 0, buffer_);
  }
}

QRectF GraphicRenderer::boundingRect() const { return QRectF(); }
