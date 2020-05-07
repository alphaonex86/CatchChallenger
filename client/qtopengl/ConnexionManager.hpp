#ifndef CONNEXIONMANAGER_H
#define CONNEXIONMANAGER_H

#include <QObject>
#include "foreground/Multi.hpp"
#include "../qt/ConnectedSocket.hpp"
#include "../../general/base/GeneralStructures.hpp"
#include "interface/BaseWindow.hpp"

class LoadingScreen;

class ConnexionManager : public QObject
{
    Q_OBJECT
public:
    explicit ConnexionManager(LoadingScreen *l);
    void connectToServer(ConnexionInfo connexionInfo,QString login,QString pass);
    void selectCharacter(const uint32_t indexSubServer, const uint32_t indexCharacter);
    CatchChallenger::Api_protocol_Qt *client;
signals:
    void errorString(std::string error);
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    void disconnectedFromServer();
public slots:
    void disconnected(std::string reason);
    #ifndef __EMSCRIPTEN__
    void sslErrors(const QList<QSslError> &errors);
    #endif
    void connectTheExternalSocket(ConnexionInfo connexionInfo,CatchChallenger::Api_client_real * client);
    QString serverToDatapachPath(ConnexionInfo connexionInfo) const;
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);

    void protocol_is_good();
    void connectedOnLoginServer();
    void connectingOnGameServer();
    void connectedOnGameServer();
    void haveDatapackMainSubCode();
    void gatewayCacheUpdate(const uint8_t gateway,const uint8_t progression);
private:
    CatchChallenger::ConnectedSocket *socket;
    #ifndef NOTCPSOCKET
    QSslSocket *realSslSocket;
    #endif
    #ifndef NOWEBSOCKET
    QWebSocket *realWebSocket;
    #endif
    QString lastServer;
    LoadingScreen *l;
    uint32_t datapckFileSize;
private:
    void QtdatapackSizeBase(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    void QtdatapackSizeMain(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    void QtdatapackSizeSub(const uint32_t &datapckFileNumber,const uint32_t &datapckFileSize);
    void progressingDatapackFileBase(const uint32_t &size);
    void progressingDatapackFileMain(const uint32_t &size);
    void progressingDatapackFileSub(const uint32_t &size);
};

#endif // CONNEXIONMANAGER_H
