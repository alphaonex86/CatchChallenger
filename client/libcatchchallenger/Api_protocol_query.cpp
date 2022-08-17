#include "Api_protocol.hpp"

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/cpp11addition.hpp"

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

#if ! defined (ONLYMAPRENDER)

using namespace CatchChallenger;

bool Api_protocol::parseQuery(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    int pos=0;
    if(!is_logged)
    {
        parseError("Procotol wrong or corrupted","is not logged with main ident: "+std::to_string(packetCode)+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    switch(packetCode)
    {
        //Teleport the player
        case 0xE1:
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
            uint8_t x,y;
            if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            x=data[pos];
            pos+=sizeof(uint8_t);
            y=data[pos];
            pos+=sizeof(uint8_t);
            uint8_t directionInt;
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            directionInt=data[pos];
            pos+=sizeof(uint8_t);
            if(directionInt<1 || directionInt>4)
            {
                parseError("Procotol wrong or corrupted","direction have wrong value: "+std::to_string(directionInt)+
                           ", at main ident: "+std::to_string(packetCode)+
                           ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            Direction direction=(Direction)directionInt;

            TeleportQueryInformation teleportQueryInformation;
            teleportQueryInformation.queryId=queryNumber;
            teleportQueryInformation.direction=direction;
            teleportList.push_back(teleportQueryInformation);
            teleportTo(mapId,x,y,direction);
        }
        break;
        //Event change
        case 0xE2:
        {
            uint8_t event,event_value;
            if((size-pos)<(unsigned int)sizeof(uint8_t)*2)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            event=data[pos];
            pos+=sizeof(uint8_t);
            event_value=data[pos];
            pos+=sizeof(uint8_t);

            if(event>=CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
            {
                parseError("Procotol wrong or corrupted",std::string("event index > than max, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            this->events[event]=event_value;

            newEvent(event,event_value);
            postReplyData(queryNumber,NULL,0);
        }
        break;
        //Another player request a trade
        case 0xE0:
        {
            if(!tradeRequestId.empty() || isInTrade)
            {
                parseError("Procotol wrong or corrupted","Already on trade");
                return false;
            }
            if(!battleRequestId.empty() || isInBattle)
            {
                parseError("Procotol wrong or corrupted","Already on battle");
                return false;
            }
            uint8_t pseudoSize=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(unsigned int)pseudoSize)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::string pseudo=std::string(data+pos,pseudoSize);
            pos+=pseudoSize;
            if(pseudo.empty())
            {
                parseError("Procotol wrong or corrupted","utf8 enconding failed wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t skinInt;
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            skinInt=data[pos];
            pos+=sizeof(uint8_t);
            tradeRequestId.push_back(queryNumber);
            tradeRequested(pseudo,skinInt);
        }
        break;
        //Another player request a trade
        case 0xDF:
        {
            if(!tradeRequestId.empty() || isInTrade)
            {
                parseError("Procotol wrong or corrupted","Already on trade");
                return false;
            }
            if(!battleRequestId.empty() || isInBattle)
            {
                parseError("Procotol wrong or corrupted","Already on battle");
                return false;
            }
            uint8_t pseudoSize=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(unsigned int)pseudoSize)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            std::string pseudo=std::string(data+pos,pseudoSize);
            pos+=pseudoSize;
            if(pseudo.empty())
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t skinInt;
            if((size-pos)<(unsigned int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            skinInt=data[pos];
            pos+=sizeof(uint8_t);
            battleRequestId.push_back(queryNumber);
            battleRequested(pseudo,skinInt);
        }
        break;
        default:
            parseError("Procotol wrong or corrupted","unknown ident main code: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    if((size-pos)!=0)
    {
        parseError("Procotol wrong or corrupted","remaining data: parseFullQuery("+std::to_string(packetCode)+
                   ","+binarytoHexa(data,pos)+
                   " "+binarytoHexa(data+pos,size-pos)+
                   ") line "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                      );
        return false;
    }
    return true;
}
#endif
