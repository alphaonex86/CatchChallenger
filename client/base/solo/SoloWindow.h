#ifndef SOLOWINDOW_H
#define SOLOWINDOW_H

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
#include "../base/Api_client_virtual.h"
#include "../base/interface/MapController.h"
#include "../base/interface/BaseWindow.h"
#include "../base/interface/ListEntryEnvolued.h"
#include "InternalServer.h"

namespace Ui {
    class SoloWindow;
}

class SoloWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SoloWindow(QWidget *parent,const QString &datapackPath,const QString &savegamePath,const bool &standAlone);
    ~SoloWindow();
    void updateSavegameList();
protected:
    void changeEvent(QEvent *e);
private slots:
    void on_SaveGame_Delete_clicked();
    void on_SaveGame_Rename_clicked();
    void on_SaveGame_Copy_clicked();
    void on_SaveGame_Play_clicked();
    void on_SaveGame_New_clicked();
    void SoloWindowListEntryEnvoluedClicked();
    void SoloWindowListEntryEnvoluedDoubleClicked();
    void SoloWindowListEntryEnvoluedUpdate();
    void closeDb(QSqlDatabase *db);
/*private slots:
    void stateChanged(QAbstractSocket::SocketState socketState);
    void error(QAbstractSocket::SocketError socketError);
    void haveNewError();
    void message(QString message);
    void disconnected(QString);
    void protocol_is_good();
    void try_stop_server();


    void saveTime();
    void is_started(bool started);
    void serverError(const QString &error);
    void sendSettings(CatchChallenger::InternalServer * internalServer, const QString &savegamesPath);
    */
    void on_SaveGame_Back_clicked();
    void on_languages_clicked();
    void newUpdate(const QString &version);

private:
    Ui::SoloWindow *ui;
    void resetAll();
    //bool rmpath(const QDir &dir);
    QString getMapName(const QString &file);
    QString getMapZone(const QString &file);
    QString getZoneName(const QString &zone);
    QList<ListEntryEnvolued *> savegame;
    QHash<ListEntryEnvolued *,QString> savegamePathList;
    QHash<ListEntryEnvolued *,bool> savegameWithMetaData;
    ListEntryEnvolued * selectedSavegame;
    bool datapackPathExists;
    bool standAlone;
    QSpacerItem *spacer;
    QString datapackPath;
    QString savegamePath;
    /*
    QSettings settings;
    QString lastMessageSend;
    QStringList server_list;

    //loaded game
    QString pass;
    quint64 timeLaunched;
    QString launchedGamePath;
    bool haveLaunchedGame;
    */
signals:
    void play(const QString &savegamesPath);
    void back();
};

#endif // SOLOWINDOW_H
