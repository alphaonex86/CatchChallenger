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
    if(!actionsAction->preload_the_map_step1())
        return;
    if(!actionsAction->preload_the_map_step2())
        return;
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

    ui->localTargets->clear();
    ui->label_local_target->setEnabled(true);
    ui->comboBoxStep->setEnabled(true);
    ui->globalTargets->setEnabled(true);

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

    QList<QColor> colorsList;
    colorsList << QColor(200, 70, 70, 255);
    colorsList << QColor(255, 255, 0, 255);
    colorsList << QColor(70, 187, 70, 255);
    colorsList << QColor(100, 100, 200, 255);
    colorsList << QColor(255, 128, 128, 255);
    colorsList << QColor(180, 70, 180, 255);
    colorsList << QColor(255, 200, 110, 255);
    colorsList << QColor(115, 255, 240, 255);
    colorsList << QColor(115, 255, 120, 255);

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

        if(actionsAction->id_map_to_map.find(player.mapId)!=actionsAction->id_map_to_map.cend())
        {
            const std::string &playerMapStdString=actionsAction->id_map_to_map.at(mapId);
            CatchChallenger::CommonMap *playerMap=actionsAction->map_list.at(playerMapStdString);
            QString mapString=QString::fromStdString(playerMap->map_file)+QString(" (%1,%2)").arg(player.x).arg(player.y);
            ui->label_local_target->setTitle("Target on the map: "+mapString+", displayed map: "+QString::fromStdString(mapStdString));
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
                        QColor color=colorsList[codeZone%colorsList.size()];
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
                ui->comboBox_Layer->addItem(QString::fromStdString(layer.name),index);

                const unsigned int codeZone=(index+1);
                QColor color=colorsList[codeZone%colorsList.size()];
                //replace struct1 [label="<f0> Block 1|<f1> w"]\n -> struct1 [label="<f0> Block 1|<f1> w" style=filled fillcolor="#FFFEE0"]\n
                QRegularExpression regexcolor("(\nstruct"+QString::number(codeZone)+" [^\n]+)[^\n;]\n",QRegularExpression::InvertedGreedinessOption);
                QtGraphvizText.replace(regexcolor,"\\1 style=filled fillcolor=\""+color.name(QColor::HexRgb)+"\"]\n");

                index++;
            }
            if(step.graphvizText.empty())
                ui->graphvizText->setVisible(false);
            else
            {
                ui->graphvizText->setVisible(true);
                ui->graphvizText->setPlainText(QtGraphvizText);
            }
        }
    }
    else
        ui->label_local_target->setTitle("Unknown map ("+QString::number(mapId)+")");
    updateLayerElements();
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
        unsigned int index=0;
        while(index<layer.contentList.size())
        {
            const MapServerMini::MapParsedForBot::Layer::Content &content=layer.contentList.at(index);
            QListWidgetItem *item=new QListWidgetItem();
            item->setText(content.text);
            item->setIcon(content.icon);
            item->setData(99,content.mapId);
            ui->localTargets->addItem(item);
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

    if(mapId!=selectedItem->data(99))
    {
        mapId=selectedItem->data(99).toUInt();
        updateMapInformation();
    }
}

void BotTargetList::on_comboBoxStep_currentIndexChanged(int index)
{
    (void)index;
    updateMapInformation();
}
