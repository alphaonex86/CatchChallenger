#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../base/render/MapVisualiserPlayer.h"

#define SERVER_DNS_OR_IP "localhost"
#define SERVER_NAME tr("Local")
#define SERVER_PORT 42489
#define REGISTER_URL "http://pokecraft.first-world.info/register.html"
#define WEBSITE "http://pokecraft.first-world.info/"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");

    socket=new Pokecraft::ConnectedSocket(new QTcpSocket());
    client=new Pokecraft::Api_client_real(socket);
    baseWindow=new Pokecraft::BaseWindow(client);
    ui->setupUi(this);
    ui->stackedWidget->addWidget(baseWindow);
    if(settings.contains("login"))
        ui->lineEditLogin->setText(settings.value("login").toString());
    if(settings.contains("pass"))
    {
        ui->lineEditPass->setText(settings.value("pass").toString());
        ui->checkBoxRememberPassword->setChecked(true);
    }
    connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()));
    connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)));
    connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)));
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),this,SLOT(have_current_player_info(Pokecraft::Player_private_and_public_informations)));
    connect(socket,SIGNAL(error(QAbstractSocket::SocketError)),this,SLOT(error(QAbstractSocket::SocketError)),Qt::QueuedConnection);
    connect(socket,SIGNAL(stateChanged(QAbstractSocket::SocketState)),this,SLOT(stateChanged(QAbstractSocket::SocketState)));

    stopFlood.setSingleShot(false);
    stopFlood.start(1500);
    numberForFlood=0;
    haveShowDisconnectionReason=false;
    ui->stackedWidget->addWidget(baseWindow);
    baseWindow->setMultiPlayer(true);

    ui->label_login_register->setText(QString("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Register</span></a>").arg(REGISTER_URL));
    ui->label_login_website->setText(QString("<a href=\"%1\"><span style=\"text-decoration:underline;color:#0057ae;\">Web site</span></a>").arg(WEBSITE));

    stateChanged(QAbstractSocket::UnconnectedState);

    setWindowTitle("Pokecraft - "+SERVER_NAME);
}

MainWindow::~MainWindow()
{
    client->tryDisconnect();
    delete client;
    delete baseWindow;
    delete ui;
    delete socket;
}

void MainWindow::resetAll()
{
    client->resetAll();
    baseWindow->resetAll();
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
    setWindowTitle("Pokecraft - "+SERVER_NAME);
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

void MainWindow::on_pushButtonTryLogin_pressed()
{
    ui->pushButtonTryLogin->setStyleSheet("border-radius:15px;border-color:#000;border:1px solid #000;background-color:rgb(220, 220, 220);padding:1px 10px;color:rgb(150,150,150);");
}

void MainWindow::on_pushButtonTryLogin_released()
{
    ui->pushButtonTryLogin->setStyleSheet("border-radius:15px;border-color:#000;border:1px solid #000;background-color:rgb(255, 255, 255);padding:1px 10px;color:rgb(0,0,0);");
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
    client->tryConnect(host,port);
}

void MainWindow::stateChanged(QAbstractSocket::SocketState socketState)
{
    if(socketState==QAbstractSocket::UnconnectedState)
        resetAll();
    baseWindow->stateChanged(socketState);
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
    client->tryLogin(ui->lineEditLogin->text(),ui->lineEditPass->text());
}

void MainWindow::needQuit()
{
    client->tryDisconnect();
}

void MainWindow::have_current_player_info(const Pokecraft::Player_private_and_public_informations &informations)
{
    setWindowTitle(QString("Pokecraft - %1 - %2").arg(SERVER_NAME).arg(informations.public_informations.pseudo));
}
