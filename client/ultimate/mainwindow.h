#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QRegularExpression>
#include <QSslSocket>
#include <QSettings>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDateTime>
#include <QSet>
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
#ifndef CATCHCHALLENGER_NOAUDIO
#include "../base/Audio.h"
#endif
#include "../base/Api_client_real.h"
#include "../base/render/MapController.h"
#include "../base/interface/BaseWindow.h"
#include "../base/interface/ListEntryEnvolued.h"
#include "../base/solo/SoloWindow.h"
#include "../base/LanguagesSelect.h"

namespace Ui {
    class MainWindow;
}

class ConnexionInfo
{
public:
    QString host;
    uint16_t port;
    QString name;
    uint32_t connexionCounter;
    uint32_t lastConnexion;
    QString register_page;
    QString lost_passwd_page;
    QString site_page;
    QString unique_code;
    bool operator<(const ConnexionInfo &connexionInfo) const;
    QString proxyHost;
    uint16_t proxyPort;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool toQuit;
    #ifndef CATCHCHALLENGER_NOAUDIO
    libvlc_media_player_t *vlcPlayer;
    #endif
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
    void sslErrors(const QList<QSslError> &errors);
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
    void saveConnexionInfoList();
    void on_server_select_clicked();
    void on_server_remove_clicked();
    void on_server_refresh_clicked();
    void on_login_cancel_clicked();
    QList<ConnexionInfo> loadXmlConnexionInfoList();
    QList<ConnexionInfo> loadXmlConnexionInfoList(const QByteArray &xmlContent);
    QList<ConnexionInfo> loadXmlConnexionInfoList(const QString &file);
    QList<ConnexionInfo> loadConfigConnexionInfoList();
    void closeEvent(QCloseEvent *event);
    void downloadFile();
    void metaDataChanged();
    void httpFinished();
    void on_multiplayer_clicked();
    void on_server_back_clicked();
    void gameSolo_play(const std::string &savegamesPath);
    void gameSolo_back();
    void on_solo_clicked();
    bool sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath);
    void is_started(bool started);
    void saveTime();
    void serverError(const QString &error);
    void serverErrorStd(const std::string &error);
    void on_languages_clicked();
    void newUpdate(const std::string &version);
    void feedEntryList(const std::vector<FeedNews::FeedEntry> &entryList, std::string error);
    void on_lineEditLogin_textChanged(const QString &arg1);
    void logged();
    void gameIsLoaded();
    void updateTheOkButton();
    #ifndef CATCHCHALLENGER_NOAUDIO
    static void vlceventStatic(const libvlc_event_t* event, void* ptr);
    void vlcevent(const libvlc_event_t* event);
    void audioLoop(void *player);
    #endif
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
    QList<ConnexionInfo> temp_customConnexionInfoList,temp_xmlConnexionInfoList,mergedConnexionInfoList;
    QSpacerItem *spacer;
    QSpacerItem *spacerServer;
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
    QSslSocket *realSslSocket;
    QList<ListEntryEnvolued *> datapack,server;
    QHash<ListEntryEnvolued *,QString> datapackPathList;
    QHash<ListEntryEnvolued *,ConnexionInfo *> serverConnexion;
    QSet<ListEntryEnvolued *> customServerConnexion;
    ListEntryEnvolued * selectedDatapack;
    ListEntryEnvolued * selectedServer;
    QNetworkAccessManager qnam;
    QNetworkReply *reply;
    SoloWindow *solowindow;
    QString pass;
    uint64_t timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    CatchChallenger::InternalServer * internalServer;
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
signals:
    #ifndef CATCHCHALLENGER_NOAUDIO
    void audioLoopRestart(void *vlcPlayer);
    #endif
};

#endif // MAINWINDOW_H
