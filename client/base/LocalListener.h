/** \file LocalListener.h
\brief The have local server, to have unique instance, and send arguments to the current running instance
\author alpha_one_x86
\licence GPL3, see the file COPYING */

#ifndef LOCALLISTENER_H
#define LOCALLISTENER_H

#include <QObject>
#include <QLocalServer>

#include "ExtraSocket.h"

class LocalListener : public QObject
{
    Q_OBJECT
public:
    explicit LocalListener(QObject *parent = 0);
    ~LocalListener();
    bool tryListen();
    void quitAllRunningInstance();
private:
    QLocalServer localServer;
    quint8 count;
private slots:
    void listenServer(const quint8 &count);
    void dataIncomming();
    void deconnectClient();
    void newConnexion();

};

#endif // LOCALLISTENER_H
