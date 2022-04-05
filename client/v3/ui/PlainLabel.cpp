// Copyright 2021 CatchChallenger
#include "PlainLabel.hpp"

#include <QPainter>
#include <iostream>

#include "../core/Node.hpp"

using UI::PlainLabel;

PlainLabel::PlainLabel(QColor color, Node *parent) : Node(parent) {
  font_ = new QFont();
  font_->setFamily("Roboto Condensed");
  font_->setPixelSize(25);

  text_color_ = color;
  font_height_ = 0;

  alignment_ = Qt::AlignLeft;
  auto_size_ = true;
}

PlainLabel::~PlainLabel() { delete font_; }

PlainLabel *PlainLabel::Create(Node *parent) {
  PlainLabel *instance = Create(QColor(25, 11, 1), parent);
  return instance;
}

PlainLabel *PlainLabel::Create(QColor color, Node *parent) {
  PlainLabel *instance = new (std::nothrow) PlainLabel(color, parent);
  return instance;
}

void PlainLabel::Draw(QPainter *painter) {
  if (text_.isEmpty()) {
    return;
  }
  std::cout<< "LAN_[" << __FILE__ << ":" << __LINE__ << "] "<< text_.toStdString() << std::endl;
  painter->setPen(text_color_);
  painter->setBrush(text_color_);
  painter->setFont(*font_);
  painter->drawText(0, 20, text_);
}

void PlainLabel::SetText(const QString &text) {
  if (text_ == text) return;
  text_ = text;
  UpdateTextPath();
  ReDraw();
}

void PlainLabel::SetText(const std::string &text) {
  SetText(QString::fromStdString(text));
}

QString PlainLabel::Text() const { return text_; }

void PlainLabel::SetPixelSize(uint8_t size) {
  int tempsize = size * 1.5;
  if (font_->pixelSize() == tempsize) return;
  font_height_ = 0;
  font_->setPixelSize(tempsize);
  UpdateTextPath();
  ReDraw();
}

void PlainLabel::UpdateTextPath() {
  if (text_.isEmpty()) {
    return;
  }
  QPainterPath temp_path;
  int line_height = 5;
  if (font_height_ == 0) {
    temp_path.addText(0, 0, *font_,
                      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOQPRSTUVWXYZ");

    font_height_ = temp_path.boundingRect().height() + line_height;
  }
  temp_path = QPainterPath();

  QStringList lines = text_.split("\n");
  auto pen_width = GetPenWidth() * 2;
  Node::SetSize(bounding_rect_.width(), font_height_);
}

void PlainLabel::SetFont(const QFont &font) {
  *this->font_ = font;
  font_height_ = 0;
  UpdateTextPath();
  ReDraw();
}

QFont PlainLabel::GetFont() const { return *font_; }

void PlainLabel::MousePressEvent(const QPointF &point, bool &press_validated) {
  (void)point;
  (void)press_validated;
}

void PlainLabel::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  (void)point;
  (void)prev_validated;
}

void PlainLabel::MouseMoveEvent(const QPointF &point) { (void)point; }

void PlainLabel::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void PlainLabel::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

qreal PlainLabel::GetPenWidth() {
  qreal pen_width = 2.0;
  if (font_->pixelSize() <= 12)
    pen_width = 2;
  else if (font_->pixelSize() <= 18)
    pen_width = 3;
  else if (font_->pixelSize() <= 24)
    pen_width = 4;
  else
    pen_width = 5;
  return pen_width;
}

void PlainLabel::SetTextColor(QColor color) {
  text_color_ = color;
}

void PlainLabel::SetAlignment(Qt::Alignment alignment) {
  alignment_ = alignment;
  ReDraw();
}

void PlainLabel::SetSize(qreal w, qreal h) {
  Node::SetSize(w, h);
  auto_size_ = false;
  UpdateTextPath();
}

void PlainLabel::SetWidth(qreal w) {
  auto_size_ = false;
  Node::SetWidth(w);
  UpdateTextPath();
}
