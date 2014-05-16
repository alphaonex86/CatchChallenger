#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QObject>
#include <QTimer>
#include <QList>
#include <QByteArray>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../general/base/CommonDatapack.h"
#include "../../client/base/Api_client_real.h"

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
    void logged();
    void newError(QString error,QString detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    void disconnected();
    void tryLink();
    void sslErrors(const QList<QSslError> &errors);
    void haveTheDatapack();
};

#endif // MAINWINDOW_H
