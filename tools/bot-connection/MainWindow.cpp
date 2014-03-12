#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "../../general/base/CommonSettings.h"

#include <QNetworkProxy>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    sslSocket(new QSslSocket()),
    socket(sslSocket),
    api(&socket,QString()),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    ui->setupUi(this);
    sslSocket->ignoreSslErrors();
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&api,&CatchChallenger::Api_client_virtual::insert_player,            this,&MainWindow::insert_player);
    connect(&api,&CatchChallenger::Api_client_virtual::new_chat_text,            this,&MainWindow::new_chat_text,Qt::QueuedConnection);
    connect(&api,&CatchChallenger::Api_client_virtual::haveCharacter,            this,&MainWindow::haveCharacter);
    connect(&api,&CatchChallenger::Api_client_virtual::logged,                   this,&MainWindow::logged);
    connect(&api,&CatchChallenger::Api_client_virtual::have_current_player_info, this,&MainWindow::have_current_player_info);
    connect(&api,&CatchChallenger::Api_client_virtual::newError,                 this,&MainWindow::newError);
    connect(&socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MainWindow::newSocketError);
    connect(&socket,&CatchChallenger::ConnectedSocket::disconnected,             this,&MainWindow::disconnected);
    connect(&socket,&CatchChallenger::ConnectedSocket::connected,                this,&MainWindow::tryLink,Qt::QueuedConnection);

    details=false;
    haveShowDisconnectionReason=false;
    have_informations=false;
    last_direction=CatchChallenger::Direction_look_at_bottom;
    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doMove);
    connect(&moveTimer,&QTimer::timeout,this,&MainWindow::doText);
    moveTimer.start(1000);
    textTimer.start(1000);

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
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::disconnected()
{
    if(!haveShowDisconnectionReason)
    {
        ui->statusBar->showMessage(tr("Disconnected by the host"));
        ui->connect->setEnabled(true);
    }
    haveShowDisconnectionReason=false;
}

void MainWindow::tryLink()
{
    api.startReadData();
    api.sendProtocol();
    api.tryLogin(ui->login->text(),ui->pass->text());
}

void MainWindow::doMove()
{
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket.isValid():%2, map!=NULL: %3").arg(do_step).arg(socket.isValid()).arg(map!=NULL));
    if(have_informations && ui->move->isChecked() && socket.isValid())
    {
        if(last_direction==CatchChallenger::Direction_look_at_bottom)
        {
            last_direction=CatchChallenger::Direction_look_at_left;
            send_player_move(0,last_direction);
        }
        else if(last_direction==CatchChallenger::Direction_look_at_left)
        {
            last_direction=CatchChallenger::Direction_look_at_top;
            send_player_move(0,last_direction);
        }
        else if(last_direction==CatchChallenger::Direction_look_at_top)
        {
            last_direction=CatchChallenger::Direction_look_at_right;
            send_player_move(0,last_direction);
        }
        else
        {
            last_direction=CatchChallenger::Direction_look_at_bottom;
            send_player_move(0,last_direction);
        }
    }
}

void MainWindow::doText()
{
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket.isValid():%2, map!=NULL: %3").arg(do_step).arg(socket.isValid()).arg(map!=NULL));
    if(have_informations && ui->move->isChecked() && socket.isValid())
    {
        if(CommonSettings::commonSettings.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    api.sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    api.sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    api.sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettings::commonSettings.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        api.sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        api.sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        api.sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        api.sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

//quint32,QString,quint16,quint16,quint8,quint16
void MainWindow::insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    ui->statusBar->showMessage(tr("On the map"));
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    if(player.simplifiedId==api.getId())
        this->last_direction=direction;
    have_informations=true;
}

void MainWindow::haveCharacter()
{
    ui->statusBar->showMessage(QStringLiteral("Now on the map"));
}

void MainWindow::logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList)
{
    ui->characterList->clear();
    int index=0;
    while(index<characterEntryList.size())
    {
        const CatchChallenger::CharacterEntry &character=characterEntryList.at(index);
        ui->characterList->addItem(character.pseudo,character.character_id);
        index++;
    }
    ui->characterList->setEnabled(ui->characterList->count()>0);
    ui->characterSelect->setEnabled(ui->characterList->count()>0);
}

void MainWindow::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    Q_UNUSED(informations);
//    DebugClass::debugConsole(QStringLiteral("MainWindow::have_current_player_info() pseudo: %1").arg(informations.public_informations.pseudo));
}

void MainWindow::newError(QString error,QString detailedError)
{
    haveShowDisconnectionReason=true;
    ui->statusBar->showMessage(QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError));
    CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1, detailedError: %2").arg(error).arg(detailedError));
    socket.disconnectFromHost();
}

void MainWindow::newSocketError(QAbstractSocket::SocketError error)
{
    haveShowDisconnectionReason=true;
    if(error==0)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() Connection refused").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Connection refused").arg(error));
    }
    else if(error==13)
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() SslHandshakeFailedError"));
        ui->statusBar->showMessage(QStringLiteral("SslHandshakeFailedError"));
    }
    else
    {
        CatchChallenger::DebugClass::debugConsole(QStringLiteral("MainWindow::newError() error: %1").arg(error));
        ui->statusBar->showMessage(QStringLiteral("Error: %1").arg(error));
    }

}

void MainWindow::send_player_move(const quint8 &moved_unit,const CatchChallenger::Direction &the_new_direction)
{
    api.send_player_move(moved_unit,the_new_direction);
}

void MainWindow::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    qDebug() << "Chat:" << text;
    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettings::commonSettings.chat_allow_all && ui->chatRandomReply->isChecked())
            switch(rand()%100)
            {
                case 0:
                    api.sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettings::commonSettings.chat_allow_local && ui->chatRandomReply->isChecked())
            switch(rand()%3)
            {
                case 0:
                    api.sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettings::commonSettings.chat_allow_private)
            api.sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        break;
        default:
        break;
    }
}

void MainWindow::stop_move()
{
    have_informations=false;
}

void MainWindow::show_details()
{
    details=true;
}

void MainWindow::on_connect_clicked()
{
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
    settings.setValue("login",ui->login->text());
    settings.setValue("pass",ui->pass->text());
    settings.setValue("host",ui->host->text());
    settings.setValue("port",ui->port->value());
    settings.setValue("proxy",ui->proxy->text());
    settings.setValue("proxyport",ui->proxyport->value());

    if(!ui->connect->isEnabled())
        return;
    ui->connect->setEnabled(false);

    QNetworkProxy proxy;
    if(!ui->proxy->text().isEmpty())
    {
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(ui->proxy->text());
        proxy.setPort(ui->proxyport->value());
    }
    sslSocket->setProxy(proxy);
    socket.connectToHost(ui->host->text(),ui->port->value());
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << "Ssl error:" << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
    /*QMessageBox::warning(this,tr("Ssl error"),sslErrors.join("\n"));
    realSocket->disconnectFromHost();*/
}

void MainWindow::on_characterSelect_clicked()
{
    if(ui->characterList->count()<=0 || !ui->characterSelect->isEnabled())
        return;
    const quint32 &charId=ui->characterList->currentData().toUInt();
    api.selectCharacter(charId);
    ui->characterSelect->setEnabled(false);
    ui->characterList->setEnabled(false);
}
