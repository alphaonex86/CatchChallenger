#include "Api_protocol.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"

#include <iostream>

#ifdef BENCHMARKMUTIPLECLIENT
#include <fstream>
#include <unistd.h>
#endif

//have message without reply
bool Api_protocol::parseMessage(const uint8_t &packetCode,const char * const data,const unsigned int &size)
{
    const bool &returnValue=parseMessage(packetCode,std::string(data,size));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!returnValue)
    {
        errorParsingLayer("Api_protocol::parseMessage(): return false (abort), need be aborted before");
        abort();
    }
    #endif
    return returnValue;
}

//have message without reply
bool Api_protocol::parseMessage(const uint8_t &packetCode, const std::string &data)
{
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
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(packetCode)
    {
        //Insert player on map, need be delayed if number_of_map==0
        case 0x6B:
        {
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t mapListSize;
            in >> mapListSize;
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
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> mapId;
                }

                uint16_t playerSizeList;
                if(max_players<=255)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    Player_public_informations public_informations;
                    if(max_players<=255)
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t playerSmallId;
                        in >> playerSmallId;
                        public_informations.simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> public_informations.simplifiedId;
                    }

                    //x and y
                    if((size-pos)<(int)sizeof(uint8_t)*2)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t directionAndPlayerType;
                    in >> directionAndPlayerType;
                    uint8_t directionInt,playerTypeInt;
                    directionInt=directionAndPlayerType & 0x0F;
                    playerTypeInt=directionAndPlayerType & 0xF0;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError("Procotol wrong or corrupted","direction have wrong value: "+
                                   std::to_string(directionInt)+", at main ident: "+
                                   std::to_string(packetCode)+", directionAndPlayerType: "+
                                   std::to_string(directionAndPlayerType)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                                   ", data: "+binarytoHexa(data.data(),data.size()));
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
                                   ", data: "+binarytoHexa(data.data(),data.size()));
                        return false;
                    }
                    public_informations.type=playerType;

                    //the speed
                    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> public_informations.speed;
                    }
                    else
                        public_informations.speed=CommonSettingsServer::commonSettingsServer.forcedSpeed;

                    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                    {
                        //the pseudo
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t pseudoSize;
                        in >> pseudoSize;
                        if(pseudoSize>0)
                        {
                            if((size-pos)<(int)pseudoSize)
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
                            public_informations.pseudo=std::string(rawText.data(),rawText.size());
                            in.device()->seek(in.device()->pos()+rawText.size());
                            if(public_informations.pseudo.empty())
                            {
                                parseError("Procotol wrong or corrupted","UTF8 decoding failed for pseudo: "+public_informations.pseudo+
                                           ", rawData: "+binarytoHexa(rawText.data(),rawText.size())+", line: "+
                                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                                           );
                                return false;
                            }
                        }
                    }
                    else
                        public_informations.pseudo.clear();

                    //the skin
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t skinId;
                    in >> skinId;
                    public_informations.skinId=skinId;

                    //the following monster id to show
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t monsterId;
                    in >> monsterId;
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
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            //move the player
            uint8_t directionInt,moveListSize;
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                uint8_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerId;
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    std::vector<std::pair<uint8_t,Direction> > movement;
                    std::pair<uint8_t,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError("Procotol wrong or corrupted","move size == 0 with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((size-pos)<(int)sizeof(uint8_t)*2)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
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
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> playerSizeList;
                uint16_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerId;
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    std::vector<std::pair<uint8_t,Direction> > movement;
                    std::pair<uint8_t,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError("Procotol wrong or corrupted","move size == 0 with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((size-pos)<(int)sizeof(uint8_t)*2)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident at move player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
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
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            //remove player
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                uint8_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerId;
                    remove_player(playerId);
                    index++;
                }
            }
            else
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> playerSizeList;
                uint16_t playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident at remove player: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerId;
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
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if(max_players<=255)
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t current_player_connected_8Bits;
                in >> current_player_connected_8Bits;
                number_of_player(current_player_connected_8Bits,max_players);
            }
            else
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                               ", in.device()->pos(): "+std::to_string(in.device()->pos())+
                               ", in.device()->size(): "+std::to_string(in.device()->size())+
                               ", in.device()->isOpen(): "+std::to_string(in.device()->isOpen()));
                    return false;
                }
                uint16_t current_player_connected_16Bits;
                in >> current_player_connected_16Bits;
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
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            uint16_t playerSizeList;
            if(max_players<=255)
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
            }
            else
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> playerSizeList;
            }
            int index_sub_loop=0;
            while(index_sub_loop<playerSizeList)
            {
                //player id
                uint16_t simplifiedId;
                if(max_players<=255)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t playerSmallId;
                    in >> playerSmallId;
                    simplifiedId=playerSmallId;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> simplifiedId;
                }

                //x and y
                if((size-pos)<(int)sizeof(uint8_t)*2)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t x,y;
                in >> x;
                in >> y;

                //direction and player type
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t directionInt;
                in >> directionInt;
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
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t mapListSize;
            in >> mapListSize;
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
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> mapId;
                }
                uint16_t playerSizeList;
                if(max_players<=255)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    uint16_t simplifiedId;
                    if(max_players<=255)
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        uint8_t playerSmallId;
                        in >> playerSmallId;
                        simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> simplifiedId;
                    }

                    //x and y
                    if((size-pos)<(int)sizeof(uint8_t)*2)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t directionInt;
                    in >> directionInt;
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
            if((size-pos)<(int)sizeof(uint8_t))
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t chat_type_int;
            in >> chat_type_int;
            if(chat_type_int<1 || chat_type_int>8)
            {
                parseError("Procotol wrong or corrupted","wrong chat type with main ident: "+std::to_string(packetCode)+", chat_type_int: "+std::to_string(chat_type_int)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            Chat_type chat_type=(Chat_type)chat_type_int;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong text with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string text;
            uint16_t textSize;
            if(chat_type_int>=1 && chat_type_int<=6)
            {
                uint8_t textSize8;
                in >> textSize8;
                textSize=textSize8;
            }
            else
                in >> textSize;
            if(textSize>0)
            {
                if((size-pos)<(int)textSize)
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", pseudoSize: "+std::to_string(textSize)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),textSize);
                text=std::string(rawText.data(),rawText.size());
                in.device()->seek(in.device()->pos()+rawText.size());
            }
            if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
                new_system_text(chat_type,text);
            else
            {
                std::string pseudo;
                uint8_t pseudoSize;
                in >> pseudoSize;
                if(pseudoSize>0)
                {
                    if((size-pos)<(int)pseudoSize)
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", pseudoSize: "+std::to_string(pseudoSize)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+
                                   ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                   );
                        return false;
                    }
                    QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
                    pseudo=std::string(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(pseudo.empty())
                    {
                        parseError("Procotol wrong or corrupted","UTF8 decoding failed: packetCode: "+std::to_string(packetCode)+
                                   ", rawText: "+binarytoHexa(rawText.data(),rawText.size())+
                                   ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                }
                uint8_t player_type_int;
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> player_type_int;
                Player_type player_type=(Player_type)player_type_int;
                if(player_type!=Player_type_normal && player_type!=Player_type_premium && player_type!=Player_type_gm && player_type!=Player_type_dev)
                {
                    parseError("Procotol wrong or corrupted","wrong player type with main ident: "+std::to_string(packetCode)+
                               ", player_type_int: "+std::to_string(player_type_int)+
                               ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
                    return false;
                }
                new_chat_text(chat_type,text,pseudo,player_type);
            }
        }
        break;
        //Insert plant on map
        case 0x5C:
        {
            if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
            {
                parseError("Procotol wrong or corrupted","CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t plantListSize;
            in >> plantListSize;
            int index=0;
            while(index<plantListSize)
            {
                uint32_t mapId;
                if(number_of_map==0)
                {
                    parseError("Internal error","number_of_map==0 with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                else if(number_of_map<=255)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> mapId;
                }
                //x and y
                if((size-pos)<(int)sizeof(uint8_t)*2)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t x,y;
                in >> x;
                in >> y;
                //plant
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t plant;
                in >> plant;
                //seconds to mature
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint16_t seconds_to_mature;
                in >> seconds_to_mature;

                insert_plant(mapId,x,y,plant,seconds_to_mature);
                index++;
            }
        }
        break;
        //Remove plant on map
        case 0x5D:
        {
            if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true)
            {
                parseError("Procotol wrong or corrupted","CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer==true with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if(!character_selected || number_of_map==0)
            {
                //because befine max_players
                DelayedMessage delayedMessageTemp;
                delayedMessageTemp.data=data;
                delayedMessageTemp.packetCode=packetCode;
                delayedMessages.push_back(delayedMessageTemp);
                return true;
            }
            if((size-pos)<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t plantListSize;
            in >> plantListSize;
            int index=0;
            while(index<plantListSize)
            {
                uint32_t mapId;
                if(number_of_map==0)
                {
                    parseError("Internal error","number_of_map==0 with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                else if(number_of_map<=255)
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint8_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    uint16_t mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> mapId;
                }
                //x and y
                if((size-pos)<(int)sizeof(uint8_t)*2)
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t x,y;
                in >> x;
                in >> y;

                remove_plant(mapId,x,y);
                index++;
            }
        }
        break;

        //file as input
        case 0x76://raw
        case 0x77://compressed
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            uint8_t fileListSize;
            in >> fileListSize;
            if(fileListSize==0)
            {
                parseError("Procotol wrong or corrupted","fileListSize==0 with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }

            uint32_t sub_size32=static_cast<uint32_t>(in.device()->size()-in.device()->pos());
            uint32_t decompressedSize=0;
            if(ProtocolParsingBase::compressionTypeClient==CompressionType::None || packetCode==0x76)
            {
                decompressedSize=sub_size32;
                memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data.data()+in.device()->pos(),sub_size32);
            }
            else
                decompressedSize=computeDecompression(data.data()+in.device()->pos(),ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);

            const QByteArray data2(ProtocolParsingBase::tempBigBufferForUncompressedInput,decompressedSize);
            QDataStream in2(data2);
            in2.setVersion(QDataStream::Qt_4_4);in2.setByteOrder(QDataStream::LittleEndian);

            //std::cout << QString(QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->size())).toHex()).toStdString() << " " << std::string("%1:%2").arg(__FILE__).arg(__LINE__).toStdString() << std::endl;

            int index=0;
            while(index<fileListSize)
            {
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string fileName;
                uint8_t fileNameSize;
                in2 >> fileNameSize;
                if(fileNameSize>0)
                {
                    if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)fileNameSize)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    QByteArray rawText=data2.mid(static_cast<int>(in2.device()->pos()),fileNameSize);
                    in2.device()->seek(in2.device()->pos()+rawText.size());
                    fileName=std::string(rawText.data(),rawText.size());
                }
                if(extensionAllowed.find(QFileInfo(QString::fromStdString(fileName))
                                         .suffix().toStdString())==extensionAllowed.cend())
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->size()));
                    parseError("Procotol wrong or corrupted","extension not allowed: "+fileName+
                               " with main ident: "+std::to_string(packetCode)+", line: "+
                               std::string(__FILE__)+":"+std::to_string(__LINE__)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())
                               );
                    if(!tolerantMode)
                        return false;
                }
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint32_t size;
                in2 >> size;
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<size)
                {
                    parseError("Procotol wrong or corrupted","wrong file data size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                std::string dataFile;
                {
                    QByteArray tdata=data2.mid(static_cast<int>(in2.device()->pos()),static_cast<int>(size));
                    dataFile=std::string(tdata.data(),tdata.size());
                }
                in2.device()->seek(in2.device()->pos()+size);
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
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t code;
            in >> code;
            std::string text;
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t textSize;
                in >> textSize;
                if(textSize>0)
                {
                    if((size-pos)<(int)textSize)
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),textSize);
                    in.device()->seek(in.device()->pos()+rawText.size());
                    text=std::string(rawText.data(),rawText.size());
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
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string name;
            uint8_t stringSize;
            in >> stringSize;
            if(stringSize>0)
            {
                if((size-pos)<(int)stringSize)
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                QByteArray stringRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),stringSize);
                name=std::string(stringRaw.data(),stringRaw.size());
                in.device()->seek(in.device()->pos()+stringRaw.size());
            }
            clanInformations(name);
        }
        break;
        //clan invite
        case 0x63:
        {
            if((size-pos)<(int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t clanId;
            in >> clanId;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string name;
            uint8_t stringSize;
            in >> stringSize;
            if(stringSize>0)
            {
                if((size-pos)<(int)stringSize)
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                QByteArray stringRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),stringSize);
                name=std::string(stringRaw.data(),stringRaw.size());
                in.device()->seek(in.device()->pos()+stringRaw.size());
            }
            clanInvite(clanId,name);
        }
        break;
        //Send datapack send size
        case 0x75:
        {
            if((size-pos)<(int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t datapckFileNumber;
            in >> datapckFileNumber;
            if((size-pos)<(int)sizeof(uint32_t))
            {
                parseError("Procotol wrong or corrupted","wrong string for reason with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint32_t datapckFileSize;
            in >> datapckFileSize;
            datapackSizeBase(datapckFileNumber,datapckFileSize);
        }
        break;
        //Internal server list for the current pool
        case 0x40:
        {
            haveTheServerList=true;

            if(in.device()->pos()!=0)
                abort();
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t serverMode;
            in >> serverMode;
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
            QList<ServerFromPoolForDisplayTemp> serverTempList;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> serverListSize;
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
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    in >> server.charactersGroupIndex;
                }
                //uniquekey
                {
                    if((size-pos)<(int)sizeof(uint32_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    in >> server.uniqueKey;
                }
                if(proxyMode==Api_protocol::ProxyMode::Reconnect)
                {
                    //host
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t stringSize;
                        in >> stringSize;
                        if(stringSize>0)
                        {
                            if((size-pos)<(int)stringSize)
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            QByteArray stringRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),stringSize);
                            server.host=std::string(stringRaw.data(),stringRaw.size());
                            in.device()->seek(in.device()->pos()+stringRaw.size());
                        }
                    }
                    //port
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        in >> server.port;
                    }
                }
                //xml (name, description, ...)
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint16_t stringSize;
                    in >> stringSize;
                    if(stringSize>0)
                    {
                        if((size-pos)<(int)stringSize)
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        QByteArray stringRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),stringSize);
                        server.xml=std::string(stringRaw.data(),stringRaw.size());
                        in.device()->seek(in.device()->pos()+stringRaw.size());
                    }
                }
                //logical to contruct the tree
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    in >> server.logicalGroupIndex;
                }
                //max player
                {
                    if((size-pos)<(int)sizeof(uint16_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    in >> server.maxPlayer;
                }
                serverTempList << server;
                serverListIndex++;
            }
            serverListIndex=0;
            while(serverListIndex<serverListSize)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> serverTempList[serverListIndex].currentPlayer;
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            logicialGroupIndexList.clear();
            in >> logicalGroupSize;
            if(logicalGroupSize>0)
            {
                std::string language=getLanguage();
                while(logicalGroupIndex<logicalGroupSize)
                {
                    std::string path;
                    std::string xml;
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t pathSize;
                        in >> pathSize;
                        if(pathSize>0)
                        {
                            if((size-pos)<(int)pathSize)
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            QByteArray pathRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pathSize);
                            path=std::string(pathRaw.data(),pathRaw.size());
                            in.device()->seek(in.device()->pos()+pathRaw.size());
                        }
                    }
                    {
                        if((size-pos)<(int)sizeof(uint16_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint16_t xmlSize;
                        in >> xmlSize;
                        if(xmlSize>0)
                        {
                            if((size-pos)<(int)xmlSize)
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            QByteArray xmlRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),xmlSize);
                            xml=std::string(xmlRaw.data(),xmlRaw.size());
                            in.device()->seek(in.device()->pos()+xmlRaw.size());
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
                delayedMessage.data=data;
                delayedMessages.push_back(delayedMessage);
                return true;
            }
            if((size-pos)<(int)sizeof(uint16_t))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            in >> inventorySize;
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> id;
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> quantity;
                if(items.find(id)!=items.cend())
                    items[id]+=quantity;
                else
                    items[id]=quantity;
                index++;
            }
            if((size-pos)<(int)sizeof(uint16_t))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> warehouse_items;
            in >> inventorySize;
            index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> id;
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> quantity;
                if(warehouse_items.find(id)!=warehouse_items.cend())
                    warehouse_items[id]+=quantity;
                else
                    warehouse_items[id]=quantity;
                index++;
            }
            have_inventory(items,warehouse_items);
        }
        break;
        //Add object
        case 0x55:
        {
            if((size-pos)<(int)sizeof(uint16_t))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            in >> inventorySize;
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> id;
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> quantity;
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
            if((size-pos)<(int)sizeof(uint32_t))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::unordered_map<uint16_t,uint32_t> items;
            uint16_t inventorySize,id;
            uint32_t quantity;
            in >> inventorySize;
            uint32_t index=0;
            while(index<inventorySize)
            {
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> id;
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> quantity;
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","not in trade main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t type;
            in >> type;
            switch(type)
            {
                //cash
                case 0x01:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(uint64_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add cash");
                        return false;
                    }
                    quint64 cash;
                    in >> cash;
                    tradeAddTradeCash(cash);
                }
                break;
                //item
                case 0x02:
                {
                    if((data.size()-in.device()->pos())<((int)sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add item id");
                        return false;
                    }
                    uint16_t item;
                    in >> item;
                    if((data.size()-in.device()->pos())<((int)sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong remaining size for trade add item quantity");
                        return false;
                    }
                    uint32_t quantity;
                    in >> quantity;
                    tradeAddTradeObject(item,quantity);
                }
                break;
                //monster
                case 0x03:
                {
                    PlayerMonster monster;
                    if(!dataToPlayerMonster(in,monster))
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","already in trade main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::string pseudo;
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(pseudoSize>0)
            {
                if((size-pos)<(int)pseudoSize)
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
                pseudo=std::string(rawText.data(),rawText.size());
                in.device()->seek(in.device()->pos()+rawText.size());
                if(pseudo.empty())
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
            }
            uint8_t skinId;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            in >> skinId;
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
            uint8_t monsterPlace;

            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> attackReturnListSize;
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
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> tempuint;
                if(tempuint>1)
                {
                    parseError("Procotol wrong or corrupted","code to bool with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                tempAttackReturn.doByTheCurrentMonster=(tempuint!=0);
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> tempuint;
                switch(tempuint)
                {
                    case Skill::AttackReturnCase_NormalAttack:
                    {
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> tempuint;
                        if(tempuint>1)
                        {
                            parseError("Procotol wrong or corrupted","code to bool with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        tempAttackReturn.success=(tempuint!=0);
                        if((size-pos)<(int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> tempAttackReturn.attack;
                        //add buff
                        index=0;
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buffEffect.buff;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> tempuint;
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
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buffEffect.level;
                            tempAttackReturn.addBuffEffectMonster.push_back(buffEffect);
                            index++;
                        }
                        //remove buff
                        index=0;
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buffEffect.buff;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> tempuint;
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
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buffEffect.level;
                            tempAttackReturn.removeBuffEffectMonster.push_back(buffEffect);
                            index++;
                        }
                        //life effect
                        index=0;
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((size-pos)<(int)(sizeof(uint32_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> lifeEffectReturn.quantity;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> tempuint;
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
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((size-pos)<(int)(sizeof(uint32_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> lifeEffectReturn.quantity;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> tempuint;
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
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> monsterPlace;
                        if((size-pos)<(int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> publicPlayerMonster.monster;
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> publicPlayerMonster.level;
                        if((size-pos)<(int)(sizeof(uint32_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> publicPlayerMonster.hp;
                        if((size-pos)<(int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> publicPlayerMonster.catched_with;
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> genderInt;
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
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> buffListSize;
                        index=0;
                        while(index<buffListSize)
                        {
                            PlayerBuff buff;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buff.buff;
                            if((size-pos)<(int)(sizeof(uint8_t)))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                                return false;
                            }
                            in >> buff.level;
                            publicPlayerMonster.buffs.push_back(buff);
                            index++;
                        }
                        tempAttackReturn.publicPlayerMonster=publicPlayerMonster;
                    }
                    break;
                    case Skill::AttackReturnCase_ItemUsage:
                    {
                        if((size-pos)<(int)(sizeof(uint8_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> tempuint;
                        tempAttackReturn.on_current_monster=(tempuint!=0x00);
                        if((size-pos)<(int)(sizeof(uint16_t)))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }
                        in >> tempAttackReturn.item;
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            std::string pseudo;
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(pseudoSize>0)
            {
                if((size-pos)<(int)pseudoSize)
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
                pseudo=std::string(rawText.data(),rawText.size());
                in.device()->seek(in.device()->pos()+rawText.size());
                if(pseudo.empty())
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","UTF8 decoding failed with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
            }
            uint8_t skinId;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> skinId;
            PublicPlayerMonster publicPlayerMonster;
            std::vector<uint8_t> stat;
            uint8_t genderInt;
            uint8_t buffListSize;
            uint8_t monsterPlace;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t statListSize;
            in >> statListSize;
            int index=0;
            while(index<statListSize)
            {
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                uint8_t statEntry;
                in >> statEntry;
                stat.push_back(statEntry);
                index++;
            }
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> monsterPlace;
            if(monsterPlace<=0 || monsterPlace>stat.size())
            {
                parseError("Procotol wrong or corrupted","monster place wrong range with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if((size-pos)<(int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> publicPlayerMonster.monster;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> publicPlayerMonster.level;
            if((size-pos)<(int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> publicPlayerMonster.hp;
            if((size-pos)<(int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> publicPlayerMonster.catched_with;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> genderInt;
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> buffListSize;
            index=0;
            while(index<buffListSize)
            {
                PlayerBuff buff;
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> buff.buff;
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> buff.level;
                publicPlayerMonster.buffs.push_back(buff);
                index++;
            }
            battleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
        }
        break;
        //random seeds as input
        case 0x53:
        {
            if(data.size()<128)
            {
                parseError("Procotol wrong or corrupted","have too less data for random seed with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            random_seeds(std::string(data.data(),data.size()));
            return true;//quit here because all data is always used
        }
        break;

        //Result of the turn
        case 0x5E:
        {
            uint8_t returnCode;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                    captureCityYourAreNotLeader();
                break;
                case 0x02:
                {
                    std::string city;
                    uint8_t stringSize;
                    in >> stringSize;
                    if(stringSize>0)
                    {
                        if((size-pos)<(int)stringSize)
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        QByteArray stringRaw=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),stringSize);
                        city=std::string(stringRaw.data(),stringRaw.size());
                        in.device()->seek(in.device()->pos()+stringRaw.size());
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
                    if((size-pos)<(int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> player_count;
                    if((size-pos)<(int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> clan_count;
                    if((in.device()->size()-in.device()->pos())==0)
                        captureCityStartBattle(player_count,clan_count);
                    else if((in.device()->size()-in.device()->pos())==(int)(sizeof(uint32_t)))
                    {
                        uint32_t fightId;
                        in >> fightId;
                        captureCityStartBotFight(player_count,clan_count,fightId);
                    }
                }
                break;
                case 0x05:
                {
                    uint16_t player_count,clan_count;
                    if((size-pos)<(int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> player_count;
                    if((size-pos)<(int)(sizeof(uint16_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        return false;
                    }
                    in >> clan_count;
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> gateway;
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> progression;
            emit gatewayCacheUpdate(gateway,progression);
        }
        break;

        default:
            parseError("Procotol wrong or corrupted","unknown ident main code: "+std::to_string(packetCode));
            return false;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray fdata=QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->pos()));
        QByteArray ldata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),static_cast<int>(in.device()->size()-in.device()->pos()));
        parseError("Procotol wrong or corrupted","remaining data: parseMessage("+std::to_string(packetCode)+
                   ","+binarytoHexa(fdata.data(),fdata.size())+
                   " "+binarytoHexa(ldata.data(),ldata.size())+
                   ") line "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                   );
        return false;
    }
    return true;
}

