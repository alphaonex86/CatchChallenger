#ifndef SIMPLESOLOSERVER_H
#define SIMPLESOLOSERVER_H

#include <QDialog>
#include "../base/solo/SoloWindow.h"

namespace Ui {
class SimpleSoloServer;
}

class SimpleSoloServer : public QDialog
{
    Q_OBJECT
public:
    explicit SimpleSoloServer(QWidget *parent = 0);
    ~SimpleSoloServer();
public slots:
    void play(const QString &savegamesPath);
    void sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath);
    void protocol_is_good();
    void disconnected(QString reason);
    void message(QString message);
    void stateChanged(QAbstractSocket::SocketState socketState);
    void serverError(const QString &error);
    void is_started(bool started);
    void saveTime();
    void resetAll();
    void newError(QString error,QString detailedError);
    void closeEvent(QCloseEvent *event);
private:
    Ui::SimpleSoloServer *ui;
    SoloWindow *solowindow;
    CatchChallenger::ConnectedSocket *socket;
    quint64 timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    CatchChallenger::InternalServer * internalServer;
    QString pass;
    bool haveShowDisconnectionReason;
};

#endif // SIMPLESOLOSERVER_H
