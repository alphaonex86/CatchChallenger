#include "../Client.hpp"
#include "../GlobalServerData.hpp"
#include "../DictionaryServer.hpp"
#include "../../general/base/FacilityLib.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"

using namespace CatchChallenger;

void Client::characterIsRightFinalStep()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    stat=ClientStat::CharacterSelected;
    Client::timeRangeEventNew=true;
    checkLoose(false);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(selectCharacterQueryId.empty())
        abort();
    #endif
    const uint8_t /*do ASAN crash/bug if ref enabled here*/query_id=selectCharacterQueryId.front();
    selectCharacterQueryId.erase(selectCharacterQueryId.begin());

    //send the network reply
    removeFromQueryReceived(query_id);


    uint32_t posOutput=0;
    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,Client::characterIsRightFinalStepHeader,Client::characterIsRightFinalStepHeaderSize);
    if(Client::characterIsRightFinalStepHeaderSize>100000)
        std::cerr << "strange Client::characterIsRightFinalStepHeaderSize is bigger than 100K" << std::endl;
    ProtocolParsingBase::tempBigBufferForOutput[0x01]=query_id;
    posOutput+=Client::characterIsRightFinalStepHeaderSize;

    //city capture
    {
        uint32_t remainingTimeSToCityCapture=0;
        const uint64_t &timeactual=time(NULL);
        if(timeactual<GlobalServerData::serverPrivateVariables.time_city_capture)
            remainingTimeSToCityCapture=GlobalServerData::serverPrivateVariables.time_city_capture-timeactual;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+sizeof(uint8_t)+sizeof(uint8_t)+sizeof(uint32_t)+sizeof(uint8_t)+sizeof(uint16_t),&remainingTimeSToCityCapture,sizeof(remainingTimeSToCityCapture));
    }

    /// \todo optimise and cache this block
    //send the event
    {
        std::vector<std::pair<uint8_t,uint8_t> > events;
        unsigned int index=0;
        while(index<GlobalServerData::serverPrivateVariables.events.size())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(GlobalServerData::serverPrivateVariables.events.size()<=index)
            {
                std::cerr << "GlobalServerData::serverPrivateVariables.events.size()<=index" << std::endl;
                abort();
            }
            #endif
            const uint8_t &value=GlobalServerData::serverPrivateVariables.events.at(index);
            if(value!=0)
                events.push_back(std::pair<uint8_t,uint8_t>(index,value));
            index++;
        }
        index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(events.size());
        posOutput+=1;
        while(index<events.size())
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(events.size()<=index)
            {
                std::cerr << "events.size()<=index" << std::endl;
                abort();
            }
            #endif
            const std::pair<uint8_t,uint8_t> &event=events.at(index);
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=event.first;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=event.second;
            posOutput+=1;
            index++;
        }
    }

    //temporary character id
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.public_informations.simplifiedId);
        posOutput+=1;
    }
    else
    {
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.public_informations.simplifiedId);
        posOutput+=2;
    }

    //pseudo
    {
        const std::string &text=public_and_private_informations.public_informations.pseudo;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(text.size());
        posOutput+=1;
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
        posOutput+=text.size();
    }
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.allow.size());
    posOutput+=1;
    {
        auto i=public_and_private_informations.allow.begin();
        while(i!=public_and_private_informations.allow.cend())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=*i;
            posOutput+=1;
            ++i;
        }
    }

    //clan related
    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(public_and_private_informations.clan);
    posOutput+=4;
    if(public_and_private_informations.clan_leader)
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
    else
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x00;
    posOutput+=1;

    {
        const uint64_t cash=htole64(public_and_private_informations.cash);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash,8);
        posOutput+=8;
    }
    {
        const uint64_t cash=htole64(public_and_private_informations.warehouse_cash);
        memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&cash,8);
        posOutput+=8;
    }

    //temporary variable
    uint32_t index;
    uint8_t size;

    //send monster
    index=0;
    size=static_cast<uint8_t>(public_and_private_informations.playerMonster.size());
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=size;
    posOutput+=1;
    while(index<size)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(public_and_private_informations.playerMonster.size()<=index)
        {
            std::cerr << "public_and_private_informations.playerMonster.size()<=index" << std::endl;
            abort();
        }
        #endif
        posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.playerMonster.at(index),character_id_db);
        index++;
    }
    index=0;
    size=static_cast<uint8_t>(public_and_private_informations.warehouse_playerMonster.size());
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=size;
    posOutput+=1;
    while(index<size)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(public_and_private_informations.warehouse_playerMonster.size()<=index)
        {
            std::cerr << "public_and_private_informations.warehouse_playerMonster.size()<=index" << std::endl;
            abort();
        }
        #endif
        posOutput+=FacilityLib::privateMonsterToBinary(ProtocolParsingBase::tempBigBufferForOutput+posOutput,public_and_private_informations.warehouse_playerMonster.at(index),character_id_db);
        index++;
    }

    //send reputation
    {
        unsigned int index=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(public_and_private_informations.reputation.size());
        posOutput+=1;
        auto i=public_and_private_informations.reputation.begin();
        while(index<=255 && i!=public_and_private_informations.reputation.cend())
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->first;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=i->second.level;
            posOutput+=1;
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(i->second.point);
            posOutput+=4;
            ++i;
            index++;
        }
    }
    /// \todo make the buffer overflow control here or above
    {
        char *buffer;
        if(CompressionProtocol::compressionTypeServer==CompressionProtocol::CompressionType::None)
            buffer=ProtocolParsingBase::tempBigBufferForOutput+posOutput+4;
        else
            buffer=CompressionProtocol::tempBigBufferForCompressedOutput;
        uint32_t posOutputTemp=0;

        //recipes
        if(public_and_private_informations.recipes!=NULL)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.get_craftingRecipesMaxId()==0)
            {
                errorOutput("CommonDatapack::commonDatapack.crafingRecipesMaxId==0");
                return;
            }
            #endif
            const auto &binarySize=CommonDatapack::commonDatapack.get_craftingRecipesMaxId()/8+1;
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(binarySize);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.recipes,binarySize);
            posOutputTemp+=binarySize;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }

        //encyclopedia_monster
        if(public_and_private_informations.encyclopedia_monster!=NULL)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.get_monstersMaxId()==0)
            {
                errorOutput("CommonDatapack::commonDatapack.monstersMaxId==0");
                return;
            }
            #endif
            const auto &binarySize=CommonDatapack::commonDatapack.get_monstersMaxId()/8+1;
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(binarySize);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.encyclopedia_monster,binarySize);
            posOutputTemp+=binarySize;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }

        //encyclopedia_item
        if(public_and_private_informations.encyclopedia_item!=NULL)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(CommonDatapack::commonDatapack.get_items().itemMaxId==0)
            {
                errorOutput("CommonDatapack::commonDatapack.items.itemMaxId==0");
                return;
            }
            #endif
            const auto &binarySize=CommonDatapack::commonDatapack.get_items().itemMaxId/8+1;
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(binarySize);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.encyclopedia_item,binarySize);
            posOutputTemp+=binarySize;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }

        //achievements
        buffer[posOutputTemp]=0;
        posOutputTemp++;

        if(CompressionProtocol::compressionTypeServer==CompressionProtocol::CompressionType::None)
        {
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(posOutputTemp);
            posOutput+=4;
            posOutput+=posOutputTemp;
        }
        else
        {
            //compress
            const int32_t &compressedSize=CompressionProtocol::computeCompression(buffer,ProtocolParsingBase::tempBigBufferForOutput+posOutput+4,posOutputTemp,sizeof(ProtocolParsingBase::tempBigBufferForOutput)-posOutput-1-4,CompressionProtocol::compressionTypeServer);
            if(compressedSize<0)
            {
                errorOutput("Error to compress the data");
                return;
            }
            //copy
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(compressedSize);
            posOutput+=4;
            posOutput+=compressedSize;
        }
    }

    //------------------------------------------- End of common part, start of server specific part ----------------------------------

    //send quest
    {
        unsigned int index=0;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.quests.size());
        posOutput+=2;
        auto j=public_and_private_informations.quests.begin();
        while(index<=255 && j!=public_and_private_informations.quests.cend())
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(j->first);
            posOutput+=2;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=j->second.step;
            posOutput+=1;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=j->second.finish_one_time;
            posOutput+=1;
            ++j;
            index++;
        }
    }

    {
        char *buffer;
        if(CompressionProtocol::compressionTypeServer==CompressionProtocol::CompressionType::None)
            buffer=ProtocolParsingBase::tempBigBufferForOutput+posOutput+4;
        else
            buffer=CompressionProtocol::tempBigBufferForCompressedOutput;
        uint32_t posOutputTemp=0;
        //bot
        if(public_and_private_informations.bot_already_beaten!=NULL)
        {
            const auto &binarySize=CommonDatapackServerSpec::commonDatapackServerSpec.get_botFightsMaxId()/8+1;
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=htole16(binarySize);
            posOutputTemp+=2;
            memcpy(buffer+posOutputTemp,public_and_private_informations.bot_already_beaten,binarySize);
            posOutputTemp+=binarySize;
        }
        else
        {
            *reinterpret_cast<int16_t *>(buffer+posOutputTemp)=0;
            posOutputTemp+=2;
        }

        if(CompressionProtocol::compressionTypeServer==CompressionProtocol::CompressionType::None)
        {
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(posOutputTemp);
            posOutput+=4;
            posOutput+=posOutputTemp;
        }
        else
        {
            //compress
            const int32_t &compressedSize=CompressionProtocol::computeCompression(buffer,ProtocolParsingBase::tempBigBufferForOutput+posOutput+4,posOutputTemp,sizeof(ProtocolParsingBase::tempBigBufferForOutput)-posOutput-1-4,CompressionProtocol::compressionTypeServer);
            if(compressedSize<0)
            {
                errorOutput("Error to compress the data");
                return;
            }
            //copy
            *reinterpret_cast<int32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(compressedSize);
            posOutput+=4;
            posOutput+=compressedSize;
        }
    }

    /*std::cout << "DictionaryServer::dictionary_pointOnMap_item_database_to_internal: " << DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size() << std::endl;
    for(unsigned int i=0; i<DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size(); i++)
    {
        const DictionaryServer::MapAndPointItem &t=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(i);
        std::cerr << "DictionaryServer::MapAndPointItem &t " << &t << std::endl;
        if(t.map!=nullptr)
            std::cerr << t.datapack_index_item << " " << t.map->id << " " << t.x << " " << t.y << " " << std::endl;
    }*/

    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.itemOnMap.size());
    posOutput+=2;
    {
        auto i=public_and_private_informations.itemOnMap.begin();
        while (i!=public_and_private_informations.itemOnMap.cend())
        {
            /** \warning can have entry in database but not into datapack, deleted. Used only to send to player the correct pos */
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(DictionaryServer::dictionary_pointOnMap_item_database_to_internal.size()<=*i)
            {
                std::cerr << "DictionaryServer::dictionary_pointOnMap_item_database_to_internal<=*i" << std::endl;
                abort();
            }
            #endif
            const uint16_t &pointOnMapOnlyIntoDatapackIndex=DictionaryServer::dictionary_pointOnMap_item_database_to_internal.at(*i).datapack_index_item;
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(pointOnMapOnlyIntoDatapackIndex);
            posOutput+=2;
            ++i;
        }
    }

    const auto &time=sFrom1970();
    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(public_and_private_informations.plantOnMap.size());
    posOutput+=2;
    auto i=public_and_private_informations.plantOnMap.begin();
    while(i!=public_and_private_informations.plantOnMap.cend())
    {
        /** \warning can have entry in database but not into datapack, deleted. Used only to send to player the correct pos */
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size()<=i->first)
        {
            std::cerr << "DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.size()<=i->first" << std::endl;
            abort();
        }
        #endif
        const uint16_t &pointOnMapOnlyIntoDatapackIndex=DictionaryServer::dictionary_pointOnMap_plant_database_to_internal.at(i->first).datapack_index_plant;
        const PlayerPlant &playerPlant=i->second;
        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(pointOnMapOnlyIntoDatapackIndex);
        posOutput+=2;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=playerPlant.plant;
        posOutput+=1;
        /// \todo Can essaylly int 16 ovbertflow
        if(time<playerPlant.mature_at)
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(playerPlant.mature_at-time);
        else
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=0;
        posOutput+=2;
        ++i;
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif


    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1+1)=htole32(posOutput-1-1-4);//set the dynamic size
    if(posOutput>100000 || posOutput>sizeof(ProtocolParsingBase::tempBigBufferForOutput))
        std::cerr << "strange output is bigger than 100K" << std::endl;
    if(!sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput))
        return;

    if(this->map==NULL)
        return;

    if(!sendInventory())
        return;
    updateCanDoFight();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(this->map==NULL)
        return;
    {
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    //send signals into the server
    #ifndef SERVERBENCHMARK
    if(this->map==NULL)
        return;
    /*normalOutput("Logged: "+public_and_private_informations.public_informations.pseudo+
                 " on the map: "+map->map_file+
                 " ("+std::to_string(x)+","+std::to_string(y)+")");*/
    #endif

    #ifdef DEBUG_MESSAGE_CLIENT_COMPLEXITY_LINEARE
    normalOutput("load the normal player id: "+std::to_string(character_id)+", simplified id: "+std::to_string(public_and_private_informations.public_informations.simplifiedId));
    #endif
    GlobalServerData::serverPrivateVariables.connected_players++;
    if(GlobalServerData::serverSettings.sendPlayerNumber)
        GlobalServerData::serverPrivateVariables.player_updater->addConnectedPlayer();
    playerByPseudo[public_and_private_informations.public_informations.pseudo]=this;
    clientBroadCastList.push_back(this);

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(this->map==NULL)
            return;
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(this->map==NULL)
        return;
    //need be before send monster because can be teleported for 0 hp
    put_on_the_map(
                map,//map pointer
        x,
        y,
        static_cast<Orientation>(last_direction)
    );

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    {
        if(this->map==NULL)
            return;
        std::string mapToDebug=this->map->map_file;
        mapToDebug+=this->map->map_file;
    }
    #endif

    if(GlobalServerData::serverSettings.sendPlayerNumber)
    {
        ProtocolParsingBase::tempBigBufferForOutput[0x00]=0x64;
        uint8_t outputSize;
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[0x01]=static_cast<uint8_t>(last_sended_connected_players);
            outputSize=2;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+0x01)=htole16(last_sended_connected_players);
            outputSize=3;
        }
        //can't use receive_instant_player_number() due this->connected_players==connected_players
        if(posOutput>100000 || posOutput>sizeof(ProtocolParsingBase::tempBigBufferForOutput))
            std::cerr << "strange output is bigger than 100K" << std::endl;
        sendRawBlock(reinterpret_cast<char *>(ProtocolParsingBase::tempBigBufferForOutput),outputSize);
    }
}
