// Copyright 2021 CatchChallenger
#include "Sprite.hpp"

#include <QPainter>
#include <iostream>

#include "AssetsLoader.hpp"
#include "EventManager.hpp"
#include "Node.hpp"
#include "Stateful.hpp"

Sprite::Sprite(Node *parent) : Node(parent) {
  mode_ = Qt::SmoothTransformation;
}

Sprite::~Sprite() {}

Sprite *Sprite::Create(Node *parent) {
  Sprite *instance = new (std::nothrow) Sprite(parent);
  return instance;
}

Sprite *Sprite::Create(QString pix, Node *parent) {
  Sprite *instance = new (std::nothrow) Sprite(parent);
  instance->SetPixmap(pix);
  return instance;
}

void *Sprite::StoreInternal() {
  State *data = new State();
  data->x = X();
  data->y = Y();
  return data;
}

void Sprite::UseInternal(void *data) {
  auto object = reinterpret_cast<State *>(data);
  SetPos(object->x, object->y);
}

void Sprite::Draw(QPainter *painter) { painter->drawPixmap(0, 0, cache_); }

void Sprite::SetPixmap(const QString &file_name) {
  SetPixmap(*AssetsLoader::GetInstance()->GetImage(file_name));
}

void Sprite::SetPixmap(const QString &file_name, bool preserve_size) {
  SetPixmap(*AssetsLoader::GetInstance()->GetImage(file_name), preserve_size);
}

void Sprite::SetPixmap(const QPixmap &pixmap) { SetPixmap(pixmap, true); }

void Sprite::SetPixmap(const QPixmap &pixmap, bool preserve_size) {
  if (preserve_size) {
    cache_ = pixmap;
    Node::SetSize(cache_.width(), cache_.height());
  } else {
    cache_ =
        pixmap.scaledToWidth(Width(), mode_);
  }
  original_pix_ = cache_;
  ReDraw();
}

void Sprite::SetSize(qreal width, qreal height) {
  if (!cache_.isNull()) {
    cache_ = cache_.scaled(width, height, Qt::IgnoreAspectRatio,
                           mode_);
  }
  Node::SetSize(width, height);
}

void Sprite::MousePressEvent(const QPointF &point, bool &press_validated) {
  if (press_validated) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
  }
}

void Sprite::MouseReleaseEvent(const QPointF &point, bool &prev_validated) {
  if (prev_validated) return;

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      if (on_click_) {
        on_click_(this);
      }
    }
  }
}

void Sprite::MouseMoveEvent(const QPointF &point) { (void)point; }

void Sprite::KeyPressEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Sprite::KeyReleaseEvent(QKeyEvent *event, bool &event_trigger) {
  (void)event;
  (void)event_trigger;
}

void Sprite::Strech(unsigned int border_size, int width, int height) {
  if (width == Width() && height == Height()) return;

  int tw = border_size;
  if (width < 50)
    tw = border_size / 4;
  else if (width > 250)
    tw = border_size;
  else
    tw = (width - 50) * border_size * 3 / 4 / 200 + border_size / 4;
  int th = border_size;
  if (height < 50)
    th = border_size / 4;
  else if (height > 250)
    th = border_size;
  else
    th = (height - 50) * border_size * 3 / 4 / 200 + border_size / 4;
  int min = tw;
  if (min > th) min = th;

  QPixmap background = cache_;
  QPixmap tr, tm, tl, mr, mm, ml, br, bm, bl;
  tl = background.copy(0, 0, border_size, border_size);
  tm = background.copy(border_size, 0,
                       background.width() - border_size - border_size,
                       border_size);
  tr = background.copy(background.width() - border_size, 0, border_size,
                       border_size);
  ml = background.copy(0, border_size, border_size,
                       background.height() - border_size - border_size);
  mm = background.copy(border_size, border_size,
                       background.width() - border_size - border_size,
                       background.height() - border_size - border_size);
  mr = background.copy(background.width() - border_size, border_size,
                       border_size,
                       background.height() - border_size - border_size);
  bl = background.copy(0, background.height() - border_size, border_size,
                       border_size);
  bm = background.copy(border_size, background.height() - border_size,
                       background.width() - border_size - border_size,
                       border_size);
  br = background.copy(background.width() - border_size,
                       background.height() - border_size, border_size,
                       border_size);

  if (min != tl.height()) {
    tl = tl.scaledToHeight(min, mode_);
    tm = tm.scaledToHeight(min, mode_);
    tr = tr.scaledToHeight(min, mode_);
    ml = ml.scaledToWidth(min, mode_);
    // mm=mm.scaledToHeight(minsize,Qt::SmoothTransformation);
    mr = mr.scaledToWidth(min, mode_);
    bl = bl.scaledToHeight(min, mode_);
    bm = bm.scaledToHeight(min, mode_);
    br = br.scaledToHeight(min, mode_);
  }

  auto tmp = QPixmap(width, height);
  tmp.fill(Qt::transparent);

  auto painter = new QPainter(&tmp);
  painter->drawPixmap(0, 0, min, min, tl);
  painter->drawPixmap(min, 0, width - min * 2, min, tm);
  painter->drawPixmap(width - min, 0, min, min, tr);

  painter->drawPixmap(0, min, min, height - min * 2, ml);
  painter->drawPixmap(min, min, width - min * 2, height - min * 2, mm);
  painter->drawPixmap(width - min, min, min, height - min * 2, mr);

  painter->drawPixmap(0, height - min, min, min, bl);
  painter->drawPixmap(min, height - min, width - min * 2, min, bm);
  painter->drawPixmap(width - min, height - min, min, min, br);
  painter->end();
  delete painter;
  SetPixmap(tmp);
}

QPointF Sprite::Strech(unsigned int padding_x, unsigned int padding_y,
                       unsigned int border_size, int width, int height) {
  if (width == Width() && height == Height()) return QPointF();

  if (original_pix_.width() == 0 || original_pix_.height() == 0) {
    original_pix_ = cache_.copy();
  }

  QPixmap background = original_pix_.copy();
  QPixmap tr, tm, tl, mr, mm, ml, br, bm, bl;
  tl = background.copy(0, 0, padding_x, padding_y);
  tm = background.copy(padding_x, 0, background.width() - padding_x * 2,
                       padding_y);
  tr = background.copy(background.width() - padding_x, 0, padding_x, padding_y);
  ml = background.copy(0, padding_y, padding_x,
                       background.height() - padding_y * 2);
  mm = background.copy(padding_x, padding_y, background.width() - padding_x * 2,
                       background.height() - padding_y * 2);
  mr = background.copy(background.width() - padding_x, padding_y, padding_x,
                       background.height() - padding_y * 2);
  bl =
      background.copy(0, background.height() - padding_y, padding_x, padding_y);
  bm = background.copy(padding_x, background.height() - padding_y,
                       background.width() - padding_x * 2, padding_y);
  br = background.copy(background.width() - padding_x,
                       background.height() - padding_y, padding_x, padding_y);

  int scaled_x = 0;
  int scaled_y = 0;

  if (padding_x > padding_y) {
    scaled_x = (padding_x / padding_y) * border_size;
    scaled_y = border_size;
  } else {
    scaled_y = (padding_y / padding_x) * border_size;
    scaled_x = border_size;
  }

  tl = tl.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);
  tm = tm.scaled(width - scaled_x * 2, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);
  tr = tr.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);
  ml = ml.scaled(scaled_x, height - scaled_y * 2, Qt::IgnoreAspectRatio,
                 mode_);
  mm = mm.scaled(width - scaled_x * 2, height - scaled_y * 2,
                 Qt::IgnoreAspectRatio, mode_);
  mr = mr.scaled(scaled_x, height - scaled_y * 2, Qt::IgnoreAspectRatio,
                 mode_);
  bl = bl.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);
  bm = bm.scaled(width - scaled_x * 2, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);
  br = br.scaled(scaled_x, scaled_y, Qt::IgnoreAspectRatio,
                 mode_);

  auto tmp = QPixmap(width, height);
  tmp.fill(Qt::transparent);

  auto painter = new QPainter(&tmp);
  painter->drawPixmap(0, 0, tl);
  painter->drawPixmap(scaled_x, 0, tm);
  painter->drawPixmap(width - scaled_x, 0, tr);

  painter->drawPixmap(0, scaled_y, ml);
  painter->drawPixmap(scaled_x, scaled_y, mm);
  painter->drawPixmap(width - scaled_x, scaled_y, mr);

  painter->drawPixmap(0, height - scaled_y, bl);
  painter->drawPixmap(scaled_x, height - scaled_y, bm);
  painter->drawPixmap(width - scaled_x, height - scaled_y, br);
  painter->end();
  delete painter;
  SetPixmap(tmp);

  return QPointF(scaled_x, scaled_y);
}

QPixmap Sprite::Pixmap() const { return original_pix_; }

void Sprite::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void Sprite::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

void Sprite::SetTransformationMode(Qt::TransformationMode mode) {
  mode_ = mode;
}

void Sprite::ScaledToWidth(qreal width) {
  Sprite::SetSize(width, width / Width() * Height());
}

void Sprite::ScaledToHeight(qreal height) {
  Sprite::SetSize(height / Height() * Width(), height);
}
