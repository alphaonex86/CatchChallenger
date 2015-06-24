#include "EpollClientLoginMaster.h"
#include "EpollServerLoginMaster.h"

#include <iostream>
#include <QString>
#include <time.h>

using namespace CatchChallenger;

EpollClientLoginMaster::EpollClientLoginMaster(
        #ifdef SERVERSSL
            const int &infd, SSL_CTX *ctx
        #else
            const int &infd
        #endif
        ) :
        ProtocolParsingInputOutput(
            #ifdef SERVERSSL
                infd,ctx
            #else
                infd
            #endif
           #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            ,PacketModeTransmission_Server
            #endif
            ),
        stat(None),
        socketString(NULL),
        socketStringSize(0),
        charactersGroupForGameServer(NULL),
        charactersGroupForGameServerInformation(NULL)
{
    rng.seed(time(0));
}

EpollClientLoginMaster::~EpollClientLoginMaster()
{
    if(socketString!=NULL)
        delete socketString;
    if(stat==EpollClientLoginMasterStat::LoginServer)
    {
        EpollClientLoginMaster::loginServers.removeOne(this);
        int index=0;
        while(index<EpollClientLoginMaster::gameServers.size())
        {
            int sub_index=0;
            while(sub_index<EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.size())
            {
                const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect.at(sub_index);
                if(dataForSelectedCharacterReturn.loginServer==this)
                    EpollClientLoginMaster::gameServers.at(index)->loginServerReturnForCharaterSelect[sub_index].loginServer=NULL;
                sub_index++;
            }
            index++;
        }
    }
    if(stat==EpollClientLoginMasterStat::GameServer)
    {
        EpollClientLoginMaster::gameServers.removeOne(this);
        charactersGroupForGameServer->removeGameServerUniqueKey(this);
        int index=0;
        while(index<loginServerReturnForCharaterSelect.size())
        {
            const DataForSelectedCharacterReturn &dataForSelectedCharacterReturn=loginServerReturnForCharaterSelect.at(index);
            if(dataForSelectedCharacterReturn.loginServer!=NULL)
            {
                charactersGroupForGameServer->unlockTheCharacter(dataForSelectedCharacterReturn.characterId);
                charactersGroupForGameServerInformation->lockedAccount.remove(dataForSelectedCharacterReturn.characterId);
                dataForSelectedCharacterReturn.loginServer->selectCharacter_ReturnFailed(dataForSelectedCharacterReturn.client_query_id,0x04);
            }
            index++;
        }
        EpollServerLoginMaster::epollServerLoginMaster->doTheServerList();
        EpollServerLoginMaster::epollServerLoginMaster->doTheReplyCache();
        EpollClientLoginMaster::broadcastGameServerChange();
    }
    std::cout << "Online: " << loginServers.size() << " login server and " << gameServers.size() << " game server" << std::endl;
}

void EpollClientLoginMaster::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void EpollClientLoginMaster::errorParsingLayer(const QString &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void EpollClientLoginMaster::messageParsingLayer(const QString &message) const
{
    std::cout << socketString << ": " << message.toLocal8Bit().constData() << std::endl;
}

void EpollClientLoginMaster::errorParsingLayer(const char * const error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginMaster::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::Type EpollClientLoginMaster::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void EpollClientLoginMaster::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void EpollClientLoginMaster::selectCharacter(const quint8 &query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId,const quint32 &accountId)
{
    if(charactersGroupIndex>=CharactersGroup::list.size())
    {
        errorParsingLayer("EpollClientLoginMaster::selectCharacter() charactersGroupIndex is out of range");
        return;
    }
    if(!CharactersGroup::list.at(charactersGroupIndex)->containsGameServerUniqueKey(serverUniqueKey))
    {
        EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound[1]=query_id;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        replyOutputCompression.remove(query_id);
        #endif
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound),EpollClientLoginMaster::characterSelectionIsWrongBufferSize);
        return;
    }
    if(CharactersGroup::list.at(charactersGroupIndex)->characterIsLocked(characterId))
    {
        EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline[1]=query_id;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        replyOutputCompression.remove(query_id);
        #endif
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::characterSelectionIsWrongBufferCharacterAlreadyConnectedOnline),EpollClientLoginMaster::characterSelectionIsWrongBufferSize);
        return;
    }
    EpollClientLoginMaster * gameServer=static_cast<EpollClientLoginMaster *>(CharactersGroup::list.at(charactersGroupIndex)->gameServers.value(serverUniqueKey).link);
    if(!gameServer->trySelectCharacterGameServer(this,query_id,serverUniqueKey,charactersGroupIndex,characterId,accountId))
    {
        EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound[1]=query_id;
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        removeFromQueryReceived(query_id);
        #endif
        #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
        replyOutputCompression.remove(query_id);
        #endif
        internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::characterSelectionIsWrongBufferServerNotFound),EpollClientLoginMaster::characterSelectionIsWrongBufferSize);
        return;
    }
    CharactersGroup::list[charactersGroupIndex]->lockTheCharacter(characterId);
    gameServer->charactersGroupForGameServerInformation->lockedAccount << characterId;
}

bool EpollClientLoginMaster::trySelectCharacterGameServer(EpollClientLoginMaster * const loginServer,const quint8 &client_query_id,const quint32 &serverUniqueKey,const quint8 &charactersGroupIndex,const quint32 &characterId, const quint32 &accountId)
{
    //here you are on game server link

    //check if the characterId is linked to the correct account on login server
    if(queryNumberList.empty())
    {
        std::cerr << "queryNumberList.empty() on game server to server it: "
                  << charactersGroupForGameServerInformation->uniqueKey
                  << ", host: "
                  << charactersGroupForGameServerInformation->host.toStdString()
                  << ":"
                  << charactersGroupForGameServerInformation->port
                  << std::endl;
        return false;
    }
    DataForSelectedCharacterReturn dataForSelectedCharacterReturn;
    dataForSelectedCharacterReturn.loginServer=loginServer;
    dataForSelectedCharacterReturn.client_query_id=client_query_id;
    dataForSelectedCharacterReturn.serverUniqueKey=serverUniqueKey;
    dataForSelectedCharacterReturn.charactersGroupIndex=charactersGroupIndex;
    dataForSelectedCharacterReturn.characterId=characterId;
    loginServerReturnForCharaterSelect << dataForSelectedCharacterReturn;
    //the data
    const quint8 &queryNumber=queryNumberList.back();
    waitedReply_mainCodeType[queryNumber]=0x81;
    waitedReply_subCodeType[queryNumber]=0x01;
    EpollClientLoginMaster::getTokenForCharacterSelect[0x02]=queryNumber;
    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::getTokenForCharacterSelect+0x03)=htole32(characterId);
    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::getTokenForCharacterSelect+0x07)=htole32(accountId);
    queryNumberList.pop_back();
    return internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::getTokenForCharacterSelect),sizeof(EpollClientLoginMaster::getTokenForCharacterSelect));
}

void EpollClientLoginMaster::selectCharacter_ReturnToken(const quint8 &query_id,const char * const token)
{
    postReplyData(query_id,token,CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER);
}

void EpollClientLoginMaster::selectCharacter_ReturnFailed(const quint8 &query_id,const quint8 &errorCode)
{
    //you are on login server
    postReplyData(query_id,reinterpret_cast<const char * const>(&errorCode),1);
}

void EpollClientLoginMaster::broadcastGameServerChange()
{
    int index=0;
    while(index<EpollClientLoginMaster::loginServers.size())
    {
        EpollClientLoginMaster * const loginServer=EpollClientLoginMaster::loginServers.at(index);
        loginServer->internalSendRawSmallPacket(EpollClientLoginMaster::serverServerList,EpollClientLoginMaster::serverServerListSize);
        index++;
    }
}

bool EpollClientLoginMaster::sendRawSmallPacket(const char * const data,const int &size)
{
    return internalSendRawSmallPacket(data,size);
}

void EpollClientLoginMaster::sendCurrentPlayer()
{
    if(!currentPlayerForGameServerToUpdate)
        return;
    if(EpollClientLoginMaster::gameServers.size()==0)
        return;
    //do the list
    int size=0;
    {
        int index=0;
        while(index<EpollClientLoginMaster::gameServers.size())
        {
            EpollClientLoginMaster * const gameServer=EpollClientLoginMaster::gameServers.at(index);
            *reinterpret_cast<quint16 *>(EpollClientLoginMaster::tempBuffer2+index*2)=htole16(gameServer->charactersGroupForGameServerInformation->currentPlayer);
            index++;
        }
        size=computeFullOutcommingData(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    false,
                    #endif
                    EpollClientLoginMaster::tempBuffer,
                    0xE1,0x01,EpollClientLoginMaster::tempBuffer2,EpollClientLoginMaster::gameServers.size()*2
                    );
    }
    //broadcast all
    {
        int index=0;
        while(index<EpollClientLoginMaster::loginServers.size())
        {
            EpollClientLoginMaster * const loginServer=EpollClientLoginMaster::loginServers.at(index);
            loginServer->internalSendRawSmallPacket(EpollClientLoginMaster::tempBuffer,size);
            index++;
        }
    }
}

void EpollClientLoginMaster::disconnectForDuplicateConnexionDetected(const quint32 &characterId)
{
    *reinterpret_cast<quint32 *>(EpollClientLoginMaster::duplicateConnexionDetected+0x02)=htole32(characterId);
    internalSendRawSmallPacket(reinterpret_cast<char *>(EpollClientLoginMaster::duplicateConnexionDetected),sizeof(EpollClientLoginMaster::duplicateConnexionDetected));
}
