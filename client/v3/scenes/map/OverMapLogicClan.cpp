// Copyright 2021 CatchChallenger
#include <iostream>

#include "../../../../general/base/CommonDatapackServerSpec.hpp"
#include "../../../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../base/ConnectionManager.hpp"
#include "../../entities/PlayerInfo.hpp"
#include "OverMapLogic.hpp"

using Scenes::OverMapLogic;

void OverMapLogic::ClanActionSuccessSlot(const uint32_t &clan_id) {
  CatchChallenger::Player_private_and_public_informations &info =
      PlayerInfo::GetInstance()->GetInformation();
  switch (actionClan.front()) {
    case ActionClan_Create:
      if (info.clan == 0) {
        info.clan = clan_id;
        info.clan_leader = true;
      }
      showTip(tr("The clan is created").toStdString());
      break;
    case ActionClan_Leave:
    case ActionClan_Dissolve:
      info.clan = 0;
      showTip(tr("You are leaved the clan").toStdString());
      break;
    case ActionClan_Invite:
      showTip(tr("You have correctly invited the player").toStdString());
      break;
    case ActionClan_Eject:
      showTip(
          tr("You have correctly ejected the player from clan").toStdString());
      break;
    default:
      error(tr("Internal error").toStdString() + ", file: " +
            std::string(__FILE__) + ":" + std::to_string(__LINE__));
      return;
  }
  actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::ClanActionFailedSlot() {
  switch (actionClan.front()) {
    case ActionClan_Create:
      showTip(tr("The clan is not created").toStdString());
      break;
    case ActionClan_Leave:
    case ActionClan_Dissolve:
      break;
    case ActionClan_Invite:
      showTip(tr("You have failed to invite the player").toStdString());
      break;
    case ActionClan_Eject:
      showTip(
          tr("You have failed to eject the player from clan").toStdString());
      break;
    default:
      error(tr("Internal error").toStdString() + ", file: " +
            std::string(__FILE__) + ":" + std::to_string(__LINE__));
      return;
  }
  actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::ClanDissolvedSlot() {
  CatchChallenger::Player_private_and_public_informations &info =
      PlayerInfo::GetInstance()->GetInformation();
  auto player_info = PlayerInfo::GetInstance();
  player_info->HaveClanInformation = false;
  player_info->ClanName = "";
  info.clan = 0;
}

void OverMapLogic::ClanInformationSlot(const std::string &name) {
  auto info = PlayerInfo::GetInstance();
  info->HaveClanInformation = true;
  info->ClanName = name;
}

void OverMapLogic::ClanInviteSlot(const uint32_t &clan_id, const std::string &name) {
  /*todoQMessageBox::StandardButton
  button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to
  become a member. Do you accept?")
                                                           .arg(QStringLiteral("<b>%1</b>").arg(QString::fromStdString(name))));
  client->inviteAccept(button==QMessageBox::Yes);
  if(button==QMessageBox::Yes)
  {
      Player_private_and_public_informations
  &playerInformations=client->get_player_informations();
      playerInformations.clan=clanId;
      playerInformations.clan_leader=false;
      haveClanInformations=false;
      updateClanDisplay();
  }*/
}
