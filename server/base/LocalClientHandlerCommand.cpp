#include "Client.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "PreparedDBQuery.hpp"
#include "GlobalServerData.hpp"
#include "MapServer.hpp"
#include "StaticText.hpp"

using namespace CatchChallenger;

void Client::sendHandlerCommand(const std::string &command,const std::string &extraText)
{
    if(command==StaticText::text_give)
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back(StaticText::text_1);
        while(arguments.size()>3)
        {
            const std::string arg1=arguments.at(arguments.size()-3);
            arguments.erase(arguments.end()-3);
            const std::string arg2=arguments.at(arguments.size()-2);
            arguments.erase(arguments.end()-2);
            arguments.insert(arguments.end(),arg1+StaticText::text_space+arg2);
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
        if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
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
                    arguments.insert(arguments.end(),arg1+StaticText::text_space+arg2);
                }
                quantity=1;
                //receiveSystemText(std::stringLiteral("quantity is not a number, usage: /give objectId player [quantity=1]"));
                //return;
            }
        }
        if(playerByPseudo.find(arguments.at(1))==playerByPseudo.cend())
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
        Client * const client=playerByPseudo.at(arguments.at(1));
        client->addObjectAndSend(objectId,quantity);
    }
    else if(command==StaticText::text_setevent)
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()!=2)
        {
            receiveSystemText("Wrong arguments number for the command, usage: /give setevent [event] [value]");
            return;
        }
        uint8_t index=0,sub_index;
        while(index<CommonDatapack::commonDatapack.events.size())
        {
            const Event &event=CommonDatapack::commonDatapack.events.at(index);
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
        if(index==CommonDatapack::commonDatapack.events.size())
        {
            receiveSystemText("The event is not found");
            return;
        }
    }
    else if(command==StaticText::text_take)
    {
        bool ok;
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==2)
            arguments.push_back(StaticText::text_1);
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
        if(CommonDatapack::commonDatapack.items.item.find(objectId)==CommonDatapack::commonDatapack.items.item.cend())
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
        if(playerByPseudo.find(arguments.at(1))==playerByPseudo.end())
        {
            receiveSystemText("player is not connected, usage: /take objectId player [quantity=1]");
            return;
        }
        normalOutput(public_and_private_informations.public_informations.pseudo+" have take to "+arguments.at(1)+" the item with id: "+std::to_string(objectId)+" in quantity: "+std::to_string(quantity));
        Client * const client=playerByPseudo.at(arguments.at(1));
        client->sendRemoveObject(
                    objectId,client->
                    removeObject(objectId,quantity));
    }
    else if(command==StaticText::text_tp)
    {
        std::vector<std::string> arguments=stringsplit(extraText,' ');
        vectorRemoveEmpty(arguments);
        if(arguments.size()==3)
        {
            if(arguments.at(1)!=StaticText::text_to)
            {
                receiveSystemText("wrong second arguement: "+arguments.at(1)+", usage: /tp player1 to player2");
                return;
            }
            if(playerByPseudo.find(arguments.front())==playerByPseudo.end())
            {
                receiveSystemText(arguments.front()+" is not connected, usage: /tp player1 to player2");
                return;
            }
            if(playerByPseudo.find(arguments.back())==playerByPseudo.end())
            {
                receiveSystemText(arguments.back()+" is not connected, usage: /tp player1 to player2");
                return;
            }
            Client * const otherPlayerTo=playerByPseudo.at(arguments.back());
            playerByPseudo.at(arguments.front())->receiveTeleportTo(otherPlayerTo->map,otherPlayerTo->x,otherPlayerTo->y,
                             MoveOnTheMap::directionToOrientation(otherPlayerTo->getLastDirection()));
        }
        else
        {
            receiveSystemText("Wrong arguments number for the command, usage: /tp player1 to player2");
            return;
        }
    }
    else if(command==StaticText::text_trade)
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /trade player");
            return;
        }
        if(playerByPseudo.find(extraText)==playerByPseudo.end())
        {
            receiveSystemText(extraText+" is not connected");
            return;
        }
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
        Client * const client=playerByPseudo.at(extraText);
        if(client->getInTrade())
        {
            receiveSystemText(extraText+" is already in trade");
            return;
        }
        if(client->isInBattle())
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
        otherPlayerTrade=client;
        otherPlayerTrade->registerTradeRequest(this);
    }
    else if(command==StaticText::text_battle)
    {
        if(extraText.size()==0)
        {
            receiveSystemText("no player given, syntaxe: /battle player");
            return;
        }
        if(playerByPseudo.find(extraText)==playerByPseudo.end())
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
        Client * const client=playerByPseudo.at(extraText);
        if(client->isInBattle())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(client->getInTrade())
        {
            receiveSystemText(extraText+" is already in battle");
            return;
        }
        if(!otherPlayerIsInRange(client))
        {
            receiveSystemText(extraText+" is not in range");
            return;
        }
        if(!client->getAbleToFight())
        {
            receiveSystemText("The other player can't fight");
            return;
        }
        if(!getAbleToFight())
        {
            receiveSystemText("You can't fight");
            return;
        }
        if(client->isInFight())
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
        client->registerBattleRequest(this);
    }
}
