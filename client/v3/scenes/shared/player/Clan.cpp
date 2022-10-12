// Copyright 2021 CatchChallenger
#include "Clan.hpp"

#include <QPainter>
#include <iostream>

#include "../../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../../Globals.hpp"
#include "../../../base/ConnectionManager.hpp"
#include "../../../entities/PlayerInfo.hpp"
#include "../../../ui/LinkedDialog.hpp"
#include "../../../ui/ThemedButton.hpp"

using Scenes::Clan;

Clan::Clan() {
  action_buttons_ = UI::Row::Create(this);

  clan_label_ = UI::Label::Create(this);
  clan_value_ = UI::Label::Create(this);
  admin_label_ = UI::Label::Create(this);
  status_label_ = UI::Label::Create(this);

  leave_clan_ = UI::YellowButton::Create(this);

  invite_clan_ = UI::YellowButton::Create();
  disolve_clan_ = UI::YellowButton::Create();
  expulse_clan_ = UI::YellowButton::Create();

  action_buttons_->AddChild(invite_clan_);
  action_buttons_->AddChild(disolve_clan_);
  action_buttons_->AddChild(expulse_clan_);

  leave_clan_->SetOnClick([this](Node *) {
    ConnectionManager::GetInstance()->client->leaveClan();
    static_cast<UI::LinkedDialog *>(Parent())->Close();
  });
  invite_clan_->SetOnClick([this](Node *) {
    Globals::GetInputDialog()->ShowInputText(
        QString("Invite player"), QString("Player name"),
        [this](QString pseudo) {
          if (pseudo.isEmpty()) return;
          ConnectionManager::GetInstance()->client->inviteClan(
              pseudo.toStdString());
        },
        QString());
  });
  disolve_clan_->SetOnClick([this](Node *) {
    ConnectionManager::GetInstance()->client->dissolveClan();
    static_cast<UI::LinkedDialog *>(Parent())->Close();
  });
  expulse_clan_->SetOnClick([this](Node *) {
    Globals::GetInputDialog()->ShowInputText(
        QString("Eject player"), QString("Player name"),
        [this](QString pseudo) {
          if (pseudo.isEmpty()) return;
          ConnectionManager::GetInstance()->client->ejectClan(
              pseudo.toStdString());
        },
        QString());
  });

  newLanguage();
}

Clan::~Clan() {
  delete action_buttons_;
  delete clan_label_;
  delete clan_value_;
  delete admin_label_;
  delete status_label_;
  delete leave_clan_;
  delete invite_clan_;
  delete disolve_clan_;
  delete expulse_clan_;

  action_buttons_ = nullptr;
  clan_label_ = nullptr;
  clan_value_ = nullptr;
  admin_label_ = nullptr;
  status_label_ = nullptr;
  leave_clan_ = nullptr;
  invite_clan_ = nullptr;
  disolve_clan_ = nullptr;
  expulse_clan_ = nullptr;
}

Clan *Clan::Create() { return new (std::nothrow) Clan(); }

void Clan::OnResize() {
  auto bounding = BoundingRect();

  clan_label_->SetPos(0, 0);
  clan_value_->SetPos(clan_label_->Width(), 0);
  leave_clan_->SetSize(UI::Button::kRectSmall);
  leave_clan_->SetPos(bounding.width() - leave_clan_->Width(), 0);

  admin_label_->SetPos(bounding.width() / 2 - admin_label_->Width() / 2, 150);

  invite_clan_->SetSize(UI::Button::kRectSmall);
  disolve_clan_->SetSize(UI::Button::kRectSmall);
  expulse_clan_->SetSize(UI::Button::kRectSmall);

  action_buttons_->SetPos(bounding.width() / 2 - action_buttons_->Width() / 2,
                          bounding.height() - action_buttons_->Height());
}

void Clan::newLanguage() {
  clan_label_->SetText(QObject::tr("Name: "));
  admin_label_->SetText(QObject::tr("Leader"));
  status_label_->SetText(QObject::tr("No clan"));

  leave_clan_->SetText(QObject::tr("Leave"));
  invite_clan_->SetText(QObject::tr("Invite a player"));
  disolve_clan_->SetText(QObject::tr("Disolve"));
  expulse_clan_->SetText(QObject::tr("Eject a player"));
}

void Clan::RegisterEvents() {
  static_cast<UI::LinkedDialog *>(Parent())->SetTitle(QObject::tr("CLAN"));
  const auto &info = PlayerInfo::GetInstance()->GetInformationRO();
  auto player_info = PlayerInfo::GetInstance();

  bool has_clan = info.clan != 0;

  clan_label_->SetVisible(has_clan);
  clan_value_->SetVisible(has_clan);
  status_label_->SetVisible(!has_clan);
  leave_clan_->SetVisible(has_clan);

  admin_label_->SetVisible(has_clan && info.clan_leader);
  invite_clan_->SetVisible(has_clan && info.clan_leader);
  disolve_clan_->SetVisible(has_clan && info.clan_leader);
  expulse_clan_->SetVisible(has_clan && info.clan_leader);
  if (!has_clan) {
    return;
  }

  clan_value_->SetText(player_info->ClanName);

  leave_clan_->RegisterEvents();
  if (info.clan_leader) {
    invite_clan_->RegisterEvents();
    disolve_clan_->RegisterEvents();
    expulse_clan_->RegisterEvents();
  }
}

void Clan::UnRegisterEvents() {
  leave_clan_->UnRegisterEvents();
  invite_clan_->UnRegisterEvents();
  disolve_clan_->UnRegisterEvents();
  expulse_clan_->UnRegisterEvents();
}

void Clan::Draw(QPainter *painter) { (void)painter; }
