#include "LinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"
#include "../../general/base/CommonSettingsCommon.h"

#include <QCryptographicHash>

using namespace CatchChallenger;

void LinkToMaster::parseInputBeforeLogin(const uint8_t &mainCodeType, const uint8_t &queryNumber, const char * const data, const unsigned int &size)
{
    Q_UNUSED(queryNumber);
    Q_UNUSED(size);
    Q_UNUSED(data);
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+std::to_string(mainCodeType));
        break;
    }
}

void LinkToMaster::parseMessage(const uint8_t &mainCodeType,const char * const data,const unsigned int &size)
{
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
}

void LinkToMaster::parseFullMessage(const uint8_t &mainCodeType,const uint8_t &subCodeType,const char *rawData,const unsigned int &size)
{
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::ProtocolGood && mainCodeType==0xC2 &&
                (subCodeType==0x0F || subCodeType==0x10 || subCodeType==0x11)
                )
        {}
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+std::to_string(mainCodeType)+" "+std::to_string(subCodeType));
            return;
        }
    }
    (void)rawData;
    (void)size;
    switch(mainCodeType)
    {
        case 0xC2:
            switch(subCodeType)
            {
                case 0x0F:
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
                    EpollClientLoginSlave::serverLogicalGroupListSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverLogicalGroupList,
                            0xC2,0x0F,rawData,size);
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
                case 0x10:
                {
                    //purge the internal data
                    {
                        int index=0;
                        while(index<CharactersGroupForLogin::list.size())
                        {
                            CharactersGroupForLogin::list.at(index)->clearServerPair();
                            index++;
                        }
                    }

                    if(size<1)
                    {
                        std::cerr << "C210 missing first bytes (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    int pos=0;

                    const unsigned short int &serverListSize=rawData[0x00];
                    pos+=1;

                    int serverListIndex=0;
                    uint32_t serverUniqueKey;const char * hostData;uint8_t hostDataSize;uint16_t port;

                    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                    {
                        //EpollClientLoginSlave::serverServerList[0x00]=0x02;//do into EpollServerLoginSlave::EpollServerLoginSlave()
                        EpollClientLoginSlave::serverServerList[0x01]=serverListSize;
                        /// \warning not linked with above
                        EpollClientLoginSlave::serverServerListSize=0x02;
                        uint8_t charactersGroupIndex;
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
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                serverUniqueKey=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(rawData+pos)));
                                pos+=4;
                                EpollClientLoginSlave::serverServerListSize+=4;
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
                                const uint8_t &logicalGroupStringSize=rawData[pos];
                                if((size-pos)<static_cast<unsigned int>(logicalGroupStringSize+1))
                                {
                                    std::cerr << "C210 size logicalGroupString + size 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(EpollClientLoginSlave::serverServerList+EpollClientLoginSlave::serverServerListSize,rawData+pos,1+logicalGroupStringSize);
                                pos+=1+logicalGroupStringSize;
                                EpollClientLoginSlave::serverServerListSize+=1+logicalGroupStringSize;
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
                        uint8_t charactersGroupIndex;
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
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
                                  << std::string(std::vector<char>(rawData,pos).toHex()).toStdString()
                                  << " "
                                  << std::string(std::vector<char>(rawData+pos,size-pos).toHex()).toStdString()
                                  << " "
                                  << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::serverServerListComputedMessageSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverServerListComputedMessage,
                            0xC2,0x0E,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
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
                }
                break;
                case 0x11:
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

                            //cache query to compose
                            profile.preparedQueryChar=NULL;
                            {
                                unsigned int index=0;
                                while(index<sizeof(profile.preparedQueryPos))
                                {
                                    profile.preparedQueryPos[index]=0;
                                    profile.preparedQuerySize[index]=0;
                                    index++;
                                }
                            }

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

                                profile.monsters.push_back(monster);
                                monsterListIndex++;
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

                                profile.reputation.push_back(reputation);
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
                default:
                    parseNetworkReadError("unknown sub ident: "+std::to_string(mainCodeType)+" "+std::to_string(subCodeType));
                    return;
                break;
            }
        break;
        case 0xE1:
        switch(subCodeType)
        {
            case 0x01:
                /// \todo broadcast to client before the logged step
                if(size!=EpollClientLoginSlave::serverServerListCurrentPlayerSize)
                {
                    parseNetworkReadError("size!=EpollClientLoginSlave::serverServerListCurrentPlayerSize main ident: "+std::to_string(mainCodeType)+", sub ident: "+std::to_string(subCodeType));
                    return;
                }
                /// \warning C20E compressed! can't direct alter!
                if(compressionTypeServer==CompressionType::None)
                {
                    memcpy(EpollClientLoginSlave::serverServerListComputedMessage+
                           (EpollClientLoginSlave::serverServerListComputedMessageSize-EpollClientLoginSlave::serverServerListCurrentPlayerSize),
                            rawData,
                            EpollClientLoginSlave::serverServerListCurrentPlayerSize);
                    memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+
                           (EpollClientLoginSlave::serverLogicalGroupAndServerListSize-EpollClientLoginSlave::serverServerListCurrentPlayerSize),
                            rawData,
                            EpollClientLoginSlave::serverServerListCurrentPlayerSize);
                }
                else
                {
                    {
                        memcpy(EpollClientLoginSlave::serverServerList+
                           (EpollClientLoginSlave::serverServerListSize-EpollClientLoginSlave::serverServerListCurrentPlayerSize),
                            rawData,
                            EpollClientLoginSlave::serverServerListCurrentPlayerSize);
                        EpollClientLoginSlave::serverServerListComputedMessageSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverServerListComputedMessage,
                            0xC2,0x0E,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
                    }
                    if(EpollClientLoginSlave::serverServerListComputedMessageSize>0)
                    {
                        EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListComputedMessageSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                        memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerList+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerListComputedMessage,EpollClientLoginSlave::serverServerListComputedMessageSize);
                    }
                }
            return;
            default:
                parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+", sub ident: "+std::to_string(subCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
                return;
            break;
        }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
}

//have query with reply
void LinkToMaster::parseQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    Q_UNUSED(data);
    if(stat!=Stat::Logged)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
}

void LinkToMaster::parseFullQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *rawData,const unsigned int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError(std::stringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
}

//send reply
void LinkToMaster::parseReplyData(const uint8_t &mainCodeType,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::Connected && mainCodeType==0x01)
        {}
        else if(stat==Stat::ProtocolGood && mainCodeType==0x08)
        {}
        else
        {
            parseNetworkReadError(std::stringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
            return;
        }
    }
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    switch(mainCodeType)
    {
        case 0x01:
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
                        std::cerr << "compression type wrong with main ident: 1 and queryNumber: %2, type: query_type_protocol" << queryNumber << std::endl;
                        abort();
                    return;
                }
                stat=Stat::ProtocolGood;
                //send the query 0x08
                {
                    QCryptographicHash hash(QCryptographicHash::Sha224);
                    hash.addData(reinterpret_cast<const char *>(LinkToMaster::private_token),TOKEN_SIZE_FOR_MASTERAUTH);
                    hash.addData(data+1,TOKEN_SIZE_FOR_CLIENT_AUTH_AT_CONNECT);
                    const std::vector<char> &hashedToken=hash.result();
                    memset(LinkToMaster::private_token,0x00,sizeof(LinkToMaster::private_token));

                    packOutcommingQuery(0x08,queryNumberList.back(),hashedToken.constData(),hashedToken.size());
                    queryNumberList.pop_back();
                }
                return;
            }
            else
            {
                if(returnCode==0x02)
                    std::cerr << "Protocol not supported" << std::endl;
                else
                    std::cerr << "Unknown error " << returnCode << std::endl;
                abort();
                return;
            }
        }
        break;
        case 0x08:
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
                int index=0;
                while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                {
                    if((size-pos)<4)
                    {
                        std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::maxAccountIdList << le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                    pos+=4;
                    index++;
                }
            }
            {
                int groupIndex=0;
                while(groupIndex<CharactersGroupForLogin::list.size())
                {
                    int index=0;
                    while(index<CATCHCHALLENGER_SERVER_MAXIDBLOCK)
                    {
                        if((size-pos)<4)
                        {
                            std::cerr << "reply to 08 size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        CharactersGroupForLogin::list[groupIndex]->maxCharacterId .push_back(le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos))));
                        pos+=4;
                        index++;
                    }
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
                    groupIndex++;
                }
            }
            stat=Stat::Logged;
            if(EpollServerLoginSlave::epollServerLoginSlave->getSfd()==-1)
                if(!EpollServerLoginSlave::epollServerLoginSlave->tryListen())
                    abort();
        }
        return;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
    parseNetworkReadError(std::stringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LinkToMaster::parseFullReplyData(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const char *data,const unsigned int &size)
{
    queryNumberList.push_back(queryNumber);
    if(stat!=Stat::Logged)
    {
        parseNetworkReadError(std::stringLiteral("is not logged, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    (void)data;
    (void)size;
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
            switch(subCodeType)
            {
                case 0x07:
                {
                    if(selectCharacterClients.contains(queryNumber))
                    {
                        const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=selectCharacterClients.value(queryNumber);
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
                                            ->parseNetworkReadError("client in wrong state main ident: "+std::to_string(mainCodeType)+", with sub ident:"+std::to_string(subCodeType)+", reply size for 0207 wrong");
                                            return;
                                        }
                                        //check again if the game server is not disconnected, don't check charactersGroupIndex because previously checked at EpollClientLoginSlave::selectCharacter()
                                        const uint8_t &charactersGroupIndex=static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->charactersGroupIndex;
                                        const uint32_t &serverUniqueKey=static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->serverUniqueKey;
                                        if(!CharactersGroupForLogin::list.at(charactersGroupIndex)->containsServerUniqueKey(serverUniqueKey))
                                        {
                                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                            ->parseNetworkReadError("client server not found to proxy it main ident: "+std::to_string(mainCodeType)+", with sub ident:"+std::to_string(subCodeType)+", reply size for 0207 wrong");
                                            return;
                                        }
                                        const CharactersGroupForLogin::InternalGameServer &server=CharactersGroupForLogin::list.at(charactersGroupIndex)->getServerInformation(serverUniqueKey);

                                        static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                        ->stat=EpollClientLoginSlave::EpollClientLoginStat::GameServerConnecting;
                                        /// \todo do the async connect
                                        /// linkToGameServer->stat=Stat::Connecting;
                                        const int &socketFd=LinkToGameServer::tryConnect(server.host.toLatin1(),server.port,5,1);
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
                                        }
                                        else
                                        {
                                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)
                                            ->parseNetworkReadError(std::stringLiteral("not able to connect on the game server as proxy, parseReplyData(%1,%2)").arg(mainCodeType).arg(queryNumber));
                                            return;
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
                                parseNetworkReadError("main ident: "+std::to_string(mainCodeType)+", with sub ident:"+std::to_string(subCodeType)+", reply size for 0207 wrong");
                        }
                        selectCharacterClients.remove(queryNumber);
                    }
                    else
                        std::cerr << "parseFullReplyData() !selectCharacterClients.contains(queryNumber): mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                }
                return;
                default:
                    parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+", with sub ident:"+std::to_string(subCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+std::to_string(mainCodeType)+std::string(", file:%1:%2").arg(__FILE__).arg(__LINE__));
            return;
        break;
    }
    parseNetworkReadError(std::stringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LinkToMaster::parseNetworkReadError(const std::string &errorString)
{
    errorParsingLayer(errorString);
}
