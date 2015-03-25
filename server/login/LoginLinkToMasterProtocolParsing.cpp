#include "LoginLinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include <iostream>

using namespace CatchChallenger;

void LoginLinkToMaster::parseInputBeforeLogin(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(size);
    switch(mainCodeType)
    {
        case 0x03:
        break;
        case 0x04:
        break;
        case 0x05:
        break;
        default:
            parseNetworkReadError("wrong data before login with mainIdent: "+QString::number(mainCodeType));
        break;
    }
}

void LoginLinkToMaster::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
{
    (void)data;
    (void)size;
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void LoginLinkToMaster::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *rawData,const int &size)
{
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::ProtocolGood && mainCodeType==0xC2 &&
                (subCodeType==0x000F || subCodeType==0x0010 || subCodeType==0x0011)
                )
        {}
        else
        {
            parseNetworkReadError("parseFullMessage() not logged to send: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
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
                case 0x000F:
                {
                    EpollClientLoginSlave::serverLogicalGroupListSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverLogicalGroupList,
                            0xC2,0x000F,rawData,size);
                    if(EpollClientLoginSlave::serverLogicalGroupListSize==0)
                    {
                        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::serverServerListSize>0)
                    {
                        EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                        if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                            memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerListSize,EpollClientLoginSlave::serverLogicalGroupList,EpollClientLoginSlave::serverLogicalGroupListSize);
                        if(EpollClientLoginSlave::serverServerListSize>0)
                            memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerListSize+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
                    }
                }
                break;
                case 0x0010:
                {
                    char rawServerList[1024*1024];
                    memset(rawServerList,0x00,sizeof(rawServerList));
                    int rawServerListSize=0x01;

                    const int &serverListSize=rawData[0x00];
                    rawServerList[0x00]=serverListSize;
                    int pos=0x01;
                    int serverListIndex=0;
                    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                        while(serverListIndex<serverListSize)
                        {
                            //copy the charactersgroup
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size charactersGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &charactersGroupStringSize=rawData[pos];
                                if((size-pos)<(charactersGroupStringSize+1))
                                {
                                    std::cerr << "C20010 size charactersGroupString + size 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+charactersGroupStringSize);
                                pos+=1+charactersGroupStringSize;
                                rawServerListSize+=1+charactersGroupStringSize;
                            }

                            //copy the key
                            {
                                if((size-pos)<4)
                                {
                                    std::cerr << "C20010 size key 32Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,4);
                                pos+=4;
                                rawServerListSize+=4;
                            }

                            //skip the host + port
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size charactersGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &hostStringSize=rawData[pos];
                                if((size-pos)<(1+hostStringSize+2))
                                {
                                    std::cerr << "C20010 size charactersGroupString + size 8Bits header + port too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                pos+=1+hostStringSize+2;
                            }

                            //copy the xml string
                            {
                                if((size-pos)<2)
                                {
                                    std::cerr << "C20010 size xmlString 16Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &xmlStringSize=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                if((size-pos)<(xmlStringSize+2))
                                {
                                    std::cerr << "C20010 size xmlString + size 16Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,2+xmlStringSize);
                                pos+=2+xmlStringSize;
                                rawServerListSize+=2+xmlStringSize;
                            }

                            //copy the logical group
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size logicalGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &logicalGroupStringSize=rawData[pos];
                                if((size-pos)<(logicalGroupStringSize+1))
                                {
                                    std::cerr << "C20010 size logicalGroupString + size 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+logicalGroupStringSize);
                                pos+=1+logicalGroupStringSize;
                                rawServerListSize+=1+logicalGroupStringSize;
                            }

                            serverListIndex++;
                        }
                    else
                        while(serverListIndex<serverListSize)
                        {
                            //copy the charactersgroup
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size charactersGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &charactersGroupStringSize=rawData[pos];
                                if((size-pos)<(charactersGroupStringSize+1))
                                {
                                    std::cerr << "C20010 size charactersGroupString + size 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+charactersGroupStringSize);
                                pos+=1+charactersGroupStringSize;
                                rawServerListSize+=1+charactersGroupStringSize;
                            }

                            //skip the key
                            {
                                if((size-pos)<4)
                                {
                                    std::cerr << "C20010 size key 32Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                pos+=4;
                            }

                            //copy the host + port
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size charactersGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &hostStringSize=rawData[pos];
                                if((size-pos)<(1+hostStringSize+2))
                                {
                                    std::cerr << "C20010 size charactersGroupString + size 8Bits header + port too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+hostStringSize+2);
                                pos+=1+hostStringSize+2;
                                rawServerListSize+=1+hostStringSize+2;
                            }

                            //copy the xml string
                            {
                                if((size-pos)<2)
                                {
                                    std::cerr << "C20010 size xmlString 16Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &xmlStringSize=be16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                if((size-pos)<(xmlStringSize+2))
                                {
                                    std::cerr << "C20010 size xmlString + size 16Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,2+xmlStringSize);
                                pos+=2+xmlStringSize;
                                rawServerListSize+=2+xmlStringSize;
                            }

                            //copy the logical group
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C20010 size logicalGroupString 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &logicalGroupStringSize=rawData[pos];
                                if((size-pos)<(logicalGroupStringSize+1))
                                {
                                    std::cerr << "C20010 size logicalGroupString + size 8Bits header too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+logicalGroupStringSize);
                                pos+=1+logicalGroupStringSize;
                                rawServerListSize+=1+logicalGroupStringSize;
                            }

                            serverListIndex++;
                        }
                    if((size-pos)!=0)
                    {
                        std::cerr << "C20010 remaining data (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::serverServerListSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverServerList,
                            0xC2,0x000F,rawData,size);
                    if(EpollClientLoginSlave::serverServerListSize==0)
                    {
                        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                    {
                        EpollClientLoginSlave::serverLogicalGroupAndServerListSize=EpollClientLoginSlave::serverServerListSize+EpollClientLoginSlave::serverLogicalGroupListSize;
                        if(EpollClientLoginSlave::serverLogicalGroupListSize>0)
                            memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerListSize,EpollClientLoginSlave::serverLogicalGroupList,EpollClientLoginSlave::serverLogicalGroupListSize);
                        if(EpollClientLoginSlave::serverServerListSize>0)
                            memcpy(EpollClientLoginSlave::serverLogicalGroupAndServerListSize+EpollClientLoginSlave::serverLogicalGroupListSize,EpollClientLoginSlave::serverServerList,EpollClientLoginSlave::serverServerListSize);
                    }
                }
                break;
                case 0x0011:
                {
                    if(size<(1+4+1+1+1+1))
                    {
                        std::cerr << "C20011 base size too small (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(size!=(1+4+1+1+1+1+EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize))
                    {
                        std::cerr << "C20011 base size != EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::automatic_account_creation=rawData[0x00];
                    EpollClientLoginSlave::character_delete_time=be32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                    EpollClientLoginSlave::min_character=rawData[0x05];
                    EpollClientLoginSlave::max_character=rawData[0x06];
                    EpollClientLoginSlave::max_pseudo_size=rawData[0x07];
                    if(EpollClientLoginSlave::min_character>EpollClientLoginSlave::max_character)
                    {
                        std::cerr << "C20011 min_character>max_character (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::max_character==0)
                    {
                        std::cerr << "C20011 max_character==0 (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::max_pseudo_size<3)
                    {
                        std::cerr << "C20011 max_pseudo_size<3 (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::character_delete_time==0)
                    {
                        std::cerr << "C20011 character_delete_time==0 (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(memcmp(rawData+0x08,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize)!=0)
                    {
                        std::cerr << "C20011 different CharactersGroup registred on master server (abort) in " << __LINE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                }
                break;
                default:
                    parseNetworkReadError("unknown sub ident: "+QString::number(mainCodeType)+" "+QString::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//have query with reply
void LoginLinkToMaster::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    Q_UNUSED(data);
    if(!have_send_protocol)
    {
        parseInputBeforeLogin(mainCodeType,queryNumber,data,size);
        return;
    }
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

void LoginLinkToMaster::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
{
    (void)subCodeType;
    (void)queryNumber;
    (void)rawData;
    (void)size;
    if(account_id==0)
    {
        parseNetworkReadError(QStringLiteral("is not logged, parseQuery(%1,%2)").arg(mainCodeType).arg(queryNumber));
        return;
    }
    //do the work here
    switch(mainCodeType)
    {
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
}

//send reply
void LoginLinkToMaster::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    Q_UNUSED(size);
    //do the work here
    switch(mainCodeType)
    {
        case 0x08:
        50 maxAccountIdList: 00000001
            50 maxCharacterIdList: 00000001
            50 maxMonsterIdList: 00000001
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1, %2").arg(mainCodeType).arg(queryNumber));
    return;
}

void LoginLinkToMaster::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    if(!have_send_protocol)
    {
        std::cerr << "parseFullReplyData() reply to unknown query: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
        abort();
    }
    (void)data;
    (void)size;
    queryNumberList.push_back(queryNumber);
    Q_UNUSED(data);
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LoginLinkToMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
