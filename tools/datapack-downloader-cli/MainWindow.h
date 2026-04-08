#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QList>
#include <QByteArray>

#include "../../client/libqtcatchchallenger/ConnectedSocket.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../client/libqtcatchchallenger/Api_client_real.hpp"

class MainClass : public QObject
{
    Q_OBJECT
public:
    explicit MainClass();
    void help();
    bool needQuit;
private:
    struct CatchChallengerClient
    {
        QSslSocket *sslSocket;
        CatchChallenger::ConnectedSocket *socket;
        CatchChallenger::Api_client_real *api;
    };
    CatchChallengerClient client;

    QString login,pass,proxy,proxy_port,host,port;
private slots:
    void protocol_is_good();
    void logged(const std::vector<std::vector<CatchChallenger::CharacterEntry> > &characterEntryList);
    void newError(const std::string &error,const std::string &detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    void disconnected(const std::string &reason);
    void sslErrors(const QList<QSslError> &errors);
    void haveTheDatapack();
};

#endif // MAINWINDOW_H
