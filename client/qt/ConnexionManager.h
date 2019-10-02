#ifndef CONNEXIONMANAGER_H
#define CONNEXIONMANAGER_H

#include <QObject>
#include "Multi.h"
#include "ConnectedSocket.h"
#include "interface/BaseWindow.h"

class ConnexionManager : public QObject
{
    Q_OBJECT
public:
    explicit ConnexionManager(CatchChallenger::BaseWindow *baseWindow);
    void connectToServer(Multi::ConnexionInfo connexionInfo,QString login,QString pass);
signals:
    void errorString(std::string error);
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
public slots:
    void disconnected(std::string reason);
    #ifndef __EMSCRIPTEN__
    void sslErrors(const QList<QSslError> &errors);
    #endif
    void connectTheExternalSocket();
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
private:
    CatchChallenger::ConnectedSocket *socket;
    #ifndef NOTCPSOCKET
    QSslSocket *realSslSocket;
    #endif
    #ifndef NOWEBSOCKET
    QWebSocket *realWebSocket;
    #endif
    CatchChallenger::BaseWindow *baseWindow;
};

#endif // CONNEXIONMANAGER_H
