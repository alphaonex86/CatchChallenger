// Copyright 2021 CatchChallenger
#include "Reputations.hpp"

#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/LinkedDialog.hpp"

using Scenes::Reputations;

Reputations::Reputations() {
  list_ = UI::ListView::Create(this);
  list_->SetItemSpacing(5);
}

Reputations::~Reputations() {
  delete list_;
  list_ = nullptr;
}

Reputations *Reputations::Create() { return new (std::nothrow) Reputations(); }

void Reputations::OnResize() {
  auto bounding = BoundingRect();
  list_->SetPos(bounding.x(), bounding.y());
  list_->SetSize(bounding.width(), bounding.height());
}

void Reputations::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(
      QObject::tr("REPUTATIONS"));

  LoadReputations();
}

void Reputations::UnRegisterEvents() {}

void Reputations::LoadReputations() {
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  list_->Clear();
  if (info.reputation.size() == 0) {
    auto label = UI::Label::Create();
    label->SetText(QObject::tr("Empty"));
    list_->AddItem(label);
    return;
  }
  auto i = info.reputation.begin();
  while (i != info.reputation.cend()) {
    if (i->second.level >= 0) {
      if ((i->second.level + 1) ==
          (int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
              .at(i->first)
              .reputation_positive.size()) {
        auto label = UI::Label::Create();
        label->SetText(QStringLiteral("100% %1").arg(QString::fromStdString(
            QtDatapackClientLoader::GetInstance()->get_reputationExtra()
                .at(CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                        .at(i->first)
                        .name)
                .reputation_positive.back())));
        list_->AddItem(label);
      } else {
        int32_t next_level_xp =
            CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                .at(i->first)
                .reputation_positive.at(i->second.level + 1);
        if (next_level_xp == 0) {
          // error("Next level can't be need 0 xp");
          return;
        } else {
          std::string text =
              QtDatapackClientLoader::GetInstance()->get_reputationExtra()
                  .at(CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                          .at(i->first)
                          .name)
                  .reputation_positive.at(i->second.level);
          auto label = UI::Label::Create();
          label->SetText(QStringLiteral("%1% %2")
                             .arg((i->second.point * 100) / next_level_xp)
                             .arg(QString::fromStdString(text)));
          list_->AddItem(label);
        }
      }
    } else {
      if ((-i->second.level) ==
          (int32_t)CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
              .at(i->first)
              .reputation_negative.size()) {
        auto label = UI::Label::Create();
        label->SetText(QStringLiteral("100% %1").arg(QString::fromStdString(
            QtDatapackClientLoader::GetInstance()->get_reputationExtra()
                .at(CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                        .at(i->first)
                        .name)
                .reputation_negative.back())));
        list_->AddItem(label);
      } else {
        int32_t next_level_xp =
            CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                .at(i->first)
                .reputation_negative.at(-i->second.level);
        if (next_level_xp == 0) {
          // error("Next level can't be need 0 xp");
          return;
        } else {
          std::string text =
              QtDatapackClientLoader::GetInstance()->get_reputationExtra()
                  .at(CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                          .at(i->first)
                          .name)
                  .reputation_negative.at(-i->second.level - 1);
          auto label = UI::Label::Create();
          label->SetText(QStringLiteral("%1% %2")
                             .arg((i->second.point * 100) / next_level_xp)
                             .arg(QString::fromStdString(text)));
          list_->AddItem(label);
        }
      }
    }
    ++i;
  }
}

void Reputations::Draw(QPainter *painter) { (void)painter; }
