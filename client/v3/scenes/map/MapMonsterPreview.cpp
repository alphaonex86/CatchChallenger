// Copyright 2021 CatchChallenger
#include "MapMonsterPreview.hpp"

#include <QPainter>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/fight/CommonFightEngine.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/EventManager.hpp"

using Scenes::MapMonsterPreview;

MapMonsterPreview::MapMonsterPreview(
    const CatchChallenger::PlayerMonster &monster, int index, Node *parent)
    : Node(parent) {
  on_drop_ = nullptr;
  index_ = index;
  monster_ = monster;
  pressed_ = false;
  drag_ = false;
  SetSize(56, 55);
  DrawContent();
}

MapMonsterPreview::~MapMonsterPreview() {}

MapMonsterPreview *MapMonsterPreview::Create(
    const CatchChallenger::PlayerMonster &m, int index, Node *parent) {
  return new (std::nothrow) MapMonsterPreview(m, index, parent);
}

void MapMonsterPreview::DrawContent() {
  unsigned int bx = 0;
  unsigned int by = 0;

  auto tmp = QPixmap(Width(), Height());
  tmp.fill(Qt::transparent);
  auto painter = new QPainter(&tmp);

  const QtDatapackClientLoader::QtMonsterExtra &monsterExtra =
      QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster_.monster);
  QPixmap front = monsterExtra.thumb.scaledToWidth(
      monsterExtra.thumb.width() * 2, Qt::FastTransformation);
  painter->drawPixmap(bounding_rect_.width() / 2 - front.width() / 2,
                      bounding_rect_.height() / 2 - front.height() / 2,
                      front.width(), front.height(), front);

  const CatchChallenger::Monster &monsterGeneralInfo =
      CatchChallenger::CommonDatapack::commonDatapack.monsters.at(
          monster_.monster);
  const CatchChallenger::Monster::Stat &stat =
      CatchChallenger::CommonFightEngine::getStat(monsterGeneralInfo,
                                                  monster_.level);

  if (monster_.hp == 0) {
    QPixmap monsterko = QPixmap(":/CC/images/interface/monsterko.png");
    painter->drawPixmap(bounding_rect_.width() / 2 - monsterko.width() / 2,
                        bounding_rect_.height() / 2 - monsterko.height() / 2,
                        monsterko.width(), monsterko.height(), monsterko);
  }

  int barx = bx + 3;
  int bary = by + 46;
  QPixmap backgroundLeft, backgroundMiddle, backgroundRight;
  QPixmap barLeft, barMiddle, barRight;
  if (backgroundLeft.isNull() || backgroundLeft.height() != 8) {
    QPixmap background(":/CC/images/interface/mbb.png");
    if (background.isNull()) abort();
    QPixmap bar;
    if (monster_.hp > (stat.hp / 2))
      bar = QPixmap(":/CC/images/interface/mbgreen.png");
    else if (monster_.hp > (stat.hp / 4))
      bar = QPixmap(":/CC/images/interface/mborange.png");
    else
      bar = QPixmap(":/CC/images/interface/mbred.png");
    if (bar.isNull()) abort();
    backgroundLeft = background.copy(0, 0, 3, 8);
    backgroundMiddle = background.copy(3, 0, 44, 8);
    backgroundRight = background.copy(47, 0, 3, 8);
    barLeft = bar.copy(0, 0, 2, 8);
    barMiddle = bar.copy(2, 0, 46, 8);
    barRight = bar.copy(48, 0, 2, 8);
  }
  painter->drawPixmap(barx + 0, bary + 0, backgroundLeft.width(),
                      backgroundLeft.height(), backgroundLeft);
  painter->drawPixmap(barx + backgroundLeft.width(), bary + 0,
                      50 - backgroundLeft.width() - backgroundRight.width(),
                      backgroundLeft.height(), backgroundMiddle);
  painter->drawPixmap(barx + 50 - backgroundRight.width(), bary + 0,
                      backgroundRight.width(), backgroundRight.height(),
                      backgroundRight);

  int startX = 0;
  int size = 50 - startX - startX;
  int inpixel = monster_.hp * size / stat.hp;
  if (inpixel < (barLeft.width() + barRight.width())) {
    if (inpixel > 0) {
      const int tempW = inpixel / 2;
      if (tempW > 0) {
        QPixmap barLeftC = barLeft.copy(0, 0, tempW, barLeft.height());
        painter->drawPixmap(barx + startX, bary + 0, barLeftC.width(),
                            barLeftC.height(), barLeftC);
        const unsigned int pixelremaining = inpixel - barLeftC.width();
        if (pixelremaining > 0) {
          QPixmap barRightC =
              barRight.copy(barRight.width() - pixelremaining, 0,
                            pixelremaining, barRight.height());
          painter->drawPixmap(barx + startX + barLeftC.width(), bary + 0,
                              barRightC.width(), barRightC.height(), barRightC);
        }
      }
    }
  } else {
    painter->drawPixmap(barx + startX, bary + 0, barLeft.width(),
                        barLeft.height(), barLeft);
    const unsigned int pixelremaining =
        inpixel - (barLeft.width() + barRight.width());
    if (pixelremaining > 0)
      painter->drawPixmap(barx + startX + barLeft.width(), bary + 0,
                          pixelremaining, barMiddle.height(), barMiddle);
    painter->drawPixmap(barx + startX + barLeft.width() + pixelremaining,
                        bary + 0, barRight.width(), barRight.height(),
                        barRight);
  }
  painter->end();
  delete painter;
  cache_.swap(tmp);
}

void MapMonsterPreview::Draw(QPainter *painter) {
  QPixmap background;
  if (drag_)
    background = *AssetsLoader::GetInstance()->GetImage(
        ":/CC/images/interface/mgbDrag.png");
  else
    background =
        *AssetsLoader::GetInstance()->GetImage(":/CC/images/interface/mgb.png");
  unsigned int bx = 0;
  unsigned int by = 0;
  painter->drawPixmap(bx, by, background.width(), background.height(),
                      background);
  painter->drawPixmap(bx, by, cache_);
}

void MapMonsterPreview::SetPressed(const bool &pressed) { pressed_ = pressed; }

bool MapMonsterPreview::IsPressed() { return this->pressed_; }

const CatchChallenger::PlayerMonster &MapMonsterPreview::GetMonster() const {
  return monster_;
}

void MapMonsterPreview::MousePressEvent(const QPointF &point,
                                        bool &press_validated) {
  drag_ = false;
  if (pressed_) return;
  if (press_validated) return;
  if (!IsVisible()) return;
  if (!IsEnabled()) return;
  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (t.contains(point)) {
    press_validated = true;
    SetPressed(true);
  }
}

void MapMonsterPreview::MouseReleaseEvent(const QPointF &point,
                                          bool &prev_validated) {
  if (prev_validated) {
    SetPressed(false);
    return;
  }

  const QRectF &b = BoundingRect();
  const QRectF &t = MapRectToScene(b);
  if (!pressed_) return;
  if (!IsEnabled()) return;
  if (!prev_validated && IsVisible()) {
    if (t.contains(point)) {
      if (on_click_ && !drag_) {
        on_click_(this);
        prev_validated = true;
      }
    }
  }
  if (drag_) {
    drag_ = false;
    if (on_drop_) {
      on_drop_(this);
    }
    ReDraw();
  }
  SetPressed(false);
}

void MapMonsterPreview::MouseMoveEvent(const QPointF &point) {
  if (pressed_) {
    if (!drag_) {
      drag_ = true;
      ReDraw();
    }
    SetX(point.x() - Parent()->X());
  }
}

void MapMonsterPreview::RegisterEvents() {
  EventManager::GetInstance()->AddMouseListener(this);
}

void MapMonsterPreview::UnRegisterEvents() {
  EventManager::GetInstance()->RemoveListener(this);
}

int MapMonsterPreview::Index() {
  return index_;
}

void MapMonsterPreview::SetOnDrop(std::function<void (Node *)> callback) {
  on_drop_ = callback;
}
