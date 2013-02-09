#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../../general/base/GeneralVariable.h"
#include "ItemDialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QSqlError>
#include <QSqlQuery>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);
}

MainWindow::~MainWindow()
{
    datapackLoader.quit();
    datapackLoader.wait();
    delete ui;
}

void MainWindow::on_sqlite_browse_file_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Select the SQLite database");
    if(file.isEmpty())
        return;
    ui->sqlite_file->setText(file);
}

void MainWindow::on_mysql_try_connect_clicked()
{
    if(ui->mysql_host->text().isEmpty())
    {
        QMessageBox::warning(this,"Warning","The host can't be empty");
        return;
    }
    if(ui->mysql_db->text().isEmpty())
    {
        QMessageBox::warning(this,"Warning","The db can't be empty");
        return;
    }
    if(ui->mysql_login->text().isEmpty())
    {
        QMessageBox::warning(this,"Warning","The login can't be empty");
        return;
    }
    db_type=db_type_mysql;
    db = QSqlDatabase::addDatabase("QMYSQL");
    db.setConnectOptions("MYSQL_OPT_RECONNECT=1");
    db.setHostName(ui->mysql_host->text());
    db.setDatabaseName(ui->mysql_db->text());
    db.setUserName(ui->mysql_login->text());
    db.setPassword(ui->mysql_pass->text());
    try_connect();
}

void MainWindow::on_sqlite_try_connect_clicked()
{
    if(ui->sqlite_file->text().isEmpty())
    {
        QMessageBox::warning(this,"Warning","Select before the file to open");
        return;
    }
    if(!QFile(ui->sqlite_file->text()).exists())
    {
        QMessageBox::warning(this,"Warning","The selected file don't exists");
        return;
    }
    db_type=db_type_sqlite;
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(ui->sqlite_file->text());
    try_connect();
}

void MainWindow::try_connect()
{
    if(!db.open())
    {
        QMessageBox::warning(this,"Warning",
                             QString("Unable to connect to the database: %1, database text: %2")
                             .arg(db.lastError().driverText())
                             .arg(db.lastError().databaseText())
                             );
        return;
    }
    ui->stackedWidget->setCurrentIndex(1);
    datapackLoader.parseDatapack(datapack);
    updatePlayerList();
}

void MainWindow::on_player_return_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void MainWindow::updatePlayerList()
{
    ui->playerList->clear();
    player_id_list.clear();
    //do the query
    QString queryText;
    switch(db_type)
    {
        default:
        case db_type_mysql:
            queryText=QString("SELECT pseudo,id,skin FROM player");
        break;
        case db_type_sqlite:
            queryText=QString("SELECT pseudo,id,skin FROM player");
        break;
    }
    QSqlQuery listQuery(queryText);
    QIcon defaultIcon(":/im-user.png");
    QIcon icon;

    //parse the result
    while(listQuery.next())
    {
        if(ui->searchPlayer->text().isEmpty() || listQuery.value(0).toString().contains(ui->searchPlayer->text(),Qt::CaseInsensitive))
        {
            bool ok;
            quint32 id=listQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(icons_cache.contains(listQuery.value(2).toString()))
                    icon=icons_cache[listQuery.value(2).toString()];
                else
                {
                    QPixmap newIcon(datapack+DATAPACK_BASE_PATH_SKIN+listQuery.value(2).toString()+"/trainer.png");
                    if(newIcon.isNull())
                        icons_cache[listQuery.value(2).toString()]=defaultIcon;
                    else
                    {
                        QPixmap cutedIcon=newIcon.copy(16,48,16,24);
                        if(cutedIcon.isNull())
                            icons_cache[listQuery.value(2).toString()]=defaultIcon;
                        else
                        {
                            icons_cache[listQuery.value(2).toString()]=cutedIcon.scaled(48,72);
                        }
                    }
                    icon=icons_cache[listQuery.value(2).toString()];
                }
                QListWidgetItem *item=new QListWidgetItem(icon,QString(listQuery.value(0).toString()));
                player_id_list[listQuery.value(0).toString()]=id;
                ui->playerList->addItem(item);
            }
        }
    }
}

void MainWindow::on_searchIntoThePlayerList_clicked()
{
    updatePlayerList();
}

void MainWindow::on_playerList_itemActivated(QListWidgetItem *item)
{
    //load the player information
    player_id=player_id_list[item->text()];
    havePlayerSelected=false;
    QString queryText;

    switch(db_type)
    {
        default:
        case db_type_mysql:
            queryText=QString("SELECT cash FROM player WHERE id=%1").arg(player_id);
        break;
        case db_type_sqlite:
            queryText=QString("SELECT cash FROM player WHERE id=%1").arg(player_id);
        break;
    }
    QSqlQuery infoPlayer(queryText);
    if(infoPlayer.isValid())
    {
        QMessageBox::warning(this,"Warning",QString("The player can't be loaded: %1\nQuery: %2").arg(infoPlayer.lastError().text()).arg(queryText));
        return;
    }

    //parse the result
    while(infoPlayer.next())
    {
        bool ok;
        quint32 cash=infoPlayer.value(0).toUInt(&ok);
        if(ok)
            ui->cash->setValue(cash);
    }

    //load the inventory
    items.clear();
    switch(db_type)
    {
        default:
        case db_type_mysql:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=%1").arg(player_id);
        break;
        case db_type_sqlite:
            queryText=QString("SELECT item_id,quantity FROM item WHERE player_id=%1").arg(player_id);
        break;
    }
    QSqlQuery itemsPlayer(queryText);
    while(itemsPlayer.next())
    {
        bool ok;
        quint32 item_id=itemsPlayer.value(0).toUInt(&ok);
        if(ok)
        {
            quint32 quantity=itemsPlayer.value(1).toUInt(&ok);
            if(ok)
                items[item_id]=quantity;
        }
    }
    load_inventory();

    //change the page
    ui->stackedWidget->setCurrentIndex(2);
    havePlayerSelected=true;
}

void MainWindow::on_selectPlayer_clicked()
{
    QList<QListWidgetItem *> items=ui->playerList->selectedItems();
    if(items.size()==1)
        on_playerList_itemActivated(items.first());
}

void MainWindow::on_cash_editingFinished()
{
    if(!havePlayerSelected)
        return;
    QString queryText;
    switch(db_type)
    {
        default:
        case db_type_mysql:
            queryText=QString("UPDATE player SET cash=%1 WHERE id=%2").arg(ui->cash->value()).arg(player_id);
        break;
        case db_type_sqlite:
            queryText=QString("UPDATE player SET cash=%1 WHERE id=%2").arg(ui->cash->value()).arg(player_id);
        break;
    }
    QSqlQuery updateCashPlayer(queryText);
}

void MainWindow::on_datapack_path_browse_clicked()
{
    QString folder=QFileDialog::getExistingDirectory(this,"Select the datapack");
    if(folder.isEmpty())
        return;
    ui->datapack_path->setText(folder);
    on_datapack_path_textChanged(folder);
}

void MainWindow::on_datapack_path_textChanged(const QString &arg1)
{
    datapack=ui->datapack_path->text();
    if(!datapack.endsWith('/') && !datapack.endsWith('\\'))
        datapack+="/";
    bool datapackIsValid=false;
    if(!arg1.isEmpty() && QFile(datapack+"/"+DATAPACK_BASE_PATH_ITEM+"items.xml").exists())
        datapackIsValid=true;
    ui->groupBoxMysql->setEnabled(datapackIsValid);
    ui->groupBoxSQLite->setEnabled(datapackIsValid);
}

void MainWindow::load_inventory()
{
    ui->items->clear();
    items_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();
        QListWidgetItem *item=new QListWidgetItem();
        items_graphical[item]=i.key();
        if(DatapackClientLoader::items.contains(i.key()))
        {
            item->setIcon(DatapackClientLoader::items[i.key()].image);
            item->setText(QString("%1 (%2)").arg(DatapackClientLoader::items[i.key()].name).arg(i.value()));
        }
        else
        {
            item->setIcon(datapackLoader.defaultInventoryImage());
            item->setText(QString("??? (id: %1, x%2)").arg(i.key()).arg(i.value()));
        }
        item->setToolTip(DatapackClientLoader::items[i.key()].description);
        ui->items->addItem(item);
    }
}

void MainWindow::on_add_item_clicked()
{
    ItemDialog itemDialog(this);
    itemDialog.exec();
    if(!itemDialog.haveItemSelected())
        return;

    QString queryText;
    if(items.contains(itemDialog.itemId()))
    {
        items[itemDialog.itemId()]+=itemDialog.itemQuantity();
        switch(db_type)
        {
            default:
            case db_type_mysql:
                queryText=QString("UPDATE item SET quantity=%1 WHERE player_id=%2 AND item_id=%3").arg(items[itemDialog.itemId()]).arg(player_id).arg(itemDialog.itemId());
            break;
            case db_type_sqlite:
                queryText=QString("UPDATE item SET quantity=%1 WHERE player_id=%2 AND item_id=%3").arg(items[itemDialog.itemId()]).arg(player_id).arg(itemDialog.itemId());
            break;
        }
    }
    else
    {
        items[itemDialog.itemId()]=itemDialog.itemQuantity();
        switch(db_type)
        {
            default:
            case db_type_mysql:
                queryText=QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);").arg(itemDialog.itemId()).arg(player_id).arg(items[itemDialog.itemId()]);
            break;
            case db_type_sqlite:
                queryText=QString("INSERT INTO item(item_id,player_id,quantity) VALUES(%1,%2,%3);").arg(itemDialog.itemId()).arg(player_id).arg(items[itemDialog.itemId()]);
            break;
        }
    }
    QSqlQuery updateItemPlayer(queryText);
    load_inventory();
}

void MainWindow::on_items_itemSelectionChanged()
{
    ui->remove_item->setEnabled(ui->items->selectedItems().size()==1);
}

void MainWindow::on_remove_item_clicked()
{
    if(ui->items->selectedItems().size()!=1)
        return;

    QString queryText;
    switch(db_type)
    {
        default:
        case db_type_mysql:
            queryText=QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2").arg(player_id).arg(items_graphical[ui->items->selectedItems().first()]);
        break;
        case db_type_sqlite:
            queryText=QString("DELETE FROM item WHERE player_id=%1 AND item_id=%2").arg(player_id).arg(items_graphical[ui->items->selectedItems().first()]);
        break;
    }
    QSqlQuery updateItemPlayer(queryText);

    items.remove(items_graphical[ui->items->selectedItems().first()]);
    load_inventory();
}
