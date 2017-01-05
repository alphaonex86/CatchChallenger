#include "ActionsAction.h"
#include "../../general/base/CommonSettingsServer.h"

ActionsAction *ActionsAction::actionsAction=NULL;

ActionsAction::ActionsAction()
{
    connect(&moveTimer,&QTimer::timeout,this,&ActionsAction::doMove);
    connect(&textTimer,&QTimer::timeout,this,&ActionsAction::doText);
    textTimer.start(1000);
    flat_map_list=NULL;
    loaded=0;
    ActionsAction::actionsAction=this;
}

ActionsAction::~ActionsAction()
{
}

void ActionsAction::insert_player(CatchChallenger::Api_protocol *api,const CatchChallenger::Player_public_informations &player,const quint32 &mapId,const quint16 &x,const quint16 &y,const CatchChallenger::Direction &direction)
{
    Q_UNUSED(player);
    Q_UNUSED(mapId);
    Q_UNUSED(x);
    Q_UNUSED(y);
    Q_UNUSED(direction);

    ActionsBotInterface::insert_player(api,player,mapId,x,y,direction);
    connect(api,&CatchChallenger::Api_protocol::new_chat_text,      actionsAction,&ActionsAction::new_chat_text,Qt::QueuedConnection);

    if(!moveTimer.isActive())
    {
        const CatchChallenger::Player_private_and_public_informations &player_private_and_public_informations=api->get_player_informations();
        moveTimer.start(player_private_and_public_informations.public_informations.speed);
    }
}

bool ActionsAction::canGoTo(CatchChallenger::Api_protocol *api,const CatchChallenger::Direction &direction,const MapServerMini &map,COORD_TYPE x,COORD_TYPE y)
{
    CatchChallenger::Player_private_and_public_informations &player=api->get_player_informations();
    CatchChallenger::ParsedLayerLedges ledge;
    const std::pair<uint8_t,uint8_t> point(x,y);
    if(map.pointOnMap_Item.find(point)!=map.pointOnMap_Item.cend())
    {
        const MapServerMini::ItemOnMap &item=map.pointOnMap_Item.at(point);
        if(item.visible)
            if(item.infinite || player.itemOnMap.find(item.indexOfItemOnMap)==player.itemOnMap.cend())
                return false;
    }
    ledge=CatchChallenger::MoveOnTheMap::getLedge(map,x,y);
    if(ledge!=CatchChallenger::ParsedLayerLedges_NoLedges)
        switch(direction)
        {
            case CatchChallenger::Direction_move_at_bottom:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesBottom)
                return false;
            break;
            case CatchChallenger::Direction_move_at_top:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesTop)
                return false;
            break;
            case CatchChallenger::Direction_move_at_left:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesLeft)
                return false;
            break;
            case CatchChallenger::Direction_move_at_right:
            if(ledge!=CatchChallenger::ParsedLayerLedges_LedgesRight)
                return false;
            break;
            default:
            break;
        }
    /*if(CatchChallenger::ClientFightEngine::fightEngine.isInFight())
    {
        qDebug() << "Strange, try move when is in fight";
        return false;
    }*/
    const MapServerMini *new_map=&map;
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
            if(x>0)
                x-=1;
            else
            {
                if(map.border.left.map==NULL)
                    return false;
                x=map.border.left.map->width-1;
                y+=map.border.left.y_offset;
                new_map=static_cast<const MapServerMini *>(map.border.left.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_right:
            if(x<(map.width-1))
                x+=1;
            else
            {
                if(map.border.right.map==NULL)
                    return false;
                x=0;
                y+=map.border.right.y_offset;
                new_map=static_cast<const MapServerMini *>(map.border.right.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_top:
            if(y>0)
                y-=1;
            else
            {
                if(map.border.top.map==NULL)
                    return false;
                y=map.border.top.map->height-1;
                x+=map.border.top.x_offset;
                new_map=static_cast<const MapServerMini *>(map.border.top.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_bottom:
            if(y<(map.height-1))
                y+=1;
            else
            {
                if(map.border.bottom.map==NULL)
                    return false;
                y=0;
                x+=map.border.bottom.x_offset;
                new_map=static_cast<const MapServerMini *>(map.border.bottom.map);
            }
            return true;
        break;
        default:
            return false;
    }

    {
        int list_size=new_map->teleporter_list_size;
        int index=0;
        while(index<list_size)
        {
            const CatchChallenger::CommonMap::Teleporter &teleporter=new_map->teleporter[index];
            if(teleporter.source_x==x && teleporter.source_y==y)
            {
                switch(teleporter.condition.type)
                {
                    case CatchChallenger::MapConditionType_None:
                    case CatchChallenger::MapConditionType_Clan://not do for now
                    break;
                    case CatchChallenger::MapConditionType_FightBot:
                        /*if(!haveBeatBot(teleporter.condition.value))
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    case CatchChallenger::MapConditionType_Item:
                        /*if(items==NULL)
                            break;
                        if(items->find(teleporter.condition.value)==items->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    case CatchChallenger::MapConditionType_Quest:
                        /*if(quests==NULL)
                            break;
                        if(quests->find(teleporter.condition.value)==quests->cend())
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }
                        if(!quests->at(teleporter.condition.value).finish_one_time)
                        {
                            if(!map_client.teleport_condition_texts.at(index).isEmpty())
                                emit teleportConditionNotRespected(map_client.teleport_condition_texts.at(index));
                            return false;
                        }*/
                    break;
                    default:
                    break;
                }
            }
            index++;
        }
    }

    /*{
        std::pair<uint8_t,uint8_t> pos(x,y);
        if(map_client.botsFightTrigger.find(pos)!=map_client.botsFightTrigger.cend())
        {
            std::vector<uint32_t> botFightList=map_client.botsFightTrigger.at(pos);
            unsigned int index=0;
            while(index<botFightList.size())
            {
                if(!haveBeatBot(botFightList.at(index)))
                {
                    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_Fight);
                        return false;
                    }
                }
                index++;
            }
        }
    }*/
    /*const CatchChallenger::MonstersCollisionValue &monstersCollisionValue=CatchChallenger::MoveOnTheMap::getZoneCollision(*new_map,x,y);
    if(!monstersCollisionValue.walkOn.empty())
    {
        unsigned int index=0;
        while(index<monstersCollisionValue.walkOn.size())
        {
            const CatchChallenger::MonstersCollision &monstersCollision=CatchChallenger::CommonDatapack::commonDatapack.monstersCollision.at(monstersCollisionValue.walkOn.at(index));
            if(monstersCollision.item==0 || items->find(monstersCollision.item)!=items->cend())
            {
                if(!monstersCollisionValue.walkOnMonsters.at(index).defaultMonsters.empty())
                {
                    if(!CatchChallenger::ClientFightEngine::fightEngine.getAbleToFight())
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneFight);
                        return false;
                    }
                    if(!CatchChallenger::ClientFightEngine::fightEngine.canDoRandomFight(*new_map,x,y))
                    {
                        emit blockedOn(MapVisualiserPlayer::BlockedOn_RandomNumber);
                        return false;
                    }
                }
                return true;
            }
            index++;
        }
        emit blockedOn(MapVisualiserPlayer::BlockedOn_ZoneItem);
        return false;
    }*/
    return true;
}

bool ActionsAction::move(CatchChallenger::Api_protocol *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y)
{
    if(!moveWithoutTeleport(api,direction,map,x,y))
        return false;
    teleport(map,x,y);
    return true;
}

bool ActionsAction::teleport(const MapServerMini **map, COORD_TYPE *x, COORD_TYPE *y)
{
    const CatchChallenger::CommonMap::Teleporter * const teleporter=(*map)->teleporter;
    const uint8_t &teleporter_list_size=(*map)->teleporter_list_size;
    uint8_t index=0;
    while(index<teleporter_list_size)
    {
        if(teleporter[index].source_x==*x && teleporter[index].source_y==*y)
        {
            *x=teleporter[index].destination_x;
            *y=teleporter[index].destination_y;
            *map=static_cast<const MapServerMini *>(teleporter[index].map);
            return true;
        }
        index++;
    }
    return false;
}

bool ActionsAction::moveWithoutTeleport(CatchChallenger::Api_protocol *api,CatchChallenger::Direction direction, const MapServerMini ** map, COORD_TYPE *x, COORD_TYPE *y)
{
    if(*map==NULL)
        return false;
    if(!canGoTo(api,direction,**map,*x,*y))
        return false;
    switch(direction)
    {
        case CatchChallenger::Direction_move_at_left:
            if(*x>0)
                *x-=1;
            else
            {
                *x=(*map)->border.left.map->width-1;
                *y+=(*map)->border.left.y_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.left.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_right:
            if(*x<((*map)->width-1))
                *x+=1;
            else
            {
                *x=0;
                *y+=(*map)->border.right.y_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.right.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_top:
            if(*y>0)
                *y-=1;
            else
            {
                *y=(*map)->border.top.map->height-1;
                *x+=(*map)->border.top.x_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.top.map);
            }
            return true;
        break;
        case CatchChallenger::Direction_move_at_bottom:
            if(*y<((*map)->height-1))
                *y+=1;
            else
            {
                *y=0;
                *x+=(*map)->border.bottom.x_offset;
                *map=static_cast<const MapServerMini *>((*map)->border.bottom.map);
            }
            return true;
        break;
        default:
            return false;
    }
}

void ActionsAction::checkOnTileEvent(Player &player)
{
/*    if(event)
        player.target.localStep.clear();*/
}

void ActionsAction::doMove()
{
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        CatchChallenger::Api_protocol *api=i.key();
        Player &player=clientList[i.key()];
        if(id_map_to_map.find(player.mapId)==id_map_to_map.cend())
            abort();
        const std::string &playerMapStdString=actionsAction->id_map_to_map.at(player.mapId);
        const MapServerMini * playerMap=static_cast<const MapServerMini *>(actionsAction->map_list.at(playerMapStdString));
        //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
        if(api->getCaracterSelected())
        {
            if(!player.target.localStep.empty())
            {
                std::pair<CatchChallenger::Orientation,uint8_t/*step number*/> &step=player.target.localStep[0];
                step.second--;
                //need just continue to walk
                const CatchChallenger::Direction direction=(CatchChallenger::Direction)((uint8_t)step.first+4);
                if(player.direction==direction)
                {
                    if(canGoTo(api,direction,*playerMap,player.x,player.y))
                    {
                        //api->send_player_move(0,player.direction);
                        player.previousStepWalked++;
                        if(step.second==0)
                            player.target.localStep.erase(player.target.localStep.cbegin());
                        move(api,direction,&playerMap,&player.x,&player.y);
                        player.mapId=playerMap->id;
                        checkOnTileEvent(player);
                    }
                    else
                    {
                        //stop
                        player.direction=(CatchChallenger::Direction)((uint8_t)player.direction-4);
                        api->send_player_move(player.previousStepWalked,player.direction);
                        player.previousStepWalked=0;
                        player.target.localStep.clear();
                        player.target.bestPath.clear();
                    }
                }
                //need start to walk or direction change
                else
                {
                    if(canGoTo(api,direction,*playerMap,player.x,player.y))
                    {
                        player.direction=direction;
                        api->send_player_move(player.previousStepWalked,player.direction);
                        player.previousStepWalked=1;
                        move(api,direction,&playerMap,&player.x,&player.y);
                        player.mapId=playerMap->id;
                        checkOnTileEvent(player);
                    }
                    else
                    {
                        //turn on the new direction
                        player.direction=(CatchChallenger::Direction)((uint8_t)direction-4);
                        api->send_player_move(player.previousStepWalked,player.direction);
                        player.previousStepWalked=0;
                        player.target.localStep.clear();
                        player.target.bestPath.clear();
                    }
                }
            }
            else
            {
                //stop the player if is not stopped
                if(player.direction>4)
                {
                    player.direction=(CatchChallenger::Direction)((uint8_t)player.direction-4);
                    api->send_player_move(player.previousStepWalked,player.direction);
                    player.previousStepWalked=0;
                }
            }
        }
    }
}

void ActionsAction::doText()
{
    if(!randomText)
        return;
    if(clientList.isEmpty())
        return;

    QList<CatchChallenger::Api_protocol *> clientListApi;
    QHashIterator<CatchChallenger::Api_protocol *,Player> i(clientList);
    while (i.hasNext()) {
        i.next();
        clientListApi << i.key();
    }
    CatchChallenger::Api_protocol *api=clientListApi.at(rand()%clientListApi.size());
    //DebugClass::debugConsole(QStringLiteral("MainWindow::doStep(), do_step: %1, socket->isValid():%2, map!=NULL: %3").arg(do_step).arg(socket->isValid()).arg(map!=NULL));
    if(api->getCaracterSelected())
    {
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local && rand()%10==0)
        {
            switch(rand()%3)
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"What's up?");
                break;
                case 1:
                    api->sendChatText(CatchChallenger::Chat_type_local,"Have good day!");
                break;
                case 2:
                    api->sendChatText(CatchChallenger::Chat_type_local,"... and now, what I have win :)");
                break;
            }
        }
        else
        {
            if(CommonSettingsServer::commonSettingsServer.chat_allow_all && rand()%100==0)
            {
                switch(rand()%4)
                {
                    case 0:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Hello world! :)");
                    break;
                    case 1:
                        api->sendChatText(CatchChallenger::Chat_type_all,"It's so good game!");
                    break;
                    case 2:
                        api->sendChatText(CatchChallenger::Chat_type_all,"This game have reason to ask donations!");
                    break;
                    case 3:
                        api->sendChatText(CatchChallenger::Chat_type_all,"Donate if you can!");
                    break;
                }
            }
        }
    }
}

void ActionsAction::new_chat_text(const CatchChallenger::Chat_type &chat_type,const QString &text,const QString &pseudo,const CatchChallenger::Player_type &type)
{
    if(!globalChatRandomReply && chat_type!=CatchChallenger::Chat_type_pm)
        return;

    Q_UNUSED(text);
    Q_UNUSED(pseudo);
    Q_UNUSED(type);
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;

    Q_UNUSED(type);
    switch(chat_type)
    {
        case CatchChallenger::Chat_type_all:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_all)
            switch(rand()%(100*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"I'm according "+pseudo);
                break;
                default:
                break;
            }
        break;
        case CatchChallenger::Chat_type_local:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
            switch(rand()%(3*clientList.size()))
            {
                case 0:
                    api->sendChatText(CatchChallenger::Chat_type_local,"You are in right "+pseudo);
                break;
            }
        break;
        case CatchChallenger::Chat_type_pm:
        if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
        {
            if(text==QStringLiteral("version"))
                api->sendPM(QStringLiteral("Version %1 %2").arg(name()).arg(version()),pseudo);
            else
                api->sendPM(QStringLiteral("Hello %1, I'm few bit busy for now").arg(pseudo),pseudo);
        }
        break;
        default:
        break;
    }
}

void ActionsAction::have_inventory(const std::unordered_map<uint16_t,uint32_t> &items, const std::unordered_map<uint16_t, uint32_t> &warehouse_items)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    Player &player=clientList[api];

    player.items=items;
    player.warehouse_items=warehouse_items;
}

void ActionsAction::add_to_inventory(const uint32_t &item,const uint32_t &quantity,const bool &showGain)
{
    QList<QPair<uint16_t,uint32_t> > items;
    items << QPair<uint16_t,uint32_t>(item,quantity);
    add_to_inventory(items,showGain);
}

void ActionsAction::add_to_inventory(const QList<QPair<uint16_t,uint32_t> > &items,const bool &showGain)
{
    int index=0;
    QHash<uint16_t,uint32_t> tempHash;
    while(index<items.size())
    {
        tempHash[items.at(index).first]=items.at(index).second;
        index++;
    }
    add_to_inventory(tempHash,showGain);
}

void ActionsAction::add_to_inventory_slot(const QHash<uint16_t,uint32_t> &items)
{
    add_to_inventory(items);
}

void ActionsAction::add_to_inventory(const QHash<uint16_t,uint32_t> &items,const bool &showGain)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    Player &player=clientList[api];

/*    Player_private_and_public_informations &informations=CatchChallenger::Api_client_real::client->get_player_informations();
    if(items.empty())
        return;
    if(showGain)
    {
        QStringList objects;
        QHashIterator<uint16_t,uint32_t> i(items);
        while (i.hasNext()) {
            i.next();

            const uint16_t &item=i.key();
            if(informations.encyclopedia_item!=NULL)
                informations.encyclopedia_item[item/8]|=(1<<(7-item%8));
            else
                std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
            //add really to the list
            if(this->items.find(item)!=this->items.cend())
                this->items[item]+=i.value();
            else
                this->items[item]=i.value();

            QPixmap image;
            QString name;
            if(DatapackClientLoader::datapackLoader.itemsExtra.contains(i.key()))
            {
                image=DatapackClientLoader::datapackLoader.itemsExtra.value(i.key()).image;
                name=DatapackClientLoader::datapackLoader.itemsExtra.value(i.key()).name;
            }
            else
            {
                image=DatapackClientLoader::datapackLoader.defaultInventoryImage();
                name=QStringLiteral("id: %1").arg(i.key());
            }

            image=image.scaled(24,24);
            QByteArray byteArray;
            QBuffer buffer(&byteArray);
            image.save(&buffer, "PNG");
            if(objects.size()<16)
            {
                if(i.value()>1)
                    objects << QStringLiteral("<b>%2x</b> %3 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(i.value()).arg(name);
                else
                    objects << QStringLiteral("%2 <img src=\"data:image/png;base64,%1\" />").arg(QString(byteArray.toBase64())).arg(name);
            }
        }
        if(objects.size()==16)
            objects << "...";
        add_to_inventoryGainList << objects.join(", ");
        add_to_inventoryGainTime << QTime::currentTime();
        ActionsAction::showGain();
    }
    else
    {
        //add without show
        QHashIterator<uint16_t,uint32_t> i(items);
        while (i.hasNext()) {
            i.next();

            const uint16_t &item=i.key();
            if(informations.encyclopedia_item!=NULL)
                informations.encyclopedia_item[item/8]|=(1<<(7-item%8));
            else
                std::cerr << "encyclopedia_item is null, unable to set" << std::endl;
            //add really to the list
            if(this->items.find(item)!=this->items.cend())
                this->items[item]+=i.value();
            else
                this->items[item]=i.value();
        }
    }

    load_inventory();
    load_plant_inventory();
    on_listCraftingList_itemSelectionChanged();*/
}

void ActionsAction::remove_to_inventory(const uint32_t &itemId,const uint32_t &quantity)
{
    QHash<uint16_t,uint32_t> items;
    items[itemId]=quantity;
    remove_to_inventory(items);
}

void ActionsAction::remove_to_inventory_slot(const QHash<uint16_t,uint32_t> &items)
{
    remove_to_inventory(items);
}

void ActionsAction::remove_to_inventory(const QHash<uint16_t,uint32_t> &items)
{
    CatchChallenger::Api_protocol *api = qobject_cast<CatchChallenger::Api_protocol *>(sender());
    if(api==NULL)
        return;
    Player &player=clientList[api];

    QHashIterator<uint16_t,uint32_t> i(items);
    while (i.hasNext()) {
        i.next();

        //add really to the list
        if(player.items.find(i.key())!=player.items.cend())
        {
            if(player.items.at(i.key())<=i.value())
                player.items.erase(i.key());
            else
                player.items[i.key()]-=i.value();
        }
    }
}


