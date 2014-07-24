#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QTimer>
#include <QList>
#include <QSemaphore>
#include <QByteArray>
#include <QSettings>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../general/base/CommonDatapack.h"
#include "../../client/base/Api_client_real.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
private:
    struct CatchChallengerClient
    {
        QSslSocket *sslSocket;
        CatchChallenger::ConnectedSocket *socket;
        CatchChallenger::Api_client_real *api;
        bool have_informations;
        bool haveShowDisconnectionReason;
        CatchChallenger::Direction direction;
        QList<CatchChallenger::CharacterEntry> charactersList;
        quint16 number;
        QString login;
        QString pass;
        bool selectedCharacter;
        bool haveFirstHeader;
    };
    QHash<CatchChallenger::Api_client_real *,CatchChallengerClient *> apiToCatchChallengerClient;
    QHash<CatchChallenger::ConnectedSocket *,CatchChallengerClient *> connectedSocketToCatchChallengerClient;
    QHash<QSslSocket *,CatchChallengerClient *> sslSocketToCatchChallengerClient;

    QTimer connectTimer;
    QSettings settings;
    QTimer moveTimer;
    QTimer textTimer;
    QTimer slowDownTimer;
    QFileInfoList skinsList;
    quint16 number;
    QSet<quint32> characterOnMap;
    quint16 numberOfBotConnected;
    quint16 numberOfSelectedCharacter;
public slots:
    void doMove();
    void doText();
    void detectSlowDown();
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
private slots:
    void createClient();
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void haveCharacter();
    void logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList);
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void newError(QString error,QString detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    void disconnected();
    void lastReplyTime(const quint32 &time);
    void tryLink(CatchChallengerClient *client);
    void protocol_is_good();
    void on_connect_clicked();
    void sslErrors(const QList<QSslError> &errors);
    void on_characterSelect_clicked();
    void haveTheDatapack();
    void ifMultipleConnexionStartCreation();
    void connectTimerSlot();
    void newCharacterId(const quint8 &returnCode, const quint32 &characterId);
    void sslHandcheckIsFinished();
    void readForFirstHeader();
    void connectTheExternalSocket(CatchChallengerClient *client);
signals:
    void isDisconnected();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
