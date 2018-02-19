#include "LinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include "EpollServerLoginSlave.h"
#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../epoll/EpollSocket.h"
#include "VariableLoginServer.h"

#include <iostream>
#include <openssl/sha.h>

using namespace CatchChallenger;

bool LinkToMaster::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &, const char * const , const unsigned int &)
{
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        return false;
    }
    return false;
}

bool LinkToMaster::parseMessage(const uint8_t &mainCodeType,const char *rawData,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::ProtocolGood &&
                (mainCodeType==0x44 || mainCodeType==0x45 || mainCodeType==0x46)
                )
        {}
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType));
            return false;
        }
    }
    switch(mainCodeType)
    {
        //Logical group
        case 0x44:
        {
            //control it
            uint32_t pos=1;
            uint8_t logicalGroupSize=rawData[0x00];
            uint8_t logicalGroupIndex=0;
            while(logicalGroupIndex<logicalGroupSize)
            {
                //path
                {
                    if((size-pos)<sizeof(uint8_t))
                    {
                        std::cerr << "Wrong remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint8_t &pathSize=rawData[pos];
                    pos+=sizeof(uint8_t);
                    if((size-pos)<pathSize)
                    {
                        std::cerr << "Wrong remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    pos+=pathSize;
                }

                //xml
                {
                    if((size-pos)<sizeof(uint16_t))
                    {
                        std::cerr << "Wrong remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint16_t &xmlSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos)));
                    pos+=sizeof(uint16_t);
                    if((size-pos)<xmlSize)
                    {
                        std::cerr << "Wrong remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    pos+=xmlSize;
                }

                logicalGroupIndex++;
            }
            if(pos!=size)
            {
                std::cerr << "Wrong remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            //copy it
            EpollClientLoginSlave::serverLogicalGroupList[0x00]=0x44;
            *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::serverLogicalGroupList+1)=htole32(size);//set the dynamic size
            memcpy(EpollClientLoginSlave::serverLogicalGroupList+1+4,rawData,size);
            EpollClientLoginSlave::serverLogicalGroupListSize=size+1+4;
            if(EpollClientLoginSlave::serverLogicalGroupListSize==0)
            {
                std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(EpollClientLoginSlave::serverServerListComputedMessageSize>0)
            {
                EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListComputedMessageSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList,
                           EpollClientLoginSlave::serverLogicalGroupList,
                           EpollClientLoginSlave::serverLogicalGroupListSize);
                if(EpollClientLoginSlave::serverServerListComputedMessageSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+EpollClientLoginSlave::serverLogicalGroupListSize,
                           EpollClientLoginSlave::serverServerListComputedMessage,
                           EpollClientLoginSlave::serverServerListComputedMessageSize);
            }
            else
            {
                memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList,
                       EpollClientLoginSlave::serverLogicalGroupList,
                       EpollClientLoginSlave::serverLogicalGroupListSize);
            }
        }
        break;
        //Raw server list master to login
        case 0x45:
        {
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "set server list by set in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            //purge the internal data
            {
                unsigned int index=0;
                while(index<CharactersGroupForLogin::list.size())
                {
                    CharactersGroupForLogin * const group=CharactersGroupForLogin::list.at(index);
                    group->clearServerPair();
                    index++;
                }
            }
            std::unordered_map<uint8_t/*charactersgroup index*/,std::unordered_set<uint32_t/*unique key*/> > duplicateDetect;

            if(size<1)
            {
                std::cerr << "C210 missing first bytes (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            int pos=0;

            const unsigned short int &serverListSize=rawData[0x00];
            pos+=1;

            int serverListIndex=0;
            uint32_t serverUniqueKey;const char * hostData;uint8_t hostDataSize;uint16_t port;uint8_t charactersGroupIndex;

            if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
            {
                //EpollClientLoginSlave::serverServerList[0x00]=0x02;//do into EpollServerLoginSlave::EpollServerLoginSlave()
                EpollClientLoginSlave::serverServerList[0x01]=serverListSize;
                /// \warning not linked with above
                EpollClientLoginSlave::serverServerListSize=0x02;
                while(serverListIndex<serverListSize)
                {
                    //copy the charactersgroup
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size charactersGroupIndex 8Bits too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        charactersGroupIndex=rawData[pos];
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize]=charactersGroupIndex;
                        EpollClientLoginSlave::serverServerListSize+=1;
                        pos+=1;
                    }

                    //copy the key
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,4);
                        if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
                        {
                            std::cerr << "C210 CharactersGroupForLogin not found, charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            std::cerr << "CharactersGroupForLogin found:" << std::endl;
                            auto it=CharactersGroupForLogin::hash.begin();
                            while(it!=CharactersGroupForLogin::hash.cend())
                            {
                                std::cerr << "- " << it->first << std::endl;
                                ++it;
                            }
                            std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                            abort();
                        }
                        serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+pos)));
                        pos+=4;
                        EpollClientLoginSlave::serverServerListSize+=4;
                    }
                    //add more control
                    {
                        if(duplicateDetect.find(charactersGroupIndex)==duplicateDetect.cend())
                            duplicateDetect[charactersGroupIndex]=std::unordered_set<uint32_t/*unique key*/>();
                        std::unordered_set<uint32_t/*unique key*/> &duplicateDetectEntry=duplicateDetect[charactersGroupIndex];
                        if(duplicateDetectEntry.find(serverUniqueKey)!=duplicateDetectEntry.cend())//exists, bug
                        {
                            std::cerr << "Duplicate unique key for packet 45 found: " << std::to_string(serverUniqueKey) << std::endl;
                            abort();
                        }
                        else
                            duplicateDetectEntry.insert(serverUniqueKey);
                    }

                    //skip the host + port
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const uint8_t &hostStringSize=rawData[pos];
                        if((size-pos)<static_cast<unsigned int>(1+hostStringSize+2))
                        {
                            std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        hostData=rawData+pos+1;
                        hostDataSize=hostStringSize;
                        port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos+1+hostStringSize)));
                        pos+=1+hostStringSize+2;
                    }

                    CharactersGroupForLogin::list.at(charactersGroupIndex)->setServerUniqueKey(serverListIndex,serverUniqueKey,hostData,hostDataSize,port);
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->setServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    #endif

                    //copy the xml string
                    {
                        if((size-pos)<2)
                        {
                            std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos)));
                        if((size-pos)<static_cast<unsigned int>(xmlStringSize+2))
                        {
                            std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,2+xmlStringSize);
                        pos+=2+xmlStringSize;
                        EpollClientLoginSlave::serverServerListSize+=2+xmlStringSize;
                    }

                    //copy the logical group
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize]=rawData[pos];
                        pos+=1;
                        EpollClientLoginSlave::serverServerListSize+=1;
                    }

                    //copy the max player
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size the max player 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize+0]=rawData[pos+0];
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize+1]=rawData[pos+1];
                        pos+=2;
                        EpollClientLoginSlave::serverServerListSize+=2;
                    }

                    serverListIndex++;
                }
            }
            else
            {
                //EpollClientLoginSlave::serverServerList[0x00]=0x01;//do into EpollServerLoginSlave::EpollServerLoginSlave()
                EpollClientLoginSlave::serverServerList[0x01]=serverListSize;
                /// \warning not linked with above
                EpollClientLoginSlave::serverServerListSize=0x02;
                while(serverListIndex<serverListSize)
                {
                    //copy the charactersgroup
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size charactersGroupIndex 8Bits too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        charactersGroupIndex=rawData[pos];
                        pos+=1;
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize]=charactersGroupIndex;
                        EpollClientLoginSlave::serverServerListSize+=1;
                    }

                    //copy the unique key
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,4);
                        if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
                        {
                            std::cerr << "C210 CharactersGroupForLogin not found, charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            std::cerr << "CharactersGroupForLogin found:" << std::endl;
                            auto it=CharactersGroupForLogin::hash.begin();
                            while(it!=CharactersGroupForLogin::hash.cend())
                            {
                                std::cerr << "- " << it->first << std::endl;
                                ++it;
                            }
                            std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                            abort();
                        }
                        serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+pos)));
                        EpollClientLoginSlave::serverServerListSize+=4;
                        pos+=4;
                    }

                    //copy the host + port
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const uint8_t &hostStringSize=rawData[pos];
                        if((size-pos)<static_cast<unsigned int>(1+hostStringSize+2))
                        {
                            std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,1+hostStringSize+2);
                        hostData=rawData+pos+1;
                        hostDataSize=hostStringSize;
                        port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos+1+hostStringSize)));
                        if(port<=0)
                        {
                            std::cerr << "C210 port can't be <= 0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        if(hostStringSize<=0)
                        {
                            std::cerr << "C210 hostStringSize can't be <= 0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        pos+=1+hostStringSize+2;
                        EpollClientLoginSlave::serverServerListSize+=1+hostStringSize+2;
                    }

                    CharactersGroupForLogin::list.at(charactersGroupIndex)->setServerUniqueKey(serverListIndex,serverUniqueKey,hostData,hostDataSize,port);
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->setServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    #endif

                    //copy the xml string
                    {
                        if((size-pos)<2)
                        {
                            std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos)));
                        if((size-pos)<static_cast<unsigned int>(xmlStringSize+2))
                        {
                            std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,2+xmlStringSize);
                        pos+=2+xmlStringSize;
                        EpollClientLoginSlave::serverServerListSize+=2+xmlStringSize;
                    }

                    //copy the logical group
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize]=rawData[pos];
                        pos+=1;
                        EpollClientLoginSlave::serverServerListSize+=1;
                    }

                    //copy the max player
                    {
                        if((size-pos)<1)
                        {
                            std::cerr << "C210 size the max player 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize+0]=rawData[pos+0];
                        EpollClientLoginSlave::serverServerList[EpollClientLoginSlave::serverServerListSize+1]=rawData[pos+1];
                        pos+=2;
                        EpollClientLoginSlave::serverServerListSize+=2;
                    }

                    serverListIndex++;
                }
            }
            EpollClientLoginSlave::serverServerListCurrentPlayerSize=serverListSize*sizeof(uint16_t);
            if(serverListSize>0)
            {
                if((size-pos)<static_cast<unsigned int>(serverListSize*(sizeof(uint16_t))))
                {
                    std::cerr << "C210 co player list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,serverListSize*(sizeof(uint16_t)));
                pos+=serverListSize*(sizeof(uint16_t));
                EpollClientLoginSlave::serverServerListSize+=serverListSize*(sizeof(uint16_t));
            }

            if((size-pos)!=0)
            {
                std::cerr << "C210 remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                std::cerr << "Data: "
                          << binarytoHexa(rawData,pos)
                          << " "
                          << binarytoHexa(rawData+pos,size-pos)
                          << " "
                          << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            EpollClientLoginSlave::serverServerListComputedMessage[0x00]=0x40;
            *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::serverServerListComputedMessage+1)=htole32(EpollClientLoginSlave::serverServerListSize);//set the dynamic size
            memcpy(EpollClientLoginSlave::serverServerListComputedMessage+1+4,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
            EpollClientLoginSlave::serverServerListComputedMessageSize=EpollClientLoginSlave::serverServerListSize+1+4;
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            if(EpollClientLoginSlave::serverServerListSize==0)
            {
                std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
            {
                EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListComputedMessageSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                /* First query already set
                if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList,EpollClientLoginSlave::serverLogicalGroupList,EpollClientLoginSlave::serverLogicalGroupListSize);*/
                if(EpollClientLoginSlave::serverServerListComputedMessageSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerListComputedMessage,EpollClientLoginSlave::serverServerListComputedMessageSize);
            }

            {
                unsigned int index=0;
                while(index<EpollClientLoginSlave::stat_client_list.size())
                {
                    EpollClientLoginSlave * const client=EpollClientLoginSlave::stat_client_list.at(index);
                    client->sendRawBlock(EpollClientLoginSlave::serverServerListComputedMessage,EpollClientLoginSlave::serverServerListComputedMessageSize);
                    index++;
                }
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            CharactersGroupForLogin::serverDumpCharactersGroup();
            #endif
        }
        break;
        //Login settings and Characters group
        case 0x46:
        {
            if(size<(1+4+1+1+1+1 +1+2+1+2))
            {
                std::cerr << "C211 base size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            CommonSettingsCommon::commonSettingsCommon.automatic_account_creation=rawData[0x00];
            CommonSettingsCommon::commonSettingsCommon.character_delete_time=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+0x01)));
            CommonSettingsCommon::commonSettingsCommon.min_character=rawData[0x05];
            CommonSettingsCommon::commonSettingsCommon.max_character=rawData[0x06];
            CommonSettingsCommon::commonSettingsCommon.max_pseudo_size=rawData[0x07];
            CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters=rawData[0x08];
            CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+0x09)));
            CommonSettingsCommon::commonSettingsCommon.maxPlayerItems=rawData[0x0B];
            CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerItems=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+0x0C)));
            if(CommonSettingsCommon::commonSettingsCommon.min_character>CommonSettingsCommon::commonSettingsCommon.max_character)
            {
                std::cerr << "C211 min_character>max_character (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(CommonSettingsCommon::commonSettingsCommon.max_character==0)
            {
                std::cerr << "C211 max_character==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size<3)
            {
                std::cerr << "C211 max_pseudo_size<3 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(CommonSettingsCommon::commonSettingsCommon.character_delete_time==0)
            {
                std::cerr << "C211 character_delete_time==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            unsigned int cursor=0x0E;
            if((size-cursor)<EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize)
            {
                std::cerr << "C211 too small for different CharactersGroup (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            if(memcmp(rawData+cursor,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize)!=0)
            {
                std::cerr << "C211 different CharactersGroup registred on master server (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            //dynamic data
            cursor+=EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize;
            //skins
            {
                if((size-cursor)<1)
                {
                    std::cerr << "C211 too small for skinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                const uint8_t &skinListSize=rawData[cursor];
                cursor+=1;
                uint8_t skinListIndex=0;
                while(skinListIndex<skinListSize)
                {
                    if((size-cursor)<(int)sizeof(uint16_t))
                    {
                        std::cerr << "C211 too small for databaseId skin (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint16_t &databaseId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                    cursor+=sizeof(uint16_t);
                    //index is the internal id
                    EpollServerLoginSlave::epollServerLoginSlave->setSkinPair(skinListIndex,databaseId);
                    skinListIndex++;
                }
            }
            //profile
            {
                if((size-cursor)<1)
                {
                    std::cerr << "C211 too small for profileListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                const uint8_t &profileListSize=rawData[cursor];
                if(profileListSize==0 && CommonSettingsCommon::commonSettingsCommon.min_character!=CommonSettingsCommon::commonSettingsCommon.max_character)
                {
                    std::cout << "no profile loaded!" << std::endl;
                    abort();
                }
                cursor+=1;
                uint8_t profileListIndex=0;
                while(profileListIndex<profileListSize)
                {
                    EpollServerLoginSlave::LoginProfile profile;

                    //database id
                    if((size-cursor)<(int)sizeof(uint16_t))
                    {
                        std::cerr << "C211 too small for databaseId profile (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    profile.databaseId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                    cursor+=sizeof(uint16_t);

                    //skin
                    if((size-cursor)<1)
                    {
                        std::cerr << "C211 too small for forcedskinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint8_t &forcedskinListSize=rawData[cursor];
                    cursor+=1;
                    uint8_t forcedskinListIndex=0;
                    while(forcedskinListIndex<forcedskinListSize)
                    {
                        if((size-cursor)<1)
                        {
                            std::cerr << "C211 too small for forcedskin skin (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        profile.forcedskin.push_back(rawData[cursor]);
                        cursor+=1;
                        forcedskinListIndex++;
                    }
                    //cash
                    {
                        /* will crash on ARM with grsec due to unaligned 64Bits access
                        profile.cash=le64toh(*reinterpret_cast<uint64_t *>(const_cast<char *>(rawData+cursor))); */

                        uint64_t tempCash=0;
                        memcpy(&tempCash,rawData+cursor,8);
                        profile.cash=le64toh(tempCash);
                        cursor+=sizeof(uint64_t);
                    }

                    //monster
                    if((size-cursor)<1)
                    {
                        std::cerr << "C211 too small for forcedskinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint8_t &monsterGroupListSize=rawData[cursor];
                    cursor+=1;
                    uint8_t monsterGroupListIndex=0;
                    while(monsterGroupListIndex<monsterGroupListSize)
                    {
                        std::vector<EpollServerLoginSlave::LoginProfile::Monster> monsters;
                        //monster
                        if((size-cursor)<1)
                        {
                            std::cerr << "C211 too small for forcedskinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const uint8_t &monsterListSize=rawData[cursor];
                        cursor+=1;
                        uint8_t monsterListIndex=0;
                        while(monsterListIndex<monsterListSize)
                        {
                            EpollServerLoginSlave::LoginProfile::Monster monster;
                            //monster id
                            if((size-cursor)<sizeof(uint16_t))
                            {
                                std::cerr << "C211 too small for id monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            monster.id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(uint16_t);
                            //level
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for level monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            monster.level=rawData[cursor];
                            cursor+=1;
                            //captured with
                            if((size-cursor)<sizeof(uint16_t))
                            {
                                std::cerr << "C211 too small for captured with monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            monster.captured_with=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(uint16_t);
                            //hp
                            if((size-cursor)<sizeof(uint32_t))
                            {
                                std::cerr << "C211 too small for hp monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            monster.hp=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(uint32_t);
                            //gender
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for gender monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            monster.ratio_gender=rawData[cursor];
                            cursor+=1;
                            //skill
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for skill list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const uint8_t &skillListSize=rawData[cursor];
                            cursor+=1;
                            uint8_t skillListIndex=0;
                            while(skillListIndex<skillListSize)
                            {
                                EpollServerLoginSlave::LoginProfile::Monster::Skill skill;
                                //skill id
                                if((size-cursor)<sizeof(uint16_t))
                                {
                                    std::cerr << "C211 too small for id monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                skill.id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(uint16_t);
                                //level
                                if((size-cursor)<1)
                                {
                                    std::cerr << "C211 too small for level monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                skill.level=rawData[cursor];
                                cursor+=1;
                                //endurance
                                if((size-cursor)<1)
                                {
                                    std::cerr << "C211 too small for endurance monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                skill.endurance=rawData[cursor];
                                cursor+=1;

                                monster.skills.push_back(skill);
                                skillListIndex++;
                            }

                            monsters.push_back(monster);
                            monsterListIndex++;
                        }
                        profile.monstergroup.push_back(monsters);
                        monsterGroupListIndex++;
                    }

                    //reputation
                    if((size-cursor)<1)
                    {
                        std::cerr << "C211 too small for reputation list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint8_t &reputationListSize=rawData[cursor];
                    cursor+=1;
                    uint8_t reputationListIndex=0;
                    while(reputationListIndex<reputationListSize)
                    {
                        EpollServerLoginSlave::LoginProfile::Reputation reputation;

                        //reputation id
                        if((size-cursor)<sizeof(uint16_t))
                        {
                            std::cerr << "C211 too small for id reputationDatabaseId (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        reputation.reputationDatabaseId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                        cursor+=sizeof(uint16_t);
                        //level
                        if((size-cursor)<1)
                        {
                            std::cerr << "C211 too small for level reputation (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        reputation.level=rawData[cursor];
                        cursor+=1;
                        //point
                        if((size-cursor)<sizeof(int32_t))
                        {
                            std::cerr << "C211 too small for point reputation (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        reputation.point=le32toh(*reinterpret_cast<int32_t *>(const_cast<char *>(rawData+cursor)));
                        cursor+=sizeof(int32_t);

                        profile.reputations.push_back(reputation);
                        reputationListIndex++;
                    }

                    //item
                    if((size-cursor)<1)
                    {
                        std::cerr << "C211 too small for item list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    const uint8_t &itemListSize=rawData[cursor];
                    cursor+=1;
                    uint8_t itemListIndex=0;
                    while(itemListIndex<itemListSize)
                    {
                        EpollServerLoginSlave::LoginProfile::Item item;

                        //item id
                        if((size-cursor)<sizeof(uint16_t))
                        {
                            std::cerr << "C211 too small for id item (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        item.id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+cursor)));
                        cursor+=sizeof(uint16_t);
                        //point
                        if((size-cursor)<sizeof(int32_t))
                        {
                            std::cerr << "C211 too small for quantity item (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        item.quantity=le32toh(*reinterpret_cast<int32_t *>(const_cast<char *>(rawData+cursor)));
                        cursor+=sizeof(int32_t);

                        profile.items.push_back(item);
                        itemListIndex++;
                    }

                    EpollServerLoginSlave::epollServerLoginSlave->loginProfileList.push_back(profile);
                    profileListIndex++;
                }
            }

            //sha224 sum
            if((size-cursor)<28)
            {
                std::cerr << "C211 sha224 sum item list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            memcpy(EpollClientLoginSlave::baseDatapackSum,rawData+cursor,28);
            cursor+=28;

            if((size-cursor)!=0)
            {
                std::cerr << "C211 remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }

            EpollServerLoginSlave::epollServerLoginSlave->compose04Reply();
        }
        break;
        //Update the game server current player number on the game server
        case 0x47:
            /// \todo broadcast to client before the logged step
            if(size!=EpollClientLoginSlave::serverServerListCurrentPlayerSize)
            {
                parseNetworkReadError("size!=EpollClientLoginSlave::serverServerListCurrentPlayerSize main ident: "+std::to_string(mainCodeType));
                return false;
            }
            memcpy(EpollClientLoginSlave::serverServerList+
               (EpollClientLoginSlave::serverServerListSize-EpollClientLoginSlave::serverServerListCurrentPlayerSize),
                rawData,
                EpollClientLoginSlave::serverServerListCurrentPlayerSize);

            EpollClientLoginSlave::serverServerListComputedMessage[0x00]=0x40;
            *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::serverServerListComputedMessage+1)=htole32(EpollClientLoginSlave::serverServerListSize);//set the dynamic size
            memcpy(EpollClientLoginSlave::serverServerListComputedMessage+1+4,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
            EpollClientLoginSlave::serverServerListComputedMessageSize=EpollClientLoginSlave::serverServerListSize+1+4;
            EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListComputedMessageSize+EpollClientLoginSlave::serverLogicalGroupListSize;
            memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerListComputedMessage,EpollClientLoginSlave::serverServerListComputedMessageSize);

            {
                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x47;
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,rawData,size);

                unsigned int index=0;
                while(index<EpollClientLoginSlave::stat_client_list.size())
                {
                    EpollClientLoginSlave * const client=EpollClientLoginSlave::stat_client_list.at(index);
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+4+size);
                    index++;
                }
            }
        return true;
        //Update server list master to login
        case 0x48:
        {
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "set server list by diff in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            /// \todo broadcast to client before the logged step
            if(EpollClientLoginSlave::serverServerListComputedMessageSize==0)
            {
                parseNetworkReadError("EpollClientLoginSlave::serverServerListComputedMessageSize==0 main ident: "+std::to_string(mainCodeType));
                return false;
            }
            if(size==2 && rawData[0x00]==0 && rawData[0x01]==0)
                return true;
            uint8_t serverListCount=EpollClientLoginSlave::serverServerList[0x01];
            unsigned int pos=0;
            size_t posTempBuffer=2;
            const uint8_t &deleteSize=rawData[pos];
            pos+=1;
            std::vector<uint16_t> currentPlayerNumberList;
            uint8_t serverBlockListSizeBeforeAdd=0;

            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            //performance boost and null size problem covered with this
            if(deleteSize>=serverListCount)//do without control, should be true
            {
                if(deleteSize>serverListCount)
                {
                    parseNetworkReadError("deleteSize>serverListCount main ident: "+std::to_string(mainCodeType));
                    return false;
                }
                //EpollClientLoginSlave::serverServerList[0x00]=EpollClientLoginSlave::proxyMode;
                EpollClientLoginSlave::serverServerList[0x01]=0;//server list size
                EpollClientLoginSlave::serverServerListSize=2;
                EpollClientLoginSlave::serverServerListCurrentPlayerSize=0;
                *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::serverServerListComputedMessage+1)=htole32(EpollClientLoginSlave::serverServerListSize);//set the dynamic size
                memcpy(EpollClientLoginSlave::serverServerListComputedMessage+1+4,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
                EpollClientLoginSlave::serverServerListComputedMessageSize=EpollClientLoginSlave::serverServerListSize+1+4;
                //purge the internal data
                {
                    unsigned int index=0;
                    while(index<CharactersGroupForLogin::list.size())
                    {
                        CharactersGroupForLogin * const group=CharactersGroupForLogin::list.at(index);
                        group->clearServerPair();
                        index++;
                    }
                }
                pos+=deleteSize;
                serverListCount=0;
                posTempBuffer=EpollClientLoginSlave::serverServerListSize;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
            }
            else
            {
                //unserialise the data from EpollClientLoginSlave::serverServerList
                struct ServerBlock
                {
                    char * data;
                    uint16_t size;
                    uint8_t oldIndex;
                    uint16_t rawCurrentPlayerNumber;
                    uint8_t charactersgroup;
                    uint32_t serverUniqueKey;
                };
                std::vector<ServerBlock> serverBlockList;
                serverBlockList.reserve(serverListCount);

                {
                    size_t serverServerListPos=2;
                    size_t index=0;
                    ServerBlock newBlock;
                    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                    {
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "serverBlock raw dump EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        #endif
                        while(index<serverListCount)
                        {
                            newBlock.data=EpollClientLoginSlave::serverServerList+serverServerListPos;
                            newBlock.oldIndex=index;

                            //here because minor cost
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            newBlock.charactersgroup=EpollClientLoginSlave::serverServerList[serverServerListPos];
                            serverServerListPos+=1;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            newBlock.serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(EpollClientLoginSlave::serverServerList+serverServerListPos)));
                            serverServerListPos+=4;

                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(EpollClientLoginSlave::serverServerList+serverServerListPos));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=2;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=xmlStringSize;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=3;

                            newBlock.size=1+4+2+xmlStringSize+3;
                            serverBlockList.push_back(newBlock);

                            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                            std::cout << std::to_string(newBlock.charactersgroup) << " " << std::to_string(newBlock.serverUniqueKey) << " " << binarytoHexa(newBlock.data,newBlock.size) << std::endl;
                            #endif

                            index++;
                        }
                    }
                    else
                    {
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "serverBlock raw dump EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Reconnect at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        #endif
                        while(index<serverListCount)
                        {
                            newBlock.data=EpollClientLoginSlave::serverServerList+serverServerListPos;
                            newBlock.oldIndex=index;

                            //here because minor cost
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            newBlock.charactersgroup=EpollClientLoginSlave::serverServerList[serverServerListPos];
                            serverServerListPos+=1;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            newBlock.serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(EpollClientLoginSlave::serverServerList+serverServerListPos)));
                            serverServerListPos+=4;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            const uint8_t &hostStringSize=EpollClientLoginSlave::serverServerList[serverServerListPos];
                            serverServerListPos+=1;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=hostStringSize;
                            //port
                            serverServerListPos+=2;

                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(EpollClientLoginSlave::serverServerList+serverServerListPos));
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=2;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=xmlStringSize;
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                            {
                                std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                                abort();
                            }
                            #endif
                            serverServerListPos+=3;

                            newBlock.size=1+4+1+hostStringSize+2+xmlStringSize+2+3;
                            serverBlockList.push_back(newBlock);

                            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                            std::cout << std::to_string(newBlock.charactersgroup) << " " << std::to_string(newBlock.serverUniqueKey) << " " << binarytoHexa(newBlock.data,newBlock.size) << std::endl;
                            #endif

                            index++;
                        }
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(serverServerListPos>=EpollClientLoginSlave::serverServerListSize)
                    {
                        std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    #endif
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if((EpollClientLoginSlave::serverServerListSize-serverServerListPos)!=(serverListCount*sizeof(uint16_t)))
                    {
                        std::cout << "EpollClientLoginSlave::serverServerList out of range at file:" << __FILE__ << ":" << std::to_string(__LINE__) << std::endl;
                        abort();
                    }
                    #endif
                    index=0;
                    while(index<serverListCount)
                    {
                        serverBlockList[index].rawCurrentPlayerNumber=*reinterpret_cast<uint16_t *>(EpollClientLoginSlave::serverServerList+serverServerListPos);
                        serverServerListPos+=2;
                        index++;
                    }
                }
                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                std::cout << "serverBlock dump at file:" << __FILE__ << ":" << std::to_string(__LINE__) << ":" << std::endl;
                {
                    size_t index=0;
                    while(index<serverBlockList.size())
                    {
                        const ServerBlock &serverBlock=serverBlockList[index];
                        std::cout << std::to_string(serverBlock.oldIndex) << ") " << std::to_string(serverBlock.charactersgroup) << " - " << std::to_string(serverBlock.serverUniqueKey) << std::endl;
                        index++;
                    }
                }
                #endif
                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                CharactersGroupForLogin::serverDumpCharactersGroup();
                #endif

                //do the delete
                if(deleteSize>0)
                {
                    size_t index=0;
                    if((size-pos)<(deleteSize*sizeof(uint8_t)))
                    {
                        parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", (size-cursor)<(deleteSize*sizeof(uint16_t)), file:"+__FILE__+":"+std::to_string(__LINE__));
                        return false;
                    }
                    while(index<deleteSize)
                    {
                        const uint8_t &deleteIndex=rawData[pos];
                        pos+=1;
                        if(deleteIndex<serverBlockList.size())
                        {
                            const ServerBlock &serverBlock=serverBlockList.at(deleteIndex);
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(serverBlock.charactersgroup>=CharactersGroupForLogin::list.size())
                            {
                                parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", serverBlock.charactersgroup>=CharactersGroupForLogin::list.size(), file:"+__FILE__+":"+std::to_string(__LINE__));
                                return false;
                            }
                            if(!CharactersGroupForLogin::list.at(serverBlock.charactersgroup)->containsServerUniqueKey(serverBlock.serverUniqueKey))
                            {
                                std::cerr << "!CharactersGroupForLogin::list.at(" << std::to_string(serverBlock.charactersgroup) << ")->containsServerUniqueKey(" << std::to_string(serverBlock.serverUniqueKey) << ") to remove, pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                std::cerr << "CharactersGroupForLogin found:" << std::endl;
                                auto it=CharactersGroupForLogin::hash.begin();
                                while(it!=CharactersGroupForLogin::hash.cend())
                                {
                                    std::cerr << "- " << it->first << std::endl;
                                    ++it;
                                }
                                std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                                abort();
                            }
                            #endif
                            CharactersGroupForLogin::list.at(serverBlock.charactersgroup)->removeServerUniqueKey(serverBlock.serverUniqueKey);
                            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                            std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(serverBlock.charactersgroup) << ")->removeServerUniqueKey(" << std::to_string(serverBlock.serverUniqueKey) << "), pos: " << pos << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            #endif
                            serverBlockList.erase(serverBlockList.cbegin()+deleteIndex);
                        }
                        else
                        {
                            parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", delete entry is out of range, file:"+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        index++;
                    }
                }
                ProtocolParsingBase::tempBigBufferForOutput[0x00]=EpollClientLoginSlave::proxyMode;
                ProtocolParsingBase::tempBigBufferForOutput[0x01]=serverBlockList.size();
                serverBlockListSizeBeforeAdd=serverBlockList.size();

                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                CharactersGroupForLogin::serverDumpCharactersGroup();
                #endif

                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                std::cout << "serverBlock dump at file:" << __FILE__ << ":" << std::to_string(__LINE__) << ":" << std::endl;
                {
                    size_t index=0;
                    while(index<serverBlockList.size())
                    {
                        const ServerBlock &serverBlock=serverBlockList[index];
                        std::cout << std::to_string(serverBlock.oldIndex) << ") " << std::to_string(serverBlock.charactersgroup) << " - " << std::to_string(serverBlock.serverUniqueKey) << std::endl;
                        index++;
                    }
                }
                #endif
                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                CharactersGroupForLogin::serverDumpCharactersGroup();
                #endif

                #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                #endif
                //copy into the big buffer
                {
                    uint8_t previousIndex=0;
                    char * firstBlockToCopy=NULL;
                    char * lastBlockToCopy=NULL;
                    size_t index=0;
                    while(index<serverBlockListSizeBeforeAdd)
                    {
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        CharactersGroupForLogin::serverDumpCharactersGroup();
                        #endif
                        const ServerBlock &serverBlock=serverBlockList[index];
                        //if continue
                        if(firstBlockToCopy==NULL || previousIndex==(serverBlock.oldIndex-1))
                        {
                            if(firstBlockToCopy==NULL)
                                firstBlockToCopy=serverBlock.data;
                        }
                        else //if need flush
                        {
                            const size_t &mergedSize=lastBlockToCopy-firstBlockToCopy;
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,firstBlockToCopy,mergedSize);
                            posTempBuffer+=mergedSize;
                            firstBlockToCopy=serverBlock.data;
                            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            #endif
                        }
                        previousIndex=serverBlock.oldIndex;
                        lastBlockToCopy=serverBlock.data+serverBlock.size;
                        currentPlayerNumberList.push_back(serverBlock.rawCurrentPlayerNumber);
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        CharactersGroupForLogin::serverDumpCharactersGroup();
                        #endif

                        CharactersGroupForLogin * const charactersGroup=CharactersGroupForLogin::list.at(serverBlock.charactersgroup);
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        if(serverBlock.charactersgroup>=CharactersGroupForLogin::list.size())
                        {
                            parseNetworkReadError("for main ident: "+std::to_string(mainCodeType)+", serverBlock.charactersgroup>=CharactersGroupForLogin::list.size(), file:"+__FILE__+":"+std::to_string(__LINE__));
                            return false;
                        }
                        //removed above at charactersGroup->removeServerUniqueKey(serverBlock.serverUniqueKey); line 1145
                        if(!charactersGroup->containsServerUniqueKey(serverBlock.serverUniqueKey))
                        {
                            std::cerr << "!CharactersGroupForLogin::list.at(" << std::to_string(serverBlock.charactersgroup) << ")->containsServerUniqueKey(" << std::to_string(serverBlock.serverUniqueKey) << ") to remove, pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            std::cerr << "CharactersGroupForLogin found:" << std::endl;
                            auto it=CharactersGroupForLogin::hash.begin();
                            while(it!=CharactersGroupForLogin::hash.cend())
                            {
                                std::cerr << "- " << it->first << std::endl;
                                ++it;
                            }
                            std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                            abort();
                        }
                        #endif
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        CharactersGroupForLogin::serverDumpCharactersGroup();
                        #endif
                        charactersGroup->setIndexServerUniqueKey(index,serverBlock.serverUniqueKey);
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        CharactersGroupForLogin::serverDumpCharactersGroup();
                        #endif
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(serverBlock.charactersgroup) << ")->setIndexServerUniqueKey(" << std::to_string(serverBlock.serverUniqueKey) << "," << std::to_string(index) << "), pos: " << pos << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif

                        index++;
                    }
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    CharactersGroupForLogin::serverDumpCharactersGroup();
                    #endif
                    //forced flush
                    if(firstBlockToCopy!=NULL && lastBlockToCopy!=NULL)
                    {
                        const size_t &mergedSize=lastBlockToCopy-firstBlockToCopy;
                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,firstBlockToCopy,mergedSize);
                        posTempBuffer+=mergedSize;
                    }
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    #endif
                }
                //copy the current player number into local variable
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif

            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            CharactersGroupForLogin::serverDumpCharactersGroup();
            #endif
            //do the insert the first part
            size_t serverListRawDataAddedFrom=0;
            size_t serverListRawDataAddedTo=0;
            const uint8_t &serverListSize=rawData[pos];
            pos+=1;
            if(serverListSize>0)
            {
                int serverListIndex=0;
                uint32_t serverUniqueKey;const char * hostData;uint8_t hostDataSize;uint16_t port;uint8_t charactersGroupIndex;

                if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                {
                    serverListRawDataAddedFrom=posTempBuffer;
                    while(serverListIndex<serverListSize)
                    {
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif
                        //copy the charactersgroup
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size charactersGroupIndex 8Bits too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            charactersGroupIndex=rawData[pos];
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer]=charactersGroupIndex;
                            posTempBuffer+=1;
                            pos+=1;
                        }

                        //copy the key
                        {
                            if((size-pos)<4)
                            {
                                std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,4);
                            if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
                            {
                                std::cerr << "C210 CharactersGroupForLogin not found, charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                std::cerr << "CharactersGroupForLogin found:" << std::endl;
                                auto it=CharactersGroupForLogin::hash.begin();
                                while(it!=CharactersGroupForLogin::hash.cend())
                                {
                                    std::cerr << "- " << it->first << std::endl;
                                    ++it;
                                }
                                std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                                abort();
                            }
                            serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+pos)));
                            pos+=4;
                            posTempBuffer+=4;
                            /*disabled, can update via add if needed//if same than current mean error: update is with remove before
                            if(CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
                            {
                                std::cerr << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->containsServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                std::cerr << "CharactersGroupForLogin found:" << std::endl;
                                auto it=CharactersGroupForLogin::hash.begin();
                                while(it!=CharactersGroupForLogin::hash.cend())
                                {
                                    std::cerr << "- " << it->first << std::endl;
                                    ++it;
                                }
                                std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                                abort();
                            }*/
                        }

                        //skip the host + port
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const uint8_t &hostStringSize=rawData[pos];
                            if((size-pos)<static_cast<unsigned int>(1+hostStringSize+2))
                            {
                                std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            hostData=rawData+pos+1;
                            hostDataSize=hostStringSize;
                            port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos+1+hostStringSize)));
                            pos+=1+hostStringSize+2;
                        }

                        CharactersGroupForLogin::list.at(charactersGroupIndex)->setServerUniqueKey(serverBlockListSizeBeforeAdd+serverListIndex,serverUniqueKey,hostData,hostDataSize,port);
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->setServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif

                        //copy the xml string
                        {
                            if((size-pos)<2)
                            {
                                std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos)));
                            if((size-pos)<static_cast<unsigned int>(xmlStringSize+2))
                            {
                                std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,2+xmlStringSize);
                            pos+=2+xmlStringSize;
                            posTempBuffer+=2+xmlStringSize;
                        }

                        //copy the logical group
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer]=rawData[pos];
                            pos+=1;
                            posTempBuffer+=1;
                        }

                        //copy the max player
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size the max player 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer+0]=rawData[pos+0];
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer+1]=rawData[pos+1];
                            pos+=2;
                            posTempBuffer+=2;
                        }
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif

                        serverListIndex++;
                    }
                    serverListRawDataAddedTo=posTempBuffer;
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    #endif
                }
                else
                {
                    while(serverListIndex<serverListSize)
                    {
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif
                        //copy the charactersgroup
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size charactersGroupIndex 8Bits too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            charactersGroupIndex=rawData[pos];
                            pos+=1;
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer]=charactersGroupIndex;
                            posTempBuffer+=1;
                        }

                        //copy the unique key
                        {
                            if((size-pos)<4)
                            {
                                std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,4);
                            if(charactersGroupIndex>=CharactersGroupForLogin::list.size())
                            {
                                std::cerr << "C210 CharactersGroupForLogin not found, charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                std::cerr << "CharactersGroupForLogin found:" << std::endl;
                                auto it=CharactersGroupForLogin::hash.begin();
                                while(it!=CharactersGroupForLogin::hash.cend())
                                {
                                    std::cerr << "- " << it->first << std::endl;
                                    ++it;
                                }
                                std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                                abort();
                            }
                            serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+pos)));
                            posTempBuffer+=4;
                            pos+=4;
                            /*if same than current mean: update if(CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
                            {
                                std::cerr << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->containsServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                std::cerr << "CharactersGroupForLogin found:" << std::endl;
                                auto it=CharactersGroupForLogin::hash.begin();
                                while(it!=CharactersGroupForLogin::hash.cend())
                                {
                                    std::cerr << "- " << it->first << std::endl;
                                    ++it;
                                }
                                std::cerr << "Data:" << binarytoHexa(rawData,pos) << " " << binarytoHexa(rawData+pos,(size-pos)) << std::endl;
                                abort();
                            }*/
                        }

                        //copy the host + port
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const uint8_t &hostStringSize=rawData[pos];
                            if((size-pos)<static_cast<unsigned int>(1+hostStringSize+2))
                            {
                                std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,1+hostStringSize+2);
                            hostData=rawData+pos+1;
                            hostDataSize=hostStringSize;
                            port=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos+1+hostStringSize)));
                            if(port<=0)
                            {
                                std::cerr << "C210 port can't be <= 0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            if(hostStringSize<=0)
                            {
                                std::cerr << "C210 hostStringSize can't be <= 0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            pos+=1+hostStringSize+2;
                            posTempBuffer+=1+hostStringSize+2;
                        }

                        CharactersGroupForLogin::list.at(charactersGroupIndex)->setServerUniqueKey(serverBlockListSizeBeforeAdd+serverListIndex,serverUniqueKey,hostData,hostDataSize,port);
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "CharactersGroupForLogin::list.at(" << std::to_string(charactersGroupIndex) << ")->setServerUniqueKey(" << std::to_string(serverUniqueKey) << "), charactersGroupIndex: " << std::to_string(charactersGroupIndex) << ", pos: " << pos << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif

                        //copy the xml string
                        {
                            if((size-pos)<2)
                            {
                                std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const uint16_t &xmlStringSize=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(rawData+pos)));
                            if((size-pos)<static_cast<unsigned int>(xmlStringSize+2))
                            {
                                std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,2+xmlStringSize);
                            pos+=2+xmlStringSize;
                            posTempBuffer+=2+xmlStringSize;
                        }

                        //copy the logical group
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer]=rawData[pos];
                            pos+=1;
                            posTempBuffer+=1;
                        }

                        //copy the max player
                        {
                            if((size-pos)<1)
                            {
                                std::cerr << "C210 size the max player 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer+0]=rawData[pos+0];
                            ProtocolParsingBase::tempBigBufferForOutput[posTempBuffer+1]=rawData[pos+1];
                            pos+=2;
                            posTempBuffer+=2;
                        }
                        #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                        std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        #endif

                        serverListIndex++;
                    }
                    #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
                    std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    #endif
                }
                ProtocolParsingBase::tempBigBufferForOutput[0x01]+=serverListSize;
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            CharactersGroupForLogin::serverDumpCharactersGroup();
            #endif
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            //copy the old current player number
            if(!currentPlayerNumberList.empty())
            {
                size_t index=0;
                while(index<currentPlayerNumberList.size())
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer)=currentPlayerNumberList.at(index);
                    posTempBuffer+=2;
                    index++;
                }
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            //the the remaing size
            {
                EpollClientLoginSlave::serverServerListCurrentPlayerSize=(ProtocolParsingBase::tempBigBufferForOutput[0x01])*sizeof(uint16_t);
                if((size-pos)!=(serverListSize*sizeof(uint16_t)))
                {
                    std::cerr << "48 remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    std::cerr << "Data: "
                              << binarytoHexa(rawData,pos)
                              << " "
                              << binarytoHexa(rawData+pos,size-pos)
                              << " "
                              << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif
            //copy the new current player number
            {
                const size_t blockSize=size-pos;
                if(blockSize>0)
                {
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posTempBuffer,rawData+pos,blockSize);
                    posTempBuffer+=blockSize;
                    pos+=blockSize;
                }
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(tempBigBufferForOutput,posTempBuffer) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif

            //serialise to EpollClientLoginSlave::serverServerList
            memcpy(EpollClientLoginSlave::serverServerList,ProtocolParsingBase::tempBigBufferForOutput,posTempBuffer);
            //EpollClientLoginSlave::serverServerList[0x00]=0x02;//do into EpollServerLoginSlave::EpollServerLoginSlave()
            //EpollClientLoginSlave::serverServerList[0x01]+=serverListSize;do above
            EpollClientLoginSlave::serverServerListSize=posTempBuffer;
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            std::cout << "EpollClientLoginSlave::serverServerList: " << binarytoHexa(EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize) << " in " << __FILE__ << ":" <<__LINE__ << std::endl;
            #endif

            //update the EpollClientLoginSlave::serverServerListComputedMessage
            EpollClientLoginSlave::serverServerListComputedMessage[0x00]=0x40;
            *reinterpret_cast<uint32_t *>(EpollClientLoginSlave::serverServerListComputedMessage+1)=htole32(EpollClientLoginSlave::serverServerListSize);//set the dynamic size
            memcpy(EpollClientLoginSlave::serverServerListComputedMessage+1+4,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
            EpollClientLoginSlave::serverServerListComputedMessageSize=EpollClientLoginSlave::serverServerListSize+1+4;
            if(EpollClientLoginSlave::serverServerListSize==0)
            {
                std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }

            //merge with EpollClientLoginSlave::serverLogicalGroupAndServerListSize
            if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
            {
                EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListComputedMessageSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                /* First query already set
                if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList,EpollClientLoginSlave::serverLogicalGroupList,EpollClientLoginSlave::serverLogicalGroupListSize);*/
                if(EpollClientLoginSlave::serverServerListComputedMessageSize>0)
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerListComputedMessage,EpollClientLoginSlave::serverServerListComputedMessageSize);
            }

            //send to stat client
            {
                uint32_t sizeTosend=0;
                ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x48;
                if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Reconnect)
                {
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(size);
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+4,rawData,size);
                    sizeTosend=1+4+size;
                }
                else
                {
                    uint32_t pos=1+4;
                    const uint16_t &sizeOfDelete=1+rawData[0x00];
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,rawData,sizeOfDelete);
                    pos+=sizeOfDelete;
                    ProtocolParsingBase::tempBigBufferForOutput[pos]=serverListSize;
                    pos+=1;
                    const uint32_t &sizeOfInsert=serverListRawDataAddedTo-serverListRawDataAddedFrom;
                    if(sizeOfInsert>0)
                    {
                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,EpollClientLoginSlave::serverServerList+serverListRawDataAddedFrom,sizeOfInsert);
                        pos+=sizeOfInsert;
                        //copy the current connected player
                        memcpy(ProtocolParsingBase::tempBigBufferForOutput+pos,rawData+(size-serverListSize*sizeof(uint16_t)),serverListSize*sizeof(uint16_t));
                        pos+=serverListSize*sizeof(uint16_t);

                    }
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(pos-1-4);
                    sizeTosend=pos;
                }
                unsigned int index=0;
                while(index<EpollClientLoginSlave::stat_client_list.size())
                {
                    EpollClientLoginSlave * const client=EpollClientLoginSlave::stat_client_list.at(index);
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,sizeTosend);
                    index++;
                }
            }
            #ifdef CATCHCHALLENGER_DEBUG_SERVERLIST
            CharactersGroupForLogin::serverDumpCharactersGroup();
            #endif
        }
        return true;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    return true;
}

//have query with reply
bool LinkToMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        return parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+", file:"+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
}

//send reply
bool LinkToMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::Connected && mainCodeType==0xB8)
        {}
        else if(stat==Stat::ProtocolGood && mainCodeType==0xBD)
        {}
        else
        {
            parseNetworkReadError("is not logged, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+"): "+std::string(__FILE__)+", "+std::to_string(__LINE__));
            return false;
        }
    }
    //do the work here
    switch(mainCodeType)
    {
        //Protocol initialization and auth for master
        case 0xB8:
        {
            if(size<1)
            {
                std::cerr << "Need more size for protocol header " << std::endl;
                abort();
            }
            //Protocol initialization
            const uint8_t &returnCode=data[0x00];
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                if(size!=(1+TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT))
                {
                    std::cerr << "wrong size for protocol header " << returnCode << std::endl;
                    abort();
                }
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::None;
                    break;
                    case 0x08:
                        ProtocolParsing::compressionTypeClient=ProtocolParsing::CompressionType::Zstandard;
                    break;
                    default:
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return false;
                }
                stat=Stat::ProtocolGood;
                //send the query 0x08
                {
                    SHA256_CTX hash;
                    if(SHA224_Init(&hash)!=1)
                    {
                        std::cerr << "SHA224_Init(&hash)!=1" << std::endl;
                        abort();
                    }
                    SHA224_Update(&hash,reinterpret_cast<const char *>(LinkToMaster::private_token_master),TOKEN_SIZE_FOR_MASTERAUTH);
                    SHA224_Update(&hash,data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    std::cout << "SHA224(" << binarytoHexa(reinterpret_cast<const char *>(LinkToMaster::private_token_master),TOKEN_SIZE_FOR_MASTERAUTH)
                              << " " << binarytoHexa(data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT) << ") to auth on master" << std::endl;
                    #endif
                    unsigned char tempHashResult[CATCHCHALLENGER_SHA224HASH_SIZE];
                    SHA224_Final(tempHashResult,&hash);

                    //memset(LinkToMaster::private_token_master,0x00,sizeof(LinkToMaster::private_token_master));//To reauth after disconnexion

                    //send the network query
                    registerOutputQuery(queryNumberList.back(),0xBD);
                    ProtocolParsingBase::tempBigBufferForOutput[0x00]=0xBD;
                    ProtocolParsingBase::tempBigBufferForOutput[0x01]=queryNumberList.back();

                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+1+1,tempHashResult,CATCHCHALLENGER_SHA224HASH_SIZE);

                    sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,1+1+CATCHCHALLENGER_SHA224HASH_SIZE);

                    queryNumberList.pop_back();
                }
                return true;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return false;
            }
        }
        return true;
        //Register login server
        case 0xBD:
        {
            if(CharactersGroupForLogin::list.size()==0)
            {
                std::cerr << "CharactersGroupForLogin::list.size()==0, C211 never send? (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                abort();
            }
            unsigned int pos=0;
            {
                if((size-pos)<1)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                if(data[pos]!=1)
                {
                    std::cerr << "reply to 08 return code wrong too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                pos++;
            }
            {
                EpollClientLoginSlave::maxAccountIdList.reserve(EpollClientLoginSlave::maxAccountIdList.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                unsigned int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    if((size-pos)<4)
                    {
                        std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::maxAccountIdList.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                    pos+=4;
                    index++;
                }
                if(vectorHaveDuplicatesForSmallList(EpollClientLoginSlave::maxAccountIdList))
                {
                    std::cerr << "reply to 08: duplicate maxAccountIdList in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                    unsigned int index=0;
                    while(index<EpollClientLoginSlave::maxAccountIdList.size())
                    {
                        if(index>0)
                            std::cerr << ", ";
                        std::cerr << EpollClientLoginSlave::maxAccountIdList[index];
                        index++;
                    }
                    std::cerr << std::endl;
                    abort();
                }
            }
            {
                unsigned int groupIndex=0;
                while(groupIndex<CharactersGroupForLogin::list.size())
                {
                    CharactersGroupForLogin::list[groupIndex]->maxCharacterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                    unsigned int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        CharactersGroupForLogin::list[groupIndex]->maxCharacterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxCharacterId))
                    {
                        std::cerr << "reply to 08: duplicate maxCharacterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                        unsigned int index=0;
                        while(index<CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size())
                        {
                            if(index>0)
                                std::cerr << ", ";
                            std::cerr << CharactersGroupForLogin::list[groupIndex]->maxCharacterId[index];
                            index++;
                        }
                        std::cerr << std::endl;
                        abort();
                    }

                    CharactersGroupForLogin::list[groupIndex]->maxMonsterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
                    index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        CharactersGroupForLogin::list[groupIndex]->maxMonsterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
                    if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxMonsterId))
                    {
                        std::cerr << "reply to 08: duplicate maxMonsterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                        unsigned int index=0;
                        while(index<CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size())
                        {
                            if(index>0)
                                std::cerr << ", ";
                            std::cerr << CharactersGroupForLogin::list[groupIndex]->maxMonsterId[index];
                            index++;
                        }
                        std::cerr << std::endl;
                        abort();
                    }

                    groupIndex++;
                }
            }
            stat=Stat::Logged;
            if(!EpollServerLoginSlave::epollServerLoginSlave->isListening())
                if(!EpollServerLoginSlave::epollServerLoginSlave->tryListen())
                    abort();
        }
        return true;
        //Select character on master
        case 0xBE:
        {
            if(selectCharacterClients.find(queryNumber)!=selectCharacterClients.cend())
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=selectCharacterClients.at(queryNumber);
                if(dataForSelectedCharacterReturn.client!=NULL)
                {
                    if(size==CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER)
                    {
                        if(dataForSelectedCharacterReturn.client!=NULL)
                        {
                            if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                            {
                                if(static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->stat!=EpollClientLoginSlave::EpollClientLoginStat::CharacterSelecting)
                                {
                                    static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadError("client in wrong state main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                                    return false;
                                }
                                //check again if the game server is not disconnected, don't check charactersGroupIndex because previously checked at EpollClientLoginSlave::selectCharacter()
                                const uint8_t &charactersGroupIndex=static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->charactersGroupIndex;
                                const uint32_t &serverUniqueKey=static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->serverUniqueKey;
                                if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
                                {
                                    static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadError("client server not found to proxy it main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                                    return false;
                                }
                                const CharactersGroupForLogin::InternalGameServer &server=CharactersGroupForLogin::list.at(charactersGroupIndex)->getServerInformation(serverUniqueKey);

                                static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                ->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                                /// \todo do the async connect
                                /// linkToGameServer->stat=Stat::Connecting;
                                const int &socketFd=LinkToGameServer::tryConnect(server.host.c_str(),server.port,5,1);
                                if(Q_LIKELY(socketFd>=0))
                                {
                                    static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                    ->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnected;
                                    LinkToGameServer *linkToGameServer=new LinkToGameServer(socketFd);
                                    static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                    ->linkToGameServer=linkToGameServer;
                                    linkToGameServer->queryIdToReconnect=dataForSelectedCharacterReturn.client_query_id;
                                    linkToGameServer->stat=LinkToGameServer::Stat::Connected;
                                    linkToGameServer->client=static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client);
                                    memcpy(linkToGameServer->tokenForGameServer,data,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
                                    //send the protocol
                                    //wait readTheFirstSslHeader() to sendProtocolHeader();
                                    linkToGameServer->setConnexionSettings();
                                    linkToGameServer->parseIncommingData();
                                    /*const int &s = EpollSocket::make_non_blocking(socketFd);
                                    if(s == -1)
                                        std::cerr << "unable to make to socket non blocking" << std::endl;*/
                                }
                                else
                                {
                                    static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                    ->parseNetworkReadError("not able to connect on the game server as proxy, parseReplyData("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+")");
                                    return false;
                                }
                            }
                            else
                            {
                                static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                ->stat=EpollClientLoginSlave::EpollClientLoginStat::CharacterSelected;
                                static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                ->selectCharacter_ReturnToken(dataForSelectedCharacterReturn.client_query_id,data);
                                static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                ->closeSocket();
                            }
                        }
                    }
                    else if(size==1)
                    {
                        if(dataForSelectedCharacterReturn.client!=NULL)
                        {
                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                            ->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,data[0]);
                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                            ->closeSocket();
                        }
                    }
                    else
                        parseNetworkReadError("main ident: "+std::to_string(mainCodeType)+", reply size for 0207 wrong");
                }
                selectCharacterClients.erase(queryNumber);
            }
            else
                std::cerr << "parseFullReplyData() !selectCharacterClients.contains(queryNumber): mainCodeType: " << mainCodeType << ", queryNumber: " << queryNumber << std::endl;
        }
        return true;
        case 0xBF:
        {
            unsigned int pos=0;
            EpollClientLoginSlave::maxAccountIdList.reserve(EpollClientLoginSlave::maxAccountIdList.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                EpollClientLoginSlave::maxAccountIdList.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(EpollClientLoginSlave::maxAccountIdList))
            {
                std::cerr << "reply to 08: duplicate maxAccountIdList in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<EpollClientLoginSlave::maxAccountIdList.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << EpollClientLoginSlave::maxAccountIdList[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            EpollClientLoginSlave::maxAccountIdRequested=false;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cout << "Add more id to list: EpollClientLoginSlave::maxAccountIdList.size(): " << EpollClientLoginSlave::maxAccountIdList.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif
        }
        return true;
        case 0xC0:
        {
            const uint8_t groupIndex=LinkToMaster::queryNumberToCharacterGroup[queryNumber];

            unsigned int pos=0;
            CharactersGroupForLogin::list[groupIndex]->maxCharacterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                CharactersGroupForLogin::list[groupIndex]->maxCharacterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxCharacterId))
            {
                std::cerr << "reply to 08: duplicate maxCharacterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << CharactersGroupForLogin::list[groupIndex]->maxCharacterId[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            #ifdef DEBUG_MESSAGE_QUERY_IDLIST
            std::cout << "Add more id to list: CharactersGroupForLogin::list[" << std::to_string(groupIndex) << "]->maxCharacterId: " << CharactersGroupForLogin::list[groupIndex]->maxCharacterId.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif

            LinkToMaster::queryNumberToCharacterGroup[queryNumber]=0;
            CharactersGroupForLogin::list[groupIndex]->maxCharacterIdRequested=false;
        }
        return true;
        case 0xC1:
        {
            const uint8_t groupIndex=LinkToMaster::queryNumberToCharacterGroup[queryNumber];

            unsigned int pos=0;
            CharactersGroupForLogin::list[groupIndex]->maxMonsterId.reserve(CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size()+CATCHCHALLENGER_SERVER_MAXIDBLOCK);
            unsigned int index=0;
            while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
            {
                if((size-pos)<4)
                {
                    std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                    abort();
                }
                CharactersGroupForLogin::list[groupIndex]->maxMonsterId.push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                pos+=4;
                index++;
            }
            if(vectorHaveDuplicatesForSmallList(CharactersGroupForLogin::list[groupIndex]->maxMonsterId))
            {
                std::cerr << "reply to 08: duplicate maxMonsterId " << std::to_string(groupIndex) << " in " << __FILE__ << ":" <<__LINE__ << ", content: ";
                unsigned int index=0;
                while(index<CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size())
                {
                    if(index>0)
                        std::cerr << ", ";
                    std::cerr << CharactersGroupForLogin::list[groupIndex]->maxMonsterId[index];
                    index++;
                }
                std::cerr << std::endl;
                abort();
            }
            #ifdef DEBUG_MESSAGE_QUERY_IDLIST
            std::cout << "Add more id to list: CharactersGroupForLogin::list[" << std::to_string(groupIndex) << "]->maxMonsterId.size(): " << CharactersGroupForLogin::list[groupIndex]->maxMonsterId.size() << ", file: " << std::string(__FILE__) << ":" << std::to_string(__LINE__) << std::endl;
            #endif

            LinkToMaster::queryNumberToCharacterGroup[queryNumber]=0;
            CharactersGroupForLogin::list[groupIndex]->maxMonsterIdRequested=false;
        }
        return true;
        default:
            parseNetworkReadError("The master server responds to not coded value into the switch (or end with break not return): "+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+", file:"+__FILE__+":"+std::to_string(__LINE__));
            return false;
        break;
    }
    return true;
}

void LinkToMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
