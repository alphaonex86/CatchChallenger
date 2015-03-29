#include "Map_server_MapVisibility_Simple_StoreOnSender.h"
#include "../GlobalServerData.h"

using namespace CatchChallenger;

MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataNewClients[65535];
MapVisibilityAlgorithm_Simple_StoreOnSender * Map_server_MapVisibility_Simple_StoreOnSender::clientsToSendDataOldClients[65535];
char Map_server_MapVisibility_Simple_StoreOnSender::buffer[CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER];

Map_server_MapVisibility_Simple_StoreOnSender::Map_server_MapVisibility_Simple_StoreOnSender() :
    show(true),
    to_send_insert(false),
    send_drop_all(false),
    send_reinsert_all(false)
{
}

void Map_server_MapVisibility_Simple_StoreOnSender::purgeBuffer()
{
    if(send_drop_all)
    {
        unsigned const char mainCode[]={0xC4};
        int index=0;
        while(index<clients.size())
        {
            //clients.at(index)->dropAllClients();
            clients.at(index)->sendRawSmallPacket(reinterpret_cast<const char *>(mainCode),sizeof(mainCode));
            index++;
        }
        send_drop_all=false;
        return;
    }
    if(!show)
        return;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
    if(send_reinsert_all)
    {
        QByteArray purgeBuffer_outputData;
        //prepare the data
        {
            QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

            //////////////////////////// insert //////////////////////////
            /* can be only this map with this algo, then 1 map */
            out << (quint8)0x01;
            if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                out << (quint8)id;
            else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                out << (quint16)id;
            else
                out << (quint32)id;
            if(GlobalServerData::serverSettings.max_players<=255)
                out << (quint8)clients.size();
            else
                out << (quint16)clients.size();
            int index=0;
            while(index<clients.size())
            {
                if(GlobalServerData::serverSettings.max_players<=255)
                    out << (quint8)clients.at(index)->public_and_private_informations.public_informations.simplifiedId;
                else
                    out << (quint16)clients.at(index)->public_and_private_informations.public_informations.simplifiedId;
                out << (COORD_TYPE)clients.at(index)->getX();
                out << (COORD_TYPE)clients.at(index)->getY();
                if(GlobalServerData::serverSettings.dontSendPlayerType)
                    out << (quint8)((quint8)clients.at(index)->getLastDirection() | (quint8)Player_type_normal);
                else
                    out << (quint8)((quint8)clients.at(index)->getLastDirection() | (quint8)clients.at(index)->public_and_private_informations.public_informations.type);
                if(CommonSettings::commonSettings.forcedSpeed==0)
                    out << clients.at(index)->public_and_private_informations.public_informations.speed;
                //pseudo
                if(!CommonSettings::commonSettings.dontSendPseudo)
                {
                    const QByteArray &rawPseudo=clients.at(index)->getRawPseudo();
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
        send_reinsert_all=true;
        return;
    }

    if(clients.size()<=1 && to_send_remove.isEmpty())
        return;
    int clientsToSendDataSizeNewClients=0;
    int clientsToSendDataSizeOldClients=0;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
    if(to_send_insert)
    {
        //insert new on old
        {
            //count the player will be insered
            int insert_player=0;
            {
                int index=0;
                while(index<clients.size())
                {
                    if(clients.at(index)->to_send_insert)
                        insert_player++;
                    index++;
                }
            }
            //compose the query
            QByteArray purgeBuffer_outputData;
            if(insert_player>0)
            {
                {
                    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

                    //////////////////////////// insert //////////////////////////
                    /* can be only this map with this algo, then 1 map */
                    out << (quint8)0x01;
                    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                        out << (quint8)id;
                    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                        out << (quint16)id;
                    else
                        out << (quint32)id;
                    if(GlobalServerData::serverSettings.max_players<=255)
                        out << (quint8)(insert_player);
                    else
                        out << (quint16)(insert_player);
                    int index=0;
                    while(index<clients.size())
                    {
                        MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index);
                        if(client->to_send_insert)
                        {
                            if(GlobalServerData::serverSettings.max_players<=255)
                                out << (quint8)client->public_and_private_informations.public_informations.simplifiedId;
                            else
                                out << (quint16)client->public_and_private_informations.public_informations.simplifiedId;
                            out << (COORD_TYPE)client->getX();
                            out << (COORD_TYPE)client->getY();
                            if(GlobalServerData::serverSettings.dontSendPlayerType)
                                out << (quint8)((quint8)client->getLastDirection() | (quint8)Player_type_normal);
                            else
                                out << (quint8)((quint8)client->getLastDirection() | (quint8)client->public_and_private_informations.public_informations.type);
                            if(CommonSettings::commonSettings.forcedSpeed==0)
                                out << client->public_and_private_informations.public_informations.speed;
                            //pseudo
                            if(!CommonSettings::commonSettings.dontSendPseudo)
                            {
                                const QByteArray &rawPseudo=client->getRawPseudo();
                                purgeBuffer_outputData+=rawPseudo;
                                out.device()->seek(out.device()->pos()+rawPseudo.size());
                            }
                            //skin
                            out << client->public_and_private_informations.public_informations.skinId;
                        }
                        ++index;
                    }
                }
            }
            else
                qDebug() << "insert_player count is null!";
            //send the packet
            {
                int index=0;
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
                        client->sendPacket(0xC0,purgeBuffer_outputData);
                        clientsToSendDataOldClients[clientsToSendDataSizeOldClients]=client;
                        clientsToSendDataSizeOldClients++;
                    }
                    index++;
                }
            }
        }
        //insert old + new (excluding them self) on new
        {
            //count the player will be insered
            int index=0;
            while(index<clients.size())
            {
                if(clients.at(index)->to_send_insert)
                {
                    QByteArray purgeBuffer_outputData;
                    QDataStream out(&purgeBuffer_outputData, QIODevice::WriteOnly);
                    out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);

                    //////////////////////////// insert //////////////////////////
                    /* can be only this map with this algo, then 1 map */
                    out << (quint8)0x01;
                    if(GlobalServerData::serverPrivateVariables.map_list.size()<=255)
                        out << (quint8)id;
                    else if(GlobalServerData::serverPrivateVariables.map_list.size()<=65535)
                        out << (quint16)id;
                    else
                        out << (quint32)id;
                    if(GlobalServerData::serverSettings.max_players<=255)
                        out << (quint8)(clients.size()-1);
                    else
                        out << (quint16)(clients.size()-1);
                    int index_subindex=0;
                    while(index_subindex<clients.size())
                    {
                        MapVisibilityAlgorithm_Simple_StoreOnSender * client=clients.at(index_subindex);
                        if(index!=index_subindex)
                        {
                            if(GlobalServerData::serverSettings.max_players<=255)
                                out << (quint8)client->public_and_private_informations.public_informations.simplifiedId;
                            else
                                out << (quint16)client->public_and_private_informations.public_informations.simplifiedId;
                            out << (COORD_TYPE)client->getX();
                            out << (COORD_TYPE)client->getY();
                            if(GlobalServerData::serverSettings.dontSendPlayerType)
                                out << (quint8)((quint8)client->getLastDirection() | (quint8)Player_type_normal);
                            else
                                out << (quint8)((quint8)client->getLastDirection() | (quint8)client->public_and_private_informations.public_informations.type);
                            if(CommonSettings::commonSettings.forcedSpeed==0)
                                out << client->public_and_private_informations.public_informations.speed;
                            //pseudo
                            if(!CommonSettings::commonSettings.dontSendPseudo)
                            {
                                const QByteArray &rawPseudo=client->getRawPseudo();
                                purgeBuffer_outputData+=rawPseudo;
                                out.device()->seek(out.device()->pos()+rawPseudo.size());
                            }
                            //skin
                            out << client->public_and_private_informations.public_informations.skinId;
                        }
                        ++index_subindex;
                    }
                    clients.at(index)->sendPacket(0xC0,purgeBuffer_outputData);
                }
                index++;
            }
        }
        to_send_insert=false;//of map, not client
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
    //send drop
    if(!to_send_remove.isEmpty())
    {
        if(GlobalServerData::serverSettings.max_players<=255)
        {
            if((sizeof(quint8)+to_send_remove.size()*sizeof(quint8))<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
            {
                char buffer[sizeof(quint8)+to_send_remove.size()*sizeof(quint8)];
                buffer[0]=(quint8)to_send_remove.size();
                int index_subindex=0;
                while(index_subindex<to_send_remove.size())
                {
                    buffer[sizeof(quint8)+index_subindex*sizeof(quint8)]=(quint8)to_send_remove.at(index_subindex);
                    index_subindex++;
                }
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    clientsToSendDataOldClients[index_subindex]->packOutcommingData(0xC8,buffer,sizeof(quint8)+sizeof(quint8)*to_send_remove.size());
                    index_subindex++;
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                qDebug() << "Out of buffer for map management to send remove" << __LINE__;
            #endif
        }
        else
        {
            if((sizeof(quint16)+to_send_remove.size()*sizeof(quint16))<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
            {
                *reinterpret_cast<quint16 *>(buffer+0)=(quint16)htole16((quint16)to_send_remove.size());
                int index_subindex=0;
                while(index_subindex<to_send_remove.size())
                {
                    *reinterpret_cast<quint16 *>(buffer+sizeof(quint16)+index_subindex*sizeof(quint16))=(quint16)htole16((quint16)to_send_remove.at(index_subindex));
                    index_subindex++;
                }
                index_subindex=0;
                while(index_subindex<clientsToSendDataSizeOldClients)
                {
                    clientsToSendDataOldClients[index_subindex]->packOutcommingData(0xC8,buffer,sizeof(quint16)+sizeof(quint16)*to_send_remove.size());
                    index_subindex++;
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                qDebug() << "Out of buffer for map management to send remove on 16bits" << __LINE__;
            #endif
        }
    }

    //send small reinsert
    const int &small_reinsert_count=clientsToSendDataSizeOldClients;
    if(small_reinsert_count>1)//then player who is not drop/insert
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
        {
            int real_reinsert_count;
            int index_subindex;

            real_reinsert_count=0;
            index_subindex=0;
            int bufferSizeToHave;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(GlobalServerData::serverSettings.max_players<=255)
                bufferSizeToHave=sizeof(quint8)+real_reinsert_count*(sizeof(quint8)+sizeof(quint8)*3);
            else
                bufferSizeToHave=sizeof(quint16)+real_reinsert_count*(sizeof(quint16)+sizeof(quint8)*3);
            if(bufferSizeToHave<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
            {
                if(real_reinsert_count>0)
                {
                    int bufferCursor;
                    //////////////////////////// insert //////////////////////////
                    if(GlobalServerData::serverSettings.max_players<=255)
                    {
                        buffer[0]=(quint8)real_reinsert_count;
                        bufferCursor=sizeof(quint8);
                    }
                    else
                    {
                        *reinterpret_cast<quint16 *>(buffer+0)=(quint16)htole16((quint16)real_reinsert_count);
                        bufferCursor=sizeof(quint16);
                    }
                    index_subindex=0;
                    if(GlobalServerData::serverSettings.max_players<=255)
                        while(index_subindex<clientsToSendDataSizeOldClients)
                        {
                            if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                            {
                                buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                                buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                                buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                                buffer[bufferCursor+3]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                bufferCursor+=sizeof(quint8)+sizeof(quint8)*3;
                            }
                            index_subindex++;
                        }
                    else
                        while(index_subindex<clientsToSendDataSizeOldClients)
                        {
                            if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                            {
                                *reinterpret_cast<quint16 *>(buffer+bufferCursor)=(quint16)htole16((quint16)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                                buffer[bufferCursor+sizeof(quint16)+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                                buffer[bufferCursor+sizeof(quint16)+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                                buffer[bufferCursor+sizeof(quint16)+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                bufferCursor+=sizeof(quint16)+sizeof(quint8)*3;
                            }
                            index_subindex++;
                        }
                    int index=0;
                    while(index<clientsToSendDataSizeOldClients)
                    {
                        clientsToSendDataOldClients[index]->packOutcommingData(0xC5,buffer,bufferCursor);
                        index++;
                    }
                }
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            else
                qDebug() << "Out of buffer for map management to send reinsert" << __LINE__ << "bufferSizeToHave" << bufferSizeToHave << "CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER" << CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER;
            #endif
        }
        else
        {
            int real_reinsert_count=0;
            int index_subindex=0;
            int bufferCursor;
            int bufferBaseCursor;
            int bufferSizeToHave;
            if(GlobalServerData::serverSettings.max_players<=255)
                bufferBaseCursor=sizeof(quint8);
            else
                bufferBaseCursor=sizeof(quint16);

            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(real_reinsert_count>0)
            {
                if(GlobalServerData::serverSettings.max_players<=255)
                    bufferSizeToHave=sizeof(quint8)+real_reinsert_count*(sizeof(quint8)+sizeof(quint8)*3);
                else
                    bufferSizeToHave=sizeof(quint16)+real_reinsert_count*(sizeof(quint16)+sizeof(quint8)*3);
                if(bufferSizeToHave<CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER)
                {
                    int index=0;
                    if(GlobalServerData::serverSettings.max_players<=255)
                    {
                        while(index<clientsToSendDataSizeOldClients)
                        {
                            int temp_reinsert=real_reinsert_count;
                            if(clientsToSendDataOldClients[index]->haveNewMove)
                                temp_reinsert--;
                            bufferCursor=bufferBaseCursor;
                            if(temp_reinsert>0)
                            {
                                index_subindex=0;
                                buffer[0]=(quint8)temp_reinsert;
                                while(index_subindex<clientsToSendDataSizeOldClients)
                                {
                                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                                    {
                                        buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                                        buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                                        buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                                        buffer[bufferCursor+3]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                        bufferCursor+=sizeof(quint8)+sizeof(quint8)*3;
                                    }
                                    index_subindex++;
                                }
                                clientsToSendDataOldClients[index]->packOutcommingData(0xC5,buffer,bufferCursor);
                            }
                            index++;
                        }
                    }
                    else
                    {
                        while(index<clientsToSendDataSizeOldClients)
                        {
                            int temp_reinsert=real_reinsert_count;
                            if(clientsToSendDataOldClients[index]->haveNewMove)
                                temp_reinsert--;
                            bufferCursor=bufferBaseCursor;
                            if(temp_reinsert>0)
                            {
                                index_subindex=0;
                                *reinterpret_cast<quint16 *>(buffer+0)=(quint16)htole16((quint16)temp_reinsert);
                                while(index_subindex<clientsToSendDataSizeOldClients)
                                {
                                    if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                                    {
                                        *reinterpret_cast<quint16 *>(buffer+bufferCursor)=(quint16)htole16((quint16)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                                        buffer[bufferCursor+sizeof(quint16)+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                                        buffer[bufferCursor+sizeof(quint16)+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                                        buffer[bufferCursor+sizeof(quint16)+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                                        bufferCursor+=sizeof(quint16)+sizeof(quint8)*3;
                                    }
                                    index_subindex++;
                                }
                                clientsToSendDataOldClients[index]->packOutcommingData(0xC5,buffer,bufferCursor);
                            }
                            index++;
                        }
                    }
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                else
                    qDebug() << "Out of buffer for map management to send reinsert en 16bits" << __LINE__ << "bufferSizeToHave" << bufferSizeToHave << "CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER" << CATCHCHALLENGER_BIGBUFFERSIZE_FORTOPLAYER;
                #endif
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
            client->haveNewMove=false;
            ++index_subindex;
        }
        to_send_remove.clear();
        to_send_insert=false;
        send_drop_all=false;
        send_reinsert_all=false;
    }
}
