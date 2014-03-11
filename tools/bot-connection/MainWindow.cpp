#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QNetworkProxy>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    sslSocket(new QSslSocket()),
    socket(sslSocket),
    api(&socket,QString()),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sslSocket->ignoreSslErrors();
    sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    connect(&api,&CatchChallenger::Api_client_virtual::insert_player,            this,&MainWindow::insert_player);
    connect(&api,&CatchChallenger::Api_client_virtual::haveCharacter,            this,&MainWindow::haveCharacter);
    connect(&api,&CatchChallenger::Api_client_virtual::logged,                   this,&MainWindow::logged);
    connect(&api,&CatchChallenger::Api_client_virtual::have_current_player_info, this,&MainWindow::have_current_player_info);
    connect(&api,&CatchChallenger::Api_client_virtual::newError,                 this,&MainWindow::newError);
    connect(&socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MainWindow::newSocketError);
    connect(&socket,&CatchChallenger::ConnectedSocket::disconnected,             this,&MainWindow::disconnected);
    connect(&socket,&CatchChallenger::ConnectedSocket::connected,                this,&MainWindow::tryLink,Qt::QueuedConnection);

    details=false;
    haveShowDisconnectionReason=false;
    do_move=false;

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
    if(do_move && socket.isValid())
    {
        random_new_step();
/*		if(rand()%(GlobalServerData::botNumber*10)==0)
            api.sendChatText(Chat_type_local,"Hello world!");*/
    }
}

void MainWindow::start_step()
{
    do_move=true;
}

void MainWindow::random_new_step()
{
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

void MainWindow::stop_move()
{
    do_move=false;
}

void MainWindow::show_details()
{
    details=true;
}

void MainWindow::send_player_move(const quint8 &moved_unit,const CatchChallenger::Direction &the_direction)
{
    api.send_player_move(moved_unit,the_direction);
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
