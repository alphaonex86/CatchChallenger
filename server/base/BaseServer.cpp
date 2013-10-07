#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DatapackGeneralLoader.h"

#include <QFile>
#include <QByteArray>
#include <QDateTime>

using namespace CatchChallenger;

BaseServer::BaseServer()
{
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

    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.tolerantMode                           = false;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;

    GlobalServerData::serverSettings.database.type                              = CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    GlobalServerData::serverSettings.database.sqlite.file                       = "";
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = CatchChallenger::MapVisibilityAlgorithm_none;

    GlobalServerData::serverSettings.datapack_basePath                          = QCoreApplication::applicationDirPath()+"/datapack/";
    GlobalServerData::serverSettings.rates_gold_premium                         = 1;
    GlobalServerData::serverSettings.rates_shiny_premium                        = 1;
    GlobalServerData::serverSettings.rates_xp_premium                           = 1;
    GlobalServerData::serverSettings.server_ip                                  = "";
    GlobalServerData::serverSettings.server_port                                = 42489;
    GlobalServerData::serverSettings.compressionType                            = CompressionType_Zlib;
    CommonSettings::commonSettings.chat_allow_aliance     = true;
    CommonSettings::commonSettings.chat_allow_clan        = true;
    CommonSettings::commonSettings.chat_allow_local       = true;
    CommonSettings::commonSettings.chat_allow_all         = true;
    CommonSettings::commonSettings.chat_allow_private     = true;
    CommonSettings::commonSettings.pvp                    = true;
    CommonSettings::commonSettings.rates_gold             = 1;
    CommonSettings::commonSettings.rates_shiny            = 1;
    CommonSettings::commonSettings.rates_xp               = 1;
    CommonSettings::commonSettings.factoryPriceChange     = 20;
    GlobalServerData::serverSettings.database.type                              = ServerSettings::Database::DatabaseType_Mysql;
    GlobalServerData::serverSettings.database.fightSync                         = ServerSettings::Database::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.database.positionTeleportSync              = true;
    GlobalServerData::serverSettings.database.secondToPositionSync              = 0;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = MapVisibilityAlgorithm_simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max                   = 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow                = 20;
    GlobalServerData::serverSettings.city.capture.frenquency                    = City::Capture::Frequency_week;
    GlobalServerData::serverSettings.city.capture.day                           = City::Capture::Monday;
    GlobalServerData::serverSettings.city.capture.hour                          = 0;
    GlobalServerData::serverSettings.city.capture.minute                        = 0;
    GlobalServerData::serverSettings.bitcoin.address                            = "1Hz3GtkiDBpbWxZixkQPuTGDh2DUy9bQUJ";
    #ifdef Q_OS_WIN32
    GlobalServerData::serverSettings.bitcoin.binaryPath                         = "%application_path%/bitcoin/bitcoind.exe";
    GlobalServerData::serverSettings.bitcoin.workingPath                        = "%application_path%/bitcoin-storage/";
    #else
    GlobalServerData::serverSettings.bitcoin.binaryPath                         = "/usr/bin/bitcoind";
    GlobalServerData::serverSettings.bitcoin.workingPath                        = QDir::homePath()+"/.config/CatchChallenger/server/bitoin/";
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
    GlobalServerData::serverPrivateVariables.stopIt=false;

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
}

void BaseServer::preload_zone()
{
    //open and quick check the file
    QFileInfoList entryList=QDir(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot|QDir::Hidden|QDir::System,QDir::DirsFirst|QDir::Name|QDir::IgnoreCase);
    int index=0;
    while(index<entryList.size())
    {
        if(!entryList.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryList.at(index).fileName().contains(QRegularExpression("^[a-zA-Z0-9\\- _]+\\.xml$")))
        {
            qDebug() << QString("%1 the zone file name not match").arg(entryList.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryList.at(index).fileName();
        zoneCodeName.remove(QRegularExpression("\\.xml$"));
        QFile itemsFile(entryList.at(index).absoluteFilePath());
        QByteArray xmlContent;
        if(!itemsFile.open(QIODevice::ReadOnly))
        {
            qDebug() << QString("Unable to open the file: %1, error: %2").arg(itemsFile.fileName()).arg(itemsFile.errorString());
            index++;
            continue;
        }
        xmlContent=itemsFile.readAll();
        itemsFile.close();
        QDomDocument domDocument;
        QString errorStr;
        int errorLine,errorColumn;
        if(!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QString("Unable to open the file: %1, Parse error at line %2, column %3: %4").arg(itemsFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            index++;
            continue;
        }
        if(GlobalServerData::serverPrivateVariables.captureFightIdList.contains(zoneCodeName))
        {
            qDebug() << QString("Unable to open the file: %1, zone code name already found");
            index++;
            continue;
        }
        QDomElement root = domDocument.documentElement();
        if(root.tagName()!="zone")
        {
            qDebug() << QString("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(itemsFile.fileName());
            index++;
            continue;
        }

        //load capture
        QList<quint32> fightIdList;
        QDomElement capture = root.firstChildElement("capture");
        if(!capture.isNull())
        {
            if(capture.isElement() && capture.hasAttribute("fightId"))
            {
                bool ok;
                const QStringList &fightIdStringList=capture.attribute("fightId").split(";");
                int sub_index=0;
                while(sub_index<fightIdStringList.size())
                {
                    quint32 fightId=fightIdStringList.at(sub_index).toUInt(&ok);
                    if(ok)
                    {
                        if(!CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightId))
                            qDebug() << QString("bot fightId %1 not found for capture zone %2").arg(fightId).arg(zoneCodeName);
                        else
                            fightIdList << fightId;
                    }
                    sub_index++;
                }
                if(sub_index==fightIdStringList.size() && !fightIdList.isEmpty())
                    GlobalServerData::serverPrivateVariables.captureFightIdList[zoneCodeName]=fightIdList;
                break;
            }
            else
                qDebug() << QString("Unable to open the file: %1, is not an element: child.tagName(): %2 (at line: %3)").arg(itemsFile.fileName()).arg(capture.tagName()).arg(capture.lineNumber());
        }
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QString("SELECT clan FROM city WHERE city='%1';").arg(zoneCodeName);
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QString("SELECT clan FROM city WHERE city='%1';").arg(zoneCodeName);
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
                DebugClass::debugConsole(QString("clan id is failed to convert to number for city status"));
        }
        index++;
    }

    qDebug() << QString("%1 zone(s) loaded").arg(GlobalServerData::serverPrivateVariables.captureFightIdList.size());
}

void BaseServer::preload_industries()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,resources,products,last_update FROM factory");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,resources,products,last_update FROM factory");
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
            DebugClass::debugConsole(QString("preload_industries: id is not a number"));
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.industriesLink.contains(id))
            {
                ok=false;
                DebugClass::debugConsole(QString("preload_industries: industries link not found"));
            }
        }
        if(ok)
        {
            QStringList resourcesStringList=industryStatusQuery.value(1).toString().split(";");
            int index=0;
            while(index<resourcesStringList.size())
            {
                QStringList itemStringList=resourcesStringList.at(index).split("->");
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QString("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                quint32 item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QString("preload_industries: item is not a number"));
                    break;
                }
                quint32 quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QString("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.resources.contains(item))
                {
                    DebugClass::debugConsole(QString("preload_industries: item already set"));
                    ok=false;
                    break;
                }
                const Industry &industry=CommonDatapack::commonDatapack.industries[CommonDatapack::commonDatapack.industriesLink[id]];
                int indexItem=0;
                while(indexItem<industry.resources.size())
                {
                    if(industry.resources.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==industry.resources.size())
                {
                    DebugClass::debugConsole(QString("preload_industries: item in db not found"));
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
            QStringList productsStringList=industryStatusQuery.value(2).toString().split(";");
            int index=0;
            while(index<productsStringList.size())
            {
                QStringList itemStringList=productsStringList.at(index).split("->");
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QString("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                quint32 item=itemStringList.first().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QString("preload_industries: item is not a number"));
                    break;
                }
                quint32 quantity=itemStringList.last().toUInt(&ok);
                if(!ok)
                {
                    DebugClass::debugConsole(QString("preload_industries: quantity is not a number"));
                    break;
                }
                if(industryStatus.products.contains(item))
                {
                    DebugClass::debugConsole(QString("preload_industries: item already set"));
                    ok=false;
                    break;
                }const Industry &industry=CommonDatapack::commonDatapack.industries[CommonDatapack::commonDatapack.industriesLink[id]];
                int indexItem=0;
                while(indexItem<industry.products.size())
                {
                    if(industry.products.at(indexItem).item==item)
                        break;
                    indexItem++;
                }
                if(indexItem==industry.products.size())
                {
                    DebugClass::debugConsole(QString("preload_industries: item in db not found"));
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
                DebugClass::debugConsole(QString("preload_industries: last_update is not a number"));
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
                    queryText=QString("DELETE FROM industries WHERE id='%1'").arg(industryStatusQuery.value(0).toString());
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QString("DELETE FROM industries WHERE id='%1'").arg(industryStatusQuery.value(0).toString());
                break;
            }
            QSqlQuery industryDeleteQuery(*GlobalServerData::serverPrivateVariables.db);
            if(!industryDeleteQuery.exec(queryText))
                DebugClass::debugConsole(industryDeleteQuery.lastQuery()+": "+industryDeleteQuery.lastError().text());
        }
    }
    qDebug() << QString("%1 industrie(s) status loaded").arg(GlobalServerData::serverPrivateVariables.industriesStatus.size());
}

void BaseServer::preload_market_monsters()
{    QString queryText;
     switch(GlobalServerData::serverSettings.database.type)
     {
         default:
         case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QString("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,player,market_price,market_bitcoin FROM monster WHERE place='market' ORDER BY position ASC");
         break;
         case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,player,market_price,market_bitcoin FROM monster WHERE place='market' ORDER BY position ASC");
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
             DebugClass::debugConsole(QString("monsterId: %1 is not a number").arg(monstersQuery.value(0).toString()));
         if(ok)
         {
             playerMonster.monster=monstersQuery.value(2).toUInt(&ok);
             if(ok)
             {
                 if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                 {
                     ok=false;
                     DebugClass::debugConsole(QString("monster: %1 is not into monster list").arg(playerMonster.monster));
                 }
             }
             else
                 DebugClass::debugConsole(QString("monster: %1 is not a number").arg(monstersQuery.value(2).toString()));
         }
         if(ok)
         {
             playerMonster.level=monstersQuery.value(3).toUInt(&ok);
             if(ok)
             {
                 if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                 {
                     DebugClass::debugConsole(QString("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                     playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                 }
             }
             else
                 DebugClass::debugConsole(QString("level: %1 is not a number").arg(monstersQuery.value(3).toString()));
         }
         if(ok)
         {
             playerMonster.remaining_xp=monstersQuery.value(4).toUInt(&ok);
             if(ok)
             {
                 if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters[playerMonster.monster].level_to_xp.at(playerMonster.level-1))
                 {
                     DebugClass::debugConsole(QString("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(CommonDatapack::commonDatapack.monsters[playerMonster.monster].level_to_xp.at(playerMonster.level-1)));
                     playerMonster.remaining_xp=0;
                 }
             }
             else
                 DebugClass::debugConsole(QString("monster xp: %1 is not a number").arg(monstersQuery.value(4).toString()));
         }
         if(ok)
         {
             playerMonster.sp=monstersQuery.value(5).toUInt(&ok);
             if(!ok)
                 DebugClass::debugConsole(QString("monster sp: %1 is not a number").arg(monstersQuery.value(5).toString()));
         }
         if(ok)
         {
             playerMonster.captured_with=monstersQuery.value(6).toUInt(&ok);
             if(ok)
             {
                 if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.captured_with))
                     DebugClass::debugConsole(QString("captured_with: %1 is not is not into items list").arg(playerMonster.captured_with));
             }
             else
                 DebugClass::debugConsole(QString("captured_with: %1 is not a number").arg(monstersQuery.value(6).toString()));
         }
         if(ok)
         {
             if(monstersQuery.value(7).toString()=="male")
                 playerMonster.gender=Gender_Male;
             else if(monstersQuery.value(7).toString()=="female")
                 playerMonster.gender=Gender_Female;
             else if(monstersQuery.value(7).toString()=="unknown")
                 playerMonster.gender=Gender_Unknown;
             else
             {
                 playerMonster.gender=Gender_Unknown;
                 DebugClass::debugConsole(QString("unknown monster gender: %1").arg(monstersQuery.value(7).toString()));
                 ok=false;
             }
         }
         if(ok)
         {
             playerMonster.egg_step=monstersQuery.value(8).toUInt(&ok);
             if(!ok)
                 DebugClass::debugConsole(QString("monster egg_step: %1 is not a number").arg(monstersQuery.value(8).toString()));
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
                 const Monster::Stat &stat=CommonFightEngine::getStat(CommonDatapack::commonDatapack.monsters[playerMonster.monster],playerMonster.level);
                 if(playerMonster.hp>stat.hp)
                 {
                     DebugClass::debugConsole(QString("monster hp: %1 greater than max hp %2 for the level %3 of the monster %4, truncated")
                                  .arg(playerMonster.hp)
                                  .arg(stat.hp)
                                  .arg(playerMonster.level)
                                  .arg(playerMonster.monster)
                                  );
                     playerMonster.hp=stat.hp;
                 }
             }
             else
                 DebugClass::debugConsole(QString("monster hp: %1 is not a number").arg(monstersQuery.value(1).toString()));
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
            queryText=QString("SELECT item_id,quantity,player_id,market_price,market_bitcoin FROM item WHERE place='market'");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT item_id,quantity,player_id,market_price,market_bitcoin FROM item WHERE place='market'");
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
            DebugClass::debugConsole(QString("item id is not a number, skip"));
            continue;
        }
        marketItem.quantity=itemQuery.value(1).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("quantity is not a number, skip"));
            continue;
        }
        marketItem.player=itemQuery.value(2).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("player id is not a number, skip"));
            continue;
        }
        marketItem.cash=itemQuery.value(3).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("cash is not a number, skip"));
            continue;
        }
        marketItem.bitcoin=itemQuery.value(4).toDouble(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("bitcoin is not a number, skip"));
            continue;
        }
        if(LocalClientHandler::marketObjectIdList.isEmpty())
        {
            DebugClass::debugConsole(QString("not more marketObjectId into the list, skip"));
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
            queryText=QString("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(monsterId);
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
                DebugClass::debugConsole(QString("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
            else if(CommonDatapack::commonDatapack.monsterBuffs[buff.buff].duration!=Buff::Duration_Always)
            {
                ok=false;
                DebugClass::debugConsole(QString("buff %1 for monsterId: %2 can't be loaded from the db if is not permanent").arg(buff.buff).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QString("buff id: %1 is not a number").arg(monsterBuffsQuery.value(0).toString()));
        if(ok)
        {
            buff.level=monsterBuffsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(buff.level>CommonDatapack::commonDatapack.monsterBuffs[buff.buff].level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QString("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                DebugClass::debugConsole(QString("buff level: %1 is not a number").arg(monsterBuffsQuery.value(2).toString()));
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
            queryText=QString("SELECT skill,level FROM monster_skill WHERE monster=%1").arg(monsterId);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT skill,level FROM monster_skill WHERE monster=%1").arg(monsterId);
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
                DebugClass::debugConsole(QString("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QString("skill id: %1 is not a number").arg(monsterSkillsQuery.value(0).toString()));
        if(ok)
        {
            skill.level=monsterSkillsQuery.value(1).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills[skill.skill].level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QString("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                DebugClass::debugConsole(QString("skill level: %1 is not a number").arg(monsterSkillsQuery.value(2).toString()));
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
    DebugClass::debugConsole(QString("start preload the map, into: %1").arg(GlobalServerData::serverPrivateVariables.datapack_mapPath));
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
    QRegularExpression mapFilter("\\.tmx$");
    while(index<size)
    {
        QString fileName=returnList.at(index);
        fileName.replace('\\','/');
        if(fileName.contains(mapFilter) && GlobalServerData::serverPrivateVariables.filesList.contains(DATAPACK_BASE_PATH_MAP+fileName))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(QString("load the map: %1").arg(fileName));
            #endif
            full_map_name << fileName;
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithm_simple:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new Map_server_MapVisibility_simple;
                    break;
                    case MapVisibilityAlgorithm_none:
                    default:
                        GlobalServerData::serverPrivateVariables.map_list[fileName]=new MapServer;
                    break;
                }

                GlobalServerData::serverPrivateVariables.map_list[fileName]->width			= map_temp.map_to_send.width;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->height			= map_temp.map_to_send.height;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->parsed_layer	= map_temp.map_to_send.parsed_layer;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->map_file			= fileName;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->grassMonster     = map_temp.map_to_send.grassMonster;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->waterMonster     = map_temp.map_to_send.waterMonster;
                GlobalServerData::serverPrivateVariables.map_list[fileName]->caveMonster     = map_temp.map_to_send.caveMonster;

                map_name << fileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list[fileName];

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
                while(sub_index<map_temp.map_to_send.teleport.size())
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    sub_index++;
                }

                map_semi.old_map=map_temp.map_to_send;

                semi_loaded_map << map_semi;
            }
            else
                DebugClass::debugConsole(QString("error at loading: %1, error: %2").arg(fileName).arg(map_temp.errorString()));
        }
        index++;
    }
    full_map_name.sort();

    //resolv the border map name into their pointer
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(semi_loaded_map.at(index).border.bottom.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.bottom.fileName))
            semi_loaded_map[index].map->border.bottom.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.bottom.fileName];
        else
            semi_loaded_map[index].map->border.bottom.map=NULL;

        if(semi_loaded_map.at(index).border.top.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.top.fileName))
            semi_loaded_map[index].map->border.top.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.top.fileName];
        else
            semi_loaded_map[index].map->border.top.map=NULL;

        if(semi_loaded_map.at(index).border.left.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.left.fileName))
            semi_loaded_map[index].map->border.left.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.left.fileName];
        else
            semi_loaded_map[index].map->border.left.map=NULL;

        if(semi_loaded_map.at(index).border.right.fileName!="" && GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map.at(index).border.right.fileName))
            semi_loaded_map[index].map->border.right.map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map.at(index).border.right.fileName];
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
        while(sub_index<semi_loaded_map[index].old_map.teleport.size())
        {
            if(GlobalServerData::serverPrivateVariables.map_list.contains(semi_loaded_map[index].old_map.teleport.at(sub_index).map))
            {
                if(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->width
                        && semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map]->height)
                {
                    int virtual_position=semi_loaded_map[index].old_map.teleport.at(sub_index).source_x+semi_loaded_map[index].old_map.teleport.at(sub_index).source_y*semi_loaded_map[index].map->width;
                    if(semi_loaded_map[index].map->teleporter.contains(virtual_position))
                    {
                        DebugClass::debugConsole(QString("already found teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                             .arg(semi_loaded_map[index].map->map_file)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                             .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
                    }
                    else
                    {
                        #ifdef DEBUG_MESSAGE_MAP_LOAD
                        DebugClass::debugConsole(QString("teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                                     .arg(semi_loaded_map[index].map->map_file)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
                        #endif
                        semi_loaded_map[index].map->teleporter[virtual_position].map=GlobalServerData::serverPrivateVariables.map_list[semi_loaded_map[index].old_map.teleport.at(sub_index).map];
                        semi_loaded_map[index].map->teleporter[virtual_position].x=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x;
                        semi_loaded_map[index].map->teleporter[virtual_position].y=semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y;
                    }
                }
                else
                    DebugClass::debugConsole(QString("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the tp is out of range")
                         .arg(semi_loaded_map[index].map->map_file)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                         .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));
            }
            else
                DebugClass::debugConsole(QString("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the map is not found")
                     .arg(semi_loaded_map[index].map->map_file)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_x)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).source_y)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).map)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_x)
                     .arg(semi_loaded_map[index].old_map.teleport.at(sub_index).destination_y));

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
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have not a top map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have bottom map: %2, the map %2 have this top map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map->border.top.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have not a bottom map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have top map: %2, the map %2 have this bottom map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map->border.bottom.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have not a right map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have left map: %2, the map %2 have this right map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map->border.right.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map=NULL;
        }
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map!=GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)])
        {
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map==NULL)
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have not a left map").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file));
            else
                DebugClass::debugConsole(QString("the map %1 have right map: %2, the map %2 have this left map: %3").arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->map_file).arg(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map->border.left.map->map_file));
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map=NULL;
        }
        index++;
    }

    //resolv the near map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)];

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL && !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map;
            if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL &&  !GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map.contains(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map))
                GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->near_map << GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map;
        }

        index++;
    }

    //resolv the offset to change of map
    size=semi_loaded_map.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=semi_loaded_map.at(index).border.bottom.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.bottom.fileName)).border.top.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.bottom.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=semi_loaded_map.at(index).border.top.x_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.top.fileName)).border.bottom.x_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.top.x_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=semi_loaded_map.at(index).border.left.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.left.fileName)).border.right.y_offset;
        else
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.left.y_offset=0;
        if(GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.map!=NULL)
            GlobalServerData::serverPrivateVariables.map_list[map_name.at(index)]->border.right.y_offset=semi_loaded_map.at(index).border.right.y_offset-semi_loaded_map.at(map_name.indexOf(semi_loaded_map.at(index).border.right.fileName)).border.left.y_offset;
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
        while(sub_index<semi_loaded_map[index].old_map.rescue_points.size())
        {
            const Map_to_send::Map_Point &point=semi_loaded_map[index].old_map.rescue_points.at(sub_index);
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

    DebugClass::debugConsole(QString("%1 map(s) loaded").arg(GlobalServerData::serverPrivateVariables.map_list.size()));

    botFiles.clear();
}

void BaseServer::preload_the_skin()
{
    QStringList skinFolderList=FacilityLib::skinIdList(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_SKIN);
    int index=0;
    while(index<skinFolderList.size())
    {
        GlobalServerData::serverPrivateVariables.skinList[skinFolderList.at(index)]=index;
        index++;
    }

    DebugClass::debugConsole(QString("%1 skin(s) loaded").arg(GlobalServerData::serverPrivateVariables.skinList.size()));
}

void BaseServer::preload_the_datapack()
{
    QStringList extensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_ALLOWED+QString(";")+CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    QSet<QString> extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(";");
    ClientHeavyLoad::compressedExtension=compressedExtensionAllowedTemp.toSet();
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverSettings.datapack_basePath);
    int index=0;
    int size=returnList.size();
    while(index<size)
    {
        QString fileName=returnList.at(index);
        if(fileName.contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            if(!QFileInfo(fileName).suffix().isEmpty() && extensionAllowed.contains(QFileInfo(fileName).suffix()))
            {
                QFile file(GlobalServerData::serverSettings.datapack_basePath+returnList.at(index));
                if(file.size()<=8*1024*1024)
                {
                    if(file.open(QIODevice::ReadOnly))
                    {
                        fileName.replace("\\","/");//remplace if is under windows server
                        GlobalServerData::serverPrivateVariables.filesList[fileName]=QFileInfo(file).lastModified().toTime_t();
                        file.close();
                    }
                }
            }
        }
        index++;
    }
    DebugClass::debugConsole(QString("%1 file(s) list for datapack cached").arg(GlobalServerData::serverPrivateVariables.filesList.size()));
}

void BaseServer::preload_the_players()
{
    ClientHeavyLoad::simplifiedIdList.clear();
    int index=0;
    while(index<GlobalServerData::serverSettings.max_players)
    {
        ClientHeavyLoad::simplifiedIdList << index;
        index++;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    QHash<QString,Map *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithm_simple:
        while (i != i_end)
        {
            static_cast<Map_server_MapVisibility_simple *>(i.value())->show=true;
            i++;
        }
        break;
        case MapVisibilityAlgorithm_none:
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
    int size=semi_loaded_map.size();
    int index=0;
    bool ok;
    while(index<size)
    {
        int sub_index=0;
        while(sub_index<semi_loaded_map[index].old_map.bots.size())
        {
            bots_number++;
            Map_to_send::Bot_Semi bot_Semi=semi_loaded_map[index].old_map.bots.at(sub_index);
            loadBotFile(bot_Semi.file);
            if(botFiles.contains(bot_Semi.file))
                if(botFiles[bot_Semi.file].contains(bot_Semi.id))
                {
                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                    CatchChallenger::DebugClass::debugConsole(QString("Bot %1 (%2) at %3 (%4,%5)").arg(bot_Semi.file).arg(bot_Semi.id).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                    #endif
                    QHashIterator<quint8,QDomElement> i(botFiles[bot_Semi.file][bot_Semi.id].step);
                    while (i.hasNext()) {
                        i.next();
                        QDomElement step = i.value();
                        if(step.attribute("type")=="shop")
                        {
                            if(!step.hasAttribute("shop"))
                                CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"shop\": for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 shop=step.attribute("shop").toUInt(&ok);
                                if(!ok)
                                    CatchChallenger::DebugClass::debugConsole(QString("shop is not a number: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else if(!GlobalServerData::serverPrivateVariables.shops.contains(shop))
                                    CatchChallenger::DebugClass::debugConsole(QString("shop number is not valid shop: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                        .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                                else
                                {
                                    #ifdef DEBUG_MESSAGE_MAP_LOAD
                                    CatchChallenger::DebugClass::debugConsole(QString("shop put at: %1 (%2,%3)")
                                        .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    #endif
                                    static_cast<MapServer *>(semi_loaded_map[index].map)->shops.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),shop);
                                    shops_number++;
                                }
                            }
                        }
                        else if(step.attribute("type")=="learn")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->learn.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("learn point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("learn point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->learn.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                learnpoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="heal")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->heal.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("heal point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("heal point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->heal.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                healpoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="market")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->market.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("market point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("market point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->market.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y));
                                marketpoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="zonecapture")
                        {
                            if(!step.hasAttribute("zone"))
                                CatchChallenger::DebugClass::debugConsole(QString("zonecapture point have not the zone attribute: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else if(static_cast<MapServer *>(semi_loaded_map[index].map)->zonecapture.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("zonecapture point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                #ifdef DEBUG_MESSAGE_MAP_LOAD
                                CatchChallenger::DebugClass::debugConsole(QString("zonecapture point put at: %1 (%2,%3)")
                                    .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                #endif
                                static_cast<MapServer *>(semi_loaded_map[index].map)->zonecapture[QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)]=step.attribute("zone");
                                zonecapturepoint_number++;
                            }
                        }
                        else if(step.attribute("type")=="fight")
                        {
                            if(static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.contains(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y)))
                                CatchChallenger::DebugClass::debugConsole(QString("botsFight point already on the map: for bot id: %1 (%2), spawn at: %3 (%4,%5), for step: %6")
                                    .arg(bot_Semi.id).arg(bot_Semi.file).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(i.key()));
                            else
                            {
                                quint32 fightid=step.attribute("fightid").toUInt(&ok);
                                if(ok)
                                {
                                    if(CommonDatapack::commonDatapack.botFights.contains(fightid))
                                    {
                                        if(bot_Semi.property_text.contains("lookAt"))
                                        {
                                            Direction direction;
                                            if(bot_Semi.property_text["lookAt"]=="left")
                                                direction=CatchChallenger::Direction_move_at_left;
                                            else if(bot_Semi.property_text["lookAt"]=="right")
                                                direction=CatchChallenger::Direction_move_at_right;
                                            else if(bot_Semi.property_text["lookAt"]=="top")
                                                direction=CatchChallenger::Direction_move_at_top;
                                            else
                                            {
                                                if(bot_Semi.property_text["lookAt"]!="bottom")
                                                    CatchChallenger::DebugClass::debugConsole(QString("Wrong direction for the bot at %1 (%2,%3)")
                                                        .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                                direction=CatchChallenger::Direction_move_at_bottom;
                                            }
                                            #ifdef DEBUG_MESSAGE_MAP_LOAD
                                            CatchChallenger::DebugClass::debugConsole(QString("botsFight point put at: %1 (%2,%3)")
                                                .arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                            #endif
                                            static_cast<MapServer *>(semi_loaded_map[index].map)->botsFight.insert(QPair<quint8,quint8>(bot_Semi.point.x,bot_Semi.point.y),fightid);
                                            botfights_number++;

                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            CatchChallenger::DebugClass::debugConsole(QString("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(direction));
                                            #endif
                                            quint8 temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            int index_botfight_range=0;
                                            CatchChallenger::Map *map=semi_loaded_map[index].map;
                                            CatchChallenger::Map *old_map=map;
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
                                            DebugClass::debugConsole(QString("lookAt not found: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                    }
                                    else
                                        DebugClass::debugConsole(QString("fightid not found into the list: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                                }
                                else
                                    DebugClass::debugConsole(QString("botsFight point have wrong fightid: %1 at %2(%3,%4)").arg(shops_number).arg(semi_loaded_map[index].map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y));
                            }
                        }
                    }
                }
            sub_index++;
        }
        index++;
    }

    DebugClass::debugConsole(QString("%1 learn point(s) on map loaded").arg(learnpoint_number));
    DebugClass::debugConsole(QString("%1 zonecapture point(s) on map loaded").arg(zonecapturepoint_number));
    DebugClass::debugConsole(QString("%1 heal point(s) on map loaded").arg(healpoint_number));
    DebugClass::debugConsole(QString("%1 market point(s) on map loaded").arg(marketpoint_number));
    DebugClass::debugConsole(QString("%1 bot fight(s) on map loaded").arg(botfights_number));
    DebugClass::debugConsole(QString("%1 bot fights tigger(s) on map loaded").arg(botfightstigger_number));
    DebugClass::debugConsole(QString("%1 shop(s) on map loaded").arg(shops_number));
    DebugClass::debugConsole(QString("%1 bots(s) on map loaded").arg(bots_number));
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
        DebugClass::debugConsole(QString("Disconnected to %1 at %2 (%3), previous connection: %4")
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
        DebugClass::debugConsole(QString("database type unknow"));
        return false;
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QMYSQL","server");
        GlobalServerData::serverPrivateVariables.db->setConnectOptions("MYSQL_OPT_RECONNECT=1");
        GlobalServerData::serverPrivateVariables.db->setHostName(GlobalServerData::serverSettings.database.mysql.host);
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.mysql.db);
        GlobalServerData::serverPrivateVariables.db->setUserName(GlobalServerData::serverSettings.database.mysql.login);
        GlobalServerData::serverPrivateVariables.db->setPassword(GlobalServerData::serverSettings.database.mysql.pass);
        GlobalServerData::serverPrivateVariables.db_type_string="mysql";
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalServerData::serverPrivateVariables.db = new QSqlDatabase();
        *GlobalServerData::serverPrivateVariables.db = QSqlDatabase::addDatabase("QSQLITE","server");
        GlobalServerData::serverPrivateVariables.db->setDatabaseName(GlobalServerData::serverSettings.database.sqlite.file);
        GlobalServerData::serverPrivateVariables.db_type_string="sqlite";
        break;
    }
    if(!GlobalServerData::serverPrivateVariables.db->open())
    {
        DebugClass::debugConsole(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        emit error(QString("Unable to connect to the database: %1, with the login: %2, database text: %3").arg(GlobalServerData::serverPrivateVariables.db->lastError().driverText()).arg(GlobalServerData::serverSettings.database.mysql.login).arg(GlobalServerData::serverPrivateVariables.db->lastError().databaseText()));
        return false;
    }
    DebugClass::debugConsole(QString("Connected to %1 at %2 (%3)")
                             .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                             .arg(GlobalServerData::serverSettings.database.mysql.host)
                             .arg(GlobalServerData::serverPrivateVariables.db->isOpen()));
    return true;
}

void BaseServer::loadBotFile(const QString &fileName)
{
    if(botFiles.contains(fileName))
        return;
    botFiles[fileName];//create the entry
    QFile mapFile(fileName);
    if(!mapFile.open(QIODevice::ReadOnly))
    {
        qDebug() << mapFile.fileName()+": "+mapFile.errorString();
        return;
    }
    QByteArray xmlContent=mapFile.readAll();
    mapFile.close();
    QDomDocument domDocument;
    QString errorStr;
    int errorLine,errorColumn;
    if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
    {
        qDebug() << QString("%1, Parse error at line %2, column %3: %4").arg(mapFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
        return;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!="bots")
    {
        qDebug() << QString("\"bots\" root balise not found for the xml file");
        return;
    }
    //load the bots
    QDomElement child = root.firstChildElement("bot");
    while(!child.isNull())
    {
        if(!child.hasAttribute("id"))
            CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"id\": child.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        else if(!child.isElement())
            CatchChallenger::DebugClass::debugConsole(QString("Is not an element: child.tagName(): %1, name: %2 (at line: %3)").arg(child.tagName().arg(child.attribute("name")).arg(child.lineNumber())));
        else
        {
            quint32 id=child.attribute("id").toUInt(&ok);
            if(ok)
            {
                botFiles[fileName][id];
                QDomElement step = child.firstChildElement("step");
                while(!step.isNull())
                {
                    if(!step.hasAttribute("id"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.hasAttribute("type"))
                        CatchChallenger::DebugClass::debugConsole(QString("Has not attribute \"type\": bot.tagName(): %1 (at line: %2)").arg(step.tagName()).arg(step.lineNumber()));
                    else if(!step.isElement())
                        CatchChallenger::DebugClass::debugConsole(QString("Is not an element: bot.tagName(): %1, type: %2 (at line: %3)").arg(step.tagName().arg(step.attribute("type")).arg(step.lineNumber())));
                    else
                    {
                        quint32 stepId=step.attribute("id").toUInt(&ok);
                        if(ok)
                            botFiles[fileName][id].step[stepId]=step;
                    }
                    step = step.nextSiblingElement("step");
                }
                if(!botFiles[fileName][id].step.contains(1))
                    botFiles[fileName].remove(id);
            }
            else
                CatchChallenger::DebugClass::debugConsole(QString("Attribute \"id\" is not a number: bot.tagName(): %1 (at line: %2)").arg(child.tagName()).arg(child.lineNumber()));
        }
        child = child.nextSiblingElement("bot");
    }
}

////////////////////////////////////////////////// server stopping ////////////////////////////////////////////

void BaseServer::unload_the_data()
{
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
    QHash<QString,Map *>::const_iterator i = GlobalServerData::serverPrivateVariables.map_list.constBegin();
    QHash<QString,Map *>::const_iterator i_end = GlobalServerData::serverPrivateVariables.map_list.constEnd();
    while (i != i_end)
    {
        Map::removeParsedLayer(i.value()->parsed_layer);
        delete i.value();
        i++;
    }
    GlobalServerData::serverPrivateVariables.map_list.clear();
    GlobalServerData::serverPrivateVariables.botSpawn.clear();
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
    GlobalServerData::serverPrivateVariables.filesList.clear();
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
        DebugClass::debugConsole(QString("Disconnected to %1 at %2 (%3)")
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

    if(GlobalServerData::serverSettings.mapVisibility.simple.max>GlobalServerData::serverSettings.max_players)
        GlobalServerData::serverSettings.mapVisibility.simple.max=GlobalServerData::serverSettings.max_players;

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
        case CatchChallenger::MapVisibilityAlgorithm_none:
        case CatchChallenger::MapVisibilityAlgorithm_simple:
        break;
        default:
            GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm=CatchChallenger::MapVisibilityAlgorithm_simple;
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
                                                                       << QString("-datadir=%1").arg(GlobalServerData::serverSettings.bitcoin.workingPath)
                                                                       << QString("-port=%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                       << QString("-bind=127.0.0.1:%1").arg(GlobalServerData::serverSettings.bitcoin.port)
                                                                       << QString("-rpcport=%1").arg(GlobalServerData::serverSettings.bitcoin.port+1)
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
        case MapVisibilityAlgorithm_simple:
            return new MapVisibilityAlgorithm_Simple();
        break;
        case MapVisibilityAlgorithm_none:
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
            DebugClass::debugConsole(QString("newConnection(): new client connected by fake socket"));
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
            queryText=QString("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QString("SELECT id FROM clan ORDER BY id DESC LIMIT 0,1;");
        break;
    }
    QSqlQuery maxMonsterIdQuery(*GlobalServerData::serverPrivateVariables.db);
    if(!maxMonsterIdQuery.exec(queryText))
        DebugClass::debugConsole(maxMonsterIdQuery.lastQuery()+": "+maxMonsterIdQuery.lastError().text());
    GlobalServerData::serverPrivateVariables.maxClanId=0;
    while(maxMonsterIdQuery.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxClanId=maxMonsterIdQuery.value(0).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QString("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxClanId=0;
            continue;
        }
    }
}
