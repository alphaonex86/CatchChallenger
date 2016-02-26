#include "Map_server_MapVisibility_WithBorder_StoreOnSender.h"

using namespace CatchChallenger;

Map_server_MapVisibility_WithBorder_StoreOnSender::Map_server_MapVisibility_WithBorder_StoreOnSender() :
    clientsOnBorder(0),
    showWithBorder(true),
    show(true)
{
}

void Map_server_MapVisibility_WithBorder_StoreOnSender::purgeBuffer()
{
    /// \todo reinsert full for map change else small reinsert
    /*//send move
    if(move_count>0)
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
        {
            int real_move_count=0;
            int index_subindex=0;

            int bufferSize=0;
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    bufferSize+=sizeof(uint8_t);
                else
                    bufferSize+=sizeof(uint16_t);
            }
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->to_send_move_size>0 && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                {
                    bufferSize+=sizeof(uint8_t)*2*client.to_send_move_size;
                    real_move_count++;
                }
                index_subindex++;
            }
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    bufferSize+=sizeof(uint8_t)*real_move_count;
                else
                    bufferSize+=sizeof(uint16_t)*real_move_count;
                bufferSize+=sizeof(uint8_t)*real_move_count;
            }
            char buffer[bufferSize];
            int bufferCursor=0;
            if(real_move_count>0)
            {
                //////////////////////////// insert //////////////////////////
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                {
                    buffer[0]=(uint8_t)real_move_count;
                    bufferCursor+=sizeof(uint8_t);
                }
                else
                {
                    *reinterpret_cast<uint16_t *>(buffer+0)=(uint16_t)htole16((uint16_t)real_move_count);
                    bufferCursor+=sizeof(uint16_t);
                }
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(clientsToSendDataOldClients[index_subindex]->to_send_move_size>0 && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    {
                        const MapVisibilityAlgorithm_Simple_StoreOnSender &client=*clientsToSendDataOldClients[index_subindex];
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                        {
                            buffer[bufferCursor]=(uint8_t)client.public_and_private_informations.public_informations.simplifiedId;
                            bufferCursor+=sizeof(uint8_t);
                        }
                        else
                        {
                            *reinterpret_cast<uint16_t *>(buffer+bufferCursor)=(uint16_t)htole16((uint16_t)client.public_and_private_informations.public_informations.simplifiedId);
                            bufferCursor+=sizeof(uint16_t);
                        }
                        buffer[bufferCursor]=(uint8_t)client.to_send_move_size;
                        bufferCursor+=sizeof(uint8_t);
                        const int &tempSize=client.to_send_move_size*2*sizeof(uint8_t);
                        memcpy(buffer,client.to_send_move,tempSize);
                        bufferCursor+=tempSize;
                    }
                    index_subindex++;
                }
                int index=0;
                while(index<clientsToSendDataSizeOldClients)
                {
                    clientsToSendDataOldClients[index]->sendPacket(0xC7,purgeBuffer_outputData);
                    index++;
                }
            }
        }
        else
        {
            int real_move_count;
            int index_subindex;

            int bufferSize=0;
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    bufferSize+=sizeof(uint8_t);
                else
                    bufferSize+=sizeof(uint16_t);
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(clientsToSendDataOldClients[index_subindex]->to_send_move_size>0 && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    {
                        bufferSize+=sizeof(uint8_t)*2*client.to_send_move_size;
                        real_move_count++;
                    }
                    index_subindex++;
                }
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    bufferSize+=sizeof(uint8_t)*real_move_count;
                else
                    bufferSize+=sizeof(uint16_t)*real_move_count;
                bufferSize+=sizeof(uint8_t)*real_move_count;
            }

            char buffer[bufferSize];
            int bufferCursor;
            int index=0;
            while(index<clientsToSendDataSizeOldClients)
            {
                real_move_count=0;
                index_subindex=0;
                bufferCursor=0;

                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(index!=index_subindex && !clientsToSendDataOldClients[index_subindex]->to_send_move.isEmpty() && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                        real_move_count++;
                    index_subindex++;
                }
                if(real_move_count>0)
                {
                    //////////////////////////// insert //////////////////////////
                    if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    {
                        buffer[0]=(uint8_t)real_move_count;
                        bufferCursor+=sizeof(uint8_t);
                    }
                    else
                    {
                        *reinterpret_cast<uint16_t *>(buffer+0)=(uint16_t)htole16((uint16_t)real_move_count);
                        bufferCursor+=sizeof(uint16_t);
                    }
                    index_subindex=0;
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(index!=index_subindex && !clientsToSendDataOldClients[index_subindex]->to_send_move.isEmpty() && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                        {
                            const MapVisibilityAlgorithm_Simple_StoreOnSender &client=*clientsToSendDataOldClients[index_subindex];
                            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            {
                                buffer[bufferCursor]=(uint8_t)client.public_and_private_informations.public_informations.simplifiedId;
                                bufferCursor+=sizeof(uint8_t);
                            }
                            else
                            {
                                *reinterpret_cast<uint16_t *>(buffer+bufferCursor)=(uint16_t)htole16((uint16_t)client.public_and_private_informations.public_informations.simplifiedId);
                                bufferCursor+=sizeof(uint16_t);
                            }
                            buffer[bufferCursor]=(uint8_t)client.to_send_move_size;
                            bufferCursor+=sizeof(uint8_t);
                            const int &tempSize=client.to_send_move_size*2*sizeof(uint8_t);
                            memcpy(buffer,client.to_send_move,tempSize);
                            bufferCursor+=tempSize;
                        }
                        index_subindex++;
                    }
                    clientsToSendDataOldClients[index]->sendPacket(0xC7,purgeBuffer_outputData);
                }
                index++;
            }
        }
    }
    //send full insert
    if(insert_count>0)
    {
        std::vector<char> purgeBuffer;
        //prepare the data
        {

            //////////////////////////// insert //////////////////////////
            // can be only this map with this algo, then 1 map
            out << (uint8_t)0x01;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (uint8_t)id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (uint16_t)id;
            else
                out << (uint32_t)id;
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (uint8_t)insert_count;
            else
                out << (uint16_t)insert_count;
            int index_subindex=0;
            while(index_subindex<clientsToSendDataSizeNewClients)
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (uint8_t)clientsToSendDataNewClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (uint16_t)clientsToSendDataNewClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)clientsToSendDataNewClients[index_subindex]->getX();
                out << (COORD_TYPE)clientsToSendDataNewClients[index_subindex]->getY();
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (uint8_t)((uint8_t)clientsToSendDataNewClients[index_subindex]->getLastDirection() | (uint8_t)Player_type_normal);
                else
                    out << (uint8_t)((uint8_t)clientsToSendDataNewClients[index_subindex]->getLastDirection() | (uint8_t)clientsToSendDataNewClients[index_subindex]->public_and_private_informations.public_informations.type);
                if(CommonSettings::commonSettings.forcedSpeed==0)
                    out << clientsToSendDataNewClients[index_subindex]->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettings::commonSettings.dontSendPseudo)
                {
                    const std::vector<char> &rawPseudo=clients.at(index_subindex)->getRawPseudo();
                    purgeBuffer+=rawPseudo;
                    out.device()->seek(out.device()->pos()+rawPseudo.size());
                }
                //skin
                out << clientsToSendDataNewClients[index_subindex]->public_and_private_informations.public_informations.skinId;
                ++index_subindex;
            }
        }
        int index=0;
        while(index<clientsToSendDataSizeOldClients)
        {
            clientsToSendDataOldClients[index]->sendPacket(0xC0,purgeBuffer);
            index++;
        }
    }*/
    /*    if(clients.size()<=1 && to_send_remove.isEmpty())
        return;

    if(send_drop_all)
    {
        int index=0;
        while(index<clients.size())
        {
            clients.at(index)->dropAllClients();
            index++;
        }
        return;
    }
    if(send_reinsert_all)
    {
        std::vector<char> purgeBuffer_outputData;
        //prepare the data
        {

            //////////////////////////// insert //////////////////////////
            // can be only this map with this algo, then 1 map
            out << (uint8_t)0x01;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (uint8_t)id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (uint16_t)id;
            else
                out << (uint32_t)id;
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (uint8_t)clients.size();
            else
                out << (uint16_t)clients.size();
            int index=0;
            while(index<clients.size())
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (uint8_t)clients.at(index)->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (uint16_t)clients.at(index)->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)clients.at(index)->getX();
                out << (COORD_TYPE)clients.at(index)->getY();
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (uint8_t)((uint8_t)clients.at(index)->getLastDirection() | (uint8_t)Player_type_normal);
                else
                    out << (uint8_t)((uint8_t)clients.at(index)->getLastDirection() | (uint8_t)clients.at(index)->public_and_private_informations.public_informations.type);
                if(CommonSettings::commonSettings.forcedSpeed==0)
                    out << clients.at(index)->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettings::commonSettings.dontSendPseudo)
                {
                    const std::vector<char> &rawPseudo=clients.at(index)->getRawPseudo();
                    purgeBuffer_outputData+=rawPseudo;
                    out.device()->seek(out.device()->pos()+rawPseudo.size());
                }
                //skin
                out << clients.at(index)->public_and_private_informations.public_informations.skinId;
                ++index;
            }
        }
        int index=0;
        while(index<clients.size())
        {
            clients.at(index)->sendPacket(0xC0,purgeBuffer_outputData);
            index++;
        }
        return;
    }

    MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataNewClients[clients.size()];
    MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataOldClients[clients.size()];
    int clientsToSendDataSizeNewClients=0;
    int clientsToSendDataSizeOldClients=0;
    if(to_send_insert)
    {
        int index=0;
        while(index<clients.size())
        {
            if(clients.at(index)->to_send_insert)
            {
                std::vector<char> purgeBuffer_outputData;

                //////////////////////////// insert //////////////////////////
                // can be only this map with this algo, then 1 map
                out << (uint8_t)0x01;
                if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                    out << (uint8_t)id;
                else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                    out << (uint16_t)id;
                else
                    out << (uint32_t)id;
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (uint8_t)(clients.size()-1);
                else
                    out << (uint16_t)(clients.size()-1);
                int index_subindex=0;
                while(index_subindex<clients.size())
                {
                    if(index!=index_subindex)
                    {
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (uint8_t)clients.at(index_subindex)->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (uint16_t)clients.at(index_subindex)->public_and_private_informations.public_informations.simplifiedId;
                        out << (COORD_TYPE)clients.at(index_subindex)->getX();
                        out << (COORD_TYPE)clients.at(index_subindex)->getY();
                        if(GlobalServerData::serverSettings.dontSendPlayerType)
                            out << (uint8_t)((uint8_t)clients.at(index_subindex)->getLastDirection() | (uint8_t)Player_type_normal);
                        else
                            out << (uint8_t)((uint8_t)clients.at(index_subindex)->getLastDirection() | (uint8_t)clients.at(index_subindex)->public_and_private_informations.public_informations.type);
                        if(CommonSettings::commonSettings.forcedSpeed==0)
                            out << clients.at(index_subindex)->public_and_private_informations.public_informations.speed;
                        //pseudo
                        if(!CommonSettings::commonSettings.dontSendPseudo)
                        {
                            const std::vector<char> &rawPseudo=clients.at(index_subindex)->getRawPseudo();
                            purgeBuffer_outputData+=rawPseudo;
                            out.device()->seek(out.device()->pos()+rawPseudo.size());
                        }
                        //skin
                        out << clients.at(index_subindex)->public_and_private_informations.public_informations.skinId;
                    }
                    ++index_subindex;
                }
                clients.at(index)->sendPacket(0xC0,purgeBuffer_outputData);
                clientsToSendDataNewClients[clientsToSendDataSizeNewClients]=clients.at(index);
                clientsToSendDataSizeNewClients++;
            }
            else
            {
                clientsToSendDataOldClients[clientsToSendDataSizeOldClients]=clients.at(index);
                clientsToSendDataSizeOldClients++;
            }
            index++;
        }
    }
    else
    {
        int index=0;
        while(index<clients.size())
        {
            clientsToSendDataOldClients[clientsToSendDataSizeOldClients]=clients.at(index);
            clientsToSendDataSizeOldClients++;
            index++;
        }
    }
    int insert_count=clientsToSendDataSizeNewClients;
    int move_count=0;
    int small_reinsert_count=0;
    //do the count
    {
        int index_subindex=0;
        while(index_subindex<clientsToSendDataSizeOldClients)
        {
            if(clientsToSendDataOldClients[index_subindex]->to_send_move_size>0)
                move_count++;
            else if(clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                small_reinsert_count++;
            ++index_subindex;
        }
    }
    //send drop
    if(!to_send_remove.isEmpty())
    {
        std::vector<char> purgeBuffer_outputData;
        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
            purgeBuffer_outputData.resize(to_send_remove.size()*(to_send_remove.size()*sizeof(uint8_t))+sizeof(uint8_t));
        else
            purgeBuffer_outputData.resize(to_send_remove.size()*(to_send_remove.size()*sizeof(uint16_t))+sizeof(uint16_t));
        //prepare the data
        {

            //////////////////////////// insert //////////////////////////
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (uint8_t)to_send_remove.size();
            else
                out << (uint16_t)to_send_remove.size();
            int index_subindex=0;
            while(index_subindex<to_send_remove.size())
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (uint8_t)to_send_remove.at(index_subindex);
                else
                    out << (uint16_t)to_send_remove.at(index_subindex);
                index_subindex++;
            }
        }
        int index_subindex=0;
        while(index_subindex<clientsToSendDataSizeOldClients)
        {
            clientsToSendDataOldClients[index_subindex]->sendPacket(0xC8,purgeBuffer_outputData);
            index_subindex++;
        }
    }

    //send small reinsert
    if(small_reinsert_count>0)
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
        {
            int real_reinsert_count;
            int index_subindex;

            real_reinsert_count=0;
            index_subindex=0;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(real_reinsert_count>0)
            {
                std::vector<char> purgeBuffer_outputData;

                //////////////////////////// insert //////////////////////////
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (uint8_t)real_reinsert_count;
                else
                    out << (uint16_t)real_reinsert_count;
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    {
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (uint8_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (uint16_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                        out << clientsToSendDataOldClients[index_subindex]->getX();
                        out << clientsToSendDataOldClients[index_subindex]->getY();
                        out << (uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                    }
                    index_subindex++;
                }
                int index=0;
                while(index<clientsToSendDataSizeOldClients)
                {
                    clientsToSendDataOldClients[index]->sendPacket(0xC5,purgeBuffer_outputData);
                    index++;
                }
            }
        }
        else
        {
            int real_reinsert_count;
            int index_subindex;

            int index=0;
            while(index<clientsToSendDataSizeOldClients)
            {
                real_reinsert_count=0;
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                        real_reinsert_count++;
                    index_subindex++;
                }
                if(real_reinsert_count>0)
                {
                    std::vector<char> purgeBuffer_outputData;

                    //////////////////////////// insert //////////////////////////
                    if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                        out << (uint8_t)real_reinsert_count;
                    else
                        out << (uint16_t)real_reinsert_count;
                    index_subindex=0;
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                        {
                            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                                out << (uint8_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                            else
                                out << (uint16_t)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                            out << clientsToSendDataOldClients[index_subindex]->getX();
                            out << clientsToSendDataOldClients[index_subindex]->getY();
                            out << (uint8_t)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                        }
                        index_subindex++;
                    }
                    clientsToSendDataOldClients[index]->sendPacket(0xC5,purgeBuffer_outputData);
                }
                index++;
            }
        }
    }
    //purge
    {
        int index_subindex=0;
        while(index_subindex<clients.size())
        {
            MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index_subindex);
            client->to_send_insert=false;
            client->to_send_reinsert=false;
            client->to_send_move.clear();
            ++index_subindex;
        }
        to_send_remove.clear();
        to_send_insert=false;
        send_drop_all=false;
        send_reinsert_all=false;
    }*/
}
