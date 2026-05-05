#include "MainWindow.hpp"
#include "ui_MainWindow.h"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../base/NormalServerGlobal.hpp"

#include <QCoreApplication>
#include <QString>
#include <QStringLiteral>

using namespace CatchChallenger;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    settings(nullptr),
    need_be_closed(false)
{
    settings=new TinyXMLSettings(
        (QCoreApplication::applicationDirPath()
         +QStringLiteral("/server-properties.xml")).toStdString());
    // checkSettingsFile lives on NormalServerGlobal (was on
    // NormalServer in the qmake-era code). It seeds default values
    // for every key the server reads at start_server() time so the
    // first launch in an empty directory still produces a runnable
    // config.
    NormalServerGlobal::checkSettingsFile(
        settings,
        FacilityLibGeneral::getFolderFromFile(
            FacilityLibGeneral::applicationDirPath)+"/datapack/");

    ui->setupUi(this);
    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<Player_private_and_public_informations>(
        "Player_private_and_public_informations");

    connect(&server,&NormalServer::is_started,
            this,&MainWindow::server_is_started);
    connect(&server,&NormalServer::need_be_stopped,
            this,&MainWindow::server_need_be_stopped);
    connect(&server,&NormalServer::need_be_restarted,
            this,&MainWindow::server_need_be_restarted);
    connect(&server,&NormalServer::new_player_is_connected,
            this,&MainWindow::new_player_is_connected);
    connect(&server,&NormalServer::player_is_disconnected,
            this,&MainWindow::player_is_disconnected);
    connect(&server,&NormalServer::new_chat_message,
            this,&MainWindow::new_chat_message);
    connect(&server,&NormalServer::error,
            this,&MainWindow::server_error);
    connect(&server,&NormalServer::haveQuitForCriticalDatabaseQueryFailed,
            this,&MainWindow::haveQuitForCriticalDatabaseQueryFailed);

    ui->tabWidget->setCurrentIndex(0);
    updateActionButton();
}

MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
}

void MainWindow::autostart()
{
    on_pushButton_server_start_clicked();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    event->ignore();
    this->hide();
    need_be_closed=true;
    if(!server.isStopped())
        server_need_be_stopped();
    else
        QCoreApplication::exit();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    if(e->type()==QEvent::LanguageChange)
        ui->retranslateUi(this);
}

void MainWindow::updateActionButton()
{
    const bool stopped=server.isStopped();
    ui->pushButton_server_start->setEnabled(stopped);
    ui->pushButton_server_stop->setEnabled(!stopped);
    ui->pushButton_server_restart->setEnabled(!stopped);
}

void MainWindow::on_pushButton_server_start_clicked()
{
    server.start_server();
    updateActionButton();
}

void MainWindow::on_pushButton_server_stop_clicked()
{
    server.stop_server();
    updateActionButton();
}

void MainWindow::on_pushButton_server_restart_clicked()
{
    server.stop_server();
    // NormalServer emits need_be_stopped → server_need_be_stopped(),
    // which calls start_server() again when need_be_restarted is set.
    // For a manual restart, just stop here; the user clicks Start
    // again. Avoids depending on the legacy auto-restart flag.
    updateActionButton();
}

void MainWindow::server_is_started(bool /*is_started*/)
{
    updateActionButton();
}

void MainWindow::server_need_be_stopped()
{
    server.stop_server();
    updateActionButton();
    if(need_be_closed && server.isStopped())
        QCoreApplication::exit();
}

void MainWindow::server_need_be_restarted()
{
    server.stop_server();
    updateActionButton();
}

void MainWindow::new_player_is_connected(Player_private_and_public_informations player)
{
    QListWidgetItem *item=new QListWidgetItem(QString::fromStdString(player.public_informations.pseudo));
    ui->listPlayer->addItem(item);
}

void MainWindow::player_is_disconnected(std::string pseudo)
{
    int idx=0;
    while(idx<ui->listPlayer->count())
    {
        if(ui->listPlayer->item(idx)->text().toStdString()==pseudo)
        {
            QListWidgetItem *gone=ui->listPlayer->takeItem(idx);
            delete gone;
            return;
        }
        idx++;
    }
}

void MainWindow::new_chat_message(std::string pseudo,Chat_type /*type*/,std::string text)
{
    // Plain-text concat: the qmake-era ChatParsing helper that did
    // HTML formatting lives client-side and pulling client code into
    // the server target would create a circular dep. Plain text is
    // good enough for an admin chat log.
    ui->textBrowserChat->append(
        QString::fromStdString(pseudo)+": "+QString::fromStdString(text));
}

void MainWindow::server_error(std::string error)
{
    ui->textBrowserChat->append(
        QStringLiteral("[server error] ")+QString::fromStdString(error));
}

void MainWindow::haveQuitForCriticalDatabaseQueryFailed()
{
    ui->textBrowserChat->append(
        QStringLiteral("[server fatal] critical database query failed; quitting"));
    QCoreApplication::exit(1);
}
