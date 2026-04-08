#include "MainWindow.h"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"

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
    client.api=new CatchChallenger::Api_client_real(client.socket);
    client.sslSocket->ignoreSslErrors();
    client.sslSocket->setPeerVerifyMode(QSslSocket::VerifyNone);
    client.api->setDatapackPath(QCoreApplication::applicationDirPath().toStdString()+"/datapack/");
    if(!connect(client.sslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),      this,&MainClass::sslErrors,Qt::QueuedConnection))
        abort();
    if(!connect(client.api,&CatchChallenger::Api_client_real::Qtprotocol_is_good,        this,&MainClass::protocol_is_good))
        abort();
    if(!connect(client.api,&CatchChallenger::Api_client_real::Qtlogged,                  this,&MainClass::logged))
        abort();
    if(!connect(client.api,&CatchChallenger::Api_client_real::QtnewError,                this,&MainClass::newError))
        abort();
    if(!connect(client.sslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::errorOccurred),    this,&MainClass::newSocketError))
        abort();
    if(!connect(client.api,&CatchChallenger::Api_client_real::Qtdisconnected,            this,&MainClass::disconnected))
        abort();
    if(!connect(client.api,&CatchChallenger::Api_client_real::QthaveTheDatapack,         this,&MainClass::haveTheDatapack))
        abort();
    client.sslSocket->setProxy(proxyObject);
    client.sslSocket->connectToHost(host,port_int);
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

void MainClass::disconnected(const std::string &reason)
{
    qDebug() << "Disconnected by the host:" << QString::fromStdString(reason);
    QCoreApplication::exit(2);
}

void MainClass::protocol_is_good()
{
    client.api->tryLogin(login.toStdString(),pass.toStdString());
}

void MainClass::logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    Q_UNUSED(characterEntryList);
    client.api->sendDatapackContentBase();
}

void MainClass::haveTheDatapack()
{
    //load the datapack
    qDebug() << "Have the datapack";
    QCoreApplication::exit(0);
}

void MainClass::newError(const std::string &error,const std::string &detailedError)
{
    qDebug() << QStringLiteral("Error: %1, detailedError: %2").arg(QString::fromStdString(error)).arg(QString::fromStdString(detailedError));
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
