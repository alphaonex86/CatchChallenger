#include "BaseServer.hpp"
#include <cstring>
#include "../GlobalServerData.hpp"

#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CatchChallenger_Hash.hpp"
#include "../Client.hpp"

using namespace CatchChallenger;

void BaseServer::preload_30_sync_other()
{
    #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
    //game server only, the server data message
    {
        if(CommonSettingsServer::commonSettingsServer.exportedXml.size()>(1024-64))
        {
            std::cerr << "The server is alonem why do you need more than 900 char for description? Limited to 900 to limit the memory usage" << std::endl;
            abort();
        }
        char logicalGroup[64];
        Client::protocolMessageLogicalGroupAndServerListSize=0;
        //0x44
        {
            //no logical group
            //send the network message
            logicalGroup[0x00]=0x44;
            //size
            logicalGroup[0x01]=0x04;
            logicalGroup[0x02]=0x00;
            logicalGroup[0x03]=0x00;
            logicalGroup[0x04]=0x00;
            //one logical group
            logicalGroup[0x05]=0x01;
            //empty string
            logicalGroup[0x06]=0x00;
            logicalGroup[0x07]=0x00;
            logicalGroup[0x08]=0x00;

            Client::protocolMessageLogicalGroupAndServerListSize+=9;
        }
        uint32_t posOutput=0;
        //0x40
        {
            //send the network message
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x40;
            posOutput+=1+4;

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x02;//Server mode, unique then proxy
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;//server list size, only this alone server
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;//charactersgroup empty
            posOutput+=1;
            {const uint32_t _tmp_le=(0x00000000);memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}//unique key, useless here
            posOutput+=4;
            {
                const std::string &text=CommonSettingsServer::commonSettingsServer.exportedXml;
                {const uint16_t _tmp_le=(htole16(text.size()));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=2;
                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                posOutput+=text.size();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;//logical group empty
            posOutput+=1;
            if(GlobalServerData::serverSettings.sendPlayerNumber)
            {
                {const uint16_t _tmp_le=(htole16(GlobalServerData::serverSettings.max_players));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=2;
                Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber=Client::protocolMessageLogicalGroupAndServerListSize+static_cast<uint16_t>(posOutput);
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber==0)
                {
                    std::cerr << "Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber==0 corruption detected" << std::endl;
                    abort();
                }
                #endif
                {const uint16_t _tmp_le=(0);memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=2;
            }
            else
            {
                if(GlobalServerData::serverSettings.max_players<=255)
                    {const uint16_t _tmp_le=(htole16(65534));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                else
                    {const uint16_t _tmp_le=(htole16(65535));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=2;
                //allow onfly change GlobalServerData::serverSettings.sendPlayerNumber
                Client::protocolMessageLogicalGroupAndServerListPosPlayerNumber=Client::protocolMessageLogicalGroupAndServerListSize+static_cast<uint16_t>(posOutput);
                {const uint16_t _tmp_le=(htole16(0));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

                posOutput+=2;
            }
            Client::protocolMessageLogicalGroupAndServerListSize+=posOutput;
            {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
        }
        if(Client::protocolMessageLogicalGroupAndServerList!=NULL)
            delete Client::protocolMessageLogicalGroupAndServerList;
        Client::protocolMessageLogicalGroupAndServerList=(unsigned char *)malloc(Client::protocolMessageLogicalGroupAndServerListSize+16);
        memset(Client::protocolMessageLogicalGroupAndServerList,0,Client::protocolMessageLogicalGroupAndServerListSize+16);
        memcpy(Client::protocolMessageLogicalGroupAndServerList,logicalGroup,9);
        memcpy(Client::protocolMessageLogicalGroupAndServerList+9,ProtocolParsingBase::tempBigBufferForOutput,Client::protocolMessageLogicalGroupAndServerListSize);
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(Client::protocolMessageLogicalGroupAndServerList[0]!=0x44 && Client::protocolMessageLogicalGroupAndServerList[9]!=0x40)
    {
        std::cerr << "Client::protocolMessageLogicalGroupAndServerList corruption detected" << std::endl;
        abort();
    }
    #endif

    //charater list reply header
    {
        //send the network reply
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1+1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=01;//all is good
        posOutput+=1;

        //login/common part
        {const uint32_t _tmp_le=(htole32(CommonSettingsCommon::commonSettingsCommon.character_delete_time));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.max_character;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.min_character;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.max_pseudo_size;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsCommon::commonSettingsCommon.maxPlayerMonsters;
        posOutput+=1;
        {const uint16_t _tmp_le=(htole16(CommonSettingsCommon::commonSettingsCommon.maxWarehousePlayerMonsters));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=2;

        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=CATCHCHALLENGER_HASH_SIZE)
        {
            std::cerr << "CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=CATCHCHALLENGER_HASH_SIZE into BaseServer::preload_other()" << std::endl;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            abort();
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            CommonSettingsCommon::commonSettingsCommon.datapackHashBase.resize(CATCHCHALLENGER_HASH_SIZE);
            #elif CATCHCHALLENGER_DB_FILE
            abort();
            #else
            #error Define what do here
            #endif
        }
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsCommon::commonSettingsCommon.datapackHashBase.data(),CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size());
        posOutput+=CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size();
        {
            const std::string &text=CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase;
            if(CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.size()>255)
            {
                std::cerr << "CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.size()>255" << std::endl;
                abort();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }

        if(posOutput>65535)
        {
            std::cerr << "preload_other() posOutput>65535 abort" << std::endl;
            abort();
        }
        Client::protocolReplyCharacterListSize=static_cast<uint16_t>(posOutput);

        if(Client::protocolReplyCharacterList!=NULL)
            delete Client::protocolReplyCharacterList;
        Client::protocolReplyCharacterList=(unsigned char *)malloc(Client::protocolReplyCharacterListSize+16);
        memcpy(Client::protocolReplyCharacterList,ProtocolParsingBase::tempBigBufferForOutput,Client::protocolReplyCharacterListSize);
        //std::cout << "protocolReplyCharacterList: " << binarytoHexa(Client::protocolReplyCharacterList,Client::protocolReplyCharacterListSize) << std::endl;
    }
    #endif




    //load the character reply header
    {
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CATCHCHALLENGER_PROTOCOL_REPLY_SERVER_TO_CLIENT;
        posOutput+=1+1+4;

        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;// all is good
        posOutput+=1;

        if(GlobalServerData::serverSettings.sendPlayerNumber)
            {const uint16_t _tmp_le=(htole16(GlobalServerData::serverSettings.max_players));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        else
        {
            if(GlobalServerData::serverSettings.max_players<=255)
                {const uint16_t _tmp_le=(htole16(65534));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            else
                {const uint16_t _tmp_le=(htole16(65535));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        }
        posOutput+=2;
        {const uint32_t _tmp_le=(0xffffffff);memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}//-1 to disable, set at sending time
        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=GlobalServerData::serverSettings.city.capture.frenquency;
        posOutput+=1;

        //common settings
        {const uint32_t _tmp_le=(htole32(CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.dontSendPseudo;
        posOutput+=1;
        {const uint32_t _tmp_le=(htole32(CommonSettingsServer::commonSettingsServer.rates_xp));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        {const uint32_t _tmp_le=(htole32(CommonSettingsServer::commonSettingsServer.rates_gold));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        {const uint32_t _tmp_le=(htole32(CommonSettingsServer::commonSettingsServer.rates_xp_pow));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        {const uint32_t _tmp_le=(htole32(CommonSettingsServer::commonSettingsServer.rates_drop));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

        posOutput+=4;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_all;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_local;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_private;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.chat_allow_clan;
        posOutput+=1;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.factoryPriceChange;
        posOutput+=1;

        //Main type code
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.mainDatapackCode;
            if(text.size()>255)
            {
                std::cerr << "preload_other() CommonSettingsServer::commonSettingsServer.mainDatapackCode size > 255 abort" << std::endl;
                abort();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        //Sub type code
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.subDatapackCode;
            if(text.size()>255)
            {
                std::cerr << "preload_other() CommonSettingsServer::commonSettingsServer.subDatapackCode size > 255 abort" << std::endl;
                abort();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=CATCHCHALLENGER_HASH_SIZE)
        {
            std::cerr << "CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size()!=CATCHCHALLENGER_HASH_SIZE" << std::endl;
            #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
            abort();
            #elif CATCHCHALLENGER_DB_BLACKHOLE
            CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(CATCHCHALLENGER_HASH_SIZE);
            #elif CATCHCHALLENGER_DB_FILE
            abort();
            #else
            #error Define what do here
            #endif
        }
        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=CATCHCHALLENGER_HASH_SIZE)
        {
            std::cerr << "CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=CATCHCHALLENGER_HASH_SIZE into BaseServer::preload_other()" << std::endl;
            abort();
        }
        //main hash
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),CATCHCHALLENGER_HASH_SIZE);
        posOutput+=CATCHCHALLENGER_HASH_SIZE;
        //sub hash
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=CATCHCHALLENGER_HASH_SIZE)
            {
                std::cerr << "CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size()!=CATCHCHALLENGER_HASH_SIZE" << std::endl;
                #if defined(CATCHCHALLENGER_DB_MYSQL) || defined(CATCHCHALLENGER_DB_POSTGRESQL) || defined(CATCHCHALLENGER_DB_SQLITE)
                abort();
                #elif CATCHCHALLENGER_DB_BLACKHOLE
                CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(CATCHCHALLENGER_HASH_SIZE);
                #elif CATCHCHALLENGER_DB_FILE
                abort();
                #else
                #error Define what do here
                #endif
            }
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),CATCHCHALLENGER_HASH_SIZE);
            posOutput+=CATCHCHALLENGER_HASH_SIZE;
        }
        //mirror
        {
            const std::string &text=CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer;
            if(text.size()>255)
            {
                std::cerr << "preload_other() CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer size > 255 abort" << std::endl;
                abort();
            }
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
            posOutput+=1;
            memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
            posOutput+=text.size();
        }
        //everyBodyIsRoot
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=CommonSettingsServer::commonSettingsServer.everyBodyIsRoot;
        posOutput+=1;


        if(Client::characterIsRightFinalStepHeader!=NULL)
            delete Client::characterIsRightFinalStepHeader;
        Client::characterIsRightFinalStepHeaderSize=posOutput;
        Client::characterIsRightFinalStepHeader=(unsigned char *)malloc(Client::characterIsRightFinalStepHeaderSize+16);
        memcpy(Client::characterIsRightFinalStepHeader,ProtocolParsingBase::tempBigBufferForOutput,Client::characterIsRightFinalStepHeaderSize);
    }
}
