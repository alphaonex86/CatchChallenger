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

//send reply
void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    parseReplyData(mainCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0x03:
        {
            //Protocol initialization
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                return;
            }
            quint8 returnCode;
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
                    default:
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(queryNumber));
                    return;
                }
                if(data.size()!=(sizeof(quint8)+CATCHCHALLENGER_TOKENSIZE))
                {
                    newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(queryNumber));
                    return;
                }
                token=data.right(CATCHCHALLENGER_TOKENSIZE);
                have_receive_protocol=true;
                protocol_is_good();
                return;
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
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("the server have returned: %1").arg(string));
                disconnected(QStringLiteral("the server have returned: %1").arg(string));
                return;
            }
        }
        break;
        //Get first data and send the login
        case 0x04:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                return;
            }
            quint8 returnCode;
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
                    tryCreate();
                    return;
                }
                else
                    string=tr("Unknown error %1").arg(returnCode);
                DebugClass::debugConsole("is not logged, reason: "+string);
                notLogged(string);
                return;
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.character_delete_time;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_character;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the min_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.min_character;
                if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("max_character<min_character, line: %1").arg(__LINE__));
                    return;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerMonsters, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerMonsters, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerItems, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerItems, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(__LINE__));
                    return;
                }
                CommonSettingsCommon::commonSettingsCommon.datapackHashBase=data.mid(in.device()->pos(),28);
                in.device()->seek(in.device()->pos()+CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
                {
                    //the mirror
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mirrorSize;
                    in >> mirrorSize;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),mirrorSize);
                    CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                }
                //characters
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 charatersGroupSize;
                    in >> charatersGroupSize;
                    quint8 charatersGroupIndex=0;
                    while(charatersGroupIndex<charatersGroupSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        QList<CharacterEntry> characterEntryList;
                        quint8 characterListSize;
                        in >> characterListSize;
                        quint8 characterListIndex=0;
                        while(characterListIndex<characterListSize)
                        {
                            CharacterEntry characterEntry;
                            //characterId
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
                            }
                            in >> characterEntry.character_id;
                            {
                                //pseudo
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                    return;
                                }
                                quint8 pseudoSize;
                                in >> pseudoSize;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                    return;
                                }
                                QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                                characterEntry.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                                in.device()->seek(in.device()->pos()+rawText.size());
                            }
                            //Skin id
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
                            }
                            in >> characterEntry.skinId;
                            //Time left before delete
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
                            }
                            in >> characterEntry.delete_time_left;
                            //Played time
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
                            }
                            in >> characterEntry.played_time;
                            //Last connect
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
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
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 serverListSize;
                    in >> serverListSize;
                    quint8 serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        //Server index
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 serverIndex;
                        in >> serverIndex;
                        //Played time
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint32 playedTime;
                        in >> playedTime;
                        //Last connect
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint32 lastConnect;
                        in >> lastConnect;
                        if(playedTime>0 && lastConnect==0)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("playedTime>0 && lastConnect==0 with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("out of range with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }

                        serverListIndex++;
                    }
                }

                /*remake all this
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max player, line: %1").arg(__LINE__));
                    return;
                }
                in >> max_players;
                setMaxPlayers(max_players);

                quint32 captureRemainingTime;
                quint8 captureFrequencyType;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture remainingTime, line: %1").arg(__LINE__));
                    return;
                }
                in >> captureRemainingTime;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture type, line: %1").arg(__LINE__));
                    return;
                }
                in >> captureFrequencyType;
                switch(captureFrequencyType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong captureFrequencyType, line: %1").arg(__LINE__));
                    return;
                }
                cityCapture(captureRemainingTime,captureFrequencyType);

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
                {
                    quint8 tempForceClientToSendAtBorder;
                    in >> tempForceClientToSendAtBorder;
                    if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("forceClientToSendAtBorder have wrong value, line: %1").arg(__LINE__));
                        return;
                    }
                    CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.forcedSpeed;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the forcedSpeed, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.useSP;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the tcpCork, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.tcpCork;
                {
                    socket->setTcpCork(CommonSettingsServer::commonSettingsServer.tcpCork);
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.autoLearn;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.dontSendPseudo;


                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_xp, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.rates_xp;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.rates_gold;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.rates_xp_pow;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.rates_drop;

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.chat_allow_all;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_local, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.chat_allow_local;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_private, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.chat_allow_private;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_clan, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.chat_allow_clan;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the factoryPriceChange, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsServer::commonSettingsServer.factoryPriceChange;
                if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the httpDatapackMirror, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the number of map, line: %1").arg(__LINE__));
                    return;
                }
                /// \todo, change on login, why transmit it?
                in >> number_of_map;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the characterListSize, line: %1").arg(__LINE__));
                    return;
                }
                quint8 characterListSize;
                in >> characterListSize;

                QList<CharacterEntry> characterEntryList;
                int index=0;
                while(index<characterListSize)
                {
                    CharacterEntry characterEntry;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the character_id, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.character_id;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.pseudo;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.skinId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the delete_time_left, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.delete_time_left;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the played_time, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.played_time;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the last_connect, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.last_connect;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.mapId;
                    characterEntryList << characterEntry;
                    index++;
                }*/
                is_logged=true;
                logicialGroupIndexList.clear();
                logged(serverOrdenedList,characterListForSelection);
            }
        }
        break;
        //Account creation
        case 0x05:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                return;
            }
            quint8 returnCode;
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
                DebugClass::debugConsole("is not logged, reason: "+string);
                notLogged(string);
                return;
            }
            else
            {
                QByteArray outputData;
                //reemit the login try
                outputData+=loginHash;
                QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
                hashAndToken.addData(passHash+token);
                outputData+=hashAndToken.result();
                const quint8 &query_number=Api_protocol::queryNumber();
                packOutcommingQuery(0x04,query_number,outputData.constData(),outputData.size());
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseReplyData(%1,%2), line: %3, data: %4 %5")
                   .arg(mainCodeType).arg(queryNumber)
                   .arg(__LINE__)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void Api_protocol::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char * const data,const unsigned int &size)
{
    parseFullReplyData(mainCodeType,subCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0x02:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Get the character id return
                case 0x03:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 characterId;
                    in >> characterId;
                    newCharacterId(returnCode,characterId);
                }
                break;
                //Get the character id return
                case 0x04:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    //just don't emited and used
                }
                break;
                //get the character selection return
                case 0x05:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode!=0x01)
                    {
                        QString string;
                        if(returnCode==0x02)
                            string=tr("Character not found");
                        else if(returnCode==0x03)
                            string=tr("Already logged");
                        else if(returnCode==0x04)
                            string=tr("Server internal problem");
                        else
                            string=tr("Unknown error: %1").arg(returnCode);
                        DebugClass::debugConsole("Selected character not found, reason: "+string);
                        notLogged(string);
                        return;
                    }
                    else
                    {
                        if(max_players<=255)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 simplifiedId;
                            in >> simplifiedId;
                            player_informations.public_informations.simplifiedId=simplifiedId;
                        }
                        else
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> player_informations.public_informations.simplifiedId;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_informations.public_informations.pseudo;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 tempAllowSize;
                        in >> tempAllowSize;
                        {
                            int index=0;
                            while(index<tempAllowSize)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                    return;
                                }
                                quint8 tempAllow;
                                in >> tempAllow;
                                player_informations.allow << static_cast<ActionAllow>(tempAllow);
                                index++;
                            }
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.clan;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan leader, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 tempClanLeader;
                        in >> tempClanLeader;
                        if(tempClanLeader==0x01)
                            player_informations.clan_leader=true;
                        else
                            player_informations.clan_leader=false;
                        {
                            QList<QPair<quint8,quint8> > events;
                            quint8 tempListSize;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> tempListSize;
                            quint8 event,value;
                            int index=0;
                            while(index<tempListSize)
                            {

                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event id, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> event;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event value, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> value;
                                index++;
                                events << QPair<quint8,quint8>(event,value);
                            }
                            setEvents(events);
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.cash;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.warehouse_cash;

                        //item on map
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 itemOnMapSize;
                            in >> itemOnMapSize;
                            quint8 index=0;
                            while(index<itemOnMapSize)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(__LINE__));
                                    return;
                                }
                                quint8 itemOnMap;
                                in >> itemOnMap;
                                player_informations.itemOnMap << itemOnMap;
                                index++;
                            }
                        }

                        //recipes
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the recipe list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint16 recipe_list_size;
                        in >> recipe_list_size;
                        quint16 recipeId;
                        quint32 index=0;
                        while(index<recipe_list_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player local recipe, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> recipeId;
                            player_informations.recipes << recipeId;
                            index++;
                        }

                        //monsters
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 gender;
                        quint8 monster_list_size;
                        in >> monster_list_size;
                        index=0;
                        quint32 sub_index;
                        while(index<monster_list_size)
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.catched_with;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> gender;
                            switch(gender)
                            {
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                    monster.gender=(Gender)gender;
                                break;
                                default:
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.character_origin;

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 sub_size8;
                            in >> sub_size8;
                            sub_index=0;
                            while(sub_index<sub_size8)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint16 sub_size16;
                            in >> sub_size16;
                            sub_index=0;
                            while(sub_index<sub_size16)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.endurance;
                                monster.skills << skill;
                                sub_index++;
                            }
                            player_informations.playerMonster << monster;
                            index++;
                        }
                        //monsters
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster_list_size;
                        index=0;
                        while(index<monster_list_size)
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.catched_with;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> gender;
                            switch(gender)
                            {
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                    monster.gender=(Gender)gender;
                                break;
                                default:
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.character_origin;

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 sub_size8;
                            in >> sub_size8;
                            sub_index=0;
                            while(sub_index<sub_size8)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint16 sub_size16;
                            in >> sub_size16;
                            sub_index=0;
                            while(sub_index<sub_size16)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.endurance;
                                monster.skills << skill;
                                sub_index++;
                            }
                            player_informations.warehouse_playerMonster << monster;
                            index++;
                        }
                        //reputation
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerReputation playerReputation;
                        quint8 type;
                        index=0;
                        quint8 sub_size8;
                        in >> sub_size8;
                        while(index<sub_size8)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong reputation code: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> type;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.point;
                            player_informations.reputation[type]=playerReputation;
                            index++;
                        }
                        //quest
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerQuest playerQuest;
                        quint16 playerQuestId;
                        index=0;
                        quint16 sub_size16;
                        in >> sub_size16;
                        while(index<sub_size16)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> playerQuestId;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.finish_one_time;
                            if(playerQuest.step<=0 && !playerQuest.finish_one_time)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("can't be to step 0 if have never finish the quest, line: %1").arg(__LINE__));
                                return;
                            }
                            player_informations.quests[playerQuestId]=playerQuest;
                            index++;
                        }
                        //bot_already_beaten
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint16 bot_already_beaten;
                        index=0;
                        in >> sub_size16;
                        while(index<sub_size16)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> bot_already_beaten;
                            player_informations.bot_already_beaten << bot_already_beaten;
                            index++;
                        }
                        character_selected=true;
                        haveCharacter();
                    }
                }
                break;
                //Clan action
                case 0x0D:
                {
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                clanActionSuccess(0);
                            else
                            {
                                quint32 clanId;
                                in >> clanId;
                                clanActionSuccess(clanId);
                            }
                        break;
                        case 0x02:
                            clanActionFailed();
                        break;
                        default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code at clan action: %1, line: %2").arg(returnCode).arg(__LINE__));
                        break;
                    }
                }
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
                    return;
                break;
            }
        }
        break;
        case 0x10:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Use seed into dirt
                case 0x06:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                        seed_planted(true);
                    else if(returnCode==0x02)
                        seed_planted(false);
                    else
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code to use seed: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                //Collect mature plant
                case 0x07:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Usage of recipe
                case 0x08:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                            recipeUsed((RecipeUsage)returnCode);
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Use object
                case 0x09:
                {
                    quint16 item=lastObjectUsed.first();
                    lastObjectUsed.removeFirst();
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(CommonDatapack::commonDatapack.items.trap.contains(item))
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        quint32 newMonsterId;
                        in >> newMonsterId;
                        monsterCatch(newMonsterId);
                    }
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                    }
                }
                break;
                //Get shop list
                case 0x0A:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint16 shopListSize;
                    in >> shopListSize;
                    quint32 index=0;
                    QList<ItemToSellOrBuy> items;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
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
                case 0x0B:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Sell object
                case 0x0C:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x0D:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 remainingProductionTime;
                    in >> remainingProductionTime;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint16 shopListSize;
                    quint32 index;
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> resources;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        resources << item;
                        index++;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> products;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
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
                case 0x0E:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x0F:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellFactoryObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x10:
                {
                    quint32 listSize,index;
                    QList<MarketObject> marketObjectList;
                    QList<MarketMonster> marketMonsterList;
                    QList<MarketObject> marketOwnObjectList;
                    QList<MarketMonster> marketOwnMonsterList;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        marketObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        marketMonsterList << marketMonster;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        marketOwnObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        marketOwnMonsterList << marketMonster;
                        index++;
                    }
                    marketList(cash,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
                }
                break;
                case 0x11:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
                    {
                        PlayerMonster monster;
                        PlayerBuff buff;
                        PlayerMonster::PlayerSkill skill;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.remaining_xp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.hp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.sp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.catched_with;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 gender;
                        in >> gender;
                        switch(gender)
                        {
                            case 0x01:
                            case 0x02:
                            case 0x03:
                                monster.gender=(Gender)gender;
                            break;
                            default:
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                return;
                            break;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.egg_step;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.character_origin;

                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                            return;
                        }
                        quint32 sub_size,sub_index;
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> buff.buff;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> buff.level;
                            monster.buffs << buff;
                            sub_index++;
                        }

                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> skill.skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> skill.level;
                            monster.skills << skill;
                            sub_index++;
                        }
                        marketBuyMonster(monster);
                    }
                }
                break;
                case 0x12:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                break;
                case 0x13:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    marketGetCash(cash);
                break;
                case 0x14:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        break;
                        case 0x02:
                            marketWithdrawCanceled();
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        quint8 returnType;
                        in >> returnType;
                        switch(returnType)
                        {
                            case 0x01:
                            case 0x02:
                            break;
                            default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                            return;
                        }
                        if(returnType==0x01)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 objectId;
                            in >> objectId;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            marketWithdrawObject(objectId,quantity);
                        }
                        else
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.catched_with;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 gender;
                            in >> gender;
                            switch(gender)
                            {
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                    monster.gender=(Gender)gender;
                                break;
                                default:
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.character_origin;

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint32 sub_size,sub_index;
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                monster.skills << skill;
                                sub_index++;
                            }
                            marketWithdrawMonster(monster);
                        }
                    }
                }
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
                    return;
                break;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident reply code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseFullReplyData(%1,%2,%3), line: %4, data: %5 %6")
                   .arg(mainCodeType).arg(subCodeType).arg(queryNumber)
                   .arg(__LINE__)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}
