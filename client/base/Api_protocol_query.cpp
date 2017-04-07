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
    const bool &returnValue=parseQuery(packetCode,queryNumber,QByteArray(data,size));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!returnValue)
    {
        errorParsingLayer("Api_protocol::parseQuery(): return false (abort), need be aborted before");
        abort();
    }
    #endif
    return returnValue;
}

bool Api_protocol::parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1 %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(packetCode)
    {
        //Teleport the player
        case 0xE1:
        {
            uint32_t mapId;
            if(number_of_map<=255)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> mapId;
            }
            uint8_t x,y;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> x;
            in >> y;
            uint8_t directionInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> directionInt;
            if(directionInt<1 || directionInt>4)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            Direction direction=(Direction)directionInt;

            TeleportQueryInformation teleportQueryInformation;
            teleportQueryInformation.queryId=queryNumber;
            teleportQueryInformation.direction=direction;
            teleportList << teleportQueryInformation;
            teleportTo(mapId,x,y,direction);
        }
        break;
        //Event change
        case 0xE2:
        {
            uint8_t event,event_value;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
            if(!tradeRequestId.isEmpty() || isInTrade)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                return false;
            }
            if(!battleRequestId.isEmpty() || isInBattle)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                return false;
            }
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                              .arg(packetCode)
                              .arg('X')
                              .arg(pseudoSize)
                              .arg(QString(data.mid(in.device()->pos()).toHex()))
                              .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                              );
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
            QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(pseudo.isEmpty())
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: packetCode: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                              .arg(packetCode)
                              .arg('X')
                              .arg(QString(rawText.toHex()))
                              .arg(rawText.size())
                              .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                              );
                return false;
            }
            uint8_t skinInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skinInt;
            tradeRequestId << queryNumber;
            tradeRequested(pseudo,skinInt);
        }
        break;
        //Another player request a trade
        case 0xDF:
        {
            if(!tradeRequestId.isEmpty() || isInTrade)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                return false;
            }
            if(!battleRequestId.isEmpty() || isInBattle)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                return false;
            }
            uint8_t pseudoSize;
            in >> pseudoSize;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                              .arg(packetCode)
                              .arg('X')
                              .arg(pseudoSize)
                              .arg(QString(data.mid(in.device()->pos()).toHex()))
                              .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                              );
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
            QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(pseudo.isEmpty())
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: packetCode: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                              .arg(packetCode)
                              .arg('X')
                              .arg(QString(rawText.toHex()))
                              .arg(rawText.size())
                              .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                              );
                return false;
            }
            uint8_t skinInt;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skinInt;
            battleRequestId << queryNumber;
            battleRequested(pseudo,skinInt);
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident main code: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseFullQuery(%1,%2 %3) line %4")
                      .arg(packetCode)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                      );
        return false;
    }
    return true;
}
