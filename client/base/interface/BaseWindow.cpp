#include "BaseWindow.h"
#include "ui_BaseWindow.h"
#include "../../general/base/FacilityLib.h"
#include "../ClientVariable.h"
#include "DatapackClientLoader.h"

#include <QListWidgetItem>
#include <QBuffer>
#include <QInputDialog>
#include <QMessageBox>
#include <QDesktopServices>

//do buy queue
//do sell queue

using namespace Pokecraft;

BaseWindow::BaseWindow(Api_protocol *client) :
    ui(new Ui::BaseWindowUI)
{
    qRegisterMetaType<Pokecraft::Chat_type>("Pokecraft::Chat_type");
    qRegisterMetaType<Pokecraft::Player_type>("Pokecraft::Player_type");
    qRegisterMetaType<Pokecraft::Player_private_and_public_informations>("Pokecraft::Player_private_and_public_informations");
    qRegisterMetaType<QHash<quint32,quint32> >("QHash<quint32,quint32>");
    qRegisterMetaType<QHash<quint32,quint32> >("Pokecraft::Plant_collect");
    qRegisterMetaType<QList<ItemToSellOrBuy> >("QList<ItemToSell>");

    this->client=client;
    socketState=QAbstractSocket::UnconnectedState;

    mapController=new MapController(client,true,false,true,false);
    ProtocolParsing::initialiseTheVariable();
    ui->setupUi(this);
    chat=new Chat(ui->page_map,client);

    connect(client,SIGNAL(protocol_is_good()),this,SLOT(protocol_is_good()),Qt::QueuedConnection);
    connect(client,SIGNAL(disconnected(QString)),this,SLOT(disconnected(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(error(QString)),this,SLOT(error(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(message(QString)),this,SLOT(message(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(notLogged(QString)),this,SLOT(notLogged(QString)),Qt::QueuedConnection);
    connect(client,SIGNAL(logged()),this,SLOT(logged()),Qt::QueuedConnection);
    connect(client,SIGNAL(haveTheDatapack()),this,SLOT(haveTheDatapack()),Qt::QueuedConnection);
    connect(client,SIGNAL(newError(QString,QString)),this,SLOT(newError(QString,QString)),Qt::QueuedConnection);

    //connect the map controler
    connect(client,SIGNAL(have_current_player_info(Pokecraft::Player_private_and_public_informations)),this,SLOT(have_current_player_info()),Qt::QueuedConnection);

    //inventory
    connect(client,SIGNAL(have_inventory(QHash<quint32,quint32>)),this,SLOT(have_inventory(QHash<quint32,quint32>)));
    connect(client,SIGNAL(add_to_inventory(QHash<quint32,quint32>)),this,SLOT(add_to_inventory(QHash<quint32,quint32>)));
    connect(client,SIGNAL(remove_to_inventory(QHash<quint32,quint32>)),this,SLOT(remove_to_inventory(QHash<quint32,quint32>)));

    //chat
    connect(client,SIGNAL(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)),chat,SLOT(new_chat_text(Pokecraft::Chat_type,QString,QString,Pokecraft::Player_type)));
    connect(client,SIGNAL(new_system_text(Pokecraft::Chat_type,QString)),chat,SLOT(new_system_text(Pokecraft::Chat_type,QString)));
    connect(this,SIGNAL(sendsetMultiPlayer(bool)),chat,SLOT(setVisible(bool)),Qt::QueuedConnection);

    connect(client,SIGNAL(number_of_player(quint16,quint16)),this,SLOT(number_of_player(quint16,quint16)));
    //connect(client,SIGNAL(new_player_info()),this,SLOT(update_chat()),Qt::QueuedConnection);

    //connect the datapack loader
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),this,SLOT(datapackParsed()),Qt::QueuedConnection);
    connect(this,SIGNAL(parseDatapack(QString)),&DatapackClientLoader::datapackLoader,SLOT(parseDatapack(QString)),Qt::QueuedConnection);
    connect(&DatapackClientLoader::datapackLoader,SIGNAL(datapackParsed()),mapController,SLOT(datapackParsed()),Qt::QueuedConnection);

    //render, logical part into Map_Client
    connect(mapController,SIGNAL(stopped_in_front_of(Pokecraft::Map_client*,quint8,quint8)),this,SLOT(stopped_in_front_of(Pokecraft::Map_client*,quint8,quint8)));
    connect(mapController,SIGNAL(actionOn(Pokecraft::Map_client*,quint8,quint8)),this,SLOT(actionOn(Pokecraft::Map_client*,quint8,quint8)));
    connect(mapController,SIGNAL(blockedOn(Pokecraft::Map_client*,quint8,quint8)),this,SLOT(blockedOn(Pokecraft::Map_client*,quint8,quint8)));

    //fight
    connect(client,SIGNAL(random_seeds(QByteArray)),&DatapackClientLoader::datapackLoader.fightEngine,SLOT(appendRandomSeeds(QByteArray)));

    //plants
    connect(this,SIGNAL(useSeed(quint8)),client,SLOT(useSeed(quint8)));
    connect(this,SIGNAL(collectMaturePlant()),client,SLOT(collectMaturePlant()));
    connect(client,SIGNAL(seed_planted(bool)),this,SLOT(seed_planted(bool)));
    connect(client,SIGNAL(plant_collected(Pokecraft::Plant_collect)),this,SLOT(plant_collected(Pokecraft::Plant_collect)));
    //crafting
    connect(client,SIGNAL(recipeUsed(RecipeUsage)),this,SLOT(recipeUsed(RecipeUsage)));
    //inventory
    connect(client,SIGNAL(objectUsed(ObjectUsage)),this,SLOT(objectUsed(ObjectUsage)));
    //shop
    connect(client,SIGNAL(haveShopList(QList<ItemToSellOrBuy>)),this,SLOT(haveShopList(QList<ItemToSellOrBuy>)));
    connect(client,SIGNAL(haveSellObject(SoldStat,quint32)),this,SLOT(haveSellObject(SoldStat,quint32)));
    connect(client,SIGNAL(haveBuyObject(BuyStat,quint32)),this,SLOT(haveBuyObject(BuyStat,quint32)));

    connect(this,SIGNAL(destroyObject(quint32,quint32)),client,SLOT(destroyObject(quint32,quint32)));
    connect(&updateRXTXTimer,SIGNAL(timeout()),this,SLOT(updateRXTX()));

    updateRXTXTimer.start(1000);
    updateRXTXTime.restart();

    tip_timeout.setInterval(TIMETODISPLAY_TIP);
    gain_timeout.setInterval(TIMETODISPLAY_GAIN);
    tip_timeout.setSingleShot(true);
    gain_timeout.setSingleShot(true);
    connect(&tip_timeout,SIGNAL(timeout()),this,SLOT(tipTimeout()));
    connect(&gain_timeout,SIGNAL(timeout()),this,SLOT(gainTimeout()));

    mapController->setDatapackPath(client->get_datapack_base_name());

    renderFrame = new QFrame(ui->page_map);
    renderFrame->setObjectName(QString::fromUtf8("renderFrame"));
    renderFrame->setMinimumSize(QSize(600, 572));
    QVBoxLayout *renderLayout = new QVBoxLayout(renderFrame);
    renderLayout->setSpacing(0);
    renderLayout->setContentsMargins(0, 0, 0, 0);
    renderLayout->setObjectName(QString::fromUtf8("renderLayout"));
    renderLayout->addWidget(mapController);
    renderFrame->setGeometry(QRect(0, 0, 800, 516));
    renderFrame->lower();
    renderFrame->lower();
    renderFrame->lower();

    chat->setGeometry(QRect(0, 0, 300, 400));

    resetAll();
    loadSettings();

    mapController->setFocus();
}

BaseWindow::~BaseWindow()
{
    delete ui;
    delete mapController;
    delete chat;
}

QString BaseWindow::lastLocation() const
{
    return mapController->lastLocation();
}

void BaseWindow::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
    break;
    default:
    break;
    }
}

void BaseWindow::message(QString message)
{
    qDebug() << message;
}

void BaseWindow::number_of_player(quint16 number,quint16 max)
{
    ui->frame_main_display_interface_player->show();
    ui->label_interface_number_of_player->setText(QString("%1/%2").arg(number).arg(max));
}

void BaseWindow::on_toolButton_interface_quit_clicked()
{
    client->tryDisconnect();
}

void BaseWindow::on_toolButton_quit_interface_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void BaseWindow::on_pushButton_interface_trainer_clicked()
{
    ui->stackedWidget->setCurrentIndex(2);
}

//return ok, itemId
void BaseWindow::selectObject(const ObjectType &objectType)
{
    inSelection=true;
    waitedObjectType=objectType;
    switch(objectType)
    {
        case ObjectType_Seed:
            ui->stackedWidget->setCurrentIndex(5);
            on_listPlantList_itemSelectionChanged();
        break;
        case ObjectType_Sell:
            ui->stackedWidget->setCurrentIndex(7);
            displaySellList();
        break;
        case ObjectType_All:
        default:
            ui->stackedWidget->setCurrentIndex(3);
            on_listCraftingList_itemSelectionChanged();
        break;
    }
}

void BaseWindow::objectSelection(const bool &ok, const quint32 &itemId, const quint32 &quantity)
{
    inSelection=false;
    ui->stackedWidget->setCurrentIndex(1);
    ui->inventoryUse->setText(tr("Select"));
    switch(waitedObjectType)
    {
        case ObjectType_Sell:
            ui->plantUse->setVisible(false);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            if(items[itemId]<quantity)
            {
                qDebug() << "item id have not the quantity";
                break;
            }
            items[itemId]-=quantity;
            if(items[itemId]==0)
                items.remove(itemId);
            ItemToSellOrBuy tempItem;
            tempItem.object=itemId;
            tempItem.quantity=quantity;
            tempItem.price=DatapackClientLoader::datapackLoader.items[itemId].price/2;
            itemsToSell << tempItem;
            client->sellObject(shopId,tempItem.object,tempItem.quantity,tempItem.price);
            load_inventory();
            load_plant_inventory();
        break;
        case ObjectType_Seed:
            ui->plantUse->setVisible(false);
            if(!ok)
                break;
            if(!items.contains(itemId))
            {
                qDebug() << "item id is not into the inventory";
                break;
            }
            items[itemId]--;
            if(items[itemId]==0)
                items.remove(itemId);
            seed_in_waiting=itemId;
            addQuery(QueryType_Seed);
            seedWait=true;
            if(DatapackClientLoader::datapackLoader.itemToPlants.contains(itemId))
            {
                load_plant_inventory();
                load_inventory();
                qDebug() << QString("send seed for: %1").arg(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
                emit useSeed(DatapackClientLoader::datapackLoader.itemToPlants[itemId]);
            }
            else
                qDebug() << QString("seed not found for item: %1").arg(itemId);
        break;
        default:
        qDebug() << "waitedObjectType is unknow";
        return;
    }
}

void BaseWindow::add_to_inventory(const QHash<quint32,quint32> &items)
{
    QString html=tr("You have obtained: ");
    QStringList objects;
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
            this->items[i.key()]+=i.value();
        else
            this->items[i.key()]=i.value();

        QPixmap image;
        QString name;
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            image=DatapackClientLoader::datapackLoader.items[i.key()].image;
            name=DatapackClientLoader::datapackLoader.items[i.key()].name;
        }
        else
        {
            image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
            name=QString("id: %1").arg(i.key());
        }

        image=image.scaled(24,24);
        QByteArray byteArray;
        QBuffer buffer(&byteArray);
        image.save(&buffer, "PNG");
        if(objects.size()<16)
        {
            if(i.value()>1)
                objects << QString("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
            else
                objects << QString("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
        }
    }
    if(objects.size()==16)
        objects << "...";
    html+=objects.join(", ");
    showGain(html);

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();
}

void BaseWindow::remove_to_inventory(const QHash<quint32,quint32> &items)
{
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(this->items.contains(i.key()))
        {
            if(this->items[i.key()]<=i.value())
                this->items.remove(i.key());
            else
                this->items[i.key()]-=i.value();
        }
    }
    load_inventory();
    load_plant_inventory();
}

void BaseWindow::newError(QString error,QString detailedError)
{
    qDebug() << detailedError.toLocal8Bit();
    if(socketState!=QAbstractSocket::ConnectedState)
        return;
    client->tryDisconnect();
    resetAll();
    QMessageBox::critical(this,tr("Error"),error);
}

void BaseWindow::error(QString error)
{
    newError("Error with the protocol",error);
}

void BaseWindow::on_pushButton_interface_bag_clicked()
{
    if(inSelection)
    {
        qDebug() << "BaseWindow::on_pushButton_interface_bag_clicked() in selection, can't click here";
        return;
    }
    ui->stackedWidget->setCurrentIndex(3);
}

void BaseWindow::on_toolButton_quit_inventory_clicked()
{
    ui->inventory->reset();
    ui->stackedWidget->setCurrentIndex(1);
    if(inSelection)
        objectSelection(false,0);
    on_inventory_itemSelectionChanged();
}

void BaseWindow::on_inventory_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText("");
        ui->inventory_description->setText(tr("Select an object"));
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(false);
        return;
    }
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.items.contains(items_graphical[item]))
    {
        ui->inventoryInformation->setVisible(false);
        ui->inventoryUse->setVisible(false);
        ui->inventoryDestroy->setVisible(!inSelection);
        ui->inventory_image->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->inventory_name->setText(tr("Unknow name"));
        ui->inventory_description->setText(tr("Unknow description"));
        return;
    }

    const DatapackClientLoader::Item &content=DatapackClientLoader::datapackLoader.items[items_graphical[item]];
    ui->inventoryInformation->setVisible(!inSelection &&
                                         /* is a plant */
                                         DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item])
                                         );
    ui->inventoryUse->setVisible(!inSelection &&
                                         /* is a recipe */
                                         DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(items_graphical[item])
                                         );
    ui->inventoryDestroy->setVisible(!inSelection);
    ui->inventory_image->setPixmap(content.image);
    ui->inventory_name->setText(content.name);
    ui->inventory_description->setText(content.description);
}

void BaseWindow::tipTimeout()
{
    ui->tip->setVisible(false);
}

void BaseWindow::gainTimeout()
{
    ui->gain->setVisible(false);
}

void BaseWindow::showTip(const QString &tip)
{
    ui->tip->setVisible(true);
    ui->tip->setText(tip);
    tip_timeout.start();
}

void BaseWindow::showGain(const QString &gain)
{
    ui->gain->setVisible(true);
    ui->gain->setText(gain);
    gain_timeout.start();
}

void BaseWindow::addQuery(const QueryType &queryType)
{
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(true);
    queryList << queryType;
    updateQueryList();
}

void BaseWindow::removeQuery(const QueryType &queryType)
{
    queryList.removeOne(queryType);
    if(queryList.size()==0)
        ui->persistant_tip->setVisible(false);
    else
        updateQueryList();
}

void BaseWindow::updateQueryList()
{
    QStringList queryStringList;
    int index=0;
    while(index<queryList.size() && index<5)
    {
        switch(queryList.at(index))
        {
            case QueryType_Seed:
                queryStringList << tr("Planting seed...");
            break;
            case QueryType_CollectPlant:
                queryStringList << tr("Collecting plant...");
            break;
            default:
                queryStringList << tr("Unknow action...");
            break;
        }
        index++;
    }
    ui->persistant_tip->setText(queryStringList.join("<br />"));
}

void BaseWindow::on_toolButton_quit_options_clicked()
{
    ui->stackedWidget->setCurrentIndex(1);
}

void BaseWindow::stopped_in_front_of(Pokecraft::Map_client *map, quint8 x, quint8 y)
{
    if(stopped_in_front_of_check_bot(map,x,y))
        return;
    else if(Pokecraft::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
                    showTip(tr("To recolt the plant press <i>Enter</i>"));
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        showTip(tr("To plant a seed press <i>Enter</i>"));
        return;
    }
    else
    {
        //check bot with border
        Pokecraft::Map * current_map=map;
        switch(mapController->getDirection())
        {
            case Pokecraft::Direction_look_at_left:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_left,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_right:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_right,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_top:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_top,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_bottom:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_bottom,&current_map,&x,&y,false))
                    stopped_in_front_of_check_bot(map,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::stopped_in_front_of_check_bot(Pokecraft::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    showTip(tr("To interact with the bot press <i>Enter</i>"));
    return true;
}

void BaseWindow::actionOn(Map_client *map, quint8 x, quint8 y)
{
    if(actionOnCheckBot(map,x,y))
        return;
    else if(Pokecraft::MoveOnTheMap::isDirt(*map,x,y))
    {
        int index=0;
        while(index<map->plantList.size())
        {
            if(map->plantList.at(index).x==x && map->plantList.at(index).y==y)
            {
                if(collectWait)
                {
                    showTip(tr("Wait to finish to collect the previous plant"));
                    return;
                }
                quint64 current_time=QDateTime::currentMSecsSinceEpoch()/1000;
                if(map->plantList.at(index).mature_at<current_time)
                {
                    collectWait=true;
                    addQuery(QueryType_CollectPlant);
                    emit collectMaturePlant();
                }
                else
                    showTip(tr("This plant is growing and can't be collected"));
                return;
            }
            index++;
        }
        if(seedWait)
        {
            showTip(tr("Wait to finish to plant the previous seed"));
            return;
        }
        selectObject(ObjectType_Seed);
        return;
    }
    else
    {
        //check bot with border
        Pokecraft::Map * current_map=map;
        switch(mapController->getDirection())
        {
            case Pokecraft::Direction_look_at_left:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_left,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_left,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_right:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_right,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_right,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_top:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_top,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_top,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            case Pokecraft::Direction_look_at_bottom:
            if(Pokecraft::MoveOnTheMap::canGoTo(Pokecraft::Direction_move_at_bottom,*map,x,y,false))
                if(Pokecraft::MoveOnTheMap::move(Pokecraft::Direction_move_at_bottom,&current_map,&x,&y,false))
                    actionOnCheckBot(map,x,y);
            break;
            default:
            break;
        }
    }
}

bool BaseWindow::actionOnCheckBot(Pokecraft::Map_client *map, quint8 x, quint8 y)
{
    if(!map->bots.contains(QPair<quint8,quint8>(x,y)))
        return false;
    actualBot=map->bots[QPair<quint8,quint8>(x,y)];
    goToBotStep(1);
    return true;
}

void BaseWindow::blockedOn(Pokecraft::Map_client *map, quint8 x, quint8 y)
{
    if(!canFight)
        if(Pokecraft::MoveOnTheMap::isGrass(*map,x,y))
        {
            qDebug() << "block on:" << map->map_file << x << y;
            showTip(tr("You can't enter to the grass if you are not able to fight"));
            return;
        }
}

//network
void BaseWindow::updateRXTX()
{
    quint64 RXSize=client->getRXSize();
    quint64 TXSize=client->getTXSize();
    if(previousRXSize>RXSize)
        previousRXSize=RXSize;
    if(previousTXSize>TXSize)
        previousTXSize=TXSize;
    double RXSpeed=(RXSize-previousRXSize)*1000/updateRXTXTime.elapsed();
    double TXSpeed=(TXSize-previousTXSize)*1000/updateRXTXTime.elapsed();
    if(RXSpeed<1024)
        ui->labelInput->setText(QString("%1B/s").arg(RXSpeed,0,'g',0));
    else if(RXSpeed<10240)
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',1));
    else
        ui->labelInput->setText(QString("%1KB/s").arg(RXSpeed/1024,0,'g',0));
    if(TXSpeed<1024)
        ui->labelOutput->setText(QString("%1B/s").arg(TXSpeed,0,'g',0));
    else if(TXSpeed<10240)
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',1));
    else
        ui->labelOutput->setText(QString("%1KB/s").arg(TXSpeed/1024,0,'g',0));
    if(RXSpeed>0 && TXSpeed>0)
        qDebug() << QString("received: %1B/s, transmited: %2B/s").arg(RXSpeed).arg(TXSpeed);
    else
    {
        if(RXSpeed>0)
            qDebug() << QString("received: %1B/s").arg(RXSpeed);
        if(TXSpeed>0)
            qDebug() << QString("transmited: %1B/s").arg(TXSpeed);
    }
    updateRXTXTime.restart();
    previousRXSize=RXSize;
    previousTXSize=TXSize;
}

//bot
void BaseWindow::goToBotStep(const quint8 &step)
{
    if(!actualBot.step.contains(step))
    {
        showTip(tr("Error into the bot, repport this error please"));
        return;
    }
    if(actualBot.step[step].attribute("type")=="text")
    {
        QDomElement text = actualBot.step[step].firstChildElement("text");
        while(!text.isNull())
        {
            if(text.hasAttribute("lang") && text.attribute("lang")!="en")
            {
                //not enlish, skip for now
            }
            else
            {
                QString textToShow=text.text();
                textToShow.replace("href=\"http","style=\"color:#BB9900;\" href=\"http",Qt::CaseInsensitive);
                textToShow.replace(QRegExp("(href=\"http[^>]+>[^<]+)</a>"),"\\1 <img src=\":/images/link.png\" alt=\"\" /></a>");
                ui->IG_dialog_text->setText(textToShow);
                ui->IG_dialog->setVisible(true);
                return;
            }
            text = actualBot.step[step].nextSiblingElement("text");
        }
        showTip(tr("Bot text not found, repport this error please"));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="shop")
    {
        if(client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
        {
            pixmap=QPixmap(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
            if(pixmap.isNull())
            {
                qDebug() << QString("Unable to load seller skin: %1").arg(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
                pixmap=QPixmap(":/images/player_default/front.png");
            }
        }
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        ui->stackedWidget->setCurrentIndex(7);
        ui->shopItemList->clear();
        on_shopItemList_itemSelectionChanged();
        ui->shopDescription->setText(tr("Waiting the shop content"));
        ui->shopBuy->setVisible(false);
        qDebug() << "goToBotStep(), client->getShopList(shopId): " << shopId;
        client->getShopList(shopId);
        ui->shopCash->setText(tr("Cash: %1").arg(cash));
        return;
    }
    else if(actualBot.step[step].attribute("type")=="sell")
    {
        if(!actualBot.step[step].hasAttribute("shop"))
        {
            showTip(tr("The shop call, but missing informations"));
            return;
        }
        bool ok;
        shopId=actualBot.step[step].attribute("shop").toUInt(&ok);
        if(!ok)
        {
            showTip(tr("The shop call, but wrong shop id"));
            return;
        }
        QPixmap pixmap;
        if(actualBot.properties.contains("skin"))
        {
            pixmap=QPixmap(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
            if(pixmap.isNull())
            {
                qDebug() << QString("Unable to load seller skin: %1").arg(client->get_datapack_base_name()+DATAPACK_BASE_PATH_SKIN+"/"+actualBot.properties["skin"]+"/front.png");
                pixmap=QPixmap(":/images/player_default/front.png");
            }
        }
        else
            pixmap=QPixmap(":/images/player_default/front.png");
        pixmap=pixmap.scaled(160,160);
        ui->shopSellerImage->setPixmap(pixmap);
        if(client->getHaveShopAction())
        {
            showTip(tr("Already in shop action"));
            return;
        }
        waitToSell=true;
        selectObject(ObjectType_Sell);
        return;
    }
    else
    {
        showTip(tr("Bot step type error, repport this error please"));
        return;
    }
}

void BaseWindow::on_inventory_itemActivated(QListWidgetItem *item)
{
    if(!items_graphical.contains(item))
    {
        qDebug() << "BaseWindow::on_inventory_itemActivated(): activated item not found";
        return;
    }
    if(inSelection)
    {
        objectSelection(true,items_graphical[item]);
        return;
    }

    //is crafting recipe
    if(DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(items_graphical[item]))
    {
        Player_private_and_public_informations informations=client->get_player_informations();
        if(informations.recipes.contains(DatapackClientLoader::datapackLoader.itemToCrafingRecipes[items_graphical[item]]))
        {
            QMessageBox::information(this,tr("Information"),tr("You already know this recipe"));
            return;
        }
        objectInUsing << items_graphical[item];
        items[items_graphical[item]]--;
        if(items[items_graphical[item]]==0)
            items.remove(items_graphical[item]);
        client->useObject(items_graphical[item]);
        load_inventory();
    }
    else
        qDebug() << "BaseWindow::on_inventory_itemActivated(): unknow object type";
}

void BaseWindow::objectUsed(const ObjectUsage &objectUsage)
{
    switch(objectUsage)
    {
        case ObjectUsage_correctlyUsed:
        //is crafting recipe
        if(DatapackClientLoader::datapackLoader.itemToCrafingRecipes.contains(objectInUsing.first()))
        {
            client->addRecipe(DatapackClientLoader::datapackLoader.itemToCrafingRecipes[objectInUsing.first()]);
            load_crafting_inventory();
        }
        else
            qDebug() << "BaseWindow::objectUsed(): unknow object type";
        break;
        case ObjectUsage_failed:
        break;
        case ObjectUsage_impossible:
            if(items.contains(objectInUsing.first()))
                items[objectInUsing.first()]++;
            else
                items[objectInUsing.first()]=1;
            load_inventory();
        break;
        default:
        break;
    }
    objectInUsing.removeFirst();
}

void BaseWindow::on_inventoryDestroy_clicked()
{
    qDebug() << "on_inventoryDestroy_clicked()";
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    quint32 itemId=items_graphical[items.first()];
    if(!this->items.contains(itemId))
        return;
    quint32 quantity=this->items[itemId];
    if(quantity>1)
    {
        bool ok;
        quint32 quantity_temp=QInputDialog::getInt(this,tr("Destroy"),tr("Quantity to destroy"),quantity,1,quantity,1,&ok);
        if(!ok)
            return;
        quantity=quantity_temp;
    }
    QMessageBox::StandardButton button;
    if(DatapackClientLoader::datapackLoader.items.contains(itemId))
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 %2?").arg(quantity).arg(DatapackClientLoader::datapackLoader.items[itemId].name),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    else
        button=QMessageBox::question(this,tr("Destroy"),tr("Are you sure you want to destroy %1 unknow item (id: %2)?").arg(quantity).arg(itemId),QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes);
    if(button!=QMessageBox::Yes)
        return;
    if(!this->items.contains(itemId))
        return;
    if(this->items[itemId]<quantity)
        quantity=this->items[itemId];
    emit destroyObject(itemId,quantity);
    if(this->items[itemId]<=quantity)
        this->items.remove(itemId);
    else
        this->items[itemId]-=quantity;
    load_inventory();
    load_plant_inventory();
}

void BaseWindow::on_inventoryUse_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
        return;
    on_inventory_itemActivated(items.first());
}

void BaseWindow::on_inventoryInformation_clicked()
{
    QList<QListWidgetItem *> items=ui->inventory->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_inventoryInformation_clicked() should not be accessible here";
        return;
    }
    QListWidgetItem *item=items.first();
    if(!items_graphical.contains(item))
    {
        qDebug() << "on_inventoryInformation_clicked() item not found here";
        return;
    }
    if(DatapackClientLoader::datapackLoader.itemToPlants.contains(items_graphical[item]))
    {
        if(!plants_items_to_graphical.contains(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]))
        {
            qDebug() << QString("on_inventoryInformation_clicked() is not into plant list: item: %1, plant: %2").arg(items_graphical[item]).arg(DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]);
            return;
        }
        ui->listPlantList->reset();
        ui->stackedWidget->setCurrentIndex(5);
        plants_items_to_graphical[DatapackClientLoader::datapackLoader.itemToPlants[items_graphical[item]]]->setSelected(true);
        on_listPlantList_itemSelectionChanged();
    }
    else
    {
        qDebug() << "on_inventoryInformation_clicked() information on unknow object type";
        return;
    }
}

void BaseWindow::on_toolButtonOptions_clicked()
{
    ui->stackedWidget->setCurrentIndex(4);
}

void BaseWindow::on_IG_dialog_text_linkActivated(const QString &link)
{
    ui->IG_dialog->setVisible(false);
    if(link.startsWith("http://") || link.startsWith("https://"))
    {
        if(!QDesktopServices::openUrl(QUrl(link)))
            showTip(QString("Unable to open the url: %1").arg(link));
        return;
    }
    bool ok;
    quint8 step=link.toUShort(&ok);
    if(!ok)
    {
        showTip(QString("Unable to open interpret the link: %1").arg(link));
        return;
    }
    goToBotStep(step);
}

void BaseWindow::on_toolButton_quit_shop_clicked()
{
    waitToSell=false;
    inSelection=false;
    ui->stackedWidget->setCurrentIndex(1);
}

void BaseWindow::on_shopItemList_itemActivated(QListWidgetItem *item)
{
    if(client->getHaveShopAction())
        return;
    if(!waitToSell)
    {
        if(cash<itemsIntoTheShop[shop_items_graphical[item]].price)
        {
            QMessageBox::information(this,tr("Buy"),tr("You have not the cash to buy this item"));
            return;
        }
        bool ok=true;
        if(cash/itemsIntoTheShop[shop_items_graphical[item]].price>1)
            tempQuantityForBuy=QInputDialog::getInt(this,tr("Buy"),tr("Quantity to buy"),1,1,cash/itemsIntoTheShop[shop_items_graphical[item]].price,1,&ok);
        else
            tempQuantityForBuy=1;
        if(!ok)
            return;
        tempItemForBuy=shop_items_graphical[item];
        client->buyObject(shopId,tempItemForBuy,tempQuantityForBuy,itemsIntoTheShop[tempItemForBuy].price);
        ui->stackedWidget->setCurrentIndex(1);
        tempCashForBuy=itemsIntoTheShop[tempItemForBuy].price*tempQuantityForBuy;
        removeCash(tempCashForBuy);
        showTip(tr("Buying the object..."));
    }
    else
    {
        if(!items.contains(shop_items_graphical[item]))
            return;
        bool ok=true;
        if(items[shop_items_graphical[item]]>1)
            tempQuantityForSell=QInputDialog::getInt(this,tr("Sell"),tr("Quantity to sell"),1,1,items[shop_items_graphical[item]],1,&ok);
        else
            tempQuantityForSell=1;
        if(!ok)
            return;
        if(!items.contains(shop_items_graphical[item]))
            return;
        if(items[shop_items_graphical[item]]<tempQuantityForSell)
            return;
        objectSelection(true,shop_items_graphical[item],tempQuantityForSell);
        ui->stackedWidget->setCurrentIndex(1);
        showTip(tr("Selling the object..."));
    }
}

void BaseWindow::on_shopItemList_itemSelectionChanged()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        ui->shopName->setText("");
        ui->shopDescription->setText(tr("Select an object"));
        ui->shopBuy->setVisible(false);
        ui->shopImage->setPixmap(QPixmap(":/images/inventory/unknow-object.png"));
        return;
    }
    ui->shopBuy->setVisible(true);
    QListWidgetItem *item=items.first();
    if(!DatapackClientLoader::datapackLoader.items.contains(shop_items_graphical[item]))
    {
        ui->shopImage->setPixmap(DatapackClientLoader::datapackLoader.defaultInventoryImage());
        ui->shopName->setText(tr("Unknow name"));
        ui->shopDescription->setText(tr("Unknow description"));
        return;
    }
    const DatapackClientLoader::Item &content=DatapackClientLoader::datapackLoader.items[shop_items_graphical[item]];

    ui->shopImage->setPixmap(content.image);
    ui->shopName->setText(content.name);
    ui->shopDescription->setText(content.description);
}

void BaseWindow::on_shopBuy_clicked()
{
    QList<QListWidgetItem *> items=ui->shopItemList->selectedItems();
    if(items.size()!=1)
    {
        qDebug() << "on_shopBuy_clicked() no correct selection";
        return;
    }
    on_shopItemList_itemActivated(items.first());
}

void BaseWindow::haveShopList(const QList<ItemToSellOrBuy> &items)
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::haveShopList()";
    #endif
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    int index=0;
    while(index<items.size())
    {
        itemsIntoTheShop[items.at(index).object]=items.at(index);
        QListWidgetItem *item=new QListWidgetItem();
        shop_items_to_graphical[items.at(index).object]=item;
        shop_items_graphical[item]=items.at(index).object;
        if(DatapackClientLoader::datapackLoader.items.contains(items.at(index).object))
        {
            item->setIcon(DatapackClientLoader::datapackLoader.items[items.at(index).object].image);
            if(items.at(index).quantity==0)
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.items[items.at(index).object].name).arg(items.at(index).price));
            else
                item->setText(tr("%1 at %2$\nQuantity: %3").arg(DatapackClientLoader::datapackLoader.items[items.at(index).object].name).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        else
        {
            item->setIcon(DatapackClientLoader::datapackLoader.defaultInventoryImage());
            if(items.at(index).quantity==0)
                item->setText(tr("Item %1\nPrice: %2$").arg(items.at(index).object).arg(items.at(index).price));
            else
                item->setText(tr("Item %1 at %2$\nQuantity: %3").arg(items.at(index).object).arg(items.at(index).price).arg(items.at(index).quantity));
        }
        ui->shopItemList->addItem(item);
        index++;
    }
    ui->shopBuy->setText(tr("Buy"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::displaySellList()
{
    #ifdef DEBUG_BASEWINDOWS
    qDebug() << "BaseWindow::displaySellList()";
    #endif
    ui->shopItemList->clear();
    itemsIntoTheShop.clear();
    shop_items_graphical.clear();
    shop_items_to_graphical.clear();
    QHashIterator<quint32,quint32> i(items);
    while (i.hasNext()) {
        i.next();
        if(DatapackClientLoader::datapackLoader.items.contains(i.key()))
        {
            QListWidgetItem *item=new QListWidgetItem();
            shop_items_to_graphical[i.key()]=item;
            shop_items_graphical[item]=i.key();
            item->setIcon(DatapackClientLoader::datapackLoader.items[i.key()].image);
            if(i.value()>1)
                item->setText(tr("%1\nPrice: %2$, quantity: %3").arg(DatapackClientLoader::datapackLoader.items[i.key()].name).arg(DatapackClientLoader::datapackLoader.items[i.key()].price/2).arg(i.value()));
            else
                item->setText(tr("%1\nPrice: %2$").arg(DatapackClientLoader::datapackLoader.items[i.key()].name).arg(DatapackClientLoader::datapackLoader.items[i.key()].price/2));
            ui->shopItemList->addItem(item);
        }
    }
    ui->shopBuy->setText(tr("Sell"));
    on_shopItemList_itemSelectionChanged();
}

void BaseWindow::haveBuyObject(const BuyStat &stat,const quint32 &newPrice)
{
    QHash<quint32,quint32> items;
    switch(stat)
    {
        case BuyStat_Done:
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() Can't buy at 0$!";
                return;
            }
            addCash(tempCashForBuy);
            removeCash(newPrice*tempQuantityForBuy);
            items[tempItemForBuy]=tempQuantityForBuy;
            add_to_inventory(items);
        break;
        case BuyStat_HaveNotQuantity:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case BuyStat_PriceHaveChanged:
            addCash(tempCashForBuy);
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
}

void BaseWindow::haveSellObject(const SoldStat &stat,const quint32 &newPrice)
{
    waitToSell=false;
    switch(stat)
    {
        case SoldStat_Done:
            addCash(itemsToSell.first().price*itemsToSell.first().quantity);
            showTip(tr("Item sold"));
        break;
        case SoldStat_BetterPrice:
            if(newPrice==0)
            {
                qDebug() << "haveSellObject() the price 0$ can't be better price!";
                return;
            }
            addCash(newPrice*itemsToSell.first().quantity);
            showTip(tr("Item sold at better price"));
        break;
        case SoldStat_WrongQuantity:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but have not the quantity of this item"));
        break;
        case SoldStat_PriceHaveChanged:
            if(items.contains(itemsToSell.first().object))
                items[itemsToSell.first().object]+=itemsToSell.first().quantity;
            else
                items[itemsToSell.first().object]=itemsToSell.first().quantity;
            load_inventory();
            load_plant_inventory();
            showTip(tr("Sorry but now the price is worse"));
        break;
        default:
            qDebug() << "haveBuyObject(stat) have unknow value";
        break;
    }
    itemsToSell.removeFirst();
}

void BaseWindow::addCash(const quint32 &cash)
{
    this->cash+=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
}

void BaseWindow::removeCash(const quint32 &cash)
{
    this->cash-=cash;
    ui->player_informations_cash->setText(QString("%1$").arg(this->cash));
}
