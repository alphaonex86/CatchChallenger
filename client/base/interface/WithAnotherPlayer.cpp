#include "WithAnotherPlayer.h"
#include "ui_WithAnotherPlayer.h"

WithAnotherPlayer::WithAnotherPlayer(QWidget *parent,const WithAnotherPlayerType &type,const QPixmap &skin,const QString &pseudo) :
    QDialog(parent),
    ui(new Ui::WithAnotherPlayer)
{
    actionAccepted=false;
    ui->setupUi(this);
    QPixmap newSkin=skin.scaledToHeight(160);
    ui->skin->setPixmap(newSkin);
    switch(type)
    {
        case WithAnotherPlayerType_Battle:
        ui->question->setText(tr("Do you want battle with the player %1?").arg(QString("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Battle"));
        break;
        case WithAnotherPlayerType_Trade:
        ui->question->setText(tr("Do you want trade with the player %1?").arg(QString("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Trade"));
        break;
        default:
        ui->question->setText(tr("Do you want do an action with the player %1?").arg(QString("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Action"));
        break;
    }
}

WithAnotherPlayer::~WithAnotherPlayer()
{
    delete ui;
}

void WithAnotherPlayer::on_yes_clicked()
{
    actionAccepted=true;
    close();
}

void WithAnotherPlayer::on_no_clicked()
{
    close();
}

bool WithAnotherPlayer::actionIsAccepted()
{
    return actionAccepted;
}
