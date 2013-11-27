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

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/ConnectedSocket.h"
#include "../base/Api_client_real.h"
#include "../base/interface/MapController.h"
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
    quint16 port;
    QString name;
    quint32 connexionCounter;
    quint32 lastConnexion;
    QString register_page;
    QString lost_passwd_page;
    QString site_page;
    QString unique_code;
    bool operator<(const ConnexionInfo &connexionInfo) const;
    QString proxyHost;
    quint16 proxyPort;
};

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
    QString serverToDatapachPath(ListEntryEnvolued *selectedServer) const;
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
    void haveNewError();
    void newError(QString error,QString detailedError);
    void message(QString message);
    void disconnected(QString reason);
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
    QPair<QString,QString> getDatapackInformations(const QString &filePath);
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
    void gameSolo_play(const QString &savegamesPath);
    void gameSolo_back();
    void on_solo_clicked();
    void sendSettings(CatchChallenger::InternalServer * internalServer,const QString &savegamesPath);
    void is_started(bool started);
    void saveTime();
    void serverError(const QString &error);
    void on_languages_clicked();
    void newUpdate(const QString &version);
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
    QSslSocket *realSocket;
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
    quint64 timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    CatchChallenger::InternalServer * internalServer;
    QSet<QString> customServerName;
};

#endif // MAINWINDOW_H
