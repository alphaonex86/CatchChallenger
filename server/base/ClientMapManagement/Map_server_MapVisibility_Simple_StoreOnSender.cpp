#include "Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "../GlobalServerData.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataNewClients[65535];
MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataOldClients[65535];
Map_server_MapVisibility_Simple_StoreOnSender ** Map_server_MapVisibility_Simple_StoreOnSender::map_to_update=NULL;
uint32_t Map_server_MapVisibility_Simple_StoreOnSender::map_to_update_size=0;

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender() :
    show(true),
    to_send_insert(false),
    send_drop_all(false),
    send_reinsert_all(false),
    have_change(false)
{
}

void Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(have_change==false)
    {
        std::cerr << "Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer(): have_change==false" << std::endl;
    }
    #endif
    have_change=false;
    if(send_drop_all)
    {
        unsigned const char mainCode[]={0x65};
        unsigned int index=0;
        while(index<clients.size())
        {
            Client * const client=clients.at(index);
            //clientdropAllClients();
            client->sendRawBlock(reinterpret_cast<const char *>(mainCode),sizeof(mainCode));
            index++;
        }
        send_drop_all=false;
        return;
    }
    if(!show)
        return;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
    //Insert player on map (Fast)
    if(send_reinsert_all)
    {
        //send the network message
        uint32_t posOutput=0;
        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x6B;
        posOutput+=1+4;

        //prepare the data
        {
            //////////////////////////// insert //////////////////////////
            /* can be only this map with this algo, then 1 map */
            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x01;
            posOutput+=1;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
            {
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=id;
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
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.size();
                posOutput+=1;
            }
            else
            {
                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.size());
                posOutput+=2;
            }
            unsigned int index=0;
            while(index<clients.size())
            {
                if(GlobalServerData::serverSettings.max_players<=255)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.at(index)->public_and_private_informations.public_informations.simplifiedId;
                    posOutput+=1;
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.at(index)->public_and_private_informations.public_informations.simplifiedId);
                    posOutput+=2;
                }
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.at(index)->getX();
                posOutput+=1;
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.at(index)->getY();
                posOutput+=1;
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)clients.at(index)->getLastDirection() | (uint8_t)Player_type_normal);
                else
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)clients.at(index)->getLastDirection() | (uint8_t)clients.at(index)->public_and_private_informations.public_informations.type);
                posOutput+=1;
                if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                {
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.at(index)->public_and_private_informations.public_informations.speed;
                    posOutput+=1;
                }
                //pseudo
                if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                {
                    const std::string &text=clients.at(index)->public_and_private_informations.public_informations.pseudo;
                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                    posOutput+=1;
                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                    posOutput+=text.size();
                }
                //skin
                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.at(index)->public_and_private_informations.public_informations.skinId;
                posOutput+=1;

                ++index;
            }
        }
        *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

        unsigned int index=0;
        while(index<clients.size())
        {
            Client * const client=clients.at(index);
            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
            index++;
        }
        send_reinsert_all=true;
        return;
    }

    if(clients.size()<=1 && to_send_remove.empty())
        return;
    unsigned int clientsToSendDataSizeNewClients=0;
    unsigned int clientsToSendDataSizeOldClients=0;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
    //Insert player on map (Fast)
    if(to_send_insert)
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
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=id;
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
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=insert_player;
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
                        {
                            if(GlobalServerData::serverSettings.max_players<=255)
                            {
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.simplifiedId;
                                posOutput+=1;
                            }
                            else
                            {
                                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(client->public_and_private_informations.public_informations.simplifiedId);
                                posOutput+=2;
                            }
                            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getX();
                            posOutput+=1;
                            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getY();
                            posOutput+=1;
                            if(GlobalServerData::serverSettings.dontSendPlayerType)
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
                            else
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
                            posOutput+=1;
                            if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                            {
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.speed;
                                posOutput+=1;
                            }
                            //pseudo
                            if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                            {
                                const std::string &text=client->public_and_private_informations.public_informations.pseudo;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                                posOutput+=1;
                                memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                                posOutput+=text.size();
                            }
                            //skin
                            ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.skinId;
                            posOutput+=1;
                        }
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
                            client->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
        //insert old + new (excluding them self) on new
        {
            //count the player will be insered
            unsigned int index=0;
            while(index<clients.size())
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
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=id;
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
                        ProtocolParsingBase::tempBigBufferForOutput[posOutput]=clients.size()-1;
                        posOutput+=1;
                        unsigned int index_subindex=0;
                        while(index_subindex<clients.size())
                        {
                            const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index_subindex);
                            if(index!=index_subindex)
                            {
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.simplifiedId;
                                posOutput+=1;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getX();
                                posOutput+=1;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getY();
                                posOutput+=1;
                                /// \todo, put this out of loop
                                if(GlobalServerData::serverSettings.dontSendPlayerType)
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
                                else
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
                                posOutput+=1;
                                /// \todo, put this out of loop
                                if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.speed;
                                    posOutput+=1;
                                }
                                /// \todo, put this out of loop
                                //pseudo
                                if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                                {
                                    const std::string &text=client->public_and_private_informations.public_informations.pseudo;
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                                    posOutput+=1;
                                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                                    posOutput+=text.size();
                                }
                                //skin
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.skinId;
                                posOutput+=1;
                            }
                            ++index_subindex;
                        }
                    }
                    else
                    {
                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(clients.size()-1);
                        posOutput+=2;
                        unsigned int index_subindex=0;
                        while(index_subindex<clients.size())
                        {
                            const MapVisibilityAlgorithm_Simple_StoreOnSender * const client=clients.at(index_subindex);
                            if(index!=index_subindex)
                            {
                                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(client->public_and_private_informations.public_informations.simplifiedId);
                                posOutput+=2;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getX();
                                posOutput+=1;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->getY();
                                posOutput+=1;
                                /// \todo, put this out of loop
                                if(GlobalServerData::serverSettings.dontSendPlayerType)
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)Player_type_normal);
                                else
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=((uint8_t)client->getLastDirection() | (uint8_t)client->public_and_private_informations.public_informations.type);
                                posOutput+=1;
                                /// \todo, put this out of loop
                                if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                                {
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.speed;
                                    posOutput+=1;
                                }
                                /// \todo, put this out of loop
                                //pseudo
                                if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                                {
                                    const std::string &text=client->public_and_private_informations.public_informations.pseudo;
                                    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=text.size();
                                    posOutput+=1;
                                    memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,text.data(),text.size());
                                    posOutput+=text.size();
                                }
                                //skin
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=client->public_and_private_informations.public_informations.skinId;
                                posOutput+=1;
                            }
                            ++index_subindex;
                        }
                    }

                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size

                    clients.at(index)->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                }
                index++;
            }
        }
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

                ProtocolParsingBase::tempBigBufferForOutput[posOutput]=to_send_remove.size();
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
                    clientsToSendDataOldClients[index_subindex]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
                std::cout << "data: " << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput) << ", line " << std::to_string(__LINE__) << std::endl;

                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_remove.size());
                posOutput+=2;
                std::cout << "data: " << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput) << ", line " << std::to_string(__LINE__) << std::endl;
                unsigned int index_subindex=0;
                while(index_subindex<to_send_remove.size())
                {
                    *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=htole16(to_send_remove.at(index_subindex));
                    posOutput+=2;
                    index_subindex++;
                }
                std::cout << "data: " << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput) << ", line " << std::to_string(__LINE__) << std::endl;
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    clientsToSendDataOldClients[index_subindex]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                    std::cout << "data: " << binarytoHexa(ProtocolParsingBase::tempBigBufferForOutput,posOutput) << ", line " << std::to_string(__LINE__) << std::endl;
                    index_subindex++;
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                std::cerr << "Out of buffer for map management to send remove on 16bits" << __LINE__ << std::endl;
            #endif
        }
    }

    //send small reinsert, used to remplace move and improve the performance
    const int &small_reinsert_count=clientsToSendDataSizeOldClients;
    if(small_reinsert_count>1)//then player who is not drop/insert
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
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
                            if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                            {
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+0]=(uint8_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+1]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getX();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+2]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getY();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+3]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                posOutput+=sizeof(uint8_t)+sizeof(uint8_t)*3;
                            }
                            index_subindex++;
                        }
                    else
                        while(index_subindex<clientsToSendDataSizeOldClients)
                        {
                            if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                            {
                                *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint16_t)htole16((uint16_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+0]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getX();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+1]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getY();
                                ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+2]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                posOutput+=sizeof(uint16_t)+sizeof(uint8_t)*3;
                            }
                            index_subindex++;
                        }
                    *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                    unsigned int index=0;
                    while(index<clientsToSendDataSizeOldClients)
                    {
                        clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
                        index++;
                    }
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                std::cerr << "Out of buffer for map management to send reinsert" << __LINE__ << "bufferSizeToHave" << bufferSizeToHave << "CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER" << CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER << std::endl;
            #endif
        }
        else
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
                                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                                    {
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+0]=(uint8_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+1]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getX();
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+2]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getY();
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+3]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                        posOutput+=sizeof(uint8_t)+sizeof(uint8_t)*3;
                                    }
                                    index_subindex++;
                                }
                                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                                clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
                                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                                    {
                                        *reinterpret_cast<uint16_t *>(ProtocolParsingBase::tempBigBufferForOutput+posOutput)=(uint16_t)htole16((uint16_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+0]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getX();
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+1]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getY();
                                        ProtocolParsingBase::tempBigBufferForOutput[posOutput+sizeof(uint16_t)+2]=(uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                        posOutput+=sizeof(uint16_t)+sizeof(uint8_t)*3;
                                    }
                                    index_subindex++;
                                }
                                *reinterpret_cast<uint32_t *>(ProtocolParsingBase::tempBigBufferForOutput+1)=htole32(posOutput-1-4);//set the dynamic size
                                clientsToSendDataOldClients[index]->sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
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
    }
    //purge
    {
        unsigned int index_subindex=0;
        while(index_subindex<clients.size())
        {
            MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index_subindex);
            client->to_send_insert=false;
            client->haveNewMove=false;
            ++index_subindex;
        }
        to_send_remove.clear();
        to_send_insert=false;
        send_drop_all=false;
        send_reinsert_all=false;
    }
}
