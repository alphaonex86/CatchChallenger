// Copyright 2021 CatchChallenger
#include "MapMonsterPreview.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/fight/CommonFightEngine.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Constants.hpp"
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
  SetSize(Constants::ButtonSmallHeight(), Constants::ButtonSmallHeight());
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

  int padding = Width() * 0.05;
  qreal bar_height = Height() * 0.15;
  qreal bar_width = Width() - padding * 2;
  qreal barx = bx + padding;
  qreal bary = Height() - bar_height - padding;

  auto tmp = QPixmap(Width(), Height());
  tmp.fill(Qt::transparent);
  auto painter = new QPainter(&tmp);

  const QtDatapackClientLoader::QtMonsterExtra &monsterExtra =
      QtDatapackClientLoader::GetInstance()->getMonsterExtra(monster_.monster);

  QPixmap front =
      monsterExtra.thumb.scaledToWidth(Width(), Qt::FastTransformation);

  painter->drawPixmap(
      bounding_rect_.width() / 2 - front.width() / 2,
      (bounding_rect_.height() - padding * 2) / 2 - front.height() / 2,
      front.width(), front.height(), front);

  const CatchChallenger::Monster &monsterGeneralInfo =
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
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

  QPixmap background = QPixmap(":/CC/images/interface/mbb.png");
  QPixmap bar;
  if (monster_.hp > (stat.hp / 2)) {
    bar = QPixmap(":/CC/images/interface/mbgreen.png");
  } else if (monster_.hp > (stat.hp / 4)) {
    bar = QPixmap(":/CC/images/interface/mborange.png");
  } else {
    bar = QPixmap(":/CC/images/interface/mbred.png");
  }

  painter->drawPixmap(barx, bary, bar_width, bar_height,
                      background);

  qreal percent = monster_.hp / (float)stat.hp;
  painter->drawPixmap(barx, bary, bar_width * percent, bar_height,
                      bar);

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
  painter->drawPixmap(bx, by, Width(), Height(), background);
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

int MapMonsterPreview::Index() { return index_; }

void MapMonsterPreview::SetOnDrop(std::function<void(Node *)> callback) {
  on_drop_ = callback;
}
