#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"
#include "../bot/simple/SimpleAction.h"

#include <QNetworkProxy>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    ui->setupUi(this);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::loggedDone,this,&MainWindow::logged,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::statusError,this,&MainWindow::statusError,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::emit_numberOfSelectedCharacter,this,&MainWindow::display_numberOfSelectedCharacter,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::emit_numberOfBotConnected,this,&MainWindow::display_numberOfBotConnected,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::emit_detectSlowDown,this,&MainWindow::detectSlowDown,Qt::QueuedConnection);
    connect(&multipleBotConnexion,&MultipleBotConnection::emit_lastReplyTime,this,&MainWindow::lastReplyTime,Qt::QueuedConnection);
    connect(&slowDownTimer,&QTimer::timeout,&multipleBotConnexion,&MultipleBotConnectionImplFoprGui::detectSlowDown,Qt::QueuedConnection);
    slowDownTimer.start(200);

    multipleBotConnexion.botInterface=new SimpleAction();

    if(settings.contains("login"))
        ui->login->setText(settings.value("login").toString());
    if(settings.contains("pass"))
        ui->pass->setText(settings.value("pass").toString());
    if(settings.contains("host"))
        ui->host->setText(settings.value("host").toString());
    if(settings.contains("port"))
        ui->port->setValue(settings.value("port").toUInt());
    if(settings.contains("proxy"))
        ui->proxy->setText(settings.value("proxy").toString());
    if(settings.contains("proxyport"))
        ui->proxyport->setValue(settings.value("proxyport").toUInt());
    if(settings.contains("multipleConnexion"))
    {
        if(settings.value("multipleConnexion").toUInt()<2)
            ui->multipleConnexion->setChecked(false);
        else
        {
            ui->multipleConnexion->setChecked(true);
            ui->connexionCount->setValue(settings.value("multipleConnexion").toUInt());
        }
        if(settings.contains("connectBySeconds"))
            ui->connectBySeconds->setValue(settings.value("connectBySeconds").toUInt());
        if(settings.contains("maxDiffConnectedSelected"))
            ui->maxDiffConnectedSelected->setValue(settings.value("maxDiffConnectedSelected").toUInt());
    }
    if(settings.contains("autoCreateCharacter"))
        ui->autoCreateCharacter->setChecked(settings.value("autoCreateCharacter").toBool());
}

MainWindow::~MainWindow()
{
    delete multipleBotConnexion.botInterface;
    delete ui;
}

void MainWindow::lastReplyTime(const quint32 &time)
{
    ui->labelLastReplyTime->setText(tr("Last reply time: %1ms").arg(time));
}


void MainWindow::detectSlowDown(QString text)
{
    ui->labelQueryList->setText(text);
/*    quint32 queryCount=0;
    quint32 worseTime=0;
    QHashIterator<CatchChallenger::Api_client_real *,CatchChallengerClient *> i(apiToCatchChallengerClient);
    while (i.hasNext()) {
        i.next();
        const QMap<quint8,QTime> &values=i.key()->getQuerySendTimeList();
        queryCount+=values.size();
        QMapIterator<quint8,QTime> i(values);
        while (i.hasNext()) {
            i.next();
            const quint32 &time=i.value().elapsed();
            if(time>worseTime)
                worseTime=time;
        }
    }
    if(worseTime>3600*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2h").arg(queryCount).arg(worseTime/(3600*1000)));
    else if(worseTime>2*60*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2min").arg(queryCount).arg(worseTime/(60*1000)));
    else if(worseTime>5*1000)
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2s").arg(queryCount).arg(worseTime/(1000)));
    else
        ui->labelQueryList->setText(tr("Running query: %1 Query with worse time: %2ms").arg(queryCount).arg(worseTime));*/
}

void MainWindow::logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList,bool haveTheDatapack)
{
    CatchChallenger::Api_client_real *senderObject = qobject_cast<CatchChallenger::Api_client_real *>(sender());
    if(senderObject==NULL)
        return;

    ui->characterList->clear();
    if(!ui->multipleConnexion->isChecked())
    {
        int index=0;
        while(index<characterEntryList.size())fix this
        {
            const CatchChallenger::CharacterEntry &character=characterEntryList.at(index);
            ui->characterList->addItem(character.pseudo,character.character_id);
            index++;
        }
    }
    ui->characterList->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());
    ui->characterSelect->setEnabled(ui->characterList->count()>0 && !ui->multipleConnexion->isChecked());

    if(!haveTheDatapack)
    {
        if(!CommonSettings::commonSettings.chat_allow_all && !CommonSettings::commonSettings.chat_allow_local)
        {
            ui->randomText->setEnabled(false);
            ui->randomText->setChecked(false);
            ui->chatRandomReply->setEnabled(false);
            ui->chatRandomReply->setChecked(false);
        }
        return;
    }
}

void MainWindow::statusError(QString error)
{
    ui->statusBar->showMessage(error);
}

void MainWindow::on_connect_clicked()
{
    if(!ui->connect->isEnabled())
        return;
    if(ui->pass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    if(ui->login->text().size()<3)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your login need to be at minimum of 3 characters"));
        return;
    }
    ui->groupBox_MultipleConnexion->setEnabled(false);
    ui->groupBox_Proxy->setEnabled(false);
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());
    if(ui->multipleConnexion->isChecked())
        settings.setValue("multipleConnexion",ui->connexionCount->value());
    else
        settings.setValue("multipleConnexion",0);
    settings.setValue("connectBySeconds",ui->connectBySeconds->value());
    settings.setValue("maxDiffConnectedSelected",ui->maxDiffConnectedSelected->value());
    settings.setValue("autoCreateCharacter",ui->autoCreateCharacter->isChecked());

    multipleBotConnexion.mLogin=ui->login->text();
    multipleBotConnexion.mPass=ui->pass->text();
    multipleBotConnexion.mMultipleConnexion=ui->multipleConnexion->isChecked();
    multipleBotConnexion.mAutoCreateCharacter=ui->autoCreateCharacter->isChecked();
    multipleBotConnexion.mConnectBySeconds=ui->connectBySeconds->value();
    multipleBotConnexion.mConnexionCount=ui->connexionCount->value();
    multipleBotConnexion.mMaxDiffConnectedSelected=ui->maxDiffConnectedSelected->value();
    multipleBotConnexion.mProxy=ui->proxy->text();
    multipleBotConnexion.mProxyport=ui->proxyport->value();
    multipleBotConnexion.mHost=ui->host->text();
    multipleBotConnexion.mPort=ui->port->value();

    if(ui->autoCreateCharacter->isChecked())
    {
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
    }
    if(ui->multipleConnexion->isChecked())
    {
        //for the other client
        ui->characterSelect->setEnabled(false);
        ui->characterList->setEnabled(false);
        ui->groupBox_MultipleConnexion->setEnabled(false);
    }
    ui->connect->setEnabled(false);

    //do only the first client to download the datapack
    multipleBotConnexion.createClient();
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    multipleBotConnexion.characterSelect(ui->characterList->currentData().toUInt());
    ui->characterSelect->setEnabled(false);
    ui->characterList->setEnabled(false);
}

void MainWindow::display_numberOfBotConnected(quint16 numberOfBotConnected)
{
    ui->numberOfBotConnected->setText(tr("Number of bot connected: %1").arg(numberOfBotConnected));
}

void MainWindow::display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter)
{
    ui->numberOfSelectedCharacter->setText(tr("Selected character: %1").arg(numberOfSelectedCharacter));
}

void MainWindow::on_move_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("move",checked);
}

void MainWindow::on_randomText_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("randomText",checked);
}

void MainWindow::on_chatRandomReply_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("globalChatRandomReply",checked);
}

void MainWindow::on_bugInDirection_toggled(bool checked)
{
    multipleBotConnexion.botInterface->setValue("bugInDirection",checked);
}
