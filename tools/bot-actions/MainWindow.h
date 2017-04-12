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
#include <QTreeWidgetItem>

#include "../bot/MultipleBotConnectionImplForGui.h"
#include "../../client/base/ClientStructures.h"
#include "BotTargetList.h"
#include "SocialChat.h"

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
    QSettings settings;
    QTimer slowDownTimer;
    BotTargetList *botTargetList;

    MultipleBotConnectionImplForGui multipleBotConnexion;
    QHash<uint8_t/*character group index*/,QPair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    QList<CatchChallenger::ServerFromPoolForDisplay *> serverOrdenedList;
    QList<QList<CatchChallenger::CharacterEntry> > characterEntryList;
    std::vector<std::string> pseudoNotUsed;
    std::unordered_set<std::string> pseudoUsed;
public slots:
    void detectSlowDown(uint32_t queryCount, uint32_t worseTime);
private slots:
    void lastReplyTime(const quint32 &time);
    void on_connect_clicked();
    void on_characterSelect_clicked();
    void logged(CatchChallenger::Api_client_real *senderObject,const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList,bool haveTheDatapack);
    void statusError(QString error);
    void display_numberOfBotConnected(quint16 numberOfBotConnected);
    void display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter);
    void on_serverList_activated(const QModelIndex &index);
    void on_serverListSelect_clicked();
    void updateServerList(CatchChallenger::Api_client_real *senderObject);
    void addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView=true);
    void datapackIsReady();
    void datapackMainSubIsReady();
    void on_autoCreateCharacter_stateChanged(int arg1);
    void all_player_connected();
    void all_player_on_map();
    void on_host_returnPressed();
    std::string getNewPseudo();
signals:
    void isDisconnected();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
