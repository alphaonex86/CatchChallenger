#include "ConnexionManager.h"
#include "../qt/Api_client_real.h"
#include "LoadingScreen.h"
#include <iostream>
#include <QStandardPaths>

ConnexionManager::ConnexionManager(CatchChallenger::BaseWindow *baseWindow, LoadingScreen *l)
{
    socket=nullptr;
    #ifndef NOTCPSOCKET
    realSslSocket=nullptr;
    #endif
    #ifndef NOWEBSOCKET
    realWebSocket=nullptr;
    #endif
    this->baseWindow=baseWindow;
    this->datapckFileSize=0;
    this->l=l;
}

void ConnexionManager::connectToServer(Multi::ConnexionInfo connexionInfo,QString login,QString pass)
{
    if(socket!=NULL)
    {
        socket->disconnectFromHost();
        socket->abort();
        delete socket;
        socket=NULL;
        #ifndef NOTCPSOCKET
        realSslSocket=nullptr;
        #endif
        #ifndef NOWEBSOCKET
        realWebSocket=nullptr;
        #endif
    }
    #if defined(NOTCPSOCKET) && defined(NOWEBSOCKET)
        #error can t be both NOTCPSOCKET and NOWEBSOCKET disabled
    #else
        #ifndef NOTCPSOCKET
        if(!connexionInfo.host.isEmpty())
        {
            realSslSocket=new QSslSocket();
            socket=new CatchChallenger::ConnectedSocket(realSslSocket);
        }
        else {
            std::cerr << "host is empty" << std::endl;
            abort();
        }
        #endif
        #ifndef NOWEBSOCKET
        if(socket==nullptr)
        {
            if(!connexionInfo.ws.isEmpty())
            {
                realWebSocket=new QWebSocket();
                socket=new CatchChallenger::ConnectedSocket(realWebSocket);
            }
            else {
                std::cerr << "ws is empty" << std::endl;
                abort();
            }
        }
        #endif
    #endif
    CatchChallenger::Api_client_real *client=new CatchChallenger::Api_client_real(socket);
/*    if(serverMode==ServerMode_Internal)
        client->tryLogin("admin",pass.toStdString());
    else*/
        client->tryLogin(login.toStdString(),pass.toStdString());
    if(!connect(client,               &CatchChallenger::Api_client_real::Qtdisconnected,       this,&ConnexionManager::disconnected))
        abort();
    //connect(client,               &CatchChallenger::Api_protocol::Qtmessage,            this,&ConnexionManager::message,Qt::QueuedConnection);
    if(!connect(client,               &CatchChallenger::Api_client_real::Qtlogged,             this,&ConnexionManager::logged,Qt::QueuedConnection))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtdatapackSizeBase,       this,&ConnexionManager::QtdatapackSizeBase))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtdatapackSizeMain,       this,&ConnexionManager::QtdatapackSizeMain))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtdatapackSizeSub,       this,&ConnexionManager::QtdatapackSizeSub))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::progressingDatapackFileBase,       this,&ConnexionManager::progressingDatapackFileBase))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::progressingDatapackFileMain,       this,&ConnexionManager::progressingDatapackFileMain))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::progressingDatapackFileSub,       this,&ConnexionManager::progressingDatapackFileSub))
        abort();

    if(!connexionInfo.proxyHost.isEmpty())
    {
        QNetworkProxy proxy;
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            proxy=realSslSocket->proxy();
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            proxy=realWebSocket->proxy();
        #endif
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(connexionInfo.proxyHost);
        proxy.setPort(connexionInfo.proxyPort);
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            realSslSocket->setProxy(proxy);
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            realWebSocket->setProxy(proxy);
        #endif
    }
    /*baseWindow->setMultiPlayer(true,static_cast<CatchChallenger::Api_client_real *>(client));
    baseWindow->stateChanged(QAbstractSocket::ConnectingState);*/
    #ifndef NOTCPSOCKET
    if(realSslSocket!=nullptr)
    {
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),  this,&ConnexionManager::sslErrors,Qt::QueuedConnection))
            abort();
        if(!connect(realSslSocket,&QSslSocket::stateChanged,    this,&ConnexionManager::stateChanged,Qt::DirectConnection))
            abort();
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),           this,&ConnexionManager::error,Qt::QueuedConnection))
            abort();

        socket->connectToHost(connexionInfo.host,connexionInfo.port);
    }
    #endif
    #ifndef NOWEBSOCKET
    if(realWebSocket!=nullptr)
    {
        if(!connect(realWebSocket,&QWebSocket::stateChanged,    this,&ConnexionManager::stateChanged,Qt::DirectConnection))
            abort();
        if(!connect(realWebSocket,static_cast<void(QWebSocket::*)(QAbstractSocket::SocketError)>(&QWebSocket::error),           this,&ConnexionManager::error,Qt::QueuedConnection))
            abort();

        QUrl url{QString(connexionInfo.ws)};
        QNetworkRequest request{url};
        request.setRawHeader("Sec-WebSocket-Protocol", "binary");
        realWebSocket->open(request);
    }
    #endif
    connexionInfo.connexionCounter++;
    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
    this->client=static_cast<CatchChallenger::Api_protocol_Qt *>(client);
    connectTheExternalSocket(connexionInfo,client);
}

void ConnexionManager::disconnected(std::string reason)
{
    //QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(QString::fromStdString(reason)));
    emit errorString(reason);
    /*if(serverConnexion.contains(selectedServer))
        lastServerIsKick[serverConnexion.value(selectedServer)->host]=true;*/
    //baseWindow->resetAll();
}

#ifndef __EMSCRIPTEN__
void ConnexionManager::sslErrors(const QList<QSslError> &errors)
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
#endif

void ConnexionManager::connectTheExternalSocket(Multi::ConnexionInfo connexionInfo,CatchChallenger::Api_client_real * client)
{
    //continue the normal procedure
    if(!connexionInfo.proxyHost.isEmpty())
    {
        QNetworkProxy proxy;
        #ifndef NOTCPSOCKET
        if(realSslSocket!=nullptr)
            proxy=realSslSocket->proxy();
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=nullptr)
            proxy=realWebSocket->proxy();
        #endif
        proxy.setType(QNetworkProxy::Socks5Proxy);
        proxy.setHostName(connexionInfo.proxyHost);
        proxy.setPort(connexionInfo.proxyPort);
        client->setProxy(proxy);
    }

    /*baseWindow->connectAllSignals();
    baseWindow->setMultiPlayer(true,client);*/
    QDir datapack(serverToDatapachPath(connexionInfo));
    if(!datapack.exists())
        if(!datapack.mkpath(datapack.absolutePath()))
        {
            disconnected(tr("Not able to create the folder %1").arg(datapack.absolutePath()).toStdString());
            return;
        }
    client->setDatapackPath(datapack.absolutePath().toStdString());
    //baseWindow->stateChanged(QAbstractSocket::ConnectedState);
}

QString ConnexionManager::serverToDatapachPath(Multi::ConnexionInfo connexionInfo) const
{
    QDir datapack;
    if(connexionInfo.isCustom)
        datapack=QDir(QStringLiteral("%1/datapack/Custom-%2/")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
                      .arg(connexionInfo.unique_code)
                      );
    else
        datapack=QDir(QStringLiteral("%1/datapack/Xml-%2")
                      .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation))
                      .arg(connexionInfo.unique_code)
                      );
    return datapack.absolutePath();
}

/*QString ConnexionManager::serverToDatapachPath(ListEntryEnvolued * selectedServer) const
{
    QDir datapack;
    if(customServerConnexion.contains(selectedServer))
    {
        if(!serverConnexion.value(selectedServer)->name.isEmpty())
             datapack=QDir(QStringLiteral("%1/datapack/%2/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->name));
        else
             datapack=QDir(QStringLiteral("%1/datapack/%2-%3/").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->host).arg(serverConnexion.value(selectedServer)->port));
    }
    else
        datapack=QDir(QStringLiteral("%1/datapack/Xml-%2").arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation)).arg(serverConnexion.value(selectedServer)->unique_code));
    return datapack.absolutePath();
}*/

void ConnexionManager::stateChanged(QAbstractSocket::SocketState socketState)
{
    std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ")" << std::endl;
    if(socketState==QAbstractSocket::ConnectedState)
    {
        //If comment: Internal problem: Api_protocol::sendProtocol() !haveFirstHeader
        #if !defined(NOTCPSOCKET) && !defined(NOWEBSOCKET)
        if(realSslSocket==NULL && realWebSocket==NULL)
            client->sendProtocol();
        /*else
            qDebug() << "Tcp/Web socket found, skip sendProtocol(), previouslusy send by void Api_protocol::connectTheExternalSocketInternal()";*/
        #elif !defined(NOTCPSOCKET)
        if(realSslSocket==NULL)
            client->sendProtocol();
        /*else
            qDebug() << "Tcp socket found, skip sendProtocol(), previouslusy send by void Api_protocol::connectTheExternalSocketInternal()";*/
        #elif !defined(NOWEBSOCKET)
        if(realWebSocket==NULL)
            client->sendProtocol();
        /*else
            qDebug() << "Web socket found, skip sendProtocol(), previouslusy send by void Api_protocol::connectTheExternalSocketInternal()";*/
        #endif
    }
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        if(client!=NULL)
        {
            std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") client!=NULL" << std::endl;
            if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2 || client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage3)
            {
                std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") call socketDisconnectedForReconnect" << std::endl;
                const std::string &lastServer=client->socketDisconnectedForReconnect();
                if(!lastServer.empty())
                    this->lastServer=QString::fromStdString(lastServer);
                return;
            }
        }
        std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") mostly quit" << std::endl;
        /*if(!isVisible()
                #ifndef NOSINGLEPLAYER
                && internalServer==NULL
                #endif
                )
        {
            QCoreApplication::quit();
            return;
        }*/
        if(client!=NULL)
            static_cast<CatchChallenger::Api_client_real *>(this->client)->closeDownload();
        /*if(client!=NULL && client->protocolWrong())
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/
        #ifndef NOSINGLEPLAYER
        /*if(internalServer!=NULL)
            internalServer->stop();*/
        #endif
        /* to fix bug: firstly try connect but connexion refused on localhost, secondly try local game */
        #ifndef NOTCPSOCKET
        if(realSslSocket!=NULL)
        {
            realSslSocket->deleteLater();
            realSslSocket=NULL;
        }
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=NULL)
        {
            realWebSocket->deleteLater();
            realWebSocket=NULL;
        }
        #endif
        if(socket!=NULL)
        {
            socket->deleteLater();
            socket=NULL;
        }
        /*socket will do that's if(realSocket!=NULL)
        {
            delete realSocket;
            realSocket=NULL;
        }*/
        //resetAll();
        /*if(serverMode==ServerMode_Remote)
            QMessageBox::about(this,tr("Quit"),tr("The server have closed the connexion"));*/
        emit disconnectedFromServer();
    }
    /*if(baseWindow!=NULL)
        baseWindow->stateChanged(socketState);*/
}

void ConnexionManager::error(QAbstractSocket::SocketError socketError)
{
    if(client!=NULL)
        if(client->stage()==CatchChallenger::Api_client_real::StageConnexion::Stage2)
            return;
    QString additionalText;
    if(!lastServer.isEmpty())
        additionalText=tr(" on %1").arg(lastServer);
    //resetAll();
    switch(socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        #ifndef NOTCPSOCKET
        if(realSslSocket!=NULL)
            return;
        #endif
        #ifndef NOWEBSOCKET
        if(realWebSocket!=NULL)
            return;
        #endif
        /*if(haveShowDisconnectionReason)
        {
            haveShowDisconnectionReason=false;
            return;
        }*/
        /*QMessageBox::information(this,tr("Connection closed"),tr("Connection closed by the server")+additionalText);
    break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this,tr("Connection closed"),tr("Connection refused by the server")+additionalText);
    break;
    case QAbstractSocket::SocketTimeoutError:
        QMessageBox::information(this,tr("Connection closed"),tr("Socket time out, server too long")+additionalText);
    break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this,tr("Connection closed"),tr("The host address was not found")+additionalText);
    break;
    case QAbstractSocket::SocketAccessError:
        QMessageBox::information(this,tr("Connection closed"),tr("The socket operation failed because the application lacked the required privileges")+additionalText);
    break;
    case QAbstractSocket::SocketResourceError:
        QMessageBox::information(this,tr("Connection closed"),tr("The local system ran out of resources")+additionalText);
    break;
    case QAbstractSocket::NetworkError:
        QMessageBox::information(this,tr("Connection closed"),tr("An error occurred with the network (Connection refused on game server?)")+additionalText);
    break;
    case QAbstractSocket::UnsupportedSocketOperationError:
        QMessageBox::information(this,tr("Connection closed"),tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)")+additionalText);
    break;
    case QAbstractSocket::SslHandshakeFailedError:
        QMessageBox::information(this,tr("Connection closed"),tr("The SSL/TLS handshake failed, so the connection was closed")+additionalText);
    break;
    default:
        QMessageBox::information(this,tr("Connection error"),tr("Connection error: %1").arg(socketError)+additionalText);*/
            errorString((tr("Connection closed by the server")+additionalText).toStdString());
        break;
        case QAbstractSocket::ConnectionRefusedError:
            errorString((tr("Connection refused by the server")+additionalText).toStdString());
        break;
        case QAbstractSocket::SocketTimeoutError:
            errorString((tr("Socket time out, server too long")+additionalText).toStdString());
        break;
        case QAbstractSocket::HostNotFoundError:
            errorString((tr("The host address was not found")+additionalText).toStdString());
        break;
        case QAbstractSocket::SocketAccessError:
            errorString((tr("The socket operation failed because the application lacked the required privileges")+additionalText).toStdString());
        break;
        case QAbstractSocket::SocketResourceError:
            errorString((tr("The local system ran out of resources")+additionalText).toStdString());
        break;
        case QAbstractSocket::NetworkError:
            errorString((tr("An error occurred with the network (Connection refused on game server?)")+additionalText).toStdString());
        break;
        case QAbstractSocket::UnsupportedSocketOperationError:
            errorString((tr("The requested socket operation is not supported by the local operating system (e.g., lack of IPv6 support)")+additionalText).toStdString());
        break;
        case QAbstractSocket::SslHandshakeFailedError:
            errorString((tr("The SSL/TLS handshake failed, so the connection was closed")+additionalText).toStdString());
        break;
        default:
            errorString((tr("Connection error: %1").arg(socketError)+additionalText).toStdString());
    }
}

void ConnexionManager::QtdatapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->progression(0,datapckFileSize);
}

void ConnexionManager::QtdatapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->progression(0,datapckFileSize);
}

void ConnexionManager::QtdatapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->progression(0,datapckFileSize);
}

void ConnexionManager::progressingDatapackFileBase(const uint32_t &size)
{
    l->progression(size,datapckFileSize);
}

void ConnexionManager::progressingDatapackFileMain(const uint32_t &size)
{
    l->progression(size,datapckFileSize);
}

void ConnexionManager::progressingDatapackFileSub(const uint32_t &size)
{
    l->progression(size,datapckFileSize);
}
