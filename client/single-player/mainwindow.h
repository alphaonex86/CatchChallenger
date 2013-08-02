#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QSettings>
#include <QTimer>
#include <QDateTime>
#include <QSpacerItem>

#include "../../general/base/QFakeSocket.h"
#include "../../general/base/ChatParsing.h"
#include "../../general/base/GeneralStructures.h"
#include "../base/Api_client_real.h"
#include "../base/interface/MapController.h"
#include "../base/interface/BaseWindow.h"
#include "SaveGameLabel.h"
#include "InternalServer.h"

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
    void closeEvent(QCloseEvent *event);
private slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
    void haveNewError();
    void message(QString message);
    void disconnected(QString);
    void protocol_is_good();
    void try_stop_server();
    void on_SaveGame_New_clicked();
    void savegameLabelClicked();
    void savegameLabelDoubleClicked();
    void savegameLabelUpdate();
    void on_SaveGame_Delete_clicked();
    void on_SaveGame_Rename_clicked();
    void on_SaveGame_Copy_clicked();
    void on_SaveGame_Play_clicked();
    void saveTime();
    void is_started(bool started);
    void play(const QString &savegamesPath);
    void serverError(const QString &error);
    void sendSettings(CatchChallenger::InternalServer * internalServer, const QString &savegamesPath);
private:
    Ui::MainWindow *ui;
    void resetAll();
    bool rmpath(const QDir &dir);
    void updateSavegameList();
    QString getMapName(const QString &file);
    QString getMapZone(const QString &file);
    QString getZoneName(const QString &zone);
    void closeDb(QSqlDatabase *db);
    QSettings settings;
    QString lastMessageSend;
    QStringList server_list;
    CatchChallenger::ConnectedSocket *socket;
    QList<SaveGameLabel *> savegame;
    QHash<SaveGameLabel *,QString> savegamePathList;
    QHash<SaveGameLabel *,bool> savegameWithMetaData;
    SaveGameLabel * selectedSavegame;
    QSpacerItem *spacer;
    CatchChallenger::InternalServer * internalServer;
    QString datapackPath;
    QString savegamePath;
    //loaded game
    QString pass;
    quint64 timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    bool datapackPathExists;
};

#endif // MAINWINDOW_H
