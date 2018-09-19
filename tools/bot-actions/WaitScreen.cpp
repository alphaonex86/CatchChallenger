#include "WaitScreen.h"
#include "ui_WaitScreen.h"
#include "../bot/actions/ActionsAction.h"

WaitScreen::WaitScreen(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::WaitScreen)
{
    ui->setupUi(this);
    if(!connect(&updateWaitScreenTimer,&QTimer::timeout,this,&WaitScreen::updateWaitScreen))
        abort();
}

WaitScreen::~WaitScreen()
{
    delete ui;
}

void WaitScreen::updateWaitScreen()
{
    if(this->isVisible())
    {
        if(!updateWaitScreenTimer.isActive())
            updateWaitScreenTimer.start(50);
    }
    else
    {
        if(updateWaitScreenTimer.isActive())
            updateWaitScreenTimer.stop();
        return;
    }
    ui->progressBar->setValue(ActionsAction::actionsAction->elementLoaded());
    ui->progressBar->setMaximum(ActionsAction::actionsAction->elementToLoad());
    ui->labelBottom->setText(QString("%1/%2")
                             .arg(ActionsAction::actionsAction->elementLoaded())
                             .arg(ActionsAction::actionsAction->elementToLoad())
                             );
}
