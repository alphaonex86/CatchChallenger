#include "Client.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::sendLocalChatText(const QString &text)
{
    if((static_cast<MapServer *>(map)->localChatDropTotalCache+static_cast<MapServer *>(map)->localChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
        return;
    static_cast<MapServer *>(map)->localChatDropNewValue++;
    if(map==NULL)
        return;
    normalOutput(QStringLiteral("[chat local] %1: %2").arg(this->public_and_private_informations.public_informations.pseudo).arg(text));

    QByteArray finalData;
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);
        out << (quint8)Chat_type_local;
        out << text;

        QByteArray outputData2;
        QDataStream out2(&outputData2, QIODevice::WriteOnly);
        out2.setVersion(QDataStream::Qt_4_4);
        if(GlobalServerData::serverSettings.dontSendPlayerType)
            out2 << (quint8)Player_type_normal;
        else
            out2 << (quint8)this->public_and_private_informations.public_informations.type;
        finalData=ProtocolParsingInputOutput::computeFullOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    0xC2,0x0005,outputData+rawPseudo+outputData2);
    }

    const int &size=static_cast<MapServer *>(map)->clientsForBroadcast.size();
    int index=0;
    while(index<size)
    {
        if(static_cast<MapServer *>(map)->clientsForBroadcast.at(index)!=this)
            static_cast<MapServer *>(map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData);
        index++;
    }
}

void Client::insertClientOnMap(CommonMap *map)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<MapServer *>(map)->clientsForBroadcast.contains(this))
        normalOutput(QLatin1String("static_cast<MapServer *>(map)->clientsForBroadcast already have this"));
    else
    #endif
    static_cast<MapServer *>(map)->clientsForBroadcast << this;

    sendNearPlant();
}

void Client::removeClientOnMap(CommonMap *map, const bool &withDestroy)
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1)
        normalOutput(QStringLiteral("static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1: %1").arg(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)));
    #endif
    static_cast<MapServer *>(map)->clientsForBroadcast.removeOne(this);

    if(!withDestroy)
        removeNearPlant();
    map=NULL;
}
