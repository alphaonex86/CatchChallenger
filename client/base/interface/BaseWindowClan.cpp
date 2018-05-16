#include "BaseWindow.h"
#include "ui_BaseWindow.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::clanActionSuccess(const uint32_t &clanId)
{
    Player_private_and_public_informations &playerInformations=client->get_player_informations();
    switch(actionClan.first())
    {
        case ActionClan_Create:
            if(playerInformations.clan==0)
            {
                playerInformations.clan=clanId;
                playerInformations.clan_leader=true;
            }
            updateClanDisplay();
            showTip(tr("The clan is created"));
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
            playerInformations.clan=0;
            updateClanDisplay();
            showTip(tr("You are leaved the clan"));
        break;
        case ActionClan_Invite:
            showTip(tr("You have correctly invited the player"));
        break;
        case ActionClan_Eject:
            showTip(tr("You have correctly ejected the player from clan"));
        break;
        default:
        newError(tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.removeFirst();
}

void BaseWindow::clanActionFailed()
{
    switch(actionClan.first())
    {
        case ActionClan_Create:
            updateClanDisplay();
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
        break;
        case ActionClan_Invite:
            showTip(tr("You have failed to invite the player"));
        break;
        case ActionClan_Eject:
            showTip(tr("You have failed to eject the player from clan"));
        break;
        default:
        newError(tr("Internal error")+", file: "+QString(__FILE__)+":"+QString::number(__LINE__),"ActionClan unknown");
        return;
    }
    actionClan.removeFirst();
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
    const CatchChallenger::Player_private_and_public_informations &playerInformations=client->get_player_informations_ro();
    ui->tabWidgetTrainerCard->setTabEnabled(4,playerInformations.clan!=0);
    ui->clanGrouBoxNormal->setVisible(!playerInformations.clan_leader);
    ui->clanGrouBoxLeader->setVisible(playerInformations.clan_leader);
    ui->clanGrouBoxInformations->setVisible(haveClanInformations);
    if(haveClanInformations)
    {
        if(clanName.isEmpty())
            ui->clanName->setText(tr("Your clan"));
        else
            ui->clanName->setText(clanName);
    }
    chat->setClan(playerInformations.clan!=0);
}

void BaseWindow::on_clanActionLeave_clicked()
{
    actionClan << ActionClan_Leave;
    client->leaveClan();
}

void BaseWindow::on_clanActionDissolve_clicked()
{
    actionClan << ActionClan_Dissolve;
    client->dissolveClan();
}

void BaseWindow::on_clanActionInvite_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Invite;
        client->inviteClan(text);
    }
}

void BaseWindow::on_clanActionEject_clicked()
{
    bool ok;
    std::string text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Eject;
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
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?").arg(QStringLiteral("<b>%1</b>").arg(name)));
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
