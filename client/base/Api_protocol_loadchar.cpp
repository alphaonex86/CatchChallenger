#include "Api_protocol.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"
#include "LanguagesSelect.h"

#include <QRegularExpression>

using namespace CatchChallenger;

bool Api_protocol::parseCharacterBlock(const uint8_t &packetCode, const uint8_t &queryNumber, const QByteArray &data)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max player, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }

    uint16_t max_players;
    in >> max_players;
    if(max_players==65534)
        setMaxPlayers(255);
    else
        setMaxPlayers(max_players);
    this->max_players=max_players;

    uint32_t captureRemainingTime;
    uint8_t captureFrequencyType;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture remainingTime, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> captureRemainingTime;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture type, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> captureFrequencyType;
    switch(captureFrequencyType)
    {
        case 0x01:
        case 0x02:
        break;
        default:
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong captureFrequencyType, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    cityCapture(captureRemainingTime,captureFrequencyType);

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t tempForceClientToSendAtBorder;
        in >> tempForceClientToSendAtBorder;
        if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("forceClientToSendAtBorder have wrong value, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.forcedSpeed;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the forcedSpeed, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.useSP;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the tcpCork, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.tcpCork;
    {
        socket->setTcpCork(CommonSettingsServer::commonSettingsServer.tcpCork);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.autoLearn;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.dontSendPseudo;


    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_gold;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_drop;

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_all;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_local, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_local;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_private, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_private;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the factoryPriceChange, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> CommonSettingsServer::commonSettingsServer.factoryPriceChange;

    {
        //Main type code
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.empty())
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("mainDatapackCode is empty, please put it into the settings"));
                return false;
            }
            if(!regex_search(CommonSettingsServer::commonSettingsServer.mainDatapackCode,std::regex(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),
                           QStringLiteral("CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE %1 not contain %2")
                           .arg(QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode))
                           .arg(QStringLiteral(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE))
                           );
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.mainDatapackCode.clear();

        //here to be sure of the order
        mDatapackMain=mDatapackBase+"map/main/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.mainDatapackCode)+"/";
    }
    {
        //Sub type code
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            CommonSettingsServer::commonSettingsServer.subDatapackCode=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
            {
                if(!regex_search(CommonSettingsServer::commonSettingsServer.subDatapackCode,std::regex(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("CommonSettingsServer::commonSettingsServer.subDatapackCode not match CATCHCHALLENGER_CHECK_SUBDATAPACKCODE"));
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
            mDatapackSub=mDatapackMain+"sub/"+QString::fromStdString(CommonSettingsServer::commonSettingsServer.subDatapackCode)+"/";
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain.resize(28);
    memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerMain.data(),data.mid(in.device()->pos(),28).constData(),28);
    in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size());

    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub.resize(28);
        memcpy(CommonSettingsServer::commonSettingsServer.datapackHashServerSub.data(),data.mid(in.device()->pos(),28).constData(),28);
        in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size());
    }

    {
        //the mirror
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t mirrorSize;
        in >> mirrorSize;
        if(mirrorSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),mirrorSize);
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(!regex_search(CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer,std::regex("^https?://")))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("mirror with not http(s) protocol with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.clear();
    }

    if(max_players<=255)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> player_informations.public_informations.simplifiedId;
    }
    {
        //pseudo
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            player_informations.public_informations.pseudo=std::string(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
        }
        else
            player_informations.public_informations.pseudo.clear();
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong id with main ident: %1, line: %2").arg(packetCode).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> player_informations.clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan leader, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    uint8_t tempClanLeader;
    in >> tempClanLeader;
    if(tempClanLeader==0x01)
        player_informations.clan_leader=true;
    else
        player_informations.clan_leader=false;
    {
        QList<QPair<uint8_t,uint8_t> > events;
        uint8_t tempListSize;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> tempListSize;
        uint8_t event,value;
        int index=0;
        while(index<tempListSize)
        {

            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> event;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event value, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> value;
            index++;
            events << QPair<uint8_t,uint8_t>(event,value);
        }
        setEvents(events);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint64_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    quint64 cash;
    in >> cash;
    player_informations.cash=cash;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint64_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> cash;
    player_informations.warehouse_cash=cash;

    //item on map
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t itemOnMapSize;
        in >> itemOnMapSize;
        uint8_t index=0;
        while(index<itemOnMapSize)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t itemOnMap;
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t plantOnMapSize;
        in >> plantOnMapSize;
        uint8_t index=0;
        while(index<plantOnMapSize)
        {
            PlayerPlant playerPlant;

            //plant on map,x,y getted
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint8_t plantOnMap;
            in >> plantOnMap;

            //plant
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> playerPlant.plant;

            //seconds to mature
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            uint16_t seconds_to_mature;
            in >> seconds_to_mature;
            playerPlant.mature_at=QDateTime::currentMSecsSinceEpoch()/1000+seconds_to_mature;

            player_informations.plantOnMap[plantOnMap]=playerPlant;
            index++;
        }
    }

    //recipes
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the recipe list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    uint16_t recipe_list_size;
    in >> recipe_list_size;
    uint16_t recipeId;
    uint32_t index=0;
    while(index<recipe_list_size)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player local recipe, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> recipeId;
        player_informations.recipes.insert(recipeId);
        index++;
    }

    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    uint8_t gender;
    uint8_t monster_list_size;
    in >> monster_list_size;
    index=0;
    uint32_t sub_index;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        PlayerBuff buff;
        PlayerMonster::PlayerSkill skill;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.id;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.monster;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.remaining_xp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.hp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.sp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.catched_with;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> gender;
        switch(gender)
        {
            case 0x01:
            case 0x02:
            case 0x03:
                monster.gender=(Gender)gender;
            break;
            default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                return false;
            break;
        }
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.egg_step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.character_origin;

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t sub_size8;
        in >> sub_size8;
        sub_index=0;
        while(sub_index<sub_size8)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> buff.buff;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> buff.level;
            monster.buffs.push_back(buff);
            sub_index++;
        }

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint16_t sub_size16;
        in >> sub_size16;
        sub_index=0;
        while(sub_index<sub_size16)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.skill;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.level;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.endurance;
            monster.skills.push_back(skill);
            sub_index++;
        }
        player_informations.playerMonster.push_back(monster);
        index++;
    }
    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    in >> monster_list_size;
    index=0;
    while(index<monster_list_size)
    {
        PlayerMonster monster;
        PlayerBuff buff;
        PlayerMonster::PlayerSkill skill;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.id;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.monster;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.remaining_xp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.hp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.sp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.catched_with;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> gender;
        switch(gender)
        {
            case 0x01:
            case 0x02:
            case 0x03:
                monster.gender=(Gender)gender;
            break;
            default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)).arg(gender));
                return false;
            break;
        }
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.egg_step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> monster.character_origin;

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint8_t sub_size8;
        in >> sub_size8;
        sub_index=0;
        while(sub_index<sub_size8)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> buff.buff;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> buff.level;
            monster.buffs.push_back(buff);
            sub_index++;
        }

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        uint16_t sub_size16;
        in >> sub_size16;
        sub_index=0;
        while(sub_index<sub_size16)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.skill;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.level;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return false;
            }
            in >> skill.endurance;
            monster.skills.push_back(skill);
            sub_index++;
        }
        player_informations.warehouse_playerMonster.push_back(monster);
        index++;
    }
    //reputation
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong reputation code: %1, and queryNumber: %2").arg(packetCode).arg(queryNumber));
            return false;
        }
        in >> type;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> playerReputation.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> playerReputation.point;
        player_informations.reputation[type]=playerReputation;
        index++;
    }
    //quest
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, and queryNumber: %2").arg(packetCode).arg(queryNumber));
            return false;
        }
        in >> playerQuestId;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> playerQuest.step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        in >> playerQuest.finish_one_time;
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("can't be to step 0 if have never finish the quest, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return false;
        }
        player_informations.quests[playerQuestId]=playerQuest;
        index++;
    }
    //bot_already_beaten
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return false;
    }
    uint16_t bot_already_beaten;
    index=0;
    in >> sub_size16;
    while(index<sub_size16)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, and queryNumber: %2").arg(packetCode).arg(queryNumber));
            return false;
        }
        in >> bot_already_beaten;
        player_informations.bot_already_beaten.insert(bot_already_beaten);
        index++;
    }
    character_selected=true;
    haveCharacter();
    return true;
}
