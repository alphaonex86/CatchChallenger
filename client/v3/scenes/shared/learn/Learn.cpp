// Copyright 2021 CatchChallenger
#include "Learn.hpp"

#include <iostream>

#include "../../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../../general/base/CommonSettingsServer.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Logger.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/Row.hpp"
#include "../../../ui/ThemedItem.hpp"

using Scenes::Learn;
using std::placeholders::_1;
using std::placeholders::_2;

Learn::Learn() {
  selected_ = nullptr;
  SetDialogSize(600, 500);

  portrait_ = Sprite::Create(this);

  learn_ = UI::Button::Create(":/CC/images/interface/buy.png", this);
  learn_->SetOnClick(std::bind(&Learn::OnActionClick, this, _1));

  monster_selector_ =
      UI::Button::Create(":/CC/images/interface/button.png", this);
  monster_selector_->SetText(QObject::tr("Change"));
  monster_selector_->SetPixelSize(14);

  content_ = UI::ListView::Create(this);

  sp_ = UI::Label::Create(this);
  sp_->SetPixelSize(14);
  sp_->SetAlignment(Qt::AlignCenter);

  info_ = UI::Label::Create(this);
  info_->SetPixelSize(14);
  info_->SetAlignment(Qt::AlignCenter);

  description_ = UI::Label::Create(this);
  description_->SetPixelSize(14);
  description_->SetAlignment(Qt::AlignCenter);

  // if (!QObject::connect(connexionManager->client,
  //&CatchChallenger::Api_client_real::QthaveLearnList,
  // std::bind(&Learn::HaveLearnList, this, _1)))
  // abort();

  AddActionButton(learn_);

  NewLanguage();
}

Learn::~Learn() {
  delete portrait_;
  portrait_ = nullptr;
  delete content_;
  content_ = nullptr;
}

Learn *Learn::Create() { return new (std::nothrow) Learn(); }

void Learn::NewLanguage() { SetTitle(QObject::tr("Learn")); }

void Learn::Draw(QPainter *painter) {
  UI::Dialog::Draw(painter);
  auto inner = ContentBoundary();
  auto b2 = Sprite::Create(":/CC/images/interface/b2.png");
  b2->Strech(24, 22, 20, 160, 160);
  b2->SetPos(inner.x(), inner.y());
  b2->Render(painter);

  int x = b2->X() + b2->Width() + 20;

  b2->Strech(24, 22, 20, inner.width() + inner.x() - x, inner.height());
  b2->SetPos(x, inner.y());
  b2->Render(painter);

  delete b2;
}

void Learn::OnEnter() {
  UI::Dialog::OnEnter();
  content_->RegisterEvents();
  monster_selector_->RegisterEvents();

  LoadLearnSkills(0);
}

void Learn::OnExit() {
  content_->UnRegisterEvents();
  monster_selector_->UnRegisterEvents();
  UI::Dialog::OnExit();
}

void Learn::OnActionClick(Node *node) {
  if (node == learn_) {
    if (selected_ != nullptr) {
      ConnectionManager::GetInstance()->client->learnSkill(&current_monster_,
                                                           selected_->Data(99));
      Close();
    }
  }
}

void Learn::OnScreenResize() {
  UI::Dialog::OnScreenResize();

  auto content = ContentPlainBoundary();
  qreal width = content.width() / 2;

  content_->SetPos(content.x() + width, content.y() + 15);
  content_->SetSize(width, content.height() - 25);
  content_->SetItemSpacing(5);

  sp_->SetPos(content.x(), content.y());
  sp_->SetWidth(width);

  portrait_->SetSize(160, 160);
  portrait_->SetPos((sp_->X() + width / 2) - portrait_->Width() / 2,
                    sp_->Bottom() + 5);

  monster_selector_->SetSize(width - 80, 40);
  monster_selector_->SetPos(
      (sp_->X() + width / 2) - monster_selector_->Width() / 2,
      portrait_->Bottom() + 5);

  info_->SetPos(sp_->X(), monster_selector_->Bottom() + 5);
  info_->SetWidth(width);

  description_->SetPos(sp_->X(), content.y() + content.height() - 100);
  description_->SetWidth(width);
}

void Learn::Close() { Parent()->RemoveChild(this); }

void Learn::OnItemClick(Node *node) {
  selected_ = node;
  auto items = content_->Items();
  for (auto item : items) {
    if (node == item) {
      continue;
    }
    // static_cast<LearnItem *>(item)->SetSelected(false);
  }
}

void Learn::LoadLearnSkills(uint monster_pos) {
  content_->Clear();
  QHash<uint16_t, uint8_t> skillToDisplay;
  unsigned int index = monster_pos;
  CatchChallenger::PlayerMonster monster =
      PlayerInfo::GetInstance()->GetInformationRO().playerMonster.at(index);
  portrait_->SetPixmap(
      QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster)
          .front.scaled(160, 160));

  current_monster_ = monster;
  if (CommonSettingsServer::commonSettingsServer.useSP) {
    sp_->SetVisible(true);
    sp_->SetText(QObject::tr("SP: %1").arg(monster.sp));
  } else {
    sp_->SetVisible(false);
  }
#ifdef CATCHCHALLENGER_VERSION_ULTIMATE
  info_->SetText(QObject::tr("<b>%1</b><br />Level %2")
                     .arg(QString::fromStdString(
                         QtDatapackClientLoader::datapackLoader->monsterExtra
                             .at(monster.monster)
                             .name))
                     .arg(monster.level));
#endif
  unsigned int sub_index = 0;
  while (sub_index < CatchChallenger::CommonDatapack::commonDatapack.get_monsters()
                         .at(monster.monster)
                         .learn.size()) {
    CatchChallenger::Monster::AttackToLearn learn =
        CatchChallenger::CommonDatapack::commonDatapack.get_monsters()
            .at(monster.monster)
            .learn.at(sub_index);
    if (learn.learnAtLevel <= monster.level) {
      unsigned int sub_index2 = 0;
      while (sub_index2 < monster.skills.size()) {
        const CatchChallenger::PlayerMonster::PlayerSkill &skill =
            monster.skills.at(sub_index2);
        if (skill.skill == learn.learnSkill) break;
        sub_index2++;
      }
      if (
          // if skill not found
          (sub_index2 == monster.skills.size() && learn.learnSkillLevel == 1) ||
          // if skill already found and need level up
          (sub_index2 < monster.skills.size() &&
           (monster.skills.at(sub_index2).level + 1) ==
               learn.learnSkillLevel)) {
        if (skillToDisplay.contains(learn.learnSkill)) {
          if (skillToDisplay.value(learn.learnSkill) > learn.learnSkillLevel)
            skillToDisplay[learn.learnSkill] = learn.learnSkillLevel;
        } else
          skillToDisplay[learn.learnSkill] = learn.learnSkillLevel;
      }
    }
    sub_index++;
  }
  QHashIterator<uint16_t, uint8_t> i(skillToDisplay);
  while (i.hasNext()) {
    i.next();
    auto *item = UI::IconItem::Create();
    item->SetData(99, i.key());
    const uint32_t &level = i.value();
    if (CommonSettingsServer::commonSettingsServer.useSP) {
      if (level <= 1)
        item->SetText(
            QObject::tr("%1\nSP cost: %2")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                        .at(i.key())
                        .name))
                .arg(CatchChallenger::CommonDatapack::commonDatapack
                         .get_monsterSkills().at(i.key())
                         .level.at(i.value() - 1)
                         .sp_to_learn));
      else
        item->SetText(
            QObject::tr("%1 level %2\nSP cost: %3")
                .arg(QString::fromStdString(
                    QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                        .at(i.key())
                        .name))
                .arg(level)
                .arg(CatchChallenger::CommonDatapack::commonDatapack
                         .get_monsterSkills().at(i.key())
                         .level.at(i.value() - 1)
                         .sp_to_learn));
    } else {
      if (level <= 1) {
        item->SetText(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_monsterSkillsExtra()
                .at(i.key())
                .name));
      } else {
        item->SetText(QObject::tr("%1 level %2")
                          .arg(QString::fromStdString(
                                   QtDatapackClientLoader::datapackLoader
                                       ->get_monsterSkillsExtra().at(i.key())
                                       .name)
                                   .arg(level)));
      }
    }
    if (CommonSettingsServer::commonSettingsServer.useSP &&
        CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills()
                .at(i.key())
                .level.at(i.value() - 1)
                .sp_to_learn > monster.sp) {
      // item->setFont(MissingQuantity);
      // item->setForeground(QBrush(QColor(200, 20, 20)));
      item->SetToolTip(QObject::tr("You need more sp"));
    }
    // attack_to_learn_graphical[item] = i.key();
    content_->AddItem(item);
  }
  if (content_->Count() == 0) {
    description_->SetText(QObject::tr("No more attack to learn"));
  } else {
    description_->SetText(QObject::tr("Select attack to learn"));
  }
}
