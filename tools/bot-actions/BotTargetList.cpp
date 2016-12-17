#include "BotTargetList.h"
#include "ui_BotTargetList.h"
#include "../../client/base/interface/DatapackClientLoader.h"

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
    if(!actionsAction->preload_the_map())
        return;
}

void BotTargetList::on_bots_itemSelectionChanged()
{
    ui->localTargets->clear();
    const QList<QListWidgetItem*> &selectedItems=ui->bots->selectedItems();
    if(selectedItems.size()!=1)
        return;
    const QString &pseudo=selectedItems.at(0)->text();
    if(!pseudoToBot.contains(pseudo))
        return;
    MultipleBotConnection::CatchChallengerClient * client=pseudoToBot.value(pseudo);
    if(!actionsAction->clientList.contains(client->api))
        return;

    {
        ui->label_local_target->setEnabled(true);
        ui->comboBoxStep->setEnabled(true);
        ui->globalTargets->setEnabled(true);
    }

    const ActionsBotInterface::Player &player=actionsAction->clientList.value(client->api);
    QString mapString="Unknown map ("+QString::number(player.mapId)+")";
    if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
    {
        const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
        mapString=QString::fromStdString(mapStdString)+QString(" (%1,%2)").arg(player.x).arg(player.y);
        CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
        MapServerMini *mapServer=static_cast<MapServerMini *>(map);

        zoneHash.clear();
        {
            ui->mapPreview->setColumnCount(0);
            ui->mapPreview->setRowCount(0);
            ui->mapPreview->setColumnCount(mapServer->width);
            ui->mapPreview->setRowCount(mapServer->height);

            int y=0;
            while(y<mapServer->height)
            {
                int x=0;
                while(x<mapServer->width)
                {
                    QString zone;
                    if(mapServer->parsed_layer.walkable!=NULL)
                        if(mapServer->parsed_layer.walkable[x+y*mapServer->width])
                            zone+="w";
                    if(mapServer->parsed_layer.monstersCollisionMap!=NULL)
                        if(mapServer->parsed_layer.monstersCollisionMap[x+y*mapServer->width])
                            zone+="m"+QString::number(mapServer->parsed_layer.monstersCollisionMap[x+y*mapServer->width]);
                    if(mapServer->parsed_layer.dirt!=NULL)
                        if(mapServer->parsed_layer.dirt[x+y*mapServer->width])
                            zone+="d";
                    if(mapServer->parsed_layer.ledges!=NULL)
                        if(mapServer->parsed_layer.ledges[x+y*mapServer->width])
                            zone+="l"+QString::number(mapServer->parsed_layer.ledges[x+y*mapServer->width]);
                    if(!zoneHash.contains(zone))
                    {
                        int size=zoneHash.size();
                        zoneHash[zone]=size;
                    }
                    //color
                    int codeZone=zoneHash[zone];
                    if(codeZone>0)
                    {
                        QBrush brush1(QColor(0, 0, 0, 0));
                        brush1.setStyle(Qt::SolidPattern);
                        QTableWidgetItem *tablewidgetitem = ui->mapPreview->itemAt(y,x);
                        if(tablewidgetitem==NULL)
                            tablewidgetitem = new QTableWidgetItem();
                        tablewidgetitem->setBackground(brush1);
                        tablewidgetitem->setText(QString::number(codeZone));
                        ui->mapPreview->setItem(y,x,tablewidgetitem);
                    }
                    //icon
                    if(x==player.x && y==player.y)
                    {
                        QIcon icon;
                        icon.addFile(QStringLiteral(":/playerloc.png"), QSize(), QIcon::Normal, QIcon::Off);
                        QTableWidgetItem *tablewidgetitem = ui->mapPreview->itemAt(y,x);
                        if(tablewidgetitem==NULL)
                            tablewidgetitem = new QTableWidgetItem();
                        tablewidgetitem->setText("");
                        tablewidgetitem->setIcon(icon);
                        ui->mapPreview->setItem(y,x,tablewidgetitem);
                    }

                    x++;
                }
                y++;
            }
        }
        {
            layerList.clear();
            ui->comboBox_Layer->clear();
            zoneHash.remove(0);
            QStringList text;
            while(layerList.size()<(zoneHash.size()+1))
                layerList << "";

            QHash<QString, int>::const_iterator i = zoneHash.constBegin();
            while (i != zoneHash.constEnd()) {
                layerList[i.value()]=i.key();
                ++i;
            }

            int index=1;
            while(index<layerList.size())
            {
                const QString &string=layerList.at(index);
                text << QString("%1:%2").arg(index).arg(string);
                ui->comboBox_Layer->addItem("Layer "+QString::number(index),index);
                index++;
            }

            ui->label_zone->setText(text.join(", "));
        }
    }
    updateLayerElements();
    ui->label_local_target->setTitle("Target on the map: "+mapString);
}

void BotTargetList::updateLayerElements()
{
    ui->localTargets->clear();
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
    QString mapString="Unknown map ("+QString::number(player.mapId)+")";
    if(actionsAction->id_map_to_map.find(player.mapId)==actionsAction->id_map_to_map.cend())
        return;
    const std::string &mapStdString=actionsAction->id_map_to_map.at(player.mapId);
    mapString=QString::fromStdString(mapStdString)+QString(" (%1,%2)").arg(player.x).arg(player.y);
    CatchChallenger::CommonMap *map=actionsAction->map_list.at(mapStdString);
    MapServerMini *mapServer=static_cast<MapServerMini *>(map);
    int index=0;
    while(index<mapServer->teleporter_list_size)
    {
        const CatchChallenger::CommonMap::Teleporter &teleporter=mapServer->teleporter[index];

        QString zone;
        if(mapServer->parsed_layer.walkable!=NULL)
            if(mapServer->parsed_layer.walkable[teleporter.source_x+teleporter.source_y*mapServer->width])
                zone+="w";
        if(mapServer->parsed_layer.monstersCollisionMap!=NULL)
            if(mapServer->parsed_layer.monstersCollisionMap[teleporter.source_x+teleporter.source_y*mapServer->width])
                zone+="m"+QString::number(mapServer->parsed_layer.monstersCollisionMap[teleporter.source_x+teleporter.source_y*mapServer->width]);
        if(mapServer->parsed_layer.dirt!=NULL)
            if(mapServer->parsed_layer.dirt[teleporter.source_x+teleporter.source_y*mapServer->width])
                zone+="d";
        if(mapServer->parsed_layer.ledges!=NULL)
            if(mapServer->parsed_layer.ledges[teleporter.source_x+teleporter.source_y*mapServer->width])
                zone+="l"+QString::number(mapServer->parsed_layer.ledges[teleporter.source_x+teleporter.source_y*mapServer->width]);

        if(zoneHash.contains(zone) && zoneHash.value(zone)==ui->comboBox_Layer->currentData())
        {
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(QString("From (%1,%2) to %3 (%4,%5)\nZone %6")
                          .arg(teleporter.source_x)
                          .arg(teleporter.source_y)
                          .arg(QString::fromStdString(teleporter.map->map_file))
                          .arg(teleporter.destination_x)
                          .arg(teleporter.destination_y)
                          .arg(zone)
                          );
            item->setIcon(QIcon(":/7.png"));
            ui->localTargets->addItem(item);
        }
        index++;
    }
}

void BotTargetList::on_comboBox_Layer_activated(int index)
{
    (void)index;
    updateLayerElements();
}
