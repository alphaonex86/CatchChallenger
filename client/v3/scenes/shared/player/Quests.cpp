// Copyright 2021 CatchChallenger
#include "Quests.hpp"

#include <iostream>

#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../FacilityLibClient.hpp"
#include "../../../Globals.hpp"
#include "../../../Ultimate.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../core/AssetsLoader.hpp"
#include "../../../core/Sprite.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../entities/Utils.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "../../../ui/Row.hpp"

using Scenes::QuestItem;
using Scenes::Quests;
using std::placeholders::_1;

Quests::Quests() {
  quests_ = UI::ListView::Create(this);
  quests_->SetItemSpacing(5);
  details_ = UI::ListView::Create(this);
  details_->SetItemSpacing(5);
  cancel_ = UI::Button::Create(":/CC/images/interface/redbutton.png", this);
  cancel_->SetText(QObject::tr("Cancel"));
  cancel_->SetOnClick(std::bind(&Quests::OnCancel, this));
  cancel_->SetSize(100, 30);
  cancel_->SetPixelSize(14);
  cancel_->SetVisible(false);

  selected_item_ = nullptr;
}

Quests::~Quests() {
  delete quests_;
  quests_ = nullptr;

  delete details_;
  details_ = nullptr;

  delete cancel_;
  cancel_ = nullptr;
}

Quests *Quests::Create() { return new (std::nothrow) Quests(); }

void Quests::OnResize() {
  const auto &bounding = BoundingRect();
  const int width = bounding.width() / 2;

  quests_->SetPos(0, 0);
  quests_->SetSize(width, bounding.height());

  details_->SetPos(width + 10, 0);
  details_->SetSize(width, bounding.height() - cancel_->Height());

  cancel_->SetPos(bounding.x() + width + (width / 2) - (cancel_->Width() / 2),
                  bounding.height() - cancel_->Height());
}

void Quests::LoadQuests() {
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  quests_->Clear();
  auto i = info.quests.begin();
  while (i != info.quests.cend()) {
    const uint16_t questId = i->first;
    if (QtDatapackClientLoader::datapackLoader->get_questsExtra().find(questId) !=
        QtDatapackClientLoader::datapackLoader->get_questsExtra().cend()) {
      if (i->second.step > 0) {
        auto item = QuestItem::Create(QString::fromStdString(
            QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId)
                .name));
        item->SetData(99, questId);
        item->SetOnClick(std::bind(&Quests::OnQuestClick, this, _1));
        quests_->AddItem(item);
      }
    }
    ++i;
  }
}

void Quests::OnCancel() {
  if (selected_item_ == nullptr) return;
  ConnectionManager::GetInstance()->client->cancelQuest(
      selected_item_->Data(99));

  auto &info = PlayerInfo::GetInstance()->GetInformation();
  info.quests.erase(selected_item_->Data(99));
  PlayerInfo::GetInstance()->Notify();
  cancel_->SetVisible(false);
  LoadQuests();
  OnQuestClick(nullptr);
}

void Quests::OnQuestItemChanged() {
  const uint8_t font_size = 12;
  details_->Clear();
  if (selected_item_ == nullptr) return;
  cancel_->SetVisible(true);
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  const uint16_t &questId = selected_item_->Data(99);
  if (info.quests.find(questId) == info.quests.cend()) {
    qDebug() << "Selected quest is not into the player list";
    details_->AddItem(CreateLabel(QObject::tr("Select a quest")));
    return;
  }
  const QtDatapackClientLoader::QuestExtra &questExtra =
      QtDatapackClientLoader::datapackLoader->get_questsExtra().at(questId);
  if (info.quests.at(questId).step == 0 ||
      info.quests.at(questId).step > questExtra.steps.size()) {
    qDebug() << "Selected quest step is out of range";
    details_->AddItem(CreateLabel(QObject::tr("Select a quest")));
    return;
  }
  const uint8_t stepQuest = info.quests.at(questId).step - 1;
  {
    if (stepQuest >= questExtra.steps.size()) {
      qDebug() << "no condition match into stepDescriptionList";
      details_->AddItem(CreateLabel(QObject::tr("Select a quest")));
      return;
    } else {
      details_->AddItems(Utils::HTML2Node(QString::fromStdString(
                                              questExtra.steps.at(stepQuest)))
                             .nodes);
    }
  }

  {
    std::vector<CatchChallenger::Quest::Item> items =
        CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
            .get_quests().at(questId)
            .steps.at(info.quests.at(questId).step - 1)
            .requirements.items;

    if (items.size() > 0) {
      details_->AddItem(CreateLabel(QObject::tr("Step requirements:")));
    }

    unsigned int index = 0;
    while (index < items.size()) {
      QPixmap image;
      std::string name;
      if (QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(
              items.at(index).item) !=
          QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend()) {
        image = QtDatapackClientLoader::datapackLoader
                    ->getItemExtra(items.at(index).item)
                    .image;
        name = QtDatapackClientLoader::datapackLoader->get_itemsExtra()
                   .at(items.at(index).item)
                   .name;
      } else {
        image = QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
        name = "id: " + std::to_string(items.at(index).item);
      }

      auto row = UI::Row::Create();
      auto sprite = Sprite::Create(row);
      sprite->SetPixmap(image.scaled(24, 24));
      QString item;

      if (items.at(index).quantity > 1) {
        item = QStringLiteral("%2x %1")
                   .arg(QString::fromStdString(name))
                   .arg(items.at(index).quantity);
      } else {
        item = QString::fromStdString(name);
      }

      auto label = UI::Label::Create(row);
      label->SetText(item);
      label->SetPixelSize(font_size);
      details_->AddItem(row);

      if (index > 16) {
        details_->AddItem(CreateLabel(QString::fromStdString("...")));
        break;
      }
      index++;
    }
  }

  if (questExtra.showRewards || Ultimate::ultimate.isUltimate()) {
    details_->AddItem(CreateLabel(QObject::tr("Final rewards: ")));
    {
      std::vector<CatchChallenger::Quest::Item> items =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().at(questId)
              .rewards.items;
      std::vector<std::string> objects;
      unsigned int index = 0;
      while (index < items.size()) {
        QPixmap image;
        std::string name;
        if (QtDatapackClientLoader::datapackLoader->get_itemsExtra().find(
                items.at(index).item) !=
            QtDatapackClientLoader::datapackLoader->get_itemsExtra().cend()) {
          image = QtDatapackClientLoader::datapackLoader
                      ->getItemExtra(items.at(index).item)
                      .image;
          name = QtDatapackClientLoader::datapackLoader->get_itemsExtra()
                     .at(items.at(index).item)
                     .name;
        } else {
          image =
              QtDatapackClientLoader::datapackLoader->defaultInventoryImage();
          name =
              QStringLiteral("id: %1").arg(items.at(index).item).toStdString();
        }

        auto row = UI::Row::Create();
        auto sprite = Sprite::Create(row);
        sprite->SetPixmap(image.scaled(24, 24));
        QString item;

        if (items.at(index).quantity > 1) {
          item = QStringLiteral("%2x %1")
                     .arg(QString::fromStdString(name))
                     .arg(items.at(index).quantity);
        } else {
          item = QString::fromStdString(name);
        }

        auto label = UI::Label::Create(row);
        label->SetText(item);
        label->SetPixelSize(font_size);
        details_->AddItem(row);

        if (index > 16) {
          details_->AddItem(CreateLabel(QString::fromStdString("...")));
          break;
        }
        index++;
      }
    }
    {
      std::vector<CatchChallenger::ReputationRewards> reputationRewards =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().at(questId)
              .rewards.reputation;
      unsigned int index = 0;
      while (index < reputationRewards.size()) {
        if (QtDatapackClientLoader::datapackLoader->get_reputationExtra().find(
                CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                    .at(reputationRewards.at(index).reputationId)
                    .name) !=
            QtDatapackClientLoader::datapackLoader->get_reputationExtra().cend()) {
          const std::string &reputationName =
              CatchChallenger::CommonDatapack::commonDatapack.get_reputation()
                  .at(reputationRewards.at(index).reputationId)
                  .name;
          if (reputationRewards.at(index).point < 0) {
            details_->AddItem(CreateLabel(
                QObject::tr("Less reputation for %1")
                    .arg(QString::fromStdString(
                        QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                            .at(reputationName)
                            .name))));
          }
          if (reputationRewards.at(index).point > 0) {
            details_->AddItem(CreateLabel(
                QObject::tr("More reputation for %1")
                    .arg(QString::fromStdString(
                        QtDatapackClientLoader::datapackLoader->get_reputationExtra()
                            .at(reputationName)
                            .name))));
          }
        }
        index++;
      }
      if (reputationRewards.size() > 16) {
        details_->AddItem(CreateLabel(QString::fromStdString("...")));
      }
    }
    {
      std::vector<CatchChallenger::ActionAllow> allowRewards =
          CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec
              .get_quests().at(questId)
              .rewards.allow;
      std::vector<std::string> allows;
      unsigned int index = 0;
      while (index < allowRewards.size()) {
        if (vectorcontainsAtLeastOne(allowRewards,
                                     CatchChallenger::ActionAllow_Clan)) {
          details_->AddItem(
              CreateLabel(QObject::tr("Add permission to create clan")));
        }
        index++;
      }
      if (allows.size() > 16) {
        details_->AddItem(CreateLabel(QString::fromStdString("...")));
      }
    }
  }
}

void Quests::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(QObject::tr("QUESTS"));

  quests_->RegisterEvents();
  cancel_->RegisterEvents();
  cancel_->SetVisible(false);
  details_->Clear();
  LoadQuests();
}

void Quests::UnRegisterEvents() {
  quests_->UnRegisterEvents();
  cancel_->UnRegisterEvents();
}

void Quests::OnQuestClick(Node *node) {
  selected_item_ = node;
  auto items = quests_->Items();
  for (auto item : items) {
    if (node == item && node != nullptr) {
      continue;
    }
    static_cast<QuestItem *>(item)->SetSelected(false);
  }
  quests_->ReDraw();
  OnQuestItemChanged();
}

void Quests::Draw(QPainter *painter) { (void)painter; }

UI::Label *Quests::CreateLabel(const QString &text) {
  auto label = UI::Label::Create();
  label->SetText(text);
  label->SetPixelSize(12);
  return label;
}

UI::Label *Quests::CreateLabel(const std::string &text) {
  auto label = UI::Label::Create();
  label->SetText(text);
  label->SetPixelSize(12);
  return label;
}
