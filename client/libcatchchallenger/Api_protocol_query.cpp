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
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"

#include <QCoreApplication>
#include <QDataStream>

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//have query with reply
bool Api_protocol::parseQuery(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    const bool &returnValue=parseQuery(packetCode,queryNumber,std::string(data,size));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!returnValue)
    {
        errorParsingLayer("Api_protocol::parseQuery(): return false (abort), need be aborted before");
        abort();
    }
    #endif
    return returnValue;
}

bool Api_protocol::parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const std::string &data)
{
    if(!is_logged)
    {
        parseError("Procotol wrong or corrupted","is not logged with main ident: "+std::to_string(packetCode)+", file: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
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
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
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
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
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
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> mapId;
            }
            uint8_t x,y;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> x;
            in >> y;
            uint8_t directionInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> directionInt;
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
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> event;
            in >> event_value;
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
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
            std::string pseudo=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(pseudo.empty())
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","utf8 enconding failed wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t skinInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> skinInt;
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
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
            std::string pseudo=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(pseudo.empty())
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t skinInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> skinInt;
            battleRequestId.push_back(queryNumber);
            battleRequested(pseudo,skinInt);
        }
        break;
        default:
            parseError("Procotol wrong or corrupted","unknown ident main code: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray fdata=QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->pos()));
        QByteArray ldata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),static_cast<int>(in.device()->size()-in.device()->pos()));
        parseError("Procotol wrong or corrupted","remaining data: parseFullQuery("+std::to_string(packetCode)+
                   ","+binarytoHexa(fdata.data(),fdata.size())+
                   " "+binarytoHexa(ldata.data(),ldata.size())+
                   ") line "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                      );
        return false;
    }
    return true;
}
