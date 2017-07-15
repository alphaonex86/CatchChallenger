#include "MainWindow.h"

#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"

#include <QNetworkProxy>

MainClass::MainClass()
{
    needQuit=true;
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_type>("CatchChallenger::Player_type");
    CatchChallenger::ProtocolParsing::initialiseTheVariable();
    CatchChallenger::ProtocolParsing::setMaxPlayers(65535);

    const QStringList &arguments=QCoreApplication::arguments();
    if(arguments.size()<=1)
    {
        help();
        return;
    }
    int index=1;
    while(index<arguments.size())
    {
        if(arguments.at(index)==QStringLiteral("--help"))
        {
            help();
            QCoreApplication::exit(1);
            return;
        }
        if((index+1)>=arguments.size())
        {
            qDebug() << "Wrong arguments count";
            help();
            QCoreApplication::exit(1);
            return;
        }
        if(arguments.at(index)==QStringLiteral("--login"))
            login=arguments.at(index+1);
        else if(arguments.at(index)==QStringLiteral("--pass"))
            pass=arguments.at(index+1);
        else if(arguments.at(index)==QStringLiteral("--proxy"))
            proxy=arguments.at(index+1);
        else if(arguments.at(index)==QStringLiteral("--proxy_port"))
            proxy_port=arguments.at(index+1);
        else if(arguments.at(index)==QStringLiteral("--host"))
            host=arguments.at(index+1);
        else if(arguments.at(index)==QStringLiteral("--port"))
            port=arguments.at(index+1);
        else
        {
            qDebug() << "Unknown arguments: " << arguments.at(index);
            help();
            return;
        }
        index+=2;
    }
    if(login.isEmpty())
    {
        qDebug() << "Login need be specified";
        help();
        return;
    }
    if(pass.isEmpty())
    {
        qDebug() << "Pass need be specified";
        help();
        return;
    }
    if(host.isEmpty())
    {
        qDebug() << "Host need be specified";
        help();
        return;
    }
    if(port.isEmpty())
        port=QStringLiteral("42489");
    if(login.size()<3)
    {
        qDebug() << "Login need be longer";
        help();
        return;
    }
    if(pass.size()<6)
    {
        qDebug() << "Pass need be longer";
        help();
        return;
    }
    bool ok;
    int port_int=port.toUShort(&ok);
    if(!ok)
    {
        qDebug() << "Port is not a number";
        help();
        return;
    }
    int proxy_port_int=proxy_port.toUShort(&ok);
    if(!proxy.isEmpty() && !ok)
    {
        qDebug() << "Proxy port is not a number";
        help();
        return;
    }

    //try connect
    QNetworkProxy proxyObject;
    if(!proxy.isEmpty())
    {
        proxyObject.setType(QNetworkProxy::Socks5Proxy);
        proxyObject.setHostName(proxy);
        proxyObject.setPort(proxy_port_int);
    }

    client.sslSocket=new QSslSocket();
    client.socket=new CatchChallenger::ConnectedSocket(client.sslSocket);
    client.api=new CatchChallenger::Api_client_real(client.socket,false);
    client.sslSocket->ignoreSslErrors();
    client.sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    client.api->setDatapackPath(QCoreApplication::applicationDirPath()+QStringLiteral("/datapack/"));
    connect(client.sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainClass::sslErrors,Qt::QueuedConnection);
    connect(client.api,&CatchChallenger::Api_client_real::logged,                   this,&MainClass::logged);
    connect(client.api,&CatchChallenger::Api_client_real::newError,                 this,&MainClass::newError);
    connect(client.socket,static_cast<void(CatchChallenger::ConnectedSocket::*)(QAbstractSocket::SocketError)>(&CatchChallenger::ConnectedSocket::error),                    this,&MainClass::newSocketError);
    connect(client.socket,&CatchChallenger::ConnectedSocket::disconnected,          this,&MainClass::disconnected);
    connect(client.socket,&CatchChallenger::ConnectedSocket::connected,             this,&MainClass::tryLink,Qt::QueuedConnection);
    connect(client.api,&CatchChallenger::Api_client_real::haveTheDatapack,      this,&MainClass::haveTheDatapack);
    client.sslSocket->setProxy(proxyObject);
    client.socket->connectToHost(host,port_int);
    needQuit=false;
}

void MainClass::help()
{
    qDebug() << "The options are:";
    qDebug() << "--login [login]";
    qDebug() << "--pass [pass]";
    qDebug() << "--proxy [proxy]";
    qDebug() << "--proxy_port [proxy_port]";
    qDebug() << "--host [host]";
    qDebug() << "--port [port]";
}

void MainClass::disconnected()
{
    qDebug() << "Disconnected by the host";
    QCoreApplication::exit(2);
}

void MainClass::tryLink()
{
    client.api->startReadData();
    client.api->sendProtocol();
    client.api->tryLogin(login,pass);
}

void MainClass::logged()
{
    client.api->sendDatapackContent();
}

void MainClass::haveTheDatapack()
{
    //load the datapack
    qDebug() << "Have the datapack";
    QCoreApplication::exit(0);
}

void MainClass::newError(QString error,QString detailedError)
{
    qDebug() << QStringLiteral("Error: %1, detailedError: %2").arg(error).arg(detailedError);
    client.socket->disconnectFromHost();
    QCoreApplication::exit(3);
}

void MainClass::newSocketError(QAbstractSocket::SocketError error)
{
    if(error==0)
    {
        qDebug() << QStringLiteral("Connection refused");
        QCoreApplication::exit(4);
    }
    else if(error==13)
    {
        qDebug() << QStringLiteral("SslHandshakeFailedError");
        QCoreApplication::exit(5);
    }
    else
    {
        qDebug() << QStringLiteral("Socket error: %1").arg(error);
        QCoreApplication::exit(6);
    }

}

void MainClass::sslErrors(const QList<QSslError> &errors)
{
    QStringList sslErrors;
    int index=0;
    while(index<errors.size())
    {
        qDebug() << "Ssl error:" << errors.at(index).errorString();
        sslErrors << errors.at(index).errorString();
        index++;
    }
}
