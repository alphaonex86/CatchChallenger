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

#include "../bot/MultipleBotConnectionImplFoprGui.h"
#include "../../client/base/ClientStructures.h"

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

    MultipleBotConnectionImplFoprGui multipleBotConnexion;
    QHash<uint8_t/*character group index*/,QPair<uint8_t/*server count*/,uint8_t/*temp Index to display*/> > serverByCharacterGroup;
    QList<CatchChallenger::ServerFromPoolForDisplay *> serverOrdenedList;
    QList<QList<CatchChallenger::CharacterEntry> > characterEntryList;
public slots:
    void detectSlowDown(QString text);
private slots:
    void lastReplyTime(const quint32 &time);
    void on_connect_clicked();
    void on_characterSelect_clicked();
    void logged(const QList<CatchChallenger::ServerFromPoolForDisplay *> &serverOrdenedList,const QList<QList<CatchChallenger::CharacterEntry> > &characterEntryList,bool haveTheDatapack);
    void statusError(QString error);
    void display_numberOfBotConnected(quint16 numberOfBotConnected);
    void display_numberOfSelectedCharacter(quint16 numberOfSelectedCharacter);
    void on_move_toggled(bool checked);
    void on_randomText_toggled(bool checked);
    void on_chatRandomReply_toggled(bool checked);
    void on_bugInDirection_toggled(bool checked);
    void on_serverList_activated(const QModelIndex &index);
    void on_serverListSelect_clicked();
    void updateServerList();
    void addToServerList(CatchChallenger::LogicialGroup &logicialGroup, QTreeWidgetItem *item, const uint64_t &currentDate, const bool &fullView=true);
signals:
    void isDisconnected();
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
