#include "Api_protocol.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonDatapackServerSpec.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"

#include <QRegularExpression>
#include <QDataStream>

#include <iostream>

using namespace CatchChallenger;

bool Api_protocol::parseCharacterBlockServer(const uint8_t &packetCode, const uint8_t &queryNumber, const std::string &data)
{
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }

    uint16_t max_players;
    in >> max_players;
    max_players_real=max_players;
    if(max_players==65534)
    {
        this->max_players=255;
        setMaxPlayers(255);
    }
    else
    {
        setMaxPlayers(max_players);
        this->max_players=max_players;
    }

    uint32_t captureRemainingTime;
    uint8_t captureFrequencyType;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the city capture remainingTime, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> captureRemainingTime;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the city capture type, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> captureFrequencyType;
    switch(captureFrequencyType)
    {
        case 0x01:
        case 0x02:
        break;
        default:
        parseError("Procotol wrong or corrupted",std::string("wrong captureFrequencyType, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    cityCapture(captureRemainingTime,captureFrequencyType);

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t tempForceClientToSendAtBorder;
        in >> tempForceClientToSendAtBorder;
        if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
        {
            parseError("Procotol wrong or corrupted",std::string("forceClientToSendAtBorder have wrong value, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.forcedSpeed;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the forcedSpeed, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.useSP;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the tcpCork, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.tcpCork;
    {
        //socket->setTcpCork(CommonSettingsServer::commonSettingsServer.tcpCork);->ignored for now
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.autoLearn;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.dontSendPseudo;


    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_xp, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_gold, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_gold;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_all, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the rates_gold, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_drop;

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_all, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_all;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_local, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_local;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_private, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_private;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the chat_allow_clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the factoryPriceChange, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.factoryPriceChange;

    {
        //Main type code
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),textSize);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.empty())
            {
                parseError("Procotol wrong or corrupted",std::string("mainDatapackCode is empty, please put it into the settings"));
                return false;
            }
            if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode,std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
            {
                parseError("Procotol wrong or corrupted",
                           "CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE "+
                                       CommonSettingsServer::commonSettingsServer.mainDatapackCode+
                                                              "not contain "+
                            CATCHCHALLENGER_CHECK_MAINDATAPACKCODE
                           );
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.mainDatapackCode.clear();

        //here to be sure of the order
        mDatapackMain=mDatapackBase+"map/main/"+CommonSettingsServer::commonSettingsServer.mainDatapackCode+"/";
    }
    {
        //Sub type code
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),textSize);
            CommonSettingsServer::commonSettingsServer.subDatapackCode=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            {
                if(!regex_search(CommonSettingsServer::commonSettingsServer.subDatapackCode,std::regex(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
                {
                    parseError("Procotol wrong or corrupted",std::string("CommonSettingsServer::commonSettingsServer.subDatapackCode not match CATCHCHALLENGER_CHECK_SUBDATAPACKCODE"));
                    return false;
                }
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();

        //here to be sure of the order
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            mDatapackSub=mDatapackMain+"sub/emptyRandomCoder23osaQ9mb5hYh2j/";
        else
            mDatapackSub=mDatapackMain+"sub/"+CommonSettingsServer::commonSettingsServer.subDatapackCode+"/";
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the datapack checksum, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(28);
    memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),28).constData(),28);
    in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size());

    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the datapack checksum, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(28);
        memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),28).constData(),28);
        in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size());
    }

    {
        //the mirror
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t mirrorSize;
        in >> mirrorSize;
        if(mirrorSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),mirrorSize);
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(!regex_search(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,std::regex("^https?://")))
            {
                parseError("Procotol wrong or corrupted",std::string("mirror with not http(s) protocol with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.clear();
    }
    emit haveDatapackMainSubCode();

    if(!CatchChallenger::CommonDatapack::commonDatapack.isParsedContent())//for bit array
    {
        if(delayedLogin.data.empty())
        {
            QByteArray tdata=QByteArray(data.data(),data.size()).mid(
                        static_cast<int>(in.device()->pos()),
                        static_cast<int>(in.device()->size()-in.device()->pos())
                        );
            delayedLogin.data=std::string(tdata.data(),tdata.size());
            delayedLogin.packetCode=packetCode;
            delayedLogin.queryNumber=queryNumber;
            return true;
        }
        else
        {
            parseError("Procotol wrong or corrupted",
                       "Login in suspend already set with main ident: "+std::to_string(packetCode)+
                       ", and queryNumber: "+std::to_string(queryNumber)+", line: "+
                       std::string(__FILE__)+":"+std::to_string(__LINE__)
                       );
            return false;
        }
    }
    if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.isParsedContent())
    {
        if(delayedLogin.data.empty())
        {
            QByteArray tdata=QByteArray(data.data(),data.size()).mid(
                        static_cast<int>(in.device()->pos()),
                        static_cast<int>(in.device()->size()-in.device()->pos())
                        );
            delayedLogin.data=std::string(tdata.data(),tdata.size());
            delayedLogin.packetCode=packetCode;
            delayedLogin.queryNumber=queryNumber;
            return true;
        }
        else
        {
            parseError("Procotol wrong or corrupted",
                       "Login in suspend already set with main ident: "+std::to_string(packetCode)+
                                   ", and queryNumber: "+std::to_string(queryNumber)+", line: "+
                                   std::string(__FILE__)+":"+std::to_string(__LINE__)
                                   );
            return false;
        }
    }

    QByteArray tdata=QByteArray(data.data(),data.size()).mid(
                static_cast<int>(in.device()->pos()),
                static_cast<int>(in.device()->size()-in.device()->pos())
                );
    delayedLogin.data=std::string(tdata.data(),tdata.size());
    parseCharacterBlockCharacter(packetCode,queryNumber,std::string(tdata.data(),tdata.size()));
    return true;
}

bool Api_protocol::parseCharacterBlockCharacter(const uint8_t &packetCode, const uint8_t &queryNumber, const std::string &data)
{
    QDataStream in(QByteArray(data.data(),data.size()));
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }

    //Events not with default value list size
    {
        std::vector<std::pair<uint8_t,uint8_t> > events;
        uint8_t tempListSize;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max_character, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> tempListSize;
        uint8_t event,value;
        int index=0;
        while(index<tempListSize)
        {

            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the event id, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> event;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the event value, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> value;
            index++;
            events.push_back(std::pair<uint8_t,uint8_t>(event,value));
        }
        setEvents(events);
    }

    if(this->max_players<=255)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t simplifiedId;
        in >> simplifiedId;
        player_informations.public_informations.simplifiedId=simplifiedId;
    }
    else
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> player_informations.public_informations.simplifiedId;
    }
    {
        //pseudo
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            QByteArray rawText=QByteArray(data.data(),data.size()).mid(static_cast<int>(in.device()->pos()),textSize);
            player_informations.public_informations.pseudo=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
        }
        else
            player_informations.public_informations.pseudo.clear();
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        newError("Procotol wrong or corrupted",std::string("wrong text with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t tempAllowSize;
    in >> tempAllowSize;
    {
        int index=0;
        while(index<tempAllowSize)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                newError("Procotol wrong or corrupted",std::string("wrong id with main ident: %1, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint8_t tempAllow;
            in >> tempAllow;
            player_informations.allow.insert(static_cast<ActionAllow>(tempAllow));
            index++;
        }
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> player_informations.clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player clan leader, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t tempClanLeader;
    in >> tempClanLeader;
    if(tempClanLeader==0x01)
        player_informations.clan_leader=true;
    else
        player_informations.clan_leader=false;

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint64_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    quint64 cash;
    in >> cash;
    player_informations.cash=cash;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint64_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> cash;
    player_informations.warehouse_cash=cash;

    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint8_t monster_list_size;
    in >> monster_list_size;
    uint32_t index=0;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        if(!dataToPlayerMonster(in,monster))
            return false;
        player_informations.playerMonster.push_back(monster);
        index++;
    }
    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the monster list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> monster_list_size;
    index=0;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        if(!dataToPlayerMonster(in,monster))
            return false;
        player_informations.warehouse_playerMonster.push_back(monster);
        index++;
    }
    //reputation
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    PlayerReputation playerReputation;
    uint8_t type;
    index=0;
    uint8_t sub_size8;
    in >> sub_size8;
    while(index<sub_size8)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted","wrong reputation code: "+std::to_string(packetCode)+", and queryNumber: "+std::to_string(queryNumber));
            return false;
        }
        in >> type;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> playerReputation.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int32_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation point, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> playerReputation.point;
        player_informations.reputation[type]=playerReputation;
        index++;
    }
    //compressed block
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint32_t sub_size32;
    in >> sub_size32;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<sub_size32)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    uint32_t decompressedSize=0;
    if(ProtocolParsingBase::compressionTypeClient==CompressionType::None)
    {
        decompressedSize=sub_size32;
        memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data.data()+in.device()->pos(),sub_size32);
    }
    else
        decompressedSize=computeDecompression(data.data()+in.device()->pos(),ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);
    {
        const QByteArray data2(ProtocolParsingBase::tempBigBufferForUncompressedInput,decompressedSize);
        QDataStream in2(data2);
        in2.setVersion(QDataStream::Qt_4_4);in2.setByteOrder(QDataStream::LittleEndian);
        if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }

        //alloc
        if(player_informations.recipes!=NULL)
        {
            delete player_informations.recipes;
            player_informations.recipes=NULL;
        }
        if(player_informations.encyclopedia_monster!=NULL)
        {
            delete player_informations.encyclopedia_monster;
            player_informations.encyclopedia_monster=NULL;
        }
        if(player_informations.encyclopedia_item!=NULL)
        {
            delete player_informations.encyclopedia_item;
            player_informations.encyclopedia_item=NULL;
        }
        if(CommonDatapack::commonDatapack.crafingRecipesMaxId>1)
        {
            player_informations.recipes=(char *)malloc(CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
            memset(player_informations.recipes,0x00,CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
        }
        else
        {
            player_informations.recipes=NULL;
            std::cerr << "player_informations.recipes=NULL;" << std::endl;
        }
        if(CommonDatapack::commonDatapack.monstersMaxId>1)
        {
            player_informations.encyclopedia_monster=(char *)malloc(CommonDatapack::commonDatapack.monstersMaxId/8+1);
            memset(player_informations.encyclopedia_monster,0x00,CommonDatapack::commonDatapack.monstersMaxId/8+1);
        }
        else
        {
            player_informations.encyclopedia_monster=NULL;
            std::cerr << "CommonDatapack::commonDatapack.monstersMaxId=NULL;" << std::endl;
        }
        if(CommonDatapack::commonDatapack.items.itemMaxId>1)
        {
            player_informations.encyclopedia_item=(char *)malloc(CommonDatapack::commonDatapack.items.itemMaxId/8+1);
            memset(player_informations.encyclopedia_item,0x00,CommonDatapack::commonDatapack.items.itemMaxId/8+1);
        }
        else
        {
            player_informations.encyclopedia_item=NULL;
            std::cerr << "CommonDatapack::commonDatapack.items.itemMaxId=NULL;" << std::endl;
        }

        //recipes
        {
            if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16;
            in2 >> sub_size16;
            if(sub_size16>0)
            {
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.crafingRecipesMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1)
                        memcpy(player_informations.recipes,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),CommonDatapack::commonDatapack.crafingRecipesMaxId/8+1);
                    else
                        memcpy(player_informations.recipes,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),sub_size16);
                }
                in2.device()->seek(in2.device()->pos()+sub_size16);
            }
        }

        //encyclopedia_monster
        {
            if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16;
            in2 >> sub_size16;
            if(sub_size16>0)
            {
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.monstersMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.monstersMaxId/8+1)
                        memcpy(player_informations.encyclopedia_monster,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),CommonDatapack::commonDatapack.monstersMaxId/8+1);
                    else
                        memcpy(player_informations.encyclopedia_monster,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),sub_size16);
                }
                in2.device()->seek(in2.device()->pos()+sub_size16);
            }
        }

        //encyclopedia_item
        {
            if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16;
            in2 >> sub_size16;
            if(sub_size16>0)
            {
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(CommonDatapack::commonDatapack.items.itemMaxId>0)
                {
                    if(sub_size16>CommonDatapack::commonDatapack.items.itemMaxId/8+1)
                        memcpy(player_informations.encyclopedia_item,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),CommonDatapack::commonDatapack.items.itemMaxId/8+1);
                    else
                        memcpy(player_informations.encyclopedia_item,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),sub_size16);
                }
                in2.device()->seek(in2.device()->pos()+sub_size16);
            }
        }

        if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in2.device()->seek(in2.device()->pos()+sizeof(uint8_t));

        if(in2.device()->size()!=in2.device()->pos())
        {
            parseError("Procotol wrong or corrupted","wrong size to get the in2.device()->size() "+
                                                                 std::to_string(in2.device()->size())+" != in2.device()->pos() "+
                                                                 std::to_string(in2.device()->pos())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
    }
    in.device()->seek(in.device()->pos()+sub_size32);

    //------------------------------------------- End of common part, start of server specific part ----------------------------------

    //quest
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    PlayerQuest playerQuest;
    uint16_t playerQuestId;
    index=0;
    uint16_t sub_size16;
    in >> sub_size16;
    while(index<sub_size16)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int16_t))
        {
            parseError("Procotol wrong or corrupted","wrong text with main ident: "+std::to_string(packetCode)+", and queryNumber: "+std::to_string(queryNumber));
            return false;
        }
        in >> playerQuestId;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation level, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> playerQuest.step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation point, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        in >> playerQuest.finish_one_time;
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            parseError("Procotol wrong or corrupted",std::string("can't be to step 0 if have never finish the quest, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        player_informations.quests[playerQuestId]=playerQuest;
        index++;
    }

    //compressed block
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    in >> sub_size32;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<sub_size32)
    {
        parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
        return false;
    }
    decompressedSize=0;
    if(ProtocolParsingBase::compressionTypeClient==CompressionType::None)
    {
        decompressedSize=sub_size32;
        memcpy(ProtocolParsingBase::tempBigBufferForUncompressedInput,data.data()+in.device()->pos(),sub_size32);
    }
    else
        decompressedSize=computeDecompression(data.data()+in.device()->pos(),ProtocolParsingBase::tempBigBufferForUncompressedInput,sub_size32,sizeof(ProtocolParsingBase::tempBigBufferForUncompressedInput),ProtocolParsingBase::compressionTypeClient);
    {
        const QByteArray data2(ProtocolParsingBase::tempBigBufferForUncompressedInput,decompressedSize);
        QDataStream in2(data2);
        in2.setVersion(QDataStream::Qt_4_4);in2.setByteOrder(QDataStream::LittleEndian);
        if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }

        //alloc
        if(player_informations.bot_already_beaten!=NULL)
        {
            delete player_informations.bot_already_beaten;
            player_informations.bot_already_beaten=NULL;
        }
        player_informations.bot_already_beaten=(char *)malloc(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        memset(player_informations.bot_already_beaten,0x00,CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
        if(CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId<=0)
             std::cerr << "CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId == 0, take care" << std::endl;

        //bot
        {
            if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the max player, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t sub_size16;
            in2 >> sub_size16;
            if(sub_size16>0)
            {
                if(in2.device()->pos()<0 || !in2.device()->isOpen() || (in2.device()->size()-in2.device()->pos())<sub_size16)
                {
                    parseError("Procotol wrong or corrupted",std::string("wrong size to get the reputation list size, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                    return false;
                }
                if(sub_size16>CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1)
                    memcpy(player_informations.bot_already_beaten,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),CommonDatapackServerSpec::commonDatapackServerSpec.botFightsMaxId/8+1);
                else
                    memcpy(player_informations.bot_already_beaten,ProtocolParsingBase::tempBigBufferForUncompressedInput+in2.device()->pos(),sub_size16);
                in2.device()->seek(in2.device()->pos()+sub_size16);
            }
        }

        if(in2.device()->size()!=in2.device()->pos())
        {
            parseError("Procotol wrong or corrupted","wrong size to get the in2.device()->size() "+
                       std::to_string(in2.device()->size())+" != in2.device()->pos() "+
                       std::to_string(in2.device()->pos())+", line: "+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
    }
    in.device()->seek(in.device()->pos()+sub_size32);

    //item on map
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint16_t itemOnMapSize;
        in >> itemOnMapSize;
        uint16_t index=0;
        while(index<itemOnMapSize)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t itemOnMap;
            in >> itemOnMap;
            player_informations.itemOnMap.insert(itemOnMap);
            index++;
        }
    }

    //plant on map
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError("Procotol wrong or corrupted",std::string("wrong size to get the player cash ware house, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
            return false;
        }
        uint16_t plantOnMapSize;
        in >> plantOnMapSize;
        uint16_t index=0;
        while(index<plantOnMapSize)
        {
            PlayerPlant playerPlant;

            //plant on map,x,y getted
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t plantOnMap;
            in >> plantOnMap;

            //plant
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            in >> playerPlant.plant;

            //seconds to mature
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError("Procotol wrong or corrupted",std::string("wrong size to get the player item on map, line: ")+std::string(__FILE__)+":"+std::to_string(__LINE__));
                return false;
            }
            uint16_t seconds_to_mature;
            in >> seconds_to_mature;
            playerPlant.mature_at=QDateTime::currentMSecsSinceEpoch()/1000+seconds_to_mature;

            player_informations.plantOnMap[plantOnMap]=playerPlant;
            index++;
        }
    }

    character_selected=true;
    haveCharacter();
    return true;
}
