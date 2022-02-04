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
  clan_label_ = UI::Label::Create(this);
  clan_value_ = UI::Label::Create(this);
  admin_label_ = UI::Label::Create(this);
  status_label_ = UI::Label::Create(this);

  leave_clan_ = UI::YellowButton::Create(this);
  invite_clan_ = UI::YellowButton::Create(this);
  disolve_clan_ = UI::YellowButton::Create(this);
  expulse_clan_ = UI::YellowButton::Create(this);

  leave_clan_->SetOnClick([this](Node *) {
    ConnectionManager::GetInstance()->client->leaveClan();
    static_cast<UI::LinkedDialog *>(Parent())->Close();
  });
  invite_clan_->SetOnClick([this](Node *) {
    Globals::GetInputDialog()->ShowInputText(
        QString("Invite player"),
        QString("Player name"),
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
        QString("Eject player"),
        QString("Player name"),
        [this](QString pseudo) {
          if (pseudo.isEmpty()) return;
          ConnectionManager::GetInstance()->client->ejectClan(
              pseudo.toStdString());
        },
        QString());
  });

  newLanguage();
}

Clan::~Clan() {}

Clan *Clan::Create() { return new (std::nothrow) Clan(); }

void Clan::OnResize() {
  auto bounding = BoundingRect();

  clan_label_->SetPos(0, 0);
  clan_value_->SetPos(clan_label_->Width(), 0);
  leave_clan_->SetSize(150, 40);
  leave_clan_->SetPixelSize(14);
  leave_clan_->SetPos(bounding.width() - leave_clan_->Width(), 0);

  admin_label_->SetPos(bounding.width() / 2 - admin_label_->Width() / 2, 150);
  invite_clan_->SetPos(bounding.width() / 2 - 235,
                       admin_label_->Y() + admin_label_->Height() + 10);
  invite_clan_->SetSize(150, 40);
  invite_clan_->SetPixelSize(14);
  disolve_clan_->SetPos(invite_clan_->X() + invite_clan_->Width() + 10,
                        invite_clan_->Y());
  disolve_clan_->SetSize(150, 40);
  disolve_clan_->SetPixelSize(14);
  expulse_clan_->SetPos(disolve_clan_->X() + disolve_clan_->Width() + 10,
                        invite_clan_->Y());
  expulse_clan_->SetSize(150, 40);
  expulse_clan_->SetPixelSize(14);
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
