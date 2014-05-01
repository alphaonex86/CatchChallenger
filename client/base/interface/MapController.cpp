#include "MapController.h"
#include "DatapackClientLoader.h"
#include "../Api_client_real.h"
#include "../../../general/base/CommonDatapack.h"
#include "../../../general/base/GeneralStructures.h"

MapController* MapController::mapController=NULL;

QString MapController::text_left=QLatin1Literal("left");
QString MapController::text_right=QLatin1Literal("right");
QString MapController::text_top=QLatin1Literal("top");
QString MapController::text_bottom=QLatin1Literal("bottom");
QString MapController::text_slash=QLatin1Literal("/");
QString MapController::text_type=QLatin1Literal("type");
QString MapController::text_fight=QLatin1Literal("fight");
QString MapController::text_fightid=QLatin1Literal("fightid");
QString MapController::text_bot=QLatin1Literal("bot");
QString MapController::text_slashtrainerpng=QLatin1Literal("/trainer.png");
QString MapController::text_DATAPACK_BASE_PATH_SKINBOT=QLatin1Literal(DATAPACK_BASE_PATH_SKINBOT);
QString MapController::text_DATAPACK_BASE_PATH_SKIN=QLatin1Literal(DATAPACK_BASE_PATH_SKIN);


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
    imageOver->setPos(-800,-600);
    updateColorTimer.setSingleShot(true);
    updateColorTimer.setInterval(50);
    connect(&updateColorTimer,&QTimer::timeout,this,&MapController::updateColor);
}

MapController::~MapController()
{
    if(botFlags!=NULL)
    {
        delete botFlags;
        botFlags=NULL;
    }
}

void MapController::connectAllSignals()
{
    MapControllerMP::connectAllSignals();
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::insert_plant,this,&MapController::insert_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::remove_plant,this,&MapController::remove_plant);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::seed_planted,this,&MapController::seed_planted);
    connect(CatchChallenger::Api_client_real::client,&CatchChallenger::Api_client_real::plant_collected,this,&MapController::plant_collected);
}

void MapController::resetAll()
{
    setColor(Qt::transparent);
    delayedPlantInsert.clear();
    MapControllerMP::resetAll();
}

void MapController::datapackParsed()
{
    if(mHaveTheDatapack)
        return;
    MapControllerMP::datapackParsed();
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
    if(all_map.value(new_map->map_file)->logicalMap.bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    return true;
}

void MapController::loadBotOnTheMap(MapVisualiserThread::Map_full *parsedMap,const quint32 &botId,const quint8 &x,const quint8 &y,const QString &lookAt,const QString &skin)
{
    Q_UNUSED(botId);
    if(skin.isEmpty())
        return;

    if(parsedMap->logicalMap.botsDisplay.contains(QPair<quint8,quint8>(x,y)))
    {
        /*CatchChallenger::BotDisplay *botDisplay=&parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)];
        ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(botDisplay->mapObject);
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        botDisplay->mapObject->setPosition(QPoint(x,y+1));
        MapObjectItem::objectLink.value(botDisplay->mapObject)->setZValue(y);*/
        return;
    }

    if(!ObjectGroupItem::objectGroupLink.contains(parsedMap->objectGroup))
    {
        qDebug() << QStringLiteral("loadBotOnTheMap(), ObjectGroupItem::objectGroupLink not contains parsedMap->objectGroup");
        return;
    }
    CatchChallenger::Direction direction;
    int baseTile=-1;
    if(lookAt==MapController::text_left)
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
            CatchChallenger::DebugClass::debugConsole(QStringLiteral("Wrong direction for the bot at %1 (%2,%3)").arg(parsedMap->logicalMap.map_file).arg(x).arg(y));
        baseTile=7;
        direction=CatchChallenger::Direction_move_at_bottom;
    }

    CatchChallenger::BotDisplay *botDisplay=&parsedMap->logicalMap.botsDisplay[QPair<quint8,quint8>(x,y)];
    botDisplay->mapObject=new Tiled::MapObject();
    botDisplay->tileset=new Tiled::Tileset(MapController::text_bot,16,24);
    QString skinPath=datapackPath+MapController::text_DATAPACK_BASE_PATH_SKIN+MapController::text_slash+skin+MapController::text_slashtrainerpng;
    if(!QFile(skinPath).exists())
        skinPath=datapackPath+MapController::text_DATAPACK_BASE_PATH_SKINBOT+MapController::text_slash+skin+MapController::text_slashtrainerpng;
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

    {
        //add flags
        if(botFlags==NULL)
        {
            botFlags=new Tiled::Tileset(QLatin1Literal("botflags"),16,16);
            botFlags->loadFromImage(QImage(QStringLiteral(":/images/flags.png")),QStringLiteral(":/images/flags.png"));
        }

        if(parsedMap->logicalMap.shops.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(2);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        if(parsedMap->logicalMap.learn.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(3);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        /*if(parsedMap->logicalMap.clan.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(7);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }*/
        if(parsedMap->logicalMap.heal.contains(QPair<quint8,quint8>(x,y)))
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
        if(parsedMap->logicalMap.market.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(4);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        if(parsedMap->logicalMap.zonecapture.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(6);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }
        /*if(parsedMap->logicalMap.industry.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
            botDisplay->flags << flag;
            Tiled::Cell cell=flag->cell();
            cell.tile=botFlags->tileAt(8);
            flag->setCell(cell);
            ObjectGroupItem::objectGroupLink.value(parsedMap->objectGroup)->addObject(flag);
            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            flag->setPosition(QPointF(x,y-1.0*botDisplay->flags.size()+0.5));
            MapObjectItem::objectLink.value(flag)->setZValue(y);
        }*/
        /* asked by tgjklmda if(parsedMap->logicalMap.botsFight.contains(QPair<quint8,quint8>(x,y)))
        {
            Tiled::MapObject * flag=new Tiled::MapObject();
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

    if(parsedMap->logicalMap.bots.value(QPair<quint8,quint8>(x,y)).step.contains(1))
    {
        QDomElement stepBot=parsedMap->logicalMap.bots.value(QPair<quint8,quint8>(x,y)).step.value(1);
        if(stepBot.hasAttribute(MapController::text_type) && stepBot.attribute(MapController::text_type)==MapController::text_fight && stepBot.hasAttribute(MapController::text_fightid))
        {
            bool ok;
            quint32 fightid=stepBot.attribute(MapController::text_fightid).toUInt(&ok);
            if(ok)
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.botFights.contains(fightid))
                {
                    #ifdef DEBUG_CLIENT_BOT
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("Put bot fight point %1 at %2 (%3,%4) in direction: %5").arg(fightid).arg(parsedMap->logicalMap.map_file).arg(x).arg(y).arg(direction));
                    #endif
                    quint8 temp_x=x,temp_y=y;
                    int index_botfight_range=0;
                    CatchChallenger::CommonMap *map=&parsedMap->logicalMap;
                    CatchChallenger::CommonMap *old_map=map;
                    while(index_botfight_range<CATCHCHALLENGER_BOTFIGHT_RANGE)
                    {
                        if(!CatchChallenger::MoveOnTheMap::canGoTo(direction,*map,temp_x,temp_y,true,false))
                            break;
                        if(!CatchChallenger::MoveOnTheMap::move(direction,&map,&temp_x,&temp_y,true,false))
                            break;
                        if(map!=old_map)
                            break;
                        parsedMap->logicalMap.botsFightTrigger.insert(QPair<quint8,quint8>(temp_x,temp_y),fightid);
                        parsedMap->logicalMap.botsFightTriggerExtra.insert(QPair<quint8,quint8>(temp_x,temp_y),QPair<quint8,quint8>(x,y));
                        index_botfight_range++;
                    }
                }
                else
                    CatchChallenger::DebugClass::debugConsole(QStringLiteral("No fightid %1 at MapController::loadBotOnTheMap").arg(fightid));
            }
        }
    }
}

void MapController::setColor(const QColor &color, const quint32 &timeInMS)
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
        QPixmap pixmap(800*2*10,600*2*10);
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
    QPixmap pixmap(800*2,600*2);
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
