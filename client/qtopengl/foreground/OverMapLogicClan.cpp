#include "OverMapLogic.hpp"
#include "../ConnexionManager.hpp"
#include "../Language.hpp"
#include "../cc/QtDatapackClientLoader.hpp"
#include "../../../general/base/CommonDatapackServerSpec.hpp"
#include <iostream>
#include <QDesktopServices>

void OverMapLogic::clanActionSuccess(const uint32_t &clanId)
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    switch(actionClan.front())
    {
        case ActionClan_Create:
            if(playerInformations.clan==0)
            {
                playerInformations.clan=clanId;
                playerInformations.clan_leader=true;
            }
            updateClanDisplay();
            showTip(tr("The clan is created").toStdString());
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
            playerInformations.clan=0;
            updateClanDisplay();
            showTip(tr("You are leaved the clan").toStdString());
        break;
        case ActionClan_Invite:
            showTip(tr("You have correctly invited the player").toStdString());
        break;
        case ActionClan_Eject:
            showTip(tr("You have correctly ejected the player from clan").toStdString());
        break;
        default:
        error(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::clanActionFailed()
{
    switch(actionClan.front())
    {
        case ActionClan_Create:
            updateClanDisplay();
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
        break;
        case ActionClan_Invite:
            showTip(tr("You have failed to invite the player").toStdString());
        break;
        case ActionClan_Eject:
            showTip(tr("You have failed to eject the player from clan").toStdString());
        break;
        default:
        error(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void OverMapLogic::clanDissolved()
{
    CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations();
    haveClanInformations=false;
    clanName.clear();
    playerInformations.clan=0;
    updateClanDisplay();
}

void OverMapLogic::updateClanDisplay()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=connexionManager->client->get_player_informations_ro();//do a crash due to reference
    //const CatchChallenger::Player_private_and_public_informations playerInformations=client->get_player_informations_ro();
    //nothing to do
}

void OverMapLogic::clanInformations(const std::string &name)
{
    haveClanInformations=true;
    clanName=name;
    updateClanDisplay();
}

void OverMapLogic::clanInvite(const uint32_t &clanId,const std::string &name)
{
    /*todoQMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?")
                                                             .arg(QStringLiteral("<b>%1</b>").arg(QString::fromStdString(name))));
    client->inviteAccept(button==QMessageBox::Yes);
    if(button==QMessageBox::Yes)
    {
        Player_private_and_public_informations &playerInformations=client->get_player_informations();
        playerInformations.clan=clanId;
        playerInformations.clan_leader=false;
        haveClanInformations=false;
        updateClanDisplay();
    }*/
}
