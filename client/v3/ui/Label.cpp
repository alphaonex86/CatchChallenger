// Copyright 2021 CatchChallenger
#include "Label.hpp"

#include <QPainter>
#include <iostream>

#include "../core/Node.hpp"

using UI::Label;

Label::Label(QLinearGradient text_color, QLinearGradient border_color,
             Node *parent)
    : Node(parent) {
  font_ = new QFont();
  font_->setFamily("Calibri");
  font_->setPixelSize(25);

  pen_gradient_ = border_color;
  brush_gradient_ = text_color;
  font_height_ = 0;

  text_path_ = nullptr;
  alignment_ = Qt::AlignLeft;
  auto_size_ = true;
}

Label::~Label() { delete font_; }

Label *Label::Create(Node *parent) {
  Label *instance = Create(QColor(25, 11, 1), QColor(255, 255, 255), parent);
  return instance;
}

Label *Label::Create(QColor text, QColor border, Node *parent) {
  QLinearGradient pen(0, 0, 0, 100);
  pen.setColorAt(0, text);
  pen.setColorAt(1, text);
  QLinearGradient brush(0, 0, 0, 100);
  brush.setColorAt(0, border);
  brush.setColorAt(1, border);

  Label *instance = new (std::nothrow) Label(pen, brush, parent);
  return instance;
}

Label *Label::Create(QColor text, QLinearGradient border, Node *parent) {
  QLinearGradient pen(0, 0, 0, 100);
  pen.setColorAt(0, text);
  pen.setColorAt(1, text);

  Label *instance = new (std::nothrow) Label(pen, border, parent);
  return instance;
}

Label *Label::Create(QLinearGradient pen, QLinearGradient border,
                     Node *parent) {
  Label *instance = new (std::nothrow) Label(pen, border, parent);
  return instance;
}

void Label::Draw(QPainter *painter) {
  if (text_.isEmpty()) {
    return;
  }
  const unsigned int h = BoundingRect().height();
  const unsigned int w = BoundingRect().width();

  QImage image(w, h, QImage::Format_ARGB32);
  image.fill(Qt::transparent);
  QPainter paint;
  if (image.isNull()) {
    abort();
  }
  paint.begin(&image);

  if (text_path_ != nullptr && paint.isActive()) {
    paint.setRenderHint(QPainter::Antialiasing);
    pen_gradient_.setFinalStop(0, h);
    auto pen_width = GetPenWidth();
    //TODO(lanstat): investigate why this abort the app
    QPen pen(pen_gradient_, pen_width, Qt::SolidLine, Qt::RoundCap,
             Qt::RoundJoin);
    paint.setPen(pen);
    paint.drawPath(*text_path_);
    QBrush brush(brush_gradient_);
    paint.setBrush(brush);
    paint.setPen(
        QPen(Qt::transparent, 0, Qt::NoPen, Qt::RoundCap, Qt::RoundJoin));
    paint.drawPath(*text_path_);
  }

  painter->drawPixmap(0, 0, QPixmap::fromImage(image));
}

void Label::SetText(const QString &text) {
  if (text_ == text) return;
  text_ = text;
  UpdateTextPath();
  ReDraw();
}

void Label::SetText(const std::string &text) {
  SetText(QString::fromStdString(text));
}

QString Label::Text() const { return text_; }

void Label::SetPixelSize(uint8_t size) {
  int tempsize = size * 1.5;
  if (font_->pixelSize() == tempsize) return;
  font_height_ = 0;
  font_->setPixelSize(tempsize);
  UpdateTextPath();
  ReDraw();
}

void Label::UpdateTextPath() {
  if (text_.isEmpty()) {
    return;
  }
  if (text_path_ != nullptr) delete text_path_;
  QPainterPath temp_path;
  int line_height = 5;
  if (font_height_ == 0) {
    temp_path.addText(0, 0, *font_,
                      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOQPRSTUVWXYZ");

    font_height_ = temp_path.boundingRect().height() + line_height;
  }
  temp_path = QPainterPath();

  QStringList lines = text_.split("\n");
  text_path_ = new QPainterPath();
  auto pen_width = GetPenWidth() * 2;

  if (auto_size_) {
    QRectF rect = temp_path.boundingRect();
    int max_width = 0;
    int max_height = 0;
    for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
      temp_path.addText(0, 0, *font_, (*iter));
      QRectF rect = temp_path.boundingRect();
      if (rect.width() > max_width) {
        max_width = rect.width();
      }
    }
    for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
      max_height += font_height_;
      text_path_->addText((pen_width / 2) - 2, max_height, *font_, (*iter));
    }
    Node::SetSize(max_width + pen_width, max_height + pen_width);
  } else {
    int max_height = 0;
    auto bounding = BoundingRect();
    for (auto iter = lines.begin(); iter != lines.end(); ++iter) {
      QString line = (*iter);
      temp_path.addText(0, 0, *font_, line);
      QRectF rect = temp_path.boundingRect();
      if (rect.width() > bounding.width()) {
        // It process the line that excess label max width
        int max_char = line.size() * bounding.width() / rect.width();
        QString aux = line;
        do {
          if (aux.size() < max_char) {
            max_char = aux.size();
          }
          QString tmp = QString();
          if (max_char < aux.size() && aux.at(max_char) == " ") {
            tmp = aux.left(max_char);
            aux.remove(0, max_char);
          } else {
            int pivot = aux.lastIndexOf(" ", max_char);
            if (pivot >= 0) {
              tmp = aux.left(pivot);
              aux.remove(0, pivot);
            } else {
              tmp = aux;
              aux = QString();
            }
          }
          aux = aux.trimmed();
          temp_path = QPainterPath();
          temp_path.addText(0, 0, *font_, tmp);
          rect = temp_path.boundingRect();
          max_height += font_height_;
          switch (alignment_) {
            case Qt::AlignCenter:
              text_path_->addText(bounding.width() / 2 - rect.width() / 2,
                                  max_height, *font_, tmp);
              break;
            case Qt::AlignRight:
              text_path_->addText(bounding.width() - rect.width() - pen_width,
                                  max_height, *font_, tmp);
              break;
            default:
              text_path_->addText((pen_width / 2) - 2, max_height, *font_, tmp);
          }
        } while (!aux.isEmpty());
      } else {
        max_height += font_height_;
        switch (alignment_) {
          case Qt::AlignCenter:
            text_path_->addText(bounding.width() / 2 - rect.width() / 2,
                                max_height, *font_, line);
            break;
          case Qt::AlignRight:
            text_path_->addText(bounding.width() - rect.width() - pen_width,
                                max_height, *font_, line);
            break;
          default:
            text_path_->addText((pen_width / 2) - 2, max_height, *font_, line);
        }
      }
    }
    Node::SetHeight(max_height + pen_width);
  }
}

void Label::SetFont(const QFont &font) {
  *this->font_ = font;
  font_height_ = 0;
  UpdateTextPath();
  ReDraw();
}

QFont Label::GetFont() const { return *font_; }

void Label::MousePressEvent(const QPointF &point, bool &press_validated) {
  (void)point;
  (void)press_validated;
}

void Label::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  (void)point;
  (void)prev_validated;
}

void Label::MouseMoveEvent(const QPointF &point) { (void)point; }

void Label::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Label::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

qreal Label::GetPenWidth() {
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

void Label::SetTextColor(QColor color) {
  QLinearGradient pen(0, 0, 0, 100);
  pen.setColorAt(0, color);
  pen.setColorAt(1, color);
  SetTextColor(pen);
}

void Label::SetTextColor(QLinearGradient gradient) {
  brush_gradient_ = gradient;
  ReDraw();
}

void Label::SetBorderColor(QColor color) {
  QLinearGradient pen(0, 0, 0, 100);
  pen.setColorAt(0, color);
  pen.setColorAt(1, color);
  SetBorderColor(pen);
}

void Label::SetBorderColor(QLinearGradient gradient) {
  pen_gradient_ = gradient;
  ReDraw();
}

void Label::SetAlignment(Qt::Alignment alignment) {
  alignment_ = alignment;
  ReDraw();
}

void Label::SetSize(qreal w, qreal h) {
  Node::SetSize(w, h);
  auto_size_ = false;
  UpdateTextPath();
}

void Label::SetWidth(qreal w) {
  auto_size_ = false;
  Node::SetWidth(w);
  UpdateTextPath();
}
