#ifndef SOLOWINDOW_H
#define SOLOWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <QAbstractSocket>
#include <QTimer>
#include <QDateTime>
#include <QSpacerItem>

#include "../QFakeSocket.h"
#include "../../../general/base/ChatParsing.h"
#include "../../../general/base/GeneralStructures.h"
#include "../Api_client_real.h"
#include "../Api_client_virtual.h"
#include "../render/MapController.h"
#include "../interface/BaseWindow.h"
#include "../interface/ListEntryEnvolued.h"
#include "InternalServer.h"

namespace Ui {
    class SoloWindow;
}

class SoloWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit SoloWindow(QWidget *parent,const std::string &datapackPath,const std::string &savegamePath,const bool &standAlone);
    ~SoloWindow();
    void updateSavegameList();
    void setOnlySolo();
    void setBuggyStyle();//work around QSS crash
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
    void newUpdate(const std::string &version);

private:
    Ui::SoloWindow *ui;
    void resetAll();
    //bool rmpath(const QDir &dir);
    std::string getMapName(const std::string &file);
    std::string getMapZone(const std::string &file);
    std::string getZoneName(const std::string &zone);
    bool versionIsNewer(const std::string &version);
    std::vector<ListEntryEnvolued *> savegame;
    std::unordered_map<ListEntryEnvolued *,std::string> savegamePathList;
    std::unordered_map<ListEntryEnvolued *,bool> savegameWithMetaData;
    ListEntryEnvolued * selectedSavegame;
    bool datapackPathExists;
    bool standAlone;
    QSpacerItem *spacer;
    std::string datapackPath;
    std::string savegamePath;

    static const QString text_savegame_version;
    static const QString text_QSQLITE;
    static const QString text_savegameupdate;
    static const QString text_catchchallenger_db_sqlite;
    static const QString text_time_played;
    static const QString text_location;
    static const QString text_dotxml;
    static const QString text_dottmx;
    static const QString text_metadatadotconf;
    static const QString text_slash;
    static const QString text_title;
    static const QString text_hover_entry;
    static const QString text_pass;
    static const QString text_map;
    static const QString text_zone;
    static const QString text_name;
    static const QString text_value;
    static const QString text_properties;
    static const QString text_property;
    static const QString text_full_entry;
    static const QString text_lang;
    static const QString text_en;
    static const QString text_CATCHCHALLENGER_SAVEGAME_VERSION;
signals:
    void play(const std::string &savegamesPath);
    void back();
};

#endif // SOLOWINDOW_H
