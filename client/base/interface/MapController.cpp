#include "MapController.h"
#include "DatapackClientLoader.h"
#include "../Api_client_real.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/CommonDatapackServerSpec.h"
#include "../../../general/base/CommonSettingsServer.h"
#include "../../../general/base/GeneralStructures.h"

QString MapController::text_random=QStringLiteral("random");
QString MapController::text_loop=QStringLiteral("loop");
QString MapController::text_move=QStringLiteral("move");
QString MapController::text_left=QStringLiteral("left");
QString MapController::text_right=QStringLiteral("right");
QString MapController::text_top=QStringLiteral("top");
QString MapController::text_bottom=QStringLiteral("bottom");
QString MapController::text_slash=QStringLiteral("/");
QString MapController::text_type=QStringLiteral("type");
QString MapController::text_fightRange=QStringLiteral("fightRange");
QString MapController::text_fight=QStringLiteral("fight");
QString MapController::text_fightid=QStringLiteral("fightid");
QString MapController::text_bot=QStringLiteral("bot");
QString MapController::text_slashtrainerpng=QStringLiteral("/trainer.png");
QString MapController::text_DATAPACK_BASE_PATH_SKIN=QStringLiteral(DATAPACK_BASE_PATH_SKIN);

#define IMAGEOVERSIZEWITDH 800*2*2
#define IMAGEOVERSIZEHEIGHT 600*2*2


MapController::MapController(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &OpenGL) :
    MapControllerMP(centerOnPlayer,debugTags,useCache,OpenGL)
{
    connect(this,&MapController::mapDisplayed,this,&MapController::tryLoadPlantOnMapDisplayed,Qt::QueuedConnection);
    botFlags=NULL;

    imageOverAdded=false;
    updateColorIntervale=0;
    actualColor=Qt::transparent;
    tempColor=Qt::transparent;
    newColor=Qt::transparent;
    imageOver=new QGraphicsPixmapItem();
    imageOver->setZValue(1000);
    if(centerOnPlayer)
        imageOver->setPos(-800,-600);
    else
        imageOver->setPos(0,0);
    updateColorTimer.setSingleShot(true);
    updateColorTimer.setInterval(50);
    connect(&updateColorTimer,&QTimer::timeout,this,&MapController::updateColor);
    updateBotTimer.setInterval(1000);
    updateBotTimer.start();
    connect(&updateBotTimer,&QTimer::timeout,this,&MapController::updateBot);
}

MapController::~MapController()
{
    if(botFlags!=NULL)
    {
        delete botFlags;
        botFlags=NULL;
    }
}

bool MapController::asyncMapLoaded(const QString &fileName,MapVisualiserThread::Map_full * tempMapObject)
{
    if(MapControllerMP::asyncMapLoaded(fileName,tempMapObject))
    {
        if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer)
        {
            if(DatapackClientLoader::datapackLoader.plantOnMap.contains(fileName))
            {
                if(plantOnMap==NULL)
                    abort();
                const QHash<QPair<uint8_t,uint8_t>,uint16_t> &plantCoor=DatapackClientLoader::datapackLoader.plantOnMap.value(fileName);
                QHashIterator<QPair<uint8_t,uint8_t>,uint16_t> i(plantCoor);
                while (i.hasNext()) {
                    i.next();
                    const uint16_t indexOfMap=i.value();
                    if(plantOnMap->size()>1000000)
                        abort();
                    if(plantOnMap->find(indexOfMap)!=plantOnMap->cend())
                    {
                        const uint8_t &x=i.key().first;
                        const uint8_t &y=i.key().second;
                        const CatchChallenger::PlayerPlant &playerPlant=plantOnMap->at(indexOfMap);
                        uint32_t seconds_to_mature=0;
                        if(playerPlant.mature_at>(uint64_t)QDateTime::currentMSecsSinceEpoch()/1000)
                            seconds_to_mature=static_cast<uint32_t>(playerPlant.mature_at-QDateTime::currentMSecsSinceEpoch()/1000);
                        if(DatapackClientLoader::datapackLoader.fullMapPathToId.contains(fileName))
                            insert_plant(DatapackClientLoader::datapackLoader.fullMapPathToId.value(fileName),
                            static_cast<uint8_t>(x),static_cast<uint8_t>(y),playerPlant.plant,static_cast<uint16_t>(seconds_to_mature));
                        else
                            qDebug() << "!DatapackClientLoader::datapackLoader.fullMapPathToId.contains(plantIndexContent.map) for CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer";
                    }
                }
            }
        }
        return true;
    }
    else
        return false;
}

void MapController::loadPlayerFromCurrentMap()
{
    MapVisualiserPlayer::loadPlayerFromCurrentMap();
    //const QRectF &rect=mScene->sceneRect();
    //qreal x=rect.x()-800,y=rect.y()-600;
    //imageOver->setPos(QPointF(x,y));
    //imageOver->setPos(QPointF(-800,-600));
}

void MapController::updateBot()
{
    if(!player_informations_is_set)
        return;
    if(current_map.isEmpty())
        return;
    if(all_map.find(current_map)==all_map.cend())
        return;
    MapVisualiserThread::Map_full * currentMap=all_map.value(current_map);
    if(currentMap==NULL)
        return;
    QHash<QPair<uint8_t,uint8_t>,CatchChallenger::BotDisplay>::const_iterator i = currentMap->logicalMap.botsDisplay.constBegin();

    while (i != currentMap->logicalMap.botsDisplay.constEnd())
    {
        CatchChallenger::BotDisplay &botDisplay=currentMap->logicalMap.botsDisplay[i.key()];
        if(botDisplay.mapObject==playerMapObject)
        {
            ++i;
            continue;
        }
        if(botDisplay.tileset!=NULL && botDisplay.mapObject!=NULL)
        switch(botDisplay.botMove)
        {
            default:
            case CatchChallenger::BotMove::BotMove_Fixed:
            break;
            case CatchChallenger::BotMove::BotMove_Loop:
            {
                uint8_t baseTile=static_cast<uint8_t>(botDisplay.mapObject->cell().tile->id());
                switch(baseTile)
                {
                    default:
                    case 7:
                        baseTile=10;//left
                    break;
                    case 1:
                        baseTile=4;//right
                    break;
                    case 10:
                        baseTile=1;//top
                    break;
                    case 4:
                        baseTile=7;//bottom
                    break;
                }
                Tiled::Cell cell=botDisplay.mapObject->cell();
                cell.tile=botDisplay.tileset->tileAt(baseTile);
                botDisplay.mapObject->setCell(cell);
            }
            break;
            case CatchChallenger::BotMove::BotMove_Random:
            {
                uint8_t baseTile=0;
                switch(rand()%4)
                {
                    default:
                    case 0:
                        baseTile=10;//left
                    break;
                    case 1:
                        baseTile=4;//right
                    break;
                    case 2:
                        baseTile=1;//top
                    break;
                    case 3:
                        baseTile=7;//bottom
                    break;
                }
                Tiled::Cell cell=botDisplay.mapObject->cell();
                cell.tile=botDisplay.tileset->tileAt(baseTile);
                botDisplay.mapObject->setCell(cell);
            }
            break;
        }

        ++i;
    }
}

void MapController::connectAllSignals(CatchChallenger::Api_protocol *client)
{
    MapControllerMP::connectAllSignals(client);
    connect(client,&CatchChallenger::Api_client_real::insert_plant,this,&MapController::insert_plant);
    connect(client,&CatchChallenger::Api_client_real::remove_plant,this,&MapController::remove_plant);
    connect(client,&CatchChallenger::Api_client_real::seed_planted,this,&MapController::seed_planted);
    connect(client,&CatchChallenger::Api_client_real::plant_collected,this,&MapController::plant_collected);
}

void MapController::resetAll()
{
    setColor(Qt::transparent);
    delayedPlantInsert.clear();
    MapControllerMP::resetAll();
}

void MapController::datapackParsed()
{
    MapControllerMP::datapackParsed();
}

void MapController::datapackParsedMainSub()
{
    if(mHaveTheDatapack)
        return;
    MapControllerMP::datapackParsedMainSub();
    int index;

    index=0;
    while(index<delayedPlantInsert.size())
    {
        insert_plant(delayedPlantInsert.at(index).mapId,delayedPlantInsert.at(index).x,delayedPlantInsert.at(index).y,delayedPlantInsert.at(index).plant_id,delayedPlantInsert.at(index).seconds_to_mature);
        index++;
    }
    delayedPlantInsert.clear();
}

bool MapController::canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision)
{
    if(!MapVisualiserPlayerWithFight::canGoTo(direction,map,x,y,checkCollision))
        return false;
    CatchChallenger::CommonMap *new_map=&map;
    CatchChallenger::MoveOnTheMap::move(direction,&new_map,&x,&y,false);
    if(all_map.value(QString::fromStdString(new_map->map_file))->logicalMap.bots.contains(QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))))
        return false;
    return true;
}

void MapController::loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap,const uint32_t &botId,const uint8_t &x,const uint8_t &y,
                                    const QString &lookAt,const QString &skin)
{
    Q_UNUSED(botId);
    if(skin.isEmpty())
    {
        std::cerr << "MapController::loadBotOnTheMap() skin empty" << std::endl;
        return;
    }

    if(parsedMap->logicalMap.botsDisplay.contains(QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))))
    {
        /*CatchChallenger::BotDisplay *botDisplay=&parsedMap->logicalMap.botsDisplay[QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
        ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(botDisplay->mapObject);
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        botDisplay->mapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink.value(botDisplay->mapObject)->setZValue(y);*/

        std::cerr << "MapController::loadBotOnTheMap() parsedMap->logicalMap.botsDisplay.contains(QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y)))" << std::endl;
        return;
    }

    if(!ObjectGroupItem::objectGroupLink.contains(parsedMap->objectGroup))
    {
        qDebug() << QStringLiteral("loadBotOnTheMap(), ObjectGroupItem::objectGroupLink not contains parsedMap->objectGroup");
        return;
    }
    CatchChallenger::BotDisplay *botDisplay=&parsedMap->logicalMap.botsDisplay[QPair<uint8_t,uint8_t>(static_cast<uint8_t>(x),static_cast<uint8_t>(y))];
    botDisplay->botMove=CatchChallenger::BotMove::BotMove_Fixed;
    CatchChallenger::Direction direction;
    int baseTile=-1;
    if(lookAt==MapController::text_random || lookAt==MapController::text_loop || lookAt==MapController::text_move)
    {
        switch(rand()%4)
        {
            default:
            case 0:
                baseTile=10;
                direction=CatchChallenger::Direction_move_at_left;
            break;
            case 1:
                baseTile=4;
                direction=CatchChallenger::Direction_move_at_right;
            break;
            case 2:
                baseTile=1;
                direction=CatchChallenger::Direction_move_at_top;
            break;
            case 3:
                baseTile=7;
                direction=CatchChallenger::Direction_move_at_bottom;
            break;
        }
        if(lookAt==MapController::text_random)
            botDisplay->botMove=CatchChallenger::BotMove::BotMove_Random;
        else if(lookAt==MapController::text_loop)
            botDisplay->botMove=CatchChallenger::BotMove::BotMove_Loop;
        else
            botDisplay->botMove=CatchChallenger::BotMove::BotMove_Move;
    }
    else if(lookAt==MapController::text_left)
    {
        baseTile=10;
        direction=CatchChallenger::Direction_move_at_left;
    }
    else if(lookAt==MapController::text_right)
    {
        baseTile=4;
        direction=CatchChallenger::Direction_move_at_right;
    }
    else if(lookAt==MapController::text_top)
    {
        baseTile=1;
        direction=CatchChallenger::Direction_move_at_top;
    }
    else
    {
        if(lookAt!=MapController::text_bottom)
            qDebug() << (QStringLiteral("Wrong direction for the bot at %1 (%2,%3)").arg(QString::fromStdString(parsedMap->logicalMap.map_file)).arg(x).arg(y));
        baseTile=7;
        direction=CatchChallenger::Direction_move_at_bottom;
    }

    botDisplay->mapObject=new Tiled::MapObject();
    botDisplay->mapObject->setName("botDisplay");
    botDisplay->tileset=new Tiled::Tileset(MapController::text_bot,16,24);
    QString skinPath=datapackPath+MapController::text_DATAPACK_BASE_PATH_SKIN+MapController::text_slash+skin+MapController::text_slashtrainerpng;
    if(!QFile(skinPath).exists())
    {
        QDir folderList(QStringLiteral("%1/skin/").arg(datapackPath));
        const QStringList &entryList=folderList.entryList(QDir::Dirs|QDir::NoDotAndDotDot);
        int entryListIndex=0;
        while(entryListIndex<entryList.size())
        {
            skinPath=datapackPath+DATAPACK_BASE_PATH_SKINBASE+entryList.at(entryListIndex)+MapController::text_slash+skin+MapController::text_slashtrainerpng;
            if(QFile(skinPath).exists())
                break;
            entryListIndex++;
        }
    }
    //do a cache here?
    if(!QFile(skinPath).exists())
    {
        qDebug() << "Unable the load the bot tileset (not found):" << skinPath;
        if(!botDisplay->tileset->loadFromImage(QImage(QStringLiteral(":/images/player_default/trainer.png")),QStringLiteral(":/images/player_default/trainer.png")))
            qDebug() << "Unable the load the default bot tileset";
    }
    else if(!botDisplay->tileset->loadFromImage(QImage(skinPath),skinPath))
    {
        qDebug() << "Unable the load the bot tileset";
        if(!botDisplay->tileset->loadFromImage(QImage(QStringLiteral(":/images/player_default/trainer.png")),QStringLiteral(":/images/player_default/trainer.png")))
            qDebug() << "Unable the load the default bot tileset";
    }

    {
        Tiled::Cell cell=botDisplay->mapObject->cell();
        cell.tile=botDisplay->tileset->tileAt(baseTile);
        botDisplay->mapObject->setCell(cell);

        ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(botDisplay->mapObject);
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        botDisplay->mapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink.value(botDisplay->mapObject)->setZValue(y);
    }

    std::pair<uint8_t,uint8_t> pos(x,y);
    QPair<uint8_t,uint8_t> Qtpos(x,y);
    {
        //add flags
        if(botFlags==NULL)
        {
            botFlags=new Tiled::Tileset(QStringLiteral("botflags"),16,16);
            botFlags->loadFromImage(QImage(QStringLiteral(":/images/flags.png")),QStringLiteral(":/images/flags.png"));
            TemporaryTile::empty=botFlags->tileAt(15);
        }

        //fight collision bot
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("fight collision bot");
            botDisplay->temporaryTile=new TemporaryTile(flag);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.5));
            MapObjectItem::objectLink.value(flag)->setZValue(9999);
        }
        if(parsedMap->logicalMap.shops.find(pos)!=parsedMap->logicalMap.shops.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Shops");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(2);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        if(parsedMap->logicalMap.learn.find(pos)!=parsedMap->logicalMap.learn.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Learn");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(3);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        /*if(parsedMap->logicalMap.clan.find(pos)!=parsedMap->logicalMap.clan.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Clan");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(7);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }*/
        if(parsedMap->logicalMap.heal.find(pos)!=parsedMap->logicalMap.heal.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(0);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        if(parsedMap->logicalMap.market.find(pos)!=parsedMap->logicalMap.market.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Market");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(4);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        if(parsedMap->logicalMap.zonecapture.find(pos)!=parsedMap->logicalMap.zonecapture.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Zonecapture");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(6);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        /*if(parsedMap->logicalMap.industry.find(pos)!=parsedMap->logicalMap.industry.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("Industry");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(8);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }*/
        /* asked by tgjklmda if(parsedMap->logicalMap.botsFight.find(pos)!=parsedMap->logicalMap.botsFight.cend())
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            flag->setName("botsFight");
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(5);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }*/
    }

    if(parsedMap->logicalMap.bots.value(Qtpos).step.find(1)!=parsedMap->logicalMap.bots.value(Qtpos).step.cend())
    {
        auto stepBot=parsedMap->logicalMap.bots.value(Qtpos).step.at(1);
        if(stepBot->Attribute(std::string("type"))!=NULL && *stepBot->Attribute(std::string("type"))=="fight" && stepBot->Attribute(std::string("fightid"))!=NULL)
        {
            bool ok;
            //16Bit: \see CommonDatapackServerSpec, Map_to_send,struct Bot_Semi,uint16_t id
            const uint16_t fightid=stringtouint16(*stepBot->Attribute(std::string("fightid")),&ok);
            if(ok)
            {
                if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.find(fightid)!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.botFights.cend())
                {
                    #ifdef DEBUG_CLIENT_BOT
                    qDebug() << (QStringLiteral("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(parsedMap->logicalMap.map_file).arg(x).arg(y).arg(direction));
                    #endif

                    uint32_t fightRange=5;
                    if(parsedMap->logicalMap.bots.value(Qtpos).properties.find("fightRange")!=parsedMap->logicalMap.bots.value(Qtpos).properties.cend())
                    {
                        fightRange=stringtouint32(parsedMap->logicalMap.bots.value(Qtpos).properties.at("fightRange"),&ok);
                        if(!ok)
                        {
                            qDebug() << (QStringLiteral("fightRange is not a number at %1 (%2,%3): %4")
                                .arg(QString::fromStdString(parsedMap->logicalMap.map_file)).arg(x).arg(y)
                                .arg(QString::fromStdString(parsedMap->logicalMap.bots.value(Qtpos).properties.at("fightRange"))));
                            fightRange=5;
                        }
                        else
                        {
                            if(fightRange>10)
                            {
                                qDebug() << (QStringLiteral("fightRange is greater than 10 at %1 (%2,%3): %4")
                                    .arg(QString::fromStdString(parsedMap->logicalMap.map_file)).arg(x).arg(y)
                                    .arg(fightRange)
                                    );
                                fightRange=5;
                            }
                        }
                    }

                    uint8_t temp_x=x,temp_y=y;
                    uint32_t index_botfight_range=0;
                    CatchChallenger::CommonMap *map=&parsedMap->logicalMap;
                    CatchChallenger::CommonMap *old_map=map;
                    while(index_botfight_range<fightRange)
                    {
                        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                            break;
                        if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                            break;
                        if(map!=old_map)
                            break;
                        std::pair<uint8_t,uint8_t> temp_pos(temp_x,temp_y);
                        QPair<uint8_t,uint8_t> Qttemp_pos(temp_x,temp_y);
                        parsedMap->logicalMap.botsFightTrigger[temp_pos].push_back(fightid);
                        parsedMap->logicalMap.botsFightTriggerExtra.insert(Qttemp_pos,Qtpos);
                        index_botfight_range++;
                    }
                }
                else
                    qDebug() << QStringLiteral("No fightid %1 at MapController::loadBotOnTheMap").arg(fightid);
            }
            else
                qDebug() << QStringLiteral("fightid not a number: ") << QString::fromStdString(*stepBot->Attribute(std::string("fightid")));
        }
        else
            qDebug() << QStringLiteral("stepBot->Attribute(std::string(\"type\"))!=NULL && *stepBot->Attribute(std::string(\"type\"))==\"fight\" && stepBot->Attribute(std::string(\"fightid\"))!=NULL");
    }
    /*else
        qDebug() << QStringLiteral("parsedMap->logicalMap.bots.value(Qtpos).step.find(1)!=parsedMap->logicalMap.bots.value(Qtpos).step.cend()");*/
}

void MapController::setColor(const QColor &color, const uint32_t &timeInMS)
{
    /*QBrush brush;
    brush.setColor(QColor(45,85,111,150));
    QGraphicsRectItem *rect=mScene->addRect(0,0,8000,6000,QPen(),brush);
    rect->setZValue(1000);
    mScene->addItem(rect);*/
    updateColorTimer.stop();
    if(imageOverAdded)
    {
        imageOverAdded=false;
        mScene->removeItem(imageOver);
    }
    if(timeInMS<50)
    {
        updateColorIntervale=0;
        actualColor=color;
        tempColor=color;
        newColor=color;
        QPixmap pixmap(IMAGEOVERSIZEWITDH,IMAGEOVERSIZEHEIGHT);
        pixmap.fill(color);
        imageOver->setPixmap(pixmap);
        if(newColor.alpha()!=0)
            if(!imageOverAdded)
            {
                imageOverAdded=true;
                mScene->addItem(imageOver);
            }
    }
    else
    {
        newColor=color;
        updateColorIntervale=timeInMS/50;
        if(newColor.alpha()!=0 || tempColor.alpha()!=0)
            if(!imageOverAdded)
            {
                imageOverAdded=true;
                mScene->addItem(imageOver);
            }
        if(tempColor.alpha()==0)
            tempColor=QColor(color.red(),color.green(),color.blue(),0);
        if(newColor.alpha()==0)
            newColor=QColor(tempColor.red(),tempColor.green(),tempColor.blue(),0);
        actualColor=tempColor;
        updateColor();
    }
}

void MapController::updateColor()
{
    if(!player_informations_is_set)
        return;
    int rdiff;
    {
        rdiff=newColor.red()-actualColor.red();
        if(rdiff==0)
        {}
        else if(rdiff/(updateColorIntervale)!=0)
        {
            rdiff/=updateColorIntervale;
            if(rdiff>0)
            {
                if(rdiff>(newColor.red()-tempColor.red()))
                    rdiff=(newColor.red()-tempColor.red());
            }
            else
            {
                if(rdiff<(newColor.red()-tempColor.red()))
                    rdiff=(newColor.red()-tempColor.red());
            }
        }
        else
        {
            if(newColor.red()!=tempColor.red())
            {
                if(newColor.red()>tempColor.red())
                    rdiff=1;
                else
                    rdiff=-1;
            }
        }
    }
    int gdiff;
    {
        gdiff=newColor.green()-actualColor.green();
        if(gdiff==0)
        {}
        else if(gdiff/(updateColorIntervale)!=0)
        {
            gdiff/=updateColorIntervale;
            if(gdiff>0)
            {
                if(gdiff>(newColor.green()-tempColor.green()))
                    gdiff=(newColor.green()-tempColor.green());
            }
            else
            {
                if(gdiff<(newColor.green()-tempColor.green()))
                    gdiff=(newColor.green()-tempColor.green());
            }
        }
        else
        {
            if(newColor.green()!=tempColor.green())
            {
                if(newColor.green()>tempColor.green())
                    gdiff=1;
                else
                    gdiff=-1;
            }
        }
    }
    int bdiff;
    {
        bdiff=newColor.blue()-actualColor.blue();
        if(bdiff==0)
        {}
        else if(bdiff/(updateColorIntervale)!=0)
        {
            bdiff/=updateColorIntervale;
            if(bdiff>0)
            {
                if(bdiff>(newColor.blue()-tempColor.blue()))
                    bdiff=(newColor.blue()-tempColor.blue());
            }
            else
            {
                if(bdiff<(newColor.blue()-tempColor.blue()))
                    bdiff=(newColor.blue()-tempColor.blue());
            }
        }
        else
        {
            if(newColor.blue()!=tempColor.blue())
            {
                if(newColor.blue()>tempColor.blue())
                    bdiff=1;
                else
                    bdiff=-1;
            }
        }
    }
    int adiff;
    {
        adiff=newColor.alpha()-actualColor.alpha();
        if(adiff==0)
        {}
        else if(adiff/(updateColorIntervale)!=0)
        {
            adiff/=updateColorIntervale;
            if(adiff>0)
            {
                if(adiff>(newColor.alpha()-tempColor.alpha()))
                    adiff=(newColor.alpha()-tempColor.alpha());
            }
            else
            {
                if(adiff<(newColor.alpha()-tempColor.alpha()))
                    adiff=(newColor.alpha()-tempColor.alpha());
            }
        }
        else
        {
            if(newColor.alpha()!=tempColor.alpha())
            {
                if(newColor.alpha()>tempColor.alpha())
                    adiff=1;
                else
                    adiff=-1;
            }
        }
    }
    tempColor=QColor(tempColor.red()+rdiff,tempColor.green()+gdiff,tempColor.blue()+bdiff,tempColor.alpha()+adiff);
    QPixmap pixmap(IMAGEOVERSIZEWITDH,IMAGEOVERSIZEHEIGHT);
    pixmap.fill(tempColor);
    imageOver->setPixmap(pixmap);
    if(tempColor==newColor)
    {
        actualColor=newColor;
        if(newColor.alpha()==0)
            if(imageOverAdded)
            {
                imageOverAdded=false;
                mScene->removeItem(imageOver);
            }
    }
    else
        updateColorTimer.start();
}
