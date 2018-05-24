#ifndef SIMPLESOLOSERVER_H
#define SIMPLESOLOSERVER_H

#include <QMainWindow>
#include "../base/ClientVariableAudio.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include <vlc/vlc.h>
#include <vlc/libvlc_structures.h>
#include "../base/Audio.h"
#endif
#include "../base/solo/SoloWindow.h"

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
    void play(const std::string &savegamesPath);
    bool sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath);
    void protocol_is_good();
    void disconnected(std::string reason);
    void message(std::string message);
    void stateChanged(QAbstractSocket::SocketState socketState);
    void serverError(const QString &error);
    void serverErrorStd(const std::string &error);
    void is_started(bool started);
    void saveTime();
    void resetAll();
    void newError(std::string error, std::string detailedError);
    void closeEvent(QCloseEvent *event);
    void gameIsLoaded();
    #ifndef CATCHCHALLENGER_NOAUDIO
    static void vlceventStatic(const libvlc_event_t* event, void* ptr);
    void vlcevent(const libvlc_event_t* event);
    void audioLoop(void *player);
    #endif
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
    QSettings settings;
    #ifndef CATCHCHALLENGER_NOAUDIO
    libvlc_media_player_t *vlcPlayer;
    #endif
    CatchChallenger::BaseWindow *baseWindow;
    CatchChallenger::Api_protocol *client;
signals:
    #ifndef CATCHCHALLENGER_NOAUDIO
    void audioLoopRestart(void *vlcPlayer);
    #endif
};

#endif // SIMPLESOLOSERVER_H
