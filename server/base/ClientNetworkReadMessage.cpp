#include "Client.h"
#include "GlobalServerData.h"
#include "MapServer.h"
#include "../../general/base/ProtocolParsingCheck.h"
#include "../../general/base/CommonSettingsCommon.h"
#ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
#include "BaseServerLogin.h"
#endif

using namespace CatchChallenger;

bool Client::parseMessage(const uint8_t &packetCode,const char * const data,const int unsigned &size)
{
    if(stopIt)
        return false;
    if(stat==ClientStat::None)
    {
        disconnectClient();
        return false;
    }
    if(stat!=ClientStat::CharacterSelected)
    {
        //wrong protocol
        disconnectClient();
        //parseError(std::stringLiteral("is not logged, parsenormalOutput(%1)").arg(packetCode));
        return false;
    }
    #ifdef CATCHCHALLENGER_DDOS_FILTER
    switch(packetCode)
    {
        case 0x02:
            if((movePacketKickTotalCache+movePacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitMove)
            {
                errorOutput("Too many move in sort time, check DDOS limit: ("+std::to_string(movePacketKickTotalCache)+"+"+std::to_string(movePacketKickNewValue)+")>="+std::to_string(GlobalServerData::serverSettings.ddos.kickLimitMove));
                return false;
            }
            movePacketKickNewValue++;
        break;
        case 0x03:
            if((chatPacketKickTotalCache+chatPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitChat)
            {
                errorOutput("Too many chat in sort time, check DDOS limit: ("+std::to_string(chatPacketKickTotalCache)+"+"+std::to_string(chatPacketKickNewValue)+")>="+std::to_string(GlobalServerData::serverSettings.ddos.kickLimitChat));
                return false;
            }
            chatPacketKickNewValue++;
        break;
        default:
            if((otherPacketKickTotalCache+otherPacketKickNewValue)>=GlobalServerData::serverSettings.ddos.kickLimitOther)
            {
                errorOutput("Too many packet in sort time, check DDOS limit: ("+std::to_string(otherPacketKickTotalCache)+"+"+std::to_string(otherPacketKickNewValue)+")>="+std::to_string(GlobalServerData::serverSettings.ddos.kickLimitOther));
                return false;
            }
            otherPacketKickNewValue++;
        break;
    }
    #endif
    //do the work here
    #ifdef DEBUG_MESSAGE_CLIENT_RAW_NETWORK
    normalOutput("parsenormalOutput("+std::to_string(packetCode)+","+binarytoHexa(data,size)+")");
    #endif
    switch(packetCode)
    {
        case 0x02:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)*2)
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            #endif
            const uint8_t &direction=*(data+sizeof(uint8_t));
            if(direction<1 || direction>8)
            {
                errorOutput("Bad direction number: "+std::to_string(direction));
                return false;
            }
            moveThePlayer(static_cast<uint8_t>(*data),static_cast<Direction>(direction));
            return true;
        }
        break;
        //Chat
        case 0x03:
        {
            uint32_t pos=0;
            if(size<((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for chat");
                return false;
            }
            const uint8_t &chatType=data[pos];
            pos+=1;
            if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_pm)
            {
                errorOutput("chat type error: "+std::to_string(chatType));
                return false;
            }
            if(chatType==Chat_type_pm)
            {
                if(CommonSettingsServer::commonSettingsServer.chat_allow_private)
                {
                    std::string text;
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            errorOutput("wrong utf8 to std::string size in PM for text size");
                            return false;
                        }
                        const uint8_t &textSize=data[pos];
                        pos+=1;
                        if(textSize>0)
                        {
                            if((size-pos)<textSize)
                            {
                                errorOutput("wrong utf8 to std::string size in PM for text");
                                return false;
                            }
                            text=std::string(data+pos,textSize);
                            pos+=textSize;
                        }
                    }
                    std::string pseudo;
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            errorOutput("wrong utf8 to std::string size in PM for pseudo");
                            return false;
                        }
                        const uint8_t &pseudoSize=data[pos];
                        pos+=1;
                        if(pseudoSize>0)
                        {
                            if((size-pos)<pseudoSize)
                            {
                                errorOutput("wrong utf8 to std::string size in PM for pseudo");
                                return false;
                            }
                            pseudo=std::string(data+pos,pseudoSize);
                            pos+=pseudoSize;
                        }
                    }

                    normalOutput(Client::text_slashpmspace+pseudo+Client::text_space+text);
                    sendPM(text,pseudo);
                }
                else
                {
                    errorOutput("can't send pm because is disabled: "+std::to_string(chatType));
                    return false;
                }
            }
            else
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    errorOutput("wrong utf8 to std::string header size");
                    return false;
                }
                std::string text;
                const uint8_t &textSize=data[pos];
                pos+=1;
                if(textSize>0)
                {
                    if((size-pos)<textSize)
                    {
                        errorOutput("wrong utf8 to std::string size");
                        return false;
                    }
                    text=std::string(data+pos,textSize);
                    pos+=textSize;
                }

                if(!stringStartWith(text,'/'))
                {
                    if(chatType==Chat_type_local)
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_local)
                            sendLocalChatText(text);
                        else
                        {
                            errorOutput("can't send chat local because is disabled: "+std::to_string(chatType));
                            return false;
                        }
                    }
                    else
                    {
                        if(CommonSettingsServer::commonSettingsServer.chat_allow_clan || CommonSettingsServer::commonSettingsServer.chat_allow_all)
                            sendChatText((Chat_type)chatType,text);
                        else
                        {
                            errorOutput("can't send chat other because is disabled: "+std::to_string(chatType));
                            return false;
                        }
                    }
                }
                else
                {
                    std::string command;
                    /// \warning don't use regex here, slow and do DDOS risk
                    std::size_t found=text.find(Client::text_space);
                    if(found!=std::string::npos)
                    {
                        command=text.substr(1,found-1);
                        text=text.substr(found+1,text.size()-found-1);
                    }
                    else
                        command=text.substr(1,text.size()-1);

                    //the normal player command
                    {
                        if(command==Client::text_playernumber)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                        else if(command==Client::text_playerlist)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+" "+text);
                            return true;
                        }
                        else if(command==Client::text_trade)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                        else if(command==Client::text_battle)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                            return true;
                        }
                    }
                    //the admin command
                    if(public_and_private_informations.public_informations.type==Player_type_gm || public_and_private_informations.public_informations.type==Player_type_dev || GlobalServerData::serverSettings.everyBodyIsRoot)
                    {
                        if(command==Client::text_give)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_setevent)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_take)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_tp)
                        {
                            sendHandlerCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_kick)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_chat)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_setrights)
                        {
                            sendBroadCastCommand(command,text);
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else if(command==Client::text_stop || command==Client::text_restart)
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVER
                            BroadCastWithoutSender::broadCastWithoutSender.emit_serverCommand(command,text);
                            #endif
                            normalOutput(Client::text_send_command_slash+command+Client::text_space+text);
                        }
                        else
                        {
                            normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                            receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                        }
                    }
                    else
                    {
                        normalOutput(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                        receiveSystemText(Client::text_unknown_send_command_slash+command+Client::text_space+text);
                    }
                }
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            return true;
        }
        break;
        //Clan invite accept
        case 0x04:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for clan invite");
                return false;
            }
            #endif
            const uint8_t &returnCode=*(data+sizeof(uint8_t));
            switch(returnCode)
            {
                case 0x01:
                    clanInvite(true);
                break;
                case 0x02:
                    clanInvite(false);
                break;
                default:
                    errorOutput("wrong return code for clan invite ident: "+std::to_string(packetCode));
                return false;
            }
            return true;
        }
        break;
        case 0x05:
        {
        }
        break;
        case 0x06:
        {
        }
        break;
        //Try escape
        case 0x07:
            tryEscape();
        break;
        case 0x08:
        {
        }
        break;
        //Learn skill
        case 0x09:
        {
            if(size!=((int)sizeof(uint8_t)+sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for learn skill");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            const uint16_t &skill=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint8_t))));
            return learnSkill(monsterPosition,skill);
        }
        break;
        case 0x0A:
        {
        }
        break;
        //Heal all the monster
        case 0x0B:
            heal();
        break;
        //Request bot fight
        case 0x0C:
        {
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for request bot fight");
                return false;
            }
            const uint16_t &fightId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            requestFight(fightId);
            return true;
        }
        break;
        //move the monster
        case 0x0D:
        {
            if(size!=((int)sizeof(uint8_t)*2))
            {
                errorOutput("wrong remaining size for move monster");
                return false;
            }
            const uint8_t &moveWay=*data;
            bool moveUp;
            switch(moveWay)
            {
                case 0x01:
                    moveUp=true;
                break;
                case 0x02:
                    moveUp=false;
                break;
                default:
                    errorOutput("wrong move up value");
                return false;
            }
            const uint8_t &position=*(data+sizeof(uint8_t));
            moveMonster(moveUp,position);
            return true;
        }
        break;
        //change monster in fight, monster id in db
        case 0x0E:
        {
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for monster position in fight");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            return changeOfMonsterInFight(monsterPosition);
        }
        break;
        /// \todo check double validation
        //Monster evolution validated
        case 0x0F:
        {
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for monster evolution validated");
                return false;
            }
            const uint8_t &monsterPosition=data[0x00];
            confirmEvolution(monsterPosition);
            return true;
        }
        break;
        //Use object on monster
        case 0x10:
        {
            if(size<((int)sizeof(uint16_t)+(int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for use object on monster");
                return false;
            }
            const uint16_t &item=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint8_t &monsterPosition=data[sizeof(uint16_t)];
            return useObjectOnMonsterByPosition(item,monsterPosition);
        }
        break;
        //use skill
        case 0x11:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            #endif
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("Wrong size in move packet");
                return false;
            }
            useSkill(le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data))));
            return true;
        }
        break;
        case 0x12:
        {
        }
        break;
        //Destroy an object
        case 0x13:
        {
            if(size!=((int)sizeof(uint16_t)+sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for destroy item id");
                return false;
            }
            const uint16_t &itemId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            destroyObject(itemId,quantity);
            return true;
        }
        break;
        //Put object into a trade
        case 0x14:
        {
            uint32_t pos=0;
            if(size<((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for trade add type");
                return false;
            }
            const uint8_t &type=data[pos];
            pos+=1;
            switch(type)
            {
                //cash
                case 0x01:
                {
                    if((size-pos)<((int)sizeof(uint64_t)))
                    {
                        errorOutput("wrong remaining size for trade add cash");
                        return false;
                    }
                    uint64_t tempVar;
                    memcpy(&tempVar,data+pos,sizeof(uint64_t));
                    const uint64_t &cash=le64toh(tempVar);
                    pos+=sizeof(uint64_t);
                    tradeAddTradeCash(cash);
                }
                break;
                //item
                case 0x02:
                {
                    if((size-pos)<((int)sizeof(uint16_t)))
                    {
                        errorOutput("wrong remaining size for trade add item id");
                        return false;
                    }
                    const uint16_t &item=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        errorOutput("wrong remaining size for trade add item quantity");
                        return false;
                    }
                    const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint32_t);
                    tradeAddTradeObject(item,quantity);
                }
                break;
                //monster
                case 0x03:
                {
                    if((size-pos)<((int)sizeof(uint8_t)))
                    {
                        errorOutput("wrong remaining size for trade add monster");
                        return false;
                    }
                    const uint8_t &monsterPosition=data[pos];
                    pos+=sizeof(uint8_t);
                    tradeAddTradeMonster(monsterPosition);
                }
                break;
                default:
                    errorOutput("wrong type for trade add");
                    return false;
                break;
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parsenormalOutput("+
                                      std::to_string(packetCode)+
                                      "): "+
                                      binarytoHexa(data,pos)+
                                      " "+
                                      binarytoHexa(data+pos,size-pos)
                                      );
                return false;
            }
            return true;
        }
        break;
        //trade finished after the accept
        case 0x15:
            tradeFinished();
        break;
        //trade canceled after the accept
        case 0x16:
            tradeCanceled();
        break;
        //deposite/withdraw to the warehouse
        case 0x17:
        {
            uint32_t pos=0;
            std::vector<std::pair<uint16_t, int32_t> > items;
            std::vector<uint32_t> withdrawMonsters;
            std::vector<uint32_t> depositeMonsters;
            if((size-pos)<((int)sizeof(int64_t)))
            {
                errorOutput("wrong remaining size for warehouse cash");
                return false;
            }
            uint64_t tempVar;
            memcpy(&tempVar,data+pos,sizeof(uint64_t));
            const uint64_t &cash=le64toh(tempVar);
            pos+=sizeof(uint64_t);

            uint16_t size16;
            if((size-pos)<((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for warehouse item list");
                return false;
            }
            size16=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint16_t);

            {
                uint16_t id;
                uint32_t index=0;
                while(index<size16)
                {
                    if((size-pos)<((int)sizeof(uint16_t)))
                    {
                        errorOutput("wrong remaining size for warehouse item id");
                        return false;
                    }
                    id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        errorOutput("wrong remaining size for warehouse item quantity");
                        return false;
                    }
                    const int32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=sizeof(int32_t);
                    items.push_back(std::pair<uint16_t, int32_t>(id,quantity));
                    index++;
                }
            }
            if((size-pos)<((int)sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for warehouse monster list");
                return false;
            }
            uint32_t size;
            size=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            uint32_t index=0;
            while(index<size)
            {
                if((size-pos)<((int)sizeof(uint32_t)))
                {
                    errorOutput("wrong remaining size for warehouse monster id");
                    return false;
                }
                const uint32_t &id=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                withdrawMonsters.push_back(id);
                index++;
            }
            if((size-pos)<((int)sizeof(uint32_t)))
            {
                errorOutput("wrong remaining size for warehouse sub monster id");
                return false;
            }
            size=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<size)
            {
                if((size-pos)<((int)sizeof(uint32_t)))
                {
                    errorOutput("wrong remaining size for warehouse monster deposite");
                    return false;
                }
                const uint32_t &id=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                depositeMonsters.push_back(id);
                index++;
            }
            wareHouseStore(cash,items,withdrawMonsters,depositeMonsters);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parsenormalOutput("+
                            std::to_string(packetCode)+
                            "): "+
                            binarytoHexa(data,pos)+
                            " "+
                            binarytoHexa(data+pos,size-pos)
                            );
                return false;
            }
            return true;
        }
        break;
        case 0x18:
            takeAnObjectOnMap();
        break;
        #ifdef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
        //Use seed into dirt
        case 0x19:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint8_t &plant_id=*data;
            plantSeed(plant_id);
            return true;
        }
        break;
        //Collect mature plant
        case 0x1A:
            collectPlant();
        break;
        #else
        //continous switch to improve the performance
        case 0x19:
        case 0x1A:
            errorOutput("unknown main ident: "+std::to_string(packetCode)+" CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER is not def");
            return false;
        break;
        #endif
        //Quest start
        case 0x1B:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest start");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Start,questId);
            return true;
        }
        break;
        //Quest finish
        case 0x1C:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest finish");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Finish,questId);
            return true;
        }
        break;
        //Quest cancel
        case 0x1D:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest cancel");
                return false;
            }
            #endif
            const uint16_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_Cancel,questId);
            return true;
        }
        break;
        //Quest next step
        case 0x1E:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint16_t)))
            {
                errorOutput("wrong remaining size for quest next step");
                return false;
            }
            #endif
            const uint32_t &questId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            newQuestAction(QuestAction_NextStep,questId);
            return true;
        }
        break;
        #ifndef EPOLLCATCHCHALLENGERSERVER
        //Waiting for city capture
        case 0x1F:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=((int)sizeof(uint8_t)))
            {
                errorOutput("wrong remaining size for city capture");
                return false;
            }
            #endif
            const uint8_t &cancel=*data;
            if(cancel==0x00)
                waitingForCityCaputre(false);
            else
                waitingForCityCaputre(true);
            return true;
        }
        break;
        #endif
        default:
            errorOutput("unknown main ident: "+std::to_string(packetCode));
            return false;
        break;
    }
    return true;
}
