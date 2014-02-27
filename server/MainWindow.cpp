#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QFileDialog>

using namespace CatchChallenger;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings=new QSettings(QCoreApplication::applicationDirPath()+QStringLiteral("/server.properties"),QSettings::IniFormat);
    NormalServer::checkSettingsFile(settings);
    ui->setupUi(this);
    on_MapVisibilityAlgorithm_currentIndexChanged(0);
    updateActionButton();
    qRegisterMetaType<Chat_type>("Chat_type");
    connect(&server,&NormalServer::is_started,          this,&MainWindow::server_is_started);
    connect(&server,&NormalServer::need_be_stopped,     this,&MainWindow::server_need_be_stopped);
    connect(&server,&NormalServer::need_be_restarted,   this,&MainWindow::server_need_be_restarted);
    connect(&server,&NormalServer::new_player_is_connected,this,&MainWindow::new_player_is_connected);
    connect(&server,&NormalServer::player_is_disconnected,this,&MainWindow::player_is_disconnected);
    connect(&server,&NormalServer::new_chat_message,    this,&MainWindow::new_chat_message);
    connect(&server,&NormalServer::error,               this,&MainWindow::server_error);
    connect(&server,&NormalServer::benchmark_result,    this,&MainWindow::benchmark_result);
    connect(&timer_update_the_info,&QTimer::timeout,    this,&MainWindow::update_the_info);
    connect(&check_latency,&QTimer::timeout,            this,&MainWindow::start_calculate_latency);
    connect(this,&MainWindow::record_latency,           this,&MainWindow::stop_calculate_latency,Qt::QueuedConnection);
    timer_update_the_info.start(200);
    check_latency.setSingleShot(true);
    check_latency.start(1000);
    need_be_restarted=false;
    need_be_closed=false;
    ui->tabWidget->setCurrentIndex(0);
    internal_currentLatency=0;
    load_settings();
    updateDbGroupbox();
}

MainWindow::~MainWindow()
{
    delete settings;
    delete ui;
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
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void MainWindow::on_lineEdit_returnPressed()
{
    ui->lineEdit->setText("");
}

void MainWindow::updateActionButton()
{
    ui->pushButton_server_start->setEnabled(server.isStopped());
    ui->pushButton_server_restart->setEnabled(server.isListen());
    ui->pushButton_server_stop->setEnabled(server.isListen());
    ui->pushButton_server_benchmark->setEnabled(server.isStopped());
}

void MainWindow::on_pushButton_server_start_clicked()
{
    ui->pushButton_server_start->setEnabled(false);
    send_settings();
    server.start_server();
}

void MainWindow::on_pushButton_server_stop_clicked()
{
    ui->pushButton_server_restart->setEnabled(false);
    ui->pushButton_server_stop->setEnabled(false);
    server.stop_server();
}

void MainWindow::on_pushButton_server_restart_clicked()
{
    ui->pushButton_server_restart->setEnabled(false);
    ui->pushButton_server_stop->setEnabled(false);
    need_be_restarted=true;
    server.stop_server();
}

void MainWindow::server_is_started(bool is_started)
{
    updateActionButton();
    if(need_be_closed || !isVisible())
    {
        QCoreApplication::exit();
        return;
    }
    if(!is_started)
    {
        clean_updated_info();
        if(need_be_restarted)
        {
            need_be_restarted=false;
            send_settings();
            server.start_server();
        }
    }
}

void MainWindow::server_need_be_stopped()
{
    server.stop_server();
}

void MainWindow::server_need_be_restarted()
{
    need_be_restarted=true;
    server.stop_server();
}

void MainWindow::new_player_is_connected(Player_internal_informations player)
{
    QIcon icon;
    switch(player.public_and_private_informations.public_informations.type)
    {
        case Player_type_premium:
            icon=QIcon(":/images/chat/premium.png");
        break;
        case Player_type_gm:
            icon=QIcon(":/images/chat/admin.png");
        break;
        case Player_type_dev:
            icon=QIcon(":/images/chat/developer.png");
        break;
        default:
        break;
    }

    ui->listPlayer->addItem(new QListWidgetItem(icon,player.public_and_private_informations.public_informations.pseudo));
    players << player;
}

void MainWindow::player_is_disconnected(QString pseudo)
{
    QList<QListWidgetItem *> tempList=ui->listPlayer->findItems(pseudo,Qt::MatchExactly);
    int index=0;
    while(index<tempList.size())
    {
        delete tempList.at(index);
        index++;
    }
    index=0;
    while(index<players.size())
    {
        if(players.at(index).public_and_private_informations.public_informations.pseudo==pseudo)
        {
            players.removeAt(index);
            break;
        }
        index++;
    }
}

void MainWindow::new_chat_message(QString pseudo,Chat_type type,QString text)
{
    int index=0;
    while(index<players.size())
    {
        if(players.at(index).public_and_private_informations.public_informations.pseudo==pseudo)
        {
            QString html=ui->textBrowserChat->toHtml();
            html+=ChatParsing::new_chat_message(players.at(index).public_and_private_informations.public_informations.pseudo,players.at(index).public_and_private_informations.public_informations.type,type,text);
            if(html.size()>1024*1024)
                html=html.mid(html.size()-1024*1024,1024*1024);
            ui->textBrowserChat->setHtml(html);
            return;
        }
        index++;
    }
    QMessageBox::information(this,"warning","unable to locate the player");
}

void MainWindow::server_error(QString error)
{
    QMessageBox::information(this,"warning",error);
}

void MainWindow::update_the_info()
{
    if(ui->listLatency->count()<=0)
        ui->listLatency->addItem(tr("%1ms").arg(internal_currentLatency));
    else
        ui->listLatency->item(0)->setText(tr("%1ms").arg(internal_currentLatency));
    if(server.isListen() || server.isInBenchmark())
    {
        quint16 player_current,player_max;
        player_current=server.player_current();
        player_max=server.player_max();
        ui->label_player->setText(QStringLiteral("%1/%2").arg(player_current).arg(player_max));
        ui->progressBar_player->setMaximum(player_max);
        ui->progressBar_player->setValue(player_current);
    }
    else
    {
        ui->progressBar_player->setValue(0);
    }
}

QString MainWindow::sizeToString(double size)
{
    if(size<1024)
        return QString::number(size)+tr("B");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("KB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("MB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("GB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("TB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("PB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("EB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("ZB");
    if((size=size/1024)<1024)
        return adaptString(size)+tr("YB");
    return tr("Too big");
}

QString MainWindow::adaptString(float size)
{
    if(size>=100)
        return QString::number(size,'f',0);
    else
        return QString::number(size,'g',3);
}


void MainWindow::start_calculate_latency()
{
    time_latency.restart();
    emit record_latency();
}

void MainWindow::stop_calculate_latency()
{
    internal_currentLatency=time_latency.elapsed();
    check_latency.start();
}

void MainWindow::benchmark_result(int latency,double TX_speed,double RX_speed,double TX_size,double RX_size,double second)
{
    updateActionButton();
    clean_updated_info();
    QMessageBox::information(this,"benchmark_map",tr("The latency of the benchmark: %1\nTX_speed: %2/s, RX_speed %3/s\nTX_size: %4, RX_size: %5, in %6s\nThis latency is cumulated latency of different point. That's not show the database performance.")
                 .arg(latency)
                 .arg(sizeToString(TX_speed))
                 .arg(sizeToString(RX_speed))
                 .arg(sizeToString(TX_size))
                 .arg(sizeToString(RX_size))
                 .arg(second)
                 );
}

void MainWindow::clean_updated_info()
{
    ui->label_player->setText("?/?");
    ui->listLatency->clear();
}

void MainWindow::load_settings()
{
    // --------------------------------------------------
    ui->max_player->setValue(settings->value("max-players").toUInt());
    ui->server_ip->setText(settings->value("server-ip").toString());
    ui->pvp->setChecked(settings->value("pvp").toBool());
    ui->sendPlayerNumber->setChecked(settings->value("sendPlayerNumber").toBool());
    ui->server_port->setValue(settings->value("server-port").toUInt());
    ui->benchmark_benchmarkMap->setChecked(settings->value("benchmark_map").toBool());
    ui->benchmark_seconds->setValue(settings->value("benchmark_seconds").toUInt());
    ui->benchmark_clients->setValue(settings->value("benchmark_clients").toUInt());
    ui->tolerantMode->setChecked(settings->value("tolerantMode").toBool());
    ui->forceClientToSendAtBorder->setChecked(settings->value("forceClientToSendAtBorder").toBool());
    if(settings->value("compression").toString()=="none")
        ui->compression->setCurrentIndex(0);
    else if(settings->value("compression").toString()=="xz")
        ui->compression->setCurrentIndex(2);
    else
        ui->compression->setCurrentIndex(1);
    ui->min_character->setValue(settings->value("min_character").toUInt());
    ui->max_character->setValue(settings->value("max_character").toUInt());
    ui->max_pseudo_size->setValue(settings->value("max_pseudo_size").toUInt());
    ui->character_delete_time->setValue(settings->value("character_delete_time").toUInt()/3600);
    ui->automatic_account_creation->setChecked(settings->value("automatic_account_creation").toBool());
    ui->anonymous->setChecked(settings->value("anonymous").toBool());
    ui->min_character->setMaximum(ui->max_character->value());
    ui->max_character->setMinimum(ui->min_character->value());
    ui->server_message->setPlainText(settings->value("server_message").toString());
    ui->proxy->setText(settings->value("proxy").toString());
    ui->proxy_port->setValue(settings->value("proxy_port").toUInt());
    if(settings->value("forcedSpeed").toUInt()==0)
    {
        ui->forceSpeed->setChecked(true);
        ui->speed->setEnabled(false);
    }
    else
    {
        ui->forceSpeed->setChecked(false);
        ui->speed->setValue(settings->value("forcedSpeed").toUInt());
        ui->speed->setEnabled(true);
    }
    ui->dontSendPseudo->setChecked(settings->value("dontSendPseudo").toBool());
    ui->dontSendPlayerType->setChecked(settings->value("dontSendPlayerType").toBool());

    quint32 tempValue=0;
    settings->beginGroup("MapVisibilityAlgorithm");
    tempValue=settings->value("MapVisibilityAlgorithm").toUInt();
    settings->endGroup();
    if(tempValue<(quint32)ui->MapVisibilityAlgorithm->count())
        ui->MapVisibilityAlgorithm->setCurrentIndex(tempValue);

    quint32 reshow=0;
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    tempValue=settings->value("Max").toUInt();
    reshow=settings->value("Reshow").toUInt();
    if(reshow>tempValue)
    {
        DebugClass::debugConsole("Reshow number corrected");
        reshow=tempValue;
        settings->setValue("Reshow",reshow);
    }
    settings->endGroup();
    ui->MapVisibilityAlgorithmSimpleMax->setValue(tempValue);
    ui->MapVisibilityAlgorithmSimpleReshow->setValue(reshow);

    switch(ui->MapVisibilityAlgorithm->currentIndex())
    {
        case 0:
            update_benchmark();
        break;
        default:
        break;
    }

    {
        settings->beginGroup("rates");
        double rates_xp_normal=settings->value("xp_normal").toFloat();
        double rates_gold_normal=settings->value("gold_normal").toFloat();
        double rates_xp_pow_normal=settings->value("xp_pow_normal").toFloat();
        double rates_drop_normal=settings->value("drop_normal").toFloat();
        settings->endGroup();

        ui->rates_xp_normal->setValue(rates_xp_normal);
        ui->rates_gold_normal->setValue(rates_gold_normal);
        ui->rates_xp_pow_normal->setValue(rates_xp_pow_normal);
        ui->rates_drop_normal->setValue(rates_drop_normal);
    }

    {
        settings->beginGroup("chat");
        bool chat_allow_all=settings->value("allow-all").toBool();
        bool chat_allow_local=settings->value("allow-local").toBool();
        bool chat_allow_private=settings->value("allow-private").toBool();
        bool chat_allow_clan=settings->value("allow-clan").toBool();
        settings->endGroup();

        ui->chat_allow_all->setChecked(chat_allow_all);
        ui->chat_allow_local->setChecked(chat_allow_local);
        ui->chat_allow_private->setChecked(chat_allow_private);
        ui->chat_allow_clan->setChecked(chat_allow_clan);
    }

    settings->beginGroup("db");
    QString db_type=settings->value("type").toString();
    QString db_mysql_host=settings->value("mysql_host").toString();
    QString db_mysql_login=settings->value("mysql_login").toString();
    QString db_mysql_pass=settings->value("mysql_pass").toString();
    QString db_mysql_base=settings->value("mysql_db").toString();
    QString db_fight_sync=settings->value("db_fight_sync").toString();
    bool positionTeleportSync=settings->value("positionTeleportSync").toBool();
    quint32 secondToPositionSync=settings->value("secondToPositionSync").toUInt();

    if(!settings->contains("db_fight_sync"))
        settings->setValue("db_fight_sync","FightSync_AtTheEndOfBattle");
    settings->endGroup();

    if(db_type=="mysql")
        ui->db_type->setCurrentIndex(0);
    else if(db_type=="sqlite")
        ui->db_type->setCurrentIndex(1);
    else
        ui->db_type->setCurrentIndex(0);
    ui->db_mysql_host->setText(db_mysql_host);
    ui->db_mysql_login->setText(db_mysql_login);
    ui->db_mysql_pass->setText(db_mysql_pass);
    ui->db_mysql_base->setText(db_mysql_base);
    if(db_fight_sync=="FightSync_AtEachTurn")
        ui->db_fight_sync->setCurrentIndex(0);
    else if(db_fight_sync=="FightSync_AtTheEndOfBattle")
        ui->db_fight_sync->setCurrentIndex(1);
    else if(db_fight_sync=="FightSync_AtTheDisconnexion")
        ui->db_fight_sync->setCurrentIndex(2);
    else
        ui->db_fight_sync->setCurrentIndex(0);
    ui->positionTeleportSync->setChecked(positionTeleportSync);
    ui->secondToPositionSync->setValue(secondToPositionSync);

    ui->db_sqlite_file->setText(QCoreApplication::applicationDirPath()+"/catchchallenger.db.sqlite");

    {
        settings->beginGroup("city");
        if(!settings->contains("capture_frequency"))
            settings->setValue("capture_frequency","day");
        int capture_frequency_int=0;
        if(settings->value("capture_frequency").toString()=="week")
            capture_frequency_int=0;
        else if(settings->value("capture_frequency").toString()=="month")
            capture_frequency_int=1;
        update_capture();
        int capture_day_int=0;
        if(settings->value("capture_day").toString()=="monday")
            capture_day_int=0;
        else if(settings->value("capture_day").toString()=="tuesday")
            capture_day_int=1;
        else if(settings->value("capture_day").toString()=="wednesday")
            capture_day_int=2;
        else if(settings->value("capture_day").toString()=="thursday")
            capture_day_int=3;
        else if(settings->value("capture_day").toString()=="friday")
            capture_day_int=4;
        else if(settings->value("capture_day").toString()=="saturday")
            capture_day_int=5;
        else if(settings->value("capture_day").toString()=="sunday")
            capture_day_int=6;
        int capture_time_hours=0,capture_time_minutes=0;
        QStringList capture_time_string_list=settings->value("capture_time").toString().split(":");
        if(capture_time_string_list.size()!=2)
            settings->setValue("capture_time",QStringLiteral("0:0"));
        else
        {
            bool ok;
            capture_time_hours=capture_time_string_list.first().toUInt(&ok);
            if(!ok)
                settings->setValue("capture_time",QStringLiteral("0:0"));
            else
            {
                capture_time_minutes=capture_time_string_list.last().toUInt(&ok);
                if(!ok)
                    settings->setValue("capture_time",QStringLiteral("0:0"));
            }
        }
        settings->endGroup();
        ui->comboBox_city_capture_frequency->setCurrentIndex(capture_frequency_int);
        ui->comboBox_city_capture_day->setCurrentIndex(capture_day_int);
        ui->timeEdit_city_capture_time->setTime(QTime(capture_time_hours,capture_time_minutes));
    }

    {
        bool ok;
        ServerSettings::Bitcoin bitcoin;
        settings->beginGroup("bitcoin");
        if(!settings->contains("enabled"))
            settings->setValue("enabled",false);
        if(!settings->contains("address"))
            settings->setValue("address","1Hz3GtkiDBpbWxZixkQPuTGDh2DUy9bQUJ");
        if(!settings->contains("fee"))
            settings->setValue("fee",1.0);
        if(!settings->contains("history"))
            settings->setValue("history",30);
        #ifdef Q_OS_WIN32
        if(!settings->contains("binaryPath"))
            settings->setValue("binaryPath","%application_path%/bitcoin/bitcoind.exe");
        if(!settings->contains("workingPath"))
            settings->setValue("workingPath","%application_path%/bitcoin-storage/");
        #else
        if(!settings->contains("binaryPath"))
            settings->setValue("binaryPath","/usr/bin/bitcoind");
        if(!settings->contains("workingPath"))
            settings->setValue("workingPath",QDir::homePath()+"/.CatchChallenger/bitoin/");
        #endif
        if(!settings->contains("port"))
            settings->setValue("port",46349);

        bitcoin.enabled=settings->value("enabled").toBool();
        bitcoin.address=settings->value("address").toString();
        if(!bitcoin.address.contains(QRegularExpression(CATCHCHALLENGER_SERVER_BITCOIN_ADDRESS_REGEX)))
            bitcoin.enabled=false;
        bitcoin.fee=settings->value("fee").toDouble(&ok);
        if(!ok)
            bitcoin.fee=1.0;
        if(bitcoin.fee<0 || bitcoin.fee>100)
            bitcoin.fee=1.0;
        bitcoin.binaryPath=settings->value("binaryPath").toString();
        bitcoin.workingPath=settings->value("workingPath").toString();
        int port=settings->value("port").toUInt(&ok);
        if(!ok)
            port=46349;
        if(port<1 || port>65534)
            port=46349;
        bitcoin.port=port;
        settings->endGroup();
        ui->bitcoin_enabled->setChecked(bitcoin.enabled);
        ui->bitcoin_address->setText(bitcoin.address);
        ui->bitcoin_fee->setValue(bitcoin.fee);
        ui->bitcoin_binarypath->setText(bitcoin.binaryPath);
        ui->bitcoin_workingpath->setText(bitcoin.workingPath);
        ui->bitcoin_port->setValue(bitcoin.port);
        on_bitcoin_address_editingFinished();
    }

    send_settings();
}

void MainWindow::send_settings()
{
    ServerSettings formatedServerSettings=server.getSettings();

    //common var
    CommonSettings::commonSettings.min_character					= ui->min_character->value();
    CommonSettings::commonSettings.max_character					= ui->max_character->value();
    CommonSettings::commonSettings.max_pseudo_size					= ui->max_pseudo_size->value();
    CommonSettings::commonSettings.character_delete_time			= ui->character_delete_time->value()*3600;

    if(!ui->forceSpeed->isChecked())
        CommonSettings::commonSettings.forcedSpeed					= 0;
    else
        CommonSettings::commonSettings.forcedSpeed					= ui->speed->value();
    formatedServerSettings.dontSendPlayerType                       = ui->dontSendPlayerType->isChecked();
    CommonSettings::commonSettings.dontSendPseudo					= ui->dontSendPseudo->isChecked();
    CommonSettings::commonSettings.forceClientToSendAtBorder		= ui->forceClientToSendAtBorder->isChecked();

    //the listen
    formatedServerSettings.server_port					= ui->server_port->value();
    formatedServerSettings.server_ip					= ui->server_ip->text();
    formatedServerSettings.anonymous					= ui->anonymous->isChecked();
    formatedServerSettings.server_message				= ui->server_message->toPlainText();
    formatedServerSettings.proxy    					= ui->proxy->text();
    formatedServerSettings.proxy_port					= ui->proxy_port->value();



    //fight
    formatedServerSettings.pvp			= ui->pvp->isChecked();
    formatedServerSettings.sendPlayerNumber		= ui->sendPlayerNumber->isChecked();

    //compression
    switch(ui->compression->currentIndex())
    {
        case 0:
        formatedServerSettings.compressionType=CatchChallenger::CompressionType_None;
        break;
        default:
        case 1:
        formatedServerSettings.compressionType=CatchChallenger::CompressionType_Zlib;
        break;
        case 2:
        formatedServerSettings.compressionType=CatchChallenger::CompressionType_Xz;
        break;
    }

    //rates
    CommonSettings::commonSettings.rates_xp			= ui->rates_xp_normal->value();
    CommonSettings::commonSettings.rates_gold		= ui->rates_gold_normal->value();
    CommonSettings::commonSettings.rates_xp_pow     = ui->rates_xp_pow_normal->value();
    CommonSettings::commonSettings.rates_drop		= ui->rates_drop_normal->value();

    //chat allowed
    CommonSettings::commonSettings.chat_allow_all		= ui->chat_allow_all->isChecked();
    CommonSettings::commonSettings.chat_allow_local		= ui->chat_allow_local->isChecked();
    CommonSettings::commonSettings.chat_allow_private		= ui->chat_allow_private->isChecked();
    CommonSettings::commonSettings.chat_allow_clan		= ui->chat_allow_clan->isChecked();

    switch(ui->db_type->currentIndex())
    {
        default:
        case 0:
            formatedServerSettings.database.type					= ServerSettings::Database::DatabaseType_Mysql;
        break;
        case 1:
            formatedServerSettings.database.type					= ServerSettings::Database::DatabaseType_SQLite;
        break;
    }
    switch(formatedServerSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            formatedServerSettings.database.mysql.host				= ui->db_mysql_host->text();
            formatedServerSettings.database.mysql.db				= ui->db_mysql_base->text();
            formatedServerSettings.database.mysql.login				= ui->db_mysql_login->text();
            formatedServerSettings.database.mysql.pass				= ui->db_mysql_pass->text();
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            formatedServerSettings.database.sqlite.file				= ui->db_sqlite_file->text();
        break;
    }
    formatedServerSettings.database.fightSync                       = (ServerSettings::Database::FightSync)ui->db_fight_sync->currentIndex();
    formatedServerSettings.database.positionTeleportSync=ui->positionTeleportSync->isChecked();
    formatedServerSettings.database.secondToPositionSync=ui->secondToPositionSync->value();

    //connection
    formatedServerSettings.automatic_account_creation   = ui->automatic_account_creation->isChecked();
    formatedServerSettings.max_players					= ui->max_player->value();
    formatedServerSettings.tolerantMode                 = ui->tolerantMode->isChecked();

    //visibility algorithm
    switch(ui->MapVisibilityAlgorithm->currentIndex())
    {
        case 0:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_simple;
        break;
        case 1:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_none;
        break;
        default:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithm_simple;
        break;
    }

    formatedServerSettings.mapVisibility.simple.max				= ui->MapVisibilityAlgorithmSimpleMax->value();
    formatedServerSettings.mapVisibility.simple.reshow			= ui->MapVisibilityAlgorithmSimpleReshow->value();

    switch(ui->comboBox_city_capture_frequency->currentIndex())
    {
        default:
        case 0:
            formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
        break;
        case 1:
            formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;
        break;
    }
    switch(ui->comboBox_city_capture_day->currentIndex())
    {
        default:
        case 0:
            formatedServerSettings.city.capture.day=City::Capture::Monday;
        break;
        case 1:
            formatedServerSettings.city.capture.day=City::Capture::Tuesday;
        break;
        case 2:
            formatedServerSettings.city.capture.day=City::Capture::Wednesday;
        break;
        case 3:
            formatedServerSettings.city.capture.day=City::Capture::Thursday;
        break;
        case 4:
            formatedServerSettings.city.capture.day=City::Capture::Friday;
        break;
        case 5:
            formatedServerSettings.city.capture.day=City::Capture::Saturday;
        break;
        case 6:
            formatedServerSettings.city.capture.day=City::Capture::Sunday;
        break;
    }
    QTime time=ui->timeEdit_city_capture_time->time();
    formatedServerSettings.city.capture.hour=time.hour();
    formatedServerSettings.city.capture.minute=time.minute();

    formatedServerSettings.bitcoin.address=ui->bitcoin_address->text();
    formatedServerSettings.bitcoin.binaryPath=ui->bitcoin_binarypath->text();
    formatedServerSettings.bitcoin.enabled=ui->bitcoin_enabled->isChecked();
    formatedServerSettings.bitcoin.fee=ui->bitcoin_fee->value();
    formatedServerSettings.bitcoin.port=ui->bitcoin_port->value();
    formatedServerSettings.bitcoin.workingPath=ui->bitcoin_workingpath->text();

    server.setSettings(formatedServerSettings);
}

void MainWindow::update_benchmark()
{
    switch(ui->MapVisibilityAlgorithm->currentIndex())
    {
        case 0:
            ui->labelBenchTip->setText(tr("The number of bot should be lower than %1*number of map\nSee the max client into the visibility algorithm\nNeed be lower than max player: %2")
                                       .arg(ui->MapVisibilityAlgorithmSimpleMax->value())
                                       .arg(ui->max_player->value())
                                       );
            ui->pushButton_server_benchmark->setEnabled(true);
        break;
        default:
            ui->labelBenchTip->setText(tr("You need select another visibility algorithm"));
            ui->pushButton_server_benchmark->setEnabled(false);
        break;
    }
    if(ui->benchmark_clients->value()>ui->MapVisibilityAlgorithmSimpleMax->value())
        ui->benchmark_clients->setStyleSheet("color: rgb(255, 0, 0);background-color: rgb(255, 230, 230);");
    else
        ui->benchmark_clients->setStyleSheet("");
    ui->benchmark_clients->setMaximum(ui->max_player->value());
}


void MainWindow::on_max_player_valueChanged(int arg1)
{
    settings->setValue("max-players",arg1);
}

void MainWindow::on_server_ip_editingFinished()
{
    settings->setValue("server-ip",ui->server_ip->text());
}

void MainWindow::on_pvp_stateChanged(int arg1)
{
    Q_UNUSED(arg1)
    settings->setValue("pvp",ui->pvp->isChecked());
}

void MainWindow::on_server_port_valueChanged(int arg1)
{
    settings->setValue("server-port",arg1);
}

void MainWindow::on_rates_xp_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("xp_normal",arg1);
    settings->endGroup();
}

void MainWindow::on_rates_gold_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("gold_normal",arg1);
    settings->endGroup();
}

void MainWindow::on_chat_allow_all_toggled(bool checked)
{
    settings->beginGroup("chat");
    settings->setValue("allow-all",checked);
    settings->endGroup();
}

void MainWindow::on_chat_allow_local_toggled(bool checked)
{
    settings->beginGroup("chat");
    settings->setValue("allow-local",checked);
    settings->endGroup();
}

void MainWindow::on_chat_allow_private_toggled(bool checked)
{
    settings->beginGroup("chat");
    settings->setValue("allow-private",checked);
    settings->endGroup();
}

void MainWindow::on_chat_allow_clan_toggled(bool checked)
{
    settings->beginGroup("chat");
    settings->setValue("allow-clan",checked);
    settings->endGroup();
}

void MainWindow::on_db_mysql_host_editingFinished()
{
    settings->beginGroup("db");
    settings->setValue("mysql_host",ui->db_mysql_host->text());
    settings->endGroup();
}

void MainWindow::on_db_mysql_login_editingFinished()
{
    settings->beginGroup("db");
    settings->setValue("mysql_login",ui->db_mysql_login->text());
    settings->endGroup();
}

void MainWindow::on_db_mysql_pass_editingFinished()
{
    settings->beginGroup("db");
    settings->setValue("mysql_pass",ui->db_mysql_pass->text());
    settings->endGroup();
}

void MainWindow::on_db_mysql_base_editingFinished()
{
    settings->beginGroup("db");
    settings->setValue("mysql_db",ui->db_mysql_base->text());
    settings->endGroup();
}

void MainWindow::on_pushButton_server_benchmark_clicked()
{
    send_settings();
    server.start_benchmark(ui->benchmark_seconds->value(),ui->benchmark_clients->value(),ui->benchmark_benchmarkMap->isChecked());
    updateActionButton();
}

void MainWindow::on_MapVisibilityAlgorithm_currentIndexChanged(int index)
{
    if(index==0)
        ui->groupBoxMapVisibilityAlgorithmSimple->setEnabled(true);
    settings->beginGroup("MapVisibilityAlgorithm");
    settings->setValue("MapVisibilityAlgorithm",index);
    settings->endGroup();
}

void MainWindow::on_MapVisibilityAlgorithmSimpleMax_valueChanged(int arg1)
{
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    settings->setValue("Max",arg1);
    settings->endGroup();
    ui->MapVisibilityAlgorithmSimpleReshow->setMaximum(arg1);
    update_benchmark();
}


void MainWindow::on_MapVisibilityAlgorithmSimpleReshow_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    settings->setValue("Reshow",ui->MapVisibilityAlgorithmSimpleReshow->value());
    settings->endGroup();
}

void MainWindow::on_benchmark_benchmarkMap_clicked()
{
    settings->setValue("benchmark_map",ui->benchmark_benchmarkMap->isChecked());
}

void MainWindow::on_benchmark_seconds_valueChanged(int arg1)
{
    settings->setValue("benchmark_seconds",arg1);
}

void MainWindow::on_benchmark_clients_valueChanged(int arg1)
{
    settings->setValue("benchmark_clients",arg1);
    update_benchmark();
}

void MainWindow::on_db_type_currentIndexChanged(int index)
{
    settings->beginGroup("db");
    switch(index)
    {
        case 1:
            settings->setValue("type","sqlite");
        break;
        case 0:
        default:
            settings->setValue("type","mysql");
        break;
    }
    settings->endGroup();
    updateDbGroupbox();
}

void MainWindow::updateDbGroupbox()
{
    int index=ui->db_type->currentIndex();
    ui->groupBoxDbMysql->setEnabled(index==0);
    ui->groupBoxDbSQLite->setEnabled(index==1);
}

void CatchChallenger::MainWindow::on_sendPlayerNumber_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("sendPlayerNumber",ui->sendPlayerNumber->isChecked());
}

void CatchChallenger::MainWindow::on_db_sqlite_browse_clicked()
{
    QString file=QFileDialog::getOpenFileName(this,"Select the SQLite database");
    if(file.isEmpty())
        return;
    ui->db_sqlite_file->setText(file);
}

void CatchChallenger::MainWindow::on_tolerantMode_toggled(bool checked)
{
    settings->setValue("tolerantMode",checked);
}

void CatchChallenger::MainWindow::on_db_fight_sync_currentIndexChanged(int index)
{
    settings->beginGroup("db");
    switch(index)
    {
        case 0:
            settings->setValue("db_fight_sync","FightSync_AtEachTurn");
        break;
        case 1:
        default:
            settings->setValue("db_fight_sync","FightSync_AtTheEndOfBattle");
        break;
    }
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_comboBox_city_capture_frequency_currentIndexChanged(int index)
{
    settings->beginGroup("city");
    switch(index)
    {
        default:
        case 0:
            settings->setValue("capture_frequency","week");
        break;
        case 1:
            settings->setValue("capture_frequency","month");
        break;
    }
    settings->endGroup();
    update_capture();
}

void CatchChallenger::MainWindow::on_comboBox_city_capture_day_currentIndexChanged(int index)
{
    settings->beginGroup("city");
    switch(index)
    {
        default:
        case 0:
            settings->setValue("capture_day","monday");
        break;
        case 1:
            settings->setValue("capture_day","tuesday");
        break;
        case 2:
            settings->setValue("capture_day","wednesday");
        break;
        case 3:
            settings->setValue("capture_day","thursday");
        break;
        case 4:
            settings->setValue("capture_day","friday");
        break;
        case 5:
            settings->setValue("capture_day","saturday");
        break;
        case 6:
            settings->setValue("capture_day","sunday");
        break;
    }
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_timeEdit_city_capture_time_editingFinished()
{
    settings->beginGroup("city");
    QTime time=ui->timeEdit_city_capture_time->time();
    settings->setValue("capture_time",QStringLiteral("%1:%2").arg(time.hour()).arg(time.minute()));
    settings->endGroup();
}

void CatchChallenger::MainWindow::update_capture()
{
    switch(ui->comboBox_city_capture_frequency->currentIndex())
    {
        case 0:
            ui->label_city_capture_day->setVisible(true);
            ui->label_city_capture_time->setVisible(true);
            ui->comboBox_city_capture_day->setVisible(true);
            ui->timeEdit_city_capture_time->setVisible(true);
        break;
        case 1:
            ui->label_city_capture_day->setVisible(false);
            ui->label_city_capture_time->setVisible(true);
            ui->comboBox_city_capture_day->setVisible(false);
            ui->timeEdit_city_capture_time->setVisible(true);
        break;
        default:
        case 2:
            ui->label_city_capture_day->setVisible(false);
            ui->label_city_capture_time->setVisible(false);
            ui->comboBox_city_capture_day->setVisible(false);
            ui->timeEdit_city_capture_time->setVisible(false);
        break;
    }
}

void CatchChallenger::MainWindow::on_bitcoin_enabled_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->beginGroup("bitcoin");
    settings->setValue("enabled",ui->bitcoin_enabled->isChecked());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_address_editingFinished()
{
    if(ui->bitcoin_address->text().contains(QRegularExpression(CATCHCHALLENGER_SERVER_BITCOIN_ADDRESS_REGEX)))
        ui->bitcoin_address->setStyleSheet("");
    else
        ui->bitcoin_address->setStyleSheet("background-color: rgb(255, 230, 230);");
    settings->beginGroup("bitcoin");
    settings->setValue("address",ui->bitcoin_address->text());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_fee_editingFinished()
{
    settings->beginGroup("bitcoin");
    settings->setValue("fee",ui->bitcoin_fee->value());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_workingpath_editingFinished()
{
    settings->beginGroup("bitcoin");
    settings->setValue("workingPath",ui->bitcoin_workingpath->text());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_binarypath_editingFinished()
{
    settings->beginGroup("bitcoin");
    settings->setValue("binaryPath",ui->bitcoin_binarypath->text());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_port_editingFinished()
{
    settings->beginGroup("bitcoin");
    settings->setValue("port",ui->bitcoin_port->value());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_bitcoin_workingpath_browse_clicked()
{
    QString folder=QFileDialog::getExistingDirectory(this,tr("Working path"));
    if(folder.isEmpty())
        return;
    ui->bitcoin_workingpath->setText(folder);
    on_bitcoin_workingpath_editingFinished();
}

void CatchChallenger::MainWindow::on_bitcoin_binarypath_browse_clicked()
{
    QString file=
        #ifdef Q_OS_WIN32
            QFileDialog::getOpenFileName(this,tr("Bitcoind binary path"),QString(),tr("Application (*.exe)"));
        #else
            QFileDialog::getOpenFileName(this,tr("Bitcoind binary path"));
        #endif
    if(file.isEmpty())
        return;
    ui->bitcoin_binarypath->setText(file);
    on_bitcoin_binarypath_editingFinished();
}

void CatchChallenger::MainWindow::on_compression_currentIndexChanged(int index)
{
    if(index<0)
        return;
    switch(index)
    {
        case 0:
        settings->setValue("compression","none");
        break;
        default:
        case 1:
        settings->setValue("compression","zlib");
        break;
        case 2:
        settings->setValue("compression","xz");
        break;
    }
}

void CatchChallenger::MainWindow::on_min_character_editingFinished()
{
    settings->setValue("min_character",ui->min_character->value());
    ui->max_character->setMinimum(ui->min_character->value());
}

void CatchChallenger::MainWindow::on_max_character_editingFinished()
{
    settings->setValue("max_character",ui->max_character->value());
    ui->min_character->setMaximum(ui->max_character->value());
}

void CatchChallenger::MainWindow::on_max_pseudo_size_editingFinished()
{
    settings->setValue("max_pseudo_size",ui->max_pseudo_size->value());
}

void CatchChallenger::MainWindow::on_character_delete_time_editingFinished()
{
    settings->setValue("character_delete_time",ui->character_delete_time->value()*3600);
}

void CatchChallenger::MainWindow::on_automatic_account_creation_clicked()
{
    settings->setValue("automatic_account_creation",ui->automatic_account_creation->isChecked());
}

void CatchChallenger::MainWindow::on_anonymous_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("anonymous",ui->anonymous->isChecked());
}

void CatchChallenger::MainWindow::on_server_message_textChanged()
{
    settings->setValue("server_message",ui->server_message->toPlainText());
}

void CatchChallenger::MainWindow::on_proxy_editingFinished()
{
    settings->setValue("proxy",ui->proxy->text());
}

void CatchChallenger::MainWindow::on_proxy_port_editingFinished()
{
    settings->setValue("proxy_port",ui->proxy_port->value());
}

void CatchChallenger::MainWindow::on_forceSpeed_toggled(bool checked)
{
    Q_UNUSED(checked);
    ui->speed->setEnabled(ui->forceSpeed->isChecked());
    if(!ui->forceSpeed->isChecked())
        settings->setValue("forcedSpeed",0);
    else
        settings->setValue("forcedSpeed",ui->speed->value());
}

void CatchChallenger::MainWindow::on_speed_editingFinished()
{
    ui->speed->setEnabled(ui->forceSpeed->isChecked());
    if(!ui->forceSpeed->isChecked())
        settings->setValue("forcedSpeed",0);
    else
        settings->setValue("forcedSpeed",ui->speed->value());
}

void CatchChallenger::MainWindow::on_dontSendPseudo_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("dontSendPseudo",ui->dontSendPseudo->isChecked());
}

void CatchChallenger::MainWindow::on_dontSendPlayerType_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("dontSendPlayerType",ui->dontSendPlayerType->isChecked());
}

void CatchChallenger::MainWindow::on_rates_xp_pow_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("xp_pow_normal",arg1);
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_rates_drop_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("drop_normal",arg1);
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_forceClientToSendAtBorder_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("forceClientToSendAtBorder",ui->forceClientToSendAtBorder->isChecked());
}
