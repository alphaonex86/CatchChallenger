/** \file LocalListener.h
\brief The have local server, to have unique instance, and send arguments to the current running instance
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef LOCALLISTENER_H
#define LOCALLISTENER_H

#include <QObject>
#include <QLocalServer>
#include <QByteArray>
#include <QString>

#include "../../general/base/lib.h"

class QLocalSocket;

class DLL_PUBLIC LocalListener : public QObject
{
    Q_OBJECT
public:
    explicit LocalListener(QObject *parent = 0);
    ~LocalListener();
    bool tryListen();
    void quitAllRunningInstance();
signals:
    //One newline-terminated text command arrived on the control socket (the
    //QLocalServer automation channel). The client wires this to
    //MapControllerMP::remoteAction(). The legacy single-instance "quit" (a 0x00
    //byte, or the literal line "quit") is still handled here and never emitted.
    void actionReceived(const QString &line);
public slots:
    //Push a reply/event back to the connected controller (STATE/INVENTORY
    //answers now; chat events later). Wired from MapControllerMP::remoteReply().
    void sendReply(const QString &line);
private:
    QLocalServer localServer;
    uint8_t count;
    QLocalSocket *controlSocket;
    QByteArray controlBuffer;
private slots:
    void listenServer(const uint8_t &count);
    void dataIncomming();
    void deconnectClient();
    void newConnexion();

};

#endif // LOCALLISTENER_H
