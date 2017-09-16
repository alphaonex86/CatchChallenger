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

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool preload_the_map(const std::string &path);
private:
    QProcess process;
    Ui::MainWindow *ui;
    QSqlDatabase m_db;
    std::unordered_map<std::string,CatchChallenger::MapServer *> map_list;
    std::vector<Map_semi> semi_loaded_map;
};

#endif // MAINWINDOW_H
