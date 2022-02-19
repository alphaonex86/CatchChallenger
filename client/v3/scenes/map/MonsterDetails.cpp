// Copyright 2021 CatchChallenger
#include "MonsterDetails.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../general/fight/CommonFightEngineBase.hpp"
#include "../../Ultimate.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../core/AssetsLoader.hpp"
#include "../../core/EventManager.hpp"

using Scenes::MonsterDetails;

MonsterDetails::MonsterDetails() {
  backdrop_ = UI::Backdrop::Create(QColor(0, 0, 0, 0), this);
  quit = UI::Button::Create(":/CC/images/interface/cancel.png", this);

  details = UI::Label::Create(this);

  monsterDetailsImage = Sprite::Create(this);

  monsterDetailsSkills = UI::ListView::Create(this);

  monsterDetailsCatched = Sprite::Create(this);
  QLinearGradient gradient1(0, 0, 0, 100);
  gradient1.setColorAt(0.25, QColor(230, 153, 0));
  gradient1.setColorAt(0.75, QColor(255, 255, 255));
  QLinearGradient gradient2(0, 0, 0, 100);
  gradient2.setColorAt(0, QColor(64, 28, 2));
  gradient2.setColorAt(1, QColor(64, 28, 2));
  monsterDetailsName = UI::Label::Create(gradient1, gradient2, this);
  monsterDetailsLevel = UI::Label::Create(this);
  hp_text = UI::Label::Create(this);
  hp_bar = UI::Progressbar::Create(this);
  xp_text = UI::Label::Create(this);
  xp_bar = UI::Progressbar::Create(this);

  /*
  if (!connect(&Language::language, &Language::newLanguage, this,
               &MonsterDetails::newLanguage, Qt::QueuedConnection))
    abort();
  */
  backdrop_->SetOnDraw(std::bind(&MonsterDetails::DrawBackground, this, std::placeholders::_1));

  quit->SetOnClick(std::bind(&MonsterDetails::Close, this));
  NewLanguage();
}

MonsterDetails::~MonsterDetails() {
  delete quit;
  delete details;
  delete monsterDetailsImage;
  delete backdrop_;
  backdrop_ = nullptr;
}

MonsterDetails *MonsterDetails::Create() {
  return new (std::nothrow) MonsterDetails();
}

void MonsterDetails::Draw(QPainter *p) {
}

void MonsterDetails::OnScreenResize() {
  int min = bounding_rect_.height();
  if (min > bounding_rect_.width()) min = bounding_rect_.width();

  auto font = details->GetFont();
  unsigned int bigSpace = 50;
  unsigned int space = 30;
  qreal lineSize = 2.0;

  if (bounding_rect_.width() < 800 || bounding_rect_.height() < 600) {
    quit->SetSize(83 / 2, 94 / 2);
    font.setPixelSize(30 / 2);
    bigSpace /= 2;
    space = 10;
    monsterDetailsCatched->SetScale(1.0);
    hp_bar->SetSize(50 * 2, 8 * 2);
    xp_bar->SetSize(50 * 2, 8 * 2);
    lineSize = 2.0;
    monsterDetailsName->SetPixelSize(50 / 2);
  } else {
    quit->SetSize(83, 94);
    font.setPixelSize(30);
    monsterDetailsCatched->SetScale(2.0);
    if (bounding_rect_.width() < 1400 || bounding_rect_.height() < 800) {
      hp_bar->SetSize(50 * 3, 8 * 3);
      xp_bar->SetSize(50 * 3, 8 * 3);
      lineSize = 3.0;
    } else {
      hp_bar->SetSize(50 * 4, 8 * 4);
      xp_bar->SetSize(50 * 4, 8 * 4);
      lineSize = 4.0;
    }
    monsterDetailsName->SetPixelSize(50);
  }
  quit->SetPos(bounding_rect_.width() - quit->Width() - bigSpace, bigSpace);
  details->SetPos(bigSpace, bigSpace);

  int ratio = (min * 2 / 3) / monsterDetailsImage->Pixmap().width();
  if (ratio > 0) monsterDetailsImage->SetScale(ratio);
  monsterDetailsImage->SetPos(
      bounding_rect_.width() / 2 - monsterDetailsImage->Pixmap().width() *
                                       monsterDetailsImage->Scale() / 2,
      bounding_rect_.height() / 2 - monsterDetailsImage->Pixmap().height() *
                                        monsterDetailsImage->Scale() / 2);

  monsterDetailsSkills->SetSize((bounding_rect_.width() - bigSpace * 4) / 3,
                                bounding_rect_.height() / 3);
  monsterDetailsSkills->SetPos(
      bounding_rect_.width() - monsterDetailsSkills->Width() - bigSpace,
      bounding_rect_.height() / 3);

  int tempY = bounding_rect_.height() / 2 +
              monsterDetailsImage->Pixmap().height() *
                  monsterDetailsImage->Scale() / 2 +
              space - 100;
  monsterDetailsName->SetPos(
      bounding_rect_.width() / 2 - monsterDetailsName->Width() / 2, tempY);
  monsterDetailsCatched->SetPos(
      bounding_rect_.width() / 2 - monsterDetailsName->Width() / 2 - space -
          monsterDetailsCatched->Width() * monsterDetailsCatched->Scale(),
      tempY -
          (monsterDetailsCatched->Height() - monsterDetailsName->Height()) / 2 -
          18);
  monsterDetailsLevel->SetPos(
      bounding_rect_.width() / 2 + monsterDetailsName->Width() / 2 + space,
      tempY +
          (monsterDetailsName->Height() - monsterDetailsLevel->Height()) / 2 +
          15);

  int hp_bar_width = hp_bar->Width();
  int hp_bar_height = hp_bar->Height();
  tempY = monsterDetailsName->Y() + monsterDetailsName->Height() + 30;
  unsigned int tempBarW =
      hp_text->Width() + space + hp_bar_width * hp_bar->Scale();
  unsigned int tempX =
      bounding_rect_.width() / 2 - tempBarW / 2 + hp_text->Width() + space;
  hp_text->SetPos(tempX - hp_text->Width() - space / 3, tempY + 10);
  int o = +(hp_text->Height() - hp_bar_height) / 2;
  hp_bar->SetPos(tempX, tempY + o);
  tempY += hp_text->Height();
  xp_text->SetPos(tempX - xp_text->Width() - space / 3, tempY + 10);
  o = +(hp_text->Height() - hp_bar_height) / 2;
  xp_bar->SetPos(tempX, tempY + o);

  backdrop_->SetSize(bounding_rect_.width(), bounding_rect_.height());
}

void MonsterDetails::OnEnter() {
  Scene::OnEnter();
  EventManager::GetInstance()->Lock();
  quit->RegisterEvents();
}

void MonsterDetails::OnExit() {
  quit->UnRegisterEvents();
  EventManager::GetInstance()->UnLock();
}

void MonsterDetails::Close() {
  if (on_close_) {
    on_close_();
  }
  Parent()->RemoveChild(this);
}

void MonsterDetails::SetOnClose(std::function<void()> callback) {
  on_close_ = callback;
}

void MonsterDetails::NewLanguage() {
  hp_text->SetText(tr("HP"));
  xp_text->SetText(tr("XP"));
}

void MonsterDetails::SetMonster(const CatchChallenger::PlayerMonster &monster) {
  // TODO(alphaonex86): monsterDetailsBuffs

  QLinearGradient gradient1(0, 0, 0, 100);
  QLinearGradient gradient2(0, 0, 0, 100);
  switch (monster.gender) {
    case CatchChallenger::Gender::Gender_Male:
      gradient1.setColorAt(0.25, QColor(180, 200, 255));
      gradient1.setColorAt(0.75, QColor(255, 255, 255));
      gradient2.setColorAt(0, QColor(2, 28, 64));
      gradient2.setColorAt(1, QColor(2, 28, 64));
      break;
    case CatchChallenger::Gender::Gender_Female:
      gradient1.setColorAt(0.25, QColor(255, 200, 200));
      gradient1.setColorAt(0.75, QColor(255, 255, 255));
      gradient2.setColorAt(0, QColor(64, 28, 2));
      gradient2.setColorAt(1, QColor(64, 28, 2));
      break;
    default:
      gradient1.setColorAt(0.25, QColor(230, 230, 230));
      gradient1.setColorAt(0.75, QColor(255, 255, 255));
      gradient2.setColorAt(0, QColor(50, 50, 50));
      gradient2.setColorAt(1, QColor(50, 50, 50));
      break;
  }
  monsterDetailsName->SetBorderColor(gradient1);
  monsterDetailsName->SetTextColor(gradient2);

  const QtDatapackClientLoader::MonsterExtra &monsterExtraInfo =
      QtDatapackClientLoader::datapackLoader->get_monsterExtra().at(monster.monster);
  const QtDatapackClientLoader::QtMonsterExtra &QtmonsterExtraInfo =
      QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster);
  const CatchChallenger::Monster &monsterGeneralInfo =
      CatchChallenger::CommonDatapack::commonDatapack.get_monsters().at(
          monster.monster);
  const CatchChallenger::Monster::Stat &stat =
      CatchChallenger::CommonFightEngineBase::getStat(monsterGeneralInfo,
                                                      monster.level);

  QString typeString;
  if (!monsterGeneralInfo.type.empty()) {
    QStringList typeList;
    unsigned int sub_index = 0;
    while (sub_index < monsterGeneralInfo.type.size()) {
      const auto &typeSub = monsterGeneralInfo.type.at(sub_index);
      if (QtDatapackClientLoader::datapackLoader->get_typeExtra().find(typeSub) !=
          QtDatapackClientLoader::datapackLoader->get_typeExtra().cend()) {
        const QtDatapackClientLoader::TypeExtra &typeExtra =
            QtDatapackClientLoader::datapackLoader->get_typeExtra().at(typeSub);
        if (!typeExtra.name.empty()) {
          /*if(typeExtra.color.isValid())
              typeList.push_back("<span
          style=\"background-color:"+typeExtra.color.name()+";\">"+QString::fromStdString(typeExtra.name)+"</span>");
          else*/
          typeList.push_back(QString::fromStdString(typeExtra.name));
        }
      }
      sub_index++;
    }
    QStringList extraInfo;
    if (!typeList.isEmpty())
      extraInfo << tr("Type: %1").arg(typeList.join(QStringLiteral(", ")));
    if (Ultimate::ultimate.isUltimate()) {
      if (!monsterExtraInfo.kind.empty())
        extraInfo << tr("Kind: %1")
                         .arg(QString::fromStdString(monsterExtraInfo.kind));
      if (!monsterExtraInfo.habitat.empty())
        extraInfo << tr("Habitat: %1")
                         .arg(QString::fromStdString(monsterExtraInfo.habitat));
    }
    typeString = extraInfo.join(QStringLiteral("\n"));
  }
  monsterDetailsName->SetText(QString::fromStdString(monsterExtraInfo.name));
  // monsterDetailsDescription->setText(QString::fromStdString(monsterExtraInfo.description));
  {
    QPixmap front = QtmonsterExtraInfo.front;
    front = front.scaled(160, 160);
    monsterDetailsImage->SetPixmap(front);
  }
  {
    QPixmap catchPixmap;
    if (QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(
            monster.catched_with) !=
        QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend()) {
      catchPixmap = QtDatapackClientLoader::datapackLoader
                        ->getItemExtra(monster.catched_with)
                        .image;
      monsterDetailsCatched->SetToolTip(
          tr("catched with %1")
              .arg(QString::fromStdString(
                  QtDatapackClientLoader::datapackLoader->get_itemsExtra()
                      .at(monster.catched_with)
                      .name)));
    } else {
      catchPixmap =
          QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
      monsterDetailsCatched->SetToolTip(
          tr("catched with unknown item: %1").arg(monster.catched_with));
    }
    catchPixmap = catchPixmap.scaled(48, 48);
    monsterDetailsCatched->SetPixmap(catchPixmap);
  }
  const uint32_t &maxXp = monsterGeneralInfo.level_to_xp.at(monster.level - 1);
  monsterDetailsLevel->SetText(QString::number(monster.level));

  hp_bar->SetMaximum(stat.hp);
  hp_bar->SetValue(monster.hp);
  xp_bar->SetMaximum(maxXp);
  xp_bar->SetValue(monster.remaining_xp);

  QString detailsString;
  detailsString += tr("Speed: %1").arg(stat.speed) + "\n";
  detailsString += tr("Attack: %1").arg(stat.attack) + "\n";
  detailsString += tr("Defense: %1").arg(stat.defense) + "\n";
  detailsString += tr("SP attack: %1").arg(stat.special_attack) + "\n";
  detailsString += tr("SP defense: %1").arg(stat.special_defense) + "\n";
  if (CommonSettingsServer::commonSettingsServer.useSP)
    detailsString += tr("Skill point: %1").arg(monster.sp) + "\n";
  if (!typeString.isEmpty()) detailsString += "\n" + typeString;
  details->SetText(detailsString);
  // skill
  monsterDetailsSkills->Clear();
  {
    unsigned int indexSkill = 0;
    while (indexSkill < monster.skills.size()) {
      const CatchChallenger::PlayerMonster::PlayerSkill &monsterSkill =
          monster.skills.at(indexSkill);
      auto item = UI::Label::Create();
      item->SetPixelSize(12);
      if (QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().find(
              monsterSkill.skill) ==
          QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra().cend()) {
        item->SetText(tr("Unknown skill"));
      } else {
        if (monsterSkill.level > 1)
          item->SetText(
              tr("%1 at level %2")
                  .arg(QString::fromStdString(
                      QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                          .at(monsterSkill.skill)
                          .name))
                  .arg(monsterSkill.level));
        else
          item->SetText(QString::fromStdString(
              QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                  .at(monsterSkill.skill)
                  .name));
        const CatchChallenger::Skill &skill =
            CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().at(
                monsterSkill.skill);
        item->SetText(
            item->Text() +
            QStringLiteral(" (%1/%2)")
                .arg(monsterSkill.endurance)
                .arg(skill.level.at(monsterSkill.level - 1).endurance));
        if (skill.type != 255)
          if (QtDatapackClientLoader::datapackLoader->get_typeExtra().find(
                  skill.type) !=
              QtDatapackClientLoader::datapackLoader->get_typeExtra().cend())
            if (!QtDatapackClientLoader::datapackLoader->get_typeExtra()
                     .at(skill.type)
                     .name.empty())
              item->SetText(
                  item->Text() + ", " +
                  tr("Type: %1")
                      .arg(QString::fromStdString(
                          QtDatapackClientLoader::datapackLoader->get_typeExtra()
                              .at(skill.type)
                              .name)));
        item->SetText(
            item->Text() + "\n" +
            QString::fromStdString(
                QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                    .at(monsterSkill.skill)
                    .description));
        item->SetToolTip(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                .at(monsterSkill.skill)
                .description));
      }
      monsterDetailsSkills->AddItem(item);
      indexSkill++;
    }
  }
  // do the buff
  /*{
      monsterDetailsBuffs->clear();
      unsigned int index=0;
      while(index<monster.buffs.size())
      {
          const PlayerBuff &buffEffect=monster.buffs.at(index);
          QListWidgetItem *item=new QListWidgetItem();
          if(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.find(buffEffect.buff)==QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.cend())
          {
              item->setToolTip(tr("Unknown buff"));
              item->setIcon(QIcon(":/CC/images/interface/buff.png"));
          }
          else
          {
              item->setIcon(QtDatapackClientLoader::datapackLoader->QtmonsterBuffsExtra.at(buffEffect.buff).icon);
              if(buffEffect.level<=1)
                  item->setToolTip(QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name));
              else
                  item->setToolTip(tr("%1 at level
  %2").arg(QString::fromStdString(
                        QtDatapackClientLoader::datapackLoader->monsterBuffsExtra.at(buffEffect.buff).name)).arg(buffEffect.level));
              item->setToolTip(item->toolTip()+"\n"+QString::fromStdString(QtDatapackClientLoader::datapackLoader->monsterBuffsExtra
                                                                           .at(buffEffect.buff).description));
          }
          monsterDetailsBuffs->addItem(item);
          index++;
      }
  }*/
  ReDraw();
}

void MonsterDetails::DrawBackground(QPainter *p) {
  int min = bounding_rect_.height();
  if (min > bounding_rect_.width()) min = bounding_rect_.width();
  const int xLeft = (bounding_rect_.width() - min) / 2;
  const int xRight = bounding_rect_.width() - min - xLeft;
  const int yTop = (bounding_rect_.height() - min) / 2;
  const int yBottom = bounding_rect_.height() - min - yTop;
  p->drawPixmap(xLeft, yTop, min, min,
                *AssetsLoader::GetInstance()->GetImage(
                    ":/CC/images/interface/backm.png"));
  if (xLeft > 0)
    p->drawPixmap(0, 0, xLeft, bounding_rect_.height(),
                  *AssetsLoader::GetInstance()->GetImage(
                      ":/CC/images/interface/backm.png"),
                  0, 0, 1, 1);
  if (xRight > 0)
    p->drawPixmap(bounding_rect_.width() - xRight, 0, xRight,
                  bounding_rect_.height(),
                  *AssetsLoader::GetInstance()->GetImage(
                      ":/CC/images/interface/backm.png"),
                  0, 0, 1, 1);
  if (yTop > 0)
    p->drawPixmap(0, 0, bounding_rect_.width(), yTop,
                  *AssetsLoader::GetInstance()->GetImage(
                      ":/CC/images/interface/backm.png"),
                  0, 0, 1, 1);
  if (yBottom > 0)
    p->drawPixmap(bounding_rect_.height() - yBottom, 0, yBottom,
                  bounding_rect_.width(),
                  *AssetsLoader::GetInstance()->GetImage(
                      ":/CC/images/interface/backm.png"),
                  0, 0, 1, 1);

  unsigned int bigSpace = 50;
  unsigned int space = 30;
  qreal lineSize = 2.0;

  if (bounding_rect_.width() < 800 || bounding_rect_.height() < 600) {
    bigSpace /= 2;
    space = 10;
    lineSize = 2.0;
  } else {
    if (bounding_rect_.width() < 1400 || bounding_rect_.height() < 800) {
      lineSize = 3.0;
    } else {
      lineSize = 4.0;
    }
  }

  p->save();
  p->setPen(
      QPen(Qt::white, lineSize, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
  // p->drawLine(bounding_rect_.width()*1/6,bounding_rect_.height()*1/6,bounding_rect_.width()*2/6,bounding_rect_.height()*1/6);
  p->drawLine(details->Width() + space, bounding_rect_.height() * 1 / 6,
              bounding_rect_.width() * 2 / 6, bounding_rect_.height() * 1 / 6);

  p->drawLine(bounding_rect_.width() * 2 / 6, bounding_rect_.height() * 1 / 6,
              bounding_rect_.width() / 2, bounding_rect_.height() * 4 / 10);
  // p->drawLine(bounding_rect_.width()*2/6,bounding_rect_.height()*1/6,bounding_rect_.width()*4/6,bounding_rect_.height()*7/10);
  p->drawLine(bounding_rect_.width() / 2, bounding_rect_.height() * 5 / 10,
              bounding_rect_.width() * 4 / 6, bounding_rect_.height() * 7 / 10);

  p->drawLine(bounding_rect_.width() * 4 / 6, bounding_rect_.height() * 7 / 10,
              bounding_rect_.width() * 5 / 6, bounding_rect_.height() * 7 / 10);
  p->restore();
}
