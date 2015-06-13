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
#include <QRegularExpression>

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
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                    return;
                }
                if(stageConnexion==StageConnexion::Stage1)
                {
                    if(data.size()!=(sizeof(quint8)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                    {
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong size (stage 1) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    token=data.right(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    have_receive_protocol=true;
                    protocol_is_good();
                }
                else if(stageConnexion==StageConnexion::Stage3)
                {
                    if(data.size()!=(sizeof(quint8)))
                    {
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong size (stage 3) with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    have_receive_protocol=true;
                    //send token to game server
                    packFullOutcommingQuery(0x02,0x06,this->queryNumber(),tokenForGameServer.constData(),tokenForGameServer.size());
                }
                else
                {
                    newError(QStringLiteral("Internal problem"),QStringLiteral("stageConnexion!=StageConnexion::Stage1/3"));
                    return;
                }
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
                //newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("the server have returned: %1").arg(string));
                //show the message box
                disconnected(QStringLiteral("the server have returned: %1").arg(string));
                closeSocket();
                return;
            }
        }
        break;
        //Get first data and send the login
        case 0x04:
        {
            if(!haveTheServerList)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("don't have server list before this reply main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            if(!haveTheLogicalGroupList)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("don't have logical group list before this reply main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.character_delete_time;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_character;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the min_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.min_character;
                if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("max_character<min_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerMonsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerMonsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxPlayerItems, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the maxWarehousePlayerItems, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                }
                qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __LINE__;
                CommonSettingsCommon::commonSettingsCommon.datapackHashBase=data.mid(in.device()->pos(),28);
                in.device()->seek(in.device()->pos()+CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
                qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                {
                    //the mirror
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 mirrorSize;
                    in >> mirrorSize;
                    if(mirrorSize>0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),mirrorSize);
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                    }
                    else
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.clear();
                }
                qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                //characters
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 charatersGroupSize;
                    in >> charatersGroupSize;
                    qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                    quint8 charatersGroupIndex=0;
                    while(charatersGroupIndex<charatersGroupSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        QList<CharacterEntry> characterEntryList;
                        quint8 characterListSize;
                        in >> characterListSize;
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                        quint8 characterListIndex=0;
                        while(characterListIndex<characterListSize)
                        {
                            CharacterEntry characterEntry;
                            //characterId
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> characterEntry.character_id;
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                            {
                                //pseudo
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                    return;
                                }
                                quint8 pseudoSize;
                                in >> pseudoSize;
                                if(pseudoSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                        return;
                                    }
                                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                                    characterEntry.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                                    in.device()->seek(in.device()->pos()+rawText.size());
                                }
                            }
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                            //Skin id
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> characterEntry.skinId;
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                            //Time left before delete
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> characterEntry.delete_time_left;
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                            //Played time
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> characterEntry.played_time;
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                            //Last connect
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> characterEntry.last_connect;
                            qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;

                            characterEntryList << characterEntry;
                            characterListIndex++;
                        }

                        characterListForSelection << characterEntryList;
                        charatersGroupIndex++;
                    }
                }
                qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                //servers
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 serverListSize;
                    in >> serverListSize;
                    quint8 serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                        //Server index
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        quint8 serverIndex;
                        in >> serverIndex;
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                        //Played time
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        quint32 playedTime;
                        in >> playedTime;
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                        //Last connect
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        quint32 lastConnect;
                        in >> lastConnect;
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;
                        if(playedTime>0 && lastConnect==0)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("playedTime>0 && lastConnect==0 with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("out of range with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        qDebug() << QString(data.mid(0,in.device()->pos()).toHex()) << QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()) << __FILE__ << __LINE__;

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
        case 0x05:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseReplyData(%1,%2), line: %3, data: %4 %5")
                   .arg(mainCodeType).arg(queryNumber)
                   .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    //just don't emited and used
                }
                break;
                //get the character selection return
                case 0x05:
                //get the character selection return on game server
                case 0x06:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;

                    if(selectedServerIndex==-1)
                    {
                        parseError(QStringLiteral("Internal error"),QStringLiteral("selectedServerIndex==-1 with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
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
                        else
                            string=tr("Unknown error: %1").arg(returnCode);
                        DebugClass::debugConsole("Selected character not found, reason: "+string);
                        notLogged(string);
                        return;
                    }
                    else
                    {
                        if(stageConnexion==StageConnexion::Stage3 || serverFromPoolForDisplay.host.isEmpty())
                        {
                            parseCharacterBlock(
                                        mainCodeType,subCodeType,queryNumber,
                                        data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos()))
                                        );
                            return;
                        }
                        else
                        {
                            tokenForGameServer=data;
                            qDebug() << QStringLiteral("new token to go on game server: %1").arg(QString(tokenForGameServer.toHex()));
                            if(tokenForGameServer.size()==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                            {
                                connectingOnGameServer();
                                have_send_protocol=false;
                                stageConnexion=StageConnexion::Stage2;
                                if(socket!=NULL)
                                    socket->disconnectFromHost();
                                return;
                            }
                            else
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),
                                           QStringLiteral("tokenForGameServer.size()!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER: %5/%6 with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4")
                                           .arg(mainCodeType)
                                           .arg(subCodeType)
                                           .arg(queryNumber)
                                           .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                                           .arg(tokenForGameServer.size())
                                           .arg(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                                           );
                                return;
                            }
                        }
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code at clan action: %1, line: %2").arg(returnCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        break;
                    }
                }
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                }
                break;
                //Usage of recipe
                case 0x08:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(CommonDatapack::commonDatapack.items.trap.contains(item))
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                }
                break;
                //Sell object
                case 0x0C:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                }
                break;
                case 0x0D:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint32 remainingProductionTime;
                    in >> remainingProductionTime;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> products;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                }
                break;
                case 0x0F:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellFactoryObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
                    {
                        PlayerMonster monster;
                        PlayerBuff buff;
                        PlayerMonster::PlayerSkill skill;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.remaining_xp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.hp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.sp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.catched_with;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                                return;
                            break;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.egg_step;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> monster.character_origin;

                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        quint32 sub_size,sub_index;
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> buff.buff;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> buff.level;
                            monster.buffs << buff;
                            sub_index++;
                        }

                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> skill.skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                break;
                case 0x13:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                        return;
                    }
                    if(returnCode==0x01)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                            return;
                        }
                        if(returnType==0x01)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 objectId;
                            in >> objectId;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.id;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.monster;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.hp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.catched_with;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                                    return;
                                break;
                            }
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.egg_step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> monster.character_origin;

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            quint32 sub_size,sub_index;
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                    return;
                                }
                                in >> buff.buff;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                                    return;
                                }
                                in >> skill.skill;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                    return;
                break;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident reply code: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseFullReplyData(%1,%2,%3), line: %4, data: %5 %6")
                   .arg(mainCodeType).arg(subCodeType).arg(queryNumber)
                   .arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__))
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}
