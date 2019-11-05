#include "BaseWindow.h"
#include "ui_BaseWindow.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::clanActionSuccess(const uint32_t &clanId)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
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
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void BaseWindow::clanActionFailed()
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
        newError(tr("Internal error").toStdString()+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.erase(actionClan.cbegin());
}

void BaseWindow::clanDissolved()
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    haveClanInformations=false;
    clanName.clear();
    playerInformations.clan=0;
    updateClanDisplay();
}

void BaseWindow::updateClanDisplay()
{
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();//do a crash due to reference
    //const CatchChallenger::Player_private_and_public_informations playerInformations=client->get_player_informations_ro();
    ui->tabWidgetTrainerCard->setTabEnabled(4,playerInformations.clan!=0);
    ui->clanGrouBoxNormal->setVisible(!playerInformations.clan_leader);
    ui->clanGrouBoxLeader->setVisible(playerInformations.clan_leader);
    ui->clanGrouBoxInformations->setVisible(haveClanInformations);
    if(haveClanInformations)
    {
        if(clanName.empty())
            ui->clanName->setText(tr("Your clan"));
        else
            ui->clanName->setText(QString::fromStdString(clanName));
    }
    chat->setClan(playerInformations.clan!=0);
}

void BaseWindow::on_clanActionLeave_clicked()
{
    actionClan.push_back(ActionClan_Leave);
    client->leaveClan();
}

void BaseWindow::on_clanActionDissolve_clicked()
{
    actionClan.push_back(ActionClan_Dissolve);
    client->dissolveClan();
}

void BaseWindow::on_clanActionInvite_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok).toStdString();
    if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Invite);
        client->inviteClan(text);
    }
}

void BaseWindow::on_clanActionEject_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok).toStdString();
    if(ok && !text.empty())
    {
        actionClan.push_back(ActionClan_Eject);
        client->ejectClan(text);
    }
}

void BaseWindow::clanInformations(const std::string &name)
{
    haveClanInformations=true;
    clanName=name;
    updateClanDisplay();
}

void BaseWindow::clanInvite(const uint32_t &clanId,const std::string &name)
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?")
                                                             .arg(QStringLiteral("<b>%1</b>").arg(QString::fromStdString(name))));
    client->inviteAccept(button==QMessageBox::Yes);
    if(button==QMessageBox::Yes)
    {
        Player_private_and_public_informations &playerInformations=client->get_player_informations();
        playerInformations.clan=clanId;
        playerInformations.clan_leader=false;
        haveClanInformations=false;
        updateClanDisplay();
    }
}
