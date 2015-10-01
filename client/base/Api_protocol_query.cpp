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
#include "LanguagesSelect.h"

#include <QCoreApplication>

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//have query with reply
void Api_protocol::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    parseQuery(mainCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("have not query of this type, mainCodeType: %1, queryNumber: %2").arg(mainCodeType).arg(queryNumber));
}

void Api_protocol::parseFullQuery(const uint8_t &mainCodeType, const uint8_t &subCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    parseFullQuery(mainCodeType,subCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1").arg(mainCodeType));
        return;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0x79:
        {
            switch(subCodeType)
            {
                //Teleport the player
                case 0x01:
                {
                    uint32_t mapId;
                    if(number_of_map<=255)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        uint8_t mapTempId;
                        in >> mapTempId;
                        mapId=mapTempId;
                    }
                    else if(number_of_map<=65535)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        uint16_t mapTempId;
                        in >> mapTempId;
                        mapId=mapTempId;
                    }
                    else
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> mapId;
                    }
                    uint8_t x,y;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> x;
                    in >> y;
                    uint8_t directionInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> directionInt;
                    if(directionInt<1 || directionInt>4)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    teleportList << queryNumber;
                    teleportTo(mapId,x,y,direction);
                }
                break;
                //Event change
                case 0x02:
                {
                    uint8_t event,event_value;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> event;
                    in >> event_value;
                    newEvent(event,event_value);
                    postReplyData(queryNumber,NULL,0);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
        }
        break;
        case 0x80:
        {
            switch(subCodeType)
            {
                //Another player request a trade
                case 0x01:
                {
                    if(!tradeRequestId.isEmpty() || isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                        return;
                    }
                    uint8_t pseudoSize;
                    in >> pseudoSize;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(pseudoSize)
                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                      );
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(pseudo.isEmpty())
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                      );
                        return;
                    }
                    uint8_t skinInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> skinInt;
                    tradeRequestId << queryNumber;
                    tradeRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
        }
        break;
        case 0x90:
        {
            switch(subCodeType)
            {
                //Another player request a trade
                case 0x01:
                {
                    if(!tradeRequestId.isEmpty() || isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                        return;
                    }
                    uint8_t pseudoSize;
                    in >> pseudoSize;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(pseudoSize)
                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                      );
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(pseudo.isEmpty())
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                      );
                        return;
                    }
                    uint8_t skinInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> skinInt;
                    battleRequestId << queryNumber;
                    battleRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident main code: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseFullQuery(%1,%2,%3 %4) line %5")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                      );
        return;
    }
}
