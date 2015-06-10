#include "EpollClientLoginSlave.h"
#include "CharactersGroupForLogin.h"

#include <iostream>
#include <QString>

using namespace CatchChallenger;

EpollClientLoginSlave::EpollClientLoginSlave(
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
        stat(EpollClientLoginStat::None),
        socketString(NULL),
        socketStringSize(0),
        account_id(0),
        characterListForReplyInSuspend(0),
        serverListForReplyRawData(NULL),
        serverListForReplyRawDataSize(0),
        serverListForReplyInSuspend(0)
{
}

EpollClientLoginSlave::~EpollClientLoginSlave()
{
    if(socketString!=NULL)
        delete socketString;
    {
        //SQL
        int index=0;
        while(index<callbackRegistred.size())
        {
            callbackRegistred.at(index)->object=NULL;
            index++;
        }

        //from master
        index=0;
        while(index<CharactersGroupForLogin::list.size())
        {
            CharactersGroupForLogin * const charactersGroupForLogin=CharactersGroupForLogin::list.at(index);

            int sub_index=0;
            while(sub_index<charactersGroupForLogin->clientQueryForReadReturn.size())
            {
                if(charactersGroupForLogin->clientQueryForReadReturn.at(sub_index)==this)
                    charactersGroupForLogin->clientQueryForReadReturn[sub_index]=NULL;
                sub_index++;
            }
            sub_index=0;
            while(sub_index<charactersGroupForLogin->addCharacterParamList.size())
            {
                if(charactersGroupForLogin->addCharacterParamList.at(sub_index).client==this)
                    charactersGroupForLogin->addCharacterParamList[sub_index].client=NULL;
                sub_index++;
            }
            sub_index=0;
            while(sub_index<charactersGroupForLogin->removeCharacterParamList.size())
            {
                if(charactersGroupForLogin->removeCharacterParamList.at(sub_index).client==this)
                    charactersGroupForLogin->removeCharacterParamList[sub_index].client=NULL;
                sub_index++;
            }

            index++;
        }

        //selected char
        /// \todo check by crash with ASSERT failure in QHash: "Iterating beyond end()"
        {
            QHashIterator<quint8/*queryNumber*/,LoginLinkToMaster::DataForSelectedCharacterReturn> j(EpollClientLoginSlave::linkToMaster->selectCharacterClients);
            while (j.hasNext()) {
                j.next();
                if(j.value().client==this)
                    EpollClientLoginSlave::linkToMaster->selectCharacterClients[j.key()].client=NULL;
            }
        }
    }
}

void EpollClientLoginSlave::disconnectClient()
{
    epollSocket.close();
    messageParsingLayer("Disconnected client");
}

//input/ouput layer
void EpollClientLoginSlave::errorParsingLayer(const QString &error)
{
    std::cerr << socketString << ": " << error.toLocal8Bit().constData() << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const QString &message) const
{
    std::cout << socketString << ": " << message.toLocal8Bit().constData() << std::endl;
}

void EpollClientLoginSlave::errorParsingLayer(const char * const error)
{
    std::cerr << socketString << ": " << error << std::endl;
    disconnectClient();
}

void EpollClientLoginSlave::messageParsingLayer(const char * const message) const
{
    std::cout << socketString << ": " << message << std::endl;
}

BaseClassSwitch::Type EpollClientLoginSlave::getType() const
{
    return BaseClassSwitch::Type::Client;
}

void EpollClientLoginSlave::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}
