#ifndef SIMPLESOLOSERVER_H
#define SIMPLESOLOSERVER_H

#include <QMainWindow>
#include <vlc/vlc.h>
#include <vlc/libvlc_structures.h>
#include "../base/solo/SoloWindow.h"
#include "../base/Audio.h"

namespace Ui {
class SimpleSoloServer;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
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
    void gameIsLoaded();
    static void vlcevent(const libvlc_event_t *event, void *ptr);
private:
    Ui::SimpleSoloServer *ui;
    SoloWindow *solowindow;
    CatchChallenger::ConnectedSocket *socket;
    uint64_t timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    CatchChallenger::InternalServer * internalServer;
    QString pass;
    bool haveShowDisconnectionReason;
    libvlc_media_player_t *vlcPlayer;
};

#endif // SIMPLESOLOSERVER_H
