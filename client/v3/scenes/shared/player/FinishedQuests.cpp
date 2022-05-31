// Copyright 2021 CatchChallenger
#include "FinishedQuests.hpp"

#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../Ultimate.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "../../../ui/Row.hpp"

using Scenes::FinishedQuests;

FinishedQuests::FinishedQuests() { quests_ = UI::ListView::Create(this); }

FinishedQuests::~FinishedQuests() {
  delete quests_;
  quests_ = nullptr;
}

FinishedQuests *FinishedQuests::Create() {
  return new (std::nothrow) FinishedQuests();
}

void FinishedQuests::OnResize() {
  auto bounding = BoundingRect();

  quests_->SetPos(0, 0);
  quests_->SetSize(bounding.width(), bounding.height());
}

void FinishedQuests::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(
      QObject::tr("FINISHED QUESTS"));
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();

  quests_->RegisterEvents();

  bool has_quests = false;
  quests_->Clear();
  auto i = info.quests.begin();
  while (i != info.quests.cend()) {
    has_quests = true;
    const uint16_t questId = i->first;
    const CatchChallenger::PlayerQuest &value = i->second;
    if (QtDatapackClientLoader::GetInstance()->get_questsExtra().find(questId) !=
        QtDatapackClientLoader::GetInstance()->get_questsExtra().cend()) {
      if (value.step == 0 || value.finish_one_time) {
        if (Ultimate::ultimate.isUltimate()) {
          auto row = UI::Row::Create();
          if (CatchChallenger::CommonDatapackServerSpec::
                  commonDatapackServerSpec.get_quests().at(questId)
                      .repeatable) {
            auto icon =
                Sprite::Create(":/CC/images/interface/repeatable.png", row);
          }
          if (value.step > 0) {
            auto icon =
                Sprite::Create(":/CC/images/interface/in-progress.png", row);
          }
          auto label = UI::Label::Create(row);
          label->SetText(
              QtDatapackClientLoader::GetInstance()->get_questsExtra().at(questId)
                  .name);
          quests_->AddItem(row);
        } else {
          auto label = UI::Label::Create();
          label->SetText(
              QtDatapackClientLoader::GetInstance()->get_questsExtra().at(questId)
                  .name);
          quests_->AddItem(label);
        }
      }
    }
    ++i;
  }
  if (!has_quests) {
    auto label = UI::Label::Create();
    label->SetText(QObject::tr("You have no finished quests"));
    quests_->AddItem(label);
  }
}

void FinishedQuests::UnRegisterEvents() {
  quests_->UnRegisterEvents();
}

void FinishedQuests::Draw(QPainter *painter) { (void)painter; }
