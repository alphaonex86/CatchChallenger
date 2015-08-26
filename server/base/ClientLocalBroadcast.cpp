#include "Client.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLib.h"
#include "GlobalServerData.h"
#include "MapServer.h"

using namespace CatchChallenger;

void Client::sendLocalChatText(const std::string &text)
{
    if((static_cast<MapServer *>(map)->localChatDropTotalCache+static_cast<MapServer *>(map)->localChatDropNewValue)>=GlobalServerData::serverSettings.ddos.dropGlobalChatMessageLocalClan)
        return;
    static_cast<MapServer *>(map)->localChatDropNewValue++;
    if(map==NULL)
        return;
    normalOutput(std::stringLiteral("[chat local] %1: %2").arg(this->public_and_private_informations.public_informations.pseudo).arg(text));

    QByteArray finalData;
    {
        QByteArray outputData;
        QDataStream out(&outputData, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (uint8_t)Chat_type_local;
        {
            const QByteArray &tempText=text.toUtf8();
            if(tempText.size()>255)
            {
                DebugClass::debugConsole(std::stringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
                return;
            }
            out << (uint8_t)tempText.size();
            outputData+=tempText;
            out.device()->seek(out.device()->pos()+tempText.size());
        }

        QByteArray outputData2;
        QDataStream out2(&outputData2, QIODevice::WriteOnly);
        out2.setVersion(QDataStream::Qt_4_4);
        if(GlobalServerData::serverSettings.dontSendPlayerType)
            out2 << (uint8_t)Player_type_normal;
        else
            out2 << (uint8_t)this->public_and_private_informations.public_informations.type;
        QByteArray replyData(outputData+rawPseudo+outputData2);
        finalData.resize(16+outputData.size()+rawPseudo.size()+outputData2.size());
        finalData.resize(ProtocolParsingBase::computeOutcommingData(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            false,
            #endif
                    finalData.data(),0xCA,replyData.constData(),replyData.size()));
    }

    const int &size=static_cast<MapServer *>(map)->clientsForBroadcast.size();
    int index=0;
    while(index<size)
    {
        if(static_cast<MapServer *>(map)->clientsForBroadcast.at(index)!=this)
            static_cast<MapServer *>(map)->clientsForBroadcast.at(index)->sendRawSmallPacket(finalData.constData(),finalData.size());
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

    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    sendNearPlant();
    #endif
}

void Client::removeClientOnMap(CommonMap *map
                               #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
                               , const bool &withDestroy
                               #endif
                               )
{
    #ifdef CATCHCHALLENGER_SERVER_EXTRA_CHECK
    if(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1)
        normalOutput(std::stringLiteral("static_cast<MapServer *>(map)->clientsForBroadcast.count(this)!=1: %1").arg(static_cast<MapServer *>(map)->clientsForBroadcast.count(this)));
    #endif
    static_cast<MapServer *>(map)->clientsForBroadcast.removeOne(this);

    #ifndef CATCHCHALLENGER_GAMESERVER_PLANTBYPLAYER
    if(!withDestroy)
        //leave the map
        removeNearPlant();
    #endif
    map=NULL;
}
