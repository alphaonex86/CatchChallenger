#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../base/render/MapVisualiserPlayer.h"
#include "../base/LanguagesSelect.h"
#include "../base/InternetUpdater.h"
#include <QNetworkProxy>

#define SERVER_DNS_OR_IP "catchchallenger.first-world.info"
//#define SERVER_DNS_OR_IP "localhost"
#define SERVER_NAME tr("Official server")
#define SERVER_PORT 42489

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");

    realSocket=new QSslSocket();
    realSocket->ignoreSslErrors();
    realSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    connect(realSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainWindow::sslErrors,Qt::QueuedConnection);
    socket=new CatchChallenger::ConnectedSocket(realSocket);
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_real(socket);
    ui->setupUi(this);
    server_name=SERVER_NAME;
    server_dns_or_ip=SERVER_DNS_OR_IP;
    server_port=SERVER_PORT;
    QString settingsServerPath=QCoreApplication::applicationDirPath()+"/server.conf";
    if(QFile(settingsServerPath).exists())
    {
        QSettings settingsServer(settingsServerPath,QSettings::IniFormat);
        if(settingsServer.contains("server_dns_or_ip") && settingsServer.contains("server_port") && settingsServer.contains("proxy_port"))
        {
            bool ok,ok2;
            quint16 server_port_temp=settingsServer.value("server_port").toString().toUShort(&ok);
            quint16 proxy_port_temp=settingsServer.value("proxy_port").toString().toUShort(&ok2);
            if(settingsServer.value("server_dns_or_ip").toString().contains(QRegularExpression("^([a-zA-Z0-9]{8}\\.onion|.*\\.i2p)$")) && ok && ok2 && server_port_temp>0 && proxy_port_temp>0)
            {
                server_name=tr("Hidden server");
                if(settingsServer.contains("server_name"))
                    server_name=settingsServer.value("server_name").toString();
                server_dns_or_ip=settingsServer.value("server_dns_or_ip").toString();
                proxy_dns_or_ip="localhost";
                server_port=server_port_temp;
                proxy_port=proxy_port_temp;
                if(settingsServer.contains("proxy_dns_or_ip"))
                    proxy_dns_or_ip=settingsServer.value("proxy_dns_or_ip").toString();
                ui->label_login_register->setStyleSheet(ui->label_login_register->styleSheet()+"text-decoration:line-through;");
                ui->label_login_website->setStyleSheet(ui->label_login_website->styleSheet()+"text-decoration:line-through;");
                ui->label_login_register->setText(tr("Register"));
                ui->label_login_website->setText(tr("Web site"));
            }
        }
    }
    ui->update->setVisible(false);
    InternetUpdater::internetUpdater=new InternetUpdater();
    connect(InternetUpdater::internetUpdater,&InternetUpdater::newUpdate,this,&MainWindow::newUpdate);
    CatchChallenger::BaseWindow::baseWindow=new CatchChallenger::BaseWindow();
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    if(settings.contains("login"))
        ui->lineEditLogin->setText(settings.value("login").toString());
    if(settings.contains("pass"))
    {
        ui->lineEditPass->setText(settings.value("pass").toString());
        ui->checkBoxRememberPassword->setChecked(true);
    }
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::protocol_is_good,this,&MainWindow::protocol_is_good);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::message,this,&MainWindow::message);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::have_current_player_info,this,&MainWindow::have_current_player_info);
    connect(socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),this,&MainWindow::error,Qt::QueuedConnection);
    connect(socket,&CatchChallenger::ConnectedSocket::stateChanged,this,&MainWindow::stateChanged,Qt::QueuedConnection);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    CatchChallenger::BaseWindow::baseWindow->connectAllSignals();
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(true);
    connect(CatchChallenger::BaseWindow::baseWindow,&CatchChallenger::BaseWindow::newError,this,&MainWindow::newError,Qt::QueuedConnection);

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle("CatchChallenger - "+server_name);
}

MainWindow::~MainWindow()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
    delete CatchChallenger::Api_client_real::client;
    delete CatchChallenger::BaseWindow::baseWindow;
    delete ui;
    delete socket;
}

void MainWindow::resetAll()
{
    CatchChallenger::Api_client_real::client->resetAll();
    CatchChallenger::BaseWindow::baseWindow->resetAll();
    ui->stackedWidget->setCurrentWidget(ui->page_login);
    chat_list_player_pseudo.clear();
    chat_list_player_type.clear();
    chat_list_type.clear();
    chat_list_text.clear();
    lastMessageSend="";
    if(ui->lineEditLogin->text().isEmpty())
        ui->lineEditLogin->setFocus();
    else if(ui->lineEditPass->text().isEmpty())
        ui->lineEditPass->setFocus();
    else
        ui->pushButtonTryLogin->setFocus();
    //stateChanged(QAbstractSocket::UnconnectedState);//don't call here, else infinity rescursive call
    setWindowTitle("CatchChallenger - "+server_name);
}

void MainWindow::sslErrors(const QList<QSslError> &errors)
{
    haveShowDisconnectionReason=true;
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

void MainWindow::disconnected(QString reason)
{
    QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(reason));
    haveShowDisconnectionReason=true;
    resetAll();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void MainWindow::on_lineEditLogin_returnPressed()
{
    ui->lineEditPass->setFocus();
}

void MainWindow::on_lineEditPass_returnPressed()
{
    on_pushButtonTryLogin_clicked();
}

void MainWindow::on_pushButtonTryLogin_clicked()
{
    if(ui->lineEditPass->text().size()<6)
    {
        QMessageBox::warning(this,tr("Error"),tr("Your password need to be at minimum of 6 characters"));
        return;
    }
    settings.setValue("login",ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
        settings.setValue("pass",ui->lineEditPass->text());
    else
        settings.remove("pass");

    QString host=server_dns_or_ip;
    quint16 port=server_port;

    ui->stackedWidget->setCurrentWidget(CatchChallenger::BaseWindow::baseWindow);
    if(!proxy_dns_or_ip.isEmpty())
    {
        QNetworkProxy proxy=realSocket->proxy();
        proxy.setHostName(proxy_dns_or_ip);
        proxy.setPort(proxy_port);
        proxy.setType(QNetworkProxy::Socks5Proxy);
        realSocket->setProxy(proxy);
    }
    static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->tryConnect(host,port);
    MapController::mapController->setDatapackPath(CatchChallenger::Api_client_real::client->get_datapack_base());
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    qDebug() << "socketState:" << (int)socketState;
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(!isVisible())
        {
            QCoreApplication::quit();
            return;
        }
        resetAll();
    }
    if(socketState==QAbstractSocket::ConnectedState)
        haveShowDisconnectionReason=false;
    CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    qDebug() << "socketError:" << (int)socketError;
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        if(haveShowDisconnectionReason)
            return;
        QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server"));
    break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server"));
    break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long"));
    break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this,tr("Connection closed"),tr("The host address was not found."));
    break;
    case QAbstractSocket::SocketAccessError:
        QMessageBox::information(this,tr("Connection closed"),tr("The socket operation failed because the application lacked the required privileges."));
    break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::information(this,tr("Connection closed"),tr("The local system ran out of resources"));
    break;
    case QAbstractSocket::NetworkError:
        QMessageBox::information(this,tr("Connection closed"),tr("An error occurred with the network"));
    break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        QMessageBox::information(this,tr("Connection closed"),tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)"));
    break;
    case QAbstractSocket::SslHandshakeFailedError:
        QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS handshake failed, so the connection was closed"));
    break;
    default:
        QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError));
    }
}

void MainWindow::haveNewError()
{
//	QMessageBox::critical(this,tr("Error"),client->errorString());
}

void MainWindow::message(QString message)
{
    qDebug() << message;
}

void MainWindow::protocol_is_good()
{
    CatchChallenger::Api_client_real::client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
}

void MainWindow::needQuit()
{
    CatchChallenger::Api_client_real::client->tryDisconnect();
}

void MainWindow::have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations)
{
    setWindowTitle(QString("CatchChallenger - %1 - %2").arg(server_name).arg(informations.public_informations.pseudo));
}

void MainWindow::on_languages_clicked()
{
    LanguagesSelect::languagesSelect->exec();
}

void MainWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(CatchChallenger::Api_client_real::client!=NULL)
        CatchChallenger::Api_client_real::client->tryDisconnect();
    QMessageBox::critical(this,tr("Error"),error);
}

void MainWindow::newUpdate(const QString &version)
{
    ui->update->setText(InternetUpdater::getText(version));
    ui->update->setVisible(true);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    hide();
    if(socket!=NULL)
    {
        if(socket!=NULL)
            socket->disconnectFromHost();
        if(socket!=NULL)
            socket->abort();
    }
    QCoreApplication::quit();
}
