#include "Map_server_MapVisibility_Simple_StoreOnSender.hpp"
#include "MapVisibilityAlgorithm_Simple_StoreOnSender.hpp"
#include "../GlobalServerData.hpp"
#include "../Client.hpp"

/// \todo optimise: preserialize, filter streaming to send + filter
/** How use SIMD and better performance:
 *  Compose the full list with size-1
 *  char data[]
 *  int index=0;
 *  while()
 *    current client=list.at(index)
 *    sendRaw(data+0,data+pos)
 *    //skip current client pos
 *    sendRaw(data+pos+offset,size-offet-pos)
 * see line 560, after if(!GlobalServerData::serverSettings.mapVisibility.simple.reemit)
*/

using namespace CatchChallenger;
force reemit true

MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataNewClients[65535];
MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataOldClients[65535];
Map_server_MapVisibility_Simple_StoreOnSender ** Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=NULL;
uint32_t Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender() :
    to_send_remove_size(0),
    show(true),
    to_send_insert(false),
    send_drop_all(false),
    send_reinsert_all(false),
    have_change(false)
{
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_dropAll()
{
    unsigned const char mainCode[]={0x65};
    unsigned int index=0;
    while(index<clients.size())
    {
        MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients[index];
        //clientdropAllClients();
        if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
            client->sendRawBlock(reinterpret_cast<const char *>(mainCode),sizeof(mainCode));
        else
            client->mapSyncMiss=true;
        index++;
    }
    send_drop_all=false;
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_reinsertAll()
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
    posOutput+=1+4;
    //prepare the data
    {
        //////////////////////////// insert //////////////////////////
        // can be only this map with this algo, then 1 map
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
        posOutput+=1;
        if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(id);
            posOutput+=1;
        }
        else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(id);
            posOutput+=2;
        }
        else
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(id);
            posOutput+=4;
        }
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(clients.size()-1);
            posOutput+=1;
        }
        else
        {
            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.size()-1);
            posOutput+=2;
        }
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
        {
            if(clients.size()>1)
            {
                unsigned int index=0;
                while(index<clients.size())
                {
                    MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index);
                    posOutput+=playerToFullInsert(client,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                    ++index;
                }
                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                index=0;
                while(index<clients.size())
                {
                    MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index);
                    client->to_send_insert=false;
                    if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
                    {
                        #ifdef CATCHCHALLENGER_EXTRA_CHECK
                        MapServer::check6B(ProtocolParsingBase::tempBigBufferForOutput+1+4,posOutput-1-4);
                        #endif
                        client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    }
                    else
                        client->mapSyncMiss=true;
                    ++index;
                }
            }
        }
        else
        {
            unsigned int index=0;
            while(index<clients.size())
            {
                MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index);

                unsigned int indexSub=0;
                while(indexSub<clients.size())
                {
                    if(index!=indexSub)
                        posOutput+=playerToFullInsert(client,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                    ++indexSub;
                }

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                client->to_send_insert=false;
                if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    MapServer::check6B(ProtocolParsingBase::tempBigBufferForOutput+1+4,posOutput-1-4);
                    #endif
                    client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                else
                    client->mapSyncMiss=true;

                ++index;
            }
        }
    }

    send_pingAll();
    send_reinsert_all=false;
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_pingAll()
{
    unsigned int index=0;
    while(index<clients.size())
    {
        MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index);
        client->sendPing();
        index++;
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insert(unsigned int &clientsToSendDataSizeNewClients, unsigned int &clientsToSendDataSizeOldClients)
{
    //insert new on old
    {
        //count the player will be insered
        int insert_player=0;
        {
            unsigned int index=0;
            while(index<clients.size())
            {
                if(clients.at(index)->to_send_insert)
                    insert_player++;
                index++;
            }
        }
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
        posOutput+=1+4;

        if(insert_player>0)
        {
            {
                //////////////////////////// insert //////////////////////////
                /* can be only this map with this algo, then 1 map */
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                posOutput+=1;
                if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(id);
                    posOutput+=1;
                }
                else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(id);
                    posOutput+=2;
                }
                else
                {
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(id);
                    posOutput+=4;
                }
                if(GlobalServerData::serverSettings.max_players<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(insert_player);
                    posOutput+=1;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(insert_player);
                    posOutput+=2;
                }
                unsigned int index=0;
                while(index<clients.size())
                {
                    MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index);
                    if(client->to_send_insert)
                        posOutput+=playerToFullInsert(clients.at(index),ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                    ++index;
                }
            }

            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

            //send the packet
            {
                unsigned int index=0;
                while(index<clients.size())
                {
                    MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index);
                    if(client->to_send_insert)
                    {
                        clientsToSendDataNewClients[clientsToSendDataSizeNewClients]=client;
                        clientsToSendDataSizeNewClients++;
                    }
                    else
                    {
                        if(client->pingCountInProgress()<=0 && client->mapSyncMiss==false)
                        {
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            MapServer::check6B(ProtocolParsingBase::tempBigBufferForOutput+1+4,posOutput-1-4);
                            #endif
                            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        }
                        else
                            client->mapSyncMiss=true;
                        clientsToSendDataOldClients[clientsToSendDataSizeOldClients]=client;
                        clientsToSendDataSizeOldClients++;
                    }
                    index++;
                }
            }
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::cerr << "insert_player count is null! clients.size(): " << std::to_string(clients.size()) << std::endl;

            unsigned int index=0;
            while(index<clients.size())
            {
                const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index);
                std::cerr << "- " << client->public_and_private_informations.public_informations.pseudo
                          << " to_send_insert: " << std::to_string(client->to_send_insert)
                          << " haveNewMove: " << std::to_string(client->haveNewMove)
                          << std::endl;
                index++;
            }

            if(to_send_remove.size()>0)
            {
                std::cerr << "to_send_remove:";
                index=0;
                while(index<to_send_remove.size())
                {
                    std::cerr << " " << std::to_string(to_send_remove.at(index));
                    index++;
                }
                std::cerr << std::endl;
            }

            std::cerr << "to_send_remove_size: " << std::to_string(to_send_remove_size) << std::endl;
            std::cerr << "show: " << std::to_string(show) << std::endl;
            std::cerr << "to_send_insert: " << std::to_string(to_send_insert) << std::endl;
            std::cerr << "send_drop_all: " << std::to_string(send_drop_all) << std::endl;
            std::cerr << "send_reinsert_all: " << std::to_string(send_reinsert_all) << std::endl;
            std::cerr << "have_change: " << std::to_string(have_change) << std::endl;
            #else
            std::cerr << "insert_player count is null! clients.size(): " << std::to_string(clients.size()) << std::endl;
            #endif
        }
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insert_exclude()
{
    //insert old + new (excluding them self) on new
    {
        //count the player will be insered
        unsigned int index=0;
        while(index<clients.size())/// \todo optimize, use clientsToSendDataNewClients
        {
            if(clients.at(index)->to_send_insert)
            {
                //send the network message
                uint32_t posOutput=0;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
                posOutput+=1+4;

                //////////////////////////// insert //////////////////////////
                /* can be only this map with this algo, then 1 map */
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
                posOutput+=1;
                if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(id);
                    posOutput+=1;
                }
                else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(id);
                    posOutput+=2;
                }
                else
                {
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(id);
                    posOutput+=4;
                }
                if(GlobalServerData::serverSettings.max_players<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(clients.size()-1);
                    posOutput+=1;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.size()-1);
                    posOutput+=2;
                }
                unsigned int index_subindex=0;
                while(index_subindex<clients.size())/// \todo optimize, use clientsToSendDataNewClients (exclude) + clientsToSendDataOldClients
                {
                    const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index_subindex);
                    if(index!=index_subindex)
                        posOutput+=playerToFullInsert(client,ProtocolParsingBase::tempBigBufferForOutput+posOutput);
                    ++index_subindex;
                }

                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

                if(clients.at(index)->pingCountInProgress()<=0 && clients.at(index)->mapSyncMiss==false)
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    MapServer::check6B(ProtocolParsingBase::tempBigBufferForOutput+1+4,posOutput-1-4);
                    #endif
                    clients.at(index)->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                else
                    clients.at(index)->mapSyncMiss=true;
            }
            index++;
        }
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_remove(unsigned int &clientsToSendDataSizeOldClients)
{
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x69;
    posOutput+=1+4;

    if(GlobalServerData::serverSettings.max_players<=255)
    {
        if((sizeof(uint8_t)+to_send_remove.size()*sizeof(uint8_t))<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(sizeof(uint8_t)+to_send_remove.size()*sizeof(uint8_t));//set the dynamic size

            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=static_cast<uint8_t>(to_send_remove.size());
            posOutput+=1;
            unsigned int index_subindex=0;
            while(index_subindex<to_send_remove.size())
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)to_send_remove.at(index_subindex);
                posOutput+=1;
                index_subindex++;
            }
            index_subindex=0;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->pingCountInProgress()<=0 && clientsToSendDataOldClients[index_subindex]->mapSyncMiss==false)
                    clientsToSendDataOldClients[index_subindex]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                else
                    clientsToSendDataOldClients[index_subindex]->mapSyncMiss=true;
                index_subindex++;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
            std::cerr << "Out of buffer for map management to send remove" << __LINE__ << std::endl;
        #endif
    }
    else
    {
        if((sizeof(uint16_t)+to_send_remove.size()*sizeof(uint16_t))<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
        {
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(sizeof(uint16_t)+to_send_remove.size()*sizeof(uint16_t));//set the dynamic size

            *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_remove.size());
            posOutput+=2;
            unsigned int index_subindex=0;
            while(index_subindex<to_send_remove.size())
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_remove.at(index_subindex));
                posOutput+=2;
                index_subindex++;
            }
            index_subindex=0;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->pingCountInProgress()<=0 && clientsToSendDataOldClients[index_subindex]->mapSyncMiss==false)
                    clientsToSendDataOldClients[index_subindex]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                else
                    clientsToSendDataOldClients[index_subindex]->mapSyncMiss=true;
                index_subindex++;
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
            std::cerr << "Out of buffer for map management to send remove on 16bits" << __LINE__ << std::endl;
        #endif
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_samllreinsert_reemit(unsigned int &clientsToSendDataSizeOldClients)
{
    uint16_t index_subindex=0;
    uint16_t real_reinsert_count=0;
    unsigned int bufferSizeToHave;
    while(index_subindex<clientsToSendDataSizeOldClients)
    {
        if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
            real_reinsert_count++;
        index_subindex++;
    }
    if(GlobalServerData::serverSettings.max_players<=255)
        bufferSizeToHave=sizeof(uint8_t)+real_reinsert_count*(sizeof(uint8_t)+sizeof(uint8_t)*3);
    else
        bufferSizeToHave=sizeof(uint16_t)+real_reinsert_count*(sizeof(uint16_t)+sizeof(uint8_t)*3);
    if(bufferSizeToHave<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
    {
        if(real_reinsert_count>0)
        {
            //send the network message
            uint32_t posOutput=0;
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x66;
            posOutput+=1+4;

            //////////////////////////// insert //////////////////////////
            if(GlobalServerData::serverSettings.max_players<=255)
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)real_reinsert_count;
                posOutput+=sizeof(uint8_t);
            }
            else
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint16_t)htole16((uint16_t)real_reinsert_count);
                posOutput+=sizeof(uint16_t);
            }
            index_subindex=0;
            if(GlobalServerData::serverSettings.max_players<=255)
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clientsToSendDataOldClients[index_subindex];
                    if(client->haveNewMove)
                    {
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+0]=(uint8_t)client->public_and_private_informations.public_informations.simplifiedId;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+1]=(uint8_t)client->getX();
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+2]=(uint8_t)client->getY();
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+3]=(uint8_t)client->getLastDirection();
                        posOutput+=sizeof(uint8_t)+sizeof(uint8_t)*3;
                    }
                    index_subindex++;
                }
            else
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clientsToSendDataOldClients[index_subindex];
                    if(client->haveNewMove)
                    {
                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint16_t)htole16((uint16_t)client->public_and_private_informations.public_informations.simplifiedId);
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+0]=(uint8_t)client->getX();
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+1]=(uint8_t)client->getY();
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+2]=(uint8_t)client->getLastDirection();
                        posOutput+=sizeof(uint16_t)+sizeof(uint8_t)*3;
                    }
                    index_subindex++;
                }
            *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
            unsigned int index=0;
            while(index<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index]->pingCountInProgress()<=0 && clientsToSendDataOldClients[index]->mapSyncMiss==false)
                    clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                else
                    clientsToSendDataOldClients[index]->mapSyncMiss=true;
                index++;
            }
        }
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    else
        std::cerr << "Out of buffer for map management to send reinsert" << __LINE__ << "bufferSizeToHave" << bufferSizeToHave << "CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER" << CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER << std::endl;
    #endif
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_samllreinsert(unsigned int &clientsToSendDataSizeOldClients)
{
    unsigned int real_reinsert_count=0;
    unsigned int index_subindex=0;

    while(index_subindex<clientsToSendDataSizeOldClients)
    {
        if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
            real_reinsert_count++;
        index_subindex++;
    }
    if(real_reinsert_count>0)
    {
        uint32_t bufferSizeToHave;
        if(GlobalServerData::serverSettings.max_players<=255)
            bufferSizeToHave=sizeof(uint8_t)+real_reinsert_count*(sizeof(uint8_t)+sizeof(uint8_t)*3);
        else
            bufferSizeToHave=sizeof(uint16_t)+real_reinsert_count*(sizeof(uint16_t)+sizeof(uint8_t)*3);

        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x66;
        posOutput+=1;
        //Can't be here, need exclude him self to know the size:*reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole32(bufferSizeToHave-1-4);//set the dynamic size
        posOutput+=4;

        if(bufferSizeToHave<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
        {
            unsigned int index=0;
            if(GlobalServerData::serverSettings.max_players<=255)
            {
                while(index<clientsToSendDataSizeOldClients)
                {
                    unsigned int temp_reinsert=real_reinsert_count;
                    if(clientsToSendDataOldClients[index]->haveNewMove)
                        temp_reinsert--;
                    if(temp_reinsert>0)
                    {
                        index_subindex=0;
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=(uint8_t)temp_reinsert;
                        posOutput+=1;
                        while(index_subindex<clientsToSendDataSizeOldClients)
                        {
                            const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clientsToSendDataOldClients[index_subindex];
                            if(index!=index_subindex && client->haveNewMove)
                            {
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+0]=(uint8_t)client->public_and_private_informations.public_informations.simplifiedId;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+1]=(uint8_t)client->getX();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+2]=(uint8_t)client->getY();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+3]=(uint8_t)client->getLastDirection();
                                posOutput+=sizeof(uint8_t)+sizeof(uint8_t)*3;
                            }
                            index_subindex++;
                        }
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                        if(clientsToSendDataOldClients[index]->pingCountInProgress()<=0 && clientsToSendDataOldClients[index]->mapSyncMiss==false)
                            clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        else
                            clientsToSendDataOldClients[index]->mapSyncMiss=true;
                        posOutput=1+4;
                    }
                    index++;
                }
            }
            else
            {
                while(index<clientsToSendDataSizeOldClients)
                {
                    unsigned int temp_reinsert=real_reinsert_count;
                    if(clientsToSendDataOldClients[index]->haveNewMove)
                        temp_reinsert--;
                    if(temp_reinsert>0)
                    {
                        index_subindex=0;
                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16((uint16_t)temp_reinsert);
                        posOutput+=2;
                        while(index_subindex<clientsToSendDataSizeOldClients)
                        {
                            const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clientsToSendDataOldClients[index_subindex];
                            if(index!=index_subindex && client->haveNewMove)
                            {
                                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint16_t)htole16((uint16_t)client->public_and_private_informations.public_informations.simplifiedId);
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+0]=(uint8_t)client->getX();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+1]=(uint8_t)client->getY();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+2]=(uint8_t)client->getLastDirection();
                                posOutput+=sizeof(uint16_t)+sizeof(uint8_t)*3;
                            }
                            index_subindex++;
                        }
                        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                        if(clientsToSendDataOldClients[index]->pingCountInProgress()<=0 && clientsToSendDataOldClients[index]->mapSyncMiss==false)
                            clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        else
                            clientsToSendDataOldClients[index]->mapSyncMiss=true;
                        posOutput=1+4;
                    }
                    index++;
                }
            }
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        else
            std::cerr << "Out of buffer for map management to send reinsert en 16bits" << __LINE__ << "bufferSizeToHave" << bufferSizeToHave << "CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER" << CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER << std::endl;
        #endif
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insertcompose_header(char *buffer, int &posOutput)
{
    //send the network message
    posOutput=0;
    buffer[posOutput]=0x6B;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(buffer[0x00]!=0x6b)
    {
        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif
    posOutput+=1+4;
    //prepare the data
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(buffer[0x00]!=0x6b)
    {
        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif
    buffer[posOutput]=0x01;
    posOutput+=1;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(buffer[0x00]!=0x6b)
    {
        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insertcompose_map(char *buffer, int &posOutput)
{
    //mapId
    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        buffer[posOutput]=static_cast<uint8_t>(id);
        posOutput+=1;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }
    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        *reinterpret_cast<uint16_t *>(buffer+posOutput)=htole16(id);
        posOutput+=2;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        *reinterpret_cast<uint32_t *>(buffer+posOutput)=htole32(id);
        posOutput+=4;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insertcompose_playercount(char *buffer, int &posOutput)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    const size_t clientSize=clients.size();
    (void)clientSize;
    #endif
    //player count for this map
    if(GlobalServerData::serverSettings.max_players<=255)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        buffer[posOutput]=static_cast<uint8_t>(clients.size()-1);
        posOutput+=1;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        *reinterpret_cast<uint16_t *>(buffer+posOutput)=htole16(clients.size()-1);
        posOutput+=2;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(buffer[0x00]!=0x6b)
    {
        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif
}

void Map_server_MapVisibility_Simple_StoreOnSender::send_insertcompose_content_and_send(char *buffer, int &posOutput)
{
    if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        unsigned int indexSub=0;
        while(indexSub<clients.size())
        {
            MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(indexSub);
            posOutput+=playerToFullInsert(client,buffer+posOutput);
            ++indexSub;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
    }

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(buffer[0x00]!=0x6b)
    {
        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
        abort();
    }
    #endif
    const uint32_t initPos=posOutput;
    unsigned int index_subindex=0;
    while(index_subindex<clients.size())
    {
        posOutput=initPos;//reset the pos for each client
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index_subindex);
        client->to_send_insert=false;
        client->haveNewMove=false;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        if(client->mapSyncMiss==true && client->pingCountInProgress()<=0)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            //std::cerr << "start client " << posOutput << " " << __FILE__ << ":" << __LINE__ << std::endl;
            if(buffer[0x00]!=0x6b)
            {
                std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            #endif
            if(!GlobalServerData::serverSettings.mapVisibility.simple.reemit)
            {
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(buffer[0x00]!=0x6b)
                {
                    std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                #endif
                unsigned int indexSub=0;
                while(indexSub<clients.size())
                {
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(buffer[0x00]!=0x6b)
                    {
                        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                    }
                    //std::cerr << "before add " << posOutput << " " << __FILE__ << ":" << __LINE__ << std::endl;
                    #endif
                    if(client!=clients.at(indexSub))
                        posOutput+=playerToFullInsert(clients.at(indexSub),buffer+posOutput);
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    //std::cerr << "after add " << posOutput << " " << __FILE__ << ":" << __LINE__ << std::endl;
                    if(buffer[0x00]!=0x6b)
                    {
                        std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                        abort();
                    }
                    #endif
                    ++indexSub;
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(buffer[0x00]!=0x6b)
                {
                    std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                    abort();
                }
                #endif
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(buffer[0x00]!=0x6b)
            {
                std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            #endif

            *reinterpret_cast<uint32_t *>(buffer+1)=htole32(posOutput-1-4);//set the dynamic size
            client->to_send_insert=false;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            MapServer::check6B(buffer+1+4,posOutput-1-4);
            #endif
            client->sendRawBlock(buffer,posOutput);

            client->mapSyncMiss=false;
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        if(client->pingCountInProgress()<=0)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(buffer[0x00]!=0x6b)
            {
                std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            #endif
            client->sendPing();
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(buffer[0x00]!=0x6b)
            {
                std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
                abort();
            }
            #endif
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(buffer[0x00]!=0x6b)
        {
            std::cerr << "corrupted buffer into " << __FILE__ << ":" << __LINE__ << std::endl;
            abort();
        }
        #endif
        ++index_subindex;
    }
}

//buffer overflow check via buffer usage at player insert, per map if player are visible
void Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer()
{
    if(client.size()<=1)
        return;
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(have_change==false)
    {
        std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer(): have_change==false" << std::endl;
    }
    #endif
    // START for hysteresis
    have_change=false;
    if(send_drop_all)
    {
        send_dropAll();
        return;
    }
    if(!show)
        return;
    /// \todo use simplified id via 9
    //Insert player on map (Fast)
    if(send_reinsert_all)//for reshow because player number lower than 50, after hide because more than 100
    {
        send_reinsertAll();
        return;
    }
    // STOP for hysteresis

    if(clients.size()<=1 && to_send_remove.empty())
        return;
    unsigned int clientsToSendDataSizeNewClients=0;
    unsigned int clientsToSendDataSizeOldClients=0;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.herman-brule.com/wiki/Base_protocol_messages#C0
    //Insert player on map (Fast)
    if(to_send_insert)
    {
        send_insert(clientsToSendDataSizeNewClients,clientsToSendDataSizeOldClients);
        send_insert_exclude();
        to_send_insert=false;//of map, not client
    }
    else
    {
        unsigned int index=0;
        while(index<clients.size())
        {
            clientsToSendDataOldClients[clientsToSendDataSizeOldClients]=clients.at(index);
            clientsToSendDataSizeOldClients++;
            index++;
        }
    }
    //send drop
    if(!to_send_remove.empty())
        send_remove(clientsToSendDataSizeOldClients);

    //send small reinsert, used to remplace move and improve the performance
    const int &small_reinsert_count=clientsToSendDataSizeOldClients;
    if(small_reinsert_count>1)//then player who is not drop/insert
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
            send_samllreinsert_reemit(clientsToSendDataSizeOldClients);
        else
            send_samllreinsert(clientsToSendDataSizeOldClients);
    }
    //purge
    {
        int posOutput=0;
        send_insertcompose_header(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        send_insertcompose_map(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        send_insertcompose_playercount(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
        send_insertcompose_content_and_send(ProtocolParsingBase::tempBigBufferForOutput,posOutput);

        to_send_remove.clear();
        to_send_insert=false;
        send_drop_all=false;
        send_reinsert_all=false;
    }
}
