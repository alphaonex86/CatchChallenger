#include "BotTargetList.h"
#include "SocialChat.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "MapBrowse.h"
#include "TargetFilter.h"

#include <chrono>
#include <QMessageBox>
#include <unordered_map>

BotTargetList::BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
                             QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
                             QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
                             ActionsAction *actionsAction) :
    ui(new Ui::BotTargetList),
    apiToCatchChallengerClient(apiToCatchChallengerClient),
    connectedSocketToCatchChallengerClient(connectedSocketToCatchChallengerClient),
    sslSocketToCatchChallengerClient(sslSocketToCatchChallengerClient),
    actionsAction(actionsAction),
    botsInformationLoaded(false),
    mapId(0)
{
    ui->setupUi(this);
    ui->comboBoxStep->setCurrentIndex(1);
    ui->graphvizText->setVisible(false);

    QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *>::const_iterator i = apiToCatchChallengerClient.constBegin();
    while (i != apiToCatchChallengerClient.constEnd()) {
        MultipleBotConnection::CatchChallengerClient * const client=i.value();
        if(client->have_informations)
        {
            const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=client->api->get_player_informations();
            const QString &qtpseudo=QString::fromStdString(player_private_and_public_informations.public_informations.pseudo);
            ui->bots->addItem(qtpseudo);
            pseudoToBot[qtpseudo]=client;
        }
        ++i;
    }

    MapServerMini::colorsList << QColor(255, 255, 0, 255);
    MapServerMini::colorsList << QColor(70, 187, 70, 255);
    MapServerMini::colorsList << QColor(100, 100, 200, 255);
    MapServerMini::colorsList << QColor(255, 128, 128, 255);
    MapServerMini::colorsList << QColor(180, 70, 180, 255);
    MapServerMini::colorsList << QColor(255, 200, 110, 255);
    MapServerMini::colorsList << QColor(115, 255, 240, 255);
    MapServerMini::colorsList << QColor(115, 255, 120, 255);
    MapServerMini::colorsList << QColor(200, 70, 70, 255);
    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;

    if(!connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerStep))
        abort();
    if(!connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerMapSlot))
        abort();
    if(!connect(&autoStartActionTimer,&QTimer::timeout,this,&BotTargetList::autoStartAction))
        abort();

    dirt=true,itemOnMap=true,fight=true,shop=true,heal=true,wildMonster=true;
}

BotTargetList::~BotTargetList()
{
    delete ui;
}

bool BotTargetList::newPathIsBetterPath(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &oldPath,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &newPath)
{
    if(oldPath.size()>newPath.size())
        return true;
    else if(oldPath.size()==newPath.size())
    {
        unsigned int oldPathTile=0;
        {
            unsigned int index=0;
            while(index<oldPath.size())
            {
                const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=oldPath.at(index);
                oldPathTile+=step.second;
                index++;
            }
        }
        unsigned int newPathTile=0;
        {
            unsigned int index=0;
            while(index<oldPath.size())
            {
                const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=newPath.at(index);
                newPathTile+=step.second;
                index++;
            }
        }
        return newPathTile<oldPathTile;
    }
    else//if(oldPath.size()<newPath.size())
        return false;
}

uint16_t BotTargetList::pathTileCount(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &path)
{
    unsigned int pathTile=0;
    {
        unsigned int index=0;
        while(index<path.size())
        {
            const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=path.at(index);
            pathTile+=step.second;
            index++;
        }
    }
    return pathTile;
}

std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > BotTargetList::selectTheBetterPathToDestination(std::unordered_map<unsigned int,
                                             std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> >
                                             > pathToEachDestinations,unsigned int &destinationIndexSelected)
{
    if(pathToEachDestinations.empty())
        abort();
    destinationIndexSelected=0;
    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnVar;
    bool haveThePathSelected=false;
    for(const auto& n:pathToEachDestinations)
    {
        const unsigned int &key=n.first;
        const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &value=n.second;

        if(!haveThePathSelected)
        {
            haveThePathSelected=true;
            destinationIndexSelected=key;
            returnVar=value;
        }
        else
        {
            if(newPathIsBetterPath(returnVar,value))
            {
                destinationIndexSelected=key;
                returnVar=value;
            }
        }
    }
    //group the 2 first action if is look into differente direction
    if(returnVar.size()>1)
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &firstValue=returnVar.front();
        if(firstValue.first==CatchChallenger::Orientation::Orientation_none)
        {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(firstValue.second==0)
        #endif
                returnVar.erase(returnVar.cbegin());
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                abort();
        #endif
        }
    }
    //group the 2 last action if is look into differente direction
    if(returnVar.size()>1)
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &lastValue=returnVar.back();
        if(lastValue.first==CatchChallenger::Orientation::Orientation_none)
        {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(lastValue.second==0)
        #endif
                returnVar.erase(returnVar.cbegin()+returnVar.size()-1);
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                abort();
        #endif
        }
    }
    std::cout << "Path to return: " << BotTargetList::stepToString(returnVar) << std::endl;
    return returnVar;
}

void BotTargetList::loadAllBotsInformation()
{
    if(botsInformationLoaded)
        return;
    botsInformationLoaded=true;

    /*if(!connect(actionsAction,&ActionsAction::preload_the_map_finished,this,&BotTargetList::loadAllBotsInformation2))
        abort();
    if(!connect(this,&BotTargetList::start_preload_the_map,actionsAction,&ActionsAction::preload_the_map))
        abort();
    emit start_preload_the_map();
    waitScreen.show();
    waitScreen.updateWaitScreen();*/
    actionsAction->preload_the_map();
    loadAllBotsInformation2();
}

void BotTargetList::loadAllBotsInformation2()
{
    if(!actionsAction->preload_the_map_step2())
        abort();
    show();
    SocialChat::socialChat->show();
    //waitScreen.hide();
}

void BotTargetList::on_bots_itemSelectionChanged()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    ui->label_local_target->setEnabled(true);
    ui->comboBoxStep->setEnabled(true);
    ui->globalTargets->setEnabled(true);
    ui->browseMap->setEnabled(true);
    ui->groupBoxPlayer->setEnabled(true);

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    mapId=player.mapId;
    ui->trackThePlayer->setChecked(true);

    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;

    updateMapInformation();
    updatePlayerInformation();
}

void BotTargetList::updatePlayerMapSlot()
{
    updatePlayerMap();
}

void BotTargetList::updatePlayerMap(const bool &force)
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);

    if(player.mapId==mapId || force)
    {
        if(force)
        {
            updateMapContentX=0;
            updateMapContentY=0;
            updateMapContentMapId=0;
            updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
        }
        updateMapContent();
    }
}

void BotTargetList::startPlayerMove()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    startPlayerMove(client->api);
    updateMapInformation();
    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];
    ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(player.target.localStep)));
}

void BotTargetList::startPlayerMove(CatchChallenger::Api_protocol *api)
{
    if(!actionsAction->clientList.contains(api))
        return;

    ActionsBotInterface::Player &player=actionsAction->clientList[api];

    if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
        return;
    const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
    const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
    QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    ui->label_local_target->setTitle("Target on the map: "+mapString);
    if(playerMap->step.size()<2)
        abort();
    const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
    const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
    if(playerCodeZone<=0 || (uint32_t)(playerCodeZone-1)>=(uint32_t)stepPlayer.layers.size())
        return;
    const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
    std::unordered_map<const MapServerMini::BlockObject *,MapServerMini::BlockObjectPathFinding> resolvedBlock;
    playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value());
    const MapServerMini::MapParsedForBot &step=playerMap->step.at(ui->comboBoxStep->currentIndex());
    if(step.map==NULL)
        return;

    std::vector<DestinationForPath> destinations;
    std::vector<MapServerMini::BlockObject::LinkPoint> pointsList;
    uint8_t o=api->getDirection();
    while(o>4)
        o-=4;
    //if the target is on the same block
    if(player.target.bestPath.empty())//set into BotTargetList::on_globalTargets_itemActivated()
    {
        const std::pair<uint8_t,uint8_t> &point=getNextPosition(layer.blockObject,player.target/*hop list, first is the next hop*/);
        DestinationForPath destinationForPath;
        destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
        destinationForPath.destination_x=point.first;
        destinationForPath.destination_y=point.second;
        destinations.push_back(destinationForPath);
        MapServerMini::BlockObject::LinkPoint linkPoint;
        linkPoint.type=MapServerMini::BlockObject::LinkType::SourceNone;
        linkPoint.x=point.first;
        linkPoint.y=point.second;
        pointsList.push_back(linkPoint);
        std::cout << "player.target.bestPath.empty()" << std::endl;
    }
    else //search the best path to the next block
    {
        {
            std::cout << "player.target.bestPath (less the current block): " << std::endl;
            unsigned int index=0;
            while(index<player.target.bestPath.size())
            {
                const MapServerMini::BlockObject * blockObject=player.target.bestPath.at(index);
                std::cout << " " << blockObject->map->map_file << " Block " << std::to_string(blockObject->id+1) << std::endl;
                index++;
            }
            std::cout << std::endl;
        }
        if(layer.blockObject->links.find(player.target.bestPath.front())==layer.blockObject->links.cend())
            abort();
        const MapServerMini::BlockObject::LinkInformation &linkInformation=layer.blockObject->links.at(player.target.bestPath.front());
        unsigned int indexLinkInformation=0;
        while(indexLinkInformation<linkInformation.linkConditions.size())
        {
            const MapServerMini::BlockObject::LinkCondition &linkCondition=linkInformation.linkConditions.at(indexLinkInformation);
            if(ActionsAction::mapConditionIsRepected(api,linkCondition.condition)) //condition is respected
            {
                pointsList=linkCondition.points;
                if(pointsList.empty())
                    abort();
                unsigned int index=0;
                while(index<linkCondition.points.size())
                {
                    const MapServerMini::BlockObject::LinkPoint &point=linkCondition.points.at(index);
                    DestinationForPath destinationForPath;
                    destinationForPath.destination_x=point.x;
                    destinationForPath.destination_y=point.y;
                    switch(point.type)
                    {
                        case MapServerMini::BlockObject::LinkType::SourceTopMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalTopBlock:
                            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_top;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceRightMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalRightBlock:
                            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_right;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceBottomMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalBottomBlock:
                            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_bottom;
                        break;
                        case MapServerMini::BlockObject::LinkType::SourceLeftMap:
                        case MapServerMini::BlockObject::LinkType::SourceInternalLeftBlock:
                            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_left;
                        break;
                        default:
                            destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
                        break;
                    }
                    destinations.push_back(destinationForPath);

                    index++;
                }
            }
            indexLinkInformation++;
        }
        if(pointsList.size()==0)
        {
            std::cerr << "pointsList is empty, no condition respected?" << std::endl;
            abort();
        }
    }
    if(pointsList.size()!=destinations.size())
        abort();
    bool ok=false;
    unsigned int destinationIndexSelected=0;
    const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath=pathFinding(
                layer.blockObject,
                static_cast<CatchChallenger::Orientation>(o),player.x,player.y,
                destinations,
                destinationIndexSelected,
                &ok);
    if(!ok)
        abort();
    player.target.linkPoint=pointsList.at(destinationIndexSelected);
    player.target.localStep=returnPath;

    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
}

std::string BotTargetList::stepToString(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &returnPath)
{
    std::string stepToDo;
    unsigned int index=0;
    while(index<returnPath.size())
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &entry=returnPath.at(index);
        if(!stepToDo.empty())
            stepToDo+=", ";
        stepToDo+=std::to_string(entry.second);
        switch(entry.first)
        {
            case CatchChallenger::Orientation::Orientation_bottom:
                stepToDo+=" bottom";
            break;
            case CatchChallenger::Orientation::Orientation_top:
                stepToDo+=" top";
            break;
            case CatchChallenger::Orientation::Orientation_left:
                stepToDo+=" left";
            break;
            case CatchChallenger::Orientation::Orientation_right:
                stepToDo+=" right";
            break;
            case CatchChallenger::Orientation::Orientation_none:
                stepToDo+=" none";
            break;
            default:
                abort();
            break;
        }

        index++;
    }
    return stepToDo;
}

void BotTargetList::updateMapInformation()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);

    if(actionsAction->id_map_to_map.find(mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map);
        if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
            return;
        MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
        if(step.map==NULL)
            return;
        QString QtGraphvizText=QString::fromStdString(graphLocalMap());

        ui->mapPreview->setColumnCount(0);
        ui->mapPreview->setRowCount(0);
        /*ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);*/
        ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);

        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(playerMap->step.size()<2)
            abort();

        ui->comboBox_Layer->clear();
        unsigned int index=0;
        while(index<step.layers.size())
        {
            const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(index);

            //if(layer.name!="Lost layer" || !layer.contentList.empty())
                ui->comboBox_Layer->addItem(QString::fromStdString(layer.name),index);

            index++;
        }
        if(QtGraphvizText.isEmpty())
            ui->graphvizText->setVisible(false);
        else
        {
            ui->graphvizText->setVisible(true);
            ui->graphvizText->setPlainText(QtGraphvizText);
        }
    }
    else
        std::cerr << "updateMapInformation(): map id not found for the current player: " << mapId << std::endl;
    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
    updateMapContent();
    updateLayerElements();
}

void BotTargetList::updateMapContent()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    if(updateMapContentX==player.x && updateMapContentY==player.y && updateMapContentMapId==player.mapId && updateMapContentDirection==player.api->getDirection())
        return;
    updateMapContentX=player.x;
    updateMapContentY=player.y;
    updateMapContentMapId=player.mapId;
    updateMapContentDirection=player.api->getDirection();

    if(actionsAction->id_map_to_map.find(mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map);
        if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
            return;
        MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
        if(step.map==NULL)
            return;

        ui->mapPreview->setColumnCount(0);
        ui->mapPreview->setRowCount(0);
        /*ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);*/
        ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);

        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(playerMap->step.size()<2)
            abort();
        //if map not sync with x and y
        if(player.x>=playerMap->width)
            abort();
        if(player.y>=playerMap->height)
            abort();
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
        if(playerCodeZone==0)
        {
            std::cerr << "out of map" << std::endl;
            abort();
        }

        {
            int y=mapServer->min_y;
            while(y<mapServer->max_y)
            {
                int x=mapServer->min_x;
                while(x<mapServer->max_x)
                {
                    QTableWidgetItem *tablewidgetitem = new QTableWidgetItem();
                    //color
                    int codeZone=step.map[x+y*mapServer->width];
                    if(codeZone>0)
                    {
                        const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(codeZone-1);
                        QColor color=layer.blockObject->color;
                        QBrush brush1(color);
                        brush1.setStyle(Qt::SolidPattern);
                        tablewidgetitem->setBackground(brush1);
                        tablewidgetitem->setText(QString::number(codeZone));
                    }
                    //icon
                    if(x==player.x && y==player.y && player.mapId==mapId)
                    {
                        QIcon icon;
                        switch(player.api->getDirection())
                        {
                            case CatchChallenger::Direction::Direction_move_at_top:
                                icon.addFile(QStringLiteral(":/playerloctm.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_look_at_top:
                                icon.addFile(QStringLiteral(":/playerloct.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_move_at_right:
                                icon.addFile(QStringLiteral(":/playerlocrm.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_look_at_right:
                                icon.addFile(QStringLiteral(":/playerlocr.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_move_at_bottom:
                                icon.addFile(QStringLiteral(":/playerlocbm.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_look_at_bottom:
                                icon.addFile(QStringLiteral(":/playerlocb.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_move_at_left:
                                icon.addFile(QStringLiteral(":/playerloclm.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            case CatchChallenger::Direction::Direction_look_at_left:
                                icon.addFile(QStringLiteral(":/playerlocl.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                            default:
                                icon.addFile(QStringLiteral(":/playerloc.png"), QSize(), QIcon::Normal, QIcon::Off);
                            break;
                        }
                        tablewidgetitem->setText("");
                        tablewidgetitem->setIcon(icon);
                    }
                    else if(player.target.blockObject!=NULL)
                    {
                        const std::pair<uint8_t,uint8_t> point(player.target.linkPoint.x,player.target.linkPoint.y);
                        if(x==point.first && y==point.second && player.mapId==mapId)
                        {
                            QIcon icon;
                            icon.addFile(QStringLiteral(":/6.png"), QSize(), QIcon::Normal, QIcon::Off);
                            tablewidgetitem->setText("");
                            tablewidgetitem->setIcon(icon);
                        }
                    }
                    ui->mapPreview->setItem(y-mapServer->min_y,x-mapServer->min_x,tablewidgetitem);

                    x++;
                }
                y++;
            }
        }
    }
    else
        ui->label_local_target->setTitle("Unknown map ("+QString::number(mapId)+")");
}

void BotTargetList::updateLayerElements()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    QListWidgetItem* selectedItem=selectedItems.at(0);
    const QString &pseudo=selectedItem->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    if(ui->comboBox_Layer->count()==0)
        return;

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    QString mapString="Unknown map ("+QString::number(mapId)+")";
    if(actionsAction->id_map_to_map.find(mapId)==actionsAction->id_map_to_map.cend())
        return;
    const std::string &mapStdString=actionsAction->id_map_to_map.at(mapId);
    mapString=QString::fromStdString(mapStdString)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
    MapServerMini *mapServer=static_cast<MapServerMini *>(map);
    if((uint32_t)ui->comboBoxStep->currentIndex()>=mapServer->step.size())
        return;
    MapServerMini::MapParsedForBot &step=mapServer->step.at(ui->comboBoxStep->currentIndex());
    if(step.map==NULL)
        return;
    ui->label_select_map->setText("Displayed map: "+QString::fromStdString(mapStdString));

    const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(ui->comboBox_Layer->currentIndex());
    alternateColor=false;
    ui->localTargets->clear();
    contentToGUI(layer.blockObject,client->api,ui->localTargets);

    ui->label_zone->setText(QString::fromStdString(layer.text));
}

void BotTargetList::on_comboBox_Layer_activated(int index)
{
    (void)index;
    updateLayerElements();
}

void BotTargetList::on_localTargets_itemActivated(QListWidgetItem *item)
{
    (void)item;

    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    QListWidgetItem* selectedItem=selectedItems.at(0);
    const QString &pseudo=selectedItem->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    if(ui->comboBox_Layer->count()==0)
        return;

    if(ui->localTargets->currentRow()<0)
        return;
    if(ui->localTargets->currentRow()>=(int32_t)mapIdListLocalTarget.size())
        return;
    const uint32_t &newMapId=mapIdListLocalTarget.at(ui->localTargets->currentRow());
    if(mapId!=newMapId)
    {
        mapId=newMapId;
        updateMapContentX=0;
        updateMapContentY=0;
        updateMapContentMapId=0;
        updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
        updateMapInformation();
        ui->trackThePlayer->setChecked(false);
    }
}

void BotTargetList::on_comboBoxStep_currentIndexChanged(int index)
{
    (void)index;
    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
    updateMapInformation();
}

void BotTargetList::on_browseMap_clicked()
{
    if(!ui->browseMap->isEnabled())
        return;
    std::vector<std::string> mapList;
    for(const auto& n:actionsAction->map_list)
        mapList.push_back(n.first);
    MapBrowse mapBrowse(mapList,this);
    mapBrowse.exec();
    const std::string &selectedMapString=mapBrowse.mapSelected();
    if(selectedMapString.empty())
        return;
    if(actionsAction->map_list.find(selectedMapString)==actionsAction->map_list.cend())
        return;
    const MapServerMini * const mapServer=static_cast<MapServerMini *>(actionsAction->map_list.at(selectedMapString));
    mapId=mapServer->id;
    ui->trackThePlayer->setChecked(false);
    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
    updateMapInformation();
}

void BotTargetList::on_searchDeep_editingFinished()
{
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);

    if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        if(playerMap->step.size()<2)
            abort();
        const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
        const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
        const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
        QString overall_graphvizText=QString::fromStdString(graphStepNearMap(client,layer.blockObject,ui->searchDeep->value()));
        if(overall_graphvizText.isEmpty())
            ui->overall_graphvizText->setVisible(false);
        else
        {
            ui->overall_graphvizText->setVisible(true);
            ui->overall_graphvizText->setPlainText(overall_graphvizText);
        }
    }
}

void BotTargetList::on_globalTargets_itemActivated(QListWidgetItem *item)
{
    (void)item;
    const int &currentRow=ui->globalTargets->currentRow();
    if(currentRow==-1)
        return;
    const ActionsBotInterface::GlobalTarget &globalTarget=targetListGlobalTarget.at(currentRow);

    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;
    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];

    if(player.target.blockObject!=NULL || player.target.type!=ActionsBotInterface::GlobalTarget::GlobalTargetType::None)
        return;
    player.target=globalTarget;
    startPlayerMove();
    updatePlayerInformation();
}

void BotTargetList::on_hideTooHard_clicked()
{
    updatePlayerInformation();
}

std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > BotTargetList::pathFinding(const MapServerMini::BlockObject * const blockObject,
        const CatchChallenger::Orientation &source_orientation, const uint8_t &source_x, const uint8_t &source_y,
        /*const MapServerMini::BlockObject * const destination_blockObject,the block link to the multi-map change*/
        const std::vector<DestinationForPath> &destinations,
        unsigned int &destinationIndexSelected,
        bool *ok)
{
    if(ok==NULL)
        abort();
    *ok=false;
    /// \todo ignore and not optimised:
    (void)source_orientation;
    auto start = std::chrono::high_resolution_clock::now();

    //resolv the path
    std::vector<std::pair<uint8_t,uint8_t> > mapPointToParseList;
    SimplifiedMapForPathFinding simplifiedMap;
    if(blockObject->map==NULL)
        abort();
    if(blockObject->map->step.size()<2)
        abort();
    if(destinations.empty())
        abort();
    {
        unsigned int index=0;
        while(index<destinations.size())
        {
            const DestinationForPath &destination=destinations.at(index);
            //if good position (no more shorter than the current position)
            if(source_x==destination.destination_x && source_y==destination.destination_y/* && destination_blockObject==source_blockObject the block link to the multi-map change*/)
            {
                *ok=true;
                destinationIndexSelected=index;
                return std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> >();
            }
            index++;
        }
    }
    const MapServerMini::MapParsedForBot &step=blockObject->map->step.at(1);
    if(step.map==NULL)
        abort();
    std::unordered_map<unsigned int,
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> >
            > pathToEachDestinations;
    const uint16_t &codeZone=step.map[source_x+source_y*blockObject->map->width];

    //init the first case
    {
        std::pair<uint8_t,uint8_t> tempPoint;
        tempPoint.first=source_x;
        tempPoint.second=source_y;
        mapPointToParseList.push_back(tempPoint);

        std::pair<uint8_t,uint8_t> coord(source_x,source_y);
        SimplifiedMapForPathFinding::PathToGo &pathToGo=simplifiedMap.pathToGo[coord];
        switch(source_orientation)
        {
            case CatchChallenger::Orientation::Orientation_top:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
            break;
            case CatchChallenger::Orientation::Orientation_bottom:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
            break;
            case CatchChallenger::Orientation::Orientation_left:
                pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
            break;
            case CatchChallenger::Orientation::Orientation_right:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
            break;
            default:
            break;
        }
        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
    }

    std::pair<uint8_t,uint8_t> coord;
    while(!mapPointToParseList.empty())
    {
        const std::pair<uint8_t,uint8_t> tempPoint=mapPointToParseList.at(0);
        const uint8_t x=tempPoint.first,y=tempPoint.second;
        mapPointToParseList.erase(mapPointToParseList.begin());
        SimplifiedMapForPathFinding::PathToGo pathToGo;
        //resolv the own point
        int index=0;
        while(index<1)/*2*/
        {
            {
                //if the right case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x+1,y);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.left.empty() || pathToGo.left.size()>nearPathToGo.left.size())
                    {
                        pathToGo.left=nearPathToGo.left;
                        pathToGo.left.back().second++;
                    }
                    if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.left.size()+1))
                    {
                        pathToGo.top=nearPathToGo.left;
                        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                    }
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.left.size()+1))
                    {
                        pathToGo.bottom=nearPathToGo.left;
                        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                    }
                }
                //if the left case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x-1,y);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.right.empty() || pathToGo.right.size()>nearPathToGo.right.size())
                    {
                        pathToGo.right=nearPathToGo.right;
                        pathToGo.right.back().second++;
                    }
                    if(pathToGo.top.empty() || pathToGo.top.size()>(nearPathToGo.right.size()+1))
                    {
                        pathToGo.top=nearPathToGo.right;
                        pathToGo.top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,1));
                    }
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>(nearPathToGo.right.size()+1))
                    {
                        pathToGo.bottom=nearPathToGo.right;
                        pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,1));
                    }
                }
                //if the top case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x,y+1);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.top.empty() || pathToGo.top.size()>nearPathToGo.top.size())
                    {
                        pathToGo.top=nearPathToGo.top;
                        pathToGo.top.back().second++;
                    }
                    if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.top.size()+1))
                    {
                        pathToGo.left=nearPathToGo.top;
                        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                    }
                    if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.top.size()+1))
                    {
                        pathToGo.right=nearPathToGo.top;
                        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                    }
                }
                //if the bottom case have been parsed
                coord=std::pair<uint8_t,uint8_t>(x,y-1);
                if(simplifiedMap.pathToGo.find(coord)!=simplifiedMap.pathToGo.cend())
                {
                    const SimplifiedMapForPathFinding::PathToGo &nearPathToGo=simplifiedMap.pathToGo.at(coord);
                    if(pathToGo.bottom.empty() || pathToGo.bottom.size()>nearPathToGo.bottom.size())
                    {
                        pathToGo.bottom=nearPathToGo.bottom;
                        pathToGo.bottom.back().second++;
                    }
                    if(pathToGo.left.empty() || pathToGo.left.size()>(nearPathToGo.bottom.size()+1))
                    {
                        pathToGo.left=nearPathToGo.bottom;
                        pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,1));
                    }
                    if(pathToGo.right.empty() || pathToGo.right.size()>(nearPathToGo.bottom.size()+1))
                    {
                        pathToGo.right=nearPathToGo.bottom;
                        pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,1));
                    }
                }
            }
            index++;
        }
        coord=std::pair<uint8_t,uint8_t>(x,y);
        if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
        {
            simplifiedMap.pathToGo[coord]=pathToGo;
        }
        {
            unsigned int index=0;
            while(index<destinations.size())
            {
                const DestinationForPath &destination=destinations.at(index);
                //if good position
                if(x==destination.destination_x && y==destination.destination_y/* && destination_blockObject==source_blockObject the block link to the multi-map change*/)
                {
                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;

                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > left=pathToGo.left;
                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > right=pathToGo.right;
                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > top=pathToGo.top;
                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > bottom=pathToGo.bottom;
                    bool disableLeft=pathToGo.left.empty();
                    bool disableRight=pathToGo.right.empty();
                    bool disableTop=pathToGo.top.empty();
                    bool disableBottom=pathToGo.bottom.empty();
                    switch(destination.destination_orientation)
                    {
                        case CatchChallenger::Orientation::Orientation_top:
                            left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                        break;
                        case CatchChallenger::Orientation::Orientation_bottom:
                            left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                        break;
                        case CatchChallenger::Orientation::Orientation_left:
                            right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                        break;
                        case CatchChallenger::Orientation::Orientation_right:
                            left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            top.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                            bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_none,0));
                        break;
                        default:
                        break;
                    }
                    CatchChallenger::Orientation set_orientation=CatchChallenger::Orientation::Orientation_none;
                    if((left.size()>right.size() && !disableRight) || (left.size()>top.size() && !disableTop) || (left.size()>bottom.size() && !disableBottom))
                        disableLeft=true;
                    if((right.size()>left.size() && !disableLeft) || (right.size()>top.size() && !disableTop) || (right.size()>bottom.size() && !disableBottom))
                        disableRight=true;
                    if((top.size()>right.size() && !disableRight) || (top.size()>left.size() && !disableLeft) || (top.size()>bottom.size() && !disableBottom))
                        disableTop=true;
                    if((bottom.size()>right.size() && !disableRight) || (bottom.size()>top.size() && !disableTop) || (bottom.size()>left.size() && !disableLeft))
                        disableBottom=true;

                    if(!disableLeft &&
                            (disableRight || left.size()<right.size()) &&
                            (disableTop || left.size()<top.size()) &&
                            (disableBottom || left.size()<bottom.size())
                            )
                        set_orientation=CatchChallenger::Orientation::Orientation_left;
                    else if(!disableRight &&
                            (disableLeft || right.size()<left.size()) &&
                            (disableTop || right.size()<top.size()) &&
                            (disableBottom || right.size()<bottom.size())
                            )
                        set_orientation=CatchChallenger::Orientation::Orientation_right;
                    else if(!disableTop &&
                            (disableRight || top.size()<right.size()) &&
                            (disableLeft || top.size()<left.size()) &&
                            (disableBottom || top.size()<bottom.size())
                            )
                        set_orientation=CatchChallenger::Orientation::Orientation_top;
                    else if(!disableBottom &&
                            (disableRight || bottom.size()<right.size()) &&
                            (disableTop || bottom.size()<top.size()) &&
                            (disableLeft || bottom.size()<left.size())
                            )
                        set_orientation=CatchChallenger::Orientation::Orientation_bottom;
                    else//by tile count based
                    {
                        //multiple same sorted
                        bool haveContent=false;
                        uint8_t bastTileCount=0;
                        set_orientation=destination.destination_orientation;
                        if(!disableLeft)
                        {
                            const uint16_t &leftadd=pathTileCount(left);
                            if(!haveContent || (haveContent && leftadd<bastTileCount))
                            {
                                haveContent=true;
                                bastTileCount=leftadd;
                                set_orientation=CatchChallenger::Orientation::Orientation_left;
                            }
                        }
                        if(!disableRight)
                        {
                            const uint16_t &rightadd=pathTileCount(right);
                            if(!haveContent || (haveContent && rightadd<bastTileCount))
                            {
                                haveContent=true;
                                bastTileCount=rightadd;
                                set_orientation=CatchChallenger::Orientation::Orientation_right;
                            }
                        }
                        if(!disableTop)
                        {
                            const uint16_t &topadd=pathTileCount(top);
                            if(!haveContent || (haveContent && topadd<bastTileCount))
                            {
                                haveContent=true;
                                bastTileCount=topadd;
                                set_orientation=CatchChallenger::Orientation::Orientation_top;
                            }
                        }
                        if(!disableBottom)
                        {
                            const uint16_t &bottomadd=pathTileCount(bottom);
                            if(!haveContent || (haveContent && bottomadd<bastTileCount))
                            {
                                haveContent=true;
                                bastTileCount=bottomadd;
                                set_orientation=CatchChallenger::Orientation::Orientation_bottom;
                            }
                        }
                        if(!haveContent)
                            abort();
                    }
                    switch(set_orientation)
                    {
                        case CatchChallenger::Orientation::Orientation_top:
                            returnedVar=pathToGo.top;
                        break;
                        case CatchChallenger::Orientation::Orientation_bottom:
                            returnedVar=pathToGo.bottom;
                        break;
                        case CatchChallenger::Orientation::Orientation_left:
                            returnedVar=pathToGo.left;
                        break;
                        case CatchChallenger::Orientation::Orientation_right:
                            returnedVar=pathToGo.right;
                        break;
                        default:
                        break;
                    }

                    if(!returnedVar.empty())
                    {
                        if(returnedVar.back().second<1)
                        {
                            if(returnedVar.size()>1)
                            {
                                if(returnedVar.at(returnedVar.size()-2).second<1)
                                {
                                    auto end = std::chrono::high_resolution_clock::now();
                                    std::chrono::duration<double, std::milli> elapsed = end-start;
                                    std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;
                                    std::cerr << "Bug from " << std::to_string(source_x) << "," << std::to_string(source_y) << ": " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)source_orientation) << " due for last step (1)" << std::endl;
                                    std::cerr << "To " << std::to_string(destination.destination_x) << "," << std::to_string(destination.destination_y) << ": " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)destination.destination_orientation) << std::endl;
                                    std::cerr << "Dump: " << BotTargetList::stepToString(returnedVar) << std::endl;
                                    std::cerr << "Last path choose: " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)set_orientation) << std::endl;
                                    return returnedVar;
                                }
                                else
                                {
                                    returnedVar[returnedVar.size()-2].second--;
                                    if(returnedVar[returnedVar.size()-2].second==0)
                                        returnedVar.erase(returnedVar.cbegin()+returnedVar.size()-2);
                                }
                            }
                            else
                            {
                                auto end = std::chrono::high_resolution_clock::now();
                                std::chrono::duration<double, std::milli> elapsed = end-start;
                                std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;
                                std::cerr << "Bug from " << std::to_string(source_x) << "," << std::to_string(source_y) << ": " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)source_orientation) << " due for last step (2)" << std::endl;
                                std::cerr << "To " << std::to_string(destination.destination_x) << "," << std::to_string(destination.destination_y) << ": " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)destination.destination_orientation) << std::endl;
                                std::cerr << "Dump: " << BotTargetList::stepToString(returnedVar) << std::endl;
                                std::cerr << "Last path choose: " << CatchChallenger::MoveOnTheMap::directionToString((CatchChallenger::Direction)set_orientation) << std::endl;
                                return returnedVar;
                            }
                        }
                        else
                        {
                            returnedVar.back().second--;
                            if(returnedVar.back().second==0)
                                returnedVar.erase(returnedVar.cbegin()+returnedVar.size()-1);
                        }
                    }
                    //add one hop if direction change
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> toAdd(CatchChallenger::Orientation::Orientation_none,0);
                    if(destination.destination_orientation!=set_orientation)
                        returnedVar.push_back(toAdd);

                    std::cout << "new destination resolved: " << std::to_string(source_x) << "," << std::to_string(source_y) << " -> " << std::to_string(x) << "," << std::to_string(y) << ": " << BotTargetList::stepToString(returnedVar) << std::endl;
                    if(!returnedVar.empty())
                    {
                        if(pathToEachDestinations.find(index)==pathToEachDestinations.cend())
                            pathToEachDestinations[index]=returnedVar;
                        else
                        {
                            if(newPathIsBetterPath(pathToEachDestinations.at(index),returnedVar))
                                pathToEachDestinations[index]=returnedVar;
                        }
                        if(pathToEachDestinations.size()>=destinations.size())
                        {
                            const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &finalVar=selectTheBetterPathToDestination(pathToEachDestinations,destinationIndexSelected);
                            auto end = std::chrono::high_resolution_clock::now();
                            std::chrono::duration<double, std::milli> elapsed = end-start;
                            std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;

                            *ok=true;
                            return finalVar;
                        }
                    }
                    else
                    {
                        pathToEachDestinations[index]=returnedVar;
                        returnedVar.clear();
                        std::cout << "Warning: Bug due to resolved path is empty, Already on blockZone border to go on the next zone" << std::endl;
                        if(pathToEachDestinations.size()>=destinations.size())
                        {
                            const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &finalVar=selectTheBetterPathToDestination(pathToEachDestinations,destinationIndexSelected);
                            auto end = std::chrono::high_resolution_clock::now();
                            std::chrono::duration<double, std::milli> elapsed = end-start;
                            std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;

                            *ok=true;
                            return finalVar;
                        }
                    }
                }
                index++;
            }
        }

        //revers resolv
        //add to point to parse
        {
            //if the right case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x+1,y);
            if(coord.first<blockObject->map->width)
            {
                const uint16_t &newCodeZone=step.map[coord.first+coord.second*blockObject->map->width];
                if(codeZone==newCodeZone && simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    if(simplifiedMap.pointQueued.find(coord)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(coord);
                        mapPointToParseList.push_back(coord);
                    }
                }
            }
            //if the left case have been parsed
            if(x>0)
            {
                coord=std::pair<uint8_t,uint8_t>(x-1,y);
                const uint16_t &newCodeZone=step.map[coord.first+coord.second*blockObject->map->width];
                if(codeZone==newCodeZone && simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    if(simplifiedMap.pointQueued.find(coord)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(coord);
                        mapPointToParseList.push_back(coord);
                    }
                }
            }
            //if the bottom case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x,y+1);
            if(coord.second<blockObject->map->height)
            {
                const uint16_t &newCodeZone=step.map[coord.first+coord.second*blockObject->map->width];
                if(codeZone==newCodeZone && simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    if(simplifiedMap.pointQueued.find(coord)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(coord);
                        mapPointToParseList.push_back(coord);
                    }
                }
            }
            //if the top case have been parsed
            if(y>0)
            {
                coord=std::pair<uint8_t,uint8_t>(x,y-1);
                const uint16_t &newCodeZone=step.map[coord.first+coord.second*blockObject->map->width];
                if(codeZone==newCodeZone && simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    if(simplifiedMap.pointQueued.find(coord)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(coord);
                        mapPointToParseList.push_back(coord);
                    }
                }
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << "Path not found into " << (uint32_t)elapsed.count() << "ms" << std::endl;
    std::cout << "From " << blockObject->map->map_file << " Block " << std::to_string(blockObject->id+1) << " " << std::to_string(source_x) << "," << std::to_string(source_y) << std::endl;
    {
        unsigned int index=0;
        while(index<destinations.size())
        {
            if(pathToEachDestinations.find(index)==pathToEachDestinations.cend())
            {
                const DestinationForPath &destination=destinations.at(index);
                std::cout << "To " << std::to_string(destination.destination_x) << "," << std::to_string(destination.destination_y) << std::endl;
            }
            index++;
        }
    }
    return std::vector<std::pair<CatchChallenger::Orientation,uint8_t> >();
}

void BotTargetList::on_trackThePlayer_clicked()
{
    if(!ui->trackThePlayer->isChecked())
        return;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    mapId=player.mapId;
    updateMapContentX=0;
    updateMapContentY=0;
    updateMapContentMapId=0;
    updateMapContentDirection=CatchChallenger::Direction::Direction_look_at_bottom;
    updateMapInformation();
    updatePlayerInformation();
    updatePlayerMap(true);
}

void BotTargetList::on_autoSelectTarget_toggled(bool checked)
{
    Q_UNUSED(checked);
    if(ui->autoSelectTarget->isChecked())
        autoStartActionTimer.start(100);
    else
        autoStartActionTimer.stop();
}

void BotTargetList::autoStartAction()
{
    CatchChallenger::Api_protocol * apiSelectClient=NULL;
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()==1)
    {
        const QString &pseudo=selectedItems.at(0)->text();
        if(!pseudoToBot.contains(pseudo))
            return;
        MultipleBotConnection::CatchChallengerClient * currentSelectedclient=pseudoToBot.value(pseudo);
        apiSelectClient=currentSelectedclient->api;
    }

    QHashIterator<CatchChallenger::Api_protocol *,ActionsAction::Player> i(actionsAction->clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        ActionsAction::Player &player=actionsAction->clientList[i.key()];
        if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
            abort();
        if(api->getCaracterSelected())
        {
            if(player.target.sinceTheLastAction.isNull() || !player.target.sinceTheLastAction.isValid())
                player.target.sinceTheLastAction.restart();
            const int &msFromStart=player.target.sinceTheLastAction.elapsed();
            if(player.target.blockObject==NULL &&
                    player.target.type==ActionsBotInterface::GlobalTarget::GlobalTargetType::None &&
                    msFromStart>1000
                    )
            {
                player.target.sinceTheLastAction.restart();
                //the best target
                const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
                const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
                if((uint32_t)playerMap->step.size()<=(uint32_t)ui->comboBoxStep->currentIndex())
                    abort();
                const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
                const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
                if(playerCodeZone<=0 || (uint32_t)(playerCodeZone-1)>=(uint32_t)stepPlayer.layers.size())
                    return;
                const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
                std::unordered_map<const MapServerMini::BlockObject *,MapServerMini::BlockObjectPathFinding> resolvedBlock;
                playerMap->targetBlockList(layer.blockObject,resolvedBlock,ui->searchDeep->value(),api);
                const MapServerMini::MapParsedForBot &step=playerMap->step.at(ui->comboBoxStep->currentIndex());
                if(step.map==NULL)
                    return;
                contentToGUI(api,NULL,resolvedBlock,false,dirt,itemOnMap,fight,shop,heal,wildMonster,player.target,playerMap,player.x,player.y);
                switch(player.target.type)
                {
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::ItemOnMap:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Fight:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Shop:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Heal:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::WildMonster:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Dirt:
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::Plant:
                        if(api==apiSelectClient)
                            startPlayerMove();
                        else
                            startPlayerMove(api);
                    break;
                    case ActionsBotInterface::GlobalTarget::GlobalTargetType::None:
                    break;
                    default:
                    break;
                }
            }
        }
    }
}

void BotTargetList::on_autoSelectFilter_clicked()
{
    TargetFilter targetFilter(this,dirt,itemOnMap,fight,shop,heal,wildMonster);
    targetFilter.exec();
    dirt=targetFilter.get_dirt();
    itemOnMap=targetFilter.get_itemOnMap();
    fight=targetFilter.get_fight();
    shop=targetFilter.get_shop();
    heal=targetFilter.get_heal();
    wildMonster=targetFilter.get_wildMonster();
    updatePlayerInformation();
}

void BotTargetList::on_hideTooHard_toggled(bool checked)
{
    Q_UNUSED(checked);
    updatePlayerInformation();
}
