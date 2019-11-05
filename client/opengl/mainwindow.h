#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegularExpression>
#ifndef __EMSCRIPTEN__
#include <QSslSocket>
#else
#include <QtWebSockets/QWebSocket>
#endif
#include <QSettings>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QSet>
#include <QCompleter>
#include "../qt/ClientVariableAudio.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../qt/QInfiniteBuffer.h"
#endif

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../qt/ConnectedSocket.h"
#include "../qt/FeedNews.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../qt/Audio.h"
#endif
#include "../qt/Api_client_real.h"
#include "../qt/render/MapController.h"
#include "../qt/interface/BaseWindow.h"
#ifndef NOSINGLEPLAYER
#include "../qt/solo/SoloWindow.h"
#endif
#include "../qt/LanguagesSelect.h"

namespace Ui {
    class MainWindow;
}

class AddOrEditServer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool toQuit;
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_lineEditLogin_returnPressed();
    void on_lineEditPass_returnPressed();
    void on_pushButtonTryLogin_clicked();
    void connectTheExternalSocket();
    QString serverToDatapachPath(ListEntryEnvolued *selectedServer) const;
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
    void haveNewError();
    void newError(std::string error, std::string detailedError);
    void message(std::string message);
    void disconnected(std::string reason);
    void protocol_is_good();
    void needQuit();
    #ifndef __EMSCRIPTEN__
    void sslErrors(const QList<QSslError> &errors);
    #endif
    void ListEntryEnvoluedClicked();
    void ListEntryEnvoluedDoubleClicked();
    void ListEntryEnvoluedUpdate();
    void serverListEntryEnvoluedClicked();
    void serverListEntryEnvoluedDoubleClicked();
    void serverListEntryEnvoluedUpdate();
    void on_manageDatapack_clicked();
    std::pair<std::string,std::string> getDatapackInformations(const std::string &filePath);
    void on_backDatapack_clicked();
    void on_deleteDatapack_clicked();
    void on_login_cancel_clicked();
    void closeEvent(QCloseEvent *event);
    void downloadFile();
    void metaDataChanged();
    void httpFinished();
    void on_multiplayer_clicked();
    void on_server_back_clicked();
    #ifndef NOSINGLEPLAYER
    void gameSolo_play(const std::string &savegamesPath);
    void gameSolo_back();
    void on_solo_clicked();
    bool sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath);
    void saveTime();
    #endif
    void is_started(bool started);
    void serverError(const QString &error);
    void serverErrorStd(const std::string &error);
    void on_languages_clicked();
    #ifndef __EMSCRIPTEN__
    void newUpdate(const std::string &version);
    #endif
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error);
    void on_lineEditLogin_textChanged(const QString &arg1);
    void logged();
    void gameIsLoaded();
    void updateTheOkButton();
    void on_server_edit_clicked();
    bool askForUltimateCopy();
    void on_showPassword_toggled(bool);
    void on_UltimateKey_clicked();
private:
    enum ServerMode
    {
        ServerMode_Internal,
        ServerMode_Remote,
        ServerMode_None
    };
    ServerMode serverMode;
    QSpacerItem *spacer;
    QSpacerItem *spacerServer;
    Ui::MainWindow *ui;
    void resetAll();
    QSettings settings;
    bool haveShowDisconnectionReason;
    QStringList server_list;
    CatchChallenger::ConnectedSocket *socket;
    #ifndef NOTCPSOCKET
    QSslSocket *realSslSocket;
    #endif
    #ifndef NOWEBSOCKET
    QWebSocket *realWebSocket;
    #endif
    #ifndef NOSINGLEPLAYER
    SoloWindow *solowindow;
    #endif
    QString pass;
    uint64_t timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    #ifndef NOSINGLEPLAYER
    CatchChallenger::InternalServer * internalServer;
    #endif
    QSet<QString> customServerName;
    QHash<QString,QString> serverLoginList;
    QHash<QString,QDateTime> lastServerConnect;
    QHash<QString,uint32_t> lastServerWaitBeforeConnectAfterKick;
    QHash<QString,bool> lastServerIsKick;
    QTimer updateTheOkButtonTimer;
    CatchChallenger::BaseWindow *baseWindow;
    CatchChallenger::Api_protocol *client;
    QCompleter *completer;
    QString lastServer;
    #ifndef CATCHCHALLENGER_NOAUDIO
    QAudioOutput *player;
    QInfiniteBuffer buffer;
    QByteArray data;
    #endif
};

#endif // MAINWINDOW_H
