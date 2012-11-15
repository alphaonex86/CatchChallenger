#include "MainWindow.h"
#include "ui_MainWindow.h"

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
            queryText=QString("SELECT pseudo,id FROM player");
        break;
        case db_type_sqlite:
            queryText=QString("SELECT pseudo,id FROM player");
        break;
    }
    QSqlQuery listQuery(queryText);

    //parse the result
    while(listQuery.next())
    {
        if(ui->searchPlayer->text().isEmpty() || listQuery.value(0).toString().contains(ui->searchPlayer->text(),Qt::CaseInsensitive))
        {
            bool ok;
            quint32 id=listQuery.value(1).toUInt(&ok);
            if(ok)
            {
                QListWidgetItem *item=new QListWidgetItem(QIcon(":/im-user.png"),QString(listQuery.value(0).toString()));
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
    if(infoPlayer.size()!=1)
    {
        QMessageBox::warning(this,"Warning",QString("The player can't be loaded: %1").arg(infoPlayer.lastError().text()));
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
