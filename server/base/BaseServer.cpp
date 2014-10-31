#include "BaseServer.h"
#include "GlobalServerData.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/DatapackGeneralLoader.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_None.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_Simple_StoreOnSender.h"
#include "ClientMapManagement/MapVisibilityAlgorithm_WithBorder_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "ClientMapManagement/Map_server_MapVisibility_WithBorder_StoreOnSender.h"
#include "LocalClientHandlerWithoutSender.h"
#include "ClientNetworkReadWithoutSender.h"
#include "SqlFunction.h"

#include <QFile>
#include <QByteArray>
#include <QDateTime>
#include <QTime>
#include <time.h>
#ifndef EPOLLCATCHCHALLENGERSERVER
#include <QTimer>
#endif

using namespace CatchChallenger;

QRegularExpression BaseServer::regexXmlFile=QRegularExpression(QLatin1String("^[a-zA-Z0-9\\- _]+\\.xml$"));
const QString BaseServer::text_dotxml=QLatin1String(".xml");
const QString BaseServer::text_zone=QLatin1String("zone");
const QString BaseServer::text_capture=QLatin1String("capture");
const QString BaseServer::text_fightId=QLatin1String("fightId");
const QString BaseServer::text_dotcomma=QLatin1String(";");
const QString BaseServer::text_male=QLatin1String("male");
const QString BaseServer::text_female=QLatin1String("female");
const QString BaseServer::text_unknown=QLatin1String("unknown");
const QString BaseServer::text_slash=QLatin1String("slash");
const QString BaseServer::text_antislash=QLatin1String("antislash");
const QString BaseServer::text_type=QLatin1String("type");
const QString BaseServer::text_shop=QLatin1String("shop");
const QString BaseServer::text_learn=QLatin1String("learn");
const QString BaseServer::text_heal=QLatin1String("heal");
const QString BaseServer::text_market=QLatin1String("market");
const QString BaseServer::text_zonecapture=QLatin1String("zonecapture");
const QString BaseServer::text_fight=QLatin1String("fight");
const QString BaseServer::text_fightid=QLatin1String("fightid");
const QString BaseServer::text_lookAt=QLatin1String("lookAt");
const QString BaseServer::text_left=QLatin1String("left");
const QString BaseServer::text_right=QLatin1String("right");
const QString BaseServer::text_top=QLatin1String("top");
const QString BaseServer::text_bottom=QLatin1String("bottom");
const QString BaseServer::text_fightRange=QLatin1String("fightRange");
const QString BaseServer::text_bots=QLatin1String("bots");
const QString BaseServer::text_bot=QLatin1String("bot");
const QString BaseServer::text_id=QLatin1String("id");
const QString BaseServer::text_name=QLatin1String("name");
const QString BaseServer::text_step=QLatin1String("step");
const QString BaseServer::text_arrow=QLatin1String("->");
const QString BaseServer::text_dottmx=QLatin1Literal(".tmx");
const QString BaseServer::text_shops=QLatin1Literal("shops");
const QString BaseServer::text_product=QLatin1Literal("product");
const QString BaseServer::text_itemId=QLatin1Literal("itemId");
const QString BaseServer::text_overridePrice=QLatin1Literal("overridePrice");
const QString BaseServer::text_list=QLatin1Literal("list");
const QString BaseServer::text_monster=QLatin1Literal("monster");
const QString BaseServer::text_monsters=QLatin1Literal("monsters");
const QString BaseServer::text_drops=QLatin1Literal("drops");
const QString BaseServer::text_drop=QLatin1Literal("drop");
const QString BaseServer::text_item=QLatin1Literal("item");
const QString BaseServer::text_quantity_min=QLatin1Literal("quantity_min");
const QString BaseServer::text_quantity_max=QLatin1Literal("quantity_max");
const QString BaseServer::text_luck=QLatin1Literal("luck");
const QString BaseServer::text_percent=QLatin1Literal("percent");

BaseServer::BaseServer() :
    stat(Down),
    dataLoaded(false)
{
    ProtocolParsing::initialiseTheVariable();

    ProtocolParsing::compressionType                                = ProtocolParsing::CompressionType_Zlib;

    GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue=0;
    GlobalServerData::serverPrivateVariables.connected_players      = 0;
    GlobalServerData::serverPrivateVariables.number_of_bots_logged  = 0;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    GlobalServerData::serverPrivateVariables.timer_city_capture     = NULL;
    #endif
    #ifdef Q_OS_LINUX
    GlobalServerData::serverPrivateVariables.fpRandomFile           = NULL;
    #endif

    GlobalServerData::serverPrivateVariables.botSpawnIndex          = 0;
    GlobalServerData::serverPrivateVariables.datapack_rightFileName	= QRegularExpression(DATAPACK_FILE_REGEX);
    GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=NULL;

    GlobalServerData::serverSettings.automatic_account_creation             = false;
    GlobalServerData::serverSettings.max_players                            = 1;
    GlobalServerData::serverSettings.tolerantMode                           = false;
    GlobalServerData::serverSettings.sendPlayerNumber                       = true;
    GlobalServerData::serverSettings.pvp                                    = true;

    GlobalServerData::serverSettings.database.type                              = CatchChallenger::ServerSettings::Database::DatabaseType_SQLite;
    GlobalServerData::serverSettings.database.sqlite.file                       = QString();
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = CatchChallenger::MapVisibilityAlgorithmSelection_None;
    GlobalServerData::serverSettings.datapackCache                              = -1;
    GlobalServerData::serverSettings.datapack_basePath                          = QCoreApplication::applicationDirPath()+QLatin1String("/datapack/");
    GlobalServerData::serverSettings.compressionType                            = CompressionType_Zlib;
    GlobalServerData::serverSettings.dontSendPlayerType                         = false;
    CommonSettings::commonSettings.httpDatapackMirror                           = QString();
    CommonSettings::commonSettings.forceClientToSendAtMapChange = true;
    CommonSettings::commonSettings.forcedSpeed            = CATCHCHALLENGER_SERVER_NORMAL_SPEED;
    CommonSettings::commonSettings.useSP                  = true;
    CommonSettings::commonSettings.maxPlayerMonsters            = 8;
    CommonSettings::commonSettings.maxWarehousePlayerMonsters   = 30;
    CommonSettings::commonSettings.maxPlayerItems               = 30;
    CommonSettings::commonSettings.maxWarehousePlayerItems      = 150;
    GlobalServerData::serverSettings.anonymous            = false;
    CommonSettings::commonSettings.autoLearn              = false;//need useSP to false
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
    CommonSettings::commonSettings.waitBeforeConnectAfterKick=30;
    GlobalServerData::serverSettings.database.type                              = ServerSettings::Database::DatabaseType_Mysql;
    GlobalServerData::serverSettings.database.fightSync                         = ServerSettings::Database::FightSync_AtTheEndOfBattle;
    GlobalServerData::serverSettings.database.positionTeleportSync              = true;
    GlobalServerData::serverSettings.database.secondToPositionSync              = 0;
    GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm       = MapVisibilityAlgorithmSelection_Simple;
    GlobalServerData::serverSettings.mapVisibility.simple.max                   = 30;
    GlobalServerData::serverSettings.mapVisibility.simple.reshow                = 20;
    GlobalServerData::serverSettings.mapVisibility.simple.reemit                = true;
    GlobalServerData::serverSettings.mapVisibility.withBorder.maxWithBorder     = 20;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshowWithBorder  = 10;
    GlobalServerData::serverSettings.mapVisibility.withBorder.max               = 30;
    GlobalServerData::serverSettings.mapVisibility.withBorder.reshow            = 20;
    GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue      = 3;
    GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval       = 5;
    GlobalServerData::serverSettings.ddos.kickLimitMove                         = 60;
    GlobalServerData::serverSettings.ddos.kickLimitChat                         = 5;
    GlobalServerData::serverSettings.ddos.kickLimitOther                        = 30;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessageGeneral          = 20;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan        = 20;
    GlobalServerData::serverSettings.ddos.dropGlobalChatMessagePrivate          = 20;
    GlobalServerData::serverSettings.city.capture.frenquency                    = City::Capture::Frequency_week;
    GlobalServerData::serverSettings.city.capture.day                           = City::Capture::Monday;
    GlobalServerData::serverSettings.city.capture.hour                          = 0;
    GlobalServerData::serverSettings.city.capture.minute                        = 0;
    GlobalServerData::serverPrivateVariables.flat_map_list                      = NULL;


    initAll();

    srand(time(NULL));
}

/** call only when the server is down
 * \warning this function is thread safe because it quit all thread before remove */
BaseServer::~BaseServer()
{
    GlobalServerData::serverPrivateVariables.stopIt=true;
    closeDB();
    if(GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.flat_map_list;
        GlobalServerData::serverPrivateVariables.flat_map_list=NULL;
    }
}

void BaseServer::closeDB()
{
    GlobalServerData::serverPrivateVariables.db.syncDisconnect();
}

void BaseServer::initAll()
{
}

//////////////////////////////////////////// server starting //////////////////////////////////////

void BaseServer::preload_the_data()
{
    DebugClass::debugConsole(QStringLiteral("Preload data for server version %1").arg(CATCHCHALLENGER_VERSION));

    if(dataLoaded)
        return;
    dataLoaded=true;
    GlobalServerData::serverPrivateVariables.stopIt=false;

    {
        QTime time;
        time.restart();
        CommonDatapack::commonDatapack.parseDatapack(GlobalServerData::serverSettings.datapack_basePath);
        int index=0;
        while(index<CommonDatapack::commonDatapack.profileList.size())
        {
            CommonDatapack::commonDatapack.profileList[index].map.remove(BaseServer::text_dottmx);
            index++;
        }
        qDebug() << QStringLiteral("Loaded the common datapack into %1ms").arg(time.elapsed());
    }
    timeDatapack.restart();
    preload_the_randomData();
    preload_the_events();
    preload_the_ddos();
    preload_the_datapack();
    preload_the_skin();
    preload_the_players();
    preload_monsters_drops();
    preload_itemOnMap_sql();
}

void BaseServer::preload_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.events.size())
    {
        GlobalServerData::serverPrivateVariables.events << 0;
        index++;
    }
    {
        QHashIterator<QString,QHash<QString,ServerSettings::ProgrammedEvent> > i(GlobalServerData::serverSettings.programmedEventList);
        while (i.hasNext()) {
            i.next();
            int index=0;
            while(index<CommonDatapack::commonDatapack.events.size())
            {
                const Event &event=CommonDatapack::commonDatapack.events.at(index);
                if(event.name==i.key())
                {
                    QHashIterator<QString,ServerSettings::ProgrammedEvent> j(i.value());
                    while (j.hasNext()) {
                        j.next();
                        const int &sub_index=event.values.indexOf(j.value().value);
                        if(sub_index!=-1)
                        {
                            #ifdef EPOLLCATCHCHALLENGERSERVER
                            GlobalServerData::serverPrivateVariables.timerEvents << new TimerEvents(index,sub_index);
                            GlobalServerData::serverPrivateVariables.timerEvents.last()->start(j.value().cycle*1000*60,j.value().offset*1000*60);
                            #else
                            GlobalServerData::serverPrivateVariables.timerEvents << new QtTimerEvents(j.value().offset*1000*60,j.value().cycle*1000*60,index,sub_index);
                            #endif
                        }
                        else
                            GlobalServerData::serverSettings.programmedEventList[i.key()].remove(i.key());
                    }
                    break;
                }
                index++;
            }
            if(index==CommonDatapack::commonDatapack.events.size())
                GlobalServerData::serverSettings.programmedEventList.remove(i.key());
        }
    }
}

void BaseServer::preload_the_randomData()
{
    #ifdef Q_OS_LINUX
    if(GlobalServerData::serverPrivateVariables.fpRandomFile!=NULL)
        fclose(GlobalServerData::serverPrivateVariables.fpRandomFile);
    GlobalServerData::serverPrivateVariables.fpRandomFile = fopen("/dev/urandom","r");
    if(GlobalServerData::serverPrivateVariables.fpRandomFile==NULL)
        qDebug() << "Unable to open /dev/urandom to have trusted number generator";
    #endif
    GlobalServerData::serverPrivateVariables.tokenForAuthSize=0;
    GlobalServerData::serverPrivateVariables.randomData.clear();
    QDataStream randomDataStream(&GlobalServerData::serverPrivateVariables.randomData, QIODevice::WriteOnly);
    randomDataStream.setVersion(QDataStream::Qt_4_4);
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_RANDOM_INTERNAL_SIZE)
    {
        randomDataStream << quint8(rand()%256);
        index++;
    }
}

void BaseServer::preload_the_ddos()
{
    unload_the_ddos();
    int index=0;
    while(index<CATCHCHALLENGER_SERVER_DDOS_MAX_VALUE)
    {
        Client::generalChatDrop << 0;
        Client::clanChatDrop << 0;
        Client::privateChatDrop << 0;
        index++;
    }
    Client::generalChatDropTotalCache=0;
    Client::generalChatDropNewValue=0;
    Client::clanChatDropTotalCache=0;
    Client::clanChatDropNewValue=0;
    Client::privateChatDropTotalCache=0;
    Client::privateChatDropNewValue=0;
}

void BaseServer::preload_zone_init()
{
    const int &listsize=entryListZone.size();
    int index=0;
    while(index<listsize)
    {
        if(!entryListZone.at(index).isFile())
        {
            index++;
            continue;
        }
        if(!entryListZone.at(index).fileName().contains(regexXmlFile))
        {
            qDebug() << QStringLiteral("%1 the zone file name not match").arg(entryListZone.at(index).fileName());
            index++;
            continue;
        }
        QString zoneCodeName=entryListZone.at(index).fileName();
        zoneCodeName.remove(BaseServer::text_dotxml);
        QDomDocument domDocument;
        const QString &file=entryListZone.at(index).absoluteFilePath();
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
        QDomElement root(domDocument.documentElement());
        if(root.tagName()!=BaseServer::text_zone)
        {
            qDebug() << QStringLiteral("Unable to open the file: %1, \"zone\" root balise not found for the xml file").arg(file);
            index++;
            continue;
        }

        //load capture
        QList<quint16> fightIdList;
        QDomElement capture(root.firstChildElement(BaseServer::text_capture));
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
                    const quint16 &fightId=fightIdStringList.at(sub_index).toUShort(&ok);
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
        index++;
    }

    qDebug() << QStringLiteral("%1 zone(s) loaded").arg(GlobalServerData::serverPrivateVariables.captureFightIdList.size());
}

void BaseServer::preload_zone_sql()
{
    const int &listsize=entryListZone.size();
    while(entryListIndex<listsize)
    {
        QString zoneCodeName=entryListZone.at(entryListIndex).fileName();
        zoneCodeName.remove(BaseServer::text_dotxml);
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
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                queryText=QStringLiteral("SELECT clan FROM city WHERE city='%1';").arg(zoneCodeName);
            break;
        }
        if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_zone_static)==NULL)
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
            abort();//stop because can't do the first db access
            entryListIndex++;
            load_monsters_max_id();
            return;
        }
        else
            return;
    }
    load_monsters_max_id();
}

void BaseServer::preload_itemOnMap_sql()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`map`,`x`,`y` FROM `dictionary_itemonmap`;");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,map,x,y FROM dictionary_itemonmap;");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,map,x,y FROM dictionary_itemonmap;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_itemOnMap_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't do the first db access

        preload_the_map();
        preload_the_visibility_algorithm();
        preload_the_city_capture();
        preload_zone();
        qDebug() << QStringLiteral("Loaded the server static datapack into %1ms").arg(timeDatapack.elapsed());
        timeDatapack.restart();

        //start SQL load
        preload_zone_sql();
    }
}

void BaseServer::preload_itemOnMap_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_itemOnMap_return();
}

void BaseServer::preload_itemOnMap_return()
{
    bool ok;
    GlobalServerData::serverPrivateVariables.dictionary_item_maxId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        const quint16 &id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
            qDebug() << QStringLiteral("preload_itemOnMap_return(): Id not found: %1").arg(QString(GlobalServerData::serverPrivateVariables.db.value(0)));
        else
        {
            const QString &map=QString(GlobalServerData::serverPrivateVariables.db.value(1));
            const quint32 &x=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
            if(!ok)
                qDebug() << QStringLiteral("preload_itemOnMap_return(): x not number: %1").arg(QString(GlobalServerData::serverPrivateVariables.db.value(2)));
            else
            {
                if(x>255)
                    qDebug() << QStringLiteral("preload_itemOnMap_return(): x out of range").arg(x);
                else
                {
                    const quint32 &y=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
                    if(!ok)
                        qDebug() << QStringLiteral("preload_itemOnMap_return(): y not number: %1").arg(QString(GlobalServerData::serverPrivateVariables.db.value(3)));
                    else
                    {
                        if(y>255)
                            qDebug() << QStringLiteral("preload_itemOnMap_return(): y out of range").arg(y);
                        else
                        {
                            if(GlobalServerData::serverPrivateVariables.dictionary_item.contains(map)
                                    && GlobalServerData::serverPrivateVariables.dictionary_item.value(map).contains(QPair<quint8/*x*/,quint8/*y*/>(x,y)))
                                qDebug() << QStringLiteral("preload_itemOnMap_return(): duplicate entry: %1 %2 %3").arg(map).arg(x).arg(y);
                            else
                            {
                                GlobalServerData::serverPrivateVariables.dictionary_item[map][QPair<quint8/*x*/,quint8/*y*/>(x,y)]=id;
                                GlobalServerData::serverPrivateVariables.dictionary_item_maxId=id;
                            }
                        }
                    }
                }
            }
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    {
        preload_the_map();
        preload_the_visibility_algorithm();
        preload_the_city_capture();
        preload_zone();
        qDebug() << QStringLiteral("Loaded the server static datapack into %1ms").arg(timeDatapack.elapsed());
        timeDatapack.restart();

        //start SQL load
        preload_zone_sql();
    }
}

void BaseServer::preload_dictionary_allow()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`allow` FROM `dictionary_allow`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,allow FROM dictionary_allow");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_dictionary_allow_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServer::preload_dictionary_allow_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_allow_return();
}

void BaseServer::preload_dictionary_allow_return()
{
    GlobalServerData::serverPrivateVariables.dictionary_allow_reverse << 0x00 << 0x00;
    bool haveAllowClan=false;
    QString allowClan(QStringLiteral("clan"));
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &allow=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_allow.size()<lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_allow.size();
            while(index<lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
                index++;
            }
        }
        if(allow==allowClan)
        {
            haveAllowClan=true;
            GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Clan;
            GlobalServerData::serverPrivateVariables.dictionary_allow_reverse[ActionAllow_Clan]=lastId;
        }
        else
            GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    if(!haveAllowClan)
    {
        lastId++;
        QString queryText;
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QStringLiteral("INSERT INTO `dictionary_allow`(`id`,`allow`) VALUES(%1,'clan');").arg(lastId);
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                queryText=QStringLiteral("INSERT INTO dictionary_allow(id,allow) VALUES(%1,'clan');").arg(lastId);
            break;
        }
        if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
        {
            qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
            abort();//stop because can't resolv the name
        }
        while(GlobalServerData::serverPrivateVariables.dictionary_allow.size()<lastId)
            GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Nothing;
        GlobalServerData::serverPrivateVariables.dictionary_allow << ActionAllow_Clan;
        GlobalServerData::serverPrivateVariables.dictionary_allow_reverse[ActionAllow_Clan]=lastId;
    }
    preload_dictionary_map();
}

void BaseServer::preload_dictionary_map()
{
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`map` FROM `dictionary_map`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,map FROM dictionary_map");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,map FROM dictionary_map");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_dictionary_map_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServer::preload_dictionary_map_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_map_return();
}

void BaseServer::preload_dictionary_map_return()
{
    QSet<QString> foundMap;
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &map=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_map.size()<lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_map.size();
            while(index<lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_map << NULL;
                index++;
            }
        }
        if(GlobalServerData::serverPrivateVariables.map_list.contains(map))
        {
            GlobalServerData::serverPrivateVariables.dictionary_map << static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(map));
            foundMap << map;
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(map))->reverse_db_id=lastId;
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    QStringList map_list_flat=GlobalServerData::serverPrivateVariables.map_list.keys();
    map_list_flat.sort();
    int index=0;
    while(index<map_list_flat.size())
    {
        const QString &map=map_list_flat.at(index);
        if(!foundMap.contains(map))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_map`(`id`,`map`) VALUES(%1,'%2');").arg(lastId).arg(map);
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_map(id,map) VALUES(%1,'%2');").arg(lastId).arg(map);
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_map(id,map) VALUES(%1,'%2');").arg(lastId).arg(map);
                break;
            }
            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                abort();//stop because can't resolv the name
            }
            while(GlobalServerData::serverPrivateVariables.dictionary_map.size()<=lastId)
                GlobalServerData::serverPrivateVariables.dictionary_map << NULL;
            GlobalServerData::serverPrivateVariables.dictionary_map[lastId]=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map]);
            static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list[map])->reverse_db_id=lastId;
        }
        index++;
    }
    plant_on_the_map=0;
    preload_the_plant_on_map();
}

void BaseServer::preload_dictionary_reputation()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL plant on map").arg(plant_on_the_map));

    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`reputation` FROM `dictionary_reputation`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,reputation FROM dictionary_reputation");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_dictionary_reputation_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServer::preload_dictionary_reputation_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_reputation_return();
}

void BaseServer::preload_dictionary_reputation_return()
{
    QHash<QString,quint8> reputationResolution;
    {
        quint8 index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationResolution[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    QSet<QString> foundReputation;
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &reputation=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_reputation.size()<lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_reputation.size();
            while(index<lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_reputation << -1;
                index++;
            }
        }
        if(reputationResolution.contains(reputation))
        {
            GlobalServerData::serverPrivateVariables.dictionary_reputation << reputationResolution.value(reputation);
            foundReputation << reputation;
            CommonDatapack::commonDatapack.reputation[reputationResolution.value(reputation)].reverse_database_id=lastId;
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    int index=0;
    while(index<CommonDatapack::commonDatapack.reputation.size())
    {
        const QString &reputation=CommonDatapack::commonDatapack.reputation.at(index).name;
        if(!foundReputation.contains(reputation))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_reputation`(`id`,`reputation`) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_reputation(id,reputation) VALUES(%1,'%2');").arg(lastId).arg(reputation);
                break;
            }
            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                abort();//stop because can't resolv the name
            }
            while(GlobalServerData::serverPrivateVariables.dictionary_reputation.size()<=lastId)
                GlobalServerData::serverPrivateVariables.dictionary_reputation << -1;
            GlobalServerData::serverPrivateVariables.dictionary_reputation[lastId]=index;
            CommonDatapack::commonDatapack.reputation[index].reverse_database_id=lastId;
        }
        index++;
    }
    preload_dictionary_skin();
}

void BaseServer::preload_dictionary_skin()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL reputation dictionary").arg(GlobalServerData::serverPrivateVariables.dictionary_reputation.size()));

    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `id`,`skin` FROM `dictionary_skin`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT id,skin FROM dictionary_skin");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_dictionary_skin_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        abort();//stop because can't resolv the name
    }
}

void BaseServer::preload_dictionary_skin_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_dictionary_skin_return();
}

void BaseServer::preload_dictionary_skin_return()
{
    {
        int index=0;
        while(index<GlobalServerData::serverPrivateVariables.skinList.size())
        {
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse << 0;
            index++;
        }
    }
    QSet<QString> foundSkin;
    int lastId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        lastId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        const QString &skin=QString(GlobalServerData::serverPrivateVariables.db.value(1));
        if(GlobalServerData::serverPrivateVariables.dictionary_skin.size()<lastId)
        {
            int index=GlobalServerData::serverPrivateVariables.dictionary_skin.size();
            while(index<lastId)
            {
                GlobalServerData::serverPrivateVariables.dictionary_skin << 0;
                index++;
            }
        }
        if(GlobalServerData::serverPrivateVariables.skinList.contains(skin))
        {
            const quint8 &internalValue=GlobalServerData::serverPrivateVariables.skinList.value(skin);
            GlobalServerData::serverPrivateVariables.dictionary_skin << internalValue;
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse[internalValue]=lastId;
            foundSkin << skin;
        }
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    QHashIterator<QString,quint8> i(GlobalServerData::serverPrivateVariables.skinList);
    while (i.hasNext()) {
        i.next();
        const QString &skin=i.key();
        if(!foundSkin.contains(skin))
        {
            lastId++;
            QString queryText;
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    queryText=QStringLiteral("INSERT INTO `dictionary_skin`(`id`,`skin`) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    queryText=QStringLiteral("INSERT INTO dictionary_skin(id,skin) VALUES(%1,'%2');").arg(lastId).arg(skin);
                break;
            }
            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
            {
                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                abort();//stop because can't resolv the name
            }
            while(GlobalServerData::serverPrivateVariables.dictionary_skin.size()<lastId)
                GlobalServerData::serverPrivateVariables.dictionary_skin << 0;
            GlobalServerData::serverPrivateVariables.dictionary_skin << i.value();
            GlobalServerData::serverPrivateVariables.dictionary_skin_reverse[i.value()]=lastId;
        }
    }
    preload_profile();
}

void BaseServer::preload_profile()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL skin dictionary").arg(GlobalServerData::serverPrivateVariables.dictionary_skin.size()));

    int index=0;
    while(index<CommonDatapack::commonDatapack.profileList.size())
    {
        const Profile &profile=CommonDatapack::commonDatapack.profileList.at(index);
        ServerProfile serverProfile;
        serverProfile.valid=false;
        if(GlobalServerData::serverPrivateVariables.map_list.contains(profile.map))
        {
            const quint32 &mapId=static_cast<MapServer *>(GlobalServerData::serverPrivateVariables.map_list.value(profile.map))->reverse_db_id;
            const QString &mapQuery=QString::number(mapId)+QLatin1String(",")+QString::number(profile.x)+QLatin1String(",")+QString::number(profile.y)+QLatin1String(",")+QString::number(Orientation_bottom);
            switch(GlobalServerData::serverSettings.database.type)
            {
                default:
                case ServerSettings::Database::DatabaseType_Mysql:
                    serverProfile.preparedQuery << QStringLiteral("INSERT INTO `character`(`id`,`account`,`pseudo`,`skin`,`map`,`x`,`y`,`orientation`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`market_cash`,`date`,`warehouse_cash`,`clan_leader`,`time_to_delete`,`played_time`,`last_connect`,`starter`) VALUES(");
                    serverProfile.preparedQuery << QLatin1String(",");
                    serverProfile.preparedQuery << QLatin1String(",'");
                    serverProfile.preparedQuery << QLatin1String("',");
                    serverProfile.preparedQuery << QLatin1String(",")+//skin verificated above
                            mapQuery+QLatin1String(",0,0,")+
                            QString::number(profile.cash)+QLatin1String(",")+
                            mapQuery+QLatin1String(",")+
                            mapQuery+QLatin1String(",0,")+
                            QString::number(QDateTime::currentDateTime().toTime_t())+QLatin1String(",0,0,0,0,0,")+
                            QString::number(index)+QLatin1String(");");
                break;
                case ServerSettings::Database::DatabaseType_SQLite:
                    serverProfile.preparedQuery << QStringLiteral("INSERT INTO character(id,account,pseudo,skin,map,x,y,orientation,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                    serverProfile.preparedQuery << QLatin1String(",");
                    serverProfile.preparedQuery << QLatin1String(",'");
                    serverProfile.preparedQuery << QLatin1String("',");
                    serverProfile.preparedQuery << QLatin1String(",")+//skin verificated above
                            mapQuery+QLatin1String(",0,0,")+
                            QString::number(profile.cash)+QLatin1String(",")+
                            mapQuery+QLatin1String(",")+
                            mapQuery+QLatin1String(",0,")+
                            QString::number(QDateTime::currentDateTime().toTime_t())+QLatin1String(",0,0,0,0,0,")+
                            QString::number(index)+QLatin1String(");");
                break;
                case ServerSettings::Database::DatabaseType_PostgreSQL:
                    serverProfile.preparedQuery << QStringLiteral("INSERT INTO character(id,account,pseudo,skin,map,x,y,orientation,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,market_cash,date,warehouse_cash,clan_leader,time_to_delete,played_time,last_connect,starter) VALUES(");
                    serverProfile.preparedQuery << QLatin1String(",");
                    serverProfile.preparedQuery << QLatin1String(",'");
                    serverProfile.preparedQuery << QLatin1String("',");
                    serverProfile.preparedQuery << QLatin1String(",")+//skin verificated above
                            mapQuery+QLatin1String(",0,0,")+
                            QString::number(profile.cash)+QLatin1String(",")+
                            mapQuery+QLatin1String(",")+
                            mapQuery+QLatin1String(",0,")+
                            QString::number(QDateTime::currentDateTime().toTime_t())+QLatin1String(",0,false,0,0,0,")+
                            QString::number(index)+QLatin1String(");");
                break;
            }
            serverProfile.valid=true;
        }
        GlobalServerData::serverPrivateVariables.serverProfileList << serverProfile;
        index++;
    }

    DebugClass::debugConsole(QStringLiteral("%1 profile loaded").arg(GlobalServerData::serverPrivateVariables.serverProfileList.size()));
    preload_finish();
}

void BaseServer::preload_zone()
{
    //open and quick check the file
    entryListZone=QFileInfoList(QDir(GlobalServerData::serverSettings.datapack_basePath+DATAPACK_BASE_PATH_ZONE).entryInfoList(QDir::AllEntries|QDir::NoDotAndDotDot));
    entryListIndex=0;
    preload_zone_init();
}

void BaseServer::preload_zone_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_zone_return();
}

void BaseServer::preload_zone_return()
{
    if(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        QString zoneCodeName=entryListZone.at(entryListIndex).fileName();
        zoneCodeName.remove(BaseServer::text_dotxml);
        const QString &tempString=QString(GlobalServerData::serverPrivateVariables.db.value(0));
        const quint32 &clanId=tempString.toUInt(&ok);
        if(ok)
        {
            GlobalServerData::serverPrivateVariables.cityStatusList[zoneCodeName].clan=clanId;
            GlobalServerData::serverPrivateVariables.cityStatusListReverse[clanId]=zoneCodeName;
        }
        else
            DebugClass::debugConsole(QStringLiteral("clan id is failed to convert to number for city status"));
    }
    GlobalServerData::serverPrivateVariables.db.clear();
    entryListIndex++;
    preload_zone_sql();
}

void BaseServer::preload_industries()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL clan max id").arg(GlobalServerData::serverPrivateVariables.maxClanId));

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
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QLatin1String("SELECT id,resources,products,last_update FROM factory");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_industries_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_market_monsters();
    }
}

void BaseServer::preload_industries_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_industries_return();
}

void BaseServer::preload_industries_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        IndustryStatus industryStatus;
        bool ok;
        quint32 id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
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
            const QStringList &resourcesStringList=QString(GlobalServerData::serverPrivateVariables.db.value(1)).split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=resourcesStringList.size();
            while(index<listsize)
            {
                const QStringList &itemStringList=resourcesStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                const quint32 &item=itemStringList.first().toUInt(&ok);
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
            const QStringList &productsStringList=QString(GlobalServerData::serverPrivateVariables.db.value(2)).split(BaseServer::text_dotcomma);
            int index=0;
            const int &listsize=productsStringList.size();
            while(index<listsize)
            {
                const QStringList &itemStringList=productsStringList.at(index).split(BaseServer::text_arrow);
                if(itemStringList.size()!=2)
                {
                    DebugClass::debugConsole(QStringLiteral("preload_industries: wrong entry count"));
                    ok=false;
                    break;
                }
                const quint32 &item=itemStringList.first().toUInt(&ok);
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
            industryStatus.last_update=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
            if(!ok)
                DebugClass::debugConsole(QStringLiteral("preload_industries: last_update is not a number"));
        }
        if(ok)
            GlobalServerData::serverPrivateVariables.industriesStatus[id]=industryStatus;
    }
    preload_market_monsters();
}

void BaseServer::preload_market_monsters()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL industrie loaded").arg(GlobalServerData::serverPrivateVariables.industriesStatus.size()));

    QString queryText;
    if(CommonSettings::commonSettings.useSP)
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QLatin1String("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character`,`market_price` FROM `monster_market`");
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QLatin1String("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character,market_price FROM monster_market");
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                queryText=QLatin1String("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character,market_price FROM monster_market");
            break;
        }
    else
        switch(GlobalServerData::serverSettings.database.type)
        {
            default:
            case ServerSettings::Database::DatabaseType_Mysql:
                queryText=QLatin1String("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character`,`market_price` FROM `monster_market`");
            break;
            case ServerSettings::Database::DatabaseType_SQLite:
                queryText=QLatin1String("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character,market_price FROM monster_market");
            break;
            case ServerSettings::Database::DatabaseType_PostgreSQL:
                queryText=QLatin1String("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character,market_price FROM monster_market");
            break;
        }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_market_monsters_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_market_items();
    }
}

void BaseServer::preload_market_monsters_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_market_monsters_return();
}

void BaseServer::preload_market_monsters_return()
{
    quint8 spOffset;
    if(CommonSettings::commonSettings.useSP)
        spOffset=0;
    else
        spOffset=1;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        MarketPlayerMonster marketPlayerMonster;
        PlayerMonster playerMonster;
        playerMonster.id=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
            DebugClass::debugConsole(QStringLiteral("monsterId: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            playerMonster.monster=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.monsters.contains(playerMonster.monster))
                {
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("monster: %1 is not into monster list").arg(playerMonster.monster));
                }
            }
            else
            DebugClass::debugConsole(QStringLiteral("monster: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
        {
            playerMonster.level=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.level>CATCHCHALLENGER_MONSTER_LEVEL_MAX)
                {
                    DebugClass::debugConsole(QStringLiteral("level: %1 greater than %2, truncated").arg(playerMonster.level).arg(CATCHCHALLENGER_MONSTER_LEVEL_MAX));
                    playerMonster.level=CATCHCHALLENGER_MONSTER_LEVEL_MAX;
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(3)));
        }
        if(ok)
        {
            playerMonster.remaining_xp=QString(GlobalServerData::serverPrivateVariables.db.value(4)).toUInt(&ok);
            if(ok)
            {
                if(playerMonster.remaining_xp>CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1))
                {
                    DebugClass::debugConsole(QStringLiteral("monster xp: %1 greater than %2, truncated").arg(playerMonster.remaining_xp).arg(CommonDatapack::commonDatapack.monsters.value(playerMonster.monster).level_to_xp.at(playerMonster.level-1)));
                    playerMonster.remaining_xp=0;
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("monster xp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(4)));
        }
        if(CommonSettings::commonSettings.useSP)
        {
            if(ok)
            {
                playerMonster.sp=QString(GlobalServerData::serverPrivateVariables.db.value(5)).toUInt(&ok);
                if(!ok)
                    DebugClass::debugConsole(QStringLiteral("monster sp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(5)));
            }
        }
        else
            playerMonster.sp=0;
        if(ok)
        {
            playerMonster.catched_with=QString(GlobalServerData::serverPrivateVariables.db.value(6-spOffset)).toUInt(&ok);
            if(ok)
            {
                if(!CommonDatapack::commonDatapack.items.item.contains(playerMonster.catched_with))
                    DebugClass::debugConsole(QStringLiteral("captured_with: %1 is not is not into items list").arg(playerMonster.catched_with));
            }
            else
                DebugClass::debugConsole(QStringLiteral("captured_with: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(6-spOffset)));
        }
        if(ok)
        {
            const quint32 &value=QString(GlobalServerData::serverPrivateVariables.db.value(7-spOffset)).toUInt(&ok);
            if(ok)
            {
                if(value>=1 && value<=3)
                    playerMonster.gender=static_cast<Gender>(value);
                else
                {
                    playerMonster.gender=Gender_Unknown;
                    DebugClass::debugConsole(QStringLiteral("unknown monster gender: %1").arg(value));
                    ok=false;
                }
            }
            else
            {
                playerMonster.gender=Gender_Unknown;
                DebugClass::debugConsole(QStringLiteral("unknown monster gender: %1").arg(GlobalServerData::serverPrivateVariables.db.value(7-spOffset)));
                ok=false;
            }
        }
        if(ok)
        {
            playerMonster.egg_step=QString(GlobalServerData::serverPrivateVariables.db.value(8-spOffset)).toUInt(&ok);
            if(!ok)
                DebugClass::debugConsole(QStringLiteral("monster egg_step: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(8-spOffset)));
        }
        if(ok)
            marketPlayerMonster.player=QString(GlobalServerData::serverPrivateVariables.db.value(9-spOffset)).toUInt(&ok);
        if(ok)
            marketPlayerMonster.cash=QString(GlobalServerData::serverPrivateVariables.db.value(10-spOffset)).toULongLong(&ok);
        //stats
        if(ok)
        {
            playerMonster.hp=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
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
            DebugClass::debugConsole(QStringLiteral("monster hp: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
        }
        //finish it
        if(ok)
        {
            marketPlayerMonster.monster=playerMonster;
            GlobalServerData::serverPrivateVariables.marketPlayerMonsterList << marketPlayerMonster;
        }
    }
    if(!GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.isEmpty())
        loadMonsterBuffs(0);
    else
        preload_market_items();
}

void BaseServer::preload_market_items()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL monster list loaded").arg(GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size()));

    Client::marketObjectIdList.clear();
    Client::marketObjectIdList.reserve(65535);
    int index=0;
    while(index<=65535)
    {
        Client::marketObjectIdList << index;
        index++;
    }
    //do the query
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QLatin1String("SELECT `item`,`quantity`,`character`,`market_price` FROM `item_market`");
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QLatin1String("SELECT item,quantity,character,market_price FROM item_market");
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QLatin1String("SELECT item,quantity,character,market_price FROM item_market");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::preload_market_items_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        if(GlobalServerData::serverSettings.automatic_account_creation)
            load_account_max_id();
        else if(CommonSettings::commonSettings.max_character)
            load_character_max_id();
        else
            preload_dictionary_allow();
    }
}

void BaseServer::preload_market_items_static(void *object)
{
    static_cast<BaseServer *>(object)->preload_market_items_return();
}

void BaseServer::preload_market_items_return()
{
    bool ok;
    //parse the result
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        MarketItem marketItem;
        marketItem.item=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("item id is not a number, skip"));
            continue;
        }
        marketItem.quantity=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("quantity is not a number, skip"));
            continue;
        }
        marketItem.player=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("player id is not a number, skip"));
            continue;
        }
        marketItem.cash=QString(GlobalServerData::serverPrivateVariables.db.value(3)).toULongLong(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("cash is not a number, skip"));
            continue;
        }
        if(Client::marketObjectIdList.isEmpty())
        {
            DebugClass::debugConsole(QStringLiteral("not more marketObjectId into the list, skip"));
            return;
        }
        marketItem.marketObjectId=Client::marketObjectIdList.first();
        Client::marketObjectIdList.removeFirst();
        GlobalServerData::serverPrivateVariables.marketItemList << marketItem;
    }
    if(GlobalServerData::serverSettings.automatic_account_creation)
        load_account_max_id();
    else if(CommonSettings::commonSettings.max_character)
        load_character_max_id();
    else
        preload_dictionary_allow();
}

void BaseServer::loadMonsterBuffs(const quint32 &index)
{
    entryListIndex=index;
    if(index>=(quint32)GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        loadMonsterSkills(0);
        return;
    }
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1").arg(index);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(index);
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1").arg(index);
        break;
    }

    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::loadMonsterBuffs_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        loadMonsterSkills(0);
    }
}

void BaseServer::loadMonsterBuffs_static(void *object)
{
    static_cast<BaseServer *>(object)->loadMonsterBuffs_return();
}

void BaseServer::loadMonsterBuffs_return()
{
    const quint32 &monsterId=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(entryListIndex).monster.id;
    QList<PlayerBuff> buffs;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        PlayerBuff buff;
        buff.buff=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterBuffs.contains(buff.buff))
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 is not found into buff list").arg(buff.buff).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("buff id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            buff.level=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                if(buff.level<=0 || buff.level>CommonDatapack::commonDatapack.monsterBuffs.value(buff.buff).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("buff %1 for monsterId: %2 have not the level: %3").arg(buff.buff).arg(monsterId).arg(buff.level));
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("buff level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
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
    loadMonsterBuffs(entryListIndex+1);
}

void BaseServer::loadMonsterSkills(const quint32 &index)
{
    entryListIndex=index;
    if(index>=(quint32)GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.size())
    {
        preload_market_items();
        return;
    }
    QString queryText;
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        case ServerSettings::Database::DatabaseType_Mysql:
            queryText=QStringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1").arg(index);
        break;
        case ServerSettings::Database::DatabaseType_SQLite:
            queryText=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1").arg(index);
        break;
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1").arg(index);
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::loadMonsterSkills_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_market_items();
    }
}

void BaseServer::loadMonsterSkills_static(void *object)
{
    static_cast<BaseServer *>(object)->loadMonsterSkills_return();
}

void BaseServer::loadMonsterSkills_return()
{
    const quint32 &monsterId=GlobalServerData::serverPrivateVariables.marketPlayerMonsterList.at(entryListIndex).monster.id;
    QList<PlayerMonster::PlayerSkill> skills;
    bool ok;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        PlayerMonster::PlayerSkill skill;
        skill.skill=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(ok)
        {
            if(!CommonDatapack::commonDatapack.monsterSkills.contains(skill.skill))
            {
                ok=false;
                DebugClass::debugConsole(QStringLiteral("skill %1 for monsterId: %2 is not found into skill list").arg(skill.skill).arg(monsterId));
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("skill id: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(0)));
        if(ok)
        {
            skill.level=QString(GlobalServerData::serverPrivateVariables.db.value(1)).toUInt(&ok);
            if(ok)
            {
                if(skill.level>CommonDatapack::commonDatapack.monsterSkills.value(skill.skill).level.size())
                {
                    ok=false;
                    DebugClass::debugConsole(QStringLiteral("skill %1 for monsterId: %2 have not the level: %3").arg(skill.skill).arg(monsterId).arg(skill.level));
                }
            }
            else
                DebugClass::debugConsole(QStringLiteral("skill level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(1)));
        }
        if(ok)
        {
            skill.endurance=QString(GlobalServerData::serverPrivateVariables.db.value(2)).toUInt(&ok);
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
                DebugClass::debugConsole(QStringLiteral("skill level: %1 is not a number").arg(GlobalServerData::serverPrivateVariables.db.value(2)));
        }
        if(ok)
            skills << skill;
    }
    loadMonsterSkills(entryListIndex+1);
}

void BaseServer::preload_the_city_capture()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
    GlobalServerData::serverPrivateVariables.timer_city_capture=new QTimer();
    GlobalServerData::serverPrivateVariables.timer_city_capture->setSingleShot(true);
    #endif
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
    QStringList map_name_to_do_id;
    QStringList returnList=FacilityLib::listFolder(GlobalServerData::serverPrivateVariables.datapack_mapPath);
    returnList.sort();

    //load the map
    quint8 indexOfItemOnMap=0;
    int size=returnList.size();
    int index=0;
    int sub_index;
    QString tmxRemove(".tmx");
    QRegularExpression mapFilter(QLatin1String("\\.tmx$"));
    QRegularExpression mapExclude(QLatin1String("[\"']"));
    QList<CommonMap *> flat_map_list_temp;
    while(index<size)
    {
        QString fileName=returnList.at(index);
        fileName.replace(BaseServer::text_antislash,BaseServer::text_slash);
        if(fileName.contains(mapFilter) && !fileName.contains(mapExclude))
        {
            #ifdef DEBUG_MESSAGE_MAP_LOAD
            DebugClass::debugConsole(QStringLiteral("load the map: %1").arg(fileName));
            #endif
            QString sortFileName=fileName;
            sortFileName.remove(tmxRemove);
            map_name_to_do_id << sortFileName;
            if(map_temp.tryLoadMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName))
            {
                switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
                {
                    case MapVisibilityAlgorithmSelection_Simple:
                        flat_map_list_temp << new Map_server_MapVisibility_Simple_StoreOnSender;
                    break;
                    case MapVisibilityAlgorithmSelection_WithBorder:
                        flat_map_list_temp << new Map_server_MapVisibility_WithBorder_StoreOnSender;
                    break;
                    case MapVisibilityAlgorithmSelection_None:
                    default:
                        flat_map_list_temp << new MapServer;
                    break;
                }
                MapServer *mapServer=static_cast<MapServer *>(flat_map_list_temp.last());
                GlobalServerData::serverPrivateVariables.map_list[sortFileName]=mapServer;

                mapServer->width			= map_temp.map_to_send.width;
                mapServer->height			= map_temp.map_to_send.height;
                mapServer->parsed_layer	= map_temp.map_to_send.parsed_layer;
                mapServer->map_file		= sortFileName;

                {
                    int index=0;
                    while(index<map_temp.map_to_send.items.size())
                    {
                        const Map_to_send::ItemOnMap_Semi &item=map_temp.map_to_send.items.at(index);

                        quint16 itemDbCode;
                        if(GlobalServerData::serverPrivateVariables.dictionary_item.contains(sortFileName)
                                && GlobalServerData::serverPrivateVariables.dictionary_item.value(sortFileName).contains(QPair<quint8/*x*/,quint8/*y*/>(item.point.x,item.point.y)))
                            itemDbCode=GlobalServerData::serverPrivateVariables.dictionary_item.value(sortFileName).value(QPair<quint8,quint8>(item.point.x,item.point.y));
                        else
                        {
                            GlobalServerData::serverPrivateVariables.dictionary_item_maxId++;
                            QString queryText;
                            switch(GlobalServerData::serverSettings.database.type)
                            {
                                default:
                                case ServerSettings::Database::DatabaseType_Mysql:
                                    queryText=QStringLiteral("INSERT INTO `dictionary_itemonmap`(`id`,`map`,`x`,`y`) VALUES(%1,'%2',%3,%4);")
                                            .arg(GlobalServerData::serverPrivateVariables.dictionary_item_maxId)
                                            .arg(CatchChallenger::SqlFunction::quoteSqlVariable(sortFileName))
                                            .arg(item.point.x)
                                            .arg(item.point.y)
                                            ;
                                break;
                                case ServerSettings::Database::DatabaseType_SQLite:
                                    queryText=QStringLiteral("INSERT INTO dictionary_itemonmap(id,map,x,y) VALUES(%1,'%2',%3,%4);")
                                            .arg(GlobalServerData::serverPrivateVariables.dictionary_item_maxId)
                                            .arg(CatchChallenger::SqlFunction::quoteSqlVariable(sortFileName))
                                            .arg(item.point.x)
                                            .arg(item.point.y)
                                            ;
                                break;
                                case ServerSettings::Database::DatabaseType_PostgreSQL:
                                    queryText=QStringLiteral("INSERT INTO dictionary_itemonmap(id,map,x,y) VALUES(%1,'%2',%3,%4);")
                                            .arg(GlobalServerData::serverPrivateVariables.dictionary_item_maxId)
                                            .arg(CatchChallenger::SqlFunction::quoteSqlVariable(sortFileName))
                                            .arg(item.point.x)
                                            .arg(item.point.y)
                                            ;
                                break;
                            }
                            if(!GlobalServerData::serverPrivateVariables.db.asyncWrite(queryText.toLatin1()))
                            {
                                qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
                                abort();//stop because can't resolv the name
                            }
                            while(GlobalServerData::serverPrivateVariables.dictionary_item_reverse.size()<GlobalServerData::serverPrivateVariables.dictionary_item_maxId)
                                GlobalServerData::serverPrivateVariables.dictionary_item_reverse << 255/*-1*/;
                            GlobalServerData::serverPrivateVariables.dictionary_item_reverse << indexOfItemOnMap;
                            GlobalServerData::serverPrivateVariables.dictionary_item[sortFileName][QPair<quint8,quint8>(item.point.x,item.point.y)]=indexOfItemOnMap;

                            itemDbCode=GlobalServerData::serverPrivateVariables.dictionary_item_maxId;
                        }

                        MapServer::ItemOnMap itemOnMap;
                        itemOnMap.infinite=item.infinite;
                        itemOnMap.item=item.item;
                        itemOnMap.itemIndexOnMap=indexOfItemOnMap;
                        itemOnMap.itemDbCode=itemDbCode;
                        mapServer->itemsOnMap[QPair<quint8,quint8>(item.point.x,item.point.y)]=itemOnMap;
                        while(GlobalServerData::serverPrivateVariables.dictionary_item_reverse.size()<=itemOnMap.itemDbCode)
                            GlobalServerData::serverPrivateVariables.dictionary_item_reverse << 255/*-1*/;
                        GlobalServerData::serverPrivateVariables.dictionary_item_reverse[itemOnMap.itemDbCode]=indexOfItemOnMap;
                        indexOfItemOnMap++;
                        index++;
                    }
                }

                map_name << sortFileName;

                parseJustLoadedMap(map_temp.map_to_send,fileName);

                Map_semi map_semi;
                map_semi.map				= GlobalServerData::serverPrivateVariables.map_list.value(sortFileName);

                if(!map_temp.map_to_send.border.top.fileName.isEmpty())
                {
                    map_semi.border.top.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.top.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.top.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.top.x_offset		= map_temp.map_to_send.border.top.x_offset;
                }
                if(!map_temp.map_to_send.border.bottom.fileName.isEmpty())
                {
                    map_semi.border.bottom.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.bottom.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.bottom.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.bottom.x_offset		= map_temp.map_to_send.border.bottom.x_offset;
                }
                if(!map_temp.map_to_send.border.left.fileName.isEmpty())
                {
                    map_semi.border.left.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.left.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.left.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.left.y_offset		= map_temp.map_to_send.border.left.y_offset;
                }
                if(!map_temp.map_to_send.border.right.fileName.isEmpty())
                {
                    map_semi.border.right.fileName		= Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.border.right.fileName,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_semi.border.right.fileName.remove(BaseServer::text_dottmx);
                    map_semi.border.right.y_offset		= map_temp.map_to_send.border.right.y_offset;
                }

                sub_index=0;
                const int &listsize=map_temp.map_to_send.teleport.size();
                while(sub_index<listsize)
                {
                    map_temp.map_to_send.teleport[sub_index].map=Map_loader::resolvRelativeMap(GlobalServerData::serverPrivateVariables.datapack_mapPath+fileName,map_temp.map_to_send.teleport.at(sub_index).map,GlobalServerData::serverPrivateVariables.datapack_mapPath);
                    map_temp.map_to_send.teleport[sub_index].map.remove(BaseServer::text_dottmx);
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
    {
        GlobalServerData::serverPrivateVariables.flat_map_list=static_cast<CommonMap **>(malloc(sizeof(CommonMap *)*flat_map_list_temp.size()));
        int index=0;
        while(index<flat_map_list_temp.size())
        {
            GlobalServerData::serverPrivateVariables.flat_map_list[index]=flat_map_list_temp.at(index);
            index++;
        }
    }

    map_name_to_do_id.sort();

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
            QString teleportString=semi_loaded_map.value(index).old_map.teleport.at(sub_index).map;
            teleportString.remove(BaseServer::text_dottmx);
            if(GlobalServerData::serverPrivateVariables.map_list.contains(teleportString))
            {
                if(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x<GlobalServerData::serverPrivateVariables.map_list.value(teleportString)->width
                        && semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y<GlobalServerData::serverPrivateVariables.map_list.value(teleportString)->height)
                {
                    int virtual_position=semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x+semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y*semi_loaded_map.value(index).map->width;
                    if(semi_loaded_map.value(index).map->teleporter.contains(virtual_position))
                    {
                        DebugClass::debugConsole(QStringLiteral("already found teleporter on the map: %1(%2,%3), to %4 (%5,%6)")
                             .arg(semi_loaded_map.value(index).map->map_file)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                             .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                             .arg(teleportString)
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
                                     .arg(teleportString)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
                        #endif
                        CommonMap::Teleporter *teleporter=&semi_loaded_map[index].map->teleporter[virtual_position];
                        teleporter->map=GlobalServerData::serverPrivateVariables.map_list.value(teleportString);
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
                         .arg(teleportString)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                         .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));
            }
            else
                DebugClass::debugConsole(QStringLiteral("wrong teleporter on the map: %1(%2,%3), to %4 (%5,%6) because the map is not found")
                     .arg(semi_loaded_map.value(index).map->map_file)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).source_y)
                     .arg(teleportString)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_x)
                     .arg(semi_loaded_map.value(index).old_map.teleport.at(sub_index).destination_y));

            sub_index++;
        }
        index++;
    }

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

    //nead be after the offet
    preload_the_bots(semi_loaded_map);

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

    size=map_name_to_do_id.size();
    index=0;
    while(index<size)
    {
        if(GlobalServerData::serverPrivateVariables.map_list.contains(map_name_to_do_id.at(index)))
        {
            GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id=index;
            GlobalServerData::serverPrivateVariables.id_map_to_map[GlobalServerData::serverPrivateVariables.map_list[map_name_to_do_id.at(index)]->id]=map_name_to_do_id.at(index);
        }
        else
            abort();
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
    Client::extensionAllowed=extensionAllowedTemp.toSet();
    QStringList compressedExtensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_COMPRESSED).split(BaseServer::text_dotcomma);
    Client::compressedExtension=compressedExtensionAllowedTemp.toSet();
    Client::datapack_list_cache_timestamp=0;

    if(GlobalServerData::serverSettings.datapackCache==0)
        Client::datapack_file_list_cache=Client::datapack_file_list();

    QCryptographicHash hash(QCryptographicHash::Sha224);
    QStringList datapack_file_temp=Client::datapack_file_list().keys();
    datapack_file_temp.sort();
    int index=0;
    while(index<datapack_file_temp.size()) {
        QFile file(GlobalServerData::serverSettings.datapack_basePath+datapack_file_temp.at(index));
        if(datapack_file_temp.at(index).contains(GlobalServerData::serverPrivateVariables.datapack_rightFileName))
        {
            if(file.open(QIODevice::ReadOnly))
            {
                const QByteArray &data=file.readAll();
                {
                    QCryptographicHash hashFile(QCryptographicHash::Sha224);
                    hashFile.addData(data);
                    Client::DatapackCacheFile cacheFile;
                    cacheFile.mtime=QFileInfo(file).lastModified().toTime_t();
                    cacheFile.partialHash=*reinterpret_cast<const int *>(hashFile.result().constData());
                    Client::datapack_file_hash_cache[datapack_file_temp.at(index)]=cacheFile;
                }
                hash.addData(data);
                file.close();
            }
            else
            {
                DebugClass::debugConsole(QStringLiteral("Stop now! Unable to open the file %1 to do the datapack checksum for the mirror").arg(file.fileName()));
                abort();
            }
        }
        else
            DebugClass::debugConsole(QStringLiteral("File excluded because don't match the regex: %1").arg(file.fileName()));
        index++;
    }
    CommonSettings::commonSettings.datapackHash=hash.result();
    DebugClass::debugConsole(QStringLiteral("%1 file for datapack loaded").arg(datapack_file_temp.size()));
}

void BaseServer::preload_the_players()
{
    Client::simplifiedIdList.clear();
    Client::marketObjectIdList.reserve(GlobalServerData::serverSettings.max_players);
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        default:
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        {
            int index=0;
            while(index<GlobalServerData::serverSettings.max_players)
            {
                Client::simplifiedIdList << index;
                index++;
            }
        }
        break;
        case MapVisibilityAlgorithmSelection_None:
        break;
    }
}

void BaseServer::preload_the_visibility_algorithm()
{
    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
        case MapVisibilityAlgorithmSelection_WithBorder:
        if(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove!=NULL)
        {
            #ifndef EPOLLCATCHCHALLENGERSERVER
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->stop();
            GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->deleteLater();
            #else
            delete GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove;
            #endif
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new QTimer();
        #else
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove=new TimerSendInsertMoveRemove();
        #endif
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->start(CATCHCHALLENGER_SERVER_MAP_TIME_TO_SEND_MOVEMENT);
        break;
        case MapVisibilityAlgorithmSelection_None:
        default:
        break;
    }

    switch(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm)
    {
        case MapVisibilityAlgorithmSelection_Simple:
            DebugClass::debugConsole(QStringLiteral("Visibility: MapVisibilityAlgorithmSelection_Simple"));
        break;
        case MapVisibilityAlgorithmSelection_WithBorder:
            DebugClass::debugConsole(QStringLiteral("Visibility: MapVisibilityAlgorithmSelection_WithBorder"));
        break;
        case MapVisibilityAlgorithmSelection_None:
            DebugClass::debugConsole(QStringLiteral("Visibility: MapVisibilityAlgorithmSelection_None"));
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
            if(!bot_Semi.file.endsWith(BaseServer::text_dotxml))
                bot_Semi.file+=BaseServer::text_dotxml;
            loadBotFile(semi_loaded_map.value(index).map->map_file,bot_Semi.file);
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
                                else if(!CommonDatapack::commonDatapack.shops.contains(shop))
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
                                const quint32 &fightid=step.attribute(BaseServer::text_fightid).toUInt(&ok);
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

                                            quint32 fightRange=5;
                                            if(bot_Semi.property_text.contains(BaseServer::text_fightRange))
                                            {
                                                fightRange=bot_Semi.property_text.value(BaseServer::text_fightRange).toUInt(&ok);
                                                if(!ok)
                                                {
                                                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("fightRange is not a number at %1 (%2,%3): %4")
                                                        .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y)
                                                        .arg(bot_Semi.property_text.value(BaseServer::text_fightRange).toString()));
                                                    fightRange=5;
                                                }
                                                else
                                                {
                                                    if(fightRange>10)
                                                    {
                                                        CatchChallenger::DebugClass::debugConsole(QStringLiteral("fightRange is greater than 10 at %1 (%2,%3): %4")
                                                            .arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y)
                                                            .arg(fightRange)
                                                            );
                                                        fightRange=5;
                                                    }
                                                }
                                            }
                                            //load the botsFightTrigger
                                            #ifdef DEBUG_MESSAGE_CLIENT_FIGHT_BOT
                                            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(semi_loaded_map.value(index).map->map_file).arg(bot_Semi.point.x).arg(bot_Semi.point.y).arg(direction));
                                            #endif
                                            quint8 temp_x=bot_Semi.point.x,temp_y=bot_Semi.point.y;
                                            quint32 index_botfight_range=0;
                                            CatchChallenger::CommonMap *map=semi_loaded_map.value(index).map;
                                            CatchChallenger::CommonMap *old_map=map;
                                            while(index_botfight_range<fightRange)
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

void BaseServer::preload_finish()
{
    DebugClass::debugConsole(QStringLiteral("%1 SQL market item").arg(GlobalServerData::serverPrivateVariables.marketItemList.size()));
    qDebug() << QStringLiteral("Loaded the server SQL datapack into %1ms").arg(timeDatapack.elapsed());
}

void BaseServer::load_next_city_capture()
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    GlobalServerData::serverPrivateVariables.time_city_capture=FacilityLib::nextCaptureTime(GlobalServerData::serverSettings.city);
    const qint64 &time=GlobalServerData::serverPrivateVariables.time_city_capture.toMSecsSinceEpoch()-QDateTime::currentMSecsSinceEpoch();
    GlobalServerData::serverPrivateVariables.timer_city_capture->start(time);
    #endif
}

void BaseServer::parseJustLoadedMap(const Map_to_send &,const QString &)
{
}

bool BaseServer::initialize_the_database()
{
    if(GlobalServerData::serverPrivateVariables.db.isConnected())
    {
        DebugClass::debugConsole(QStringLiteral("Disconnected to %1 at %2")
                                 .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                 .arg(GlobalServerData::serverSettings.database.mysql.host)
                                 );
        GlobalServerData::serverPrivateVariables.db.syncDisconnect();
    }
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        DebugClass::debugConsole(QStringLiteral("database type unknown"));
        return false;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case ServerSettings::Database::DatabaseType_Mysql:
        if(!GlobalServerData::serverPrivateVariables.db.syncConnectMysql(
                    GlobalServerData::serverSettings.database.mysql.host.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.db.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.login.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.pass.toLatin1()
                    ))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to connect to the database: %1").arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
            return false;
        }
        else
            DebugClass::debugConsole(QStringLiteral("Connected to %1 at %2")
                                     .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                     .arg(GlobalServerData::serverSettings.database.mysql.host));
        GlobalServerData::serverPrivateVariables.db_type_string=QLatin1Literal("mysql");
        break;

        case ServerSettings::Database::DatabaseType_SQLite:
        if(!GlobalServerData::serverPrivateVariables.db.syncConnectSqlite(GlobalServerData::serverSettings.database.sqlite.file.toLatin1()))
        {
            DebugClass::debugConsole(QStringLiteral("Unable to connect to the database: %1").arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
            return false;
        }
        else
            DebugClass::debugConsole(QStringLiteral("SQLite db %1 open").arg(GlobalServerData::serverSettings.database.sqlite.file));
        GlobalServerData::serverPrivateVariables.db_type_string=QLatin1Literal("sqlite");
        break;
        #endif

        case ServerSettings::Database::DatabaseType_PostgreSQL:
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(!GlobalServerData::serverPrivateVariables.db.syncConnectPostgresql(
                    GlobalServerData::serverSettings.database.mysql.host.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.db.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.login.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.pass.toLatin1()
                    ))
        #else
        if(!GlobalServerData::serverPrivateVariables.db.syncConnect(
                    GlobalServerData::serverSettings.database.mysql.host.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.db.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.login.toLatin1(),
                    GlobalServerData::serverSettings.database.mysql.pass.toLatin1()
                    ))
        #endif
        {
            DebugClass::debugConsole(QStringLiteral("Unable to connect to the database: %1").arg(GlobalServerData::serverPrivateVariables.db.errorMessage()));
            return false;
        }
        else
            DebugClass::debugConsole(QStringLiteral("Connected to %1 at %2")
                                     .arg(GlobalServerData::serverPrivateVariables.db_type_string)
                                     .arg(GlobalServerData::serverSettings.database.mysql.host));
        GlobalServerData::serverPrivateVariables.db_type_string=QLatin1Literal("postgresql");
        break;
    }
    initialize_the_database_prepared_query();
    return true;
}

void BaseServer::initialize_the_database_prepared_query()
{
    switch(GlobalServerData::serverSettings.database.type)
    {
        default:
        return;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        case ServerSettings::Database::DatabaseType_Mysql:
        GlobalServerData::serverPrivateVariables.db_query_select_allow=QStringLiteral("SELECT `allow` FROM `character_allow` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_login=QStringLiteral("SELECT `id`,LOWER(HEX(`password`)) FROM `account` WHERE `login`=UNHEX('%1')");
        GlobalServerData::serverPrivateVariables.db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,UNHEX('%2'),UNHEX('%3'),%4)");
        GlobalServerData::serverPrivateVariables.db_query_characters=QStringLiteral("SELECT `id`,`pseudo`,`skin`,`time_to_delete`,`played_time`,`last_connect`,`map` FROM `character` WHERE `account`=%1 LIMIT 0,%2");
        GlobalServerData::serverPrivateVariables.db_query_played_time=QStringLiteral("UPDATE `character` SET `played_time`=`played_time`+%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_monster_skill=QStringLiteral("UPDATE `monster_skill` SET `endurance`=%1 WHERE `monster`=%2 AND `skill`=%3");
        GlobalServerData::serverPrivateVariables.db_query_character_by_id=QStringLiteral("SELECT `account`,`pseudo`,`skin`,`x`,`y`,`orientation`,`map`,`type`,`clan`,`cash`,`rescue_map`,`rescue_x`,`rescue_y`,`rescue_orientation`,`unvalidated_rescue_map`,`unvalidated_rescue_x`,`unvalidated_rescue_y`,`unvalidated_rescue_orientation`,`warehouse_cash`,`clan_leader`,`market_cash`,`time_to_delete` FROM `character` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete=QStringLiteral("UPDATE `character` SET `time_to_delete`=0 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect=QStringLiteral("UPDATE `character` SET `last_connect`=%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_clan=QStringLiteral("SELECT `name`,`cash` FROM `clan` WHERE `id`=%1");

        GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id=QStringLiteral("SELECT `id` FROM `monster` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff=QStringLiteral("DELETE FROM `monster_buff` WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill=QStringLiteral("DELETE FROM `monster_skill` WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM `bot_already_beaten` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_character=QStringLiteral("DELETE FROM `character` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item=QStringLiteral("DELETE FROM `item` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM `item_warehouse` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_market=QStringLiteral("DELETE FROM `item_market` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_character=QStringLiteral("DELETE FROM `monster` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_character=QStringLiteral("DELETE FROM `monster_warehouse` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM `monster_market` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id=QStringLiteral("DELETE FROM `monster` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_id=QStringLiteral("DELETE FROM `monster_warehouse` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM `monster_market` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant=QStringLiteral("DELETE FROM `plant` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant_by_id=QStringLiteral("DELETE FROM `plant` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_quest=QStringLiteral("DELETE FROM `quest` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_recipes=QStringLiteral("DELETE FROM `recipe` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_reputation=QStringLiteral("DELETE FROM `reputation` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_allow=QStringLiteral("DELETE FROM `character_allow` WHERE `character`=%1");

        GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo=QStringLiteral("SELECT `id` FROM `character` WHERE `pseudo`='%1'");
        GlobalServerData::serverPrivateVariables.db_query_select_clan_by_name=QStringLiteral("SELECT `id` FROM `clan` WHERE `name`='%1'");
        if(CommonSettings::commonSettings.useSP)
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`sp`=%6,`position`=%7 WHERE `id`=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO `monster_warehouse`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO `monster_market`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`market_price`) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster_warehouse` WHERE `character`=%1 ORDER BY `position` ASC");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4,`sp`=%5 WHERE `id`=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO `%3`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`sp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE `monster` SET `xp`=%2,`sp`=%3 WHERE `id`=%1");
        }
        else
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE `monster` SET `hp`=%3,`xp`=%4,`level`=%5,`position`=%6 WHERE `id`=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO `monster`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO `monster_warehouse`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO `monster_market`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`market_price`) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster` WHERE `character`=%1 ORDER BY `position` ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT `id`,`hp`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin` FROM `monster_warehouse` WHERE `character`=%1 ORDER BY `position` ASC");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE `monster` SET `hp`=%2,`xp`=%3,`level`=%4 WHERE `id`=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO `%3`(`id`,`hp`,`character`,`monster`,`level`,`xp`,`captured_with`,`gender`,`egg_step`,`character_origin`,`position`) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE `monster` SET `xp`=%2 WHERE `id`=%1");
        }
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill=QStringLiteral("INSERT INTO `monster_skill`(`monster`,`skill`,`level`,`endurance`) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item=QStringLiteral("INSERT INTO `item`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_warehouse=QStringLiteral("INSERT INTO `item_warehouse`(`item`,`character`,`quantity`) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_market=QStringLiteral("INSERT INTO `item_market`(`item`,`character`,`quantity`,`market_price`) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT `account`,`time_to_delete` FROM `character` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE `character` SET `time_to_delete`=%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id=QStringLiteral("SELECT `type`,`point`,`level` FROM `reputation` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id=QStringLiteral("SELECT `quest`,`finish_one_time`,`step` FROM `quest` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id=QStringLiteral("SELECT `recipe` FROM `recipe` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id=QStringLiteral("SELECT `item`,`quantity` FROM `item` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT `item`,`quantity` FROM `item_warehouse` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id=QStringLiteral("SELECT `skill`,`level`,`endurance` FROM `monster_skill` WHERE `monster`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id=QStringLiteral("SELECT `buff`,`level` FROM `monster_buff` WHERE `monster`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten=QStringLiteral("SELECT `botfight_id` FROM `bot_already_beaten` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_itemOnMap=QStringLiteral("SELECT `itemOnMap` FROM `character_itemonmap` WHERE `character`=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_itemonmap=QStringLiteral("INSERT INTO `character_itemonmap`(`character`,`itemOnMap`) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_change_right=QStringLiteral("UPDATE `character` SET `type`=%2 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_item=QStringLiteral("UPDATE `item` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_warehouse=QStringLiteral("UPDATE `item_warehouse` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_item=QStringLiteral("DELETE FROM `item` WHERE `item`=%1 AND `character`=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_warehouse=QStringLiteral("DELETE FROM `item_warehouse` WHERE `item`=%1 AND `character`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_cash=QStringLiteral("UPDATE `character` SET `cash`=%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_warehouse_cash=QStringLiteral("UPDATE `character` SET `warehouse_cash`=%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_recipe=QStringLiteral("INSERT INTO `recipe`(`character`,`recipe`) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_factory=QStringLiteral("INSERT INTO `factory`(`id`,`resources`,`products`,`last_update`) VALUES(%1,'%2','%3',%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_factory=QStringLiteral("UPDATE `factory` SET `resources`='%2',`products`='%3',`last_update`=%4 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_character_allow=QStringLiteral("INSERT INTO `character_allow`(`character`,`allow`) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_delete_character_allow=QStringLiteral("DELETE FROM `character_allow` WHERE `character`=%1 AND `allow`=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_reputation=QStringLiteral("INSERT INTO `reputation`(`character`,`type`,`point`,`level`) VALUES(%1,'%2',%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_reputation=QStringLiteral("UPDATE `reputation` SET `point`=%3,`level`=%4 WHERE `character`=%1 AND `type`='%2'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan=QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_and_leader=QStringLiteral("UPDATE `character` SET `clan`=%1,`clan_leader`=%2 WHERE `id`=%3;");
        GlobalServerData::serverPrivateVariables.db_query_delete_clan=QStringLiteral("DELETE FROM `clan` WHERE `id`=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_city=QStringLiteral("DELETE FROM `city` WHERE `city`='%1'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE `character` SET `clan`=0 WHERE `pseudo`=%1 AND `clan`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_hp_only=QStringLiteral("UPDATE `monster` SET `hp`=%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_sp_only=QStringLiteral("UPDATE `monster` SET `sp`=%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_skill_level=QStringLiteral("UPDATE `monster_skill` SET `level`=%1 WHERE `monster`=%2 AND `skill`=%3");
        GlobalServerData::serverPrivateVariables.db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO `bot_already_beaten`(`character`,`botfight_id`) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_buff=QStringLiteral("INSERT INTO `monster_buff`(`monster`,`buff`,`level`) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level=QStringLiteral("UPDATE `monster` SET `level`=%3 WHERE `monster`=%1 AND `buff`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_position=QStringLiteral("UPDATE `monster` SET `position`=%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_and_hp=QStringLiteral("UPDATE `monster` SET `hp`=%1,`monster`=%2 WHERE `id`=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level_only=QStringLiteral("DELETE FROM `monster_buff` WHERE `monster`=%1 AND `buff`=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM `monster_skill` WHERE `monster`=%1 AND `skill`=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_clan=QStringLiteral("INSERT INTO `clan`(`id`,`name`,`date`) VALUES(%1,'%2',%3);");
        GlobalServerData::serverPrivateVariables.db_query_insert_plant=QStringLiteral("INSERT INTO `plant`(`id`,`map`,`x`,`y`,`plant`,`character`,`plant_timestamps`) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_owner=QStringLiteral("UPDATE `monster` SET `character`=%2 WHERE `id`=%1;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_finish=QStringLiteral("UPDATE `quest` SET `step`=0,`finish_one_time`=1 WHERE `character`=%1 AND `quest`=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_step=QStringLiteral("UPDATE `quest` SET `step`=%3 WHERE `character`=%1 AND `quest`=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_restart=QStringLiteral("UPDATE `quest` SET `step`=1 WHERE `character`=%1 AND `quest`=%2;");
        GlobalServerData::serverPrivateVariables.db_query_insert_quest=QStringLiteral("INSERT INTO `quest`(`character`,`quest`,`finish_one_time`,`step`) VALUES(%1,%2,%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_update_city_clan=QStringLiteral("UPDATE `city` SET `clan`=%1 WHERE `city`='%2';");
        GlobalServerData::serverPrivateVariables.db_query_insert_city=QStringLiteral("INSERT INTO `city`(`clan`,`city`) VALUES(%1,'%2');");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_market=QStringLiteral("DELETE FROM `item_market` WHERE `item`=%1 AND `character`=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market=QStringLiteral("UPDATE `item_market` SET `quantity`=%1 WHERE `item`=%2 AND `character`=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market_and_price=QStringLiteral("UPDATE `item_market` SET `quantity`=%1,`market_price`=%2 WHERE `item`=%3 AND `character`=%4;");
        GlobalServerData::serverPrivateVariables.db_query_update_charaters_market_cash=QStringLiteral("UPDATE `character` SET `market_cash`=`market_cash`+%1 WHERE `id`=%2");
        GlobalServerData::serverPrivateVariables.db_query_get_market_cash=QStringLiteral("UPDATE `character` SET `cash`=%1,`market_cash`=0 WHERE `id`=%2;");
        break;
        #endif



        #ifndef EPOLLCATCHCHALLENGERSERVER
        case ServerSettings::Database::DatabaseType_SQLite:
        GlobalServerData::serverPrivateVariables.db_query_select_allow=QStringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_login=QStringLiteral("SELECT id,password FROM account WHERE login='%1'");
        GlobalServerData::serverPrivateVariables.db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        GlobalServerData::serverPrivateVariables.db_query_characters=QStringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 LIMIT 0,%2");
        GlobalServerData::serverPrivateVariables.db_query_played_time=QStringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_monster_skill=QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        GlobalServerData::serverPrivateVariables.db_query_character_by_id=QStringLiteral("SELECT account,pseudo,skin,x,y,orientation,map,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,clan_leader,market_cash,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete=QStringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect=QStringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_clan=QStringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id=QStringLiteral("SELECT id FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_character=QStringLiteral("DELETE FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item=QStringLiteral("DELETE FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_market=QStringLiteral("DELETE FROM item_market WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_character=QStringLiteral("DELETE FROM monster_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM monster_market WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_character=QStringLiteral("DELETE FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_id=QStringLiteral("DELETE FROM monster_warehouse WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM monster_market WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id=QStringLiteral("DELETE FROM monster WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant=QStringLiteral("DELETE FROM plant WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant_by_id=QStringLiteral("DELETE FROM plant WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_quest=QStringLiteral("DELETE FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_recipes=QStringLiteral("DELETE FROM recipe WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_reputation=QStringLiteral("DELETE FROM reputation WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1");

        GlobalServerData::serverPrivateVariables.db_query_select_clan_by_name=QStringLiteral("SELECT id FROM clan WHERE name='%1'");
        GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo=QStringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        if(CommonSettings::commonSettings.useSP)
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster_warehouse(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO monster_market(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,market_price) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster_warehouse WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO %3(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster_warehouse(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO monster_market(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,market_price) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster_warehouse WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO %3(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill=QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item=QStringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_warehouse=QStringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_market=QStringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id=QStringLiteral("SELECT type,point,level FROM reputation WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id=QStringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id=QStringLiteral("SELECT recipe FROM recipe WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id=QStringLiteral("SELECT item,quantity FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten=QStringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_itemOnMap=QStringLiteral("SELECT \"itemOnMap\" FROM \"character_itemonmap\" WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_itemonmap=QStringLiteral("INSERT INTO \"character_itemonmap\"(character,\"itemOnMap\") VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_change_right=QStringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_item=QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_warehouse=QStringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_item=QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_cash=QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_warehouse_cash=QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_recipe=QStringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_factory=QStringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_factory=QStringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_character_allow=QStringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_delete_character_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_reputation=QStringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_reputation=QStringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan=QStringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_and_leader=QStringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        GlobalServerData::serverPrivateVariables.db_query_delete_clan=QStringLiteral("DELETE FROM clan WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_hp_only=QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_sp_only=QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_skill_level=QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        GlobalServerData::serverPrivateVariables.db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_buff=QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level=QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_position=QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_and_hp=QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level_only=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_clan=QStringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        GlobalServerData::serverPrivateVariables.db_query_insert_plant=QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_owner=QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_finish=QStringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_step=QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_restart=QStringLiteral("UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_insert_quest=QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_update_city_clan=QStringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        GlobalServerData::serverPrivateVariables.db_query_insert_city=QStringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_market=QStringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market=QStringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market_and_price=QStringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        GlobalServerData::serverPrivateVariables.db_query_update_charaters_market_cash=QStringLiteral("UPDATE character SET market_cash=market_cash+%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_get_market_cash=QStringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        break;
        #endif

        case ServerSettings::Database::DatabaseType_PostgreSQL:
        GlobalServerData::serverPrivateVariables.db_query_select_allow=QStringLiteral("SELECT allow FROM character_allow WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_login=QStringLiteral("SELECT id,password FROM account WHERE login='%1'");
        GlobalServerData::serverPrivateVariables.db_query_insert_login=QStringLiteral("INSERT INTO account(id,login,password,date) VALUES(%1,'%2','%3',%4)");
        GlobalServerData::serverPrivateVariables.db_query_characters=QStringLiteral("SELECT id,pseudo,skin,time_to_delete,played_time,last_connect,map FROM character WHERE account=%1 LIMIT %2");
        GlobalServerData::serverPrivateVariables.db_query_played_time=QStringLiteral("UPDATE character SET played_time=played_time+%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_monster_skill=QStringLiteral("UPDATE monster_skill SET endurance=%1 WHERE monster=%2 AND skill=%3");
        GlobalServerData::serverPrivateVariables.db_query_character_by_id=QStringLiteral("SELECT account,pseudo,skin,x,y,orientation,map,type,clan,cash,rescue_map,rescue_x,rescue_y,rescue_orientation,unvalidated_rescue_map,unvalidated_rescue_x,unvalidated_rescue_y,unvalidated_rescue_orientation,warehouse_cash,clan_leader,market_cash,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete=QStringLiteral("UPDATE character SET time_to_delete=0 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_last_connect=QStringLiteral("UPDATE character SET last_connect=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_clan=QStringLiteral("SELECT name,cash FROM clan WHERE id=%1");

        GlobalServerData::serverPrivateVariables.db_query_monster_by_character_id=QStringLiteral("SELECT id FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_bot_already_beaten=QStringLiteral("DELETE FROM bot_already_beaten WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_character=QStringLiteral("DELETE FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item=QStringLiteral("DELETE FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_all_item_market=QStringLiteral("DELETE FROM item_market WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_character=QStringLiteral("DELETE FROM monster_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_character=QStringLiteral("DELETE FROM monster_market WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_character=QStringLiteral("DELETE FROM monster WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_warehouse_by_id=QStringLiteral("DELETE FROM monster_warehouse WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_market_by_id=QStringLiteral("DELETE FROM monster_market WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_by_id=QStringLiteral("DELETE FROM monster WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant=QStringLiteral("DELETE FROM plant WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_plant_by_id=QStringLiteral("DELETE FROM plant WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_quest=QStringLiteral("DELETE FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_recipes=QStringLiteral("DELETE FROM recipe WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_reputation=QStringLiteral("DELETE FROM reputation WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1");

        GlobalServerData::serverPrivateVariables.db_query_select_clan_by_name=QStringLiteral("SELECT id FROM clan WHERE name='%1'");
        GlobalServerData::serverPrivateVariables.db_query_select_character_by_pseudo=QStringLiteral("SELECT id FROM character WHERE pseudo='%1'");
        if(CommonSettings::commonSettings.useSP)
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,sp=%6,position=%7 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2,%3,%4,%5,0,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster_warehouse(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO monster_market(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,market_price) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4,sp=%5 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,sp,captured_with,gender,egg_step,character_origin FROM monster_warehouse WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO %3(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2,sp=%3 WHERE id=%1");
        }
        else
        {
            GlobalServerData::serverPrivateVariables.db_query_monster=QStringLiteral("UPDATE monster SET hp=%3,xp=%4,level=%5,position=%6 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2,%3,%4,%5,0,%6,%7,0,%3,%8)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_full=QStringLiteral("INSERT INTO monster(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_warehouse_monster_full=QStringLiteral("INSERT INTO monster_warehouse(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_market=QStringLiteral("INSERT INTO monster_market(id,hp,character,monster,level,xp,sp,captured_with,gender,egg_step,character_origin,market_price) VALUES(%1,%2);");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp_hp_level=QStringLiteral("UPDATE monster SET hp=%2,xp=%3,level=%4 WHERE id=%1");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_select_monsters_warehouse_by_player_id=QStringLiteral("SELECT id,hp,monster,level,xp,captured_with,gender,egg_step,character_origin FROM monster_warehouse WHERE character=%1 ORDER BY position ASC");
            GlobalServerData::serverPrivateVariables.db_query_insert_monster_catch=QStringLiteral("INSERT INTO %3(id,hp,character,monster,level,xp,captured_with,gender,egg_step,character_origin,position) VALUES(%1,%2)");
            GlobalServerData::serverPrivateVariables.db_query_update_monster_xp=QStringLiteral("UPDATE monster SET xp=%2 WHERE id=%1");
        }
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_skill=QStringLiteral("INSERT INTO monster_skill(monster,skill,level,endurance) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item=QStringLiteral("INSERT INTO item(item,character,quantity) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_warehouse=QStringLiteral("INSERT INTO item_warehouse(item,character,quantity) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_insert_item_market=QStringLiteral("INSERT INTO item_market(item,character,quantity,market_price) VALUES(%1,%2,%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_account_time_to_delete_character_by_id=QStringLiteral("SELECT account,time_to_delete FROM character WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_time_to_delete_by_id=QStringLiteral("UPDATE character SET time_to_delete=%2 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_reputation_by_id=QStringLiteral("SELECT type,point,level FROM reputation WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_quest_by_id=QStringLiteral("SELECT quest,finish_one_time,step FROM quest WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_recipes_by_player_id=QStringLiteral("SELECT recipe FROM recipe WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_by_player_id=QStringLiteral("SELECT item,quantity FROM item WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_items_warehouse_by_player_id=QStringLiteral("SELECT item,quantity FROM item_warehouse WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersSkill_by_id=QStringLiteral("SELECT skill,level,endurance FROM monster_skill WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_monstersBuff_by_id=QStringLiteral("SELECT buff,level FROM monster_buff WHERE monster=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_bot_beaten=QStringLiteral("SELECT botfight_id FROM bot_already_beaten WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_select_itemOnMap=QStringLiteral("SELECT \"itemOnMap\" FROM \"character_itemonmap\" WHERE character=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_itemonmap=QStringLiteral("INSERT INTO \"character_itemonmap\"(character,\"itemOnMap\") VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_change_right=QStringLiteral("UPDATE \"character\" SET \"type\"=%2 WHERE \"id\"=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_item=QStringLiteral("UPDATE item SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_warehouse=QStringLiteral("UPDATE item_warehouse SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_item=QStringLiteral("DELETE FROM item WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_warehouse=QStringLiteral("DELETE FROM item_warehouse WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_cash=QStringLiteral("UPDATE character SET cash=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_warehouse_cash=QStringLiteral("UPDATE character SET warehouse_cash=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_recipe=QStringLiteral("INSERT INTO recipe(character,recipe) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_factory=QStringLiteral("INSERT INTO factory(id,resources,products,last_update) VALUES(%1,'%2','%3',%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_factory=QStringLiteral("UPDATE factory SET resources='%2',products='%3',last_update=%4 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_insert_character_allow=QStringLiteral("INSERT INTO character_allow(character,allow) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_delete_character_allow=QStringLiteral("DELETE FROM character_allow WHERE character=%1 AND allow=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_reputation=QStringLiteral("INSERT INTO reputation(character,type,point,level) VALUES(%1,'%2',%3,%4)");
        GlobalServerData::serverPrivateVariables.db_query_update_reputation=QStringLiteral("UPDATE reputation SET point=%3,level=%4 WHERE character=%1 AND type='%2'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan=QStringLiteral("UPDATE character SET clan=0 WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_and_leader=QStringLiteral("UPDATE character SET clan=%1,clan_leader=%2 WHERE id=%3;");
        GlobalServerData::serverPrivateVariables.db_query_delete_clan=QStringLiteral("DELETE FROM clan WHERE id=%1");
        GlobalServerData::serverPrivateVariables.db_query_delete_city=QStringLiteral("DELETE FROM city WHERE city='%1'");
        GlobalServerData::serverPrivateVariables.db_query_update_character_clan_by_pseudo=QStringLiteral("UPDATE character SET clan=0 WHERE pseudo=%1 AND clan=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_hp_only=QStringLiteral("UPDATE monster SET hp=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_sp_only=QStringLiteral("UPDATE monster SET sp=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_skill_level=QStringLiteral("UPDATE monster_skill SET level=%1 WHERE monster=%2 AND skill=%3");
        GlobalServerData::serverPrivateVariables.db_query_insert_bot_already_beaten=QStringLiteral("INSERT INTO bot_already_beaten(character,botfight_id) VALUES(%1,%2)");
        GlobalServerData::serverPrivateVariables.db_query_insert_monster_buff=QStringLiteral("INSERT INTO monster_buff(monster,buff,level) VALUES(%1,%2,%3)");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level=QStringLiteral("UPDATE monster SET level=%3 WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_position=QStringLiteral("UPDATE monster SET position=%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_and_hp=QStringLiteral("UPDATE monster SET hp=%1,monster=%2 WHERE id=%3");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_buff=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_level_only=QStringLiteral("DELETE FROM monster_buff WHERE monster=%1 AND buff=%2");
        GlobalServerData::serverPrivateVariables.db_query_delete_monster_specific_skill=QStringLiteral("DELETE FROM monster_skill WHERE monster=%1 AND skill=%2");
        GlobalServerData::serverPrivateVariables.db_query_insert_clan=QStringLiteral("INSERT INTO clan(id,name,date) VALUES(%1,'%2',%3);");
        GlobalServerData::serverPrivateVariables.db_query_insert_plant=QStringLiteral("INSERT INTO plant(id,map,x,y,plant,character,plant_timestamps) VALUES(%1,%2,%3,%4,%5,%6,%7);");
        GlobalServerData::serverPrivateVariables.db_query_update_monster_owner=QStringLiteral("UPDATE monster SET character=%2 WHERE id=%1;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_finish=QStringLiteral("UPDATE quest SET step=0,finish_one_time=1 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_step=QStringLiteral("UPDATE quest SET step=%3 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_update_quest_restart=QStringLiteral("UPDATE quest SET step=1 WHERE character=%1 AND quest=%2;");
        GlobalServerData::serverPrivateVariables.db_query_insert_quest=QStringLiteral("INSERT INTO quest(character,quest,finish_one_time,step) VALUES(%1,%2,%3,%4);");
        GlobalServerData::serverPrivateVariables.db_query_update_city_clan=QStringLiteral("UPDATE city SET clan=%1 WHERE city='%2';");
        GlobalServerData::serverPrivateVariables.db_query_insert_city=QStringLiteral("INSERT INTO city(clan,city) VALUES(%1,'%2');");
        GlobalServerData::serverPrivateVariables.db_query_delete_item_market=QStringLiteral("DELETE FROM item_market WHERE item=%1 AND character=%2");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market=QStringLiteral("UPDATE item_market SET quantity=%1 WHERE item=%2 AND character=%3");
        GlobalServerData::serverPrivateVariables.db_query_update_item_market_and_price=QStringLiteral("UPDATE item_market SET quantity=%1,market_price=%2 WHERE item=%3 AND character=%4;");
        GlobalServerData::serverPrivateVariables.db_query_update_charaters_market_cash=QStringLiteral("UPDATE character SET market_cash=market_cash+%1 WHERE id=%2");
        GlobalServerData::serverPrivateVariables.db_query_get_market_cash=QStringLiteral("UPDATE character SET cash=%1,market_cash=0 WHERE id=%2;");
        break;
    }
}

void BaseServer::loadBotFile(const QString &mapfile,const QString &file)
{
    if(botFiles.contains(file))
        return;
    botFiles[file];//create the entry
    QDomDocument domDocument;
    if(CommonDatapack::commonDatapack.xmlLoadedFile.contains(file))
        domDocument=CommonDatapack::commonDatapack.xmlLoadedFile.value(file);
    else
    {
        QFile botFile(file);
        if(!botFile.open(QIODevice::ReadOnly))
        {
            qDebug() << mapfile << botFile.fileName()+": "+botFile.errorString();
            return;
        }
        QByteArray xmlContent=botFile.readAll();
        botFile.close();
        QString errorStr;
        int errorLine,errorColumn;
        if (!domDocument.setContent(xmlContent, false, &errorStr,&errorLine,&errorColumn))
        {
            qDebug() << QStringLiteral("%1, Parse error at line %2, column %3: %4").arg(botFile.fileName()).arg(errorLine).arg(errorColumn).arg(errorStr);
            return;
        }
        CommonDatapack::commonDatapack.xmlLoadedFile[file]=domDocument;
    }
    bool ok;
    QDomElement root = domDocument.documentElement();
    if(root.tagName()!=BaseServer::text_bots)
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

    unload_dictionary();
    unload_market();
    unload_industries();
    unload_zone();
    unload_profile();
    unload_the_city_capture();
    unload_the_visibility_algorithm();
    unload_the_plant_on_map();
    unload_the_map();
    unload_the_bots();
    unload_monsters_drops();
    unload_the_skin();
    unload_the_datapack();
    unload_the_players();
    unload_the_static_data();
    unload_the_ddos();
    unload_the_events();
    unload_the_randomData();

    CommonDatapack::commonDatapack.unload();
}

void BaseServer::unload_profile()
{
    GlobalServerData::serverPrivateVariables.serverProfileList.clear();
}

void BaseServer::unload_dictionary()
{
    GlobalServerData::serverPrivateVariables.dictionary_allow.clear();
    GlobalServerData::serverPrivateVariables.dictionary_allow_reverse.clear();
    GlobalServerData::serverPrivateVariables.dictionary_map.clear();
    GlobalServerData::serverPrivateVariables.dictionary_reputation.clear();
    GlobalServerData::serverPrivateVariables.dictionary_skin.clear();
    GlobalServerData::serverPrivateVariables.dictionary_item.clear();
    GlobalServerData::serverPrivateVariables.dictionary_item_reverse.clear();
}

void BaseServer::unload_the_static_data()
{
    Client::simplifiedIdList.clear();
}

void BaseServer::unload_zone()
{
    GlobalServerData::serverPrivateVariables.captureFightIdList.clear();
    GlobalServerData::serverPrivateVariables.plantUsedId.clear();
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
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(GlobalServerData::serverPrivateVariables.timer_city_capture!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.timer_city_capture;
        GlobalServerData::serverPrivateVariables.timer_city_capture=NULL;
    }
    #endif
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
    if(GlobalServerData::serverPrivateVariables.flat_map_list!=NULL)
    {
        delete GlobalServerData::serverPrivateVariables.flat_map_list;
        GlobalServerData::serverPrivateVariables.flat_map_list=NULL;
    }
    botIdLoaded.clear();
}

void BaseServer::unload_the_skin()
{
    GlobalServerData::serverPrivateVariables.skinList.clear();
}

void BaseServer::unload_the_visibility_algorithm()
{
    if(GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove!=NULL)
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->stop();
        GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove->deleteLater();
        #else
        delete GlobalServerData::serverPrivateVariables.timer_to_send_insert_move_remove;
        #endif
    }
}

void BaseServer::unload_the_ddos()
{
    Client::generalChatDrop.clear();
    Client::clanChatDrop.clear();
    Client::privateChatDrop.clear();
}

void BaseServer::unload_the_events()
{
    GlobalServerData::serverPrivateVariables.events.clear();
    int index=0;
    while(index<GlobalServerData::serverPrivateVariables.timerEvents.size())
    {
        delete GlobalServerData::serverPrivateVariables.timerEvents.at(index);
        index++;
    }
    GlobalServerData::serverPrivateVariables.timerEvents.clear();
}

void BaseServer::unload_the_datapack()
{
    Client::compressedExtension.clear();
    Client::datapack_file_hash_cache.clear();
    Client::datapack_file_list_cache.clear();
}

void BaseServer::unload_the_players()
{
    Client::simplifiedIdList.clear();
}

void BaseServer::unload_the_randomData()
{
    #ifdef Q_OS_LINUX
    fclose(GlobalServerData::serverPrivateVariables.fpRandomFile);
    #endif
    GlobalServerData::serverPrivateVariables.tokenForAuthSize=0;
    GlobalServerData::serverPrivateVariables.randomData.clear();
}

void BaseServer::setSettings(const ServerSettings &settings)
{
    //load it
    GlobalServerData::serverSettings=settings;

    loadAndFixSettings();
}

ServerSettings BaseServer::getSettings() const
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

    if(GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue>9)
        GlobalServerData::serverSettings.ddos.computeAverageValueNumberOfValue=9;
    if(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval<1)
        GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval=1;

    if(CommonSettings::commonSettings.min_character<1)
        CommonSettings::commonSettings.min_character=1;
    if(CommonSettings::commonSettings.max_character<1)
        CommonSettings::commonSettings.max_character=1;
    if(CommonSettings::commonSettings.max_character<CommonSettings::commonSettings.min_character)
        CommonSettings::commonSettings.max_character=CommonSettings::commonSettings.min_character;
    if(CommonSettings::commonSettings.character_delete_time<=0)
        CommonSettings::commonSettings.character_delete_time=7*24*3600;
    if(CommonSettings::commonSettings.useSP)
    {
        if(CommonSettings::commonSettings.autoLearn)
        {
            qDebug() << "Auto-learn disable when SP enabled";
            CommonSettings::commonSettings.autoLearn=false;
        }
    }

    if(GlobalServerData::serverSettings.datapackCache<-1)
        GlobalServerData::serverSettings.datapackCache=-1;
    {
        QStringList newMirrorList;
        QRegularExpression httpMatch("^https?://.+$");
        const QStringList &mirrorList=CommonSettings::commonSettings.httpDatapackMirror.split(";");
        int index=0;
        while(index<mirrorList.size())
        {
            const QString &mirror=mirrorList.at(index);
            if(!mirror.contains(httpMatch))
            {}//qDebug() << "Mirror wrong: " << mirror.toLocal8Bit(); -> single player
            else
            {
                if(mirror.endsWith("/"))
                    newMirrorList << mirror;
                else
                    newMirrorList << mirror+"/";
            }
            index++;
        }
        CommonSettings::commonSettings.httpDatapackMirror=newMirrorList.join(";");
    }

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
    if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_Simple)
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

        /*do the coding part...if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime>GlobalServerData::serverSettings.mapVisibility.simple.max)
            GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime=GlobalServerData::serverSettings.mapVisibility.simple.max;*/
    }
    else if(GlobalServerData::serverSettings.mapVisibility.mapVisibilityAlgorithm==CatchChallenger::MapVisibilityAlgorithmSelection_WithBorder)
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
    {
        #ifndef EPOLLCATCHCHALLENGERSERVER
        GlobalServerData::serverPrivateVariables.positionSync.stop();
        #endif
    }
    else
    {
        GlobalServerData::serverPrivateVariables.positionSync.start(GlobalServerData::serverSettings.database.secondToPositionSync*1000);
    }
    GlobalServerData::serverPrivateVariables.ddosTimer.start(GlobalServerData::serverSettings.ddos.computeAverageValueTimeInterval*1000);

    switch(GlobalServerData::serverSettings.database.type)
    {
        case CatchChallenger::ServerSettings::Database::DatabaseType_SQLite:
        case CatchChallenger::ServerSettings::Database::DatabaseType_Mysql:
        case CatchChallenger::ServerSettings::Database::DatabaseType_PostgreSQL:
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

void BaseServer::load_clan_max_id()
{
    GlobalServerData::serverPrivateVariables.maxClanId=0;
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
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QLatin1String("SELECT id FROM clan ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_clan_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_industries();
    }
}

void BaseServer::load_clan_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_clan_max_id_return();
}

void BaseServer::load_clan_max_id_return()
{
    GlobalServerData::serverPrivateVariables.maxClanId=0;
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxClanId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxClanId=0;
            continue;
        }
    }
    preload_industries();
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
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QLatin1String("SELECT id FROM account ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_account_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        if(CommonSettings::commonSettings.max_character)
            load_character_max_id();
        else
            preload_dictionary_allow();
    }
    GlobalServerData::serverPrivateVariables.maxAccountId=0;
}

void BaseServer::load_account_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_account_max_id_return();
}

void BaseServer::load_account_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxAccountId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxAccountId=0;
            continue;
        }
    }
    if(CommonSettings::commonSettings.max_character)
        load_character_max_id();
    else
        preload_dictionary_allow();
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
        case ServerSettings::Database::DatabaseType_PostgreSQL:
            queryText=QLatin1String("SELECT id FROM character ORDER BY id DESC LIMIT 1;");
        break;
    }
    if(GlobalServerData::serverPrivateVariables.db.asyncRead(queryText.toLatin1(),this,&BaseServer::load_character_max_id_static)==NULL)
    {
        qDebug() << QStringLiteral("Sql error for: %1, error: %2").arg(queryText).arg(GlobalServerData::serverPrivateVariables.db.errorMessage());
        preload_dictionary_allow();
    }
    GlobalServerData::serverPrivateVariables.maxCharacterId=0;
}

void BaseServer::load_character_max_id_static(void *object)
{
    static_cast<BaseServer *>(object)->load_character_max_id_return();
}

void BaseServer::load_character_max_id_return()
{
    while(GlobalServerData::serverPrivateVariables.db.next())
    {
        bool ok;
        GlobalServerData::serverPrivateVariables.maxCharacterId=QString(GlobalServerData::serverPrivateVariables.db.value(0)).toUInt(&ok);
        if(!ok)
        {
            DebugClass::debugConsole(QStringLiteral("Max monster id is failed to convert to number"));
            GlobalServerData::serverPrivateVariables.maxCharacterId=0;
            continue;
        }
    }
    preload_dictionary_allow();
}

