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

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//send reply
bool Api_protocol::parseReplyData(const uint8_t &packetCode, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    int pos=0;
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
    switch(packetCode)
    {
        case 0xA0:
        {
            //Protocol initialization
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
                    if(size!=(sizeof(uint8_t)+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                    {
                        newError("Procotol wrong or corrupted","compression type wrong size (stage 1) with main ident: "+
                                 std::to_string(packetCode)+" and queryNumber: "+std::to_string(queryNumber)+
                                 ", type: query_type_protocol"
                                 );
                        return false;
                    }
                    token=std::string(data+sizeof(uint8_t),TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    have_receive_protocol=true;
                    protocol_is_good();
                    if(!login.empty())
                        tryLogin(login,pass);
                    //std::cout << "Api_protocol::protocol_is_good(), stageConnexion==StageConnexion::Stage1" << std::endl;
                }
                else if(stageConnexion==StageConnexion::Stage4)
                {
                    if(size!=(sizeof(uint8_t)))
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
                    string=std::string("Protocol not supported");
                else if(returnCode==0x03)
                    string=std::string("Server full");
                else
                    string=std::string("Unknown error ")+std::to_string(returnCode);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            if(returnCode!=0x01)
            {
                std::string string;
                if(returnCode==0x02)
                    string=std::string("Bad login");
                else if(returnCode==0x03)
                    string=std::string("Wrong login/pass");
                else if(returnCode==0x04)
                    string=std::string("Server internal error");
                else if(returnCode==0x05)
                    string=std::string("Can't create character and don't have character");
                else if(returnCode==0x06)
                    string=std::string("Login already in progress");
                else if(returnCode==0x07)
                {
                    tryCreateAccount();
                    return true;
                }
                else
                    string=std::string("Unknown error ")+std::to_string(returnCode);
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
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_pseudo_size, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.character_delete_time=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.max_character=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the min_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.min_character=data[pos];
                pos+=sizeof(uint8_t);
                if(CommonSettingsCommon::commonSettingsCommon.max_character<CommonSettingsCommon::commonSettingsCommon.min_character)
                {
                    parseError("Procotol wrong or corrupted","max_character<min_character, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the max_pseudo_size, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.max_pseudo_size=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxPlayerMonsters, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxWarehousePlayerMonsters, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxPlayerItems, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.maxPlayerItems=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)sizeof(uint16_t))
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the maxWarehousePlayerItems, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<CATCHCHALLENGER_SHA224HASH_SIZE)
                {
                    parseError("Procotol wrong or corrupted","wrong size to get the datapack checksum, line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_SHA224HASH_SIZE);
                memcpy(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),data+pos,CATCHCHALLENGER_SHA224HASH_SIZE);
                pos+=CATCHCHALLENGER_SHA224HASH_SIZE;
                {
                    //the mirror
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint8_t mirrorSize=data[pos];
                    pos+=sizeof(uint8_t);
                    if(mirrorSize>0)
                    {
                        if((size-pos)<(int)mirrorSize)
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase=std::string(data+pos,mirrorSize);
                        pos+=mirrorSize;
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
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint8_t charatersGroupSize=data[pos];
                    pos+=sizeof(uint8_t);
                    uint8_t charatersGroupIndex=0;
                    while(charatersGroupIndex<charatersGroupSize)
                    {
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        std::vector<CharacterEntry> characterEntryList;
                        uint8_t characterListSize=data[pos];
                        pos+=sizeof(uint8_t);
                        uint8_t characterListIndex=0;
                        /*if(characterListSize>0)
                            messageParsingLayer("For the character group "+std::to_string(charatersGroupIndex)+" you have "+std::to_string(characterListSize)+" character:");
                        else
                            messageParsingLayer("For the character group "+std::to_string(charatersGroupIndex)+" you don't have character");*/
                        while(characterListIndex<characterListSize)
                        {
                            CharacterEntry characterEntry;
                            //characterId
                            if((size-pos)<(int)sizeof(uint32_t))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            characterEntry.character_id=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
                            {
                                //pseudo
                                if((size-pos)<(int)sizeof(uint8_t))
                                {
                                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                                  );
                                    return false;
                                }
                                uint8_t pseudoSize=data[pos];
                                pos+=sizeof(uint8_t);
                                if(pseudoSize>0)
                                {
                                    if((size-pos)<(int)pseudoSize)
                                    {
                                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                                      );
                                        return false;
                                    }
                                    characterEntry.pseudo=std::string(data+pos,pseudoSize);
                                    pos+=pseudoSize;
                                }
                            }
                            //Skin id
                            if((size-pos)<(int)sizeof(uint8_t))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            characterEntry.skinId=data[pos];
                            pos+=sizeof(uint8_t);
                            //Time left before delete
                            if((size-pos)<(int)sizeof(uint32_t))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            characterEntry.delete_time_left=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
                            //Played time
                            if((size-pos)<(int)sizeof(uint32_t))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            characterEntry.played_time=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
                            //Last connect
                            if((size-pos)<(int)sizeof(uint32_t))
                            {
                                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                              );
                                return false;
                            }
                            uint32_t last_connect32=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                            pos+=sizeof(uint32_t);
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
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint8_t serverListSize=data[pos];
                    pos+=sizeof(uint8_t);
                    uint8_t serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        //Server index
                        if((size-pos)<(int)sizeof(uint8_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint8_t serverIndex=data[pos];
                        pos+=sizeof(uint8_t);
                        //Played time
                        if((size-pos)<(int)sizeof(uint32_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint32_t playedTime=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
                        //Last connect
                        if((size-pos)<(int)sizeof(uint32_t))
                        {
                            parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                       ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                          );
                            return false;
                        }
                        uint32_t lastConnect=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
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
                            std::cerr << "dump data: " << binarytoHexa(data,size) << std::endl;
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
                logged(/*serverOrdenedList,*/characterListForSelection);
            }
        }
        break;
        //Account creation
        case 0xA9:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            if(returnCode!=0x01)
            {
                std::string string;
                if(returnCode==0x02)
                    string=std::string("Login already used");
                else if(returnCode==0x03)
                    string=std::string("Not created");
                else
                    string=std::string("Unknown error ")+std::to_string(returnCode);
                std::cerr << "is not logged, reason: " << string << std::endl;
                notLogged(string);
                return false;
            }
            else
            {
                char outputData[loginHash.size()+CATCHCHALLENGER_SHA224HASH_SIZE];
                char tempdata[passHash.size()+token.size()];
                memcpy(outputData,loginHash.data(),loginHash.size());
                memcpy(tempdata,passHash.data(),passHash.size());
                memcpy(tempdata+passHash.size(),token.data(),token.size());
                hashSha224(tempdata,sizeof(tempdata),outputData+loginHash.size());
                const uint8_t &query_number=Api_protocol::queryNumber();
                packOutcommingQuery(0xA8,query_number,outputData,sizeof(outputData));
            }
        }
        break;

        //Get the character id return
        case 0xAA:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            if((size-pos)<(int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint32_t characterId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);//include if null, drop the dynamic size overhead
            std::cout << "created character with id: " << characterId << ", returnCode: " << std::to_string(returnCode) << ", line: " << __FILE__ << ": " << __LINE__ << std::endl;
            newCharacterId(returnCode,characterId);
        }
        break;
        //Get the character id return
        case 0xAB:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            /*uint8_t returnCode=data[pos];*/
            pos+=sizeof(uint8_t);
            //just don't emited and used
        }
        break;
        //get the character selection return
        case 0xAC:
        //get the character selection return on game server
        case 0x93:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);

            if(selectedServerIndex==-1)
            {
                parseError("Internal error","selectedServerIndex==-1 with main ident: "+
                           std::to_string(packetCode)+", and queryNumber: "+
                           std::to_string(queryNumber)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                           );
                return false;
            }
            const ServerFromPoolForDisplay &serverFromPoolForDisplay=serverOrdenedList.at(selectedServerIndex);

            if(returnCode!=0x01 && (size==1 || serverFromPoolForDisplay.host.empty()))
            {
                std::string string;
                if(returnCode==0x02)
                    string=std::string("Character not found");
                else if(returnCode==0x03)
                    string=std::string("Already logged");
                else if(returnCode==0x04)
                    string=std::string("Server internal problem");
                else if(returnCode==0x05)
                    string=std::string("Server not found");
                else if(returnCode==0x08)
                    string=std::string("Too recently disconnected");
                else
                    string=std::string("Unknown error: ")+std::to_string(returnCode);
                std::cerr << "Selected character not found, reason: " << string << ", data: " << binarytoHexa(data,size) << std::endl;
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
                        std::cerr << "serverFromPoolForDisplay.host.isEmpty() and reconnect, seam be a bug" << std::endl;
                    return parseCharacterBlockServer(
                                packetCode,queryNumber,
                                data+pos,size
                                );
                }
                else
                {
                    tokenForGameServer=std::string(data,size);
                    std::cout << "new token to go on game server: " << binarytoHexa(tokenForGameServer.data(),tokenForGameServer.size()) << std::endl;
                    if(tokenForGameServer.size()==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                    {
                        have_send_protocol=false;
                        message("stageConnexion=CatchChallenger::Api_protocol::StageConnexion::Stage2 set at "+std::string(__FILE__)+":"+std::to_string(__LINE__));
                        stageConnexion=StageConnexion::Stage2;
                        closeSocket();
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
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                    if((size-pos)<(int)sizeof(uint32_t))
                        clanActionSuccess(0);
                    else
                    {
                        uint32_t clanId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                        pos+=sizeof(uint32_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
            if((size-pos)<(int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint16_t shopListSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            uint32_t index=0;
            std::vector<ItemToSellOrBuy> items;
            while(index<shopListSize)
            {
                if((size-pos)<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                item.object=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                item.price=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                item.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                items.push_back(item);
                index++;
            }
            haveShopList(items);
        }
        break;
        //Buy object
        case 0x88:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x04:
                    haveBuyObject((BuyStat)returnCode,0);
                break;
                case 0x03:
                {
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                case 0x02:
                case 0x04:
                    haveSellObject((SoldStat)returnCode,0);
                break;
                case 0x03:
                {
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
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
            if((size-pos)<(int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint32_t remainingProductionTime=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            if((size-pos)<(int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint16_t shopListSize;
            uint32_t index;
            shopListSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            index=0;
            std::vector<ItemToSellOrBuy> resources;
            while(index<shopListSize)
            {
                if((size-pos)<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                item.object=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                item.price=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                item.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                resources.push_back(item);
                index++;
            }
            if((size-pos)<(int)(sizeof(uint16_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            shopListSize=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
            pos+=sizeof(uint16_t);
            index=0;
            std::vector<ItemToSellOrBuy> products;
            while(index<shopListSize)
            {
                if((size-pos)<(int)(sizeof(uint32_t)*2+sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                ItemToSellOrBuy item;
                item.object=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                item.price=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                item.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                products.push_back(item);
                index++;
            }
            haveFactoryList(remainingProductionTime,resources,products);
        }
        break;
        case 0x8B:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                case 0x03:
                case 0x04:
                    haveBuyFactoryObject((BuyStat)returnCode,0);
                break;
                case 0x02:
                {
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
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
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                case 0x03:
                case 0x04:
                    haveSellFactoryObject((SoldStat)returnCode,0);
                break;
                case 0x02:
                {
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t newPrice=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
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
            if((size-pos)<(int)(sizeof(uint64_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint64_t cash=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
            pos+=sizeof(uint64_t);
            if((size-pos)<(int)(sizeof(uint32_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            listSize=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.marketObjectUniqueId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.item=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint64_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                uint64_t price=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
                pos+=sizeof(uint64_t);
                marketObject.price=price;
                marketObjectList.push_back(marketObject);
                index++;
            }
            listSize=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.index=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.monster=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.level=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)(sizeof(uint64_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                uint64_t price=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
                pos+=sizeof(uint64_t);
                marketMonster.price=price;
                marketMonsterList.push_back(marketMonster);
                index++;
            }
            listSize=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<listSize)
            {
                MarketObject marketObject;
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.marketObjectUniqueId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.item=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketObject.quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint64_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                uint64_t price=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
                pos+=sizeof(uint64_t);
                marketObject.price=price;
                marketOwnObjectList.push_back(marketObject);
                index++;
            }
            listSize=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
            pos+=sizeof(uint32_t);
            index=0;
            while(index<listSize)
            {
                MarketMonster marketMonster;
                if((size-pos)<(int)(sizeof(uint32_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.index=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                pos+=sizeof(uint32_t);
                if((size-pos)<(int)(sizeof(uint16_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.monster=le16toh(*reinterpret_cast<const uint16_t *>(data+pos));
                pos+=sizeof(uint16_t);
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                marketMonster.level=data[pos];
                pos+=sizeof(uint8_t);
                if((size-pos)<(int)(sizeof(uint64_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                uint64_t price=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
                pos+=sizeof(uint64_t);
                marketMonster.price=price;
                marketOwnMonsterList.push_back(marketMonster);
                index++;
            }
            marketList(cash,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
        }
        break;
        case 0x8E:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
            switch(returnCode)
            {
                case 0x01:
                    if((size-pos)==0)
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
            if(returnCode==0x01 && (size-pos)>0)
            {
                PlayerMonster monster;
                if(!dataToPlayerMonster(data+pos,size-pos,monster))
                    return false;
                marketBuyMonster(monster);
            }
        }
        break;
        case 0x8F:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
        }
        break;
        case 0x90:
        {
            if((size-pos)<(int)(sizeof(uint64_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint64_t cash=le64toh(*reinterpret_cast<const uint64_t *>(data+pos));
            pos+=sizeof(uint64_t);
            marketGetCash(cash);
        }
        break;
        case 0x91:
        {
            if((size-pos)<(int)(sizeof(uint8_t)))
            {
                parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                           ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                              );
                return false;
            }
            uint8_t returnCode=data[pos];
            pos+=sizeof(uint8_t);
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
                if((size-pos)<(int)(sizeof(uint8_t)))
                {
                    parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                               ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                  );
                    return false;
                }
                uint8_t returnType=data[pos];
                pos+=sizeof(uint8_t);
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
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t objectId=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                    if((size-pos)<(int)(sizeof(uint32_t)))
                    {
                        parseError("Procotol wrong or corrupted","wrong size with main ident: "+std::to_string(packetCode)+
                                   ", data: "+binarytoHexa(data+pos,size)+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)
                                      );
                        return false;
                    }
                    uint32_t quantity=le32toh(*reinterpret_cast<const uint32_t *>(data+pos));
                    pos+=sizeof(uint32_t);
                    marketWithdrawObject(objectId,quantity);
                }
                else
                {
                    PlayerMonster monster;
                    if(!dataToPlayerMonster(data+pos,size-pos,monster))
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
    if((size-pos)!=0)
    {
        parseError("Procotol wrong or corrupted","error: remaining data: parseReplyData("+std::to_string(packetCode)+
                   ","+std::to_string(queryNumber)+
                   "), line: "+std::string(__FILE__)+":"+std::to_string(__LINE__)+
                   ","+binarytoHexa(data,pos)+
                   " "+binarytoHexa(data+pos,size-pos)
                   );
        return false;
    }
    return true;
}
