#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QMessageBox>
#include <QFile>
#include <QSslSocket>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    server=NULL;
    client=NULL;
    modeNegociated=false;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_test_clicked()
{
    ui->test->setEnabled(false);
    ui->useSsl->setEnabled(false);

    if(server!=NULL)
        delete server;
    server=NULL;

    if(ui->useSsl->isChecked())
    {
        if(!QFile(QCoreApplication::applicationDirPath()+"/server.key").exists() || !QFile(QCoreApplication::applicationDirPath()+"/server.crt").exists())
        {
            QMessageBox::information(this,"Error",QString("%1 or %2 not found!").arg(QCoreApplication::applicationDirPath()+"/server.key").arg(QCoreApplication::applicationDirPath()+"/server.crt"));
            return;
        }
        QFile key(QCoreApplication::applicationDirPath()+"/server.key");
        if(!key.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this,"Error",QStringLiteral("Unable to access to the server key: %1").arg(key.errorString()));
            return;
        }
        QByteArray keyData=key.readAll();
        key.close();
        QSslKey sslKey(keyData,QSsl::Rsa);
        if(sslKey.isNull())
        {
            QMessageBox::information(this,"Error","Server key is wrong");
            return;
        }

        QFile certificate(QCoreApplication::applicationDirPath()+"/server.crt");
        if(!certificate.open(QIODevice::ReadOnly))
        {
            QMessageBox::information(this,"Error",QStringLiteral("Unable to access to the server certificate: %1").arg(certificate.errorString()));
            return;
        }
        QByteArray certificateData=certificate.readAll();
        certificate.close();
        QSslCertificate sslCertificate(certificateData);
        if(sslCertificate.isNull())
        {
            QMessageBox::information(this,"Error",QStringLiteral("Server certificate is wrong"));
            return;
        }
        server=new Server(sslCertificate,sslKey);
    }
    else
        server=new Server();
    connect(server,&QTcpServer::newConnection,this,&MainWindow::newConnection,Qt::QueuedConnection);
    if(!server->listen(QHostAddress::Any,42489))
    {
        QMessageBox::information(this,"Error",QStringLiteral("Unable to listen: %1").arg(server->errorString()));
        return;
    }

    test_socket();
}

void MainWindow::test_socket()
{
    if(client!=NULL)
        delete client;
    modeNegociated=false;
    client=new QSslSocket();
    client->connectToHost("localhost",42489);
    connect(client,&QSslSocket::readyRead,this,&MainWindow::clientReadyRead);
}

void MainWindow::clientReadyRead()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(sender());
    if(socket==NULL)
        return;
    if(!modeNegociated)
    {
        quint8 value;
        if(socket->read((char*)&value,sizeof(value))==sizeof(value))
        {
            modeNegociated=true;
            if(value)
            {
                socket->setPeerVerifyMode(QSslSocket::VerifyNone);
                socket->ignoreSslErrors();
                socket->startClientEncryption();
                connect(socket,&QSslSocket::encrypted,this,&MainWindow::clientEncrypted);
            }
            else
                socket->write(QString("Hello world!").toUtf8());
        }
    }
    else
    {
        const QByteArray &rawData=socket->readAll();
        if(rawData==QString("Hello world!").toUtf8())
            QMessageBox::information(this,"Stat","Test: success");
        else
            QMessageBox::information(this,"Stat","Test: failed");
    }
}

void MainWindow::clientEncrypted()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(sender());
    if(socket==NULL)
        return;
    socket->write(QString("Hello world!").toUtf8());
}

void MainWindow::newConnection()
{
    if(server==NULL)
        return;
    while(server->hasPendingConnections())
    {
        QSslSocket *socket=static_cast<QSslSocket *>(server->nextPendingConnection());
        if(socket!=NULL)
            connect(socket,&QSslSocket::readyRead,this,&MainWindow::newServerSocketData);
    }
}

void MainWindow::newServerSocketData()
{
    QSslSocket *socket=qobject_cast<QSslSocket *>(sender());
    if(socket==NULL)
        return;
    const QByteArray &data=socket->readAll();
    if(!data.isEmpty())
        socket->write(data);
}
