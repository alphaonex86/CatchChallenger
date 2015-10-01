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

void Api_protocol::parseCharacterBlock(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber,const QByteArray &data)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max player, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> max_players;
    setMaxPlayers(max_players);

    uint32_t captureRemainingTime;
    uint8_t captureFrequencyType;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture remainingTime, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> captureRemainingTime;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture type, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> captureFrequencyType;
    switch(captureFrequencyType)
    {
        case 0x01:
        case 0x02:
        break;
        default:
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong captureFrequencyType, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    cityCapture(captureRemainingTime,captureFrequencyType);

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.waitBeforeConnectAfterKick;
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t tempForceClientToSendAtBorder;
        in >> tempForceClientToSendAtBorder;
        if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("forceClientToSendAtBorder have wrong value, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        CommonSettingsServer::commonSettingsServer.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.forcedSpeed;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the forcedSpeed, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.useSP;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the tcpCork, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.tcpCork;
    {
        socket->setTcpCork(CommonSettingsServer::commonSettingsServer.tcpCork);
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.autoLearn;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.dontSendPseudo;


    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_gold;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_xp_pow;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.rates_drop;

    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_all;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_local, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_local;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_private, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_private;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.chat_allow_clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the factoryPriceChange, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> CommonSettingsServer::commonSettingsServer.factoryPriceChange;

    {
        //Main type code
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            CommonSettingsServer::commonSettingsServer.mainDatapackCode=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(CommonSettingsServer::commonSettingsServer.mainDatapackCode.isEmpty())
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("mainDatapackCode is empty, please put it into the settings"));
                return;
            }
            if(!CommonSettingsServer::commonSettingsServer.mainDatapackCode.contains(QRegularExpression(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),
                           QStringLiteral("CommonSettingsServer::commonSettingsServer.mainDatapackCode not match CATCHCHALLENGER_CHECK_MAINDATAPACKCODE %1 not contain %2")
                           .arg(CommonSettingsServer::commonSettingsServer.mainDatapackCode)
                           .arg(QStringLiteral(CATCHCHALLENGER_CHECK_MAINDATAPACKCODE))
                           );
                return;
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            CommonSettingsServer::commonSettingsServer.subDatapackCode=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());

            if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
            {
                if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.contains(QRegularExpression(CATCHCHALLENGER_CHECK_SUBDATAPACKCODE)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("CommonSettingsServer::commonSettingsServer.subDatapackCode not match CATCHCHALLENGER_CHECK_SUBDATAPACKCODE"));
                    return;
                }
            }
        }
        else
            CommonSettingsServer::commonSettingsServer.subDatapackCode.clear();

        //here to be sure of the order
        if(CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
            mDatapackSub=mDatapackMain+"sub/emptyRandomCoder23osaQ9mb5hYh2j/";
        else
            mDatapackSub=mDatapackMain+"sub/"+CommonSettingsServer::commonSettingsServer.subDatapackCode+"/";
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    CommonSettingsServer::commonSettingsServer.datapackHashServerMain=data.mid(in.device()->pos(),28);
    in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerMain.size());

    if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty())
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        CommonSettingsServer::commonSettingsServer.datapackHashServerSub=data.mid(in.device()->pos(),28);
        in.device()->seek(in.device()->pos()+CommonSettingsServer::commonSettingsServer.datapackHashServerSub.size());
    }

    {
        //the mirror
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t mirrorSize;
        in >> mirrorSize;
        if(mirrorSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)mirrorSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            QByteArray rawText=data.mid(in.device()->pos(),mirrorSize);
            CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.contains(QRegularExpression("^https?://")))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("mirror with not http(s) protocol with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
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
            return;
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
            return;
        }
        in >> player_informations.public_informations.simplifiedId;
    }
    {
        //pseudo
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t textSize;
        in >> textSize;
        if(textSize>0)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            QByteArray rawText=data.mid(in.device()->pos(),textSize);
            player_informations.public_informations.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
        }
        else
            player_informations.public_informations.pseudo.clear();
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    uint8_t tempAllowSize;
    in >> tempAllowSize;
    {
        int index=0;
        while(index<tempAllowSize)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong id with main ident: %1, line: %2").arg(mainCodeType).arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            uint8_t tempAllow;
            in >> tempAllow;
            player_informations.allow << static_cast<ActionAllow>(tempAllow);
            index++;
        }
    }
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> player_informations.clan;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan leader, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
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
            return;
        }
        in >> tempListSize;
        uint8_t event,value;
        int index=0;
        while(index<tempListSize)
        {

            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> event;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event value, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
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
        return;
    }
    in >> player_informations.cash;
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint64_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    in >> player_informations.warehouse_cash;

    //item on map
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t itemOnMapSize;
        in >> itemOnMapSize;
        uint8_t index=0;
        while(index<itemOnMapSize)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            uint8_t itemOnMap;
            in >> itemOnMap;
            player_informations.itemOnMap << itemOnMap;
            index++;
        }
    }

    //plant on map
    if(CommonSettingsServer::commonSettingsServer.plantOnlyVisibleByPlayer)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
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
                return;
            }
            uint8_t plantOnMap;
            in >> plantOnMap;

            //plant
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> playerPlant.plant;

            //seconds to mature
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
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
        return;
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
            return;
        }
        in >> recipeId;
        player_informations.recipes << recipeId;
        index++;
    }

    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
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
            return;
        }
        in >> monster.id;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.monster;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.remaining_xp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.hp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.sp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.catched_with;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
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
                return;
            break;
        }
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.egg_step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.character_origin;

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t sub_size8;
        in >> sub_size8;
        sub_index=0;
        while(sub_index<sub_size8)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> buff.buff;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> buff.level;
            monster.buffs << buff;
            sub_index++;
        }

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint16_t sub_size16;
        in >> sub_size16;
        sub_index=0;
        while(sub_index<sub_size16)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.skill;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.level;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.endurance;
            monster.skills << skill;
            sub_index++;
        }
        player_informations.playerMonster << monster;
        index++;
    }
    //monsters
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
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
            return;
        }
        in >> monster.id;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.monster;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.remaining_xp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.hp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.sp;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.catched_with;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
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
                return;
            break;
        }
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.egg_step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> monster.character_origin;

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint8_t sub_size8;
        in >> sub_size8;
        sub_index=0;
        while(sub_index<sub_size8)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> buff.buff;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> buff.level;
            monster.buffs << buff;
            sub_index++;
        }

        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        uint16_t sub_size16;
        in >> sub_size16;
        sub_index=0;
        while(sub_index<sub_size16)
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.skill;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.level;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
                return;
            }
            in >> skill.endurance;
            monster.skills << skill;
            sub_index++;
        }
        player_informations.warehouse_playerMonster << monster;
        index++;
    }
    //reputation
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong reputation code: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            return;
        }
        in >> type;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> playerReputation.level;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int32_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> playerReputation.point;
        player_informations.reputation[type]=playerReputation;
        index++;
    }
    //quest
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint8_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
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
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            return;
        }
        in >> playerQuestId;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> playerQuest.step;
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(int8_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        in >> playerQuest.finish_one_time;
        if(playerQuest.step<=0 && !playerQuest.finish_one_time)
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("can't be to step 0 if have never finish the quest, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
            return;
        }
        player_informations.quests[playerQuestId]=playerQuest;
        index++;
    }
    //bot_already_beaten
    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(QStringLiteral("%1:%2").arg(__FILE__).arg(__LINE__)));
        return;
    }
    uint16_t bot_already_beaten;
    index=0;
    in >> sub_size16;
    while(index<sub_size16)
    {
        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(uint16_t))
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            return;
        }
        in >> bot_already_beaten;
        player_informations.bot_already_beaten << bot_already_beaten;
        index++;
    }
    character_selected=true;
    haveCharacter();
}
