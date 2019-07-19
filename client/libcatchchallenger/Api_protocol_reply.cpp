#include "Api_protocol.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif
#include <iostream>
#include <string>

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"

#include <QCoreApplication>
#include <QRegularExpression>
#include <QDataStream>

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//send reply
bool Api_protocol::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    const bool &returnValue=parseReplyData(packetCode,queryNumber,std::string(data,size));
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!returnValue)
    {
        errorParsingLayer("Api_protocol::parseReplyData(): return false (abort), need be aborted before");
        abort();
    }
    #endif
    return returnValue;
}

bool Api_protocol::parseReplyData(const uint8_t &packetCode,const uint8_t &queryNumber,const std::string &data)
{
    if(querySendTime.find(queryNumber)!=querySendTime.cend())
    {
        std::time_t result = std::time(nullptr);
        if((uint64_t)result>querySendTime.at(queryNumber))
            lastReplyTime(result-querySendTime.at(queryNumber));
        querySendTime.erase(queryNumber);
    }
    if(vectorcontainsAtLeastOne(lastQueryNumber,queryNumber))
    {
        errorParsingLayer("Api_protocol::parseReplyData(): queryNumber: allready returned: "+std::to_string(queryNumber));
        abort();
    }
    else
        lastQueryNumber.push_back(queryNumber);
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(packetCode)
    {
        case 0xA0:
        {
            //Protocol initialization
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode==0x04 || returnCode==0x08)
            {
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x08:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zstandard;
                    break;
                    default:
                        newError("Procotol wrong or corrupted","compression type wrong with main ident: "+std::to_string(packetCode)+
                                 " and queryNumber: "+std::to_string(queryNumber)+", type: query_type_protocol");
                    return false;
                }
                if(stageConnexion==StageConnexion::Stage1)
                {
                    if(data.size()!=(sizeof(uint8_t)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                    {
                        newError("Procotol wrong or corrupted","compression type wrong size (stage 1) with main ident: "+
                                 std::to_string(packetCode)+" and queryNumber: "+std::to_string(queryNumber)+
                                 ", type: query_type_protocol"
                                 );
                        return false;
                    }
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).right(TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                        token=std::string(tdata.data(),tdata.size());
                    }
                    have_receive_protocol=true;
                    protocol_is_good();
                    //std::cout << "Api_protocol::protocol_is_good(), stageConnexion==StageConnexion::Stage1" << std::endl;
                }
                else if(stageConnexion==StageConnexion::Stage4)
                {
                    if(data.size()!=(sizeof(uint8_t)))
                    {
                        newError("Procotol wrong or corrupted","compression type wrong size (stage 3) with main ident: "+
                                 std::to_string(packetCode)+" and queryNumber: "+std::to_string(queryNumber)+
                                 ", type: query_type_protocol"
                                 );
                        return false;
                    }
                    have_receive_protocol=true;
                    //send token to game server
                    packOutcommingQuery(0x93,this->queryNumber(),tokenForGameServer.data(),tokenForGameServer.size());
                }
                else
                {
                    newError(std::string("Internal problem"),std::string("stageConnexion!=StageConnexion::Stage1/3"));
                    return false;
                }
                return true;
            }
            else
            {
                std::string string;
                if(returnCode==0x02)
                    string=tr("Protocol not supported").toStdString();
                else if(returnCode==0x03)
                    string=tr("Server full").toStdString();
                else
                    string=tr("Unknown error %1").arg(returnCode).toStdString();
                //newError("Procotol wrong or corrupted",std::string("the server have returned: %1").arg(string));
                //show the message box
                disconnected("the server have returned: "+string);
                std::cerr << string << std::endl;
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                std::string string;
                if(returnCode==0x02)
                    string=tr("Bad login").toStdString();
                else if(returnCode==0x03)
                    string=tr("Wrong login/pass").toStdString();
                else if(returnCode==0x04)
                    string=tr("Server internal error").toStdString();
                else if(returnCode==0x05)
                    string=tr("Can't create character and don't have character").toStdString();
                else if(returnCode==0x06)
                    string=tr("Login already in progress").toStdString();
                else if(returnCode==0x07)
                {
                    tryCreateAccount();
                    return true;
                }
                else
                    string=tr("Unknown error %1").arg(returnCode).toStdString();
                std::cerr << "is not logged, reason: " << string << std::endl;
                notLogged(string);
                return false;
            }
            else
            {
                //messageParsingLayer("Dump to debug login reply: "+binarytoHexa(data.constData(),data.size()));
                if(!haveTheServerList)
                {
                    parseError("Procotol wrong or corrupted","don't have server list before this reply main ident: "+
                               std::to_string(packetCode)+" and queryNumber: "+
                               std::to_string(queryNumber)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
                    return false;
                }
                if(!haveTheLogicalGroupList)
                {
                    parseError("Procotol wrong or corrupted","don't have logical group list before this reply main ident: "+
                               std::to_string(packetCode)+" and queryNumber: "+
                               std::to_string(queryNumber)+", line: "+
                               std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
                    return false;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_pseudo_size, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.character_delete_time;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_character;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the min_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.min_character;
                if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
                {
                    parseError("Procotol wrong or corrupted","max_character<min_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_pseudo_size, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxPlayerMonsters, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxWarehousePlayerMonsters, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxPlayerItems, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxWarehousePlayerItems, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                in >> CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the datapack checksum, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(28);
                memcpy(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),28).constData(),28);
                in.device()->seek(in.device()->pos()+CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
                {
                    //the mirror
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint8_t mirrorSize;
                    in >> mirrorSize;
                    if(mirrorSize>0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),mirrorSize);
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=std::string(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                        if(!regex_search(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase,std::regex("^https?://")))
                        {
                            parseError("Procotol wrong or corrupted","mirror with not http(s) protocol with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
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
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint8_t charatersGroupSize;
                    in >> charatersGroupSize;
                    uint8_t charatersGroupIndex=0;
                    while(charatersGroupIndex<charatersGroupSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        std::vector<CharacterEntry> characterEntryList;
                        uint8_t characterListSize;
                        in >> characterListSize;
                        uint8_t characterListIndex=0;
                        /*if(characterListSize>0)
                            messageParsingLayer("For the character group "+std::to_string(charatersGroupIndex)+" you have "+std::to_string(characterListSize)+" character:");
                        else
                            messageParsingLayer("For the character group "+std::to_string(charatersGroupIndex)+" you don't have character");*/
                        while(characterListIndex<characterListSize)
                        {
                            CharacterEntry characterEntry;
                            //characterId
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            in >> characterEntry.character_id;
                            {
                                //pseudo
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                                {
                                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                                  );
                                    return false;
                                }
                                uint8_t pseudoSize;
                                in >> pseudoSize;
                                if(pseudoSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                                    {
                                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                                      );
                                        return false;
                                    }
                                    QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),pseudoSize);
                                    characterEntry.pseudo=std::string(rawText.data(),rawText.size());
                                    in.device()->seek(in.device()->pos()+rawText.size());
                                }
                            }
                            //Skin id
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            in >> characterEntry.skinId;
                            //Time left before delete
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            in >> characterEntry.delete_time_left;
                            //Played time
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            in >> characterEntry.played_time;
                            //Last connect
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                            {
                                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            uint32_t last_connect32;
                            in >> last_connect32;
                            characterEntry.last_connect=last_connect32;

                            //important
                            characterEntry.charactersGroupIndex=charatersGroupIndex;

                            //messageParsingLayer("- "+characterEntry.pseudo+" ("+std::to_string(characterEntry.character_id)+")");

                            characterEntryList.push_back(characterEntry);
                            characterListIndex++;
                        }

                        characterListForSelection.push_back(characterEntryList);
                        charatersGroupIndex++;
                    }
                }
                //servers
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
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
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t serverIndex;
                        in >> serverIndex;
                        //Played time
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint32_t playedTime;
                        in >> playedTime;
                        //Last connect
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
                        {
                            QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint32_t lastConnect;
                        in >> lastConnect;
                        if(playedTime>0 && lastConnect==0)
                        {
                            parseError("Procotol wrong or corrupted","playedTime>0 && lastConnect==0 with main ident: "+std::to_string(packetCode)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                            return false;
                        }

                        if(serverIndex<serverOrdenedList.size())
                        {
                            serverOrdenedList[serverIndex].playedTime=playedTime;
                            serverOrdenedList[serverIndex].lastConnect=lastConnect;
                        }
                        else
                        {
                            std::cerr << "dump data: " << binarytoHexa(data.data(),data.size()) << std::endl;
                            parseError("Procotol wrong or corrupted",
                                       "out of range serverIndex("+
                                       std::to_string(serverIndex)+
                                       ")>=serverOrdenedList.size("+
                                       std::to_string(serverOrdenedList.size())+
                                       ") with main ident: "+std::to_string(packetCode)+
                                        ", line: "+
                                        std::string(__FILE__)+":"+std::to_string(__LINE__)
                                       );
                            return false;
                        }

                        serverListIndex++;
                    }
                }

                is_logged=true;
                logicialGroupIndexList.clear();
                logged(characterListForSelection);
            }
        }
        break;
        //Account creation
        case 0xA9:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                std::string string;
                if(returnCode==0x02)
                    string=tr("Login already used").toStdString();
                else if(returnCode==0x03)
                    string=tr("Not created").toStdString();
                else
                    string=tr("Unknown error %1").arg(returnCode).toStdString();
                std::cerr << "is not logged, reason: " << string << std::endl;
                notLogged(string);
                return false;
            }
            else
            {
                QByteArray outputData;
                //reemit the login try
                outputData+=QByteArray(loginHash.data(),loginHash.size());
                QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
                hashAndToken.addData(QByteArray(passHash.data(),passHash.size())+
                                     QByteArray(token.data(),token.size()));
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint32_t characterId;//include if null, drop the dynamic size overhead
            in >> characterId;
            std::cout << "created character with id: " << characterId << ", returnCode: " << std::to_string(returnCode) << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
            newCharacterId(returnCode,characterId);
        }
        break;
        //Get the character id return
        case 0xAB:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode;
            in >> returnCode;

            if(selectedServerIndex==-1)
            {
                parseError("Internal error","selectedServerIndex==-1 with main ident: "+
                           std::to_string(packetCode)+", and queryNumber: "+
                           std::to_string(queryNumber)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            const ServerFromPoolForDisplay &serverFromPoolForDisplay=serverOrdenedList.at(selectedServerIndex);

            if(returnCode!=0x01 && (data.size()==1 || serverFromPoolForDisplay.host.empty()))
            {
                std::string string;
                if(returnCode==0x02)
                    string=tr("Character not found").toStdString();
                else if(returnCode==0x03)
                    string=tr("Already logged").toStdString();
                else if(returnCode==0x04)
                    string=tr("Server internal problem").toStdString();
                else if(returnCode==0x05)
                    string=tr("Server not found").toStdString();
                else if(returnCode==0x08)
                    string=tr("Too recently disconnected").toStdString();
                else
                    string=tr("Unknown error: %1").arg(returnCode).toStdString();
                std::cerr << "Selected character not found, reason: " << string << ", data: " << binarytoHexa(data.data(),data.size()) << std::endl;
                /*#ifdef CATCHCHALLENGER_EXTRA_CHECK
                abort();//to debug
                #endif*/
                notLogged(string);
                return false;
            }
            else
            {
                if(stageConnexion==StageConnexion::Stage4 || serverFromPoolForDisplay.host.empty())
                {
                    /*if(stageConnexion==StageConnexion::Stage4)
                        qDebug() << "stageConnexion==StageConnexion::Stage4";
                    qDebug() << "stageConnexion==StageConnexion::Stage: " << (int)stageConnexion;
                    qDebug() << "serverFromPoolForDisplay.host: " << QString::fromStdString(serverFromPoolForDisplay.host);*/
                    if(serverFromPoolForDisplay.host.empty() && proxyMode==Api_protocol::ProxyMode::Reconnect)
                        qDebug() << "serverFromPoolForDisplay.host.isEmpty() and reconnect, seam be a bug";
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(
                                static_cast<int>(in.device()->pos()),
                                static_cast<int>(in.device()->size()-in.device()->pos())
                                );
                    return parseCharacterBlockServer(
                                packetCode,queryNumber,
                                std::string(tdata.data(),tdata.size())
                                );
                }
                else
                {
                    tokenForGameServer=data;
                    qDebug() << QString("new token to go on game server: %1")
                                .arg(QString(QByteArray(tokenForGameServer.data(),tokenForGameServer.size()).toHex()));
                    if(tokenForGameServer.size()==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                    {
                        have_send_protocol=false;
                        message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage2 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        stageConnexion=StageConnexion::Stage2;
                        if(socket!=NULL)
                            socket->disconnectFromHost();
                        connectingOnGameServer();
                        return true;
                    }
                    else
                    {
                        parseError("Procotol wrong or corrupted",
                                   "tokenForGameServer.size()!=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER: "+
                                   std::to_string(tokenForGameServer.size())+"/"+
                                   std::to_string(CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)+
                                   " with main ident: "+std::to_string(packetCode)+
                                   ", and queryNumber: "+std::to_string(queryNumber)+
                                   ", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
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
                    parseError("Procotol wrong or corrupted","bad return code at clan action: "+
                               std::to_string(returnCode)+", line: "+std::string(__FILE__)+
                               ":"+std::to_string(__LINE__)
                               );
                break;
            }
        }
        break;

        //Use seed into dirt
        case 0x83:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","bad return code to use seed: "+std::to_string(returnCode));
                return false;
            }
        }
        break;
        //Collect mature plant
        case 0x84:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+
                           std::to_string(packetCode)+", and queryNumber: "+
                           std::to_string(queryNumber)+", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        //Usage of recipe
        case 0x85:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+
                           std::to_string(packetCode)+", and queryNumber: "+
                           std::to_string(queryNumber)+", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        //Use object
        case 0x86:
        {
            const uint16_t item=lastObjectUsed.front();
            lastObjectUsed.erase(lastObjectUsed.cbegin());
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                    parseError("Procotol wrong or corrupted","unknown return code with main ident: "+
                               std::to_string(packetCode)+", and queryNumber: "+
                               std::to_string(queryNumber)+", line: "+
                               std::string(__FILE__)+":"+std::to_string(__LINE__)
                               );
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint16_t shopListSize;
            in >> shopListSize;
            uint32_t index=0;
            std::vector<ItemToSellOrBuy> items;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                items.push_back(item);
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveBuyObject((BuyStat)returnCode,newPrice);
                }
                break;
                default:
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+std::to_string(packetCode)+
                           ", and queryNumber: "+std::to_string(queryNumber)+
                           ", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        //Sell object
        case 0x89:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveSellObject((SoldStat)returnCode,newPrice);
                }
                break;
                default:
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+std::to_string(packetCode)+
                           ", and queryNumber: "+std::to_string(queryNumber)+
                           ", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        case 0x8A:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint32_t remainingProductionTime;
            in >> remainingProductionTime;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint16_t shopListSize;
            uint32_t index;
            in >> shopListSize;
            index=0;
            std::vector<ItemToSellOrBuy> resources;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                resources.push_back(item);
                index++;
            }
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            in >> shopListSize;
            index=0;
            std::vector<ItemToSellOrBuy> products;
            while(index<shopListSize)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                in >> item.object;
                in >> item.price;
                in >> item.quantity;
                products.push_back(item);
                index++;
            }
            haveFactoryList(remainingProductionTime,resources,products);
        }
        break;
        case 0x8B:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                }
                break;
                default:
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+std::to_string(packetCode)+
                           ", and queryNumber: "+std::to_string(queryNumber)+
                           ", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        case 0x8C:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice;
                    in >> newPrice;
                    haveSellFactoryObject((SoldStat)returnCode,newPrice);
                }
                break;
                default:
                parseError("Procotol wrong or corrupted","unknown return code with main ident: "+std::to_string(packetCode)+
                           ", and queryNumber: "+std::to_string(queryNumber)+
                           ", line: "+
                           std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
        }
        break;
        case 0x8D:
        {
            uint32_t listSize,index;
            std::vector<MarketObject> marketObjectList;
            std::vector<MarketMonster> marketMonsterList;
            std::vector<MarketObject> marketOwnObjectList;
            std::vector<MarketMonster> marketOwnMonsterList;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            quint64 cash;
            in >> cash;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.marketObjectUniqueId;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.item;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.quantity;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quint64 price;
                in >> price;
                marketObject.price=price;
                marketObjectList.push_back(marketObject);
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.monster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.level;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quint64 price;
                in >> price;
                marketMonster.price=price;
                marketMonsterList.push_back(marketMonster);
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.marketObjectUniqueId;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.item;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketObject.quantity;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quint64 price;
                in >> price;
                marketObject.price=price;
                marketOwnObjectList.push_back(marketObject);
                index++;
            }
            in >> listSize;
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.index;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint16_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.monster;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                in >> marketMonster.level;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                quint64 price;
                in >> price;
                marketMonster.price=price;
                marketOwnMonsterList.push_back(marketMonster);
                index++;
            }
            marketList(cash,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
        }
        break;
        case 0x8E:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","wrong return code, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
            {
                PlayerMonster monster;
                if(!dataToPlayerMonster(in,monster))
                    return false;
                marketBuyMonster(monster);
            }
        }
        break;
        case 0x8F:
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","wrong return code, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
        break;
        case 0x90:
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint64_t)))
            {
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
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
                parseError("Procotol wrong or corrupted","wrong return code, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            if(returnCode==0x01)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint8_t)))
                {
                    QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
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
                    parseError("Procotol wrong or corrupted","wrong return code, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(returnType==0x01)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t objectId;
                    in >> objectId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(uint32_t)))
                    {
                        QByteArray tdata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()));
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(tdata.data(),tdata.size())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t quantity;
                    in >> quantity;
                    marketWithdrawObject(objectId,quantity);
                }
                else
                {
                    PlayerMonster monster;
                    if(!dataToPlayerMonster(in,monster))
                        return false;
                    marketWithdrawMonster(monster);
                }
            }
        }
        break;

        default:
            parseError("Procotol wrong or corrupted","unknown sort ident reply code: %1, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        QByteArray fdata=QByteArray(data.data(),data.size()).mid(0,static_cast<int>(in.device()->pos()));
        QByteArray ldata=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),static_cast<int>(in.device()->size()-in.device()->pos()));
        parseError("Procotol wrong or corrupted","error: remaining data: parseReplyData("+std::to_string(packetCode)+
                   ","+std::to_string(queryNumber)+
                   "), line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                   ","+binarytoHexa(fdata.data(),fdata.size())+
                   " "+binarytoHexa(ldata.data(),ldata.size())
                   );
        return false;
    }
    return true;
}
