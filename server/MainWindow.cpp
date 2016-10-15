#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "../general/base/DatapackGeneralLoader.h"
#include "../general/base/CommonSettingsCommon.h"

#include <QFileDialog>
#include <QInputDialog>

using namespace CatchChallenger;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings=new TinyXMLSettings((QCoreApplication::applicationDirPath()+QLatin1Literal("/server-properties.xml")).toStdString());
    NormalServer::checkSettingsFile(settings,FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/");
    ui->setupUi(this);
    updateActionButton();
    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<Player_private_and_public_informations>("Player_private_and_public_informations");
    connect(&server,&NormalServer::is_started,          this,&MainWindow::server_is_started);
    connect(&server,&NormalServer::need_be_stopped,     this,&MainWindow::server_need_be_stopped);
    connect(&server,&NormalServer::need_be_restarted,   this,&MainWindow::server_need_be_restarted);
    connect(&server,&NormalServer::new_player_is_connected,this,&MainWindow::new_player_is_connected);
    connect(&server,&NormalServer::player_is_disconnected,this,&MainWindow::player_is_disconnected);
    connect(&server,&NormalServer::new_chat_message,    this,&MainWindow::new_chat_message);
    connect(&server,&NormalServer::error,               this,&MainWindow::server_error);
    connect(&server,&NormalServer::haveQuitForCriticalDatabaseQueryFailed,               this,&MainWindow::haveQuitForCriticalDatabaseQueryFailed);
    connect(&timer_update_the_info,&QTimer::timeout,    this,&MainWindow::update_the_info);
    connect(&check_latency,&QTimer::timeout,            this,&MainWindow::start_calculate_latency);
    connect(this,&MainWindow::record_latency,           this,&MainWindow::stop_calculate_latency,Qt::QueuedConnection);
    timer_update_the_info.start(200);
    check_latency.setSingleShot(true);
    check_latency.start(1000);
    need_be_restarted=false;
    need_be_closed=false;
    settingsLoaded=false;
    ui->tabWidget->setCurrentIndex(0);
    internal_currentLatency=0;
    load_settings();
    updateDbGroupbox();
    {
        events=DatapackGeneralLoader::loadEvents(FacilityLibGeneral::getFolderFromFile(CatchChallenger::FacilityLibGeneral::applicationDirPath)+"/datapack/player/event.xml");
        unsigned int index=0;
        while(index<events.size())
        {
            ui->programmedEventType->addItem(QString::fromStdString(events.at(index).name));
            index++;
        }
    }
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

void MainWindow::generateTokenStatClient(TinyXMLSettings &settings,char * const data)
{
    FILE *fpRandomFile = fopen(RANDOMFILEDEVICE,"rb");
    if(fpRandomFile==NULL)
    {
        std::cerr << "Unable to open " << RANDOMFILEDEVICE << " to generate random token, use unsafe rand()" << std::endl;
        unsigned int index=0;
        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
        {
            data[index]=rand();
            index++;
        }
        return;
    }
    const int &returnedSize=fread(data,1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT,fpRandomFile);
    if(returnedSize!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
    {
        std::cerr << "Unable to read the " << TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT << " needed to do the token from, use unsafe rand()" << RANDOMFILEDEVICE << std::endl;
        unsigned int index=0;
        while(index<TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT)
        {
            data[index]=rand();
            index++;
        }
        return;
    }
    settings.setValue("token",binarytoHexa(reinterpret_cast<char *>(data)
                                           ,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT).c_str());
    fclose(fpRandomFile);
    settings.sync();
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
        ui->displayPort->setText(QString());
    }
    else
        ui->displayPort->setText(QString::number(server.getNormalSettings().server_port));
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

void MainWindow::new_player_is_connected(Player_private_and_public_informations player)
{
    QIcon icon;
    switch(player.public_informations.type)
    {
        case Player_type_premium:
            icon=QIcon(QLatin1Literal(":/images/chat/premium.png"));
        break;
        case Player_type_gm:
            icon=QIcon(QLatin1Literal(":/images/chat/admin.png"));
        break;
        case Player_type_dev:
            icon=QIcon(QLatin1Literal(":/images/chat/developer.png"));
        break;
        default:
        break;
    }

    ui->listPlayer->addItem(new QListWidgetItem(icon,QString::fromStdString(player.public_informations.pseudo)));
    players << player;
}

void MainWindow::player_is_disconnected(std::string pseudo)
{
    QList<QListWidgetItem *> tempList=ui->listPlayer->findItems(QString::fromStdString(pseudo),Qt::MatchExactly);
    int index=0;
    while(index<tempList.size())
    {
        delete tempList.at(index);
        index++;
    }
    index=0;
    while(index<players.size())
    {
        if(players.at(index).public_informations.pseudo==pseudo)
        {
            players.removeAt(index);
            break;
        }
        index++;
    }
}

void MainWindow::new_chat_message(std::string pseudo,Chat_type type,std::string text)
{
    int index=0;
    while(index<players.size())
    {
        if(players.at(index).public_informations.pseudo==pseudo)
        {
            QString html=ui->textBrowserChat->toHtml();
            html+=QString::fromStdString(
                        ChatParsing::new_chat_message(
                            players.at(index).public_informations.pseudo,
                            players.at(index).public_informations.type,
                            type,
                            text
                            )
                        );
            if(html.size()>1024*1024)
                html=html.mid(html.size()-1024*1024,1024*1024);
            ui->textBrowserChat->setHtml(html);
            return;
        }
        index++;
    }
    QMessageBox::information(this,"Warning","unable to locate the player");
}

void MainWindow::server_error(std::string error)
{
    QMessageBox::information(this,"Warning",QString::fromStdString(error));
}

void MainWindow::haveQuitForCriticalDatabaseQueryFailed()
{
    QMessageBox::information(this,"Warning","Unable to do critical database query to initialise the server");
}

void MainWindow::update_the_info()
{
    if(ui->listLatency->count()<=0)
        ui->listLatency->addItem(tr("%1ms").arg(internal_currentLatency));
    else
        ui->listLatency->item(0)->setText(tr("%1ms").arg(internal_currentLatency));
    if(server.isListen())
    {
        uint16_t player_current,player_max;
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
    bool ok;
    CatchChallenger::GameServerSettings formatedServerSettings=server.getSettings();
    NormalServerSettings formatedServerNormalSettings=server.getNormalSettings();

    {
        //common var
        CommonSettingsServer::commonSettingsServer.useSP                            = stringtobool(settings->value("useSP"));
        CommonSettingsServer::commonSettingsServer.autoLearn                        = stringtobool(settings->value("autoLearn")) && !CommonSettingsServer::commonSettingsServer.useSP;
        CommonSettingsServer::commonSettingsServer.forcedSpeed                      = stringtouint32(settings->value("forcedSpeed"));
        CommonSettingsServer::commonSettingsServer.dontSendPseudo					= stringtobool(settings->value("dontSendPseudo"));
        CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer  		= stringtobool(settings->value("plantOnlyVisibleByPlayer"));
        CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= stringtobool(settings->value("forceClientToSendAtMapChange"));
        CommonSettingsServer::commonSettingsServer.exportedXml                      = settings->value("exportedXml");
        formatedServerSettings.dontSendPlayerType                                   = stringtobool(settings->value("dontSendPlayerType"));
        CommonSettingsCommon::commonSettingsCommon.min_character					= stringtouint8(settings->value("min_character"));
        CommonSettingsCommon::commonSettingsCommon.max_character					= stringtouint8(settings->value("max_character"));
        CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= stringtouint8(settings->value("max_pseudo_size"));
        CommonSettingsCommon::commonSettingsCommon.character_delete_time			= stringtouint32(settings->value("character_delete_time"));
        CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = stringtouint32(settings->value("maxPlayerMonsters"));
        CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = stringtouint32(settings->value("maxWarehousePlayerMonsters"));
        CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = stringtouint32(settings->value("maxPlayerItems"));
        CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = stringtouint32(settings->value("maxWarehousePlayerItems"));
        formatedServerSettings.everyBodyIsRoot                                      = stringtobool(settings->value("everyBodyIsRoot"));
        formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                       = stringtobool(settings->value("teleportIfMapNotFoundOrOutOfMap"));
        //connection
        formatedServerSettings.automatic_account_creation   = stringtobool(settings->value("automatic_account_creation"));

        if(!settings->contains("compressionLevel"))
            settings->setValue("compressionLevel","6");
        formatedServerSettings.compressionLevel          = stringtouint8(settings->value("compressionLevel"),&ok);
        if(!ok)
        {
            std::cerr << "Compression level not a number fixed by 6" << std::endl;
            formatedServerSettings.compressionLevel=6;
        }
        formatedServerSettings.compressionLevel                                     = stringtouint32(settings->value("compressionLevel"));
        if(settings->value("compression")=="none")
            formatedServerSettings.compressionType                                = CompressionType_None;
        else if(settings->value("compression")=="xz")
            formatedServerSettings.compressionType                                = CompressionType_Xz;
        else if(settings->value("compression")=="lz4")
            formatedServerSettings.compressionType                                = CompressionType_Lz4;
        else
            formatedServerSettings.compressionType                                = CompressionType_Zlib;

        //the listen
        formatedServerNormalSettings.server_port			= stringtouint32(settings->value("server-port"));
        formatedServerNormalSettings.server_ip				= settings->value("server-ip");
        formatedServerNormalSettings.proxy					= settings->value("proxy");
        formatedServerNormalSettings.proxy_port				= stringtouint32(settings->value("proxy_port"));
        formatedServerNormalSettings.useSsl					= stringtobool(settings->value("useSsl"));
        formatedServerSettings.common_blobversion_datapack= stringtouint8(settings->value("common_blobversion_datapack"),&ok);
        if(!ok)
        {
            std::cerr << "common_blobversion_datapack is not a number" << std::endl;
            abort();
        }
        if(formatedServerSettings.common_blobversion_datapack>15)
        {
            std::cerr << "common_blobversion_datapack > 15" << std::endl;
            abort();
        }
        formatedServerSettings.server_blobversion_datapack= stringtouint8(settings->value("server_blobversion_datapack"),&ok);
        if(!ok)
        {
            std::cerr << "server_blobversion_datapack is not a number" << std::endl;
            abort();
        }
        if(formatedServerSettings.server_blobversion_datapack>15)
        {
            std::cerr << "server_blobversion_datapack > 15" << std::endl;
            abort();
        }

        if(settings->contains("mainDatapackCode"))
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings->value("mainDatapackCode","[main]");
        else
            CommonSettingsServer::commonSettingsServer.mainDatapackCode="[main]";
        if(CommonSettingsServer::commonSettingsServer.mainDatapackCode=="[main]")
        {
            const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
            if(list.empty())
            {
                std::cerr << "No main code detected into the current datapack (abort)" << std::endl;
                settings->sync();
                abort();
            }
            if(list.size()>=1)
            {
                settings->setValue("mainDatapackCode",list.at(0).name);
                CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).name;
            }
        }
        if(settings->contains("subDatapackCode"))
            CommonSettingsServer::commonSettingsServer.subDatapackCode=settings->value("subDatapackCode","");
        else
        {
            const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/sub/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
            if(!list.empty())
            {
                if(list.size()==1)
                {
                    settings->setValue("subDatapackCode",list.at(0).name);
                    CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).name;
                }
                else
                {
                    std::cerr << "No sub code detected into the current datapack" << std::endl;
                    settings->setValue("subDatapackCode","");
                    settings->sync();
                    CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
                }
            }
            else
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
        }
        formatedServerSettings.anonymous					= stringtobool(settings->value("anonymous"));
        formatedServerSettings.server_message				= settings->value("server_message");
        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase	= settings->value("httpDatapackMirror");
        CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
        formatedServerSettings.datapackCache				= stringtoint32(settings->value("datapackCache"));
        #ifdef __linux__
        settings->beginGroup("Linux");
        CommonSettingsServer::commonSettingsServer.tcpCork	= stringtobool(settings->value("tcpCork"));
        formatedServerNormalSettings.tcpNodelay= stringtobool(settings->value("tcpNodelay"));
        settings->endGroup();
        #endif

        //fight
        //CommonSettingsCommon::commonSettingsCommon.pvp			= stringtobool(settings->value("pvp"));
        formatedServerSettings.sendPlayerNumber         = stringtobool(settings->value("sendPlayerNumber"));

        //rates
        settings->beginGroup("rates");
        CommonSettingsServer::commonSettingsServer.rates_xp             = stringtodouble(settings->value("xp_normal"));
        CommonSettingsServer::commonSettingsServer.rates_gold			= stringtodouble(settings->value("gold_normal"));
        CommonSettingsServer::commonSettingsServer.rates_xp_pow			= stringtodouble(settings->value("xp_pow_normal"));
        CommonSettingsServer::commonSettingsServer.rates_drop			= stringtodouble(settings->value("drop_normal"));
        //formatedServerSettings.rates_xp_premium                         = stringtodouble(settings->value("xp_premium"));
        //formatedServerSettings.rates_gold_premium                       = stringtodouble(settings->value("gold_premium"));
        /*CommonSettingsCommon::commonSettingsCommon.rates_shiny		= stringtodouble(settings->value("shiny_normal"));
        formatedServerSettings.rates_shiny_premium                      = stringtodouble(settings->value("shiny_premium"));*/
        settings->endGroup();

        settings->beginGroup("DDOS");
        CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick         = stringtouint32(settings->value("waitBeforeConnectAfterKick"));
        formatedServerSettings.ddos.computeAverageValueTimeInterval       = stringtouint32(settings->value("computeAverageValueTimeInterval"));
        #ifdef CATCHCHALLENGER_DDOS_FILTER
        formatedServerSettings.ddos.kickLimitMove                         = stringtouint32(settings->value("kickLimitMove"));
        formatedServerSettings.ddos.kickLimitChat                         = stringtouint32(settings->value("kickLimitChat"));
        formatedServerSettings.ddos.kickLimitOther                        = stringtouint32(settings->value("kickLimitOther"));
        #endif
        formatedServerSettings.ddos.dropGlobalChatMessageGeneral          = stringtouint32(settings->value("dropGlobalChatMessageGeneral"));
        formatedServerSettings.ddos.dropGlobalChatMessageLocalClan        = stringtouint32(settings->value("dropGlobalChatMessageLocalClan"));
        formatedServerSettings.ddos.dropGlobalChatMessagePrivate          = stringtouint32(settings->value("dropGlobalChatMessagePrivate"));
        settings->endGroup();

        //chat allowed
        settings->beginGroup("chat");
        CommonSettingsServer::commonSettingsServer.chat_allow_all         = stringtobool(settings->value("allow-all"));
        CommonSettingsServer::commonSettingsServer.chat_allow_local		= stringtobool(settings->value("allow-local"));
        CommonSettingsServer::commonSettingsServer.chat_allow_private		= stringtobool(settings->value("allow-private"));
        //CommonSettingsServer::commonSettingsServer.chat_allow_aliance		= stringtobool(settings->value("allow-aliance"));
        CommonSettingsServer::commonSettingsServer.chat_allow_clan		= stringtobool(settings->value("allow-clan"));
        settings->endGroup();

        settings->beginGroup("db-login");
        if(settings->value("type")=="mysql")
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        else if(settings->value("type")=="sqlite")
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
        else if(settings->value("type")=="postgresql")
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
        else
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        switch(formatedServerSettings.database_login.tryOpenType)
        {
            default:
            case DatabaseBase::DatabaseType::PostgreSQL:
            case DatabaseBase::DatabaseType::Mysql:
                formatedServerSettings.database_login.host				= settings->value("host");
                formatedServerSettings.database_login.db				= settings->value("db");
                formatedServerSettings.database_login.login				= settings->value("login");
                formatedServerSettings.database_login.pass				= settings->value("pass");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                formatedServerSettings.database_login.file				= settings->value("file");
            break;
        }
        formatedServerSettings.database_login.tryInterval       = stringtouint32(settings->value("tryInterval"));
        formatedServerSettings.database_login.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
        settings->endGroup();

        settings->beginGroup("db-base");
        if(settings->value("type")=="mysql")
            formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::Mysql;
        else if(settings->value("type")=="sqlite")
            formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::SQLite;
        else if(settings->value("type")=="postgresql")
            formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::PostgreSQL;
        else
            formatedServerSettings.database_base.tryOpenType                   = DatabaseBase::DatabaseType::Mysql;
        switch(formatedServerSettings.database_base.tryOpenType)
        {
            default:
            case DatabaseBase::DatabaseType::PostgreSQL:
            case DatabaseBase::DatabaseType::Mysql:
                formatedServerSettings.database_base.host              = settings->value("host");
                formatedServerSettings.database_base.db                = settings->value("db");
                formatedServerSettings.database_base.login             = settings->value("login");
                formatedServerSettings.database_base.pass              = settings->value("pass");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                formatedServerSettings.database_base.file              = settings->value("file");
            break;
        }
        formatedServerSettings.database_base.tryInterval       = stringtouint32(settings->value("tryInterval"));
        formatedServerSettings.database_base.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
        settings->endGroup();

        settings->beginGroup("db-common");
        if(settings->value("type")=="mysql")
            formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        else if(settings->value("type")=="sqlite")
            formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
        else if(settings->value("type")=="postgresql")
            formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
        else
            formatedServerSettings.database_common.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        switch(formatedServerSettings.database_common.tryOpenType)
        {
            default:
            case DatabaseBase::DatabaseType::PostgreSQL:
            case DatabaseBase::DatabaseType::Mysql:
                formatedServerSettings.database_common.host				= settings->value("host");
                formatedServerSettings.database_common.db				= settings->value("db");
                formatedServerSettings.database_common.login				= settings->value("login");
                formatedServerSettings.database_common.pass				= settings->value("pass");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                formatedServerSettings.database_common.file				= settings->value("file");
            break;
        }
        formatedServerSettings.database_common.tryInterval       = stringtouint32(settings->value("tryInterval"));
        formatedServerSettings.database_common.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
        settings->endGroup();

        settings->beginGroup("db-server");
        if(settings->value("type")=="mysql")
            formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        else if(settings->value("type")=="sqlite")
            formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
        else if(settings->value("type")=="postgresql")
            formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
        else
            formatedServerSettings.database_server.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        switch(formatedServerSettings.database_server.tryOpenType)
        {
            default:
            case DatabaseBase::DatabaseType::PostgreSQL:
            case DatabaseBase::DatabaseType::Mysql:
                formatedServerSettings.database_server.host				= settings->value("host");
                formatedServerSettings.database_server.db				= settings->value("db");
                formatedServerSettings.database_server.login				= settings->value("login");
                formatedServerSettings.database_server.pass				= settings->value("pass");
            break;
            case DatabaseBase::DatabaseType::SQLite:
                formatedServerSettings.database_server.file				= settings->value("file");
            break;
        }
        formatedServerSettings.database_server.tryInterval       = stringtouint32(settings->value("tryInterval"));
        formatedServerSettings.database_server.considerDownAfterNumberOfTry = stringtouint32(settings->value("considerDownAfterNumberOfTry"));
        settings->endGroup();

        settings->beginGroup("db-login");
        if(settings->value("db_fight_sync")=="FightSync_AtEachTurn")
            formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtEachTurn;
        else if(settings->value("db_fight_sync")=="FightSync_AtTheDisconnexion")
            formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheDisconnexion;
        else
            formatedServerSettings.fightSync                       = CatchChallenger::GameServerSettings::FightSync_AtTheEndOfBattle;
        formatedServerSettings.positionTeleportSync=stringtobool(settings->value("positionTeleportSync"));
        formatedServerSettings.secondToPositionSync=stringtouint32(settings->value("secondToPositionSync"));
        settings->endGroup();

        //connection
        formatedServerSettings.max_players					= stringtouint32(settings->value("max-players"));

        //visibility algorithm
        settings->beginGroup("MapVisibilityAlgorithm");
        switch(stringtouint32(settings->value("MapVisibilityAlgorithm")))
        {
            case 0:
                formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
            break;
            case 1:
                formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_None;
            break;
            case 2:
                formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_WithBorder;
            break;
            default:
                formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
            break;
        }
        settings->endGroup();
        if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_Simple)
        {
            settings->beginGroup("MapVisibilityAlgorithm-Simple");
            formatedServerSettings.mapVisibility.simple.max				= stringtouint32(settings->value("Max"));
            formatedServerSettings.mapVisibility.simple.reshow			= stringtouint32(settings->value("Reshow"));
            formatedServerSettings.mapVisibility.simple.reemit          = stringtobool(settings->value("Reemit"));
            settings->endGroup();
        }
        else if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==MapVisibilityAlgorithmSelection_WithBorder)
        {
            settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
            formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= stringtouint32(settings->value("MaxWithBorder"));
            formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= stringtouint32(settings->value("ReshowWithBorder"));
            formatedServerSettings.mapVisibility.withBorder.max				= stringtouint32(settings->value("Max"));
            formatedServerSettings.mapVisibility.withBorder.reshow			= stringtouint32(settings->value("Reshow"));
            settings->endGroup();
        }

        {
            settings->beginGroup("programmedEvent");
                const std::vector<std::string> &tempListType=stringsplit(settings->value("types"),';');
                unsigned int indexType=0;
                while(indexType<tempListType.size())
                {
                    const std::string &type=tempListType.at(indexType);
                    settings->beginGroup(type);
                        const std::vector<std::string> &tempList=stringsplit(settings->value("group"),';');
                        unsigned int index=0;
                        while(index<tempList.size())
                        {
                            const std::string &groupName=tempList.at(index);
                            settings->beginGroup(groupName);
                            if(settings->contains("value") && settings->contains("cycle") && settings->contains("offset"))
                            {
                                GameServerSettings::ProgrammedEvent event;
                                event.value=settings->value("value");
                                bool ok;
                                event.cycle=stringtouint32(settings->value("cycle"),&ok);
                                if(!ok)
                                    event.cycle=0;
                                event.offset=stringtouint32(settings->value("offset"),&ok);
                                if(!ok)
                                    event.offset=0;
                                if(event.cycle>0)
                                    formatedServerSettings.programmedEventList[type][groupName]=event;
                            }
                            settings->endGroup();
                            index++;
                        }
                    settings->endGroup();
                    indexType++;
                }
            settings->endGroup();
        }

        settings->beginGroup("city");
        if(settings->value("capture_frequency")=="week")
            formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_week;
        else
            formatedServerSettings.city.capture.frenquency=City::Capture::Frequency_month;

        if(settings->value("capture_day")=="monday")
            formatedServerSettings.city.capture.day=City::Capture::Monday;
        else if(settings->value("capture_day")=="tuesday")
            formatedServerSettings.city.capture.day=City::Capture::Tuesday;
        else if(settings->value("capture_day")=="wednesday")
            formatedServerSettings.city.capture.day=City::Capture::Wednesday;
        else if(settings->value("capture_day")=="thursday")
            formatedServerSettings.city.capture.day=City::Capture::Thursday;
        else if(settings->value("capture_day")=="friday")
            formatedServerSettings.city.capture.day=City::Capture::Friday;
        else if(settings->value("capture_day")=="saturday")
            formatedServerSettings.city.capture.day=City::Capture::Saturday;
        else if(settings->value("capture_day")=="sunday")
            formatedServerSettings.city.capture.day=City::Capture::Sunday;
        else
            formatedServerSettings.city.capture.day=City::Capture::Monday;
        formatedServerSettings.city.capture.hour=0;
        formatedServerSettings.city.capture.minute=0;
        const std::vector<std::string> &capture_time_string_list=stringsplit(settings->value("capture_time"),':');
        if(capture_time_string_list.size()==2)
        {
            bool ok;
            formatedServerSettings.city.capture.hour=stringtouint32(capture_time_string_list.front(),&ok);
            if(!ok)
                formatedServerSettings.city.capture.hour=0;
            else
            {
                formatedServerSettings.city.capture.minute=stringtouint32(capture_time_string_list.back(),&ok);
                if(!ok)
                    formatedServerSettings.city.capture.minute=0;
            }
        }
        settings->endGroup();

        settings->beginGroup("statclient");
        {
            if(!settings->contains("token"))
                generateTokenStatClient(*settings,formatedServerSettings.private_token_statclient);
            std::string token=settings->value("token");
            if(token.size()!=TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT*2/*String Hexa, not binary*/)
                generateTokenStatClient(*settings,formatedServerSettings.private_token_statclient);
            token=settings->value("token");
            const std::vector<char> &tokenBinary=hexatoBinary(token);
            if(tokenBinary.empty())
            {
                std::cerr << "convertion to binary for pass failed for: " << token << std::endl;
                abort();
            }
            memcpy(formatedServerSettings.private_token_statclient,tokenBinary.data(),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
        }
        settings->endGroup();
    }

    // --------------------------------------------------
    ui->everyBodyIsRoot->setChecked(formatedServerSettings.everyBodyIsRoot);
    ui->teleportIfMapNotFoundOrOutOfMap->setChecked(formatedServerSettings.teleportIfMapNotFoundOrOutOfMap);
    ui->max_player->setValue(formatedServerSettings.max_players);
    ui->server_ip->setText(QString::fromStdString(formatedServerNormalSettings.server_ip));
    //ui->pvp->setChecked(settings->value(QLatin1Literal("pvp")).toBool());
    ui->sendPlayerNumber->setChecked(formatedServerSettings.sendPlayerNumber);
    ui->server_port->setValue(formatedServerNormalSettings.server_port);
    //ui->tolerantMode->setChecked(settings->value(QLatin1Literal("tolerantMode")).toBool());
    ui->forceClientToSendAtMapChange->setChecked(CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange);
    ui->useSsl->setChecked(formatedServerNormalSettings.useSsl);
    ui->autoLearn->setChecked(CommonSettingsServer::commonSettingsServer.autoLearn);
    ui->useSP->setChecked(CommonSettingsServer::commonSettingsServer.useSP);
    switch(formatedServerSettings.compressionType)
    {
        case CompressionType_None:
            ui->compression->setCurrentIndex(0);
        break;
        case CompressionType_Xz:
            ui->compression->setCurrentIndex(2);
        break;
        case CompressionType_Lz4:
            ui->compression->setCurrentIndex(3);
        break;
        default:
        case CompressionType_Zlib:
            ui->compression->setCurrentIndex(1);
        break;
    }

    {
        ui->mainDatapackCode->clear();
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(list.empty())
        {
            QMessageBox::critical(this,tr("Error"),tr("No main code detected into the current datapack"));
            settings->sync();
            abort();
        }
        unsigned int indexFound=0;
        unsigned int index=0;
        while(index<list.size())
        {
            if(list.at(index).name==CommonSettingsServer::commonSettingsServer.mainDatapackCode)
                indexFound=index;
            ui->mainDatapackCode->addItem(QString::fromStdString(list.at(index).name));
            index++;
        }
        if(!list.empty())
        {
            ui->mainDatapackCode->setCurrentIndex(indexFound);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=ui->mainDatapackCode->currentText().toStdString();
        }
    }
    {
        ui->subDatapackCode->clear();
        ui->subDatapackCode->addItem(QString());
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/sub/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(!list.empty())
        {
            unsigned int indexFound=0;
            unsigned int index=0;
            while(index<list.size())
            {
                if(list.at(index).name==CommonSettingsServer::commonSettingsServer.subDatapackCode)
                    indexFound=index+1;
                ui->subDatapackCode->addItem(QString::fromStdString(list.at(index).name));
                index++;
            }
            if(!list.empty())
            {
                ui->subDatapackCode->setCurrentIndex(indexFound);
                CommonSettingsServer::commonSettingsServer.subDatapackCode=ui->subDatapackCode->currentText().toStdString();
            }
        }
    }

    ui->compressionLevel->setValue(formatedServerSettings.compressionLevel);
    ui->maxPlayerMonsters->setValue(CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters);
    ui->maxWarehousePlayerMonsters->setValue(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters);
    ui->maxPlayerItems->setValue(CommonSettingsCommon::commonSettingsCommon.maxPlayerItems);
    ui->maxWarehousePlayerItems->setValue(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems);
    ui->min_character->setValue(CommonSettingsCommon::commonSettingsCommon.min_character);
    ui->max_character->setValue(CommonSettingsCommon::commonSettingsCommon.max_character);
    ui->max_pseudo_size->setValue(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size);
    ui->character_delete_time->setValue(CommonSettingsCommon::commonSettingsCommon.character_delete_time);
    ui->automatic_account_creation->setChecked(formatedServerSettings.automatic_account_creation);
    ui->anonymous->setChecked(formatedServerSettings.anonymous);
    ui->min_character->setMaximum(ui->max_character->value());
    ui->max_character->setMinimum(ui->min_character->value());
    ui->server_message->setPlainText(QString::fromStdString(formatedServerSettings.server_message));
    ui->proxy->setText(QString::fromStdString(formatedServerNormalSettings.proxy));
    ui->proxy_port->setValue(formatedServerNormalSettings.proxy_port);
    ui->blobversion->setValue(formatedServerSettings.common_blobversion_datapack);
    {
        const int32_t &datapackCache=formatedServerSettings.datapackCache;
        if(datapackCache<0)
        {
            ui->datapack_cache->setChecked(false);
            ui->datapack_cache_timeout_checkbox->setChecked(false);
            ui->datapack_cache_timeout->setValue(30);
            ui->datapack_cache_timeout_checkbox->setEnabled(ui->datapack_cache->isChecked());
            ui->datapack_cache_timeout->setEnabled(ui->datapack_cache->isChecked() && ui->datapack_cache_timeout_checkbox->isChecked());
        }
        else if(datapackCache==0)
        {
            ui->datapack_cache->setChecked(true);
            ui->datapack_cache_timeout_checkbox->setChecked(false);
            ui->datapack_cache_timeout->setValue(30);
            ui->datapack_cache_timeout_checkbox->setEnabled(ui->datapack_cache->isChecked());
            ui->datapack_cache_timeout->setEnabled(ui->datapack_cache->isChecked() && ui->datapack_cache_timeout_checkbox->isChecked());
        }
        else
        {
            ui->datapack_cache->setChecked(true);
            ui->datapack_cache_timeout_checkbox->setChecked(true);
            ui->datapack_cache_timeout->setValue(datapackCache);
            ui->datapack_cache_timeout_checkbox->setEnabled(ui->datapack_cache->isChecked());
            ui->datapack_cache_timeout->setEnabled(ui->datapack_cache->isChecked() && ui->datapack_cache_timeout_checkbox->isChecked());
        }
    }
    if(ui->programmedEventType->count()>0)
        on_programmedEventType_currentIndexChanged(0);
    {
        #ifdef __linux__
        ui->linux_socket_cork->setChecked(CommonSettingsServer::commonSettingsServer.tcpCork);
        ui->tcpNodelay->setChecked(formatedServerNormalSettings.tcpNodelay);
        #else
        ui->linux_socket_cork->setEnabled(false);
        ui->tcpNodelay->setEnabled(false);
        #endif
    }
    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
    {
        ui->forceSpeed->setChecked(true);
        ui->speed->setEnabled(false);
    }
    else
    {
        ui->forceSpeed->setChecked(false);
        ui->speed->setValue(CommonSettingsServer::commonSettingsServer.forcedSpeed);
        ui->speed->setEnabled(true);
    }
    ui->dontSendPseudo->setChecked(CommonSettingsServer::commonSettingsServer.dontSendPseudo);
    ui->dontSendPlayerType->setChecked(formatedServerSettings.dontSendPlayerType);

    if(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm<(uint32_t)ui->MapVisibilityAlgorithm->count())
        ui->MapVisibilityAlgorithm->setCurrentIndex(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm);
    ui->groupBoxMapVisibilityAlgorithmSimple->setEnabled(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==0);
    ui->groupBoxMapVisibilityAlgorithmWithBorder->setEnabled(formatedServerSettings.mapVisibility.mapVisibilityAlgorithm==2);

    ui->MapVisibilityAlgorithmSimpleMax->setValue(formatedServerSettings.mapVisibility.simple.max);
    ui->MapVisibilityAlgorithmSimpleReshow->setValue(formatedServerSettings.mapVisibility.simple.reshow);
    ui->MapVisibilityAlgorithmSimpleReshow->setMaximum(formatedServerSettings.mapVisibility.simple.max);
    ui->MapVisibilityAlgorithmSimpleReemit->setChecked(formatedServerSettings.mapVisibility.simple.reemit);
    {
        if(formatedServerSettings.mapVisibility.withBorder.reshow>formatedServerSettings.mapVisibility.withBorder.max)
        {
            std::cerr << "Reshow number corrected" << std::endl;
            formatedServerSettings.mapVisibility.withBorder.reshow=formatedServerSettings.mapVisibility.withBorder.max;
            //settings->setValue(QLatin1Literal("Reshow"),formatedServerSettings.mapVisibility.withBorder.reshow);
        }
        if(formatedServerSettings.mapVisibility.withBorder.reshowWithBorder>formatedServerSettings.mapVisibility.withBorder.maxWithBorder)
        {
            std::cerr << "ReshowWithBorder number corrected" << std::endl;
            formatedServerSettings.mapVisibility.withBorder.reshowWithBorder=formatedServerSettings.mapVisibility.withBorder.maxWithBorder;
            //settings->setValue(QLatin1Literal("ReshowWithBorder"),formatedServerSettings.mapVisibility.withBorder.reshow);
        }
        if(formatedServerSettings.mapVisibility.withBorder.maxWithBorder>formatedServerSettings.mapVisibility.withBorder.max)
        {
            std::cerr << "MaxWithBorder number corrected" << std::endl;
            formatedServerSettings.mapVisibility.withBorder.maxWithBorder=formatedServerSettings.mapVisibility.withBorder.max;
            //settings->setValue(QLatin1Literal("MaxWithBorder"),formatedServerSettings.mapVisibility.withBorder.reshow);
        }
        if(formatedServerSettings.mapVisibility.withBorder.reshowWithBorder>formatedServerSettings.mapVisibility.withBorder.reshow)
        {
            std::cerr << "ReshowWithBorder number corrected" << std::endl;
            formatedServerSettings.mapVisibility.withBorder.reshowWithBorder=formatedServerSettings.mapVisibility.withBorder.reshow;
            //settings->setValue(QLatin1Literal("ReshowWithBorder"),formatedServerSettings.mapVisibility.withBorder.reshow);
        }
        ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->setValue(formatedServerSettings.mapVisibility.withBorder.maxWithBorder);
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setValue(formatedServerSettings.mapVisibility.withBorder.reshowWithBorder);
        ui->MapVisibilityAlgorithmWithBorderMax->setValue(formatedServerSettings.mapVisibility.withBorder.max);
        ui->MapVisibilityAlgorithmWithBorderReshow->setValue(formatedServerSettings.mapVisibility.withBorder.reshow);
        ui->MapVisibilityAlgorithmWithBorderReshow->setMaximum(ui->MapVisibilityAlgorithmWithBorderMax->value());
        ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMax->value());
        if(ui->MapVisibilityAlgorithmWithBorderReshow->value()>ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value())
            ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value());
        else
            ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderReshow->value());
    }

    {
        ui->rates_xp_normal->setValue(CommonSettingsServer::commonSettingsServer.rates_xp);
        ui->rates_gold_normal->setValue(CommonSettingsServer::commonSettingsServer.rates_gold);
        ui->rates_xp_pow_normal->setValue(CommonSettingsServer::commonSettingsServer.rates_xp_pow);
        ui->rates_drop_normal->setValue(CommonSettingsServer::commonSettingsServer.rates_drop);
    }

    {
        ui->chat_allow_all->setChecked(CommonSettingsServer::commonSettingsServer.chat_allow_all);
        ui->chat_allow_local->setChecked(CommonSettingsServer::commonSettingsServer.chat_allow_local);
        ui->chat_allow_private->setChecked(CommonSettingsServer::commonSettingsServer.chat_allow_private);
        ui->chat_allow_clan->setChecked(CommonSettingsServer::commonSettingsServer.chat_allow_clan);
    }

    ui->DDOSwaitBeforeConnectAfterKick->setValue(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick);
    ui->DDOScomputeAverageValueTimeInterval->setValue(formatedServerSettings.ddos.computeAverageValueTimeInterval);
    ui->DDOSkickLimitMove->setValue(formatedServerSettings.ddos.kickLimitMove);
    ui->DDOSkickLimitChat->setValue(formatedServerSettings.ddos.kickLimitChat);
    ui->DDOSkickLimitOther->setValue(formatedServerSettings.ddos.kickLimitOther);
    ui->DDOSdropGlobalChatMessageGeneral->setValue(formatedServerSettings.ddos.dropGlobalChatMessageGeneral);
    ui->DDOSdropGlobalChatMessageLocalClan->setValue(formatedServerSettings.ddos.dropGlobalChatMessageLocalClan);
    ui->DDOSdropGlobalChatMessagePrivate->setValue(formatedServerSettings.ddos.dropGlobalChatMessagePrivate);

    switch(formatedServerSettings.database_login.tryOpenType)
    {
        case DatabaseBase::DatabaseType::SQLite:
            ui->db_type->setCurrentIndex(1);
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            ui->db_type->setCurrentIndex(2);
        break;
        default:
        case DatabaseBase::DatabaseType::Mysql:
            ui->db_type->setCurrentIndex(1);
        break;
    }
    ui->tryInterval->setValue(formatedServerSettings.database_login.tryInterval);
    ui->considerDownAfterNumberOfTry->setValue(formatedServerSettings.database_login.considerDownAfterNumberOfTry);

    ui->db_mysql_host->setText(QString::fromStdString(formatedServerSettings.database_login.host));
    ui->db_mysql_login->setText(QString::fromStdString(formatedServerSettings.database_login.login));
    ui->db_mysql_pass->setText(QString::fromStdString(formatedServerSettings.database_login.pass));
    ui->db_mysql_base->setText(QString::fromStdString(formatedServerSettings.database_login.db));
    if(formatedServerSettings.database_login.file.empty())
        ui->db_sqlite_file->setText("database.sqlite");
    else
        ui->db_sqlite_file->setText(QString::fromStdString(formatedServerSettings.database_login.file));

    switch(formatedServerSettings.fightSync)
    {
    case CatchChallenger::GameServerSettings::FightSync_AtEachTurn:
        ui->db_fight_sync->setCurrentIndex(0);
    break;
    case CatchChallenger::GameServerSettings::FightSync_AtTheDisconnexion:
        ui->db_fight_sync->setCurrentIndex(2);
    break;
    default:
    case CatchChallenger::GameServerSettings::FightSync_AtTheEndOfBattle:
        ui->db_fight_sync->setCurrentIndex(1);
    break;
    }

    ui->positionTeleportSync->setChecked(formatedServerSettings.positionTeleportSync);
    ui->secondToPositionSync->setValue(formatedServerSettings.secondToPositionSync);

    ui->comboBox_city_capture_frequency->setCurrentIndex(formatedServerSettings.city.capture.frenquency);
    ui->comboBox_city_capture_day->setCurrentIndex(formatedServerSettings.city.capture.day);
    ui->timeEdit_city_capture_time->setTime(QTime(formatedServerSettings.city.capture.hour,formatedServerSettings.city.capture.minute));

    send_settings();
    settingsLoaded=true;
}

void MainWindow::send_settings()
{
    GameServerSettings formatedServerSettings=server.getSettings();
    NormalServerSettings formatedServerNormalSettings=server.getNormalSettings();

    //common var
    CommonSettingsCommon::commonSettingsCommon.min_character					= ui->min_character->value();
    CommonSettingsCommon::commonSettingsCommon.max_character					= ui->max_character->value();
    CommonSettingsCommon::commonSettingsCommon.max_pseudo_size					= ui->max_pseudo_size->value();
    CommonSettingsCommon::commonSettingsCommon.character_delete_time			= ui->character_delete_time->value()*3600;

    if(!ui->forceSpeed->isChecked())
        CommonSettingsServer::commonSettingsServer.forcedSpeed					= 0;
    else
        CommonSettingsServer::commonSettingsServer.forcedSpeed					= ui->speed->value();
    formatedServerSettings.dontSendPlayerType                                   = ui->dontSendPlayerType->isChecked();
    formatedServerSettings.everyBodyIsRoot                                      = ui->everyBodyIsRoot->isChecked();
    formatedServerSettings.teleportIfMapNotFoundOrOutOfMap                      = ui->teleportIfMapNotFoundOrOutOfMap->isChecked();
    //formatedServerSettings.announce                                 = ui->announce->isChecked();
    CommonSettingsServer::commonSettingsServer.dontSendPseudo					= ui->dontSendPseudo->isChecked();
    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange		= ui->forceClientToSendAtMapChange->isChecked();
    CommonSettingsServer::commonSettingsServer.useSP                            = ui->useSP->isChecked();
    CommonSettingsServer::commonSettingsServer.autoLearn                        = ui->autoLearn->isChecked() && !ui->useSP->isChecked();
    CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters                = ui->maxPlayerMonsters->value();
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters       = ui->maxWarehousePlayerMonsters->value();
    CommonSettingsCommon::commonSettingsCommon.maxPlayerItems                   = ui->maxPlayerItems->value();
    CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems          = ui->maxWarehousePlayerItems->value();
    CommonSettingsServer::commonSettingsServer.mainDatapackCode                 = ui->mainDatapackCode->currentText().toStdString();
    CommonSettingsServer::commonSettingsServer.subDatapackCode                  = ui->subDatapackCode->currentText().toStdString();

    //the listen
    formatedServerNormalSettings.server_port			= ui->server_port->value();
    formatedServerNormalSettings.server_ip				= ui->server_ip->text().toStdString();
    formatedServerNormalSettings.proxy    				= ui->proxy->text().toStdString();
    formatedServerNormalSettings.proxy_port				= ui->proxy_port->value();
    formatedServerNormalSettings.useSsl					= ui->useSsl->isChecked();
    formatedServerSettings.anonymous					= ui->anonymous->isChecked();
    formatedServerSettings.server_message				= ui->server_message->toPlainText().toStdString();
    CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase    		= ui->httpDatapackMirror->text().toStdString();
    CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
    if(!ui->datapack_cache->isChecked())
        formatedServerSettings.datapackCache			= -1;
    else if(!ui->datapack_cache_timeout_checkbox->isChecked())
        formatedServerSettings.datapackCache			= 0;
    else
        formatedServerSettings.datapackCache			= ui->datapack_cache_timeout->value();
    #ifdef __linux__
    CommonSettingsServer::commonSettingsServer.tcpCork  = ui->linux_socket_cork->isChecked();
    formatedServerNormalSettings.tcpNodelay  = ui->tcpNodelay->isChecked();
    #endif

    //ddos
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=ui->DDOSwaitBeforeConnectAfterKick->value();
    formatedServerSettings.ddos.computeAverageValueTimeInterval=ui->DDOScomputeAverageValueTimeInterval->value();
    formatedServerSettings.ddos.kickLimitMove=ui->DDOSkickLimitMove->value();
    formatedServerSettings.ddos.kickLimitChat=ui->DDOSkickLimitChat->value();
    formatedServerSettings.ddos.kickLimitOther=ui->DDOSkickLimitOther->value();
    formatedServerSettings.ddos.dropGlobalChatMessageGeneral=ui->DDOSdropGlobalChatMessageGeneral->value();
    formatedServerSettings.ddos.dropGlobalChatMessageLocalClan=ui->DDOSdropGlobalChatMessageLocalClan->value();
    formatedServerSettings.ddos.dropGlobalChatMessagePrivate=ui->DDOSdropGlobalChatMessagePrivate->value();
    formatedServerSettings.programmedEventList=programmedEventList;

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
        case 3:
        formatedServerSettings.compressionType=CatchChallenger::CompressionType_Lz4;
        break;
    }
    formatedServerSettings.compressionLevel                     = ui->compressionLevel->value();

    //rates
    CommonSettingsServer::commonSettingsServer.rates_xp			= ui->rates_xp_normal->value();
    CommonSettingsServer::commonSettingsServer.rates_gold		= ui->rates_gold_normal->value();
    CommonSettingsServer::commonSettingsServer.rates_xp_pow     = ui->rates_xp_pow_normal->value();
    CommonSettingsServer::commonSettingsServer.rates_drop		= ui->rates_drop_normal->value();

    //chat allowed
    CommonSettingsServer::commonSettingsServer.chat_allow_all		= ui->chat_allow_all->isChecked();
    CommonSettingsServer::commonSettingsServer.chat_allow_local		= ui->chat_allow_local->isChecked();
    CommonSettingsServer::commonSettingsServer.chat_allow_private		= ui->chat_allow_private->isChecked();
    CommonSettingsServer::commonSettingsServer.chat_allow_clan		= ui->chat_allow_clan->isChecked();

    switch(ui->db_type->currentIndex())
    {
        default:
        case 0:
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::Mysql;
        break;
        case 1:
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::SQLite;
        break;
        case 2:
            formatedServerSettings.database_login.tryOpenType					= DatabaseBase::DatabaseType::PostgreSQL;
        break;
    }
    formatedServerSettings.database_base.tryOpenType					= formatedServerSettings.database_login.tryOpenType;
    formatedServerSettings.database_common.tryOpenType					= formatedServerSettings.database_login.tryOpenType;
    formatedServerSettings.database_server.tryOpenType					= formatedServerSettings.database_login.tryOpenType;
    switch(formatedServerSettings.database_login.tryOpenType)
    {
        default:
        case DatabaseBase::DatabaseType::Mysql:
            formatedServerSettings.database_login.host				= ui->db_mysql_host->text().toStdString();
            formatedServerSettings.database_login.db                  = ui->db_mysql_base->text().toStdString();
            formatedServerSettings.database_login.login				= ui->db_mysql_login->text().toStdString();
            formatedServerSettings.database_login.pass				= ui->db_mysql_pass->text().toStdString();
        break;
        case DatabaseBase::DatabaseType::SQLite:
        {
            formatedServerSettings.database_login.file				= ui->db_sqlite_file->text().toStdString();
            QFile dest(QString::fromStdString(formatedServerSettings.database_login.file));
            if(!dest.exists())
            {
                QFile source(":/catchchallenger.db.sqlite");
                if(source.open(QIODevice::ReadOnly))
                {
                    QByteArray data=source.readAll();
                    QFile destination(QString::fromStdString(formatedServerSettings.database_login.file));
                    if(destination.open(QIODevice::WriteOnly))
                    {
                        destination.write(data);
                        destination.close();
                    }
                    else
                        QMessageBox::critical(this,tr("Error"),tr("Unable to open destination file"));
                    destination.setPermissions(destination.permissions() | QFileDevice::WriteOwner | QFileDevice::WriteUser);
                    source.close();
                }
                else
                    QMessageBox::critical(this,tr("Error"),tr("Unable to open source file"));
            }
        }
        break;
        case DatabaseBase::DatabaseType::PostgreSQL:
            formatedServerSettings.database_login.host				= ui->db_mysql_host->text().toStdString();
            formatedServerSettings.database_login.db                  = ui->db_mysql_base->text().toStdString();
            formatedServerSettings.database_login.login				= ui->db_mysql_login->text().toStdString();
            formatedServerSettings.database_login.pass				= ui->db_mysql_pass->text().toStdString();
        break;
    }
    formatedServerSettings.database_base.host				= formatedServerSettings.database_login.host;
    formatedServerSettings.database_base.db                 = formatedServerSettings.database_login.db;
    formatedServerSettings.database_base.login				= formatedServerSettings.database_login.login;
    formatedServerSettings.database_base.pass				= formatedServerSettings.database_login.pass;
    formatedServerSettings.database_base.file				= formatedServerSettings.database_login.file;
    formatedServerSettings.database_common.host				= formatedServerSettings.database_login.host;
    formatedServerSettings.database_common.db               = formatedServerSettings.database_login.db;
    formatedServerSettings.database_common.login			= formatedServerSettings.database_login.login;
    formatedServerSettings.database_common.pass				= formatedServerSettings.database_login.pass;
    formatedServerSettings.database_common.file				= formatedServerSettings.database_login.file;
    formatedServerSettings.database_server.host				= formatedServerSettings.database_login.host;
    formatedServerSettings.database_server.db               = formatedServerSettings.database_login.db;
    formatedServerSettings.database_server.login			= formatedServerSettings.database_login.login;
    formatedServerSettings.database_server.pass				= formatedServerSettings.database_login.pass;
    formatedServerSettings.database_server.file				= formatedServerSettings.database_login.file;

    formatedServerSettings.fightSync                       = (CatchChallenger::GameServerSettings::FightSync)ui->db_fight_sync->currentIndex();
    formatedServerSettings.positionTeleportSync=ui->positionTeleportSync->isChecked();
    formatedServerSettings.secondToPositionSync=ui->secondToPositionSync->value();
    formatedServerSettings.database_login.tryInterval=ui->tryInterval->value();
    formatedServerSettings.database_base.tryInterval=formatedServerSettings.database_login.tryInterval;
    formatedServerSettings.database_common.tryInterval=formatedServerSettings.database_login.tryInterval;
    formatedServerSettings.database_server.tryInterval=formatedServerSettings.database_login.tryInterval;
    formatedServerSettings.database_login.considerDownAfterNumberOfTry=ui->considerDownAfterNumberOfTry->value();
    formatedServerSettings.database_base.considerDownAfterNumberOfTry=formatedServerSettings.database_login.considerDownAfterNumberOfTry;
    formatedServerSettings.database_common.considerDownAfterNumberOfTry=formatedServerSettings.database_login.considerDownAfterNumberOfTry;
    formatedServerSettings.database_server.considerDownAfterNumberOfTry=formatedServerSettings.database_login.considerDownAfterNumberOfTry;

    //connection
    formatedServerSettings.automatic_account_creation   = ui->automatic_account_creation->isChecked();
    formatedServerSettings.max_players					= ui->max_player->value();

    //visibility algorithm
    switch(ui->MapVisibilityAlgorithm->currentIndex())
    {
        case 0:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
        case 1:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_None;
        break;
        case 2:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_WithBorder;
        break;
        default:
            formatedServerSettings.mapVisibility.mapVisibilityAlgorithm		= MapVisibilityAlgorithmSelection_Simple;
        break;
    }

    formatedServerSettings.mapVisibility.simple.max                 = ui->MapVisibilityAlgorithmSimpleMax->value();
    formatedServerSettings.mapVisibility.simple.reshow              = ui->MapVisibilityAlgorithmSimpleReshow->value();
    formatedServerSettings.mapVisibility.simple.reemit              = ui->MapVisibilityAlgorithmSimpleReemit->isChecked();
    formatedServerSettings.mapVisibility.withBorder.maxWithBorder	= ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value();
    formatedServerSettings.mapVisibility.withBorder.reshowWithBorder= ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->value();
    formatedServerSettings.mapVisibility.withBorder.max				= ui->MapVisibilityAlgorithmWithBorderMax->value();
    formatedServerSettings.mapVisibility.withBorder.reshow			= ui->MapVisibilityAlgorithmWithBorderReshow->value();

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
    if(settings->contains("mainDatapackCode"))
        CommonSettingsServer::commonSettingsServer.mainDatapackCode=settings->value("mainDatapackCode","[main]");
    else
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(list.empty())
        {
            std::cerr << "No main code detected into the current datapack (abort)" << std::endl;
            settings->sync();
            abort();
        }
        if(list.size()==1)
        {
            settings->setValue("mainDatapackCode",list.at(0).name);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=list.at(0).name;
        }
    }
    if(settings->contains("subDatapackCode"))
        CommonSettingsServer::commonSettingsServer.subDatapackCode=settings->value("subDatapackCode","");
    else
    {
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/sub/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(!list.empty())
        {
            if(list.size()==1)
            {
                settings->setValue("subDatapackCode",list.at(0).name);
                CommonSettingsServer::commonSettingsServer.subDatapackCode=list.at(0).name;
            }
            else
            {
                std::cerr << "No sub code detected into the current datapack" << std::endl;
                settings->setValue("subDatapackCode","");
                settings->sync();
                CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();
    }
    QTime time=ui->timeEdit_city_capture_time->time();
    formatedServerSettings.city.capture.hour=time.hour();
    formatedServerSettings.city.capture.minute=time.minute();

    server.setSettings(formatedServerSettings);
    server.setNormalSettings(formatedServerNormalSettings);
}

void MainWindow::on_max_player_valueChanged(int arg1)
{
    settings->setValue("max-players",arg1);
}

void MainWindow::on_server_ip_editingFinished()
{
    settings->setValue("server-ip",ui->server_ip->text().toStdString());
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
    settings->beginGroup("db-login");
    settings->setValue("host",ui->db_mysql_host->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("host",ui->db_mysql_host->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("host",ui->db_mysql_host->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("host",ui->db_mysql_host->text().toStdString());
    settings->endGroup();
}

void MainWindow::on_db_mysql_login_editingFinished()
{
    settings->beginGroup("db-login");
    settings->setValue("login",ui->db_mysql_login->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("login",ui->db_mysql_login->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("login",ui->db_mysql_login->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("login",ui->db_mysql_login->text().toStdString());
    settings->endGroup();
}

void MainWindow::on_db_mysql_pass_editingFinished()
{
    settings->beginGroup("db-login");
    settings->setValue("pass",ui->db_mysql_pass->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("pass",ui->db_mysql_pass->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("pass",ui->db_mysql_pass->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("pass",ui->db_mysql_pass->text().toStdString());
    settings->endGroup();
}

void MainWindow::on_db_mysql_base_editingFinished()
{
    settings->beginGroup("db-login");
    settings->setValue("db",ui->db_mysql_base->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("db",ui->db_mysql_base->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("db",ui->db_mysql_base->text().toStdString());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("db",ui->db_mysql_base->text().toStdString());
    settings->endGroup();
}

void MainWindow::on_MapVisibilityAlgorithm_currentIndexChanged(int index)
{
    ui->groupBoxMapVisibilityAlgorithmSimple->setEnabled(index==0);
    ui->groupBoxMapVisibilityAlgorithmWithBorder->setEnabled(index==2);
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
}


void MainWindow::on_MapVisibilityAlgorithmSimpleReshow_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    settings->setValue("Reshow",ui->MapVisibilityAlgorithmSimpleReshow->value());
    settings->endGroup();
}

void MainWindow::on_db_type_currentIndexChanged(int index)
{
    QStringList list;
    list << "db-login" << "db-base" << "db-common" << "db-server";
    int indexDb=0;
    while(indexDb<list.size())
    {
        settings->beginGroup(list.at(indexDb).toStdString());
        switch(index)
        {
            case 0:
            default:
                settings->setValue("type","mysql");
                if(!settings->contains("host"))
                    settings->setValue("host","localhost");
                if(!settings->contains("login"))
                    settings->setValue("login","catchchallenger-login");
                if(!settings->contains("pass"))
                    settings->setValue("pass","catchchallenger-pass");
                if(!settings->contains("db"))
                    settings->setValue("db","catchchallenger_server");
            break;
            case 1:
                settings->setValue("type","sqlite");
                if(!settings->contains("file"))
                    settings->setValue("file","database.sqlite");
            break;
            case 2:
                settings->setValue("type","postgresql");
                if(!settings->contains("host"))
                    settings->setValue("host","localhost");
                if(!settings->contains("login"))
                    settings->setValue("login","catchchallenger-login");
                if(!settings->contains("pass"))
                    settings->setValue("pass","catchchallenger-pass");
                if(!settings->contains("db"))
                    settings->setValue("db","catchchallenger_server");
            break;
        }
        settings->endGroup();
        indexDb++;
    }
    updateDbGroupbox();
}

void MainWindow::updateDbGroupbox()
{
    int index=ui->db_type->currentIndex();
    ui->groupBoxDbMysql->setEnabled(index==0 || index==2);
    ui->groupBoxDbSQLite->setEnabled(index==1);
}

void MainWindow::on_sendPlayerNumber_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("sendPlayerNumber",ui->sendPlayerNumber->isChecked());
}

void MainWindow::on_db_sqlite_browse_clicked()
{
    std::string file=QFileDialog::getOpenFileName(this,tr("Select the SQLite database")).toStdString();
    if(file.empty())
        return;
    ui->db_sqlite_file->setText(QString::fromStdString(file));
}

void MainWindow::on_db_fight_sync_currentIndexChanged(int index)
{
    settings->beginGroup("db-login");
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

void MainWindow::on_comboBox_city_capture_frequency_currentIndexChanged(int index)
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

void MainWindow::on_comboBox_city_capture_day_currentIndexChanged(int index)
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

void MainWindow::on_timeEdit_city_capture_time_editingFinished()
{
    settings->beginGroup("city");
    QTime time=ui->timeEdit_city_capture_time->time();
    settings->setValue("capture_time",(QStringLiteral("%1:%2").arg(time.hour()).arg(time.minute())).toStdString());
    settings->endGroup();
}

void MainWindow::update_capture()
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

void MainWindow::on_compression_currentIndexChanged(int index)
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
        case 3:
        settings->setValue("compression","lz4");
        break;
    }
}

void MainWindow::on_min_character_editingFinished()
{
    settings->setValue("min_character",ui->min_character->value());
    ui->max_character->setMinimum(ui->min_character->value());
}

void MainWindow::on_max_character_editingFinished()
{
    settings->setValue("max_character",ui->max_character->value());
    ui->min_character->setMaximum(ui->max_character->value());
}

void MainWindow::on_max_pseudo_size_editingFinished()
{
    settings->setValue("max_pseudo_size",ui->max_pseudo_size->value());
}

void MainWindow::on_character_delete_time_editingFinished()
{
    settings->setValue("character_delete_time",ui->character_delete_time->value()*3600);
}

void MainWindow::on_automatic_account_creation_clicked()
{
    settings->setValue("automatic_account_creation",ui->automatic_account_creation->isChecked());
}

void MainWindow::on_anonymous_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("anonymous",ui->anonymous->isChecked());
}

void MainWindow::on_server_message_textChanged()
{
    settings->setValue("server_message",ui->server_message->toPlainText().toStdString());
}

void MainWindow::on_proxy_editingFinished()
{
    settings->setValue("proxy",ui->proxy->text().toStdString());
}

void MainWindow::on_proxy_port_editingFinished()
{
    settings->setValue("proxy_port",ui->proxy_port->value());
}

void MainWindow::on_forceSpeed_toggled(bool checked)
{
    Q_UNUSED(checked);
    ui->speed->setEnabled(ui->forceSpeed->isChecked());
    if(!ui->forceSpeed->isChecked())
        settings->setValue("forcedSpeed",0);
    else
        settings->setValue("forcedSpeed",ui->speed->value());
}

void MainWindow::on_speed_editingFinished()
{
    ui->speed->setEnabled(ui->forceSpeed->isChecked());
    if(!ui->forceSpeed->isChecked())
        settings->setValue("forcedSpeed",0);
    else
        settings->setValue("forcedSpeed",ui->speed->value());
}

void MainWindow::on_dontSendPseudo_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("dontSendPseudo",ui->dontSendPseudo->isChecked());
}

void MainWindow::on_dontSendPlayerType_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("dontSendPlayerType",ui->dontSendPlayerType->isChecked());
}

void MainWindow::on_rates_xp_pow_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("xp_pow_normal",arg1);
    settings->endGroup();
}

void MainWindow::on_rates_drop_normal_valueChanged(double arg1)
{
    settings->beginGroup("rates");
    settings->setValue("drop_normal",arg1);
    settings->endGroup();
}

void MainWindow::on_forceClientToSendAtMapChange_toggled(bool checked)
{
    Q_UNUSED(checked);
    settings->setValue("forceClientToSendAtMapChange",ui->forceClientToSendAtMapChange->isChecked());
}

void MainWindow::on_MapVisibilityAlgorithmWithBorderMax_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
    settings->setValue("Max",ui->MapVisibilityAlgorithmWithBorderMax->value());
    settings->endGroup();
    ui->MapVisibilityAlgorithmWithBorderReshow->setMaximum(ui->MapVisibilityAlgorithmWithBorderMax->value());
    ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMax->value());
    if(ui->MapVisibilityAlgorithmWithBorderReshow->value()>ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value())
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value());
    else
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderReshow->value());
}

void MainWindow::on_MapVisibilityAlgorithmWithBorderReshow_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
    settings->setValue("Reshow",ui->MapVisibilityAlgorithmSimpleReshow->value());
    settings->endGroup();
    if(ui->MapVisibilityAlgorithmWithBorderReshow->value()>ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value())
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value());
    else
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderReshow->value());
}

void MainWindow::on_MapVisibilityAlgorithmWithBorderMaxWithBorder_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
    settings->setValue("MaxWithBorder",ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value());
    settings->endGroup();
    if(ui->MapVisibilityAlgorithmWithBorderReshow->value()>ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value())
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderMaxWithBorder->value());
    else
        ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->setMaximum(ui->MapVisibilityAlgorithmWithBorderReshow->value());
}

void MainWindow::on_MapVisibilityAlgorithmWithBorderReshowWithBorder_editingFinished()
{
    settings->beginGroup("MapVisibilityAlgorithm-WithBorder");
    settings->setValue("ReshowWithBorder",ui->MapVisibilityAlgorithmWithBorderReshowWithBorder->value());
    settings->endGroup();
}

void MainWindow::on_httpDatapackMirror_editingFinished()
{
    settings->setValue("httpDatapackMirror",ui->httpDatapackMirror->text().toStdString());
}

void MainWindow::on_datapack_cache_toggled(bool checked)
{
    Q_UNUSED(checked);
    ui->datapack_cache_timeout_checkbox->setEnabled(ui->datapack_cache->isChecked());
    ui->datapack_cache_timeout->setEnabled(ui->datapack_cache->isChecked() && ui->datapack_cache_timeout_checkbox->isChecked());
    datapack_cache_save();
}

void MainWindow::on_datapack_cache_timeout_checkbox_toggled(bool checked)
{
    Q_UNUSED(checked);
    ui->datapack_cache_timeout_checkbox->setEnabled(ui->datapack_cache->isChecked());
    ui->datapack_cache_timeout->setEnabled(ui->datapack_cache->isChecked() && ui->datapack_cache_timeout_checkbox->isChecked());
    datapack_cache_save();
}

void MainWindow::on_datapack_cache_timeout_editingFinished()
{
    datapack_cache_save();
}

void MainWindow::datapack_cache_save()
{
    if(!ui->datapack_cache->isChecked())
        settings->setValue("datapackCache",-1);
    else if(!ui->datapack_cache_timeout_checkbox->isChecked())
        settings->setValue("datapackCache",0);
    else
        settings->setValue("datapackCache",ui->datapack_cache_timeout->value());
}

void MainWindow::on_linux_socket_cork_toggled(bool checked)
{
    #ifdef __linux__
    settings->beginGroup("Linux");
    settings->setValue("tcpCork",checked);
    settings->endGroup();
    #endif
}

void CatchChallenger::MainWindow::on_MapVisibilityAlgorithmSimpleReemit_toggled(bool checked)
{
    settings->beginGroup("MapVisibilityAlgorithm-Simple");
    settings->setValue("Reemit",checked);
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_useSsl_toggled(bool checked)
{
    settings->setValue("useSsl",checked);
}

void CatchChallenger::MainWindow::on_useSP_toggled(bool checked)
{
    settings->setValue("useSP",checked);
}

void CatchChallenger::MainWindow::on_autoLearn_toggled(bool checked)
{
    settings->setValue("autoLearn",checked);
}

void CatchChallenger::MainWindow::on_programmedEventType_currentIndexChanged(int index)
{
    Q_UNUSED(index);
    ui->programmedEventList->clear();
    const std::string &selectedEvent=ui->programmedEventType->currentText().toStdString();
    if(selectedEvent.empty())
        return;
    if(programmedEventList.find(selectedEvent)!=programmedEventList.cend())
    {
        const std::unordered_map<std::string,GameServerSettings::ProgrammedEvent> &list=programmedEventList.at(selectedEvent);
        auto i=list.begin();
        while(i!=list.cend())
        {
            QListWidgetItem *listWidgetItem=new QListWidgetItem(
                        tr("%1\nCycle: %2mins, offset: %3mins\nValue: %4")
                        .arg(QString::fromStdString(i->first))
                        .arg(i->second.cycle)
                        .arg(i->second.offset)
                        .arg(QString::fromStdString(i->second.value))
                        );
            listWidgetItem->setData(99,QString::fromStdString(i->first));
            ui->programmedEventList->addItem(listWidgetItem);
            ++i;
        }
    }
}

void CatchChallenger::MainWindow::on_programmedEventList_itemActivated(QListWidgetItem *item)
{
    Q_UNUSED(item);
    on_programmedEventEdit_clicked();
}

void CatchChallenger::MainWindow::on_programmedEventAdd_clicked()
{
    const std::string &selectedEvent=ui->programmedEventType->currentText().toStdString();
    if(selectedEvent.empty())
        return;
    GameServerSettings::ProgrammedEvent programmedEvent;
    bool ok;
    const std::string &name=QInputDialog::getText(this,tr("Name"),tr("Name:"),QLineEdit::Normal,QString(),&ok).toStdString();
    if(!ok)
        return;
    if(programmedEventList[selectedEvent].find(name)!=programmedEventList[selectedEvent].cend())
    {
        QMessageBox::warning(this,tr("Error"),tr("Entry already name"));
        return;
    }
    programmedEvent.cycle=QInputDialog::getInt(this,tr("Name"),tr("Name:"),60,1,60*24,1,&ok);
    if(!ok)
        return;
    programmedEvent.offset=QInputDialog::getInt(this,tr("Name"),tr("Name:"),0,1,60*24,1,&ok);
    if(!ok)
        return;
    programmedEvent.value=QInputDialog::getText(this,tr("Value"),tr("Value:"),QLineEdit::Normal,QString(),&ok).toStdString();
    if(!ok)
        return;
    programmedEventList[selectedEvent][name]=programmedEvent;
    settings->beginGroup("programmedEvent");
        settings->beginGroup(selectedEvent);
            settings->beginGroup(name);
                settings->setValue("value",programmedEvent.value);
                settings->setValue("cycle",programmedEvent.cycle);
                settings->setValue("offset",programmedEvent.offset);
            settings->endGroup();
        settings->endGroup();
    settings->endGroup();
    on_programmedEventType_currentIndexChanged(0);
}

void CatchChallenger::MainWindow::on_programmedEventEdit_clicked()
{
    const QList<QListWidgetItem*> &selectedItems=ui->programmedEventList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const std::string &selectedEvent=ui->programmedEventType->currentText().toStdString();
    if(selectedEvent.empty())
        return;
    GameServerSettings::ProgrammedEvent programmedEvent;
    bool ok;
    const std::string &oldName=selectedItems.first()->data(99).toString().toStdString();
    const std::string &name=QInputDialog::getText(this,tr("Name"),tr("Name:"),QLineEdit::Normal,QString::fromStdString(oldName),&ok).toStdString();
    if(!ok)
        return;
    if(programmedEventList[selectedEvent].find(name)!=programmedEventList[selectedEvent].cend())
    {
        QMessageBox::warning(this,tr("Error"),tr("Entry already name"));
        return;
    }
    programmedEvent.cycle=QInputDialog::getInt(this,tr("Name"),tr("Name:"),60,1,60*24,1,&ok);
    if(!ok)
        return;
    programmedEvent.offset=QInputDialog::getInt(this,tr("Name"),tr("Name:"),0,1,60*24,1,&ok);
    if(!ok)
        return;
    programmedEvent.value=QInputDialog::getText(this,tr("Value"),tr("Value:"),QLineEdit::Normal,QString(),&ok).toStdString();
    if(!ok)
        return;
    if(oldName!=name)
        programmedEventList[selectedEvent].erase(oldName);
    programmedEventList[selectedEvent][name]=programmedEvent;
    settings->beginGroup("programmedEvent");
        settings->beginGroup(selectedEvent);
            settings->beginGroup(oldName);
                //settings->remove("");
            settings->endGroup();
            settings->beginGroup(name);
                settings->setValue("value",programmedEvent.value);
                settings->setValue("cycle",programmedEvent.cycle);
                settings->setValue("offset",programmedEvent.offset);
            settings->endGroup();
        settings->endGroup();
    settings->endGroup();
    on_programmedEventType_currentIndexChanged(0);
}

void CatchChallenger::MainWindow::on_programmedEventRemove_clicked()
{
    const QList<QListWidgetItem*> &selectedItems=ui->programmedEventList->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const std::string &selectedEvent=ui->programmedEventType->currentText().toStdString();
    if(selectedEvent.empty())
        return;
    const std::string &name=selectedItems.first()->data(99).toString().toStdString();
    programmedEventList[selectedEvent].erase(name);
    settings->beginGroup("programmedEvent");
        settings->beginGroup(selectedEvent);
            settings->beginGroup(name);
                //settings->remove("");
            settings->endGroup();
        settings->endGroup();
    settings->endGroup();
    on_programmedEventType_currentIndexChanged(0);
}

void CatchChallenger::MainWindow::on_tcpNodelay_toggled(bool checked)
{
    #ifdef __linux__
    settings->beginGroup("Linux");
    settings->setValue("tcpNodelay",checked);
    settings->endGroup();
    #endif
}

void CatchChallenger::MainWindow::on_maxPlayerMonsters_editingFinished()
{
    settings->setValue("maxPlayerMonsters",ui->maxPlayerMonsters->value());
}

void CatchChallenger::MainWindow::on_maxWarehousePlayerMonsters_editingFinished()
{
    settings->setValue("maxWarehousePlayerMonsters",ui->maxWarehousePlayerMonsters->value());
}

void CatchChallenger::MainWindow::on_maxPlayerItems_editingFinished()
{
    settings->setValue("maxPlayerItems",ui->maxPlayerItems->value());
}

void CatchChallenger::MainWindow::on_maxWarehousePlayerItems_editingFinished()
{
    settings->setValue("maxWarehousePlayerItems",ui->maxWarehousePlayerItems->value());
}

void CatchChallenger::MainWindow::on_tryInterval_editingFinished()
{
    settings->beginGroup("db-login");
    settings->setValue("tryInterval",ui->tryInterval->value());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("tryInterval",ui->tryInterval->value());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("tryInterval",ui->tryInterval->value());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("tryInterval",ui->tryInterval->value());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_considerDownAfterNumberOfTry_editingFinished()
{
    settings->beginGroup("db-login");
    settings->setValue("considerDownAfterNumberOfTry",ui->considerDownAfterNumberOfTry->value());
    settings->endGroup();
    settings->beginGroup("db-base");
    settings->setValue("considerDownAfterNumberOfTry",ui->considerDownAfterNumberOfTry->value());
    settings->endGroup();
    settings->beginGroup("db-common");
    settings->setValue("considerDownAfterNumberOfTry",ui->considerDownAfterNumberOfTry->value());
    settings->endGroup();
    settings->beginGroup("db-server");
    settings->setValue("considerDownAfterNumberOfTry",ui->considerDownAfterNumberOfTry->value());
    settings->endGroup();
}

void CatchChallenger::MainWindow::on_announce_toggled(bool checked)
{
    (void)checked;
    //settings->setValue("announce",ui->announce->value());
}

void CatchChallenger::MainWindow::on_compressionLevel_valueChanged(int value)
{
    settings->setValue("compressionLevel",value);
}

void CatchChallenger::MainWindow::on_everyBodyIsRoot_toggled(bool checked)
{
    (void)checked;
    settings->setValue("everyBodyIsRoot",ui->everyBodyIsRoot->isChecked());
}

void CatchChallenger::MainWindow::on_teleportIfMapNotFoundOrOutOfMap_toggled(bool checked)
{
    (void)checked;
    settings->setValue("everyBodyIsRoot",ui->everyBodyIsRoot->isChecked());
}

void CatchChallenger::MainWindow::on_mainDatapackCode_currentIndexChanged(const QString &arg1)
{
    if(!settingsLoaded)
        return;
    if(arg1.isEmpty())
        return;
    CommonSettingsServer::commonSettingsServer.mainDatapackCode=arg1.toStdString();
    settings->setValue("mainDatapackCode",CommonSettingsServer::commonSettingsServer.mainDatapackCode);
    {
        ui->subDatapackCode->clear();
        ui->subDatapackCode->addItem(QString());
        const std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> &list=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(GlobalServerData::serverSettings.datapack_basePath+"/map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/sub/",CatchChallenger::FacilityLibGeneral::ListFolder::Dirs);
        if(!list.empty())
        {
            unsigned int indexFound=0;
            unsigned int index=0;
            while(index<list.size())
            {
                if(list.at(index).name==CommonSettingsServer::commonSettingsServer.subDatapackCode)
                    indexFound=index+1;
                ui->subDatapackCode->addItem(QString::fromStdString(list.at(index).name));
                index++;
            }
            if(!list.empty())
                ui->subDatapackCode->setCurrentIndex(indexFound);
        }
    }
}

void CatchChallenger::MainWindow::on_subDatapackCode_currentIndexChanged(const QString &arg1)
{
    if(!settingsLoaded)
        return;
    if(ui->mainDatapackCode->count()==0)
        return;
    CommonSettingsServer::commonSettingsServer.subDatapackCode=arg1.toStdString();
    settings->setValue("subDatapackCode",CommonSettingsServer::commonSettingsServer.subDatapackCode);
}
