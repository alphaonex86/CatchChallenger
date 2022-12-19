// Copyright 2021 <CatchChallenger>
#include "MonsterItem.hpp"

#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/GeneralStructures.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Constants.hpp"
#include "../../../core/EventManager.hpp"
#include "../../../core/Node.hpp"
#include "../../../ui/Label.hpp"

using CatchChallenger::PlayerMonster;
using CatchChallenger::PublicPlayerMonster;
using Scenes::MonsterItem;
using namespace CatchChallenger;

MonsterItem::MonsterItem(CatchChallenger::PlayerMonster monster)
    : UI::SelectableItem() {
  monster_ = monster;
  icon_ = Sprite::Create(this);
  connection_manager_ = ConnectionManager::GetInstance();
  client_ = connection_manager_->client;
  Monster::Stat fight_stat = client_->getStat(
      CommonDatapack::commonDatapack.get_monsters().at(monster_.monster),
      monster_.level);

  max_hp_ = fight_stat.hp;

  icon_->SetSize(60, 60);
  SetSize(200, 80);
  if (monster.hp == 0) {
    SetDisabled(true);
  }
  LoadMonster();
}

MonsterItem::~MonsterItem() { delete icon_; }

MonsterItem *MonsterItem::Create(CatchChallenger::PlayerMonster monster) {
  return new (std::nothrow) MonsterItem(monster);
}

void MonsterItem::DrawContent(QPainter *painter) {
  auto text = UI::Label::Create();
  text->SetWidth(bounding_rect_.width());
  text->SetAlignment(Qt::AlignCenter);
  text->SetText(name_);
  text->SetPos(0, 10);
  text->Render(painter);

  text->SetPixelSize(Constants::TextExtraSmallSize());
  text->SetText("HP " + QString::number(monster_.hp) + "/" + QString::number(max_hp_));
  text->SetPos(0, text->Bottom() - 10);
  text->Render(painter);

  text->SetAlignment(Qt::AlignRight);
  text->SetText("Lvl. " + QString::number(monster_.level));
  text->SetPos(-10, Height() / 2 - text->Height() / 2);
  text->Render(painter);


  delete text;
}

void MonsterItem::LoadMonster() {
  const uint16_t id = monster_.monster;
  const DatapackClientLoader::MonsterExtra monster_extra =
      QtDatapackClientLoader::GetInstance()->get_monsterExtra().at(id);
  if (QtDatapackClientLoader::GetInstance()->get_monsterExtra().find(id) !=
      QtDatapackClientLoader::GetInstance()->get_monsterExtra().cend()) {
    QPixmap p =
        QtDatapackClientLoader::GetInstance()->getMonsterExtra(id).thumb;
    p = p.scaledToHeight(Height() - 20);
    icon_->SetPixmap(p);
    name_ = QString::fromStdString(monster_extra.name);
  } else {
    icon_->SetPixmap(
        QtDatapackClientLoader::GetInstance()->defaultInventoryImage());
    name_ = QStringLiteral("id: %1").arg(id);
  }
}

void MonsterItem::OnResize() {
  icon_->SetPos(10, Height() / 2 - icon_->Height() / 2);
}

