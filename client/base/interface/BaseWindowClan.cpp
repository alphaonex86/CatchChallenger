#include "BaseWindow.h"
#include "ui_BaseWindow.h"

#include <QInputDialog>

using namespace CatchChallenger;

void BaseWindow::clanActionSuccess(const uint32_t &clanId)
{
    switch(actionClan.first())
    {
        case ActionClan_Create:
            if(clan==0)
            {
                clan=clanId;
                clan_leader=true;
            }
            updateClanDisplay();
            showTip(tr("The clan is created"));
        break;
        case ActionClan_Leave:
        case ActionClan_Dissolve:
            clan=0;
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
    haveClanInformations=false;
    clanName.clear();
    clan=0;
    updateClanDisplay();
}

void BaseWindow::updateClanDisplay()
{
    ui->tabWidgetTrainerCard->setTabEnabled(4,clan!=0);
    ui->clanGrouBoxNormal->setVisible(!clan_leader);
    ui->clanGrouBoxLeader->setVisible(clan_leader);
    ui->clanGrouBoxInformations->setVisible(haveClanInformations);
    if(haveClanInformations)
    {
        if(clanName.isEmpty())
            ui->clanName->setText(tr("Your clan"));
        else
            ui->clanName->setText(clanName);
    }
    if(Chat::chat!=NULL)
        Chat::chat->setClan(clan!=0);
}

void BaseWindow::on_clanActionLeave_clicked()
{
    actionClan << ActionClan_Leave;
    CatchChallenger::Api_client_real::client->leaveClan();
}

void BaseWindow::on_clanActionDissolve_clicked()
{
    actionClan << ActionClan_Dissolve;
    CatchChallenger::Api_client_real::client->dissolveClan();
}

void BaseWindow::on_clanActionInvite_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Invite;
        CatchChallenger::Api_client_real::client->inviteClan(text);
    }
}

void BaseWindow::on_clanActionEject_clicked()
{
    bool ok;
    QString text = QInputDialog::getText(this,tr("Give the player pseudo"),tr("Player pseudo to invite:"),QLineEdit::Normal,QString(), &ok);
    if(ok && !text.isEmpty())
    {
        actionClan << ActionClan_Eject;
        CatchChallenger::Api_client_real::client->ejectClan(text);
    }
}

void BaseWindow::clanInformations(const QString &name)
{
    haveClanInformations=true;
    clanName=name;
    updateClanDisplay();
}

void BaseWindow::clanInvite(const uint32_t &clanId,const QString &name)
{
    QMessageBox::StandardButton button=QMessageBox::question(this,tr("Invite"),tr("The clan %1 invite you to become a member. Do you accept?").arg(QStringLiteral("<b>%1</b>").arg(name)));
    CatchChallenger::Api_client_real::client->inviteAccept(button==QMessageBox::Yes);
    if(button==QMessageBox::Yes)
    {
        this->clan=clanId;
        this->clan_leader=false;
        haveClanInformations=false;
        updateClanDisplay();
    }
}
