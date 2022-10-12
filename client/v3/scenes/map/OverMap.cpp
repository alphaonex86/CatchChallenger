// Copyright 2021 CatchChallenger
#include "OverMap.hpp"

#include <QDesktopServices>
#include <iostream>
#include <utility>

#include "../../../libcatchchallenger/ChatParsing.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../Constants.hpp"
#include "../../core/SceneManager.hpp"
#include "../../core/StackedScene.hpp"
#include "../leading/Leading.hpp"

using Scenes::OverMap;

using std::placeholders::_1;

OverMap::OverMap() {
  action_buttons_ = UI::Row::Create(this);
  action_buttons2_ = UI::Row::Create(this);

  connexionManager = ConnectionManager::GetInstance();
  player_counter_ = PlayerCounter::Create(this);

  bag = UI::Button::Create(":/CC/images/interface/bag.png", this);
  menu_ = UI::Button::Create(":/CC/images/interface/menu.png", this);
  buy = UI::Button::Create(":/CC/images/interface/buy.png", this);
  crafting_btn_ = UI::Button::Create(":/CC/images/interface/gift.png", this);

  action_buttons_->AddChild(crafting_btn_);
  action_buttons_->AddChild(bag);
  action_buttons_->AddChild(buy);
  action_buttons_->AddChild(menu_);

  monster_thumb_ = MonsterThumb::Create(this);
  player_portrait_ = PlayerPortrait::Create(this);

  action_buttons2_->AddChild(player_portrait_);
  action_buttons2_->AddChild(monster_thumb_);

  toast_ = Toast::Create(this);
  toast_->SetVisible(false);

  npc_talk_ = NpcTalk::Create();
  chat_ = Chat::Create(this);

  tip_ = Tip::Create(this);
  tip_->SetVisible(false);

  buy->SetOnClick(std::bind(&OverMap::buyClicked, this));
  menu_->SetOnClick(std::bind(&OverMap::menuClicked, this));

  newLanguage();
}

OverMap::~OverMap() {
  delete tip_;
  tip_ = nullptr;
  delete player_portrait_;
  player_portrait_ = nullptr;
  delete chat_;
  chat_ = nullptr;

  delete action_buttons_;
  action_buttons_ = nullptr;

  delete action_buttons2_;
  action_buttons2_ = nullptr;

  delete monster_thumb_;
  monster_thumb_ = nullptr;
}

void OverMap::resetAll() { monster_thumb_->Reset(); }

void OverMap::setVar(CCMap *ccmap) {
  (void)ccmap;
  connect(connexionManager->client,
          &CatchChallenger::Api_protocol_Qt::Qtnumber_of_player, this,
          &OverMap::updatePlayerNumber, Qt::UniqueConnection);
  if (!connexionManager->client->getCaracterSelected()) {
    connect(connexionManager->client,
            &CatchChallenger::Api_protocol_Qt::Qthave_current_player_info, this,
            &OverMap::have_current_player_info, Qt::UniqueConnection);
  } else {
    const auto &info = connexionManager->client->get_player_informations_ro();
    have_current_player_info(info);
  }
}

void OverMap::have_current_player_info(
    const CatchChallenger::Player_private_and_public_informations
        &informations) {
  player_portrait_->SetVisible(true);
  player_portrait_->SetInformation(informations);

  monster_thumb_->LoadMonsters(informations);
  std::pair<uint16_t, uint16_t> t =
      connexionManager->client->getLast_number_of_player();
  player_counter_->UpdateData(t.first, 0);
}

void OverMap::updatePlayerNumber(const uint16_t &number,
                                 const uint16_t &max_players) {
  player_counter_->UpdateData(number, max_players);
}

void OverMap::newLanguage() {}

void OverMap::OnScreenResize() {
  unsigned int space = Constants::ItemMediumSpacing();

  action_buttons2_->SetItemSpacing(space);
  action_buttons2_->SetPos(space, space);

  player_counter_->SetPos(Width() - player_counter_->Width() - space, space);

  buy->SetSize(UI::Button::kRoundMedium);
  menu_->SetSize(UI::Button::kRoundMedium);
  bag->SetSize(UI::Button::kRoundMedium);
  crafting_btn_->SetSize(UI::Button::kRoundMedium);

  action_buttons_->SetItemSpacing(space);
  action_buttons_->SetPos(Width() - action_buttons_->Width() - space,
                          Height() - action_buttons_->Height() - space);

  tip_->SetY(150);
  toast_->SetY(150);

  chat_->SetPos(10, Height() - chat_->Height());
}

void OverMap::OnEnter() {
  Scene::OnEnter();
  menu_->RegisterEvents();
  buy->RegisterEvents();
  tip_->RegisterEvents();
  toast_->RegisterEvents();
  chat_->RegisterEvents();
  monster_thumb_->RegisterEvents();

  if (player_portrait_->IsVisible()) {
    have_current_player_info(PlayerInfo::GetInstance()->GetInformationRO());
  }
}

void OverMap::OnExit() {
  menu_->UnRegisterEvents();
  buy->UnRegisterEvents();
  tip_->UnRegisterEvents();
  toast_->UnRegisterEvents();
  chat_->UnRegisterEvents();
  monster_thumb_->UnRegisterEvents();
}

void OverMap::buyClicked() {
  QDesktopServices::openUrl(tr("https://shop.first-world.info/en/") +
                            "#CatchChallenger");
}

void OverMap::menuClicked() {
  SceneManager::GetInstance()->PopScene();
  static_cast<StackedScene *>(SceneManager::GetInstance()->CurrentScene())
      ->Restart();
}
