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
#include "../base/ClientVariableAudio.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../base/QInfiniteBuffer.h"
#endif

#include "../../libcatchchallenger/ChatParsing.hpp"
#include "../../../general/base/GeneralStructures.hpp"
#include "../../../general/base/ConnectedSocket.hpp"
#include "../base/FeedNews.h"
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../base/Audio.h"
#endif
#include "../../libqtcatchchallenger/Api_client_real.hpp"
#include "../../qtmaprender/MapController.hpp"
#include "../base/interface/BaseWindow.h"
#include "../base/interface/ListEntryEnvolued.h"
#ifndef NOSINGLEPLAYER
#include "../base/solo/SoloWindow.h"
#include "../../../server/qt/InternalServer.hpp"
#endif
#include "../base/LanguagesSelect.h"

namespace Ui {
    class MainWindow;
}

class AddOrEditServer;

class ConnexionInfo
{
public:
    QString unique_code;
    QString name;

    //hightest priority
    QString host;
    uint16_t port;
    //lower priority
    QString ws;

    uint32_t connexionCounter;
    uint32_t lastConnexion;

    QString register_page;
    QString lost_passwd_page;
    QString site_page;

    QString proxyHost;
    uint16_t proxyPort;

    bool operator<(const ConnexionInfo &connexionInfo) const;
};

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
    void displayServerList();
    void on_server_add_clicked();
    void server_add_finished();
    void saveConnexionInfoList();
    void on_server_select_clicked();
    void on_server_remove_clicked();
    void on_server_refresh_clicked();
    void on_login_cancel_clicked();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList();
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QByteArray &xmlContent);
    std::vector<ConnexionInfo> loadXmlConnexionInfoList(const QString &file);
    std::vector<ConnexionInfo> loadConfigConnexionInfoList();
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
    std::vector<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QSpacerItem *spacer;
    QSpacerItem *spacerServer;
    AddOrEditServer *addServer;
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
    #ifndef NOTCPSOCKET
    QSslSocket *realSslSocket;
    #endif
    #ifndef NOWEBSOCKET
    QWebSocket *realWebSocket;
    #endif
    std::vector<ListEntryEnvolued *> datapack,server;
    QHash<ListEntryEnvolued *,QString> datapackPathList;
    QHash<ListEntryEnvolued *,ConnexionInfo *> serverConnexion;
    QSet<ListEntryEnvolued *> customServerConnexion;
    ListEntryEnvolued * selectedDatapack;
    ListEntryEnvolued * selectedServer;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
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
