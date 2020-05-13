#include "Api_protocol.hpp"

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/GeneralType.hpp"

#include <iostream>

using namespace CatchChallenger;

bool Api_protocol::parseCharacterBlockServer(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const int &size)
{
    int pos=0;
    if((size-pos)<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint16_t max_players=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
    pos+=sizeof(uint16_t);
    max_players_real=max_players;
    if(max_players==65534)
    {
        this->max_players=255;
        setMaxPlayers(255);
    }
    else
    {
        setMaxPlayers(max_players);
        this->max_players=max_players;
    }

    uint32_t captureRemainingTime;
    uint8_t captureFrequencyType;
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the city capture remainingTime, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    captureRemainingTime=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the city capture type, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    captureFrequencyType=data[pos];
    pos+=sizeof(uint8_t);
    switch(captureFrequencyType)
    {
        case 0x01:
        case 0x02:
        break;
        default:
        parseError("Procotol wrong or corrupted",std::string("wrong captureFrequencyType, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    cityCapture(captureRemainingTime,captureFrequencyType);

    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    {
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t tempForceClientToSendAtBorder=data[pos];
        pos+=sizeof(uint8_t);
        if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
        {
            parseError("Procotol wrong or corrupted",std::string("forceClientToSendAtBorder have wrong value, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
    }
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.forcedSpeed=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the forcedSpeed, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.useSP=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.autoLearn=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.dontSendPseudo=data[pos];
    pos+=sizeof(uint8_t);


    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_xp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.rates_xp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos))/1000;
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_gold, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.rates_gold=le32toh(*reinterpret_cast<const uint32_t *>(data+pos))/1000;
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_all, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.rates_xp_pow=le32toh(*reinterpret_cast<const uint32_t *>(data+pos))/1000;
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_gold, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.rates_drop=le32toh(*reinterpret_cast<const uint32_t *>(data+pos))/1000;
    pos+=sizeof(uint32_t);

    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_all, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.chat_allow_all=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_local, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.chat_allow_local=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_private, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.chat_allow_private=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.chat_allow_clan=data[pos];
    pos+=sizeof(uint8_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the factoryPriceChange, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.factoryPriceChange=data[pos];
    pos+=sizeof(uint8_t);

    {
        //Main type code
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize=data[pos];
        pos+=sizeof(uint8_t);
        if(textSize>0)
        {
            if((size-pos)<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=std::string(data+pos,textSize);
            mDatapackMain=mDatapackBase+"map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/";
            pos+=textSize;

            if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.empty())
            {
                parseError("Procotol wrong or corrupted",std::string("mainDatapackCode is empty, please put it into the settings"));
                return false;
            }
            if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode,std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
            {
                parseError("Procotol wrong or corrupted",
                           "CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "+
                                       CommonSettingsServer::commonSettingsServer.mainDatapackCode+
                                                              "not contain "+
                            CATCHCHALLENGER_CHECK_MAINDATAPACKCODE
                           );
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.mainDatapackCode.clear();

        //here to be sure of the order
        mDatapackMain=mDatapackBase+"map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/";
    }
    {
        //Sub type code
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize=data[pos];
        pos+=sizeof(uint8_t);
        if(textSize>0)
        {
            if((size-pos)<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            CommonSettingsServer::commonSettingsServer.subDatapackCode=std::string(data+pos,textSize);
            mDatapackSub=mDatapackMain+"sub/"+CommonSettingsServer::commonSettingsServer.subDatapackCode+"/";
            pos+=textSize;

            if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            {
                if(!regex_search(CommonSettingsServer::commonSettingsServer.subDatapackCode,std::regex(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
                {
                    parseError("Procotol wrong or corrupted",std::string("CommonSettingsServer::commonSettingsServer.subDatapackCode not match CATCHCHALLENGER_CHECK_SUBDATAPACKCODE"));
                    return false;
                }
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();

        //here to be sure of the order
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            mDatapackSub=mDatapackMain+"sub/emptyRandomCoder23osaQ9mb5hYh2j/";
        else
            mDatapackSub=mDatapackMain+"sub/"+CommonSettingsServer::commonSettingsServer.subDatapackCode+"/";
    }
    if((size-pos)<28)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the datapack checksum, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
    memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
    pos+=CATCHCHALLENGER_SHA224HASH_SIZE;

    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        if((size-pos)<28)
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the datapack checksum, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
        memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
        pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
    }

    {
        //the mirror
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t mirrorSize=data[pos];
        pos+=sizeof(uint8_t);
        if(mirrorSize>0)
        {
            if((size-pos)<(int)mirrorSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=std::string(data+pos,mirrorSize);
            pos+=mirrorSize;;
            if(!regex_search(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,std::regex("^https?://")))
            {
                parseError("Procotol wrong or corrupted",std::string("mirror with not http(s) protocol with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.clear();
    }
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_gold, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.everyBodyIsRoot=data[pos];
    pos+=sizeof(uint8_t);
    haveDatapackMainSubCode();

    if(!CatchChallenger::CommonDatapack::commonDatapack.isParsedContent())//for bit array
    {
        if(delayedLogin.data.empty())
        {
            delayedLogin.data=std::string(data+pos,size-pos);
            delayedLogin.packetCode=packetCode;
            delayedLogin.queryNumber=queryNumber;
            return true;
        }
        else
        {
            parseError("Procotol wrong or corrupted",
                       "Login in suspend already set with main ident: "+std::to_string(packetCode)+
                       ", and queryNumber: "+std::to_string(queryNumber)+", line: "+
                       std::string(__FILE__)+":"+std::to_string(__LINE__)
                       );
            return false;
        }
    }
    if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.isParsedContent())
    {
        std::cout << "!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.isParsedContent(), don't forget call CommonDatapackServerSpec::parseDatapack(), and when finished Api_protocol::have_main_and_sub_datapack_loaded()" << std::endl;
        if(delayedLogin.data.empty())
        {
            delayedLogin.data=std::string(data+pos,size-pos);
            delayedLogin.packetCode=packetCode;
            delayedLogin.queryNumber=queryNumber;
            return true;
        }
        else
        {
            parseError("Procotol wrong or corrupted",
                       "Login in suspend already set with main ident: "+std::to_string(packetCode)+
                                   ", and queryNumber: "+std::to_string(queryNumber)+", line: "+
                                   std::string(__FILE__)+":"+std::to_string(__LINE__)
                                   );
            return false;
        }
    }

    delayedLogin.data=std::string(data+pos,size-pos);
    parseCharacterBlockCharacter(packetCode,queryNumber,data+pos,size-pos);
    return true;
}

bool Api_protocol::parseCharacterBlockCharacter(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const int &size)
{
    int pos=0;

    //Events not with default value list size
    {
        std::vector<std::pair<uint8_t,uint8_t> > events;
        uint8_t tempListSize;
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        tempListSize=data[pos];
        pos+=sizeof(uint8_t);
        uint8_t event,value;

        //load events
        this->events.clear();
        while((uint32_t)this->events.size()<CatchChallenger::CommonDatapack::commonDatapack.events.size())
            this->events.push_back(0);

        int index=0;
        while(index<tempListSize)
        {
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the event id, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            event=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the event value, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            value=data[pos];
            pos+=sizeof(uint8_t);
            if(event>=CatchChallenger::CommonDatapack::commonDatapack.events.size())
            {
                parseError("Procotol wrong or corrupted",std::string("event index > than max, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            index++;
            events.push_back(std::pair<uint8_t,uint8_t>(event,value));

            this->events[event]=value;
        }
        setEvents(events);
    }

    if(this->max_players<=255)
    {
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t simplifiedId=data[pos];
        pos+=sizeof(uint8_t);
        player_informations.public_informations.simplifiedId=simplifiedId;
    }
    else
    {
        if((size-pos)<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        player_informations.public_informations.simplifiedId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
        pos+=sizeof(uint16_t);
    }
    {
        //pseudo
        if((size-pos)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize=data[pos];
        pos+=sizeof(uint8_t);
        if(textSize>0)
        {
            if((size-pos)<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            player_informations.public_informations.pseudo=std::string(data+pos,textSize);
            pos+=textSize;
        }
        else
            player_informations.public_informations.pseudo.clear();
    }
    if((size-pos)<(int)sizeof(uint8_t))
    {
        newError("Procotol wrong or corrupted",std::string("wrong text with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t tempAllowSize=data[pos];
    pos+=sizeof(uint8_t);
    {
        int index=0;
        while(index<tempAllowSize)
        {
            if((size-pos)<(int)sizeof(uint8_t))
            {
                newError("Procotol wrong or corrupted",std::string("wrong id with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t tempAllow=data[pos];
            pos+=sizeof(uint8_t);
            player_informations.allow.insert(static_cast<ActionAllow>(tempAllow));
            index++;
        }
    }
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    player_informations.clan=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan leader, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t tempClanLeader=data[pos];
    pos+=sizeof(uint8_t);
    if(tempClanLeader==0x01)
        player_informations.clan_leader=true;
    else
        player_informations.clan_leader=false;

    if((size-pos)<(int)sizeof(uint64_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    player_informations.cash=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
    pos+=sizeof(uint64_t);
    if((size-pos)<(int)sizeof(uint64_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    player_informations.warehouse_cash=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
    pos+=sizeof(uint64_t);

    //monsters
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t monster_list_size=data[pos];
    pos+=sizeof(uint8_t);
    uint32_t index=0;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        const int returnVar=dataToPlayerMonster(data+pos,size-pos,monster);
        if(returnVar<0)
            return false;
        pos+=returnVar;
        player_informations.playerMonster.push_back(monster);
        index++;
    }
    //monsters
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    monster_list_size=data[pos];
    pos+=sizeof(uint8_t);
    index=0;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        const int returnVar=dataToPlayerMonster(data+pos,size-pos,monster);
        if(returnVar<0)
            return false;
        pos+=returnVar;
        player_informations.warehouse_playerMonster.push_back(monster);
        index++;
    }
    //reputation
    if((size-pos)<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    PlayerReputation playerReputation;
    uint8_t type;
    index=0;
    uint8_t sub_size8=data[pos];
    pos+=sizeof(uint8_t);
    while(index<sub_size8)
    {
        if((size-pos)<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted","wrong reputation code: "+std::to_string(packetCode)+", and queryNumber: "+std::to_string(queryNumber));
            return false;
        }
        type=data[pos];
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        playerReputation.level=data[pos];
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(int32_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation point, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        playerReputation.point=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
        pos+=sizeof(uint32_t);
        player_informations.reputation[type]=playerReputation;
        index++;
    }
    //compressed block
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint32_t sub_size32=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sub_size32)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint32_t decompressedSize=0;
    if(ProtocolParsingBase::compressionTypeClient==CompressionType::None)
    {
        decompressedSize=sub_size32;
        memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data+pos,sub_size32);
    }
    else
        decompressedSize=computeDecompression(data+pos,ProtocolParsingBase::tempBigBufferForUncompressedInput,
            sub_size32,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);
    {
        const char * const data2=ProtocolParsingBase::tempBigBufferForUncompressedInput;
        int pos2=0;
        int size2=decompressedSize;

        //alloc
        if(player_informations.recipes!=NULL)
        {
            delete player_informations.recipes;
            player_informations.recipes=NULL;
        }
        if(player_informations.encyclopedia_monster!=NULL)
        {
            delete player_informations.encyclopedia_monster;
            player_informations.encyclopedia_monster=NULL;
        }
        if(player_informations.encyclopedia_item!=NULL)
        {
            delete player_informations.encyclopedia_item;
            player_informations.encyclopedia_item=NULL;
        }
        if(CommonDatapack::commonDatapack.craftingRecipesMaxId>1)
        {
            player_informations.recipes=(char *)malloc(CommonDatapack::commonDatapack.craftingRecipesMaxId/8+1);
            memset(player_informations.recipes,0x00,CommonDatapack::commonDatapack.craftingRecipesMaxId/8+1);
        }
        else
        {
            player_informations.recipes=NULL;
            std::cerr << "player_informations.recipes=NULL;" << std::endl;
        }
        if(CommonDatapack::commonDatapack.monstersMaxId>1)
        {
            player_informations.encyclopedia_monster=(char *)malloc(CommonDatapack::commonDatapack.monstersMaxId/8+1);
            memset(player_informations.encyclopedia_monster,0x00,CommonDatapack::commonDatapack.monstersMaxId/8+1);
        }
        else
        {
            player_informations.encyclopedia_monster=NULL;
            std::cerr << "CommonDatapack::commonDatapack.monstersMaxId=NULL;" << std::endl;
        }
        if(CommonDatapack::commonDatapack.items.itemMaxId>1)
        {
            player_informations.encyclopedia_item=(char *)malloc(CommonDatapack::commonDatapack.items.itemMaxId/8+1);
            memset(player_informations.encyclopedia_item,0x00,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
        }
        else
        {
            player_informations.encyclopedia_item=NULL;
            std::cerr << "CommonDatapack::commonDatapack.items.itemMaxId=NULL;" << std::endl;
        }

        //recipes
        {
            if((size2-pos2)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data2+pos2));
            pos2+=sizeof(uint16_t);
            if(sub_size16>0)
            {
                if((size2-pos2)<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.craftingRecipesMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.craftingRecipesMaxId/8+1)
                        memcpy(player_informations.recipes,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,CommonDatapack::commonDatapack.craftingRecipesMaxId/8+1);
                    else
                        memcpy(player_informations.recipes,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,sub_size16);
                }
                pos2+=sub_size16;
            }
        }

        //encyclopedia_monster
        {
            if((size2-pos2)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data2+pos2));
            pos2+=sizeof(uint16_t);
            if(sub_size16>0)
            {
                if((size2-pos2)<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.monstersMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.monstersMaxId/8+1)
                        memcpy(player_informations.encyclopedia_monster,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,CommonDatapack::commonDatapack.monstersMaxId/8+1);
                    else
                        memcpy(player_informations.encyclopedia_monster,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,sub_size16);
                }
                pos2+=sub_size16;
            }
        }

        //encyclopedia_item
        {
            if((size2-pos2)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data2+pos2));
            pos2+=sizeof(uint16_t);
            if(sub_size16>0)
            {
                if((size2-pos2)<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.items.itemMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.items.itemMaxId/8+1)
                        memcpy(player_informations.encyclopedia_item,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
                    else
                        memcpy(player_informations.encyclopedia_item,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,sub_size16);
                }
                pos2+=sub_size16;
            }
        }

        if((size2-pos2)<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        pos2+=sizeof(uint8_t);

        if(size2!=pos2)
        {
            parseError("Procotol wrong or corrupted","wrong size to get the in2.device()->size() "+
                                                                 std::to_string(size2)+" != pos2 "+
                                                                 std::to_string(pos2)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
    }
    pos+=sub_size32;

    //------------------------------------------- End of common part, start of server specific part ----------------------------------

    //quest
    if((size-pos)<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    PlayerQuest playerQuest;
    uint16_t playerQuestId;
    index=0;
    uint16_t sub_size16;
    sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
    pos+=sizeof(uint16_t);
    while(index<sub_size16)
    {
        if((size-pos)<(int)sizeof(int16_t))
        {
            parseError("Procotol wrong or corrupted","wrong text with main ident: "+std::to_string(packetCode)+", and queryNumber: "+std::to_string(queryNumber));
            return false;
        }
        playerQuestId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
        pos+=sizeof(uint16_t);
        if((size-pos)<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        playerQuest.step=data[pos];
        pos+=sizeof(uint8_t);
        if((size-pos)<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation point, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        playerQuest.finish_one_time=data[pos];
        pos+=sizeof(uint8_t);
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            parseError("Procotol wrong or corrupted",std::string("can't be to step 0 if have never finish the quest, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        player_informations.quests[playerQuestId]=playerQuest;
        index++;
    }

    //compressed block
    if((size-pos)<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    sub_size32=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
    pos+=sizeof(uint32_t);
    if((size-pos)<(int)sub_size32)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    decompressedSize=0;
    if(ProtocolParsingBase::compressionTypeClient==CompressionType::None)
    {
        decompressedSize=sub_size32;
        memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data+pos,sub_size32);
    }
    else
        decompressedSize=computeDecompression(data+pos,ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,
            sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);
    {
        const char * const data2=ProtocolParsingBase::tempBigBufferForUncompressedInput;
        int pos2=0;
        int size2=decompressedSize;

        //alloc
        if(player_informations.bot_already_beaten!=NULL)
        {
            delete player_informations.bot_already_beaten;
            player_informations.bot_already_beaten=NULL;
        }
        player_informations.bot_already_beaten=(char *)malloc(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        memset(player_informations.bot_already_beaten,0x00,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId<=0)
             std::cerr << "CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId == 0, take care" << std::endl;

        //bot
        {
            if((size2-pos2)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16=le16toh(*reinterpret_cast<const uint16_t *>(data2+pos2));
            pos2+=sizeof(uint16_t);
            if(sub_size16>0)
            {
                if((size2-pos2)<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(sub_size16>CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1)
                    memcpy(player_informations.bot_already_beaten,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
                else
                    memcpy(player_informations.bot_already_beaten,ProtocolParsingBase::tempBigBufferForUncompressedInput+pos2,sub_size16);
                pos2+=sub_size16;
            }
        }

        if(size2!=pos2)
        {
            parseError("Procotol wrong or corrupted","wrong size to get the in2.device()->size() "+
                       std::to_string(size2)+" != pos2 "+
                       std::to_string(pos2)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
    }
    pos+=sub_size32;

    //item on map
    {
        if((size-pos)<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint16_t itemOnMapSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
        pos+=sizeof(uint16_t);
        uint16_t index=0;
        while(index<itemOnMapSize)
        {
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t itemOnMap=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            player_informations.itemOnMap.insert(itemOnMap);
            index++;
        }
    }

    //plant on map
    {
        if((size-pos)<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint16_t plantOnMapSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
        pos+=sizeof(uint16_t);
        uint16_t index=0;
        while(index<plantOnMapSize)
        {
            PlayerPlant playerPlant;

            //plant on map,x,y getted
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t plantOnMap=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);

            //plant
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            playerPlant.plant=data[pos];
            pos+=sizeof(uint8_t);

            //seconds to mature
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t seconds_to_mature=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            playerPlant.mature_at=std::time(nullptr)/1000+seconds_to_mature;

            player_informations.plantOnMap[plantOnMap]=playerPlant;
            index++;
        }
    }

    character_selected=true;
    haveCharacter();
    return true;
}
