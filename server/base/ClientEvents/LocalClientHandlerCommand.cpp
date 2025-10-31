#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../ClientList.hpp"

using namespace CatchChallenger;

void Client::sendHandlerCommand(const std::string &command,const std::string &extraText)
{
    if(command=="give")
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back("1");
        while(arguments.size()>3)
        {
            const std::string arg1=arguments.at(arguments.size()-3);
            arguments.erase(arguments.end()-3);
            const std::string arg2=arguments.at(arguments.size()-2);
            arguments.erase(arguments.end()-2);
            arguments.insert(arguments.end(),arg1+" "+arg2);
        }
        if(arguments.size()!=3)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /give objectId player [quantity=1]");
            return;
        }
        const uint16_t &objectId=stringtouint16(arguments.at(0),&ok);
        if(!ok)
        {
            receiveSystemText("objectId is not a number, usage: /give objectId player [quantity=1]");
            return;
        }
        if(CommonDatapack::commonDatapack.get_items().item.find(objectId)==CommonDatapack::commonDatapack.get_items().item.cend())
        {
            receiveSystemText("objectId is not a valid item, usage: /give objectId player [quantity=1]");
            return;
        }
        uint32_t quantity=1;
        if(arguments.size()==3)
        {
            quantity=stringtouint32(arguments.back(),&ok);
            if(!ok)
            {
                while(arguments.size()>2)
                {
                    const std::string arg1=arguments.at(arguments.size()-2);
                    arguments.erase(arguments.end()-2);
                    const std::string arg2=arguments.at(arguments.size()-1);
                    arguments.erase(arguments.end()-1);
                    arguments.insert(arguments.end(),arg1+" "+arg2);
                }
                quantity=1;
                //receiveSystemText(std::stringLiteral("quantity is not a number, usage: /give objectId player [quantity=1]"));
                //return;
            }
        }
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnected=ClientList::list->global_clients_list_bypseudo(arguments.at(1));
        if(indexConnected==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            receiveSystemText("player is not connected, usage: /give objectId player [quantity=1]");
            return;
        }
        normalOutput(public_and_private_informations.public_informations.pseudo+
                     " have give to "+
                     arguments.at(1)+
                     " the item with id: "+
                     std::to_string(objectId)+
                     " in quantity: "+
                     std::to_string(quantity));
        if(ClientList::list->empty(indexConnected))
        {
            Client &client=ClientList::list->rw(indexConnected);
            client.addObjectAndSend(objectId,quantity);
        }
        else
            receiveSystemText("player is not connected/bug, usage: /give objectId player [quantity=1]");
    }
    else if(command=="setevent")
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()!=2)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /give setevent [event] [value]");
            return;
        }
        uint8_t index=0,sub_index;
        while(index<CommonDatapack::commonDatapack.get_events().size())
        {
            const Event &event=CommonDatapack::commonDatapack.get_events().at(index);
            if(event.name==arguments.at(0))
            {
                sub_index=0;
                while(sub_index<event.values.size())
                {
                    if(event.values.at(sub_index)==arguments.at(1))
                    {
                        if(GlobalServerData::serverPrivateVariables.events.at(index)==sub_index)
                        {
                            receiveSystemText("The event have aready this value");
                            return;
                        }
                        else
                        {
                            setEvent(index,sub_index);
                            GlobalServerData::serverPrivateVariables.events[index]=sub_index;
                        }
                        break;
                    }
                    sub_index++;
                }
                if(sub_index==event.values.size())
                {
                    receiveSystemText("The event value is not found");
                    return;
                }
                break;
            }
            index++;
        }
        if(index==CommonDatapack::commonDatapack.get_events().size())
        {
            receiveSystemText("The event is not found");
            return;
        }
    }
    else if(command=="take")
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back("1");
        if(arguments.size()!=3)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /take objectId player [quantity=1]");
            return;
        }
        const uint16_t objectId=stringtouint16(arguments.front(),&ok);
        if(!ok)
        {
            receiveSystemText("objectId is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        if(CommonDatapack::commonDatapack.get_items().item.find(objectId)==CommonDatapack::commonDatapack.get_items().item.cend())
        {
            receiveSystemText("objectId is not a valid item, usage: /take objectId player [quantity=1]");
            return;
        }
        const uint32_t quantity=stringtouint32(arguments.back(),&ok);
        if(!ok)
        {
            receiveSystemText("quantity is not a number, usage: /take objectId player [quantity=1]");
            return;
        }
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnected=ClientList::list->global_clients_list_bypseudo(arguments.at(1));
        if(indexConnected==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            receiveSystemText("player is not connected, usage: /take objectId player [quantity=1]");
            return;
        }
        normalOutput(public_and_private_informations.public_informations.pseudo+" have take to "+arguments.at(1)+" the item with id: "+std::to_string(objectId)+" in quantity: "+std::to_string(quantity));

        if(!ClientList::list->empty(indexConnected))
        {
            receiveSystemText("player is not connected, usage: /take objectId player [quantity=1]");
            return;
        }

        Client &client=ClientList::list->rw(indexConnected);

        client.sendRemoveObject(objectId,client.removeObject(objectId,quantity));
    }
    else if(command=="tp")
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!="to")
            {
                receiveSystemText("wrong second arguement: "+arguments.at(1)+", usage: /tp player1 to player2");
                return;
            }
            const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnectedFront=ClientList::list->global_clients_list_bypseudo(arguments.front());
            if(indexConnectedFront==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
            {
                receiveSystemText(arguments.front()+" is not connected, usage: /tp player1 to player2");
                return;
            }
            const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnectedBack=ClientList::list->global_clients_list_bypseudo(arguments.back());
            if(indexConnectedBack==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
            {
                receiveSystemText(arguments.back()+" is not connected, usage: /tp player1 to player2");
                return;
            }

            if(!ClientList::list->empty(indexConnectedFront))
            {
                receiveSystemText("player front is not connected or bug");
                return;
            }
            if(!ClientList::list->empty(indexConnectedBack))
            {
                receiveSystemText("player back is not connected or bug");
                return;
            }

            Client &clientFront=ClientList::list->rw(indexConnectedFront);
            Client &clientBack=ClientList::list->rw(indexConnectedBack);

            clientFront.receiveTeleportTo(clientBack.mapIndex,clientBack.x,clientBack.y,
                             MoveOnTheMap::directionToOrientation(clientBack.getLastDirection()));
        }
        else
        {
            receiveSystemText("Wrong arguments number for the command, usage: /tp player1 to player2");
            return;
        }
    }
    else if(command=="goto")
    {
        /* PUT MAP RESOLUTION INTO CLIENT
         * std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!="goto")
            {
                receiveSystemText("wrong second arguement: "+arguments.at(1)+", usage: /goto map x y");
                return;
            }
            if(Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.find(arguments.front())==Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.end())
            {
                receiveSystemText(arguments.front()+" map not found, usage: /goto map x y");
                return;
            }
            bool ok=false;
            const uint8_t tempX=stringtouint8(arguments.at(1),&ok);
            if(!ok)
            {
                receiveSystemText(arguments.front()+" x not number, usage: /goto map x y");
                return;
            }
            const uint8_t tempY=stringtouint8(arguments.at(2),&ok);
            if(!ok)
            {
                receiveSystemText(arguments.front()+" y not number, usage: /goto map x y");
                return;
            }
            CommonMap * tempMap=Map_server_MapVisibility_Simple_StoreOnSender::flat_map_list.at(arguments.front());
            if(tempX>=tempMap->width)
            {
                receiveSystemText(arguments.front()+" x too big, usage: /goto map x y");
                return;
            }
            if(tempY>=tempMap->height)
            {
                receiveSystemText(arguments.front()+" y too big, usage: /goto map x y");
                return;
            }
            teleportTo(tempMap,tempX,tempY,Orientation::Orientation_bottom);
        }
        else
        {
            receiveSystemText("Wrong arguments number for the command, usage: /goto map x y");
            return;
        }*/
        {
            receiveSystemText("Disabled for remake");
            return;
        }
    }
    else if(command=="trade")
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /trade player");
            return;
        }
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnected=ClientList::list->global_clients_list_bypseudo(extraText);
        if(indexConnected==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText("You can't trade with yourself");
            return;
        }
        if(getInTrade())
        {
            receiveSystemText("You are already in trade");
            return;
        }
        if(isInBattle())
        {
            receiveSystemText("You are already in battle");
            return;
        }

        if(!ClientList::list->empty(indexConnected))
        {
            receiveSystemText("player is not connected/bug");
            return;
        }
        Client &client=ClientList::list->rw(indexConnected);
        if(client.getInTrade())
        {
            receiveSystemText(extraText+" is already in trade");
            return;
        }
        if(client.isInBattle())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(!otherPlayerIsInRange(client))
        {
            receiveSystemText(extraText+" is not in range");
            return;
        }
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Trade requested");
        #endif
        otherPlayerTrade=client.getIndexConnect();
        client.registerTradeRequest(*this);
    }
    else if(command=="battle")
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /battle player");
            return;
        }
        const SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED indexConnected=ClientList::list->global_clients_list_bypseudo(extraText);
        if(indexConnected==SIMPLIFIED_PLAYER_INDEX_FOR_CONNECTED_MAX)
        {
            receiveSystemText(extraText+" is not connected");
            return;
        }
        if(public_and_private_informations.public_informations.pseudo==extraText)
        {
            receiveSystemText("You can't battle with yourself");
            return;
        }
        if(isInBattle())
        {
            receiveSystemText("you are already in battle");
            return;
        }
        if(getInTrade())
        {
            receiveSystemText("you are already in trade");
            return;
        }

        if(!ClientList::list->empty(indexConnected))
        {
            receiveSystemText("player is not connected/bug");
            return;
        }
        Client &client=ClientList::list->rw(indexConnected);

        if(client.isInBattle())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(client.getInTrade())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(!otherPlayerIsInRange(client))
        {
            receiveSystemText(extraText+" is not in range");
            return;
        }
        if(!client.getAbleToFight())
        {
            receiveSystemText("The other player can't fight");
            return;
        }
        if(!getAbleToFight())
        {
            receiveSystemText("You can't fight");
            return;
        }
        if(client.isInFight())
        {
            receiveSystemText("The other player is in fight");
            return;
        }
        if(isInFight())
        {
            receiveSystemText("You are in fight");
            return;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
        if(captureCityInProgress())
        {
            errorOutput("Try battle when is in capture city");
            return;
        }
        #endif
        #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
        normalOutput("Battle requested");
        #endif
        client.registerBattleRequest(*this);
    }
}
