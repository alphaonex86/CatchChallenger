#include "Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "../GlobalServerData.h"

using namespace CatchChallenger;

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender() :
    show(true),
    to_send_insert(false),
    send_drop_all(false),
    send_reinsert_all(false)
{
}

void Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer()
{
    if(clients.size()<=1 && to_send_remove.isEmpty())
        return;

    if(send_drop_all)
    {
        int index=0;
        while(index<clients.size())
        {
            clients.at(index)->sendPacket(0xC4);
            index++;
        }
        return;
    }
    if(send_reinsert_all)
    {
        QByteArray purgeBuffer_outputData;
        //prepare the data
        {
            QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);

            //////////////////////////// insert //////////////////////////
            /* can be only this map with this algo, then 1 map */
            out << (quint8)0x01;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)id;
            else
                out << (quint32)id;
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (quint8)clients.size();
            else
                out << (quint16)clients.size();
            int index=0;
            while (index<clients.size())
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)clients.at(index)->player_informations->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (quint16)clients.at(index)->player_informations->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)clients.at(index)->getX();
                out << (COORD_TYPE)clients.at(index)->getY();
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (quint8)((quint8)clients.at(index)->getLastDirection() | (quint8)Player_type_normal);
                else
                    out << (quint8)((quint8)clients.at(index)->getLastDirection() | (quint8)clients.at(index)->player_informations->public_and_private_informations.public_informations.type);
                if(CommonSettings::commonSettings.forcedSpeed==0)
                    out << clients.at(index)->player_informations->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettings::commonSettings.dontSendPseudo)
                {
                    purgeBuffer_outputData+=clients.at(index)->player_informations->rawPseudo;
                    out.device()->seek(out.device()->pos()+clients.at(index)->player_informations->rawPseudo.size());
                }
                //skin
                out << clients.at(index)->player_informations->public_and_private_informations.public_informations.skinId;
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
                QByteArray purgeBuffer_outputData;
                QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);

                //////////////////////////// insert //////////////////////////
                /* can be only this map with this algo, then 1 map */
                out << (quint8)0x01;
                if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                    out << (quint8)id;
                else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                    out << (quint16)id;
                else
                    out << (quint32)id;
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)(clients.size()-1);
                else
                    out << (quint16)(clients.size()-1);
                int index_subindex=0;
                while (index_subindex<clients.size())
                {
                    if(index!=index_subindex)
                    {
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (quint8)clients.at(index_subindex)->player_informations->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (quint16)clients.at(index_subindex)->player_informations->public_and_private_informations.public_informations.simplifiedId;
                        out << (COORD_TYPE)clients.at(index_subindex)->getX();
                        out << (COORD_TYPE)clients.at(index_subindex)->getY();
                        if(GlobalServerData::serverSettings.dontSendPlayerType)
                            out << (quint8)((quint8)clients.at(index_subindex)->getLastDirection() | (quint8)Player_type_normal);
                        else
                            out << (quint8)((quint8)clients.at(index_subindex)->getLastDirection() | (quint8)clients.at(index_subindex)->player_informations->public_and_private_informations.public_informations.type);
                        if(CommonSettings::commonSettings.forcedSpeed==0)
                            out << clients.at(index_subindex)->player_informations->public_and_private_informations.public_informations.speed;
                        //pseudo
                        if(!CommonSettings::commonSettings.dontSendPseudo)
                        {
                            purgeBuffer_outputData+=clients.at(index_subindex)->player_informations->rawPseudo;
                            out.device()->seek(out.device()->pos()+clients.at(index_subindex)->player_informations->rawPseudo.size());
                        }
                        //skin
                        out << clients.at(index_subindex)->player_informations->public_and_private_informations.public_informations.skinId;
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
    int reinsert_count=0;
    //do the count
    {
        int index_subindex=0;
        while (index_subindex<clientsToSendDataSizeOldClients)
        {
            if(!clientsToSendDataOldClients[index_subindex]->to_send_move.isEmpty())
                move_count++;
            else if(clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                reinsert_count++;
            ++index_subindex;
        }
    }
    //send insert
    if(insert_count>0)
    {
        QByteArray purgeBuffer;
        //prepare the data
        {
            QDataStream out(&purgeBuffer, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);

            //////////////////////////// insert //////////////////////////
            /* can be only this map with this algo, then 1 map */
            out << (quint8)0x01;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)id;
            else
                out << (quint32)id;
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (quint8)insert_count;
            else
                out << (quint16)insert_count;
            int index_subindex=0;
            while (index_subindex<clientsToSendDataSizeNewClients)
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)clientsToSendDataNewClients[index_subindex]->player_informations->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (quint16)clientsToSendDataNewClients[index_subindex]->player_informations->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)clientsToSendDataNewClients[index_subindex]->getX();
                out << (COORD_TYPE)clientsToSendDataNewClients[index_subindex]->getY();
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (quint8)((quint8)clientsToSendDataNewClients[index_subindex]->getLastDirection() | (quint8)Player_type_normal);
                else
                    out << (quint8)((quint8)clientsToSendDataNewClients[index_subindex]->getLastDirection() | (quint8)clientsToSendDataNewClients[index_subindex]->player_informations->public_and_private_informations.public_informations.type);
                if(CommonSettings::commonSettings.forcedSpeed==0)
                    out << clientsToSendDataNewClients[index_subindex]->player_informations->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettings::commonSettings.dontSendPseudo)
                {
                    purgeBuffer+=clientsToSendDataNewClients[index_subindex]->player_informations->rawPseudo;
                    out.device()->seek(out.device()->pos()+clientsToSendDataNewClients[index_subindex]->player_informations->rawPseudo.size());
                }
                //skin
                out << clientsToSendDataNewClients[index_subindex]->player_informations->public_and_private_informations.public_informations.skinId;
                ++index_subindex;
            }
        }
        int index=0;
        while(index<clientsToSendDataSizeOldClients)
        {
            clientsToSendDataOldClients[index]->sendPacket(0xC0,purgeBuffer);
            index++;
        }
    }
    //send move
    if(move_count>0)
    {
        int real_move_count;
        int index_subindex;
        int index_move;

        int index=0;
        while(index<clientsToSendDataSizeOldClients)
        {
            real_move_count=0;
            index_subindex=0;
            while (index_subindex<clientsToSendDataSizeOldClients)
            {
                if(index!=index_subindex && !clientsToSendDataOldClients[index_subindex]->to_send_move.isEmpty() && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    real_move_count++;
                index_subindex++;
            }
            if(real_move_count>0)
            {
                QByteArray purgeBuffer_outputData;
                QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);

                //////////////////////////// insert //////////////////////////
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)real_move_count;
                else
                    out << (quint16)real_move_count;
                index_subindex=0;
                while (index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(index!=index_subindex && !clientsToSendDataOldClients[index_subindex]->to_send_move.isEmpty() && !clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    {
                        const MapVisibilityAlgorithm_Simple_StoreOnSender &client=*clientsToSendDataOldClients[index_subindex];
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (quint8)client.player_informations->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (quint16)client.player_informations->public_and_private_informations.public_informations.simplifiedId;
                        out << (quint8)client.to_send_move.size();
                        index_move=0;
                        while(index_move<client.to_send_move.size())
                        {
                            out << client.to_send_move.at(index_move).movedUnit;
                            out << (quint8)client.to_send_move.at(index_move).direction;
                            index_move++;
                        }
                    }
                    index_subindex++;
                }
                clientsToSendDataOldClients[index]->sendPacket(0xC7,purgeBuffer_outputData);
            }
            index++;
        }
    }
    //send reinsert
    if(reinsert_count>0)
    {
        int real_reinsert_count;
        int index_subindex;

        int index=0;
        while(index<clientsToSendDataSizeOldClients)
        {
            real_reinsert_count=0;
            index_subindex=0;
            while (index_subindex<clientsToSendDataSizeOldClients)
            {
                if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(real_reinsert_count>0)
            {
                QByteArray purgeBuffer_outputData;
                QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
                out.setVersion(QDataStream::Qt_4_4);

                //////////////////////////// insert //////////////////////////
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)real_reinsert_count;
                else
                    out << (quint16)real_reinsert_count;
                index_subindex=0;
                while (index_subindex<clientsToSendDataSizeOldClients)
                {
                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->to_send_reinsert)
                    {
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (quint8)clientsToSendDataOldClients[index_subindex]->player_informations->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (quint16)clientsToSendDataOldClients[index_subindex]->player_informations->public_and_private_informations.public_informations.simplifiedId;
                        out << clientsToSendDataOldClients[index_subindex]->getX();
                        out << clientsToSendDataOldClients[index_subindex]->getY();
                        out << (quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                    }
                    index_subindex++;
                }
                clientsToSendDataOldClients[index]->sendPacket(0xC5,purgeBuffer_outputData);
            }
            index++;
        }
    }
    //send drop
    if(!to_send_remove.isEmpty())
    {
        QByteArray purgeBuffer_outputData;
        //prepare the data
        {
            QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);

            //////////////////////////// insert //////////////////////////
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                out << (quint8)to_send_remove.size();
            else
                out << (quint16)to_send_remove.size();
            int index_subindex=0;
            while (index_subindex<to_send_remove.size())
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    out << (quint8)to_send_remove.at(index_subindex);
                else
                    out << (quint16)to_send_remove.at(index_subindex);
                index_subindex++;
            }
        }
        int index_subindex=0;
        while (index_subindex<clientsToSendDataSizeOldClients)
        {
            clientsToSendDataOldClients[index_subindex]->sendPacket(0xC8,purgeBuffer_outputData);
            index_subindex++;
        }
    }
    //purge
    {
        int index_subindex=0;
        while (index_subindex<clients.size())
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
    }
}
