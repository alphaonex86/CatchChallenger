#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "../../client/fight/interface/ClientFightEngine.h"
#include "../../general/base/CommonSettingsServer.h"
#include "MapBrowse.h"

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

    connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerStep);
    connect(&actionsAction->moveTimer,&QTimer::timeout,this,&BotTargetList::updatePlayerMapSlot);
}

BotTargetList::~BotTargetList()
{
    delete ui;
}

bool BotTargetList::newPathIsBetterPath(const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &oldPath,const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &newPath)
{
    if(oldPath.size()>=newPath.size())
    {
        if(oldPath.size()==newPath.size())
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
        else
            return false;
    }
    else
        return true;
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
    destinationIndexSelected=0;
    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnVar;
    for(const auto& n:pathToEachDestinations)
    {
        const unsigned int &key=n.first;
        const std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > &value=n.second;
        if(returnVar.empty())
        {
            destinationIndexSelected=key;
            return returnVar;
        }
        if(returnVar.empty())
        {
            destinationIndexSelected=key;
            returnVar=value;
        }
        else if(newPathIsBetterPath(returnVar,value))
        {
            destinationIndexSelected=key;
            returnVar=value;
        }
    }
    //group the 2 first action if is look into differente direction
    if(returnVar.size()>1)
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &firstValue=returnVar.front();
        if(firstValue.second==0)
            returnVar.erase(returnVar.cbegin());
    }
    //group the 2 last action if is look into differente direction
    if(returnVar.size()>1)
    {
        const std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &lastValue=returnVar.back();
        if(lastValue.second==0)
        {
            returnVar[returnVar.size()-2].first=lastValue.first;
            returnVar.erase(returnVar.cbegin()+returnVar.size()-1);
        }
    }
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
        updateMapContent();
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
    if(!actionsAction->clientList.contains(client->api))
        return;

    ActionsBotInterface::Player &player=actionsAction->clientList[client->api];

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
    uint8_t o=client->api->getDirection();
    while(o>4)
        o-=4;
    //if the target is on the same block
    fix that's
    if(player.target.bestPath.empty())
    {
        const std::pair<uint8_t,uint8_t> &point=getNextPosition(layer.blockObject,player.target/*hop list, first is the next hop*/);
        DestinationForPath destinationForPath;
        destinationForPath.destination_orientation=CatchChallenger::Orientation::Orientation_none;
        destinationForPath.destination_x=point.first;
        destinationForPath.destination_y=point.second;
        destinations.push_back(destinationForPath);
    }
    else //search the best path to the next block
    {
        if(layer.blockObject->links.find(layer.blockObject)==layer.blockObject->links.cend())
            abort();
        pointsList=layer.blockObject->links.at(layer.blockObject).points;
        if(pointsList.empty())
            abort();
        unsigned int index=0;
        while(index<pointsList.size())
        {
            const MapServerMini::BlockObject::LinkPoint &point=pointsList.at(index);
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

    ui->label_action->setText("Start this: "+QString::fromStdString(BotTargetList::stepToString(returnPath)));
    updateMapInformation();
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
                        icon.addFile(QStringLiteral(":/playerloc.png"), QSize(), QIcon::Normal, QIcon::Off);
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
    contentToGUI(layer.blockObject,client,ui->localTargets);

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
        updateMapInformation();
        ui->trackThePlayer->setChecked(false);
    }
}

void BotTargetList::on_comboBoxStep_currentIndexChanged(int index)
{
    (void)index;
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

void BotTargetList::on_tooHard_clicked()
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
    const MapServerMini::MapParsedForBot &step=blockObject->map->step.at(1);
    if(step.map==NULL)
        abort();
    std::unordered_map<unsigned int,
            std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> >
            > pathToEachDestinations;

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
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,0));
            break;
            case CatchChallenger::Orientation::Orientation_bottom:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,0));
            break;
            case CatchChallenger::Orientation::Orientation_left:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_right,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,0));
            break;
            case CatchChallenger::Orientation::Orientation_right:
                pathToGo.left.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_left,0));
                pathToGo.right.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_top,0));
                pathToGo.bottom.push_back(std::pair<CatchChallenger::Orientation,uint8_t/*step number*/>(CatchChallenger::Orientation_bottom,0));
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
                    std::cout << "new destination resolved: " << std::to_string(x) << "," << std::to_string(y) << std::endl;
                    std::vector<std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> > returnedVar;

                    //add one hop if direction change
                    std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> toAdd(destination.destination_orientation,0);
                    switch(destination.destination_orientation)
                    {
                        case CatchChallenger::Orientation::Orientation_top:
                            pathToGo.left.push_back(toAdd);
                            pathToGo.right.push_back(toAdd);
                            pathToGo.bottom.push_back(toAdd);
                        break;
                        case CatchChallenger::Orientation::Orientation_bottom:
                            pathToGo.left.push_back(toAdd);
                            pathToGo.right.push_back(toAdd);
                            pathToGo.top.push_back(toAdd);
                        break;
                        case CatchChallenger::Orientation::Orientation_left:
                            pathToGo.top.push_back(toAdd);
                            pathToGo.right.push_back(toAdd);
                            pathToGo.bottom.push_back(toAdd);
                        break;
                        case CatchChallenger::Orientation::Orientation_right:
                            pathToGo.left.push_back(toAdd);
                            pathToGo.top.push_back(toAdd);
                            pathToGo.bottom.push_back(toAdd);
                        break;
                        default:
                        break;
                    }

                    if(pathToGo.left.size()<pathToGo.right.size() && pathToGo.left.size()<pathToGo.top.size() && pathToGo.left.size()<pathToGo.bottom.size())
                        returnedVar=pathToGo.left;
                    if(pathToGo.right.size()<pathToGo.left.size() && pathToGo.right.size()<pathToGo.top.size() && pathToGo.right.size()<pathToGo.bottom.size())
                        returnedVar=pathToGo.right;
                    if(pathToGo.top.size()<pathToGo.right.size() && pathToGo.top.size()<pathToGo.left.size() && pathToGo.top.size()<pathToGo.bottom.size())
                        returnedVar=pathToGo.top;
                    if(pathToGo.bottom.size()<pathToGo.right.size() && pathToGo.bottom.size()<pathToGo.top.size() && pathToGo.bottom.size()<pathToGo.left.size())
                        returnedVar=pathToGo.bottom;
                    else//by tile count based
                    {
                        const uint16_t &leftadd=pathTileCount(pathToGo.left);
                        const uint16_t &rightadd=pathTileCount(pathToGo.right);
                        const uint16_t &topadd=pathTileCount(pathToGo.top);
                        const uint16_t &bottomadd=pathTileCount(pathToGo.bottom);

                        if(leftadd<rightadd && leftadd<topadd && leftadd<bottomadd)
                            returnedVar=pathToGo.left;
                        if(rightadd<leftadd && rightadd<topadd && rightadd<bottomadd)
                            returnedVar=pathToGo.right;
                        if(topadd<rightadd && topadd<leftadd && topadd<bottomadd)
                            returnedVar=pathToGo.top;
                        if(bottomadd<rightadd && bottomadd<topadd && bottomadd<leftadd)
                            returnedVar=pathToGo.bottom;
                        else //default path
                            returnedVar=pathToGo.bottom;
                    }

                    if(!returnedVar.empty())
                    {
                        if(returnedVar.back().second<=1)
                        {
                            std::cerr << "Bug due for last step" << std::endl;
                            return returnedVar;
                        }
                        else
                        {
                            auto end = std::chrono::high_resolution_clock::now();
                            std::chrono::duration<double, std::milli> elapsed = end-start;

                            std::cout << "Path result into " <<  (uint32_t)elapsed.count() << "ms" << std::endl;
                            returnedVar.back().second--;
                            if(pathToEachDestinations.find(index)==pathToEachDestinations.cend())
                                pathToEachDestinations[index]=returnedVar;
                            else
                            {
                                if(newPathIsBetterPath(pathToEachDestinations.at(index),returnedVar))
                                    pathToEachDestinations[index]=returnedVar;
                            }
                            if(pathToEachDestinations.size()==destinations.size())
                            {
                                *ok=true;
                                return selectTheBetterPathToDestination(pathToEachDestinations,destinationIndexSelected);
                            }
                        }
                    }
                    else
                    {
                        pathToEachDestinations[index]=returnedVar;
                        returnedVar.clear();
                        std::cout << "Warning: Bug due to resolved path is empty, Already on blockZone border to go on the next zone" << std::endl;
                        if(pathToEachDestinations.size()==destinations.size())
                        {
                            *ok=true;
                            return selectTheBetterPathToDestination(pathToEachDestinations,destinationIndexSelected);
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
                if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                    newPoint.first--;
                    if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(newPoint);
                        mapPointToParseList.push_back(newPoint);
                    }
                }
            }
            //if the left case have been parsed
            if(x>0)
            {
                coord=std::pair<uint8_t,uint8_t>(x-1,y);
                if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                    newPoint.first--;
                    if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(newPoint);
                        mapPointToParseList.push_back(newPoint);
                    }
                }
            }
            //if the bottom case have been parsed
            coord=std::pair<uint8_t,uint8_t>(x,y+1);
            if(coord.second<blockObject->map->height)
            {
                if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                    newPoint.first--;
                    if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(newPoint);
                        mapPointToParseList.push_back(newPoint);
                    }
                }
            }
            //if the top case have been parsed
            if(y>0)
            {
                coord=std::pair<uint8_t,uint8_t>(x,y-1);
                if(simplifiedMap.pathToGo.find(coord)==simplifiedMap.pathToGo.cend())
                {
                    std::pair<uint8_t,uint8_t> newPoint=tempPoint;
                    newPoint.first--;
                    if(simplifiedMap.pointQueued.find(newPoint)==simplifiedMap.pointQueued.cend())
                    {
                        simplifiedMap.pointQueued.insert(newPoint);
                        mapPointToParseList.push_back(newPoint);
                    }
                }
            }
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end-start;
    std::cout << "Path not found into " << (uint32_t)elapsed.count() << "ms" << std::endl;
    std::cout << "From " << blockObject->map << " " << std::to_string(source_x) << "," << std::to_string(source_y) << std::endl;
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
    updateMapInformation();
    updatePlayerInformation();
    updatePlayerMap(true);
}
