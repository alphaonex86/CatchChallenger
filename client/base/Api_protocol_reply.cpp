#include "Api_protocol.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif
#include <iostream>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"
#include "LanguagesSelect.h"

#include <QCoreApplication>
#include <QRegularExpression>

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//send reply
bool Api_protocol::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    const bool &returnValue=parseReplyData(packetCode,queryNumber,QByteArray(data,size));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!returnValue)
    {
        errorParsingLayer("Api_protocol::parseReplyData(): return false (abort), need be aborted before");
        abort();
    }
    #endif
    return returnValue;
}

bool Api_protocol::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(packetCode)
    {
        case 0xA0:
        {
            //Protocol initialization
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(packetCode).arg(queryNumber));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x05:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zlib;
                    break;
                    case 0x06:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Xz;
                    break;
                    case 0x07:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Lz4;
                    break;
                    default:
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(packetCode).arg(queryNumber));
                    return false;
                }
                if(stageConnexion==StageConnexion::Stage1)
                {
                    if(data.size()!=(sizeof(uint8_t)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                    {
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong size (stage 1) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(packetCode).arg(queryNumber));
                        return false;
                    }
                    token=data.right(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    have_receive_protocol=true;
                    protocol_is_good();
                }
                else if(stageConnexion==StageConnexion::Stage4)
                {
                    if(data.size()!=(sizeof(uint8_t)))
                    {
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(packetCode).arg(queryNumber));
                        return false;
                    }
                    have_receive_protocol=true;
                    //send token to game server
                    packOutcommingQuery(0x93,this->queryNumber(),tokenForGameServer.constData(),tokenForGameServer.size());
                }
                else
                {
                    newError(QStringLiteral("Internal problem"),QStringLiteral("stageConnexion!=StageConnexion::Stage1/3"));
                    return false;
                }
                return true;
            }
            else
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Protocol not supported");
                else if(returnCode==0x03)
                    string=tr("Server full");
                else
                    string=tr("Unknown error %1").arg(returnCode);
                //newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("the server have returned: %1").arg(string));
                //show the message box
                disconnected(QStringLiteral("the server have returned: %1").arg(string));
                closeSocket();
                return false;
            }
        }
        break;
        //Get first data and send the login
        case 0xA8:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(packetCode).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Bad login");
                else if(returnCode==0x03)
                    string=tr("Wrong login/pass");
                else if(returnCode==0x04)
                    string=tr("Server internal error");
                else if(returnCode==0x05)
                    string=tr("Can't create character and don't have character");
                else if(returnCode==0x06)
                    string=tr("Login already in progress");
                else if(returnCode==0x07)
                {
                    tryCreateAccount();
                    return true;
                }
                else
                    string=tr("Unknown error %1").arg(returnCode);
                std::cerr << "is not logged, reason: " << string.toStdString() << std::endl;
                notLogged(string);
                return false;
            }
            else
            {
                if(!haveTheServerList)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("don't have server list before this reply main ident: %1 and queryNumber: %2, line: %3").arg(packetCode).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                if(!haveTheLogicalGroupList)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("don't have logical group list before this reply main ident: %1 and queryNumber: %2, line: %3").arg(packetCode).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.character_delete_time;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_character;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the min_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.min_character;
                if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("max_character<min_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerMonsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerMonsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerItems, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerItems, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(28);
                memcpy(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),data.mid(in.device()->pos(),28).constData(),28);
                in.device()->seek(in.device()->pos()+CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
                {
                    //the mirror
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint8_t mirrorSize;
                    in >> mirrorSize;
                    if(mirrorSize>0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),mirrorSize);
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=std::string(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                        if(!regex_search(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase,std::regex("^https?://")))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("mirror with not http(s) protocol with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                    }
                    else
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.clear();
                }
                //characters
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint8_t charatersGroupSize;
                    in >> charatersGroupSize;
                    uint8_t charatersGroupIndex=0;
                    while(charatersGroupIndex<charatersGroupSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        QList<CharacterEntry> characterEntryList;
                        uint8_t characterListSize;
                        in >> characterListSize;
                        uint8_t characterListIndex=0;
                        while(characterListIndex<characterListSize)
                        {
                            CharacterEntry characterEntry;
                            //characterId
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return false;
                            }
                            in >> characterEntry.character_id;
                            {
                                //pseudo
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                    return false;
                                }
                                uint8_t pseudoSize;
                                in >> pseudoSize;
                                if(pseudoSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                        return false;
                                    }
                                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                                    characterEntry.pseudo=std::string(rawText.data(),rawText.size());
                                    in.device()->seek(in.device()->pos()+rawText.size());
                                }
                            }
                            //Skin id
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return false;
                            }
                            in >> characterEntry.skinId;
                            //Time left before delete
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return false;
                            }
                            in >> characterEntry.delete_time_left;
                            //Played time
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return false;
                            }
                            in >> characterEntry.played_time;
                            //Last connect
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return false;
                            }
                            in >> characterEntry.last_connect;

                            characterEntryList << characterEntry;
                            characterListIndex++;
                        }

                        characterListForSelection << characterEntryList;
                        charatersGroupIndex++;
                    }
                }
                //servers
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint8_t serverListSize;
                    in >> serverListSize;
                    uint8_t serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        //Server index
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        uint8_t serverIndex;
                        in >> serverIndex;
                        //Played time
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        uint32_t playedTime;
                        in >> playedTime;
                        //Last connect
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        uint32_t lastConnect;
                        in >> lastConnect;
                        if(playedTime>0 && lastConnect==0)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("playedTime>0 && lastConnect==0 with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }

                        if(serverIndex<serverOrdenedList.size())
                        {
                            if(serverOrdenedList.at(serverIndex)!=NULL)
                            {
                                serverOrdenedList[serverIndex]->playedTime=playedTime;
                                serverOrdenedList[serverIndex]->lastConnect=lastConnect;
                            }
                        }
                        else
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("out of range with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }

                        serverListIndex++;
                    }
                }

                is_logged=true;
                logicialGroupIndexList.clear();
                logged(serverOrdenedList,characterListForSelection);
            }
        }
        break;
        //Account creation
        case 0xA9:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(packetCode).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Login already used");
                else if(returnCode==0x03)
                    string=tr("Not created");
                else
                    string=tr("Unknown error %1").arg(returnCode);
                std::cerr << "is not logged, reason: " << string.toStdString() << std::endl;
                notLogged(string);
                return false;
            }
            else
            {
                QByteArray outputData;
                //reemit the login try
                outputData+=loginHash;
                QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
                hashAndToken.addData(passHash+token);
                outputData+=hashAndToken.result();
                const uint8_t &query_number=Api_protocol::queryNumber();
                packOutcommingQuery(0xA8,query_number,outputData.constData(),outputData.size());
            }
        }
        break;

        //Get the character id return
        case 0xAA:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint32_t characterId;
            in >> characterId;
            std::cout << "created character with id: " << characterId << ", returnCode: " << returnCode << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
            newCharacterId(returnCode,characterId);
        }
        break;
        //Get the character id return
        case 0xAB:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            //just don't emited and used
        }
        break;
        //get the character selection return
        case 0xAC:
        //get the character selection return on game server
        case 0x93:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;

            if(selectedServerIndex==-1)
            {
                parseError(QStringLiteral("Internal error"),QStringLiteral("selectedServerIndex==-1 with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            const ServerFromPoolForDisplay &serverFromPoolForDisplay=*serverOrdenedList.at(selectedServerIndex);

            if(returnCode!=0x01 && (data.size()==1 || serverFromPoolForDisplay.host.isEmpty()))
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Character not found");
                else if(returnCode==0x03)
                    string=tr("Already logged");
                else if(returnCode==0x04)
                    string=tr("Server internal problem");
                else if(returnCode==0x05)
                    string=tr("Server not found");
                else if(returnCode==0x08)
                    string=tr("Too recently disconnected");
                else
                    string=tr("Unknown error: %1").arg(returnCode);
                std::cerr << "Selected character not found, reason: " << string.toStdString() << ", data: " << binarytoHexa(data.constData(),data.size()) << std::endl;
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                abort();//to debug
                #endif
                notLogged(string);
                return false;
            }
            else
            {
                if(stageConnexion==StageConnexion::Stage4 || serverFromPoolForDisplay.host.isEmpty())
                {
                    if(stageConnexion==StageConnexion::Stage4)
                        qDebug() << QStringLiteral("stageConnexion==StageConnexion::Stage4");
                    if(serverFromPoolForDisplay.host.isEmpty() && proxyMode==Api_protocol::ProxyMode::Reconnect)
                        qDebug() << QStringLiteral("serverFromPoolForDisplay.host.isEmpty()");
                    return parseCharacterBlock(
                                packetCode,queryNumber,
                                data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos()))
                                );
                }
                else
                {
                    tokenForGameServer=data;
                    qDebug() << QStringLiteral("new token to go on game server: %1").arg(QString(tokenForGameServer.toHex()));
                    if(tokenForGameServer.size()==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                    {
                        have_send_protocol=false;
                        stageConnexion=StageConnexion::Stage2;
                        if(socket!=NULL)
                            socket->disconnectFromHost();
                        connectingOnGameServer();
                        return true;
                    }
                    else
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),
                                   QStringLiteral("tokenForGameServer.size()!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER: %5/%6 with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4")
                                   .arg(packetCode)
                                   .arg('X')
                                   .arg(queryNumber)
                                   .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                   .arg(tokenForGameServer.size())
                                   .arg(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                                   );
                        return false;
                    }
                }
            }
        }
        break;
        //Clan action
        case 0x92:
        {
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        clanActionSuccess(0);
                    else
                    {
                        uint32_t clanId;
                        in >> clanId;
                        clanActionSuccess(clanId);
                    }
                break;
                case 0x02:
                    clanActionFailed();
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code at clan action: %1, line: %2").arg(returnCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                break;
            }
        }
        break;

        //Use seed into dirt
        case 0x83:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode==0x01)
                seed_planted(true);
            else if(returnCode==0x02)
                seed_planted(false);
            else
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code to use seed: %1").arg(returnCode));
                return false;
            }
        }
        break;
        //Collect mature plant
        case 0x84:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x03:
                case 0x04:
                    plant_collected((Plant_collect)returnCode);
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        //Usage of recipe
        case 0x85:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x03:
                    recipeUsed((RecipeUsage)returnCode);
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        //Use object
        case 0x86:
        {
            const uint16_t item=lastObjectUsed.first();
            lastObjectUsed.removeFirst();
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(CommonDatapack::commonDatapack.items.trap.find(item)!=CommonDatapack::commonDatapack.items.trap.cend())
                monsterCatch(returnCode==0x01);
            else
            {
                switch(returnCode)
                {
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        objectUsed((ObjectUsage)returnCode);
                    break;
                    default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
            }
        }
        break;
        //Get shop list
        case 0x87:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint16_t shopListSize;
            in >> shopListSize;
            uint32_t index=0;
            QList<ItemToSellOrBuy> items;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                items << item;
                index++;
            }
            haveShopList(items);
        }
        break;
        //Buy object
        case 0x88:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x04:
                    haveBuyObject((BuyStat)returnCode,0);
                break;
                case 0x03:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveBuyObject((BuyStat)returnCode,newPrice);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        //Sell object
        case 0x89:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x04:
                    haveSellObject((SoldStat)returnCode,0);
                break;
                case 0x03:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveSellObject((SoldStat)returnCode,newPrice);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        case 0x8A:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint32_t remainingProductionTime;
            in >> remainingProductionTime;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint16_t shopListSize;
            uint32_t index;
            in >> shopListSize;
            index=0;
            QList<ItemToSellOrBuy> resources;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                resources << item;
                index++;
            }
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> shopListSize;
            index=0;
            QList<ItemToSellOrBuy> products;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                products << item;
                index++;
            }
            haveFactoryList(remainingProductionTime,resources,products);
        }
        break;
        case 0x8B:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x03:
                case 0x04:
                    haveBuyFactoryObject((BuyStat)returnCode,0);
                break;
                case 0x02:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        case 0x8C:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                case 0x03:
                case 0x04:
                    haveSellFactoryObject((SoldStat)returnCode,0);
                break;
                case 0x02:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveSellFactoryObject((SoldStat)returnCode,newPrice);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        break;
        case 0x8D:
        {
            uint32_t listSize,index;
            QList<MarketObject> marketObjectList;
            QList<MarketMonster> marketMonsterList;
            QList<MarketObject> marketOwnObjectList;
            QList<MarketMonster> marketOwnMonsterList;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            quint64 cash;
            in >> cash;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.objectId;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.quantity;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                quint64 price;
                in >> price;
                marketObject.price=price;
                marketObjectList << marketObject;
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.monster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.level;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                quint64 price;
                in >> price;
                marketMonster.price=price;
                marketMonsterList << marketMonster;
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.objectId;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketObject.quantity;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                quint64 price;
                in >> price;
                marketObject.price=price;
                marketOwnObjectList << marketObject;
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.monster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> marketMonster.level;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                quint64 price;
                in >> price;
                marketMonster.price=price;
                marketOwnMonsterList << marketMonster;
                index++;
            }
            marketList(cash,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
        }
        break;
        case 0x8E:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                    if((in.device()->size()-in.device()->pos())==0)
                        marketBuy(true);
                break;
                case 0x02:
                case 0x03:
                    marketBuy(false);
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
            {
                PlayerMonster monster;
                PlayerBuff buff;
                PlayerMonster::PlayerSkill skill;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.id;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.monster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.level;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.remaining_xp;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.hp;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.sp;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.catched_with;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                uint8_t gender;
                in >> gender;
                switch(gender)
                {
                    case 0x01:
                    case 0x02:
                    case 0x03:
                        monster.gender=(Gender)gender;
                    break;
                    default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                        return false;
                    break;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> monster.egg_step;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                {
                    quint8 character_origin;
                    in >> character_origin;
                    monster.character_origin=character_origin;
                }

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                uint32_t sub_size,sub_index;
                in >> sub_size;
                sub_index=0;
                while(sub_index<sub_size)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> buff.buff;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> buff.level;
                    monster.buffs.push_back(buff);
                    sub_index++;
                }

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                in >> sub_size;
                sub_index=0;
                while(sub_index<sub_size)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> skill.skill;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> skill.level;
                    monster.skills.push_back(skill);
                    sub_index++;
                }
                marketBuyMonster(monster);
            }
        }
        break;
        case 0x8F:
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                    marketPut(true);
                break;
                case 0x02:
                    marketPut(false);
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        break;
        case 0x90:
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            quint64 cash;
            in >> cash;
            marketGetCash(cash);
        break;
        case 0x91:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            switch(returnCode)
            {
                case 0x01:
                break;
                case 0x02:
                    marketWithdrawCanceled();
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            if(returnCode==0x01)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                uint8_t returnType;
                in >> returnType;
                switch(returnType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return false;
                }
                if(returnType==0x01)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t objectId;
                    in >> objectId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(packetCode).arg('X').arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t quantity;
                    in >> quantity;
                    marketWithdrawObject(objectId,quantity);
                }
                else
                {
                    PlayerMonster monster;
                    PlayerBuff buff;
                    PlayerMonster::PlayerSkill skill;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.id;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.monster;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.level;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.remaining_xp;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.hp;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.sp;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.catched_with;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint8_t gender;
                    in >> gender;
                    switch(gender)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                            monster.gender=(Gender)gender;
                        break;
                        default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                            return false;
                        break;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> monster.egg_step;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    {
                        quint8 character_origin;
                        in >> character_origin;
                        monster.character_origin=character_origin;
                    }

                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    uint32_t sub_size,sub_index;
                    in >> sub_size;
                    sub_index=0;
                    while(sub_index<sub_size)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        in >> buff.buff;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        in >> buff.level;
                        monster.buffs.push_back(buff);
                        sub_index++;
                    }

                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return false;
                    }
                    in >> sub_size;
                    sub_index=0;
                    while(sub_index<sub_size)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        in >> skill.skill;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return false;
                        }
                        in >> skill.level;
                        monster.skills.push_back(skill);
                        sub_index++;
                    }
                    marketWithdrawMonster(monster);
                }
            }
        }
        break;

        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseReplyData(%1,%2), line: %3, data: %4 %5")
                   .arg(packetCode).arg(queryNumber)
                   .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return false;
    }
    return true;
}
