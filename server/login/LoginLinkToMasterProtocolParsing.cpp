#include "LoginLinkToMaster.h"
#include "EpollClientLoginSlave.h"
#include <iostream>
#include "EpollServerLoginSlave.h"
#include "CharactersGroupForLogin.h"

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

void LoginLinkToMaster::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const char *rawData,const int &size)
{
    if(stat!=Stat::Logged)
    {
        if(stat==Stat::ProtocolGood && mainCodeType==0xC2 &&
                (subCodeType==0x0F || subCodeType==0x10 || subCodeType==0x11)
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
                case 0x0F:
                {
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
                case 0x10:
                {
                    QHashIterator<QString,CharactersGroupForLogin *> i(CharactersGroupForLogin::hash);
                    while (i.hasNext()) {
                        i.next();
                        i.value()->clearServerPair();
                    }

                    EpollClientLoginSlave::serverPartialServerListSize=0;
                    memset(EpollClientLoginSlave::serverPartialServerList,0x00,sizeof(EpollClientLoginSlave::serverPartialServerList));
                    int rawServerListSize=0x01;

                    const int &serverListSize=rawData[0x00];
                    rawServerList[0x00]=serverListSize;
                    int pos=0x01;
                    int serverListIndex=0;
                    if(EpollClientLoginSlave::proxyMode==EpollClientLoginSlave::ProxyMode::Proxy)
                        while(serverListIndex<serverListSize)
                        {
                            QString charactersGroupString;

                            //copy the charactersgroup
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &charactersGroupStringSize=rawData[pos];
                                if((size-pos)<(charactersGroupStringSize+1))
                                {
                                    std::cerr << "C210 size charactersGroupString + size 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                charactersGroupString=QString::fromUtf8(rawData+pos+1,charactersGroupStringSize);
                                memcpy(rawServerList,rawData+pos,1+charactersGroupStringSize);
                                pos+=1+charactersGroupStringSize;
                                rawServerListSize+=1+charactersGroupStringSize;
                            }

                            //copy the key
                            {
                                if((size-pos)<4)
                                {
                                    std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,4);
                                if(!CharactersGroupForLogin::hash.contains(charactersGroupString))
                                {
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                pos+=4;
                                rawServerListSize+=4;
                            }

                            //skip the host + port
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &hostStringSize=rawData[pos];
                                if((size-pos)<(1+hostStringSize+2))
                                {
                                    std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                CharactersGroupForLogin::hash[charactersGroupString]->setServerUniqueKey(rawData+pos,4,rawData+1,hostStringSize,le16toh(*reinterpret_cast<quint16 *>(rawData+1+hostStringSize)));
                                pos+=1+hostStringSize+2;
                            }

                            //copy the xml string
                            {
                                if((size-pos)<2)
                                {
                                    std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &xmlStringSize=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                if((size-pos)<(xmlStringSize+2))
                                {
                                    std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
                                    std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &logicalGroupStringSize=rawData[pos];
                                if((size-pos)<(logicalGroupStringSize+1))
                                {
                                    std::cerr << "C210 size logicalGroupString + size 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+logicalGroupStringSize);
                                pos+=1+logicalGroupStringSize;
                                rawServerListSize+=1+logicalGroupStringSize;
                            }

                            //internal id
                            {
                                if((size-pos)<2)
                                {
                                    std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &databaseInternalId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                pos+=2;
                                rawServerListSize+=2;

                                if(!CharactersGroupForLogin::hash.contains(charactersGroupString))
                                {
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                CharactersGroupForLogin::hash[charactersGroupString]->setServerPair(serverListIndex,databaseInternalId);
                            }

                            serverListIndex++;
                        }
                    else
                        while(serverListIndex<serverListSize)
                        {
                            QString charactersGroupString;

                            //copy the charactersgroup
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &charactersGroupStringSize=rawData[pos];
                                if((size-pos)<(charactersGroupStringSize+1))
                                {
                                    std::cerr << "C210 size charactersGroupString + size 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                charactersGroupString=QString::fromUtf8(rawData+pos+1,charactersGroupStringSize);
                                memcpy(rawServerList,rawData+pos,1+charactersGroupStringSize);
                                pos+=1+charactersGroupStringSize;
                                rawServerListSize+=1+charactersGroupStringSize;
                            }

                            //copy the key
                            {
                                if((size-pos)<4)
                                {
                                    std::cerr << "C210 size key 32Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,4);
                                if(!CharactersGroupForLogin::hash.contains(charactersGroupString))
                                {
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                CharactersGroupForLogin::hash[charactersGroupString]->setServerUniqueKey(rawData+pos,4);
                                pos+=4;
                                rawServerListSize+=4;
                            }

                            //copy the host + port
                            {
                                if((size-pos)<1)
                                {
                                    std::cerr << "C210 size charactersGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &hostStringSize=rawData[pos];
                                if((size-pos)<(1+hostStringSize+2))
                                {
                                    std::cerr << "C210 size charactersGroupString + size 8Bits header + port too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
                                    std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &xmlStringSize=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                if((size-pos)<(xmlStringSize+2))
                                {
                                    std::cerr << "C210 size xmlString + size 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
                                    std::cerr << "C210 size logicalGroupString 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint8 &logicalGroupStringSize=rawData[pos];
                                if((size-pos)<(logicalGroupStringSize+1))
                                {
                                    std::cerr << "C210 size logicalGroupString + size 8Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                memcpy(rawServerList,rawData+pos,1+logicalGroupStringSize);
                                pos+=1+logicalGroupStringSize;
                                rawServerListSize+=1+logicalGroupStringSize;
                            }
                            //internal id
                            {
                                if((size-pos)<2)
                                {
                                    std::cerr << "C210 size xmlString 16Bits header too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                const quint16 &databaseInternalId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+pos)));
                                pos+=2;
                                rawServerListSize+=2;

                                if(!CharactersGroupForLogin::hash.contains(charactersGroupString))
                                {
                                    std::cerr << "C210 CharactersGroupForLogin not found (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                CharactersGroupForLogin::hash[charactersGroupString]->setServerPair(serverListIndex,databaseInternalId);
                            }

                            serverListIndex++;
                        }
                    EpollClientLoginSlave::serverPartialServerListSize=rawServerListSize;
                    if((size-pos)<(serverListSize*(sizeof(quint16)+sizeof(quint16))))
                    {
                        std::cerr << "C210 co player list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    memcpy(EpollClientLoginSlave::serverPartialServerList,rawData+pos,serverListSize*(sizeof(quint16)+sizeof(quint16)));
                    pos+=serverListSize*(sizeof(quint16)+sizeof(quint16));
                    rawServerListSize+=serverListSize*(sizeof(quint16)+sizeof(quint16));

                    if((size-pos)!=0)
                    {
                        std::cerr << "C210 remaining data (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::serverServerListSize=ProtocolParsingBase::computeFullOutcommingData(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            false,
                            #endif
                            EpollClientLoginSlave::serverServerList,
                            0xC2,0x0E,rawData,size);
                    if(EpollClientLoginSlave::serverServerListSize==0)
                    {
                        std::cerr << "EpollClientLoginMaster::serverLogicalGroupListSize==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
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
                case 0x11:
                {
                    if(size<(1+4+1+1+1+1))
                    {
                        std::cerr << "C211 base size too small (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    EpollClientLoginSlave::automatic_account_creation=rawData[0x00];
                    EpollClientLoginSlave::character_delete_time=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+1)));
                    EpollClientLoginSlave::min_character=rawData[0x05];
                    EpollClientLoginSlave::max_character=rawData[0x06];
                    EpollClientLoginSlave::max_pseudo_size=rawData[0x07];
                    EpollClientLoginSlave::max_player_monsters=rawData[0x08];
                    EpollClientLoginSlave::max_warehouse_player_monsters=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+0x09)));
                    EpollClientLoginSlave::max_player_items=rawData[0x0B];
                    EpollClientLoginSlave::max_warehouse_player_items=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+0x0C)));
                    if(EpollClientLoginSlave::min_character>EpollClientLoginSlave::max_character)
                    {
                        std::cerr << "C211 min_character>max_character (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::max_character==0)
                    {
                        std::cerr << "C211 max_character==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::max_pseudo_size<3)
                    {
                        std::cerr << "C211 max_pseudo_size<3 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(EpollClientLoginSlave::character_delete_time==0)
                    {
                        std::cerr << "C211 character_delete_time==0 (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    if(memcmp(rawData+0x0E,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroup,EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize)!=0)
                    {
                        std::cerr << "C211 different CharactersGroup registred on master server (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                        abort();
                    }
                    //dynamic data
                    int cursor=0x0E +EpollClientLoginSlave::replyToRegisterLoginServerCharactersGroupSize;
                    //skins
                    {
                        if((size-cursor)<1)
                        {
                            std::cerr << "C211 too small for skinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                            abort();
                        }
                        const quint8 &skinListSize=rawData[cursor];
                        cursor+=1;
                        quint8 skinListIndex=0;
                        while(skinListIndex<skinListSize)
                        {
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for internalId skin (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint8 &internalId=rawData[cursor];
                            cursor+=1;
                            if((size-cursor)<sizeof(quint16))
                            {
                                std::cerr << "C211 too small for databaseId skin (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint16 &databaseId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(quint16);
                            EpollServerLoginSlave::epollServerLoginSlave->setSkinPair(internalId,databaseId);
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
                        const quint8 &profileListSize=rawData[cursor];
                        cursor+=1;
                        quint8 profileListIndex=0;
                        while(profileListIndex<profileListSize)
                        {
                            EpollServerLoginSlave::LoginProfile profile;

                            //database id
                            if((size-cursor)<sizeof(quint16))
                            {
                                std::cerr << "C211 too small for databaseId profile (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            profile.databaseId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(quint16);
                            EpollServerLoginSlave::epollServerLoginSlave->setSkinPair(profileListIndex,databaseId);

                            //skin
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for forcedskinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint8 &forcedskinListSize=rawData[cursor];
                            cursor+=1;
                            quint8 forcedskinListIndex=0;
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
                            profile.cash=le64toh(*reinterpret_cast<quint64 *>(const_cast<char *>(rawData+cursor)));
                            cursor+=sizeof(quint64);

                            //monster
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for forcedskinListSize (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint8 &monsterListSize=rawData[cursor];
                            cursor+=1;
                            quint8 monsterListIndex=0;
                            while(monsterListIndex<monsterListSize)
                            {
                                EpollServerLoginSlave::LoginProfile::Monster monster;
                                //monster id
                                if((size-cursor)<sizeof(quint16))
                                {
                                    std::cerr << "C211 too small for id monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                monster.id=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(quint16);
                                //level
                                if((size-cursor)<1)
                                {
                                    std::cerr << "C211 too small for level monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                monster.level=rawData[cursor];
                                cursor+=1;
                                //captured with
                                if((size-cursor)<sizeof(quint16))
                                {
                                    std::cerr << "C211 too small for captured with monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                monster.captured_with=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(quint16);
                                //hp
                                if((size-cursor)<sizeof(quint32))
                                {
                                    std::cerr << "C211 too small for hp monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                monster.hp=le32toh(*reinterpret_cast<quint32 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(quint32);
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
                                const quint8 &skillListSize=rawData[cursor];
                                cursor+=1;
                                quint8 skillListIndex=0;
                                while(skillListIndex<skillListSize)
                                {
                                    EpollServerLoginSlave::LoginProfile::Monster::Skill skill;
                                    //skill id
                                    if((size-cursor)<sizeof(quint16))
                                    {
                                        std::cerr << "C211 too small for id monster (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                        abort();
                                    }
                                    skill.id=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                                    cursor+=sizeof(quint16);
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

                                    monster.skills << skill;
                                    skillListIndex++;
                                }

                                profile.monsters << monster;
                                monsterListIndex++;
                            }

                            //reputation
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for reputation list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint8 &reputationListSize=rawData[cursor];
                            cursor+=1;
                            quint8 reputationListIndex=0;
                            while(reputationListIndex<reputationListSize)
                            {
                                EpollServerLoginSlave::LoginProfile::Reputation reputation;

                                //reputation id
                                if((size-cursor)<sizeof(quint16))
                                {
                                    std::cerr << "C211 too small for id reputationDatabaseId (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                reputation.reputationDatabaseId=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(quint16);
                                //level
                                if((size-cursor)<1)
                                {
                                    std::cerr << "C211 too small for level reputation (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                reputation.level=rawData[cursor];
                                cursor+=1;
                                //point
                                if((size-cursor)<sizeof(qint32))
                                {
                                    std::cerr << "C211 too small for point reputation (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                reputation.point=le32toh(*reinterpret_cast<qint32 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(qint32);

                                profile.reputation << reputation;
                                reputationListIndex++;
                            }

                            //item
                            if((size-cursor)<1)
                            {
                                std::cerr << "C211 too small for item list (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                abort();
                            }
                            const quint8 &itemListSize=rawData[cursor];
                            cursor+=1;
                            quint8 itemListIndex=0;
                            while(itemListIndex<itemListSize)
                            {
                                EpollServerLoginSlave::LoginProfile::Item item;

                                //item id
                                if((size-cursor)<sizeof(quint16))
                                {
                                    std::cerr << "C211 too small for id item (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                item.id=le16toh(*reinterpret_cast<quint16 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(quint16);
                                //point
                                if((size-cursor)<sizeof(qint32))
                                {
                                    std::cerr << "C211 too small for quantity item (abort) in " << __FILE__ << ":" <<__LINE__ << std::endl;
                                    abort();
                                }
                                item.quantity=le32toh(*reinterpret_cast<qint32 *>(const_cast<char *>(rawData+cursor)));
                                cursor+=sizeof(qint32);

                                profile.items << item;
                                itemListIndex++;
                            }

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

void LoginLinkToMaster::parseFullQuery(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *rawData,const int &size)
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

void LoginLinkToMaster::parseFullReplyData(const quint8 &mainCodeType,const quint8 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    if(!have_send_protocol)
    {
        std::cerr << "parseFullReplyData() reply to unknown query: mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
        abort();
    }
    (void)data;
    (void)size;
    queryNumberList.push_back(queryNumber);
    //do the work here
    switch(mainCodeType)
    {
        case 0x02:
            switch(subCodeType)
            {
                case 0x05:
                {
                    if(selectCharacterClients.contains(queryNumber))
                    {
                        const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=selectCharacterClients.value(queryNumber);
                        if(size==32/*256/8*/)
                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->selectCharacter_ReturnToken(dataForSelectedCharacterReturn.client_query_id,data[0]);
                        else if(size==1)
                            static_cast<EpollClientLoginSlave * const>(dataForSelectedCharacterReturn.client)->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,data[0]);
                        else
                            parseNetworkReadError("main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType)+", reply size for 0205 wrong");
                        selectCharacterClients.remove(queryNumber);
                    }
                    else
                        std::cerr << "parseFullReplyData() !selectCharacterClients.contains(queryNumber): mainCodeType: " << mainCodeType << ", subCodeType: " << subCodeType << ", queryNumber: " << queryNumber << std::endl;
                }
                break;
                default:
                    parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType)+", with sub ident:"+QString::number(subCodeType));
                    return;
                break;
            }
        break;
        default:
            parseNetworkReadError("unknown main ident: "+QString::number(mainCodeType));
            return;
        break;
    }
    parseNetworkReadError(QStringLiteral("The server for now not ask anything: %1 %2, %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

void LoginLinkToMaster::parseNetworkReadError(const QString &errorString)
{
    errorParsingLayer(errorString);
}
