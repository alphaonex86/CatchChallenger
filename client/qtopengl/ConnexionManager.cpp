#include "ConnexionManager.hpp"
#include "../libqtcatchchallenger/Api_client_real.hpp"
#include "../libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../libqtcatchchallenger/Settings.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "foreground/LoadingScreen.hpp"
#include <iostream>
#include <QStandardPaths>

ConnexionManager::ConnexionManager(LoadingScreen *l)
{
    client=nullptr;
    socket=nullptr;
    #ifndef NOTCPSOCKET
    realSslSocket=nullptr;
    #endif
    #ifndef NOWEBSOCKET
    realWebSocket=nullptr;
    #endif
    #ifdef CATCHCHALLENGER_SOLO
    fakeSocket=nullptr;
    #endif
    this->datapckFileSize=0;
    this->l=l;
    l->reset();
    l->setText(tr("Connecting..."));

    haveDatapack=false;
    haveDatapackMainSub=false;
    datapackIsParsed=false;
}

void ConnexionManager::connectToServer(ConnexionInfo connexionInfo,QString login,QString pass)
{
    haveDatapack=false;
    haveDatapackMainSub=false;
    datapackIsParsed=false;

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
            #ifdef CATCHCHALLENGER_SOLO
            fakeSocket=new QFakeSocket();
            socket=new CatchChallenger::ConnectedSocket(fakeSocket);
            #else
            std::cerr << "host is empty" << std::endl;
            abort();
            #endif
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
    //work around for the resetAll() of protocol
    const std::string mainDatapackCode=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
    const std::string subDatapackCode=CommonSettingsServer::commonSettingsServer.subDatapackCode;
    CatchChallenger::Api_client_real *client=new CatchChallenger::Api_client_real(socket);
    CommonSettingsServer::commonSettingsServer.mainDatapackCode=mainDatapackCode;
    CommonSettingsServer::commonSettingsServer.subDatapackCode=subDatapackCode;
/*    if(serverMode==ServerMode_Internal)
        client->tryLogin("admin",pass.toStdString());
    else*/
        client->tryLogin(login.toStdString(),pass.toStdString());

    if(!connect(client,               &CatchChallenger::Api_protocol_Qt::QtnewError,       this,&ConnexionManager::newError))
        abort();
    if(!connect(client,               &CatchChallenger::Api_protocol_Qt::Qtmessage,       this,&ConnexionManager::message))
        abort();

    if(!connect(client,               &CatchChallenger::Api_client_real::Qtdisconnected,       this,&ConnexionManager::disconnected))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtnotLogged,       this,&ConnexionManager::disconnected))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QthaveTheDatapack,       this,&ConnexionManager::haveTheDatapack))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QthaveTheDatapackMainSub,       this,&ConnexionManager::haveTheDatapackMainSub))
        abort();

    if(!connect(client,               &CatchChallenger::Api_client_real::Qtlogged,             this,&ConnexionManager::Qtlogged,Qt::QueuedConnection))
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

    if(!connect(client,               &CatchChallenger::Api_client_real::Qtprotocol_is_good,       this,&ConnexionManager::protocol_is_good))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtconnectedOnLoginServer,       this,&ConnexionManager::connectedOnLoginServer))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtconnectingOnGameServer,       this,&ConnexionManager::connectingOnGameServer))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtconnectedOnGameServer,       this,&ConnexionManager::connectedOnGameServer))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QthaveDatapackMainSubCode,       this,&ConnexionManager::haveDatapackMainSubCode))
        abort();
    if(!connect(client,               &CatchChallenger::Api_client_real::QtgatewayCacheUpdate,       this,&ConnexionManager::gatewayCacheUpdate))
        abort();

    if(!connect(client,               &CatchChallenger::Api_client_real::QthaveCharacter,             this,&ConnexionManager::QthaveCharacter,Qt::QueuedConnection))
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
    connexionInfo.connexionCounter++;
    connexionInfo.lastConnexion=static_cast<uint32_t>(QDateTime::currentMSecsSinceEpoch()/1000);
    this->client=static_cast<CatchChallenger::Api_protocol_Qt *>(client);
    connectTheExternalSocket(connexionInfo,client);

    //connect the datapack loader
    if(!connect(QtDatapackClientLoader::datapackLoader,  &QtDatapackClientLoader::datapackParsed,                  this,                                   &ConnexionManager::datapackParsed,Qt::QueuedConnection))
        abort();
    if(!connect(QtDatapackClientLoader::datapackLoader,  &QtDatapackClientLoader::datapackParsedMainSub,           this,                                   &ConnexionManager::datapackParsedMainSub,Qt::QueuedConnection))
        abort();
    if(!connect(QtDatapackClientLoader::datapackLoader,  &QtDatapackClientLoader::datapackChecksumError,           this,                                   &ConnexionManager::datapackChecksumError,Qt::QueuedConnection))
        abort();
    if(!connect(this,                                   &ConnexionManager::parseDatapack,                             QtDatapackClientLoader::datapackLoader,  &QtDatapackClientLoader::parseDatapack,Qt::QueuedConnection))
        abort();
    if(!connect(this,                                   &ConnexionManager::parseDatapackMainSub,                      QtDatapackClientLoader::datapackLoader,  &QtDatapackClientLoader::parseDatapackMainSub,Qt::QueuedConnection))
        abort();

    bool haveTryConnect=false;
    #ifndef NOTCPSOCKET
    if(realSslSocket!=nullptr)
    {
        haveTryConnect=true;
        std::cout << "try connect TCP on: " << connexionInfo.host.toStdString() << ": " << connexionInfo.port << std::endl;
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(const QList<QSslError> &errors)>(&QSslSocket::sslErrors),  this,&ConnexionManager::sslErrors,Qt::QueuedConnection))
            abort();
        if(!connect(realSslSocket,&QSslSocket::stateChanged,    this,&ConnexionManager::stateChanged,Qt::QueuedConnection))
            abort();
        if(!connect(realSslSocket,static_cast<void(QSslSocket::*)(QAbstractSocket::SocketError)>(&QSslSocket::error),           this,&ConnexionManager::error,Qt::QueuedConnection))
            abort();

        socket->connectToHost(connexionInfo.host,connexionInfo.port);
    }
    #endif
    #ifndef NOWEBSOCKET
    if(realWebSocket!=nullptr)
    {
        haveTryConnect=true;
        std::cout << "try connect WebSocket on: " << connexionInfo.host.toStdString() << ": " << connexionInfo.port << std::endl;
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
    #ifdef CATCHCHALLENGER_SOLO
    if(fakeSocket!=nullptr)
    {
        haveTryConnect=true;
        std::cout << "try connect Internal on: " << connexionInfo.host.toStdString() << ": " << connexionInfo.port << std::endl;
        if(!connect(fakeSocket,&QFakeSocket::stateChanged,    this,&ConnexionManager::stateChanged,Qt::DirectConnection))
            abort();
        if(!connect(fakeSocket,static_cast<void(QFakeSocket::*)(QAbstractSocket::SocketError)>(&QFakeSocket::error),           this,&ConnexionManager::error,Qt::QueuedConnection))
            abort();

        fakeSocket->connectToHost();
    }
    #endif
    if(haveTryConnect==false)
        newError("Internal error","Unable to determine the socket to use");
}

bool ConnexionManager::isLocalGame()
{
    return fakeSocket!=nullptr;
}

void ConnexionManager::selectCharacter(const uint32_t indexSubServer, const uint32_t indexCharacter)
{
    if(!client->selectCharacter(indexSubServer,indexCharacter))
        errorString("BaseWindow::on_serverListSelect_clicked(), wrong serverSelected internal data");
}

void ConnexionManager::disconnected(std::string reason)
{
    //QMessageBox::information(this,tr("Disconnected"),tr("Disconnected by the reason: %1").arg(QString::fromStdString(reason)));
    emit errorString(reason);
    /*if(serverConnexion.contains(selectedServer))
        lastServerIsKick[serverConnexion.value(selectedServer)->host]=true;*/
    //baseWindow->resetAll();
}

void ConnexionManager::newError(const std::string &error,const std::string &detailedError)
{
    emit errorString(error+"\n"+detailedError);
    if(socket!=nullptr)
        socket->disconnectFromHost();
    if(client!=nullptr)
    {
        client->disconnectClient();
        client->resetAll();
    }
}

void ConnexionManager::message(const std::string &message)
{
    std::cerr << message << std::endl;
    //emit errorString(message);
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

void ConnexionManager::connectTheExternalSocket(ConnexionInfo connexionInfo,CatchChallenger::Api_client_real * client)
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
    //work around for the resetAll() of protocol
    const std::string mainDatapackCode=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
    const std::string subDatapackCode=CommonSettingsServer::commonSettingsServer.subDatapackCode;
    //if(!connexionInfo.unique_code.isEmpty())
    client->setDatapackPath(datapack.absolutePath().toStdString());
    CommonSettingsServer::commonSettingsServer.mainDatapackCode=mainDatapackCode;
    CommonSettingsServer::commonSettingsServer.subDatapackCode=subDatapackCode;
    //baseWindow->stateChanged(QAbstractSocket::ConnectedState);
}

QString ConnexionManager::serverToDatapachPath(ConnexionInfo connexionInfo) const
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
        //If uncomment: Internal problem: Api_protocol::sendProtocol() !haveFirstHeader
        /*if((1
            #if !defined(NOTCPSOCKET)
                && realSslSocket==NULL
            #endif
        #if !defined(NOWEBSOCKET)
             && realWebSocket==NULL
        #endif
            )
        #ifdef CATCHCHALLENGER_SOLO
             && fakeSocket!=NULL
        #endif
                )
            if(client!=nullptr)
                client->sendProtocol();*/
        /*else
            qDebug() << "Tcp/Web socket found, skip sendProtocol(), previouslusy send by void Api_protocol::connectTheExternalSocketInternal()";*/
    }
    if(socketState==QAbstractSocket::UnconnectedState)
    {
        std::cout << "ConnexionManager::stateChanged(" << std::to_string((int)socketState) << ") mostly quit " << client->stage() << std::endl;
        if(client->stage()==CatchChallenger::Api_protocol::StageConnexion::Stage2 || client->stage()==CatchChallenger::Api_protocol::StageConnexion::Stage3)
            return;
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
        #ifdef CATCHCHALLENGER_SOLO
        if(fakeSocket!=NULL)
        {
            fakeSocket->deleteLater();
            fakeSocket=NULL;
        }
        #endif
        if(socket!=NULL)
        {
            socket->deleteLater();
            socket=NULL;
        }
        emit disconnectedFromServer();
    }
}

QAbstractSocket::SocketState ConnexionManager::state()
{
    if(socket==nullptr)
        return QAbstractSocket::UnconnectedState;
    return socket->state();
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

        case QAbstractSocket::ProxyConnectionRefusedError:
            errorString((tr("Could not contact the proxy server because the connection to that server was denied")+additionalText).toStdString());
        break;
        case QAbstractSocket::ProxyConnectionClosedError:
            errorString((tr("The connection to the proxy server was closed unexpectedly (before the connection to the final peer was established)")+additionalText).toStdString());
        break;
        case QAbstractSocket::ProxyConnectionTimeoutError:
            errorString((tr("The connection to the proxy server timed out or the proxy server stopped responding in the authentication phase")+additionalText).toStdString());
        break;
        case QAbstractSocket::ProxyNotFoundError:
            errorString((tr("The proxy address set with setProxy() (or the application proxy) was not found")+additionalText).toStdString());
        break;
        case QAbstractSocket::ProxyProtocolError:
            errorString((tr("The connection negotiation with the proxy server failed, because the response from the proxy server could not be understood")+additionalText).toStdString());
        break;

        default:
            errorString((tr("Connection error: %1").arg(socketError)+additionalText).toStdString());
    }
}

void ConnexionManager::QtdatapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->reset();
    l->progression(0,datapckFileSize);
}

void ConnexionManager::QtdatapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->reset();
    l->progression(0,datapckFileSize);
}

void ConnexionManager::QtdatapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize)
{
    (void)datapckFileNumber;
    this->datapckFileSize=datapckFileSize;
    l->reset();
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


void ConnexionManager::protocol_is_good()
{
    l->setText(tr("Protocol is good"));
}

void ConnexionManager::connectedOnLoginServer()
{
    l->setText(tr("Connected on login server"));
}

void ConnexionManager::connectingOnGameServer()
{
    l->setText(tr("Connecting on game server"));
}

void ConnexionManager::connectedOnGameServer()
{
    l->setText(tr("Connected on game server"));
}

void ConnexionManager::gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression)
{
    l->setText(tr("Gateway cache update..."));
    l->progression(progression,100+1);
}

void ConnexionManager::sendDatapackContentMainSub()
{
    if(client==nullptr)
    {
        std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
        abort();
    }
    QByteArray dataMain;
    QByteArray dataSub;
    if(Settings::settings->contains("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())))
        dataMain=QByteArray::fromHex(Settings::settings->value("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain())).toString().toUtf8());
    if(Settings::settings->contains("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())))
        dataSub=QByteArray::fromHex(Settings::settings->value("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub())).toString().toUtf8());
    client->sendDatapackContentMainSub(std::string(dataMain.constData(),dataMain.size()),std::string(dataSub.constData(),dataSub.size()));
}

void ConnexionManager::haveTheDatapack()
{
    if(client==NULL)
        return;
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapack()";
    #endif
    if(haveDatapack)
        return;
    haveDatapack=true;
    Settings::settings->setValue("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase()),
                      QString(QByteArray(
                          CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),
                          static_cast<int>(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size())
                                  ).toHex())
                      );

    if(client!=NULL)
        emit parseDatapack(client->datapackPathBase());
}

void ConnexionManager::haveTheDatapackMainSub()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveTheDatapackMainSub()";
    #endif
    if(haveDatapackMainSub)
        return;
    haveDatapackMainSub=true;
    Settings::settings->setValue("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain()),
                      QString(QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size())
                                  ).toHex())
                      );
    Settings::settings->setValue("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub()),
                      QString(QByteArray(
                          CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),
                          static_cast<int>(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size())
                                  ).toHex())
                      );

    if(client!=NULL)
        emit parseDatapackMainSub(client->mainDatapackCode(),client->subDatapackCode());
}

void ConnexionManager::datapackParsed()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackParsed()";
    #endif
    datapackIsParsed=true;
    //updateConnectingStatus();
    //loadSettingsWithDatapack();
    /*{
        if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png")).exists())
            ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image: url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelBottom.png');padding:6px 6px 6px 14px;}"));
        else
            ui->frameFightBottom->setStyleSheet(QStringLiteral("#frameFightBottom{background-image: url(:/CC/images/interface/fight/labelBottom.png);padding:6px 6px 6px 14px;}"));
        if(QFile(QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png")).exists())
            ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image: url('")+QString::fromStdString(client->datapackPathBase())+QStringLiteral("/images/interface/fight/labelTop.png');padding:6px 14px 6px 6px;}"));
        else
            ui->frameFightTop->setStyleSheet(QStringLiteral("#frameFightTop{background-image: url(:/CC/images/interface/fight/labelTop.png);padding:6px 14px 6px 6px;}"));
    }*/
    //updatePlayerImage();
    emit logged(characterEntryList);
}

void ConnexionManager::datapackParsedMainSub()
{
    if(client==NULL)
        return;
    /*if(mapController==NULL)
        return;*/
    /*mainSubDatapackIsParsed=true;

    emit datapackParsedMainSubMap();

    updateConnectingStatus();*/
    if(!client->setMapNumber(QtDatapackClientLoader::datapackLoader->get_mapToId().size()))
        emit newError(tr("Internal error").toStdString(),"No map loaded to call client->setMapNumber()");
    client->have_main_and_sub_datapack_loaded();
}

void ConnexionManager::datapackChecksumError()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::datapackChecksumError()";
    #endif
    datapackIsParsed=false;
    //reset all the cached hash
    Settings::settings->remove("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase()));
    Settings::settings->remove("DatapackHashMain-"+QString::fromStdString(client->datapackPathMain()));
    Settings::settings->remove("DatapackHashSub-"+QString::fromStdString(client->datapackPathSub()));
    emit newError(tr("Datapack on mirror is corrupted").toStdString(),
                  "The checksum sended by the server is not the same than have on the mirror");
}

void ConnexionManager::Qtlogged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList)
{
    if(Settings::settings->contains("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())))
    {
        const QByteArray &data=QByteArray::fromHex(Settings::settings->value("DatapackHashBase-"+QString::fromStdString(client->datapackPathBase())).toString().toUtf8());
        client->sendDatapackContentBase(std::string(data.constData(),data.size()));
    }
    else
        client->sendDatapackContentBase();
    this->characterEntryList=characterEntryList;
    //emit logged(characterEntryList);
}

void ConnexionManager::QthaveCharacter()
{
/*    if(client==nullptr)
    {
        std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
        return;
    }
    sendDatapackContentMainSub();*/
    emit goToMap();
    std::cout << "have character" << std::endl;
}

void ConnexionManager::haveDatapackMainSubCode()
{
    /// \warn do above
    if(client==nullptr)
    {
        std::cerr << "sendDatapackContentMainSub(): client==nullptr" << std::endl;
        return;
    }
    sendDatapackContentMainSub();
    //updateConnectingStatus();
}
