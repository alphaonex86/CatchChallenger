#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QListWidgetItem>
#include <QHash>
#include <QString>
#include <QIcon>

#include "../../client/base/interface/DatapackClientLoader.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    enum db_type_enum{db_type_mysql,db_type_sqlite};
private slots:
    void on_sqlite_browse_file_clicked();
    void on_mysql_try_connect_clicked();
    void on_sqlite_try_connect_clicked();
    void on_player_return_clicked();
    void updatePlayerList();
    void on_searchIntoThePlayerList_clicked();
    void on_playerList_itemActivated(QListWidgetItem *item);
    void on_selectPlayer_clicked();
    void on_cash_editingFinished();
    void on_datapack_path_browse_clicked();
    void on_datapack_path_textChanged(const QString &arg1);
    void on_add_item_clicked();

    void on_items_itemSelectionChanged();

    void on_remove_item_clicked();

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    db_type_enum db_type;
    quint32 player_id;
    bool havePlayerSelected;
    QHash<QString,quint32> player_id_list;
    DatapackClientLoader datapackLoader;
    QString datapack;
    QHash<QString,QIcon> icons_cache;
    //player items
    QHash<quint32,quint32> items;
    QHash<QListWidgetItem *,quint32> items_graphical;
private:
    void try_connect();
    void load_inventory();
};

#endif // MAINWINDOW_H
