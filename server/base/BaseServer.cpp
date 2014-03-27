#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DatapackGeneralLoader.h"

#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTime>

using namespace CatchChallenger;

QRegularExpression BaseServer::regexXmlFile=QRegularExpression(QLatin1String("^[a-zA-Z0-9\\- _]+\\.xml$"));
QString BaseServer::text_dotxml=QLatin1String(".xml");
QString BaseServer::text_zone=QLatin1String("zone");
QString BaseServer::text_capture=QLatin1String("capture");
QString BaseServer::text_fightId=QLatin1String("fightId");
QString BaseServer::text_dotcomma=QLatin1String(";");
QString BaseServer::text_male=QLatin1String("male");
QString BaseServer::text_female=QLatin1String("female");
QString BaseServer::text_unknown=QLatin1String("unknown");
QString BaseServer::text_slash=QLatin1String("slash");
QString BaseServer::text_antislash=QLatin1String("antislash");
QString BaseServer::text_type=QLatin1String("type");
QString BaseServer::text_shop=QLatin1String("shop");
QString BaseServer::text_learn=QLatin1String("learn");
QString BaseServer::text_heal=QLatin1String("heal");
QString BaseServer::text_market=QLatin1String("market");
QString BaseServer::text_zonecapture=QLatin1String("zonecapture");
QString BaseServer::text_fight=QLatin1String("fight");
QString BaseServer::text_fightid=QLatin1String("fightid");
QString BaseServer::text_lookAt=QLatin1String("lookAt");
QString BaseServer::text_left=QLatin1String("left");
QString BaseServer::text_right=QLatin1String("right");
QString BaseServer::text_top=QLatin1String("top");
QString BaseServer::text_bottom=QLatin1String("bottom");
QString BaseServer::text_bots=QLatin1String("bots");
QString BaseServer::text_bot=QLatin1String("bot");
QString BaseServer::text_id=QLatin1String("id");
QString BaseServer::text_name=QLatin1String("name");
QString BaseServer::text_step=QLatin1String("step");
QString BaseServer::text_arrow=QLatin1String("->");

BaseServer::BaseServer()
{
    dataLoaded=false;
    ProtocolParsing::initialiseTheVariable();

    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<Player_internal_informations>("Player_internal_informations");
    qRegisterMetaType<QList<quint64> >("QList<quint64>");
    qRegisterMetaType<Orientation>("Orientation");
    qRegisterMetaType<QList<QByteArray> >("QList<QByteArray>");
    qRegisterMetaType<Chat_type>("Chat_type");
    qRegisterMetaType<QAbstractSocket::SocketError>("QAbstractSocket::SocketError");
    qRegisterMetaType<Map_server_MapVisibility_simple*>("Map_server_MapVisibility_simple*");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<QSqlQuery>("QSqlQuery");
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<Player_private_and_public_informations>("Player_private_and_public_informations");
    qRegisterMetaType<Direction>("Direction");
    qRegisterMetaType<QuestAction>("QuestAction");
    qRegisterMetaType<QList<QPair<quint32,qint32> > >("QList<QPair<quint32,qint32> >");
    qRegisterMetaType<QList<qint32> >("QList<quint32>");

    ProtocolParsing::compressionType                                = ProtocolParsing::CompressionType_Zlib;

    GlobalServerData::serverPrivateVariables.connected_players      = 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged  = 0;
    GlobalServerData::serverPrivateVariables.db                     = NULL;
    GlobalServerData::serverPrivateVariables.timer_player_map       = NULL;
    GlobalServerData::serverPrivateVariables.timer_city_capture     = NULL;
    GlobalServerData::serverPrivateVariables.bitcoin.enabled        = false;

    GlobalServerData::serverPrivateVariables.botSpawnIndex          = 0;
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= QRegularExpression(DATAPACK_FILE_REGEX);

    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove.start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);

    GlobalServerData::serverSettings.automatic_account_creation             = false;
    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.tolerantMode                           = false;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;
    GlobalServerData::serverSettings.pvp                                    = true;

    GlobalServerData::serverSettings.database.type                              = CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    GlobalServerData::serverSettings.database.sqlite.file                       = QLatin1String("");
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = CatchChallenger::MapVisibilityAlgorithmSelection_None;

    GlobalServerData::serverSettings.datapack_basePath                          = QCoreApplication::applicationDirPath()+QLatin1String("/datapack/");
    GlobalServerData::serverSettings.server_ip                                  = QLatin1String("");
    GlobalServerData::serverSettings.server_port                                = 42489;
    GlobalServerData::serverSettings.compressionType                            = CompressionType_Zlib;
    GlobalServerData::serverSettings.anonymous                                  = false;
    GlobalServerData::serverSettings.dontSendPlayerType                         = false;
    CommonSettings::commonSettings.forceClientToSendAtMapChange = true;
    CommonSettings::commonSettings.forcedSpeed            = CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    CommonSettings::commonSettings.dontSendPseudo         = false;
    CommonSettings::commonSettings.chat_allow_clan        = true;
    CommonSettings::commonSettings.chat_allow_local       = true;
    CommonSettings::commonSettings.chat_allow_all         = true;
    CommonSettings::commonSettings.chat_allow_private     = true;
    CommonSettings::commonSettings.max_character          = 16;
    CommonSettings::commonSettings.min_character          = 0;
    CommonSettings::commonSettings.max_pseudo_size        = 20;
    CommonSettings::commonSettings.rates_gold             = 1.0;
    CommonSettings::commonSettings.rates_drop             = 1.0;
    CommonSettings::commonSettings.rates_xp               = 1.0;
    CommonSettings::commonSettings.rates_xp_pow           = 1.0;
    CommonSettings::commonSettings.factoryPriceChange     = 20;
    CommonSettings::commonSettings.character_delete_time  = 604800; // 7 day
    GlobalServerData::serverSettings.database.type                              = ServerSettings::Database::DatabaseType_Mysql;
    GlobalServerData::serverSettings.database.fightSync                         = ServerSettings::Database::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.database.positionTeleportSync              = true;
    GlobalServerData::serverSettings.database.secondToPositionSync              = 0;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = MapVisibilityAlgorithmSelection_Simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max                   = 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow                = 20;
    GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder     = 20;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder  = 10;
    GlobalServerData::serverSettings.mapVisibility.withBorder.max               = 30;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshow            = 20;
    GlobalServerData::serverSettings.city.capture.frenquency                    = City::Capture::Frequency_week;
    GlobalServerData::serverSettings.city.capture.day                           = City::Capture::Monday;
    GlobalServerData::serverSettings.city.capture.hour                          = 0;
    GlobalServerData::serverSettings.city.capture.minute                        = 0;
    GlobalServerData::serverSettings.bitcoin.address                            = QLatin1String("1Hz3GtkiDBpbWxZixkQPuTGDh2DUy9bQUJ");
    #ifdef Q_OS_WIN32
    GlobalServerData::serverSettings.bitcoin.binaryPath                         = QLatin1String("%application_path%/bitcoin/bitcoind.exe");
    GlobalServerData::serverSettings.bitcoin.workingPath                        = QLatin1String("%application_path%/bitcoin-storage/");
    #else
    GlobalServerData::serverSettings.bitcoin.binaryPath                         = QLatin1String("/usr/bin/bitcoind");
    GlobalServerData::serverSettings.bitcoin.workingPath                        = QDir::homePath()+QLatin1String("/.config/CatchChallenger/server/bitcoin/");
    #endif
    GlobalServerData::serverSettings.bitcoin.enabled                            = false;
    GlobalServerData::serverSettings.bitcoin.fee                                = 1.0;
    GlobalServerData::serverSettings.bitcoin.port                               = 46349;

    stat=Down;

    connect(&QFakeServer::server,&QFakeServer::newConnection,this,&BaseServer::newConnection,       Qt::QueuedConnection);
    connect(this,&BaseServer::need_be_started,              this,&BaseServer::start_internal_server,Qt::QueuedConnection);
    connect(this,&BaseServer::try_stop_server,              this,&BaseServer::stop_internal_server, Qt::QueuedConnection);
    connect(this,&BaseServer::try_initAll,                  this,&BaseServer::initAll,              Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.bitcoin.process,&QProcess::stateChanged,                                                  this,&BaseServer::bitcoinProcessStateChanged,Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.bitcoin.process,static_cast<void(QProcess::*)(QProcess::ProcessError)>(&QProcess::error), this,&BaseServer::bitcoinProcessError,Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.bitcoin.process,&QProcess::readyReadStandardOutput,                                       this,&BaseServer::bitcoinProcessReadyReadStandardOutput,Qt::QueuedConnection);
    connect(&GlobalServerData::serverPrivateVariables.bitcoin.process,&QProcess::readyReadStandardError,                                        this,&BaseServer::bitcoinProcessReadyReadStandardError,Qt::QueuedConnection);
    emit try_initAll();

    srand(time(NULL));
}

void BaseServer::start()
{
    emit need_be_started();
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
BaseServer::~BaseServer()
{
    GlobalServerData::serverPrivateVariables.stopIt=true;
    GlobalServerData::serverPrivateVariables.eventThreaderList.clear();
    closeDB();
}

void BaseServer::closeDB()
{
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        GlobalServerData::serverPrivateVariables.db->commit();
        GlobalServerData::serverPrivateVariables.db->close();
        QString connectionName=GlobalServerData::serverPrivateVariables.db->connectionName();
        delete GlobalServerData::serverPrivateVariables.db;
        GlobalServerData::serverPrivateVariables.db=NULL;
        QSqlDatabase::removeDatabase(connectionName);
    }
}

void BaseServer::initAll()
{
    /// \bug QObject::moveToThread: Current thread (0x79e1f0) is not the object's thread (0x6b4690)
    GlobalServerData::serverPrivateVariables.player_updater.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
    BroadCastWithoutSender::broadCastWithoutSender.moveToThread(GlobalServerData::serverPrivateVariables.eventThreaderList.at(0));
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void BaseServer::preload_the_data()
{
    if(dataLoaded)
        return;
    dataLoaded=true;
    GlobalServerData::serverPrivateVariables.stopIt=false;

    QTime time;
    time.restart();
    CommonDatapack::commonDatapack.parseDatapack(GlobalServerData::serverSettings.datapack_basePath);
    preload_the_datapack();
    preload_the_skin();
    preload_shop();
    preload_the_players();
    preload_monsters_drops();
    load_monsters_max_id();
    preload_the_map();
    preload_the_plant_on_map();
    check_monsters_map();
    preload_the_visibility_algorithm();
    load_clan_max_id();
    preload_the_city_capture();
    preload_zone();
    preload_industries();
    preload_market_monsters();
    preload_market_items();
    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    if(CommonSettings::commonSettings.max_character)
        load_character_max_id();
    qDebug() << QStringLiteral("Loaded the datapack into %1ms").arg(time.elapsed());
}

void BaseServer::preload_zone()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot);
    int index=0;
    const int &listsize=entryList.size();
    while(index<listsize)
    {
        if(!entryList.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryList.at(index).fileName().contains(regexXmlFile))
        {
            qDebug() << QStringLiteral("%1 the zone file name not match").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryList.at(index).fileName();
        zoneCodeName.remove(BaseServer::text_dotxml);
        QDomDocument domDocument;
        const QString &file=entryList.at(index).absoluteFilePath();
        if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
            domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
        else
        {
            QFile itemsFile(file);
            QByteArray xmlContent;
            if(!itemsFile.open(QIODevice::ReadOnly))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, error: %2").arg(file).arg(itemsFile.errorString());
                index++;
                continue;
            }
            xmlContent=itemsFile.readAll();
            itemsFile.close();
            QString errorStr;
            int errorLine,errorColumn;
            if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
            {
                qDebug() << QStringLiteral("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(file).arg(errorLine).arg(errorColumn).arg(errorStr);
                index++;
                continue;
            }
            CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
        }
        if(GlobalServerData::serverPrivateVariables.captureFightIdList.contains(zoneCodeName))
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, zone code name already found");
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!=BaseServer::text_zone)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load capture
        QList<quint32> fightIdList;
        QDomElement capture = root.firstChildElement(BaseServer::text_capture);
        if(!capture.isNull())
        {
            if(capture.isElement() && capture.hasAttribute(BaseServer::text_fightId))
            {
                bool ok;
                const QStringList &fightIdStringList=capture.attribute(BaseServer::text_fightId).split(BaseServer::text_dotcomma);
                int sub_index=0;
                const int &listsize=fightIdStringList.size();
                while(sub_index<listsize)
                {
                    const quint32 &fightId=fightIdStringList.at(sub_index).toUInt(&ok);
                    if(ok)
                    {
                        if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightId))
                            qDebug() << QStringLiteral("bot fightId %1 not found for capture zone %2").arg(fightId).arg(zoneCodeName);
                        else
                            fightIdList << fightId;
                    }
                    sub_index++;
                }
                if(sub_index==listsize && !fightIdList.isEmpty())
                    GlobalServerData::serverPrivateVariables.captureFightIdList[zoneCodeName]=fightIdList;
                break;
            }
            else
                qDebug() << QStringLiteral("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(file).arg(capture.tagName()).arg(capture.lineNumber());
        }
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QStringLiteral("SELECT `clan` FROM `city` WHERE `city`='%1';").arg(zoneCodeName);
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QStringLiteral("SELECT clan FROM city WHERE city='%1';").arg(zoneCodeName);
            break;
        }
        QSqlQuery cityStatusQuery(*GlobalServerData::serverPrivateVariables.db);
        if(!cityStatusQuery.exec(queryText))
            DebugClass::debugConsole(cityStatusQuery.lastQuery()+": "+cityStatusQuery.lastError().text());
        if(cityStatusQuery.next())
        {
            bool ok;
            quint32 clanId=cityStatusQuery.value(0).toUInt(&ok);
            if(ok)
            {
                GlobalServerData::serverPrivateVariables.cityStatusList[zoneCodeName].clan=clanId;
                GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneCodeName;
            }
            else
                DebugClass::debugConsole(QStringLiteral("clan id is failed to convert to number for city status"));
        }
        index++;
    }

    qDebug() << QStringLiteral("%1 zone(s) loaded").arg(GlobalServerData::serverPrivateVariables.captureFightIdList.size());
}

void BaseServer::preload_industries()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `id`,`resources`,`products`,`last_update` FROM `factory`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT id,resources,products,last_update FROM factory");
        break;
    }
    QSqlQuery industryStatusQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!industryStatusQuery.exec(queryText))
        DebugClass::debugConsole(industryStatusQuery.lastQuery()+": "+industryStatusQuery.lastError().text());
    while(industryStatusQuery.next())
    {
        IndustryStatus industryStatus;
        bool ok;
        quint32 id=industryStatusQuery.value(0).toUInt(&ok);
        if(!ok)
            DebugClass::debugConsole(QStringLiteral("preload_industries: id is not a number"));
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.industriesLink.contains(id))
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("preload_industries: industries link not found"));
            }
        }
        if(ok)
        {
            QStringList resourcesStringList=industryStatusQuery.value(1).toString().split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=resourcesStringList.size();
            while(index<listsize)
            {
                QStringList itemStringList=resourcesStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                quint32 item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item is not a number"));
                    break;
                }
                quint32 quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.resources.contains(item))
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item already set"));
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(id).industry);
                int indexItem=0;
                const int &resourceslistsize=industry.resources.size();
                while(indexItem<resourceslistsize)
                {
                    if(industry.resources.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==resourceslistsize)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item in db not found"));
                    ok=false;
                    break;
                }
                if(quantity>(industry.resources.at(indexItem).quantity*industry.cycletobefull))
                    quantity=industry.resources.at(indexItem).quantity*industry.cycletobefull;
                industryStatus.resources[item]=quantity;
                index++;
            }
        }
        if(ok)
        {
            QStringList productsStringList=industryStatusQuery.value(2).toString().split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=productsStringList.size();
            while(index<listsize)
            {
                QStringList itemStringList=productsStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                quint32 item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item is not a number"));
                    break;
                }
                quint32 quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.products.contains(item))
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item already set"));
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries.value(CommonDatapack::commonDatapack.industriesLink.value(id).industry);
                int indexItem=0;
                const int &productlistsize=industry.products.size();
                while(indexItem<productlistsize)
                {
                    if(industry.products.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==productlistsize)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: item in db not found"));
                    ok=false;
                    break;
                }
                if(quantity>(industry.products.at(indexItem).quantity*industry.cycletobefull))
                    quantity=industry.products.at(indexItem).quantity*industry.cycletobefull;
                industryStatus.products[item]=quantity;
                index++;
            }
        }
        if(ok)
        {
            industryStatus.last_update=industryStatusQuery.value(3).toUInt(&ok);
            if(!ok)
                DebugClass::debugConsole(QStringLiteral("preload_industries: last_update is not a number"));
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
        else
        {
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("DELETE FROM `industries` WHERE `id`='%1'").arg(industryStatusQuery.value(0).toString());
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("DELETE FROM industries WHERE id='%1'").arg(industryStatusQuery.value(0).toString());
                break;
            }
            QSqlQuery industryDeleteQuery(*GlobalServerData::serverPrivateVariables.db);
            if(!industryDeleteQuery.exec(queryText))
                DebugClass::debugConsole(industryDeleteQuery.lastQuery()+": "+industryDeleteQuery.lastError().text());
        }
    }
    qDebug() << QStringLiteral("%1 industrie(s) status loaded").arg(GlobalServerData::serverPrivateVariables.industriesStatus.size());
}

void BaseServer::preload_market_monsters()
{    QString queryText;
     switch(GlobalServerData::serverSettings.database.type)
     {
         default:
         case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character`,`market_price`,`market_bitcoin` FROM `monster` WHERE `place`='market' ORDER BY `position` ASC");
         break;
         case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character,market_price,market_bitcoin FROM monster WHERE place='market' ORDER BY position ASC");
         break;
     }
     bool ok;
     QSqlQuery monstersQuery(*GlobalServerData::serverPrivateVariables.db);
     if(!monstersQuery.exec(queryText))
         DebugClass::debugConsole(monstersQuery.lastQuery()+": "+monstersQuery.lastError().text());
     while(monstersQuery.next())
     {
         MarketPlayerMonster marketPlayerMonster;
         PlayerMonster playerMonster;
         playerMonster.id=monstersQuery.value(0).toUInt(&ok);
         if(!ok)
             DebugClass::debugConsole(QStringLiteral("monsterId: %1 is not a number").arg(monstersQuery.value(0).toString()));
         if(ok)
         {
             playerMonster.monster=monstersQuery.value(2).toUInt(&ok);
             if(ok)
             {
                 if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                 {
                     ok=false;
                     DebugClass::debugConsole(QStringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                 }
             }
             else
                 DebugClass::debugConsole(QStringLiteral("monster: %1 is not a number").arg(monstersQuery.value(2).toString()));
         }
         if(ok)
         {
             playerMonster.level=monstersQuery.value(3).toUInt(&ok);
             if(ok)
             {
                 if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                 {
                     DebugClass::debugConsole(QStringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                     playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                 }
             }
             else
                 DebugClass::debugConsole(QStringLiteral("level: %1 is not a number").arg(monstersQuery.value(3).toString()));
         }
         if(ok)
         {
             playerMonster.remaining_xp=monstersQuery.value(4).toUInt(&ok);
             if(ok)
             {
                 if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1))
                 {
                     DebugClass::debugConsole(QStringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1)));
                     playerMonster.remaining_xp=0;
                 }
             }
             else
                 DebugClass::debugConsole(QStringLiteral("monster xp: %1 is not a number").arg(monstersQuery.value(4).toString()));
         }
         if(ok)
         {
             playerMonster.sp=monstersQuery.value(5).toUInt(&ok);
             if(!ok)
                 DebugClass::debugConsole(QStringLiteral("monster sp: %1 is not a number").arg(monstersQuery.value(5).toString()));
         }
         if(ok)
         {
             playerMonster.catched_with=monstersQuery.value(6).toUInt(&ok);
             if(ok)
             {
                 if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                     DebugClass::debugConsole(QStringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
             }
             else
                 DebugClass::debugConsole(QStringLiteral("captured_with: %1 is not a number").arg(monstersQuery.value(6).toString()));
         }
         if(ok)
         {
             if(monstersQuery.value(7).toString()==BaseServer::text_male)
                 playerMonster.gender=Gender_Male;
             else if(monstersQuery.value(7).toString()==BaseServer::text_female)
                 playerMonster.gender=Gender_Female;
             else if(monstersQuery.value(7).toString()==BaseServer::text_unknown)
                 playerMonster.gender=Gender_Unknown;
             else
             {
                 playerMonster.gender=Gender_Unknown;
                 DebugClass::debugConsole(QStringLiteral("unknown monster gender: %1").arg(monstersQuery.value(7).toString()));
                 ok=false;
             }
         }
         if(ok)
         {
             playerMonster.egg_step=monstersQuery.value(8).toUInt(&ok);
             if(!ok)
                 DebugClass::debugConsole(QStringLiteral("monster egg_step: %1 is not a number").arg(monstersQuery.value(8).toString()));
         }
         if(ok)
             marketPlayerMonster.player=monstersQuery.value(9).toUInt(&ok);
         if(ok)
             marketPlayerMonster.cash=monstersQuery.value(10).toULongLong(&ok);
         if(ok)
             marketPlayerMonster.bitcoin=monstersQuery.value(11).toDouble(&ok);
         //stats
         if(ok)
         {
             playerMonster.hp=monstersQuery.value(1).toUInt(&ok);
             if(ok)
             {
                 const Monster::Stat &stat=CommonFightEngine::getStat(CommonDatapack::commonDatapack.monsters.value(playerMonster.monster),playerMonster.level);
                 if(playerMonster.hp>stat.hp)
                 {
                     DebugClass::debugConsole(QStringLiteral("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                  .arg(playerMonster.hp)
                                  .arg(stat.hp)
                                  .arg(playerMonster.level)
                                  .arg(playerMonster.monster)
                                  );
                     playerMonster.hp=stat.hp;
                 }
             }
             else
                 DebugClass::debugConsole(QStringLiteral("monster hp: %1 is not a number").arg(monstersQuery.value(1).toString()));
         }
         //finish it
         if(ok)
         {
             playerMonster.buffs=loadMonsterBuffs(playerMonster.id);
             playerMonster.skills=loadMonsterSkills(playerMonster.id);
             marketPlayerMonster.monster=playerMonster;
             GlobalServerData::serverPrivateVariables.marketPlayerMonsterList << marketPlayerMonster;
         }
     }
}

void BaseServer::preload_market_items()
{
    LocalClientHandler::marketObjectIdList.clear();
    LocalClientHandler::marketObjectIdList.reserve(65535);
    int index=0;
    while(index<=65535)
    {
        LocalClientHandler::marketObjectIdList << index;
        index++;
    }
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `item`,`quantity`,`character`,`market_price`,`market_bitcoin` FROM `item` WHERE `place`='market'");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT item,quantity,character,market_price,market_bitcoin FROM item WHERE place='market'");
        break;
    }
    bool ok;
    QSqlQuery itemQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!itemQuery.exec(queryText))
        DebugClass::debugConsole(itemQuery.lastQuery()+": "+itemQuery.lastError().text());
    //parse the result
    while(itemQuery.next())
    {
        MarketItem marketItem;
        marketItem.item=itemQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("item id is not a number, skip"));
            continue;
        }
        marketItem.quantity=itemQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("quantity is not a number, skip"));
            continue;
        }
        marketItem.player=itemQuery.value(2).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("player id is not a number, skip"));
            continue;
        }
        marketItem.cash=itemQuery.value(3).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("cash is not a number, skip"));
            continue;
        }
        marketItem.bitcoin=itemQuery.value(4).toDouble(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("bitcoin is not a number, skip"));
            continue;
        }
        if(LocalClientHandler::marketObjectIdList.isEmpty())
        {
            DebugClass::debugConsole(QStringLiteral("not more marketObjectId into the list, skip"));
            return;
        }
        marketItem.marketObjectId=LocalClientHandler::marketObjectIdList.first();
        LocalClientHandler::marketObjectIdList.removeFirst();
        GlobalServerData::serverPrivateVariables.marketItemList << marketItem;
    }
}

QList<PlayerBuff> BaseServer::loadMonsterBuffs(const quint32 &monsterId)
{
    QList<PlayerBuff> buffs;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterBuffsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterBuffsQuery.exec(queryText))
        DebugClass::debugConsole(monsterBuffsQuery.lastQuery()+": "+monsterBuffsQuery.lastError().text());
    while(monsterBuffsQuery.next())
    {
        PlayerBuff buff;
        buff.buff=monsterBuffsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("buff id: %1 is not a number").arg(monsterBuffsQuery.value(0).toString()));
        if(ok)
        {
            buff.level=monsterBuffsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(buff.level<=0 || buff.level>CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("buff level: %1 is not a number").arg(monsterBuffsQuery.value(2).toString()));
        }
        if(ok)
        {
            if(CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.at(buff.level-1).duration!=Buff::Duration_Always)
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 can't be loaded from the db if is not permanent").arg(buff.buff).arg(monsterId));
            }
        }
        if(ok)
            buffs << buff;
    }
    return buffs;
}

QList<PlayerMonster::PlayerSkill> BaseServer::loadMonsterSkills(const quint32 &monsterId)
{
    QList<PlayerMonster::PlayerSkill> skills;
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1").arg(monsterId);
        break;
    }

    bool ok;
    QSqlQuery monsterSkillsQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!monsterSkillsQuery.exec(queryText))
        DebugClass::debugConsole(monsterSkillsQuery.lastQuery()+": "+monsterSkillsQuery.lastError().text());
    while(monsterSkillsQuery.next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=monsterSkillsQuery.value(0).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("skill id: %1 is not a number").arg(monsterSkillsQuery.value(0).toString()));
        if(ok)
        {
            skill.level=monsterSkillsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("skill level: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
        }
        if(ok)
        {
            skill.endurance=monsterSkillsQuery.value(2).toUInt(&ok);
            if(ok)
            {
                if(skill.endurance>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance)
                {
                    skill.endurance=CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.at(skill.level-1).endurance;
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("endurance of skill %1 for monsterId: %2 have been fixed by lower at ").arg(skill.endurance));
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("skill level: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
        }
        if(ok)
            skills << skill;
    }
    return skills;
}

void BaseServer::preload_the_city_capture()
{
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
    GlobalServerData::serverPrivateVariables.timer_city_capture=new QTimer();
    GlobalServerData::serverPrivateVariables.timer_city_capture->setSingleShot(true);
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,this,&BaseServer::load_next_city_capture,Qt::QueuedConnection);
    connect(GlobalServerData::serverPrivateVariables.timer_city_capture,&QTimer::timeout,&LocalClientHandler::startTheCityCapture);
    load_next_city_capture();
}

void BaseServer::preload_the_map()
{
    GlobalServerData::serverPrivateVariables.datapack_mapPath=GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_MAP;
    #ifdef DEBUG_MESSAGE_MAP_LOAD
    DebugClass::debugConsole(QStringLiteral("start preload the map, into: %1").arg(GlobalServerData::serverPrivateVariables.datapack_mapPath));
    #endif
    Map_loader map_temp;
    QList<Map_semi> semi_loaded_map;
    QStringList map_name;
    QStringList full_map_name;
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);

    //load the map
    int size=returnList.size();
    int index=0;
    int sub_index;
    QRegularExpression mapFilter(QLatin1String("\\.tmx$"));
    while(index<size)
    {
        QString fileName=returnList.at(index);
        fileName.replace(BaseServer::text_antislash,BaseServer::text_slash);
        if(fileName.contains(mapFilter))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(QStringLiteral("load the map: %1").arg(fileName));
            #endif
            full_map_name << fileName;
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithmSelection_Simple:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new Map_server_MapVisibility_simple;
                    break;
                    case MapVisibilityAlgorithmSelection_WithBorder:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new Map_server_MapVisibility_withBorder;
                    break;
                    case MapVisibilityAlgorithmSelection_None:
                    default:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new MapServer;
                    break;
                }

                GlobalServerData::serverPrivateVariables.map_list[fileName]->width			= map_temp.map_to_send.width;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->height			= map_temp.map_to_send.height;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->parsed_layer	= map_temp.map_to_send.parsed_layer;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->map_file		= fileName;

                map_name << fileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list.value(fileName);

                if(!map_temp.map_to_send.border.top.fileName.isEmpty())
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(!map_temp.map_to_send.border.left.fileName.isEmpty())
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(!map_temp.map_to_send.border.right.fileName.isEmpty())
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                const int &listsize=map_temp.map_to_send.teleport.size();
                while(sub_index<listsize)
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map << map_semi;
            }
            else
                DebugClass::debugConsole(QStringLiteral("error at loading: %1, error: %2").arg(fileName).arg(map_temp.errorString()));
        }
        index++;
    }
    full_map_name.sort();

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(!semi_loaded_map.at(index).border.bottom.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
            semi_loaded_map[index].map->border.bottom.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.bottom.fileName);
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(!semi_loaded_map.at(index).border.top.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
            semi_loaded_map[index].map->border.top.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.top.fileName);
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(!semi_loaded_map.at(index).border.left.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
            semi_loaded_map[index].map->border.left.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.left.fileName);
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(!semi_loaded_map.at(index).border.right.fileName.isEmpty() && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
            semi_loaded_map[index].map->border.right.map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.at(index).border.right.fileName);
        else
            semi_loaded_map[index].map->border.right.map=NULL;

        index++;
    }

    //resolv the teleported into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        int sub_index=0;
        while(sub_index<semi_loaded_map.value(index).old_map.teleport.size())
        {
            if(GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map))
            {
                if(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)->width
                        && semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)->height)
                {
                    int virtual_position=semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x+semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y*semi_loaded_map.value(index).map->width;
                    if(semi_loaded_map.value(index).map->teleporter.contains(virtual_position))
                    {
                        DebugClass::debugConsole(QStringLiteral("already found teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                             .arg(semi_loaded_map.value(index).map->map_file)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
                    }
                    else
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        DebugClass::debugConsole(QStringLiteral("teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                                     .arg(semi_loaded_map.value(index).map->map_file)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
                        #endif
                        CommonMap::Teleporter *teleporter=&semi_loaded_map[index].map->teleporter[virtual_position];
                        teleporter->map=GlobalServerData::serverPrivateVariables.map_list.value(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map);
                        teleporter->x=semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x;
                        teleporter->y=semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y;
                        teleporter->condition=semi_loaded_map.value(index).old_map.teleport.at(sub_index).condition;
                    }
                }
                else
                    DebugClass::debugConsole(QStringLiteral("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the tp is out of range")
                         .arg(semi_loaded_map.value(index).map->map_file)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
            }
            else
                DebugClass::debugConsole(QStringLiteral("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the map is not found")
                     .arg(semi_loaded_map.value(index).map->map_file)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).map)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));

            sub_index++;
        }
        index++;
    }

    preload_the_bots(semi_loaded_map);

    //clean border balise without another oposite border
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map==NULL)
                DebugClass::debugConsole(QStringLiteral("the map %1 have bottom map: %2, the map %2 have not a top map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->map_file));
            else
                DebugClass::debugConsole(QStringLiteral("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map->border.top.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map==NULL)
                DebugClass::debugConsole(QStringLiteral("the map %1 have top map: %2, the map %2 have not a bottom map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->map_file));
            else
                DebugClass::debugConsole(QStringLiteral("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map->border.bottom.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map==NULL)
                DebugClass::debugConsole(QStringLiteral("the map %1 have left map: %2, the map %2 have not a right map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->map_file));
            else
                DebugClass::debugConsole(QStringLiteral("the map %1 have left map: %2, the map %2 have this right map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map->border.right.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->border.left.map!=GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index)))
        {
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->border.left.map==NULL)
                DebugClass::debugConsole(QStringLiteral("the map %1 have right map: %2, the map %2 have not a left map").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->map_file));
            else
                DebugClass::debugConsole(QStringLiteral("the map %1 have right map: %2, the map %2 have this left map: %3").arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->map_file).arg(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->near_map.contains(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map;
        }

        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border_map=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map;
        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index));
        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.bottom.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset-semi_loaded_map.at(index).border.bottom.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.top.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset-semi_loaded_map.at(index).border.top.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.left.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset-semi_loaded_map.at(index).border.left.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list.value(map_name.at(index))->border.right.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset-semi_loaded_map.at(index).border.right.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=0;
        index++;
    }

    //load the rescue
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        sub_index=0;
        while(sub_index<semi_loaded_map.value(index).old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map.value(index).old_map.rescue_points.at(sub_index);
            QPair<quint8,quint8> coord;
            coord.first=point.x;
            coord.second=point.y;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])->rescue[coord]=Orientation_bottom;
            sub_index++;
        }
        index++;
    }

    size=full_map_name.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.contains(full_map_name.at(index)))
        {
            GlobalServerData::serverPrivateVariables.map_list[full_map_name.at(index)]->id=index;
            GlobalServerData::serverPrivateVariables.id_map_to_map[GlobalServerData::serverPrivateVariables.map_list[full_map_name.at(index)]->id]=full_map_name.at(index);
        }
        index++;
    }

    DebugClass::debugConsole(QStringLiteral("%1 map(s) loaded").arg(GlobalServerData::serverPrivateVariables.map_list.size()));

    botFiles.clear();
}

void BaseServer::preload_the_skin()
{
    QStringList skinFolderList=FacilityLib::skinIdList(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    const int &listsize=skinFolderList.size();
    while(index<listsize)
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    DebugClass::debugConsole(QStringLiteral("%1 skin(s) loaded").arg(listsize));
}

void BaseServer::preload_the_datapack()
{
    QStringList extensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_ALLOWED+BaseServer::text_dotcomma+CATCHCHALLENGER_EXTENSION_COMPRESSED).split(BaseServer::text_dotcomma);
    ClientHeavyLoad::extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(BaseServer::text_dotcomma);
    ClientHeavyLoad::compressedExtension=compressedExtensionAllowedTemp.toSet();
}

void BaseServer::preload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
    LocalClientHandler::marketObjectIdList.reserve(GlobalServerData::serverSettings.max_players);
    int index=0;
    while(index<GlobalServerData::serverSettings.max_players)
    {
        ClientHeavyLoad::simplifiedIdList << index;
        index++;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    QHash<QString,CommonMap *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,CommonMap *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
        while (i != i_end)
        {
            static_cast<Map_server_MapVisibility_simple *>(i.value())->show=true;
            i++;
        }
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
        while (i != i_end)
        {
            static_cast<Map_server_MapVisibility_withBorder *>(i.value())->show=true;
            static_cast<Map_server_MapVisibility_withBorder *>(i.value())->showWithBorder=true;
            static_cast<Map_server_MapVisibility_withBorder *>(i.value())->clientsOnBorder=0;
            i++;
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
        default:
        break;
    }
}

void BaseServer::preload_the_bots(const QList<Map_semi> &semi_loaded_map)
{
    int shops_number=0;
    int bots_number=0;
    int learnpoint_number=0;
    int healpoint_number=0;
    int marketpoint_number=0;
    int zonecapturepoint_number=0;
    int botfights_number=0;
    int botfightstigger_number=0;
    //resolv the botfights, bots, shops, learn, heal, zonecapture, market
    const int &size=semi_loaded_map.size();
    int index=0;
    bool ok;
    while(index<size)
    {
        int sub_index=0;
        const int &botssize=semi_loaded_map.value(index).old_map.bots.size();
        while(sub_index<botssize)
        {
            bots_number++;
            Map_to_send::Bot_Semi bot_Semi=semi_loaded_map.value(index).old_map.bots.at(sub_index);
            loadBotFile(bot_Semi.file);
            if(botFiles.contains(bot_Semi.file))
                if(botFiles.value(bot_Semi.file).contains(bot_Semi.id))
                {
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Bot %1 (%2) at %3 (%4,%5)").arg(bot_Semi.file).arg(bot_Semi.id).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                    #endif
                    QHashIterator<quint8,QDomElement> i(botFiles.value(bot_Semi.file).value(bot_Semi.id).step);
                    while (i.hasNext()) {
                        i.next();
                        QDomElement step = i.value();
                        if(step.attribute(BaseServer::text_type)==BaseServer::text_shop)
                        {
                            if(!step.hasAttribute(BaseServer::text_shop))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"shop\": for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 shop=step.attribute(BaseServer::text_shop).toUInt(&ok);
                                if(!ok)
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("shop is not a number: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else if(!GlobalServerData::serverPrivateVariables.shops.contains(shop))
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("shop number is not valid shop: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("shop put at: %1 (%2,%3)")
                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    #endif
                                    static_cast<MapServer *>(semi_loaded_map.value(index).map)->shops.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_learn)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->learn.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("learn point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("learn point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->learn.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                learnpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_heal)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->heal.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("heal point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("heal point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->heal.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                healpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_market)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->market.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("market point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("market point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map.value(index).map)->market.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                marketpoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_zonecapture)
                        {
                            if(!step.hasAttribute(BaseServer::text_zone))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("zonecapture point have not the zone attribute: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->zonecapture.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("zonecapture point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("zonecapture point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->zonecapture[QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)]=step.attribute(BaseServer::text_zone);
                                zonecapturepoint_number++;
                            }
                        }
                        else if(step.attribute(BaseServer::text_type)==BaseServer::text_fight)
                        {
                            if(static_cast<MapServer *>(semi_loaded_map.value(index).map)->botsFight.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QStringLiteral("botsFight point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 fightid=step.attribute(BaseServer::text_fightid).toUInt(&ok);
                                if(ok)
                                {
                                    if(CommonDatapack::commonDatapack.botFights.contains(fightid))
                                    {
                                        if(bot_Semi.property_text.contains(BaseServer::text_lookAt))
                                        {
                                            Direction direction;
                                            if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_left)
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_right)
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(bot_Semi.property_text.value(BaseServer::text_lookAt)==BaseServer::text_top)
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(bot_Semi.property_text.value(BaseServer::text_lookAt)!=BaseServer::text_bottom)
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Wrong direction for the bot at %1 (%2,%3)")
                                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("botsFight point put at: %1 (%2,%3)")
                                                .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                            #endif
                                            static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),fightid);
                                            botfights_number++;

                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(direction));
                                            #endif
                                            quint8 temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            int index_botfight_range=0;
                                            CatchChallenger::CommonMap *map=semi_loaded_map.value(index).map;
                                            CatchChallenger::CommonMap *old_map=map;
                                            while(index_botfight_range<CATCHCHALLENGER_BOTFIGHT_RANGE)
                                            {
                                                if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                                                    break;
                                                if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                                                    break;
                                                if(map!=old_map)
                                                    break;
                                                static_cast<MapServer *>(semi_loaded_map[index].map)->botsFightTrigger.insert(QPair<quint8,quint8>(temp_x,temp_y),fightid);
                                                index_botfight_range++;
                                                botfightstigger_number++;
                                            }
                                        }
                                        else
                                            DebugClass::debugConsole(QStringLiteral("lookAt not found: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    }
                                    else
                                        DebugClass::debugConsole(QStringLiteral("fightid not found into the list: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                }
                                else
                                    DebugClass::debugConsole(QStringLiteral("botsFight point have wrong fightid: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                            }
                        }
                    }
                }
            sub_index++;
        }
        index++;
    }
    botIdLoaded.clear();

    DebugClass::debugConsole(QStringLiteral("%1 learn point(s) on map loaded").arg(learnpoint_number));
    DebugClass::debugConsole(QStringLiteral("%1 zonecapture point(s) on map loaded").arg(zonecapturepoint_number));
    DebugClass::debugConsole(QStringLiteral("%1 heal point(s) on map loaded").arg(healpoint_number));
    DebugClass::debugConsole(QStringLiteral("%1 market point(s) on map loaded").arg(marketpoint_number));
    DebugClass::debugConsole(QStringLiteral("%1 bot fight(s) on map loaded").arg(botfights_number));
    DebugClass::debugConsole(QStringLiteral("%1 bot fights tigger(s) on map loaded").arg(botfightstigger_number));
    DebugClass::debugConsole(QStringLiteral("%1 shop(s) on map loaded").arg(shops_number));
    DebugClass::debugConsole(QStringLiteral("%1 bots(s) on map loaded").arg(bots_number));
}

void BaseServer::load_next_city_capture()
{
    GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
    qint64 time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
    GlobalServerData::serverPrivateVariables.timer_city_capture->start(time);
}

void BaseServer::parseJustLoadedMap(const Map_to_send &,const QString &)
{
}

bool BaseServer::initialize_the_database()
{
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        DebugClass::debugConsole(QStringLiteral("Disconnected to %1 at %2 (%3), previous connection: %4")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host)
                                 .arg(GlobalServerData::serverPrivateVariables.db->isOpen())
                                 .arg(QSqlDatabase::connectionNames().join(";"))
                                 );
        closeDB();
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        DebugClass::debugConsole(QStringLiteral("database type unknow"));
        return false;
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL","server");
        GlobalServerData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalServerData::serverPrivateVariables.db->setHostName(GlobalServerData::serverSettings.database.mysql.host);
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.mysql.db);
        GlobalServerData::serverPrivateVariables.db->setUserName(GlobalServerData::serverSettings.database.mysql.login);
        GlobalServerData::serverPrivateVariables.db->setPassword(GlobalServerData::serverSettings.database.mysql.pass);
        GlobalServerData::serverPrivateVariables.db_type_string=QLatin1Literal("mysql");

        GlobalServerData::serverPrivateVariables.db_query_login=QStringLiteral("SELECT `id`,LOWER(HEX(`password`)) FROM `account` WHERE `login`=UNHEX('%1')");
        GlobalServerData::serverPrivateVariables.db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4);");
        GlobalServerData::serverPrivateVariables.db_query_characters=QStringLiteral("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect`,`map` FROM `character` WHERE `account`=%1 LIMIT 0,%2");
        GlobalServerData::serverPrivateVariables.db_query_played_time=QStringLiteral("UPDATE `character` SET `played_time`=`played_time`+%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`sp`=%6,`position`=%7 WHERE `id`=%1;");
        GlobalServerData::serverPrivateVariables.db_query_monster_skill=QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3;");
        GlobalServerData::serverPrivateVariables.db_query_character_by_id=QStringLiteral("SELECT `account`,`pseudo`,`skin`,`x`,`y`,`orientation`,`map`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`warehouse_cash`,`allow`,`clan_leader`,`bitcoin_offset`,`market_cash`,`market_bitcoin`,`time_to_delete` FROM `character` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete=QStringLiteral("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect=QStringLiteral("UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_clan=QStringLiteral("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1");

        GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id=QStringLiteral("SELECT `id` FROM `monster` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff=QStringLiteral("DELETE FROM `monster_buff` WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill=QStringLiteral("DELETE FROM `monster_skill` WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM `bot_already_beaten` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_character=QStringLiteral("DELETE FROM `character` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_item=QStringLiteral("DELETE FROM `item` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster=QStringLiteral("DELETE FROM `monster` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant=QStringLiteral("DELETE FROM `plant` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_quest=QStringLiteral("DELETE FROM `quest` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_recipes=QStringLiteral("DELETE FROM `recipes` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_reputation=QStringLiteral("DELETE FROM `reputation` WHERE `character`=%1");

        GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo=QStringLiteral("SELECT `id` FROM `character` WHERE `pseudo`='%1'");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`,`market_price`,`market_bitcoin`) VALUES(%1,%2,%3,%4,%5,0,0,%6,'%7',0,%3,'wear',%8,0,0);");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill=QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_insert_reputation=QStringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_insert_item=QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'wear');");
        GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id=QStringLiteral("SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id=QStringLiteral("SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id=QStringLiteral("SELECT `recipe` FROM `recipes` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id=QStringLiteral("SELECT `item`,`quantity`,`place` FROM `item` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_by_charater_item_place=QStringLiteral("DELETE FROM `item` WHERE `character`=%1 AND `item`=%2 AND `place`='%3'");
        GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`place` FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_place_wearhouse=QStringLiteral("UPDATE `monster` SET `place`='warehouse' WHERE `id`=%1;");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id=QStringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id=QStringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten=QStringLiteral("SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE","server");
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.sqlite.file);
        GlobalServerData::serverPrivateVariables.db_type_string=QLatin1Literal("sqlite");

        GlobalServerData::serverPrivateVariables.db_query_login=QStringLiteral("SELECT id,password FROM account WHERE login='%1'");
        GlobalServerData::serverPrivateVariables.db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4);");
        GlobalServerData::serverPrivateVariables.db_query_characters=QStringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 LIMIT 0,%2");
        GlobalServerData::serverPrivateVariables.db_query_played_time=QStringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1;");
        GlobalServerData::serverPrivateVariables.db_query_monster_skill=QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3;");
        GlobalServerData::serverPrivateVariables.db_query_character_by_id=QStringLiteral("SELECT account,pseudo,skin,x,y,orientation,map,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,allow,clan_leader,bitcoin_offset,market_cash,market_bitcoin,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete=QStringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect=QStringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_clan=QStringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id=QStringLiteral("SELECT id FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_character=QStringLiteral("DELETE FROM character WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_item=QStringLiteral("DELETE FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster=QStringLiteral("DELETE FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant=QStringLiteral("DELETE FROM plant WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_quest=QStringLiteral("DELETE FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_recipes=QStringLiteral("DELETE FROM recipes WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_reputation=QStringLiteral("DELETE FROM reputation WHERE character=%1");

        GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo=QStringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`place`,`position`,`market_price`,`market_bitcoin`) VALUES(%1,%2,%3,%4,%5,0,0,%6,'%7',0,%3,'wear',%8,0,0);");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill=QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_insert_reputation=QStringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_insert_item=QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`,`place`) VALUES(%1,%2,%3,'wear');");
        GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id=QStringLiteral("SELECT type,point,level FROM reputation WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id=QStringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id=QStringLiteral("SELECT recipe FROM recipes WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id=QStringLiteral("SELECT item,quantity,place FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_by_charater_item_place=QStringLiteral("DELETE FROM item WHERE character=%1 AND item=%2 AND place='%3'");
        GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,place FROM monster WHERE character=%1 ORDER BY position ASC");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_place_wearhouse=QStringLiteral("UPDATE monster SET place='warehouse' WHERE id=%1;");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten=QStringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1");
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QStringLiteral("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        emit error(QStringLiteral("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        return false;
    }
    DebugClass::debugConsole(QStringLiteral("Connected to %1 at %2 (%3)")
                             .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                             .arg(GlobalServerData::serverSettings.database.mysql.host)
                             .arg(GlobalServerData::serverPrivateVariables.db->isOpen()));
    return true;
}

void BaseServer::loadBotFile(const QString &file)
{
    if(botFiles.contains(file))
        return;
    botFiles[file];//create the entry
    QDomDocument domDocument;
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile mapFile(file);
        if(!mapFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapFile.fileName()+": "+mapFile.errorString();
            return;
        }
        QByteArray xmlContent=mapFile.readAll();
        mapFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="bots")
    {
        qDebug() << QStringLiteral("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement(BaseServer::text_bot);
    while(!child.isNull())
    {
        if(!child.hasAttribute(BaseServer::text_id))
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 id=child.attribute(BaseServer::text_id).toUInt(&ok);
            if(ok)
            {
                if(botIdLoaded.contains(id))
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Bot %3 into file %4 have same id as another bot: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()).arg(id).arg(file));
                botIdLoaded << id;
                botFiles[file][id];
                QDomElement step = child.firstChildElement(BaseServer::text_step);
                while(!step.isNull())
                {
                    if(!step.hasAttribute(BaseServer::text_id))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.hasAttribute(BaseServer::text_type))
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.isElement())
                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute(BaseServer::text_type)).arg(step.lineNumber())));
                    else
                    {
                        quint32 stepId=step.attribute(BaseServer::text_id).toUInt(&ok);
                        if(ok)
                            botFiles[file][id].step[stepId]=step;
                    }
                    step = step.nextSiblingElement(BaseServer::text_step);
                }
                if(!botFiles.value(file).value(id).step.contains(1))
                    botFiles[file].remove(id);
            }
            else
                CatchChallenger::DebugClass::debugConsole(QStringLiteral("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement(BaseServer::text_bot);
    }
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
    dataLoaded=false;
    GlobalServerData::serverPrivateVariables.stopIt=true;

    unload_market();
    unload_industries();
    unload_zone();
    unload_the_city_capture();
    unload_the_visibility_algorithm();
    unload_the_plant_on_map();
    unload_the_map();
    unload_the_bots();
    unload_monsters_drops();
    unload_shop();
    unload_the_skin();
    unload_the_datapack();
    unload_the_players();
    unload_the_static_data();

    CommonDatapack::commonDatapack.unload();
}

void BaseServer::unload_the_static_data()
{
    LocalClientHandler::resetAll();
    ClientBroadCast::playerByPseudo.clear();
    ClientBroadCast::playerByClan.clear();
    ClientBroadCast::clientBroadCastList.clear();
    ClientHeavyLoad::simplifiedIdList.clear();
}

void BaseServer::unload_zone()
{
    GlobalServerData::serverPrivateVariables.captureFightIdList.clear();
}

void BaseServer::unload_market()
{
    GlobalServerData::serverPrivateVariables.tradedMonster.clear();
    GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.clear();
    GlobalServerData::serverPrivateVariables.marketItemList.clear();
}

void BaseServer::unload_industries()
{
    GlobalServerData::serverPrivateVariables.industriesStatus.clear();
}

void BaseServer::unload_the_city_capture()
{
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
        GlobalServerData::serverPrivateVariables.timer_city_capture=NULL;
    }
}

void BaseServer::unload_the_bots()
{
}

void BaseServer::unload_the_map()
{
    QHash<QString,CommonMap *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,CommonMap *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    while (i != i_end)
    {
        CommonMap::removeParsedLayer(i.value()->parsed_layer);
        delete i.value();
        i++;
    }
    GlobalServerData::serverPrivateVariables.map_list.clear();
    GlobalServerData::serverPrivateVariables.botSpawn.clear();
    botIdLoaded.clear();
}

void BaseServer::unload_the_skin()
{
    GlobalServerData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_visibility_algorithm()
{
}

void BaseServer::unload_the_datapack()
{
    ClientHeavyLoad::compressedExtension.clear();
}

void BaseServer::unload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
}

bool BaseServer::check_if_now_stopped()
{
    if(client_list.size()!=0)
        return false;
    if(stat!=InDown)
        return false;

    DebugClass::debugConsole("Fully stopped");
    if(GlobalServerData::serverPrivateVariables.db!=NULL)
    {
        DebugClass::debugConsole(QStringLiteral("Disconnected to %1 at %2 (%3)")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host)
                                 .arg(GlobalServerData::serverPrivateVariables.db->isOpen()));
        closeDB();
    }
    stat=Down;
    emit is_started(false);

    unload_the_data();
    return true;
}

void BaseServer::setSettings(ServerSettings settings)
{
    //load it
    GlobalServerData::serverSettings=settings;

    loadAndFixSettings();
}

ServerSettings BaseServer::getSettings()
{
    return GlobalServerData::serverSettings;
}

void BaseServer::loadAndFixSettings()
{
    GlobalServerData::serverPrivateVariables.server_message=GlobalServerData::serverSettings.server_message.split(QRegularExpression("[\n\r]+"));
    while(GlobalServerData::serverPrivateVariables.server_message.size()>16)
        GlobalServerData::serverPrivateVariables.server_message.removeLast();
    int index=0;
    const int &listsize=GlobalServerData::serverPrivateVariables.server_message.size();
    while(index<listsize)
    {
        GlobalServerData::serverPrivateVariables.server_message[index].truncate(128);
        index++;
    }
    bool removeTheLastList;
    do
    {
        removeTheLastList=!GlobalServerData::serverPrivateVariables.server_message.isEmpty();
        if(removeTheLastList)
            removeTheLastList=GlobalServerData::serverPrivateVariables.server_message.last().isEmpty();
        if(removeTheLastList)
            GlobalServerData::serverPrivateVariables.server_message.removeLast();
    } while(removeTheLastList);

    if(CommonSettings::commonSettings.max_character<CommonSettings::commonSettings.min_character)
        CommonSettings::commonSettings.max_character=CommonSettings::commonSettings.min_character;
    if(CommonSettings::commonSettings.character_delete_time<=0)
        CommonSettings::commonSettings.character_delete_time=7*24*3600;

    if(GlobalServerData::serverSettings.server_port<=0)
        GlobalServerData::serverSettings.server_port=42489;
    if(GlobalServerData::serverSettings.proxy_port<=0)
        GlobalServerData::serverSettings.proxy=QString();

    //check the settings here
    if(GlobalServerData::serverSettings.max_players<1)
        GlobalServerData::serverSettings.max_players=200;
    ProtocolParsing::setMaxPlayers(GlobalServerData::serverSettings.max_players);

/*    quint8 player_list_size;
    if(GlobalServerData::serverSettings.max_players<=255)
        player_list_size=sizeof(quint8);
    else
        player_list_size=sizeof(quint16);*/
    quint8 map_list_size;
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        map_list_size=sizeof(quint8);
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        map_list_size=sizeof(quint16);
    else
        map_list_size=sizeof(quint32);
    GlobalServerData::serverPrivateVariables.sizeofInsertRequest=
            //mutualised
            sizeof(quint8)+map_list_size+/*player_list_size same with move, delete, ...*/
            //of the player
            /*player_list_size same with move, delete, ...*/+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+sizeof(quint8)+0/*pseudo size put directy*/+sizeof(quint8);

    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.max<5)
            GlobalServerData::serverSettings.mapVisibility.simple.max=5;
        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow<3)
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=3;

        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.max_players;
        if(GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.simple.max=GlobalServerData::serverSettings.max_players;
        if(GlobalServerData::serverSettings.mapVisibility.simple.reshow>GlobalServerData::serverSettings.mapVisibility.simple.max)
            GlobalServerData::serverSettings.mapVisibility.simple.reshow=GlobalServerData::serverSettings.mapVisibility.simple.max;
    }
    {
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder<3)
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=3;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder<2)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=2;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.max<5)
            GlobalServerData::serverSettings.mapVisibility.withBorder.max=5;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow<3)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=3;

        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.max_players;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=GlobalServerData::serverSettings.max_players;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=GlobalServerData::serverSettings.max_players;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.max>GlobalServerData::serverSettings.max_players)
            GlobalServerData::serverSettings.mapVisibility.withBorder.max=GlobalServerData::serverSettings.max_players;

        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshow>GlobalServerData::serverSettings.mapVisibility.withBorder.max)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshow=GlobalServerData::serverSettings.mapVisibility.withBorder.max;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.max)
            GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.max;
        if(GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder>GlobalServerData::serverSettings.mapVisibility.withBorder.reshow)
            GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder=GlobalServerData::serverSettings.mapVisibility.withBorder.reshow;
    }

    if(GlobalServerData::serverSettings.database.secondToPositionSync==0)
        GlobalServerData::serverPrivateVariables.positionSync.stop();
    else
        GlobalServerData::serverPrivateVariables.positionSync.start(GlobalServerData::serverSettings.database.secondToPositionSync*1000);

    switch(GlobalServerData::serverSettings.database.type)
    {
        case CatchChallenger::ServerSettings::Database::DatabaseType_SQLite:
        case CatchChallenger::ServerSettings::Database::DatabaseType_Mysql:
        break;
        default:
            qDebug() << "Wrong db type";
        break;
    }
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case CatchChallenger::MapVisibilityAlgorithmSelection_None:
        case CatchChallenger::MapVisibilityAlgorithmSelection_Simple:
        case CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder:
        break;
        default:
            GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm=CatchChallenger::MapVisibilityAlgorithmSelection_Simple;
            qDebug() << "Wrong visibility algorithm";
        break;
    }

    switch(GlobalServerData::serverSettings.city.capture.frenquency)
    {
        case City::Capture::Frequency_week:
        case City::Capture::Frequency_month:
        break;
        default:
            GlobalServerData::serverSettings.city.capture.frenquency=City::Capture::Frequency_week;
            qDebug() << "Wrong City::Capture::Frequency";
        break;
    }
    switch(GlobalServerData::serverSettings.city.capture.day)
    {
        case City::Capture::Monday:
        case City::Capture::Tuesday:
        case City::Capture::Wednesday:
        case City::Capture::Thursday:
        case City::Capture::Friday:
        case City::Capture::Saturday:
        case City::Capture::Sunday:
        break;
        default:
            GlobalServerData::serverSettings.city.capture.day=City::Capture::Monday;
            qDebug() << "Wrong City::Capture::Day";
        break;
    }
    if(GlobalServerData::serverSettings.city.capture.hour>24)
    {
        qDebug() << "GlobalServerData::serverSettings.city.capture.hours out of range";
        GlobalServerData::serverSettings.city.capture.hour=0;
    }
    if(GlobalServerData::serverSettings.city.capture.minute>60)
    {
        qDebug() << "GlobalServerData::serverSettings.city.capture.minutes out of range";
        GlobalServerData::serverSettings.city.capture.minute=0;
    }
    if(GlobalServerData::serverSettings.bitcoin.enabled)
    {
        if(GlobalServerData::serverSettings.bitcoin.fee<0 || GlobalServerData::serverSettings.bitcoin.fee>100)
            GlobalServerData::serverSettings.bitcoin.enabled=false;
        if(!GlobalServerData::serverSettings.bitcoin.address.contains(QRegularExpression(CATCHCHALLENGER_SERVER_BITCOIN_ADDRESS_REGEX)))
            GlobalServerData::serverSettings.bitcoin.enabled=false;
        if(GlobalServerData::serverSettings.bitcoin.binaryPath.isEmpty())
            GlobalServerData::serverSettings.bitcoin.enabled=false;
        if(GlobalServerData::serverSettings.bitcoin.port==0 || GlobalServerData::serverSettings.bitcoin.port>65534)
            GlobalServerData::serverSettings.bitcoin.enabled=false;
        if(GlobalServerData::serverSettings.bitcoin.workingPath.isEmpty())
            GlobalServerData::serverSettings.bitcoin.enabled=false;
    }
    if(GlobalServerData::serverSettings.bitcoin.enabled)
    {
        GlobalServerData::serverSettings.bitcoin.binaryPath=GlobalServerData::serverSettings.bitcoin.binaryPath.replace("%application_path%",QCoreApplication::applicationDirPath());
        GlobalServerData::serverSettings.bitcoin.workingPath=GlobalServerData::serverSettings.bitcoin.workingPath.replace("%application_path%",QCoreApplication::applicationDirPath());
        if(!QFileInfo(GlobalServerData::serverSettings.bitcoin.binaryPath).isFile())
            GlobalServerData::serverSettings.bitcoin.enabled=false;
    }

    switch(GlobalServerData::serverSettings.compressionType)
    {
        case CatchChallenger::CompressionType_None:
            GlobalServerData::serverSettings.compressionType      = CompressionType_None;
            ProtocolParsing::compressionType=ProtocolParsing::CompressionType_None;
        break;
        default:
        case CatchChallenger::CompressionType_Zlib:
            GlobalServerData::serverSettings.compressionType      = CompressionType_Zlib;
            ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Zlib;
        break;
        case CatchChallenger::CompressionType_Xz:
            GlobalServerData::serverSettings.compressionType      = CompressionType_Xz;
            ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Xz;
        break;
    }
}

void BaseServer::start_internal_server()
{
    if(GlobalServerData::serverSettings.bitcoin.enabled)
        if(!QFileInfo(GlobalServerData::serverSettings.bitcoin.workingPath).isDir())
            if(!QDir().mkpath(GlobalServerData::serverSettings.bitcoin.workingPath))
                GlobalServerData::serverSettings.bitcoin.enabled=false;
    if(GlobalServerData::serverSettings.bitcoin.enabled)
    {
        GlobalServerData::serverPrivateVariables.bitcoin.process.start(GlobalServerData::serverSettings.bitcoin.binaryPath,QStringList()
                                                                       << QStringLiteral("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                       << QStringLiteral("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                       << QStringLiteral("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                       << QStringLiteral("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
                                                                       );
        GlobalServerData::serverPrivateVariables.bitcoin.process.waitForStarted();
        GlobalServerData::serverPrivateVariables.bitcoin.enabled=GlobalServerData::serverPrivateVariables.bitcoin.process.state()==QProcess::Running;
    }
}

void BaseServer::bitcoinProcessReadyReadStandardError()
{
    DebugClass::debugConsole("Bitcoin process output: "+QString::fromLocal8Bit(GlobalServerData::serverPrivateVariables.bitcoin.process.readAllStandardOutput()));
}

void BaseServer::bitcoinProcessReadyReadStandardOutput()
{
    DebugClass::debugConsole("Bitcoin process error output: "+QString::fromLocal8Bit(GlobalServerData::serverPrivateVariables.bitcoin.process.readAllStandardError()));
}

void BaseServer::bitcoinProcessError(QProcess::ProcessError error)
{
    DebugClass::debugConsole("Bitcoin process error: "+QString::number((int)error));
}

void BaseServer::bitcoinProcessStateChanged(QProcess::ProcessState newState)
{
    DebugClass::debugConsole("Bitcoin process have new state: "+QString::number((int)newState));
    if(newState!=QProcess::Starting && newState!=QProcess::Running)
        GlobalServerData::serverPrivateVariables.bitcoin.enabled=false;
}

//call by normal stop
void BaseServer::stop_internal_server()
{
    if(stat!=Up && stat!=InDown)
    {
        if(stat!=Down)
            DebugClass::debugConsole("Is in wrong stat for stopping: "+QString::number((int)stat));
        return;
    }
    DebugClass::debugConsole("Try stop");
    stat=InDown;

    QSetIterator<Client *> i(client_list);
     while (i.hasNext())
         i.next()->disconnectClient();
    QFakeServer::server.disconnectedSocket();
    QFakeServer::server.close();

    check_if_now_stopped();
}

/////////////////////////////////////// player related //////////////////////////////////////
ClientMapManagement * BaseServer::getClientMapManagement()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
            return new MapVisibilityAlgorithm_Simple();
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
            return new MapVisibilityAlgorithm_WithBorder();
        break;
        case MapVisibilityAlgorithmSelection_None:
            return new MapVisibilityAlgorithm_None();
        break;
    }
}


/////////////////////////////////////////////////// Object removing /////////////////////////////////////

void BaseServer::removeOneClient()
{
    Client *client=qobject_cast<Client *>(QObject::sender());
    if(client==NULL)
    {
        DebugClass::debugConsole("removeOneClient(): NULL client at disconnection");
        return;
    }
    client_list.remove(client);
    client->deleteLater();
}

/////////////////////////////////////// player related //////////////////////////////////////

void BaseServer::newConnection()
{
    while(QFakeServer::server.hasPendingConnections())
    {
        QFakeSocket *socket = QFakeServer::server.nextPendingConnection();
        if(socket!=NULL)
        {
            DebugClass::debugConsole(QStringLiteral("newConnection(): new client connected by fake socket"));
            connect_the_last_client(new Client(new ConnectedSocket(socket),false,getClientMapManagement()));
        }
        else
            DebugClass::debugConsole("NULL client at BaseServer::newConnection()");
    }
}

void BaseServer::connect_the_last_client(Client * client)
{
    client_list << client;
    connect(client,SIGNAL(isReadyToDelete()),this,SLOT(removeOneClient()),Qt::QueuedConnection);
}

bool BaseServer::isListen()
{
    return (stat==Up);
}

bool BaseServer::isStopped()
{
    return (stat==Down);
}

void BaseServer::stop()
{
    emit try_stop_server();
}

void BaseServer::load_clan_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `id` FROM `clan` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
    }
    QSqlQuery maxClanIdQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!maxClanIdQuery.exec(queryText))
        DebugClass::debugConsole(maxClanIdQuery.lastQuery()+": "+maxClanIdQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.maxClanId=0;
    while(maxClanIdQuery.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxClanId=maxClanIdQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxClanId=0;
            continue;
        }
    }
}

void BaseServer::load_account_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `id` FROM `account` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 0,1;");
        break;
    }
    QSqlQuery maxAccountIdQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!maxAccountIdQuery.exec(queryText))
        DebugClass::debugConsole(maxAccountIdQuery.lastQuery()+": "+maxAccountIdQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.maxAccountId=0;
    while(maxAccountIdQuery.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxAccountId=maxAccountIdQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxAccountId=0;
            continue;
        }
    }
}

void BaseServer::load_character_max_id()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `id` FROM `character` ORDER BY `id` DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 0,1;");
        break;
    }
    QSqlQuery maxCharacterIdQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!maxCharacterIdQuery.exec(queryText))
        DebugClass::debugConsole(maxCharacterIdQuery.lastQuery()+QLatin1String(": ")+maxCharacterIdQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.maxCharacterId=0;
    while(maxCharacterIdQuery.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxCharacterId=maxCharacterIdQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxCharacterId=0;
            continue;
        }
    }
}
