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
        ui->question->setText(tr("Do you want battle with the player %1?").arg(QStringLiteral("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Battle"));
        break;
        case WithAnotherPlayerType_Trade:
        ui->question->setText(tr("Do you want trade with the player %1?").arg(QStringLiteral("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Trade"));
        break;
        default:
        ui->question->setText(tr("Do you want do an action with the player %1?").arg(QStringLiteral("<b>%1</b>").arg(pseudo)));
        setWindowTitle(tr("Action"));
        break;
    }
    time.restart();
    connect(&timer,&QTimer::timeout,this,&WithAnotherPlayer::updateTiemout);
    timer.start(200);
}

WithAnotherPlayer::~WithAnotherPlayer()
{
    delete ui;
}

void WithAnotherPlayer::on_yes_clicked()
{
    timer.stop();
    actionAccepted=true;
    close();
}

void WithAnotherPlayer::on_no_clicked()
{
    timer.stop();
    close();
}

bool WithAnotherPlayer::actionIsAccepted()
{
    return actionAccepted;
}

void WithAnotherPlayer::updateTiemout()
{
    const uint64_t timems=time.elapsed();
    if(timems>15*1000)
        on_no_clicked();
    else
    {
        ui->timeout->setValue(15*1000-timems);
        ui->timeoutLabel->setText(QString::number(15-timems/1000)+"s");
    }
}
