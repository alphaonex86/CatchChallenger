#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QSslError>
#include <QSslSocket>
#include <QCompleter>
#include "../base/ClientVariableAudio.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include <vlc/vlc.h>
#include <vlc/libvlc_structures.h>
#endif

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/ConnectedSocket.h"
#include "../base/FeedNews.h"
#include "../base/Api_client_real.h"
#include "../base/render/MapController.h"
#include "../base/interface/BaseWindow.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_lineEditLogin_returnPressed();
    void on_lineEditPass_returnPressed();
    void on_pushButtonTryLogin_clicked();
    void connectTheExternalSocket();
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
    void haveNewError();
    void message(std::string message);
    void sslErrors(const QList<QSslError> &errors);
    void disconnected(std::string reason);
    void protocol_is_good();
    void needQuit();
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void on_languages_clicked();
    void newError(std::string error, std::string detailedError);
    void newUpdate(const std::string &version);
    void closeEvent(QCloseEvent *event);
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error);
    void on_lineEditLogin_textChanged(const QString &arg1);
    void logged();
    void updateTheOkButton();
    void gameIsLoaded();
    #ifndef CATCHCHALLENGER_NOAUDIO
    static void vlceventStatic(const libvlc_event_t* event, void* ptr);
    void vlcevent(const libvlc_event_t* event);
    void audioLoop(void *player);
    #endif
    void on_showPassword_toggled(bool);
private:
    QSslSocket *realSslSocket;
    Ui::MainWindow *ui;
    void resetAll();
    QStringList chat_list_player_pseudo;
    QList<CatchChallenger::Player_type> chat_list_player_type;
    QList<CatchChallenger::Chat_type> chat_list_type;
    QList<QString> chat_list_text;
    QSettings settings;
    QString lastMessageSend;
    QTimer stopFlood;
    int numberForFlood;
    bool haveShowDisconnectionReason;
    QStringList server_list;
    CatchChallenger::ConnectedSocket *socket;
    QHash<QString,QString> serverLoginList;
    QHash<QString,QDateTime> lastServerConnect;
    QHash<QString,uint32_t> lastServerWaitBeforeConnectAfterKick;
    QHash<QString,bool> lastServerIsKick;
    QTimer updateTheOkButtonTimer;
    #ifndef CATCHCHALLENGER_NOAUDIO
    libvlc_media_player_t *vlcPlayer;
    #endif
    CatchChallenger::BaseWindow *baseWindow;
    CatchChallenger::Api_protocol *client;
    QCompleter *completer;
    QString lastServer;

    QString server_name;
    QString server_dns_or_ip;
    QString proxy_dns_or_ip;
    uint16_t server_port;
    uint16_t proxy_port;
signals:
    #ifndef CATCHCHALLENGER_NOAUDIO
    void audioLoopRestart(void *vlcPlayer);
    #endif
};

#endif // MAINWINDOW_H
