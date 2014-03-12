#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QObject>
#include <QTimer>
#include <QSemaphore>
#include <QByteArray>
#include <QSettings>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../../general/base/MoveOnTheMap.h"
#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ConnectedSocket.h"
#include "../../server/base/Api_client_virtual.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public CatchChallenger::MoveOnTheMap
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void show_details();
    QSslSocket *sslSocket;
    CatchChallenger::ConnectedSocket socket;
private:
    CatchChallenger::Api_client_virtual api;
    //debug
    bool details;

    static int index_loop,loop_size;
    bool have_informations;
    QSettings settings;
    bool haveShowDisconnectionReason;
    QTimer moveTimer;
    QTimer textTimer;
public slots:
    void stop_move();
    void doMove();
    void doText();
    void send_player_move(const quint8 &moved_unit,const CatchChallenger::Direction &the_new_direction);
    void new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type);
private slots:
    void insert_player(const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction);
    void haveCharacter();
    void logged(const QList<CatchChallenger::CharacterEntry> &characterEntryList);
    void have_current_player_info(const CatchChallenger::Player_private_and_public_informations &informations);
    void newError(QString error,QString detailedError);
    void newSocketError(QAbstractSocket::SocketError error);
    void disconnected();
    void tryLink();
    void on_connect_clicked();
    void sslErrors(const QList<QSslError> &errors);
    void on_characterSelect_clicked();

signals:
    void isDisconnected();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
