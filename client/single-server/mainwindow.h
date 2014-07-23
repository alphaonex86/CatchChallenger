#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QSslError>
#include <QSslSocket>
#include <vlc/vlc.h>
#include <vlc/libvlc_structures.h>

#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/ConnectedSocket.h"
#include "../base/RssNews.h"
#include "../base/Api_client_real.h"
#include "../base/interface/MapController.h"
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
    void message(QString message);
    void sslErrors(const QList<QSslError> &errors);
    void disconnected(QString reason);
    void protocol_is_good();
    void needQuit();
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void on_languages_clicked();
    void newError(QString error,QString detailedError);
    void newUpdate(const QString &version);
    void closeEvent(QCloseEvent *event);
    void rssEntryList(const QList<RssNews::RssEntry> &entryList);
    void on_lineEditLogin_textChanged(const QString &arg1);
    void logged();
    void updateTheOkButton();
    void sslHandcheckIsFinished();
    void readForFirstHeader();
    void gameIsLoaded();
    static void vlcevent(const libvlc_event_t *event, void *ptr);
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
    QHash<QString,quint32> lastServerWaitBeforeConnectAfterKick;
    QHash<QString,bool> lastServerIsKick;
    QTimer updateTheOkButtonTimer;
    bool haveFirstHeader;
    libvlc_media_player_t *vlcPlayer;

    QString server_name;
    QString server_dns_or_ip;
    QString proxy_dns_or_ip;
    quint16 server_port;
    quint16 proxy_port;
};

#endif // MAINWINDOW_H
