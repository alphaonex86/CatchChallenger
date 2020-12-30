#include "Api_protocol.hpp"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/GeneralType.hpp"

#include <iostream>

#ifdef BENCHMARKMUTIPLECLIENT
#include <fstream>
#include <unistd.h>
#endif

//have message without reply
bool Api_protocol::parseMessage(const uint8_t &packetCode, const char * const data, const unsigned int &size)
{
    int pos=0;
    if(!is_logged)
    {
        if(packetCode==0x40 || packetCode==0x44 || packetCode==0x78 || packetCode==0x75)
        {}
        else
        {
            parseError("Procotol wrong or corrupted","is not logged with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
    }
    switch(packetCode)
    {
        //Insert player on map, need be delayed if number_of_map==0
        case 0x6B:
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t mapListSize=data[pos];
            pos+=sizeof(uint8_t);
            int index=0;
            while(index<mapListSize)
            {
                uint32_t mapId;
                if(number_of_map==0)
                {
                    parseError("Internal error","number_of_map==0 with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                else if(number_of_map<=255)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId=data[pos];
                    pos+=sizeof(uint8_t);
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(unsigned int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    mapId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                }

                uint16_t playerSizeList;
                if(max_players<=255)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t numberOfPlayer=data[pos];
                    pos+=sizeof(uint8_t);
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    Player_public_informations public_informations;
                    if(max_players<=255)
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t playerSmallId=data[pos];
                        pos+=sizeof(uint8_t);
                        public_informations.simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((size-pos)<(unsigned int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        public_informations.simplifiedId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                    }

                    //x and y
                    if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t x=data[pos];
                    pos+=sizeof(uint8_t);
                    uint8_t y=data[pos];
                    pos+=sizeof(uint8_t);

                    //direction and player type
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t directionAndPlayerType=data[pos];
                    pos+=sizeof(uint8_t);
                    uint8_t directionInt,playerTypeInt;
                    directionInt=directionAndPlayerType & 0x0F;
                    playerTypeInt=directionAndPlayerType & 0xF0;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError("Procotol wrong or corrupted","direction have wrong value: "+
                                   std::to_string(directionInt)+", at main ident: "+
                                   std::to_string(packetCode)+", directionAndPlayerType: "+
                                   std::to_string(directionAndPlayerType)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                                   ", data: "+binarytoHexa(data,size));
                        return false;
                    }
                    Direction direction=(Direction)directionInt;
                    Player_type playerType=(Player_type)playerTypeInt;
                    if(playerType!=Player_type_normal && playerType!=Player_type_premium && playerType!=Player_type_gm && playerType!=Player_type_dev)
                    {
                        parseError("Procotol wrong or corrupted","direction have wrong value: "+
                                   std::to_string(playerType)+", at main ident: "+
                                   std::to_string(packetCode)+", directionAndPlayerType: "+
                                   std::to_string(directionAndPlayerType)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                                   ", data: "+binarytoHexa(data,size));
                        return false;
                    }
                    public_informations.type=playerType;

                    //the speed
                    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        public_informations.speed=data[pos];
                        pos+=sizeof(uint8_t);
                    }
                    else
                        public_informations.speed=CommonSettingsServer::commonSettingsServer.forcedSpeed;

                    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                    {
                        //the pseudo
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t pseudoSize=data[pos];
                        pos+=sizeof(uint8_t);
                        if(pseudoSize>0)
                        {
                            if((size-pos)<(unsigned int)pseudoSize)
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            public_informations.pseudo=std::string(data+pos,pseudoSize);
                            pos+=pseudoSize;
                            if(public_informations.pseudo.empty())
                            {
                                parseError("Procotol wrong or corrupted","UTF8 decoding failed for pseudo: "+public_informations.pseudo+
                                           ", rawData: "+binarytoHexa(data+pos,pseudoSize)+", line: "+
                                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                                           );
                                return false;
                            }
                        }
                    }
                    else
                        public_informations.pseudo.clear();

                    //the skin
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t skinId=data[pos];
                    pos+=sizeof(uint8_t);
                    public_informations.skinId=skinId;

                    //the following monster id to show
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t monsterId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    public_informations.monsterId=monsterId;

                    if(public_informations.simplifiedId==player_informations.public_informations.simplifiedId)
                    {
                        if(last_direction_is_set==false)//to work with reemit
                        {
                            setLastDirection(direction);
                            player_informations.public_informations=public_informations;
                            have_current_player_info(player_informations);
                            insert_player(public_informations,mapId,x,y,direction);
                        }
                    }
                    else
                        insert_player(public_informations,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
        }
        break;
        //Move player on map, used too for the first insert of the current player
        case 0x68:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            //move the player
            uint8_t directionInt,moveListSize;
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer=data[pos];
                pos+=sizeof(uint8_t);
                playerSizeList=numberOfPlayer;
                uint8_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerId=data[pos];
                    pos+=sizeof(uint8_t);
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    std::vector<std::pair<uint8_t,Direction> > movement;
                    std::pair<uint8_t,Direction> new_movement;
                    moveListSize=data[pos];
                    pos+=sizeof(uint8_t);
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError("Procotol wrong or corrupted","move size == 0 with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        new_movement.first=data[pos];
                        pos+=sizeof(uint8_t);
                        directionInt=data[pos];
                        pos+=sizeof(uint8_t);
                        new_movement.second=(Direction)directionInt;
                        movement.push_back(new_movement);
                        index_sub_loop++;
                    }
                    move_player(playerId,movement);
                    index++;
                }
            }
            else
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                uint16_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    std::vector<std::pair<uint8_t,Direction> > movement;
                    std::pair<uint8_t,Direction> new_movement;
                    moveListSize=data[pos];
                    pos+=sizeof(uint8_t);
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError("Procotol wrong or corrupted","move size == 0 with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        new_movement.first=data[pos];
                        pos+=sizeof(uint8_t);
                        directionInt=data[pos];
                        pos+=sizeof(uint8_t);
                        new_movement.second=(Direction)directionInt;
                        movement.push_back(new_movement);
                        index_sub_loop++;
                    }
                    move_player(playerId,movement);
                    index++;
                }
            }
        }
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //Remove player from map
        case 0x69:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            //remove player
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer=data[pos];
                pos+=sizeof(uint8_t);
                playerSizeList=numberOfPlayer;
                uint8_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerId=data[pos];
                    pos+=sizeof(uint8_t);
                    remove_player(playerId);
                    index++;
                }
            }
            else
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                uint16_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    remove_player(playerId);
                    index++;
                }
            }
        }
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //Player number
        case 0x64:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(!character_selected)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if(max_players<=255)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t current_player_connected_8Bits=data[pos];
                pos+=sizeof(uint8_t);
                this->last_players_number=current_player_connected_8Bits;
                this->last_max_players=max_players;
                number_of_player(current_player_connected_8Bits,max_players);
            }
            else
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                               ", in.device()->pos(): "+std::to_string(pos)+
                               ", in.device()->size(): "+std::to_string(size));
                    return false;
                }
                uint16_t current_player_connected_16Bits=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                this->last_players_number=current_player_connected_16Bits;
                this->last_max_players=max_players;
                number_of_player(current_player_connected_16Bits,max_players);
            }
        }
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //drop all player on the map
        case 0x65:
        #ifndef BENCHMARKMUTIPLECLIENT
            dropAllPlayerOnTheMap();
            delayedMessages.clear();
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //Reinser player on same map
        case 0x66:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer=data[pos];
                pos+=sizeof(uint8_t);
                playerSizeList=numberOfPlayer;
            }
            else
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
            }
            int index_sub_loop=0;
            while(index_sub_loop<playerSizeList)
            {
                //player id
                uint16_t simplifiedId;
                if(max_players<=255)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t playerSmallId=data[pos];
                    pos+=sizeof(uint8_t);
                    simplifiedId=playerSmallId;
                }
                else
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    simplifiedId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                }

                //x and y
                if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t x=data[pos];
                pos+=sizeof(uint8_t);
                uint8_t y=data[pos];
                pos+=sizeof(uint8_t);

                //direction and player type
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t directionInt=data[pos];
                pos+=sizeof(uint8_t);
                if(directionInt<1 || directionInt>8)
                {
                    parseError("Procotol wrong or corrupted","direction have wrong value: "+std::to_string(directionInt)+
                               ", at main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                Direction direction=(Direction)directionInt;

                reinsert_player(simplifiedId,x,y,direction);
                index_sub_loop++;
            }
        }
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //Reinser player on other map
        case 0x67:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=std::string(data,size);
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t mapListSize=data[pos];
            pos+=sizeof(uint8_t);
            int index=0;
            while(index<mapListSize)
            {
                uint32_t mapId;
                if(number_of_map==0)
                {
                    parseError("Internal error","number_of_map==0 with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                else if(number_of_map<=255)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId=data[pos];
                    pos+=sizeof(uint8_t);
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(unsigned int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    mapId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                }
                uint16_t playerSizeList;
                if(max_players<=255)
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t numberOfPlayer=data[pos];
                    pos+=sizeof(uint8_t);
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    playerSizeList=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    uint16_t simplifiedId;
                    if(max_players<=255)
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t playerSmallId=data[pos];
                        pos+=sizeof(uint8_t);
                        simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((size-pos)<(unsigned int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        simplifiedId=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                    }

                    //x and y
                    if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t x=data[pos];
                    pos+=sizeof(uint8_t);
                    uint8_t y=data[pos];
                    pos+=sizeof(uint8_t);

                    //direction and player type
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t directionInt=data[pos];
                    pos+=sizeof(uint8_t);
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError("Procotol wrong or corrupted","direction have wrong value: %1"+std::to_string(directionInt)+", at main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    Direction direction=(Direction)directionInt;

                    full_reinsert_player(simplifiedId,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }

        }
        #else
        parseError("Procotol wrong or corrupted","packet not allow in benchmark main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
        #endif
        break;
        //chat as input
        case 0x5F:
        {
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t chat_type_int=data[pos];
            pos+=sizeof(uint8_t);
            if(chat_type_int<1 || chat_type_int>8)
            {
                parseError("Procotol wrong or corrupted","wrong chat type with main ident: "+std::to_string(packetCode)+", chat_type_int: "+std::to_string(chat_type_int)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            Chat_type chat_type=(Chat_type)chat_type_int;
            if((size-pos)<(unsigned int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong text with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string text;
            uint16_t textSize;
            if(chat_type_int>=1 && chat_type_int<=6)
            {
                uint8_t textSize8=data[pos];
                pos+=sizeof(uint8_t);
                textSize=textSize8;
            }
            else
            {
                textSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
            }
            if(textSize>0)
            {
                if((size-pos)<(unsigned int)textSize)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", pseudoSize: "+std::to_string(textSize)+
                               ", data: "+binarytoHexa(data+pos,size)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                text=std::string(data+pos,textSize);
                pos+=textSize;
            }
            if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
            {
                add_system_text(chat_type,text);
                new_system_text(chat_type,text);
            }
            else
            {
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong text with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string pseudo;
                uint8_t pseudoSize=data[pos];
                pos+=sizeof(uint8_t);
                if(pseudoSize>0)
                {
                    if((size-pos)<(unsigned int)pseudoSize)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", pseudoSize: "+std::to_string(pseudoSize)+
                                   ", data: "+binarytoHexa(data+pos,size)+
                                   ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                   );
                        return false;
                    }
                    pseudo=std::string(data+pos,pseudoSize);
                    pos+=pseudoSize;
                    if(pseudo.empty())
                    {
                        parseError("Procotol wrong or corrupted","UTF8 decoding failed: packetCode: "+std::to_string(packetCode)+
                                   ", rawText: "+binarytoHexa(data+pos,pseudoSize)+
                                   ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                }
                uint8_t player_type_int;
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                player_type_int=data[pos];
                pos+=sizeof(uint8_t);
                Player_type player_type=(Player_type)player_type_int;
                if(player_type!=Player_type_normal && player_type!=Player_type_premium && player_type!=Player_type_gm && player_type!=Player_type_dev)
                {
                    parseError("Procotol wrong or corrupted","wrong player type with main ident: "+std::to_string(packetCode)+
                               ", player_type_int: "+std::to_string(player_type_int)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
                    return false;
                }
                add_chat_text(chat_type,text,pseudo,player_type);
                new_chat_text(chat_type,text,pseudo,player_type);
            }
        }
        break;

        //file as input
        case 0x76://raw
        case 0x77://compressed
        {
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            uint8_t fileListSize=data[pos];
            pos+=sizeof(uint8_t);
            if(fileListSize==0)
            {
                parseError("Procotol wrong or corrupted","fileListSize==0 with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }

            uint32_t sub_size32=size-pos;
            uint32_t decompressedSize=0;
            if(ProtocolParsingBase::compressionTypeClient==CompressionType::None || packetCode==0x76)
            {
                decompressedSize=sub_size32;
                memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data+pos,sub_size32);
            }
            else
                decompressedSize=computeDecompression(data+pos,ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,
                    sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);

            const char * const data2=ProtocolParsingBase::tempBigBufferForUncompressedInput;
            int pos2=0;
            int size2=decompressedSize;

            //std::cout << QString(QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->size())).toHex()).toStdString() << " " << std::string("%1:%2").arg(__FILE__).arg(__LINE__).toStdString() << std::endl;

            int index=0;
            while(index<fileListSize)
            {
                if((size2-pos2)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string fileName;
                uint8_t fileNameSize=data2[pos2];
                pos2+=sizeof(uint8_t);
                if(fileNameSize>0)
                {
                    if((size2-pos2)<(int)fileNameSize)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    fileName=std::string(data2+pos2,fileNameSize);
                    pos2+=fileNameSize;
                }
                std::string ext;
                std::string::size_type n=fileName.rfind(".");
                if(n != std::string::npos)
                    ext=fileName.substr(n+1);
                else
                    std::cerr << "fileName.rfind(\".\")==std::string::npos" << std::endl;
                if(extensionAllowed.find(ext)==extensionAllowed.cend())
                {
                    if(extensionAllowed.empty())
                        std::cerr << "extensionAllowed is empty" << std::endl;
                    else
                        std::cerr << "extensionAllowed: \"" << stringimplode(std::vector<std::string>(extensionAllowed.cbegin(),extensionAllowed.cend()),";") << "\"" << std::endl;
                    parseError("Procotol wrong or corrupted","extension not allowed: \""+fileName+
                               "\", ext: \""+ext+"\" with main ident: "+std::to_string(packetCode)+", line: "+
                               std::string(__FILE__)+":"+std::to_string(__LINE__)+
                               ", data: "+binarytoHexa(data,size2)
                               );
                    if(!tolerantMode)
                        return false;
                }
                if((size2-pos2)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint32_t filesize=le32toh(*reinterpret_cast<const uint32_t *>(data2+pos2));
                pos2+=sizeof(uint32_t);
                if((size2-pos2)<(int)filesize)
                {
                    parseError("Procotol wrong or corrupted","wrong file data size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string dataFile;
                dataFile=std::string(data2+pos2,filesize);
                pos2+=filesize;
                if(packetCode==0x76)
                    std::cout << "Raw file to create: " << std::endl;
                else
                    std::cout << "Compressed file to create: " << std::endl;
                switch(datapackStatus)
                {
                    case DatapackStatus::Base:
                        std::cout << " on base:" << std::endl;
                        newFileBase(fileName,dataFile);
                    break;
                    case DatapackStatus::Main:
                        std::cout << " on main:" << std::endl;
                        newFileMain(fileName,dataFile);
                    break;
                    case DatapackStatus::Sub:
                        std::cout << " on sub:" << std::endl;
                        newFileSub(fileName,dataFile);
                    break;
                    default:
                        std::cout << " on ??? error, drop:" << std::endl;
                        std::cerr << "Create file on ??? error, drop it" << std::endl;
                    break;
                }
                std::cout << fileName << std::endl;
                switch(datapackStatus)
                {
                    case DatapackStatus::Base:
                    break;
                    case DatapackStatus::Main:
                    break;
                    case DatapackStatus::Sub:
                    break;
                    default:
                         std::cerr << "Create file on ??? error, drop it" << std::endl;
                    break;
                }

                index++;
            }
            return true;//no remaining data, because all remaing is used as file data
        }
        break;
        //kicked/ban and reason
        case 0x60:
        {
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t code=data[pos];
            pos+=sizeof(uint8_t);
            std::string text;
            {
                if((size-pos)<(unsigned int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t textSize=data[pos];
                pos+=sizeof(uint8_t);
                if(textSize>0)
                {
                    if((size-pos)<(unsigned int)textSize)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    text=std::string(data+pos,textSize);
                    pos+=textSize;
                }
            }
            switch(code)
            {
                case 0x01:
                    parseError("Disconnected by the server","You have been kicked by the server with the reason: "+std::string(text)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
                case 0x02:
                    parseError("Disconnected by the server","You have been ban by the server with the reason: "+std::string(text)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
                case 0x03:
                    parseError("Disconnected by the server","You have been disconnected by the server with the reason: "+std::string(text)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
                default:
                    parseError("Disconnected by the server","You have been disconnected by strange way by the server with the reason: "+std::string(text)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
        }
        break;
        //clan dissolved
        case 0x61:
            clanDissolved();
        break;
        //clan info
        case 0x62:
        {
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string name;
            uint8_t stringSize=data[pos];
            pos+=sizeof(uint8_t);
            if(stringSize>0)
            {
                if((size-pos)<(unsigned int)stringSize)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                name=std::string(data+pos,stringSize);
                pos+=stringSize;
            }
            clanInformations(name);
        }
        break;
        //clan invite
        case 0x63:
        {
            if((size-pos)<(unsigned int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t clanId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string name;
            uint8_t stringSize=data[pos];
            pos+=sizeof(uint8_t);
            if(stringSize>0)
            {
                if((size-pos)<(unsigned int)stringSize)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                name=std::string(data+pos,size);
                pos+=stringSize;
            }
            clanInvite(clanId,name);
        }
        break;
        //Send datapack send size
        case 0x75:
        {
            if((size-pos)<(unsigned int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t datapckFileNumber=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            if((size-pos)<(unsigned int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t datapckFileSize=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            datapackSizeBase(datapckFileNumber,datapckFileSize);
        }
        break;
        //Internal server list for the current pool
        case 0x40:
        {
            haveTheServerList=true;

            if(pos!=0)
                abort();
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t serverMode=data[pos];
            pos+=sizeof(uint8_t);
            if(serverMode<1 || serverMode>2)
            {
                parseError("Procotol wrong or corrupted","unknown serverMode "+std::to_string(serverMode)+" main code: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            proxyMode=Api_protocol::ProxyMode(serverMode);
            uint8_t serverListSize=0;
            uint8_t serverListIndex;
            std::vector<ServerFromPoolForDisplayTemp> serverTempList;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            serverListSize=data[pos];
            pos+=sizeof(uint8_t);
            serverListIndex=0;
            while(serverListIndex<serverListSize)
            {
                ServerFromPoolForDisplayTemp server;
                server.currentPlayer=0;
                server.maxPlayer=0;
                server.port=0;
                server.uniqueKey=0;
                //group index
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    server.charactersGroupIndex=data[pos];
                    pos+=sizeof(uint8_t);
                }
                //uniquekey
                {
                    if((size-pos)<(unsigned int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    server.uniqueKey=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                }
                if(proxyMode==Api_protocol::ProxyMode::Reconnect)
                {
                    //host
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t stringSize=data[pos];
                        pos+=sizeof(uint8_t);
                        if(stringSize>0)
                        {
                            if((size-pos)<(unsigned int)stringSize)
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            server.host=std::string(data+pos,stringSize);
                            pos+=stringSize;
                        }
                    }
                    //port
                    {
                        if((size-pos)<(unsigned int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        server.port=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                    }
                }
                //xml (name, description, ...)
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint16_t stringSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    if(stringSize>0)
                    {
                        if((size-pos)<(unsigned int)stringSize)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        server.xml=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                }
                //logical to contruct the tree
                {
                    if((size-pos)<(unsigned int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    server.logicalGroupIndex=data[pos];
                    pos+=sizeof(uint8_t);
                }
                //max player
                {
                    if((size-pos)<(unsigned int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    server.maxPlayer=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                }
                serverTempList.push_back(server);
                serverListIndex++;
            }
            serverListIndex=0;
            while(serverListIndex<serverListSize)
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                serverTempList[serverListIndex].currentPlayer=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                serverListIndex++;
            }
            selectedServerIndex=-1;
            serverOrdenedList.clear();
            characterListForSelection.clear();
            std::string language;
            language=getLanguage();
            serverListIndex=0;
            while(serverListIndex<serverListSize)
            {
                const ServerFromPoolForDisplayTemp &server=serverTempList.at(serverListIndex);
                ServerFromPoolForDisplay * const tempPoint=addLogicalServer(server,language);
                if(tempPoint!=NULL)
                {
                    tempPoint->serverOrdenedListIndex=static_cast<uint8_t>(serverOrdenedList.size());
                    serverOrdenedList.push_back(*tempPoint);
                }
                serverListIndex++;
            }
        }
        break;
        //Logical group
        case 0x44:
        {
            logicialGroup.logicialGroupList.clear();
            logicialGroup.name.clear();
            logicialGroup.servers.clear();

            uint8_t logicalGroupSize=0;
            uint8_t logicalGroupIndex=0;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            logicialGroupIndexList.clear();
            logicalGroupSize=data[pos];
            pos+=sizeof(uint8_t);
            if(logicalGroupSize>0)
            {
                std::string language=getLanguage();
                while(logicalGroupIndex<logicalGroupSize)
                {
                    std::string path;
                    std::string xml;
                    {
                        if((size-pos)<(unsigned int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t pathSize=data[pos];
                        pos+=sizeof(uint8_t);
                        if(pathSize>0)
                        {
                            if((size-pos)<(unsigned int)pathSize)
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            path=std::string(data+pos,pathSize);
                            pos+=pathSize;
                        }
                    }
                    {
                        if((size-pos)<(unsigned int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint16_t xmlSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                        if(xmlSize>0)
                        {
                            if((size-pos)<(unsigned int)xmlSize)
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            xml=std::string(data+pos,xmlSize);
                            pos+=xmlSize;
                        }
                    }

                    logicialGroupIndexList.push_back(addLogicalGroup(path,xml,language));
                    logicalGroupIndex++;
                }
            }
            haveTheLogicalGroupList=true;
        }
        break;
        //Logical group
        case 0x46:
        {
        }
        break;

        //Send the inventory
        case 0x54:
        {
            if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.isParsedContent())
            {
                DelayedMessage delayedMessage;
                delayedMessage.packetCode=packetCode;
                delayedMessage.data=std::string(data,size);
                delayedMessages.push_back(delayedMessage);
                return true;
            }
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            inventorySize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                id=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(unsigned int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if(items.find(id)!=items.cend())
                    items[id]+=quantity;
                else
                    items[id]=quantity;
                index++;
            }
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> warehouse_items;
            inventorySize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                id=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(unsigned int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if(warehouse_items.find(id)!=warehouse_items.cend())
                    warehouse_items[id]+=quantity;
                else
                    warehouse_items[id]=quantity;
                index++;
            }
            #ifdef MAXIMIZEPERFORMANCEOVERDATABASESIZE
            player_informations.items=items;
            player_informations.warehouse_items=warehouse_items;
            have_inventory(player_informations.items,player_informations.warehouse_items);
            #else
            for (auto itr = items.cbegin(); itr != items.cend(); ++itr)
                player_informations.items[itr->first]=itr->second;
            for (auto itr = warehouse_items.cbegin(); itr != warehouse_items.cend(); ++itr)
                player_informations.warehouse_items[itr->first]=itr->second;
            have_inventory(items,warehouse_items);
            #endif
        }
        break;
        //Add object
        case 0x55:
        {
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            inventorySize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                id=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(unsigned int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if(items.find(id)!=items.cend())
                    items[id]+=quantity;
                else
                    items[id]=quantity;
                index++;
            }
            add_to_inventory(items);
        }
        break;
        //Remove object
        case 0x56:
        {
            if((size-pos)<(unsigned int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            inventorySize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(unsigned int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                id=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(unsigned int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if(items.find(id)!=items.cend())
                    items[id]+=quantity;
                else
                    items[id]=quantity;
                index++;
            }
            remove_to_inventory(items);
        }
        break;
        //the other player have put object
        case 0x57:
        {
            if(!isInTrade)
            {
                parseError("Procotol wrong or corrupted","not in trade main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t type=data[pos];
            pos+=sizeof(uint8_t);
            switch(type)
            {
                //cash
                case 0x01:
                {
                    if((size-pos)<((int)sizeof(uint64_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add cash");
                        return false;
                    }
                    uint64_t cash=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
                    pos+=sizeof(uint64_t);
                    tradeAddTradeCash(cash);
                }
                break;
                //item
                case 0x02:
                {
                    if((size-pos)<((int)sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add item id");
                        return false;
                    }
                    uint16_t item=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<((int)sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add item quantity");
                        return false;
                    }
                    uint32_t quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                    tradeAddTradeObject(item,quantity);
                }
                break;
                //monster
                case 0x03:
                {
                    PlayerMonster monster;
                    if(!dataToPlayerMonster(data+pos,size-pos,monster))
                        return false;
                    tradeAddTradeMonster(monster);
                }
                break;
                default:
                    parseError("Procotol wrong or corrupted","wrong type for trade add");
                    return false;
                break;
            }
        }
        break;
        //the other player have accepted
        case 0x58:
        {
            if(!tradeRequestId.empty())
            {
                parseError("Internal error","request is running, skip this trade exchange");
                return false;
            }
            if(isInTrade)
            {
                parseError("Procotol wrong or corrupted","already in trade main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::string pseudo;
            uint8_t pseudoSize=data[pos];
            pos+=sizeof(uint8_t);
            if(pseudoSize>0)
            {
                if((size-pos)<(unsigned int)pseudoSize)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                pseudo=std::string(data+pos,pseudoSize);
                pos+=pseudoSize;
                if(pseudo.empty())
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
            }
            uint8_t skinId;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            skinId=data[pos];
            pos+=sizeof(uint8_t);
            isInTrade=true;
            tradeAcceptedByOther(pseudo,skinId);
        }
        break;
        //the other player have canceled
        case 0x59:
        {
            isInTrade=false;
            tradeCanceledByOther();
            if(!tradeRequestId.empty())
            {
                tradeCanceled();
                return false;
            }
        }
        break;
        //the other player have finished
        case 0x5A:
        {
            if(!isInTrade)
            {
                parseError("Procotol wrong or corrupted","not in trade with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            tradeFinishedByOther();
        }
        break;
        //the server have validated the transaction
        case 0x5B:
        {
            if(!isInTrade)
            {
                parseError("Procotol wrong or corrupted","not in trade with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            isInTrade=false;
            tradeValidatedByTheServer();
        }
        break;

        //Result of the turn
        case 0x50:
        {
            std::vector<Skill::AttackReturn> attackReturn;
            uint8_t attackReturnListSize;
            uint8_t listSizeShort,tempuint;
            int index,indexAttackReturn;
            PublicPlayerMonster publicPlayerMonster;
            uint8_t genderInt;
            uint8_t buffListSize;

            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            attackReturnListSize=data[pos];
            pos+=sizeof(uint8_t);
            indexAttackReturn=0;
            while(indexAttackReturn<attackReturnListSize)
            {
                Skill::AttackReturn tempAttackReturn;
                tempAttackReturn.success=false;
                tempAttackReturn.attack=0;
                tempAttackReturn.monsterPlace=0;
                tempAttackReturn.publicPlayerMonster.catched_with=0;
                tempAttackReturn.publicPlayerMonster.gender=Gender_Unknown;
                tempAttackReturn.publicPlayerMonster.hp=0;
                tempAttackReturn.publicPlayerMonster.level=0;
                tempAttackReturn.publicPlayerMonster.monster=0;
                tempAttackReturn.on_current_monster=false;
                tempAttackReturn.item=0;
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                tempuint=data[pos];
                pos+=sizeof(uint8_t);
                if(tempuint>1)
                {
                    parseError("Procotol wrong or corrupted","code to bool with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                tempAttackReturn.doByTheCurrentMonster=(tempuint!=0);
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                tempuint=data[pos];
                pos+=sizeof(uint8_t);
                switch(tempuint)
                {
                    case Skill::AttackReturnCase_NormalAttack:
                    {
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempuint=data[pos];
                        pos+=sizeof(uint8_t);
                        if(tempuint>1)
                        {
                            parseError("Procotol wrong or corrupted","code to bool with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempAttackReturn.success=(tempuint!=0);
                        if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempAttackReturn.attack=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                        //add buff
                        index=0;
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        listSizeShort=data[pos];
                        pos+=sizeof(uint8_t);
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.buff=data[pos];
                            pos+=sizeof(uint8_t);
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            tempuint=data[pos];
                            pos+=sizeof(uint8_t);
                            switch(tempuint)
                            {
                                case ApplyOn_AloneEnemy:
                                case ApplyOn_AllEnemy:
                                case ApplyOn_Themself:
                                case ApplyOn_AllAlly:
                                case ApplyOn_Nobody:
                                break;
                                default:
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.on=(ApplyOn)tempuint;
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.level=data[pos];
                            pos+=sizeof(uint8_t);
                            tempAttackReturn.addBuffEffectMonster.push_back(buffEffect);
                            index++;
                        }
                        //remove buff
                        index=0;
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        listSizeShort=data[pos];
                        pos+=sizeof(uint8_t);
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.buff=data[pos];
                            pos+=sizeof(uint8_t);
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            tempuint=data[pos];
                            pos+=sizeof(uint8_t);
                            switch(tempuint)
                            {
                                case ApplyOn_AloneEnemy:
                                case ApplyOn_AllEnemy:
                                case ApplyOn_Themself:
                                case ApplyOn_AllAlly:
                                case ApplyOn_Nobody:
                                break;
                                default:
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.on=(ApplyOn)tempuint;
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buffEffect.level=data[pos];
                            pos+=sizeof(uint8_t);
                            tempAttackReturn.removeBuffEffectMonster.push_back(buffEffect);
                            index++;
                        }
                        //life effect
                        index=0;
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        listSizeShort=data[pos];
                        pos+=sizeof(uint8_t);
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((size-pos)<(unsigned int)(sizeof(uint32_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            lifeEffectReturn.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            tempuint=data[pos];
                            pos+=sizeof(uint8_t);
                            switch(tempuint)
                            {
                                case ApplyOn_AloneEnemy:
                                case ApplyOn_AllEnemy:
                                case ApplyOn_Themself:
                                case ApplyOn_AllAlly:
                                case ApplyOn_Nobody:
                                break;
                                default:
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            lifeEffectReturn.on=(ApplyOn)tempuint;
                            tempAttackReturn.lifeEffectMonster.push_back(lifeEffectReturn);
                            index++;
                        }
                        //buff effect
                        index=0;
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        listSizeShort=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((size-pos)<(unsigned int)(sizeof(uint32_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            lifeEffectReturn.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            tempuint=data[pos];
                            pos+=sizeof(uint8_t);
                            switch(tempuint)
                            {
                                case ApplyOn_AloneEnemy:
                                case ApplyOn_AllEnemy:
                                case ApplyOn_Themself:
                                case ApplyOn_AllAlly:
                                case ApplyOn_Nobody:
                                break;
                                default:
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            lifeEffectReturn.on=(ApplyOn)tempuint;
                            tempAttackReturn.buffLifeEffectMonster.push_back(lifeEffectReturn);
                            index++;
                        }
                    }
                    break;
                    case Skill::AttackReturnCase_MonsterChange:
                    {
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempAttackReturn.monsterPlace=data[pos];
                        pos+=sizeof(uint8_t);
                        if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        publicPlayerMonster.monster=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        publicPlayerMonster.level=data[pos];
                        pos+=sizeof(uint8_t);
                        if((size-pos)<(unsigned int)(sizeof(uint32_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        publicPlayerMonster.hp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
                        if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        publicPlayerMonster.catched_with=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                        pos+=sizeof(uint16_t);
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        genderInt=data[pos];
                        pos+=sizeof(uint8_t);
                        switch(genderInt)
                        {
                            case 0x01:
                            case 0x02:
                            case 0x03:
                                publicPlayerMonster.gender=(Gender)genderInt;
                            break;
                            default:
                                parseError("Procotol wrong or corrupted","gender code wrong: "+std::to_string(genderInt)+
                                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                           );
                                return false;
                            break;
                        }
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        buffListSize=data[pos];
                        pos+=sizeof(uint8_t);
                        index=0;
                        while(index<buffListSize)
                        {
                            PlayerBuff buff;
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buff.buff=data[pos];
                            pos+=sizeof(uint8_t);
                            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            buff.level=data[pos];
                            pos+=sizeof(uint8_t);
                            publicPlayerMonster.buffs.push_back(buff);
                            index++;
                        }
                        tempAttackReturn.publicPlayerMonster=publicPlayerMonster;
                    }
                    break;
                    case Skill::AttackReturnCase_ItemUsage:
                    {
                        if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempuint=data[pos];
                        pos+=sizeof(uint8_t);
                        tempAttackReturn.on_current_monster=(tempuint!=0x00);
                        if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempAttackReturn.item=data[pos];
                        pos+=sizeof(uint8_t);
                    }
                    break;
                    default:
                    parseError("Procotol wrong or corrupted","Skill::AttackReturnCase wrong with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                attackReturn.push_back(tempAttackReturn);
                indexAttackReturn++;
            }

            sendBattleReturn(attackReturn);
        }
        break;
        //The other player have declined you battle request
        case 0x51:
            battleCanceledByOther();
        break;
        //The other player have accepted you battle request
        case 0x52:
        {
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string pseudo;
            uint8_t pseudoSize=data[pos];
            pos+=sizeof(uint8_t);
            if(pseudoSize>0)
            {
                if((size-pos)<(unsigned int)pseudoSize)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                pseudo=std::string(data+pos,pseudoSize);
                pos+=pseudoSize;
                if(pseudo.empty())
                {
                    parseError("Procotol wrong or corrupted","UTF8 decoding failed with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
            }
            uint8_t skinId;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            skinId=data[pos];
            pos+=sizeof(uint8_t);
            PublicPlayerMonster publicPlayerMonster;
            std::vector<uint8_t> stat;
            uint8_t genderInt;
            uint8_t buffListSize;
            uint8_t monsterPlace;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t statListSize=data[pos];
            pos+=sizeof(uint8_t);
            int index=0;
            while(index<statListSize)
            {
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t statEntry=data[pos];
                pos+=sizeof(uint8_t);
                stat.push_back(statEntry);
                index++;
            }
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            monsterPlace=data[pos];
            pos+=sizeof(uint8_t);
            if(monsterPlace<=0 || monsterPlace>stat.size())
            {
                parseError("Procotol wrong or corrupted","monster place wrong range with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if((size-pos)<(unsigned int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            publicPlayerMonster.monster=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            publicPlayerMonster.level=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(unsigned int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            publicPlayerMonster.hp=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            if((size-pos)<(unsigned int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            publicPlayerMonster.catched_with=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            genderInt=data[pos];
            pos+=sizeof(uint8_t);
            switch(genderInt)
            {
                case 0x01:
                case 0x02:
                case 0x03:
                    publicPlayerMonster.gender=(Gender)genderInt;
                break;
                default:
                    parseError("Procotol wrong or corrupted","gender code wrong: "+std::to_string(genderInt)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                break;
            }
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            buffListSize=data[pos];
            pos+=sizeof(uint8_t);
            index=0;
            while(index<buffListSize)
            {
                PlayerBuff buff;
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                buff.buff=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(unsigned int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                buff.level=data[pos];
                pos+=sizeof(uint8_t);
                publicPlayerMonster.buffs.push_back(buff);
                index++;
            }
            battleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
        }
        break;
        //random seeds as input
        case 0x53:
        {
            if(size<128)
            {
                parseError("Procotol wrong or corrupted","have too less data for random seed with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            random_seeds(std::string(data,size));
            return true;//quit here because all data is always used
        }
        break;

        //Result of the turn
        case 0x5E:
        {
            uint8_t returnCode;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                    captureCityYourAreNotLeader();
                break;
                case 0x02:
                {
                    std::string city;
                    uint8_t stringSize=data[pos];
                    pos+=sizeof(uint8_t);
                    if(stringSize>0)
                    {
                        if((size-pos)<(unsigned int)stringSize)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        city=std::string(data+pos,stringSize);
                        pos+=stringSize;
                    }
                    captureCityYourLeaderHaveStartInOtherCity(city);
                }
                break;
                case 0x03:
                    captureCityPreviousNotFinished();
                break;
                case 0x04:
                {
                    uint16_t player_count,clan_count;
                    if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    player_count=data[pos];
                    pos+=sizeof(uint8_t);
                    if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    clan_count=data[pos];
                    pos+=sizeof(uint8_t);
                    if((size-pos)==0)
                        captureCityStartBattle(player_count,clan_count);
                    else if((size-pos)==(int)(sizeof(uint32_t)))
                    {
                        uint32_t fightId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
                        captureCityStartBotFight(player_count,clan_count,fightId);
                    }
                }
                break;
                case 0x05:
                {
                    uint16_t player_count,clan_count;
                    if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    player_count=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<(unsigned int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    clan_count=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                    pos+=sizeof(uint16_t);
                    captureCityDelayedStart(player_count,clan_count);
                }
                break;
                case 0x06:
                    captureCityWin();
                break;
                default:
                    parseError("Procotol wrong or corrupted","unknown subCodeType main code: "+std::to_string(packetCode)+
                               ", returnCode: "+std::to_string(returnCode)+", line: "+
                               std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
                return false;
            }
        }
        break;

        //Gateway Cache updating
        case 0x78:
        {
            uint8_t gateway;
            uint8_t progression;
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            gateway=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(unsigned int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            progression=data[pos];
            pos+=sizeof(uint8_t);
            gatewayCacheUpdate(gateway,progression);
        }
        break;

        default:
            parseError("Procotol wrong or corrupted","unknown ident main code: "+std::to_string(packetCode));
            return false;
        break;
    }
    if((size-pos)!=0)
    {
        parseError("Procotol wrong or corrupted","remaining data: parseMessage("+std::to_string(packetCode)+
                   ","+binarytoHexa(data,pos)+
                   " "+binarytoHexa(data+pos,size-pos)+
                   ") line "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                   );
        return false;
    }
    return true;
}

