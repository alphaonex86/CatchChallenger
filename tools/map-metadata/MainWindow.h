#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidgetItem>
#include <QDomElement>
#include <QHash>
#include <QString>
#include <QSettings>
#include <QProcess>
#include <QSqlDatabase>

#include <unordered_map>

#include "../../server/base/MapServer.h"
#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralStructuresXml.h"

#define TMPDATA "/tmp/map-metadata/"/// \warning need finish with /

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    //to load/unload the content
    struct Map_semi
    {
        //conversion x,y to position: x+y*width
        CatchChallenger::CommonMap* map;
        CatchChallenger::Map_semi_border border;
        CatchChallenger::Map_to_send old_map;
    };
    struct Monster_Semi_Market
    {
        //conversion x,y to position: x+y*width
        uint32_t id;
        uint64_t price;
    };
    struct MapContent
    {
        QString region;
        QString zone;
        QString subzone;
        QString name;
        QString type;
        bool officialzone;
    };

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool preload_the_map(const std::string &path);
    void displayNewNotFinishedMap(const bool useNearMap=false);
    void displayMap(const QString key);
    QString simplifiedName(QString name);
private slots:
    void on_getAnotherUnfinishedMap_clicked();
    void on_region_textChanged(const QString &arg1);
    void on_zone_textChanged(const QString &arg1);
    void on_subzone_textChanged(const QString &arg1);
    void on_name_textChanged(const QString &arg1);
    void on_type_currentIndexChanged(const QString &arg1);
    void on_markAsFinished_clicked();
    void on_officialZone_toggled(bool checked);
    void on_label_7_linkActivated(const QString &link);
    void on_label_8_linkActivated(const QString &link);
    void on_label_9_linkActivated(const QString &link);
    void on_label_10_linkActivated(const QString &link);
private:
    QProcess process;
    Ui::MainWindow *ui;
    QSqlDatabase m_db;
    std::unordered_map<std::string,CatchChallenger::MapServer *> map_list;
    std::vector<Map_semi> semi_loaded_map;
    QHash<QString,MapContent> finishedFile;
    QHash<QString,MapContent> not_finishedFile;
    QString selectedMap;
    bool canUpdate;
};

#endif // MAINWINDOW_H
