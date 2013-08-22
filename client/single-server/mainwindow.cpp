#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../base/render/MapVisualiserPlayer.h"

#define SERVER_DNS_OR_IP "catchchallenger.first-world.info"
#define SERVER_NAME tr("CatchChallenger")
#define SERVER_PORT 42489
#define REGISTER_URL "http://catchchallenger.first-world.info/register.html"
#define WEBSITE "http://catchchallenger.first-world.info/"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");

    socket=new CatchChallenger::ConnectedSocket(new QTcpSocket());
    CatchChallenger::Api_client_real::client=new CatchChallenger::Api_client_real(socket);
    ui->setupUi(this);
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
    connect(socket,&CatchChallenger::ConnectedSocket::stateChanged,this,&MainWindow::stateChanged);

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(CatchChallenger::BaseWindow::baseWindow);
    CatchChallenger::BaseWindow::baseWindow->setMultiPlayer(true);

    ui->label_login_register->setText(QString("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Register</span></a>").arg(REGISTER_URL));
    ui->label_login_website->setText(QString("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Web site</span></a>").arg(WEBSITE));

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle("CatchChallenger - "+SERVER_NAME);
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
    ui->stackedWidget->setCurrentIndex(0);
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
    setWindowTitle("CatchChallenger - "+SERVER_NAME);
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
    settings.setValue("login",ui->lineEditLogin->text());
    if(ui->checkBoxRememberPassword->isChecked())
        settings.setValue("pass",ui->lineEditPass->text());
    else
        settings.remove("pass");

    QString host=SERVER_DNS_OR_IP;
    quint16 port=SERVER_PORT;

    ui->stackedWidget->setCurrentIndex(1);
    static_cast<CatchChallenger::Api_client_real *>(CatchChallenger::Api_client_real::client)->tryConnect(host,port);
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
    CatchChallenger::BaseWindow::baseWindow->stateChanged(socketState);
}

void MainWindow::error(QAbstractSocket::SocketError socketError)
{
    resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        if(haveShowDisconnectionReason)
        {
            haveShowDisconnectionReason=false;
            return;
        }
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
    setWindowTitle(QString("CatchChallenger - %1 - %2").arg(SERVER_NAME).arg(informations.public_informations.pseudo));
}
