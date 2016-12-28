#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"
#include "MapBrowse.h"

BotTargetList::BotTargetList(QHash<CatchChallenger::Api_client_real *,MultipleBotConnection::CatchChallengerClient *> apiToCatchChallengerClient,
                             QHash<CatchChallenger::ConnectedSocket *,MultipleBotConnection::CatchChallengerClient *> connectedSocketToCatchChallengerClient,
                             QHash<QSslSocket *,MultipleBotConnection::CatchChallengerClient *> sslSocketToCatchChallengerClient,
                             ActionsAction *actionsAction) :
    ui(new Ui::BotTargetList),
    apiToCatchChallengerClient(apiToCatchChallengerClient),
    connectedSocketToCatchChallengerClient(connectedSocketToCatchChallengerClient),
    sslSocketToCatchChallengerClient(sslSocketToCatchChallengerClient),
    actionsAction(actionsAction),
    botsInformationLoaded(false)
{
    ui->setupUi(this);
    ui->graphvizText->setVisible(false);
    mapId=0;

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
}

BotTargetList::~BotTargetList()
{
    delete ui;
}

void BotTargetList::loadAllBotsInformation()
{
    if(botsInformationLoaded)
        return;
    botsInformationLoaded=true;
    if(!connect(actionsAction,&ActionsAction::preload_the_map_finished,this,&BotTargetList::loadAllBotsInformation2))
        abort();
    if(!connect(this,&BotTargetList::start_preload_the_map,actionsAction,&ActionsAction::preload_the_map))
        abort();
    emit start_preload_the_map();
    waitScreen.show();
    waitScreen.updateWaitScreen();
}

void BotTargetList::loadAllBotsInformation2()
{
    if(!actionsAction->preload_the_map_step2())
        return;
    show();
    waitScreen.hide();
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

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    mapId=player.mapId;

    updateMapInformation();
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
        QString QtGraphvizText=QString::fromStdString(step.graphvizText);
        QString overall_graphvizText;

        if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
        {
            const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
            const MapServerMini * const playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
            QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
            ui->label_local_target->setTitle("Target on the map: "+mapString+", displayed map: "+QString::fromStdString(mapStdString));
            if(playerMap->step.size()<2)
                abort();
            const MapServerMini::MapParsedForBot &stepPlayer=playerMap->step.at(1);
            const uint8_t playerCodeZone=stepPlayer.map[player.x+player.y*playerMap->width];
            const MapServerMini::MapParsedForBot::Layer &layer=stepPlayer.layers.at(playerCodeZone-1);
            overall_graphvizText=QString::fromStdString(playerMap->graphStepNearMap(layer.blockObject,ui->searchDeep->value()));
        }
        else
            ui->label_local_target->setTitle("Unknown player map ("+QString::number(player.mapId)+")");

        ui->mapPreview->setColumnCount(0);
        ui->mapPreview->setRowCount(0);
        /*ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);*/
        ui->mapPreview->setColumnCount(mapServer->max_x-mapServer->min_x);
        ui->mapPreview->setRowCount(mapServer->max_y-mapServer->min_y);

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
                    ui->mapPreview->setItem(y-mapServer->min_y,x-mapServer->min_x,tablewidgetitem);

                    x++;
                }
                y++;
            }
        }
        {
            ui->comboBox_Layer->clear();
            unsigned int index=0;
            while(index<step.layers.size())
            {
                const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(index);

                if(layer.name!="Lost layer" || !layer.contentList.empty())
                    ui->comboBox_Layer->addItem(QString::fromStdString(layer.name),index);

                index++;
            }
            if(step.graphvizText.empty())
                ui->graphvizText->setVisible(false);
            else
            {
                ui->graphvizText->setVisible(true);
                ui->graphvizText->setPlainText(QtGraphvizText);
            }
            if(overall_graphvizText.isEmpty())
                ui->overall_graphvizText->setVisible(false);
            else
            {
                ui->overall_graphvizText->setVisible(true);
                ui->overall_graphvizText->setPlainText(overall_graphvizText);
            }
        }
    }
    else
        ui->label_local_target->setTitle("Unknown map ("+QString::number(mapId)+")");
    updateLayerElements();
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

    const MapServerMini::MapParsedForBot::Layer &layer=step.layers.at(ui->comboBox_Layer->currentIndex());
    {
        ui->localTargets->clear();
        mapIdList.clear();
        unsigned int index=0;
        while(index<layer.contentList.size())
        {
            const MapServerMini::MapParsedForBot::Layer::Content &content=layer.contentList.at(index);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(content.text);
            item->setIcon(content.icon);
            ui->localTargets->addItem(item);
            mapIdList.push_back(content.mapId);
            index++;
        }
    }


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

    const uint32_t &newMapId=mapIdList.at(ui->localTargets->currentRow());
    if(mapId!=newMapId)
    {
        mapId=newMapId;
        updateMapInformation();
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
        QString overall_graphvizText=QString::fromStdString(playerMap->graphStepNearMap(layer.blockObject,ui->searchDeep->value()));
        if(overall_graphvizText.isEmpty())
            ui->overall_graphvizText->setVisible(false);
        else
        {
            ui->overall_graphvizText->setVisible(true);
            ui->overall_graphvizText->setPlainText(overall_graphvizText);
        }
    }
}
