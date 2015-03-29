#include "Client.h"
#include "BroadCastWithoutSender.h"
#include "../../general/base/ProtocolParsing.h"
#include "../../general/base/FacilityLib.h"
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
        out.setVersion(QDataStream::Qt_4_4);out.setByteOrder(QDataStream::LittleEndian);
        out << (quint8)Chat_type_local;
        {
            const QByteArray &tempText=text.toUtf8();
            if(tempText.size()>255)
            {
                DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
                return;
            }
            out << (quint8)tempText.size();
            outputData+=tempText;
            out.device()->seek(out.device()->pos()+tempText.size());
        }

        QByteArray outputData2;
        QDataStream out2(&outputData2, QIODevice::WriteOnly);
        out2.setVersion(QDataStream::Qt_4_4);
        if(GlobalServerData::serverSettings.dontSendPlayerType)
            out2 << (quint8)Player_type_normal;
        else
            out2 << (quint8)this->public_and_private_informations.public_informations.type;
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
