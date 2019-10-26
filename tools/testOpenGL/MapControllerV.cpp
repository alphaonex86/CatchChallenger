#include "MapControllerV.h"
#include "../../general/base/MoveOnTheMap.h"
#include "../../client/qt/CustomButton.h"

#include <QMessageBox>
#include <QCoreApplication>
#include <QDebug>
#include <iostream>

MapControllerV::MapControllerV(const bool &centerOnPlayer,const bool &debugTags,const bool &useCache,const bool &openGL) :
    MapVisualiserPlayer(centerOnPlayer,debugTags,useCache,openGL)
    //MapControllerMP(centerOnPlayer,debugTags,useCache,openGL)
{
    setWindowIcon(QIcon(":/icon.png"));
    /*setStyleSheet("background: transparent");
    setWindowFlags(Qt::FramelessWindowHint);*/
    setAttribute(Qt::WA_TranslucentBackground);
    viewport()->setAutoFillBackground(false);
    QPalette p = viewport()->palette();
      p.setColor(QPalette::Base, Qt::transparent);
      viewport()->setPalette(p);
    botTileset = new Tiled::Tileset("bot",16,24);
    botTileset->loadFromImage(QImage(":/bot_skin.png"),":/bot_skin.png");
    botNumber = 0;

    //the bot management
    if(!connect(&timerBotMove,SIGNAL(timeout()),this,SLOT(botMove())))
        abort();
    if(!connect(&timerBotManagement,SIGNAL(timeout()),this,SLOT(botManagement())))
        abort();
    timerBotMove.start(66);
    timerBotManagement.start(3000);
    forcePlayerTileset("player_skin.png");
    FPS=0;
    if(!connect(this,&MapVisualiser::newFPSvalue,this,&MapControllerV::updateFPS))
        abort();
    FPSMapObject=nullptr;

    qRegisterMetaType<uint8_t>("uint8_t");
    qRegisterMetaType<std::unordered_map<uint16_t,uint32_t> >("std::unordered_map<uint16_t,uint32_t>");
    qRegisterMetaType<CatchChallenger::Direction>("CatchChallenger::Direction");
    qRegisterMetaType<CatchChallenger::Chat_type>("CatchChallenger::Chat_type");
    qRegisterMetaType<CatchChallenger::Player_public_informations>("CatchChallenger::Player_public_informations");
    qRegisterMetaType<CatchChallenger::Player_private_and_public_informations>("CatchChallenger::Player_private_and_public_informations");
    qRegisterMetaType<std::vector<std::pair<uint8_t,CatchChallenger::Direction> > >("std::vector<std::pair<uint8_t,CatchChallenger::Direction> >");
    qRegisterMetaType<std::vector<Map_full> >("std::vector<Map_full>");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Direction,uint8_t> > >("std::vector<std::pair<CatchChallenger::Direction,uint8_t> >");
    qRegisterMetaType<std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > >("std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >");
    if(!connect(&pathFinding,&PathFinding::result,this,&MapControllerV::pathFindingResult))
        abort();

    CustomButton *quit=new CustomButton(":/CC/images/interface/button.png");
    quit->setText("Quit");
    quit->move(256,256);
    mScene->addWidget(quit);
    if(!connect(quit,&CustomButton::clicked,&QCoreApplication::quit))
        abort();
}

MapControllerV::~MapControllerV()
{
    pathFinding.cancel();
    //delete playerTileset;
    delete botTileset;
}

void MapControllerV::moveStepSlot()
{
    MapVisualiserPlayer::moveStepSlot();
    if(FPSMapObject!=nullptr)
    {
        FPSMapObject->setX(MapVisualiserPlayer::getX());
        FPSMapObject->setY(MapVisualiserPlayer::getY());
    }
}

void MapControllerV::setScale(int scaleSize)
{
    scale(scaleSize,scaleSize);
}

void MapControllerV::setBotNumber(quint16 botNumber)
{
    this->botNumber=botNumber;
    botManagement();
}

bool MapControllerV::botMoveStepSlot(Bot *bot)
{
    int baseTile=1;
    //move the player for intermediate step and define the base tile (define the stopped step with direction)
    switch(bot->direction)
    {
        case CatchChallenger::Direction_move_at_left:
        baseTile=10;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setX(bot->mapObject->x()-0.33);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_right:
        baseTile=4;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setX(bot->mapObject->x()+0.33);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_top:
        baseTile=1;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setY(bot->mapObject->y()-0.33);
            break;
        }
        break;
        case CatchChallenger::Direction_move_at_bottom:
        baseTile=7;
        switch(bot->moveStep)
        {
            case 1:
            case 2:
            bot->mapObject->setY(bot->mapObject->y()+0.33);
            break;
        }
        break;
        default:
        qDebug() << QStringLiteral("botMoveStepSlot(): moveStep: %1, wrong direction").arg(bot->moveStep);
        return false;
    }

    //apply the right step of the base step defined previously by the direction
    Tiled::Cell cell=bot->mapObject->cell();
    switch(bot->moveStep)
    {
        //stopped step
        case 0:
        cell.tile=botTileset->tileAt(baseTile+0);
        break;
        //transition step
        case 1:
        cell.tile=botTileset->tileAt(baseTile-1);
        break;
        case 2:
        cell.tile=botTileset->tileAt(baseTile+1);
        break;
        //stopped step
        case 3:
        cell.tile=botTileset->tileAt(baseTile+0);
        break;
    }
    bot->mapObject->setCell(cell);

    bot->moveStep++;

    //if have finish the step
    if(bot->moveStep>3)
    {
        CatchChallenger::CommonMap * old_map=&all_map[bot->map.toStdString()]->logicalMap;
        CatchChallenger::CommonMap * map=&all_map[bot->map.toStdString()]->logicalMap;
        //set the final value (direction, position, ...)
        switch(bot->direction)
        {
            case CatchChallenger::Direction_move_at_left:
            bot->direction=CatchChallenger::Direction_look_at_left;
            CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_left,&map,&bot->x,&bot->y);
            break;
            case CatchChallenger::Direction_move_at_right:
            bot->direction=CatchChallenger::Direction_look_at_right;
            CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_right,&map,&bot->x,&bot->y);
            break;
            case CatchChallenger::Direction_move_at_top:
            bot->direction=CatchChallenger::Direction_look_at_top;
            CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_top,&map,&bot->x,&bot->y);
            break;
            case CatchChallenger::Direction_move_at_bottom:
            bot->direction=CatchChallenger::Direction_look_at_bottom;
            CatchChallenger::MoveOnTheMap::move(CatchChallenger::Direction_move_at_bottom,&map,&bot->x,&bot->y);
            break;
            default:
            qDebug() << QStringLiteral("botMoveStepSlot(): bot->moveStep: %1, wrong direction when bot->moveStep>2").arg(bot->moveStep);
            return false;
        }
        //if the map have changed
        if(old_map!=map)
        {
            //remove bot
            if(ObjectGroupItem::objectGroupLink.find(all_map[old_map->map_file]->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink[all_map[old_map->map_file]->objectGroup]->removeObject(bot->mapObject);
            else
                qDebug() << QStringLiteral("botMoveStepSlot(), ObjectGroupItem::objectGroupLink not contains bot->mapObject at remove to change the map");
            //add bot
            if(ObjectGroupItem::objectGroupLink.find(all_map[map->map_file]->objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
                ObjectGroupItem::objectGroupLink[all_map[map->map_file]->objectGroup]->addObject(bot->mapObject);
            else
                return false;
            bot->map=QString::fromStdString(map->map_file);
        }
        //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
        bot->mapObject->setPosition(QPoint(bot->x,bot->y+1));

        bot->inMove=false;
    }

    return true;
}

void MapControllerV::botMove()
{
    int index;
    QSet<int> continuedMove;
    //continue the move
    index=0;
    while(index<botList.size())
    {
        if(botList.at(index).inMove)
        {
            if(!botMoveStepSlot(&botList[index]))
            {
                delete botList.at(index).mapObject;
                botList.removeAt(index);
                index--;
            }
            continuedMove << index;
        }
        index++;
    }
    //start move
    index=0;
    while(index<botList.size())
    {
        if(!botList.at(index).inMove && !continuedMove.contains(index))
        {
            QList<CatchChallenger::Direction> directions_allowed;
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_left,all_map[botList.at(index).map.toStdString()]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << CatchChallenger::Direction_move_at_left;
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_right,all_map[botList.at(index).map.toStdString()]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << CatchChallenger::Direction_move_at_right;
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_top,all_map[botList.at(index).map.toStdString()]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << CatchChallenger::Direction_move_at_top;
            if(CatchChallenger::MoveOnTheMap::canGoTo(CatchChallenger::Direction_move_at_bottom,all_map[botList.at(index).map.toStdString()]->logicalMap,botList.at(index).x,botList.at(index).y,true))
                directions_allowed << CatchChallenger::Direction_move_at_bottom;
            if(directions_allowed.size()>0)
            {
                int random = rand()%directions_allowed.size();
                CatchChallenger::Direction final_direction=directions_allowed.at(random);

                botList[index].direction=final_direction;
                botList[index].inMove=true;
                botList[index].moveStep=1;
                switch(final_direction)
                {
                    case CatchChallenger::Direction_move_at_left:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case CatchChallenger::Direction_move_at_right:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case CatchChallenger::Direction_move_at_top:
                        botMoveStepSlot(&botList[index]);
                    break;
                    case CatchChallenger::Direction_move_at_bottom:
                        botMoveStepSlot(&botList[index]);
                    break;
                    default:
                    qDebug() << QStringLiteral("transformLookToMove(): wrong direction");
                    return;
                }
            }
        }
        index++;
    }
}

void MapControllerV::botManagement()
{
    //int index;
    //remove bot where the map is not displayed
    /*index=0;
    while(index<botList.size())
    {
        if(!displayed_map.contains(botList.at(index).map))
        {
            if(all_map.contains(botList.at(index).map))
            {
                ObjectGroupItem::objectGroupLink[all_map[botList.at(index).map]->objectGroup]->removeObject(botList.at(index).mapObject);
                delete botList.at(index).mapObject;
            }
            else
            {
                //the map have been removed then the map object too
            }
            botList.removeAt(index);
        }
        else
            index++;
    }*/
    //remove random bot
    /*index=0;
    while(index<botList.size())
    {
        if(index>=botNumber || rand()%100<20)//if to much bot
        {
            if(all_map.contains(botList.at(index).map))
            {
                if(ObjectGroupItem::objectGroupLink.contains(all_map[botList.at(index).map]->objectGroup))
                    ObjectGroupItem::objectGroupLink[all_map[botList.at(index).map]->objectGroup]->removeObject(botList.at(index).mapObject);
                else
                    qDebug() << QStringLiteral("botManagement(), ObjectGroupItem::objectGroupLink not contains botList.at(index).mapObject at remove random bot");
                delete botList.at(index).mapObject;
            }
            else
            {
                //the map have been removed then the map object too
            }
            botList.removeAt(index);
        }
        else
            index++;
    }*/
    //add bot
    /*if(!botSpawnPointList.isEmpty())
    {
        index=botList.size();
        while(index<botNumber)//do botNumber bot
        {
            BotSpawnPoint point=botSpawnPointList[rand()%botSpawnPointList.size()];

            Bot bot;
            bot.map=point.map->logicalMap.map_file;
            bot.mapObject=new Tiled::MapObject();
            bot.x=point.x;
            bot.y=point.y;
            bot.direction=CatchChallenger::Direction_look_at_bottom;
            bot.inMove=false;
            bot.moveStep=0;

            if(ObjectGroupItem::objectGroupLink.contains(all_map[bot.map]->objectGroup))
                ObjectGroupItem::objectGroupLink[all_map[bot.map]->objectGroup]->addObject(bot.mapObject);
            else
                qDebug() << QStringLiteral("botManagement(), ObjectGroupItem::objectGroupLink not contains bot.map->objectGroup");

            //move to the final position (integer), y+1 because the tile lib start y to 1, not 0
            bot.mapObject->setPosition(QPoint(bot.x,bot.y+1));

            bot.mapObject->setTile(botTileset->tileAt(7));
            botList << bot;
            index++;
        }
    }
    else
    {
        //qDebug() << "Bot spawn list is empty";
    }*/
}

//call after enter on new map
void MapControllerV::loadPlayerFromCurrentMap()
{
    MapVisualiserPlayer::loadPlayerFromCurrentMap();

    //list bot spawn point
    botSpawnPointList.clear();
    /*{
        QSet<QString>::const_iterator i = displayed_map.constBegin();
        while (i != displayed_map.constEnd()) {
            MapVisualiserThread::Map_full * map=getMap(*i);
            if(map!=NULL)
            {
                int index=0;
                while(index<map->logicalMap.bot_spawn_points.size())
                {
                    BotSpawnPoint point;
                    point.map=map;
                    point.x=map->logicalMap.bot_spawn_points.at(index).x;
                    point.y=map->logicalMap.bot_spawn_points.at(index).y;
                    botSpawnPointList << point;
                    index++;
                }
            }
            ++i;
        }
    }

    botManagement();*/

    {
        QPixmap pix=getFPSPixmap();
        FPSMapObject = new Tiled::MapObject();
        FPSMapObject->setName("FPSMapObject");
        FPSTileset = new Tiled::Tileset(QString(),pix.width(),pix.height());
        FPSTileset->addTile(pix);
        Tiled::Cell cell=FPSMapObject->cell();
        cell.tile=FPSTileset->tileAt(0);
        FPSMapObject->setCell(cell);

        Tiled::ObjectGroup * objectGroup=all_map.at(current_map)->objectGroup;
        if(ObjectGroupItem::objectGroupLink.find(objectGroup)!=ObjectGroupItem::objectGroupLink.cend())
            ObjectGroupItem::objectGroupLink.at(objectGroup)->addObject(FPSMapObject);
        else
            abort();
        FPSMapObject->setX(MapVisualiserPlayer::getX());
        FPSMapObject->setY(MapVisualiserPlayer::getY());
    }
}

void MapControllerV::updateFPS(const unsigned int FPS)
{
    this->FPS=FPS;
    if(FPSMapObject!=nullptr)
        FPSTileset->tileAt(0)->setImage(getFPSPixmap());
}

QPixmap MapControllerV::getFPSPixmap()
{
    QPixmap pix;
    QRectF destRect;
    {
        QPixmap pix(50,10);
        QPainter painter(&pix);
        QRectF sourceRec(0.0,0.0,50.0,10.0);
        destRect=painter.boundingRect(sourceRec,Qt::TextSingleLine,QString::number(FPS)+"FPS");
    }
    pix=QPixmap(destRect.width(),destRect.height());
    //draw the text & image
    {
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.drawText(QRectF(0.0,0.0,destRect.width(),destRect.height()),Qt::TextSingleLine,QString::number(FPS)+"FPS");
    }
    return pix;
}

//call before leave the old map (and before loadPlayerFromCurrentMap())
void MapControllerV::unloadPlayerFromCurrentMap()
{
    MapVisualiserPlayer::unloadPlayerFromCurrentMap();

    botSpawnPointList.clear();
}

bool MapControllerV::viewMap(const QString &fileName)
{
    loadOtherMap(fileName.toStdString());
    return true;
}

bool MapControllerV::asyncMapLoaded(const std::string &fileName, Map_full * tempMapObject)
{
    MapVisualiser::asyncMapLoaded(fileName,tempMapObject);
    if(current_map.empty())
    {
        QMessageBox::critical(this,"Error",QString::fromStdString(mLastError));
        return false;
    }
    if(tempMapObject==NULL)
    {
        //to support border missing map QMessageBox::critical(this,"Error",QString::fromStdString(mLastError));
        return false;
    }
    all_map[fileName]=tempMapObject;
    Map_full *map=tempMapObject;

    render();

    uint8_t x=0,y=0;
    //position
    if(!map->logicalMap.rescue_points.empty())
    {
        x=map->logicalMap.rescue_points.front().x;
        y=map->logicalMap.rescue_points.front().y;
    }
    else if(!map->logicalMap.bot_spawn_points.empty())
    {
        x=map->logicalMap.bot_spawn_points.front().x;
        y=map->logicalMap.bot_spawn_points.front().y;
    }
    else
    {
        x=map->logicalMap.width/2;
        y=map->logicalMap.height/2;
    }

    loadPlayerMap(current_map,x,y);
    removeUnusedMap();
    loadPlayerFromCurrentMap();

    show();

    return true;
}

CatchChallenger::Direction MapControllerV::moveFromPath()
{
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &x=pos.first;
    const uint8_t &y=pos.second;
    CatchChallenger::Orientation orientation;
    if(pathList.size()>1)
    {
        PathResolved::StartPoint startPoint=pathList.back().startPoint;
        if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
        {
            while(pathList.size()>1)
                pathList.erase(pathList.cbegin());
        }

        PathResolved &pathFirst=pathList.front();
        std::pair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.front();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.erase(pathFirst.path.cbegin());
            if(pathFirst.path.empty())
            {
                pathList.erase(pathList.cbegin());
                if(!pathList.empty())
                {
                    PathResolved::StartPoint startPoint=pathList.back().startPoint;
                    if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
                    {
                    }
                    else
                        pathList.clear();
                }
            }
        }
    }
    else
    {
        PathResolved &pathFirst=pathList.front();
        std::pair<CatchChallenger::Orientation,uint8_t> &pathFirstUnit=pathFirst.path.front();
        orientation=pathFirstUnit.first;
        pathFirstUnit.second--;
        if(pathFirstUnit.second==0)
        {
            pathFirst.path.erase(pathFirst.path.cbegin());
            if(pathFirst.path.empty())
            {
                pathList.erase(pathList.cbegin());
                if(!pathList.empty())
                {
                    PathResolved::StartPoint startPoint=pathList.back().startPoint;
                    if(startPoint.map==currentMap() && startPoint.x==x && startPoint.y==y)
                    {
                    }
                    else
                        pathList.clear();
                }
            }
        }
    }

    if(orientation==CatchChallenger::Orientation_bottom)
        return CatchChallenger::Direction_move_at_bottom;
    if(orientation==CatchChallenger::Orientation_top)
        return CatchChallenger::Direction_move_at_top;
    if(orientation==CatchChallenger::Orientation_left)
        return CatchChallenger::Direction_move_at_left;
    if(orientation==CatchChallenger::Orientation_right)
        return CatchChallenger::Direction_move_at_right;
    return CatchChallenger::Direction_move_at_bottom;
}

void MapControllerV::eventOnMap(CatchChallenger::MapEvent event,Map_full * tempMapObject,uint8_t x,uint8_t y)
{
    const std::pair<uint8_t,uint8_t> pos(getPos());
    const uint8_t &thisx=pos.first;
    const uint8_t &thisy=pos.second;
    if(event==CatchChallenger::MapEvent_SimpleClick)
    {
        if(keyAccepted.empty() || (keyAccepted.find(Qt::Key_Return)!=keyAccepted.cend() && keyAccepted.size()))
        {
            MapVisualiser::eventOnMap(event,tempMapObject,x,y);
            pathFinding.searchPath(all_map,tempMapObject->logicalMap.map_file,x,y,current_map,thisx,thisy,*items);
            if(pathList.empty())
            {
                while(pathList.size()>1)
                    pathList.pop_back();
                //pathList.clear();
            }
        }
    }
}

bool MapControllerV::nextPathStep()//true if have step
{
    if(!pathList.empty())
    {
        const CatchChallenger::Direction &direction=MapControllerV::moveFromPath();
        return MapVisualiserPlayer::nextPathStepInternal(pathList,direction);
    }
    return false;
}

void MapControllerV::pathFindingResult(const std::string &current_map,const uint8_t &x,const uint8_t &y,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t> > &path)
{
    if(!path.empty())
    {
        if(path.front().second==0)
        {
            std::cerr << "MapControllerV::pathFindingResult(): path.first().second==0" << std::endl;
            //pathFindingNotFound();
            return;
        }
        MapVisualiserPlayer::pathFindingResultInternal(pathList,current_map,x,y,path);
    }
    /*else
        pathFindingNotFound();*/
}

void MapControllerV::keyPressParse()
{
    pathFinding.cancel();
    pathList.clear();
    MapVisualiserPlayer::keyPressParse();
}

