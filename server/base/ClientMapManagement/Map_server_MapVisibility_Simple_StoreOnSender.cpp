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
        unsigned const char mainCode[]={0xC4};
        int index=0;
        while(index<clients.size())
        {
            //clients.at(index)->dropAllClients();
            clients.at(index)->sendRawSmallPacket(reinterpret_cast<const char *>(mainCode),sizeof(mainCode));
            index++;
        }
        return;
    }
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
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
            while(index<clients.size())
            {
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
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
        return;
    }

    MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataNewClients[clients.size()];
    MapVisibilityAlgorithm_Simple_StoreOnSender * clientsToSendDataOldClients[clients.size()];
    int clientsToSendDataSizeNewClients=0;
    int clientsToSendDataSizeOldClients=0;
    /// \todo use simplified id with max visible player and updater http://catchchallenger.first-world.info/wiki/Base_protocol_messages#C0
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
                while(index_subindex<clients.size())
                {
                    if(index!=index_subindex)
                    {
                        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                            out << (quint8)clients.at(index_subindex)->public_and_private_informations.public_informations.simplifiedId;
                        else
                            out << (quint16)clients.at(index_subindex)->public_and_private_informations.public_informations.simplifiedId;
                        out << (COORD_TYPE)clients.at(index_subindex)->getX();
                        out << (COORD_TYPE)clients.at(index_subindex)->getY();
                        if(GlobalServerData::serverSettings.dontSendPlayerType)
                            out << (quint8)((quint8)clients.at(index_subindex)->getLastDirection() | (quint8)Player_type_normal);
                        else
                            out << (quint8)((quint8)clients.at(index_subindex)->getLastDirection() | (quint8)clients.at(index_subindex)->public_and_private_informations.public_informations.type);
                        if(CommonSettings::commonSettings.forcedSpeed==0)
                            out << clients.at(index_subindex)->public_and_private_informations.public_informations.speed;
                        //pseudo
                        if(!CommonSettings::commonSettings.dontSendPseudo)
                        {
                            const QByteArray &rawPseudo=clients.at(index_subindex)->getRawPseudo();
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
    //send drop
    if(!to_send_remove.isEmpty())
    {
        if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
        {
            char buffer[to_send_remove.size()*(to_send_remove.size()*sizeof(quint8))+sizeof(quint8)];
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
                clientsToSendDataOldClients[index_subindex]->packOutcommingData(0xC8,buffer,sizeof(buffer));
                index_subindex++;
            }
        }
        else
        {
            char buffer[to_send_remove.size()*(to_send_remove.size()*sizeof(quint16))+sizeof(quint16)];
            buffer[0]=(quint16)htobe16((quint16)to_send_remove.size());
            int index_subindex=0;
            while(index_subindex<to_send_remove.size())
            {
                buffer[sizeof(quint16)+index_subindex*sizeof(quint16)]=(quint16)htobe16((quint16)to_send_remove.at(index_subindex));
                index_subindex++;
            }
            index_subindex=0;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                clientsToSendDataOldClients[index_subindex]->packOutcommingData(0xC8,buffer,sizeof(buffer));
                index_subindex++;
            }
        }
    }

    //send small reinsert
    const int &small_reinsert_count=clientsToSendDataSizeOldClients;
    if(small_reinsert_count>0)
    {
        if(GlobalServerData::serverSettings.mapVisibility.simple.reemit)
        {
            int real_reinsert_count;
            int index_subindex;

            real_reinsert_count=0;
            index_subindex=0;
            char *buffer;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                buffer=new char[sizeof(quint8)+real_reinsert_count*(sizeof(quint8)+sizeof(quint8)*3)];
            else
                buffer=new char[sizeof(quint16)+real_reinsert_count*(sizeof(quint16)+sizeof(quint8)*3)];
            if(real_reinsert_count>0)
            {
                int bufferCursor;
                //////////////////////////// insert //////////////////////////
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                {
                    buffer[0]=(quint8)real_reinsert_count;
                    bufferCursor=sizeof(quint8);
                }
                else
                {
                    buffer[0]=(quint16)htobe16((quint16)real_reinsert_count);
                    bufferCursor=sizeof(quint16);
                }
                index_subindex=0;
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                        {
                            buffer[bufferCursor]=(quint8)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                            bufferCursor+=sizeof(quint8);
                            buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                            buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                            buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                            bufferCursor+=sizeof(quint8)*3;
                        }
                        index_subindex++;
                    }
                else
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                        {
                            buffer[bufferCursor]=(quint16)htobe16((quint16)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                            bufferCursor+=sizeof(quint16);
                            buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                            buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                            buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                            bufferCursor+=sizeof(quint8)*3;
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
            delete buffer;
        }
        else
        {

            int real_reinsert_count=0;
            int index_subindex=0;
            int bufferCursor;
            int bufferBaseCursor;
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                bufferBaseCursor=sizeof(quint8);
            else
                bufferBaseCursor=sizeof(quint16);

            char *buffer;
            while(index_subindex<clientsToSendDataSizeOldClients)
            {
                if(clientsToSendDataOldClients[index_subindex]->haveNewMove)
                    real_reinsert_count++;
                index_subindex++;
            }
            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                buffer=new char[sizeof(quint8)+clientsToSendDataSizeOldClients*(sizeof(quint8)+sizeof(quint8)*3)];
            else
                buffer=new char[sizeof(quint16)+clientsToSendDataSizeOldClients*(sizeof(quint16)+sizeof(quint8)*3)];

            if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                buffer[0]=(quint8)real_reinsert_count;
            else
                buffer[0]=(quint16)htobe16((quint16)real_reinsert_count);

            int index=0;
            while(index<clientsToSendDataSizeOldClients)
            {
                bufferCursor=bufferBaseCursor*2;
                real_reinsert_count=0;
                index_subindex=0;
                if(GlobalServerData::serverPrivateVariables.maxVisiblePlayerAtSameTime<=255)
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                        {
                            buffer[bufferCursor]=(quint8)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId;
                            buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                            buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                            buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                            bufferCursor+=sizeof(quint8)*3;
                            real_reinsert_count++;
                        }
                        bufferCursor=bufferBaseCursor;
                        buffer[bufferCursor]=(quint8)real_reinsert_count;
                        bufferCursor+=sizeof(quint8);
                        bufferCursor=bufferBaseCursor*2+real_reinsert_count*bufferBaseCursor+real_reinsert_count*sizeof(quint8)*3;

                        index_subindex++;
                    }
                else
                    while(index_subindex<clientsToSendDataSizeOldClients)
                    {
                        if(index!=index_subindex && clientsToSendDataOldClients[index_subindex]->haveNewMove)
                        {
                            buffer[bufferCursor]=(quint16)htobe16((quint16)clientsToSendDataOldClients[index_subindex]->public_and_private_informations.public_informations.simplifiedId);
                            bufferCursor+=sizeof(quint16);
                            buffer[bufferCursor+0]=(quint8)clientsToSendDataOldClients[index_subindex]->getX();
                            buffer[bufferCursor+1]=(quint8)clientsToSendDataOldClients[index_subindex]->getY();
                            buffer[bufferCursor+2]=(quint8)clientsToSendDataOldClients[index_subindex]->getLastDirection();
                            bufferCursor+=sizeof(quint8)*3;
                            real_reinsert_count++;
                        }
                        bufferCursor=bufferBaseCursor;
                        buffer[bufferCursor]=(quint16)htobe16((quint16)real_reinsert_count);
                        bufferCursor+=sizeof(quint16);
                        bufferCursor=bufferBaseCursor*2+real_reinsert_count*bufferBaseCursor+real_reinsert_count*sizeof(quint8)*3;

                        index_subindex++;
                    }
                if(real_reinsert_count>0)
                    clientsToSendDataOldClients[index]->packOutcommingData(0xC5,buffer,bufferCursor);
                index++;
            }
            delete buffer;
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
