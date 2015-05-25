#include "Api_protocol.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettingsCommon.h"
#include "../../general/base/CommonSettingsServer.h"
#include "../../general/base/FacilityLib.h"
#include "../../general/base/GeneralType.h"
#include "LanguagesSelect.h"

#include <QCoreApplication>
#include <QDebug>

#ifdef BENCHMARKMUTIPLECLIENT
#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const char * const data,const unsigned int &size)
{
    parseMessage(mainCodeType,QByteArray(data,size));
}

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
        return;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        //Insert player on map
        case 0xC0:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            quint8 mapListSize;
            in >> mapListSize;
            int index=0;
            while(index<mapListSize)
            {
                quint32 mapId;
                if(number_of_map<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }

                quint16 playerSizeList;
                if(max_players<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    Player_public_informations public_informations;
                    if(max_players<=255)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 playerSmallId;
                        in >> playerSmallId;
                        public_informations.simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> public_informations.simplifiedId;
                    }

                    //x and y
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 directionAndPlayerType;
                    in >> directionAndPlayerType;
                    quint8 directionInt,playerTypeInt;
                    directionInt=directionAndPlayerType & 0x0F;
                    playerTypeInt=directionAndPlayerType & 0xF0;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, directionAndPlayerType: %3, line: %4").arg(directionInt).arg(mainCodeType).arg(directionAndPlayerType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;
                    Player_type playerType=(Player_type)playerTypeInt;
                    if(playerType!=Player_type_normal && playerType!=Player_type_premium && playerType!=Player_type_gm && playerType!=Player_type_dev)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("playerType have wrong value: %1, at main ident: %2, directionAndPlayerType: %3, line: %4").arg(playerTypeInt).arg(mainCodeType).arg(directionAndPlayerType).arg(__LINE__));
                        return;
                    }
                    public_informations.type=playerType;

                    //the speed
                    if(CommonSettingsServer::commonSettingsServer.forcedSpeed==0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> public_informations.speed;
                    }
                    else
                        public_informations.speed=CommonSettingsServer::commonSettingsServer.forcedSpeed;

                    if(!CommonSettingsServer::commonSettingsServer.dontSendPseudo)
                    {
                        //the pseudo
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 pseudoSize;
                        in >> pseudoSize;
                        if(pseudoSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                return;
                            }
                            QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                            public_informations.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                            in.device()->seek(in.device()->pos()+rawText.size());
                            if(public_informations.pseudo.isEmpty())
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed for pseudo: %1, rawData: %2, line: %3").arg(mainCodeType).arg(QString(rawText.toHex())).arg(__LINE__));
                                return;
                            }
                        }
                    }
                    else
                        public_informations.pseudo=QString();

                    //the skin
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 skinId;
                    in >> skinId;
                    public_informations.skinId=skinId;

                    if(public_informations.simplifiedId==player_informations.public_informations.simplifiedId)
                    {
                        setLastDirection(direction);
                        player_informations.public_informations=public_informations;
                        have_current_player_info(player_informations);
                    }
                    insert_player(public_informations,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
        }
        break;
        //Move player on map
        case 0xC7:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            //move the player
            quint8 directionInt,moveListSize;
            quint16 playerSizeList;
            if(max_players<=255)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                quint8 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("move size == 0 with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
                        new_movement.second=(Direction)directionInt;
                        movement << new_movement;
                        index_sub_loop++;
                    }
                    move_player(playerId,movement);
                    index++;
                }
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("move size == 0 with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
                        new_movement.second=(Direction)directionInt;
                        movement << new_movement;
                        index_sub_loop++;
                    }
                    move_player(playerId,movement);
                    index++;
                }
            }
        }
        #else
        return;
        #endif
        break;
        //Remove player from map
        case 0xC8:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            //remove player
            quint16 playerSizeList;
            if(max_players<=255)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                quint8 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    remove_player(playerId);
                    index++;
                }
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    remove_player(playerId);
                    index++;
                }
            }
        }
        #else
        return;
        #endif
        break;
        //Player number
        case 0xC3:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(max_players<=255)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 current_player_connected_8Bits;
                in >> current_player_connected_8Bits;
                number_of_player(current_player_connected_8Bits,max_players);
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2, in.device()->pos(): %3, in.device()->size(): %4, in.device()->isOpen(): %5").arg(mainCodeType).arg(__LINE__).arg(in.device()->pos()).arg(in.device()->size()).arg(in.device()->isOpen()));
                    return;
                }
                quint16 current_player_connected_16Bits;
                in >> current_player_connected_16Bits;
                number_of_player(current_player_connected_16Bits,max_players);
            }
        }
        #else
        return;
        #endif
        break;
        //drop all player on the map
        case 0xC4:
        #ifndef BENCHMARKMUTIPLECLIENT
            dropAllPlayerOnTheMap();
        #else
        return;
        #endif
        break;
        //Reinser player on same map
        case 0xC5:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            quint16 playerSizeList;
            if(max_players<=255)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
            }
            int index_sub_loop=0;
            while(index_sub_loop<playerSizeList)
            {
                //player id
                quint16 simplifiedId;
                if(max_players<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 playerSmallId;
                    in >> playerSmallId;
                    simplifiedId=playerSmallId;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> simplifiedId;
                }

                //x and y
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;

                //direction and player type
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 directionInt;
                in >> directionInt;
                if(directionInt<1 || directionInt>8)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                    return;
                }
                Direction direction=(Direction)directionInt;

                reinsert_player(simplifiedId,x,y,direction);
                index_sub_loop++;
            }
        }
        #else
        return;
        #endif
        break;
        //Reinser player on other map
        case 0xC6:
        #ifndef BENCHMARKMUTIPLECLIENT
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            quint8 mapListSize;
            in >> mapListSize;
            int index=0;
            while(index<mapListSize)
            {
                quint32 mapId;
                if(number_of_map<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                quint16 playerSizeList;
                if(max_players<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    quint16 simplifiedId;
                    if(max_players<=255)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 playerSmallId;
                        in >> playerSmallId;
                        simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> simplifiedId;
                    }

                    //x and y
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 directionInt;
                    in >> directionInt;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    full_reinsert_player(simplifiedId,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }

        }
        #else
        return;
        #endif
        break;
        //chat as input
        case 0xCA:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            quint8 chat_type_int;
            in >> chat_type_int;
            if(chat_type_int<1 || chat_type_int>8)
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong chat type with main ident: %1, chat_type_int: %2, line: %3").arg(mainCodeType).arg(chat_type_int).arg(__LINE__));
                return;
            }
            Chat_type chat_type=(Chat_type)chat_type_int;
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            QString text;
            quint8 textSize;
            in >> textSize;
            if(textSize>0)
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)textSize)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, pseudoSize: %2, data: %3, line: %4")
                                  .arg(mainCodeType)
                                  .arg(textSize)
                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                  .arg(__LINE__)
                                  );
                    return;
                }
                QByteArray rawText=data.mid(in.device()->pos(),textSize);
                text=QString::fromUtf8(rawText.data(),rawText.size());
                in.device()->seek(in.device()->pos()+rawText.size());
            }
            if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
                new_system_text(chat_type,text);
            else
            {
                QString pseudo;
                quint8 pseudoSize;
                in >> pseudoSize;
                if(pseudoSize>0)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, pseudoSize: %2, data: %3, line: %4")
                                      .arg(mainCodeType)
                                      .arg(pseudoSize)
                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                    pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(pseudo.isEmpty())
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, rawText.data(): %2, rawText.size(): %3, line: %4")
                                      .arg(mainCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                }
                quint8 player_type_int;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> player_type_int;
                Player_type player_type=(Player_type)player_type_int;
                if(player_type!=Player_type_normal && player_type!=Player_type_premium && player_type!=Player_type_gm && player_type!=Player_type_dev)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong player type with main ident: %1, player_type_int: %2, line: %3").arg(mainCodeType).arg(player_type_int).arg(__LINE__));
                    return;
                }
                new_chat_text(chat_type,text,pseudo,player_type);
            }
        }
        break;
        //Insert plant on map
        case 0xD1:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            quint16 plantListSize;
            in >> plantListSize;
            int index=0;
            while(index<plantListSize)
            {
                quint32 mapId;
                if(number_of_map<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;
                //plant
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 plant;
                in >> plant;
                //seconds to mature
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint16 seconds_to_mature;
                in >> seconds_to_mature;

                insert_plant(mapId,x,y,plant,seconds_to_mature);
                index++;
            }
        }
        break;
        //Remove plant on map
        case 0xD2:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            quint16 plantListSize;
            in >> plantListSize;
            int index=0;
            while(index<plantListSize)
            {
                quint32 mapId;
                if(number_of_map<=255)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;

                remove_plant(mapId,x,y);
                index++;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident main code: %1").arg(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseMessage(%1,%2 %3) line %4")
                      .arg(mainCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      .arg(__LINE__)
                      );
        return;
    }
}

void Api_protocol::parseFullMessage(const quint8 &mainCodeType, const quint8 &subCodeType, const char * const data, const unsigned int &size)
{
    parseFullMessage(mainCodeType,subCodeType,QByteArray(data,size));
}

void Api_protocol::parseFullMessage(const quint8 &mainCodeType,const quint8 &subCodeType,const QByteArray &data)
{
    if(!is_logged)
    {
        if(mainCodeType==0xC2 && (subCodeType==0x0E || subCodeType==0x0F))
        {}
        else
        {
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
            return;
        }
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);in.setByteOrder(QDataStream::LittleEndian);
    switch(mainCodeType)
    {
        case 0xC2:
        {
            switch(subCodeType)
            {
                //file as input
                case 0x03://raw
                case 0x04://compressed
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 fileListSize;
                    in >> fileListSize;
                    int index=0;
                    while(index<fileListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QString fileName;
                        quint8 fileNameSize;
                        in >> fileNameSize;
                        if(fileNameSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)fileNameSize)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                            in.device()->seek(in.device()->pos()+rawText.size());
                            fileName=QString::fromUtf8(rawText.data(),rawText.size());
                        }
                        if(!extensionAllowed.contains(QFileInfo(fileName).suffix()))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("extension not allowed: %4 with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__).arg(QFileInfo(fileName).suffix()));
                            if(!tolerantMode)
                                return;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint32 size;
                        in >> size;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<size)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong file data size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QByteArray dataFile=data.mid(in.device()->pos(),size);
                        in.device()->seek(in.device()->pos()+size);
                        if(subCodeType==0x03)
                            DebugClass::debugConsole(QStringLiteral("Raw file to create: %1").arg(fileName));
                        else
                            DebugClass::debugConsole(QStringLiteral("Compressed file to create: %1").arg(fileName));
                        newFileBase(fileName,dataFile);
                        index++;
                    }
                    return;//no remaining data, because all remaing is used as file data
                }
                break;
                //kicked/ban and reason
                case 0x08:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 code;
                    in >> code;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString text;
                    in >> text;
                    switch(code)
                    {
                        case 0x01:
                            parseError(QStringLiteral("Disconnected by the server"),QStringLiteral("You have been kicked by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        case 0x02:
                            parseError(QStringLiteral("Disconnected by the server"),QStringLiteral("You have been ban by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        case 0x03:
                            parseError(QStringLiteral("Disconnected by the server"),QStringLiteral("You have been disconnected by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        default:
                            parseError(QStringLiteral("Disconnected by the server"),QStringLiteral("You have been disconnected by strange way by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                    }
                }
                break;
                //clan dissolved
                case 0x09:
                    clanDissolved();
                break;
                //clan info
                case 0x0A:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString name;
                    in >> name;
                    clanInformations(name);
                }
                break;
                //clan invite
                case 0x0B:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint32 clanId;
                    in >> clanId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString name;
                    in >> name;
                    clanInvite(clanId,name);
                }
                break;
                //Send datapack send size
                case 0x0C:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint32 datapckFileNumber;
                    in >> datapckFileNumber;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint32 datapckFileSize;
                    in >> datapckFileSize;
                    datapackSizeBase(datapckFileNumber,datapckFileSize);
                }
                break;
                //Update file http
                case 0x0D:
                {
                    QString baseHttp;
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(data.mid(in.device()->pos()).toHex()))
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                        quint8 baseHttpSize;
                        in >> baseHttpSize;
                        if(baseHttpSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)baseHttpSize)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                              .arg(mainCodeType)
                                              .arg(subCodeType)
                                              .arg(baseHttpSize)
                                              .arg(QString(data.mid(in.device()->pos()).toHex()))
                                              .arg(__LINE__)
                                              );
                                return;
                            }
                            QByteArray baseHttpRaw=data.mid(in.device()->pos(),baseHttpSize);
                            baseHttp=QString::fromUtf8(baseHttpRaw.data(),baseHttpRaw.size());
                            in.device()->seek(in.device()->pos()+baseHttpRaw.size());
                            if(baseHttp.isEmpty())
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                              .arg(mainCodeType)
                                              .arg(subCodeType)
                                              .arg(QString(baseHttpRaw.toHex()))
                                              .arg(baseHttpRaw.size())
                                              .arg(__LINE__)
                                              );
                                return;
                            }
                        }
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    quint32 fileListSize;
                    in >> fileListSize;
                    quint32 index=0;
                    while(index<fileListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QString fileName;
                        quint8 fileNameSize;
                        in >> fileNameSize;
                        if(fileNameSize>0)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)fileNameSize)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                            in.device()->seek(in.device()->pos()+rawText.size());
                            fileName=QString::fromUtf8(rawText.data(),rawText.size());
                        }
                        if(!extensionAllowed.contains(QFileInfo(fileName).suffix()))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("extension not allowed: %4 with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__).arg(QFileInfo(fileName).suffix()));
                            if(!tolerantMode)
                                return;
                        }
                        newHttpFileBase(baseHttp+fileName,fileName);

                        index++;
                    }
                }
                break;
                //Internal server list for the current pool
                case 0x0E:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 serverMode;
                    in >> serverMode;
                    if(serverMode<1 || serverMode>2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown serverMode main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    proxyMode=ProxyMode(serverMode);
                    quint8 serverListSize=0;
                    quint8 serverListIndex;
                    QList<ServerFromPoolForDisplayTemp> serverTempList;
                    if(proxyMode==ProxyMode::Reconnect)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> serverListSize;
                        serverListIndex=0;
                        while(serverListIndex<serverListSize)
                        {
                            ServerFromPoolForDisplayTemp server;
                            server.currentPlayer=0;
                            server.maxPlayer=0;
                            server.port=0;
                            server.uniqueKey=0;
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.charactersGroupIndex;
                            }
                            //host
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                quint8 stringSize;
                                in >> stringSize;
                                if(stringSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)stringSize)
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                                      .arg(mainCodeType)
                                                      .arg(subCodeType)
                                                      .arg(stringSize)
                                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                      .arg(__LINE__)
                                                      );
                                        return;
                                    }
                                    QByteArray stringRaw=data.mid(in.device()->pos(),stringSize);
                                    server.host=QString::fromUtf8(stringRaw.data(),stringRaw.size());
                                    in.device()->seek(in.device()->pos()+stringRaw.size());
                                }
                            }
                            //port
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.port;
                            }
                            //uniquekey
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.uniqueKey;
                            }
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                quint16 stringSize;
                                in >> stringSize;
                                if(stringSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)stringSize)
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                                      .arg(mainCodeType)
                                                      .arg(subCodeType)
                                                      .arg(stringSize)
                                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                      .arg(__LINE__)
                                                      );
                                        return;
                                    }
                                    QByteArray stringRaw=data.mid(in.device()->pos(),stringSize);
                                    server.xml=QString::fromUtf8(stringRaw.data(),stringRaw.size());
                                    in.device()->seek(in.device()->pos()+stringRaw.size());
                                }
                            }
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.logicalGroupIndex;
                            }
                            serverTempList << server;
                            serverListIndex++;
                        }
                    }
                    else
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> serverListSize;
                        serverListIndex=0;
                        while(serverListIndex<serverListSize)
                        {
                            ServerFromPoolForDisplayTemp server;
                            server.currentPlayer=0;
                            server.maxPlayer=0;
                            server.port=0;
                            server.uniqueKey=0;
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.charactersGroupIndex;
                            }
                            //uniquekey
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.uniqueKey;
                            }
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                quint16 stringSize;
                                in >> stringSize;
                                if(stringSize>0)
                                {
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)stringSize)
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                                      .arg(mainCodeType)
                                                      .arg(subCodeType)
                                                      .arg(stringSize)
                                                      .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                      .arg(__LINE__)
                                                      );
                                        return;
                                    }
                                    QByteArray stringRaw=data.mid(in.device()->pos(),stringSize);
                                    server.xml=QString::fromUtf8(stringRaw.data(),stringRaw.size());
                                    in.device()->seek(in.device()->pos()+stringRaw.size());
                                }
                            }
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                in >> server.logicalGroupIndex;
                            }
                            serverTempList << server;
                            serverListIndex++;
                        }
                    }
                    serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(data.mid(in.device()->pos()).toHex()))
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                        in >> serverTempList[serverListIndex].maxPlayer;
                        in >> serverTempList[serverListIndex].currentPlayer;
                        serverListIndex++;
                    }
                    serverOrdenedList.clear();
                    characterListForSelection.clear();
                    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                    serverListIndex=0;
                    while(serverListIndex<serverListSize)
                    {
                        ServerFromPoolForDisplay * tempVar=addLogicalServer(serverTempList.at(serverListIndex),language);
                        tempVar->serverOrdenedListIndex=serverOrdenedList.size();
                        serverOrdenedList << tempVar;
                        serverListIndex++;
                    }
                }
                break;
                //Logical group
                case 0x0F:
                {
                    logicialGroup.logicialGroupList.clear();
                    logicialGroup.name.clear();
                    logicialGroup.servers.clear();

                    quint8 logicalGroupSize=0;
                    quint8 logicalGroupIndex=0;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    logicialGroupIndexList.clear();
                    const QString &language=LanguagesSelect::languagesSelect->getCurrentLanguages();
                    in >> logicalGroupSize;
                    while(logicalGroupIndex<logicalGroupSize)
                    {
                        QString path;
                        QString xml;
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                              .arg(mainCodeType)
                                              .arg(subCodeType)
                                              .arg(QString(data.mid(in.device()->pos()).toHex()))
                                              .arg(__LINE__)
                                              );
                                return;
                            }
                            quint8 pathSize;
                            in >> pathSize;
                            if(pathSize>0)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pathSize)
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(pathSize)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                QByteArray pathRaw=data.mid(in.device()->pos(),pathSize);
                                path=QString::fromUtf8(pathRaw.data(),pathRaw.size());
                                in.device()->seek(in.device()->pos()+pathRaw.size());
                            }
                        }
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, data: %3, line: %4")
                                              .arg(mainCodeType)
                                              .arg(subCodeType)
                                              .arg(QString(data.mid(in.device()->pos()).toHex()))
                                              .arg(__LINE__)
                                              );
                                return;
                            }
                            quint16 xmlSize;
                            in >> xmlSize;
                            if(xmlSize>0)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)xmlSize)
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                                  .arg(mainCodeType)
                                                  .arg(subCodeType)
                                                  .arg(xmlSize)
                                                  .arg(QString(data.mid(in.device()->pos()).toHex()))
                                                  .arg(__LINE__)
                                                  );
                                    return;
                                }
                                QByteArray xmlRaw=data.mid(in.device()->pos(),xmlSize);
                                xml=QString::fromUtf8(xmlRaw.data(),xmlRaw.size());
                                in.device()->seek(in.device()->pos()+xmlRaw.size());
                            }
                        }

                        logicialGroupIndexList << addLogicalGroup(path,xml,language);
                        logicalGroupIndex++;
                    }
                }
                break;
                //Logical group
                case 0x11:
                {
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        case 0xD0:
        {
            switch(subCodeType)
            {
                //Send the inventory
                case 0x01:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<CATCHCHALLENGER_TYPE_ITEM,quint32> items;
                    quint16 inventorySize,id;
                    quint32 quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint16,quint32> warehouse_items;
                    in >> inventorySize;
                    index=0;
                    while(index<inventorySize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(warehouse_items.contains(id))
                            warehouse_items[id]+=quantity;
                        else
                            warehouse_items[id]=quantity;
                        index++;
                    }
                    have_inventory(items,warehouse_items);
                }
                break;
                //Add object
                case 0x02:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint16,quint32> items;
                    quint16 inventorySize,id;
                    quint32 quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    add_to_inventory(items);
                }
                break;
                //Remove object
                case 0x03:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint16,quint32> items;
                    quint16 inventorySize,id;
                    quint32 quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    remove_to_inventory(items);
                }
                break;
                //the other player have put object
                case 0x04:
                {
                    if(!isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("not in trade trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 type;
                    in >> type;
                    switch(type)
                    {
                        //cash
                        case 0x01:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint64)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),"wrong remaining size for trade add cash");
                                return;
                            }
                            quint64 cash;
                            in >> cash;
                            tradeAddTradeCash(cash);
                        }
                        break;
                        //item
                        case 0x02:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),"wrong remaining size for trade add item id");
                                return;
                            }
                            quint16 item;
                            in >> item;
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),"wrong remaining size for trade add item quantity");
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            tradeAddTradeObject(item,quantity);
                        }
                        break;
                        //monster
                        case 0x03:
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if(CommonSettingsServer::commonSettingsServer.useSP)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> monster.sp;
                            }
                            else
                                monster.sp=0;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.catched_with;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 gender;
                            in >> gender;
                            switch(gender)
                            {
                                case 0x01:
                                case 0x02:
                                case 0x03:
                                    monster.gender=(Gender)gender;
                                break;
                                default:
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster character_origin, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.character_origin;

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint32 sub_size;
                            in >> sub_size;
                            quint32 sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                monster.skills << skill;
                                sub_index++;
                            }
                            tradeAddTradeMonster(monster);
                        }
                        break;
                        default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),"wrong type for trade add");
                            return;
                        break;
                    }
                }
                break;
                //the other player have accepted
                case 0x05:
                {
                    if(!tradeRequestId.empty())
                    {
                        parseError(QStringLiteral("Internal error"),QStringLiteral("request is running, skip this trade exchange"));
                        return;
                    }
                    if(isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("already in trade trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString pseudo;
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if(pseudoSize>0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(pseudoSize)
                                          .arg(QString(data.mid(in.device()->pos()).toHex()))
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                        pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                        if(pseudo.isEmpty())
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(rawText.toHex()))
                                          .arg(rawText.size())
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                    }
                    quint8 skinId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinId;
                    isInTrade=true;
                    tradeAcceptedByOther(pseudo,skinId);
                }
                break;
                //the other player have canceled
                case 0x06:
                {
                    isInTrade=false;
                    tradeCanceledByOther();
                    if(!tradeRequestId.empty())
                    {
                        tradeCanceled();
                        return;
                    }
                }
                break;
                //the other player have finished
                case 0x07:
                {
                    if(!isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("not in trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    tradeFinishedByOther();
                }
                break;
                //the server have validated the transaction
                case 0x08:
                {
                    if(!isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("not in trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    isInTrade=false;
                    tradeValidatedByTheServer();
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        case 0xE0:
        {
            switch(subCodeType)
            {
                //Result of the turn
                case 0x06:
                {
                    QList<Skill::AttackReturn> attackReturn;
                    quint8 attackReturnListSize;
                    quint8 listSizeShort,tempuint;
                    int index,indexAttackReturn;
                    PublicPlayerMonster publicPlayerMonster;
                    quint8 genderInt;
                    int buffListSize;
                    quint8 monsterPlace;

                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> attackReturnListSize;
                    indexAttackReturn=0;
                    while(indexAttackReturn<attackReturnListSize)
                    {
                        Skill::AttackReturn tempAttackReturn;
                        tempAttackReturn.success=false;
                        tempAttackReturn.attack=0;
                        tempAttackReturn.monsterPlace=0;
                        tempAttackReturn.publicPlayerMonster.catched_with=0;
                        tempAttackReturn.publicPlayerMonster.gender=Gender_Unknown;
                        tempAttackReturn.publicPlayerMonster.hp=0;
                        tempAttackReturn.publicPlayerMonster.level=0;
                        tempAttackReturn.publicPlayerMonster.monster=0;
                        tempAttackReturn.on_current_monster=false;
                        tempAttackReturn.item=0;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> tempuint;
                        if(tempuint>1)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("code to bool with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        tempAttackReturn.doByTheCurrentMonster=(tempuint!=0);
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> tempuint;
                        switch(tempuint)
                        {
                            case Skill::AttackReturnCase_NormalAttack:
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> tempuint;
                                if(tempuint>1)
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("code to bool with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                tempAttackReturn.success=(tempuint!=0);
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> tempAttackReturn.attack;
                                //add buff
                                index=0;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> listSizeShort;
                                while(index<listSizeShort)
                                {
                                    Skill::BuffEffect buffEffect;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buffEffect.buff;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> tempuint;
                                    switch(tempuint)
                                    {
                                        case ApplyOn_AloneEnemy:
                                        case ApplyOn_AllEnemy:
                                        case ApplyOn_Themself:
                                        case ApplyOn_AllAlly:
                                        case ApplyOn_Nobody:
                                        break;
                                        default:
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    buffEffect.on=(ApplyOn)tempuint;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buffEffect.level;
                                    tempAttackReturn.addBuffEffectMonster << buffEffect;
                                    index++;
                                }
                                //remove buff
                                index=0;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> listSizeShort;
                                while(index<listSizeShort)
                                {
                                    Skill::BuffEffect buffEffect;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buffEffect.buff;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> tempuint;
                                    switch(tempuint)
                                    {
                                        case ApplyOn_AloneEnemy:
                                        case ApplyOn_AllEnemy:
                                        case ApplyOn_Themself:
                                        case ApplyOn_AllAlly:
                                        case ApplyOn_Nobody:
                                        break;
                                        default:
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    buffEffect.on=(ApplyOn)tempuint;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buffEffect.level;
                                    tempAttackReturn.removeBuffEffectMonster << buffEffect;
                                    index++;
                                }
                                //life effect
                                index=0;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> listSizeShort;
                                while(index<listSizeShort)
                                {
                                    Skill::LifeEffectReturn lifeEffectReturn;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> lifeEffectReturn.quantity;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> tempuint;
                                    switch(tempuint)
                                    {
                                        case ApplyOn_AloneEnemy:
                                        case ApplyOn_AllEnemy:
                                        case ApplyOn_Themself:
                                        case ApplyOn_AllAlly:
                                        case ApplyOn_Nobody:
                                        break;
                                        default:
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    lifeEffectReturn.on=(ApplyOn)tempuint;
                                    tempAttackReturn.lifeEffectMonster << lifeEffectReturn;
                                    index++;
                                }
                                //buff effect
                                index=0;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> listSizeShort;
                                while(index<listSizeShort)
                                {
                                    Skill::LifeEffectReturn lifeEffectReturn;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> lifeEffectReturn.quantity;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> tempuint;
                                    switch(tempuint)
                                    {
                                        case ApplyOn_AloneEnemy:
                                        case ApplyOn_AllEnemy:
                                        case ApplyOn_Themself:
                                        case ApplyOn_AllAlly:
                                        case ApplyOn_Nobody:
                                        break;
                                        default:
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    lifeEffectReturn.on=(ApplyOn)tempuint;
                                    tempAttackReturn.buffLifeEffectMonster << lifeEffectReturn;
                                    index++;
                                }
                            }
                            break;
                            case Skill::AttackReturnCase_MonsterChange:
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> monsterPlace;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> publicPlayerMonster.monster;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> publicPlayerMonster.level;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> publicPlayerMonster.hp;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> publicPlayerMonster.catched_with;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> genderInt;
                                switch(genderInt)
                                {
                                    case 0x01:
                                    case 0x02:
                                    case 0x03:
                                        publicPlayerMonster.gender=(Gender)genderInt;
                                    break;
                                    default:
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(genderInt));
                                        return;
                                    break;
                                }
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> buffListSize;
                                index=0;
                                while(index<buffListSize)
                                {
                                    PlayerBuff buff;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buff.buff;
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                    {
                                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                        return;
                                    }
                                    in >> buff.level;
                                    publicPlayerMonster.buffs << buff;
                                    index++;
                                }
                                tempAttackReturn.publicPlayerMonster=publicPlayerMonster;
                            }
                            break;
                            case Skill::AttackReturnCase_ItemUsage:
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> tempuint;
                                tempAttackReturn.on_current_monster=(tempuint!=0x00);
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                    return;
                                }
                                in >> tempAttackReturn.item;
                            }
                            break;
                            default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Skill::AttackReturnCase wrong with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        attackReturn << tempAttackReturn;
                        indexAttackReturn++;
                    }

                    sendBattleReturn(attackReturn);
                }
                break;
                //The other player have declined you battle request
                case 0x07:
                    battleCanceledByOther();
                break;
                //The other player have accepted you battle request
                case 0x08:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString pseudo;
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if(pseudoSize>0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)pseudoSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(pseudoSize)
                                          .arg(QString(data.mid(in.device()->pos()).toHex()))
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                        pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                        if(pseudo.isEmpty())
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(rawText.toHex()))
                                          .arg(rawText.size())
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                    }
                    quint8 skinId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinId;
                    PublicPlayerMonster publicPlayerMonster;
                    QList<quint8> stat;
                    quint8 genderInt;
                    int buffListSize;
                    quint8 monsterPlace;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 statListSize;
                    in >> statListSize;
                    int index=0;
                    while(index<statListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 statEntry;
                        in >> statEntry;
                        stat << statEntry;
                        index++;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> monsterPlace;
                    if(monsterPlace<=0 || monsterPlace>stat.size())
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("monster place wrong range with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.monster;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.level;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.hp;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.catched_with;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> genderInt;
                    switch(genderInt)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                            publicPlayerMonster.gender=(Gender)genderInt;
                        break;
                        default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("gender code wrong: %2, line: %1").arg(__LINE__).arg(genderInt));
                            return;
                        break;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> buffListSize;
                    index=0;
                    while(index<buffListSize)
                    {
                        PlayerBuff buff;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> buff.buff;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> buff.level;
                        publicPlayerMonster.buffs << buff;
                        index++;
                    }
                    battleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
                }
                break;
                //random seeds as input
                case 0x09:
                {
                    if(data.size()<128)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("have too less data for random seed with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    random_seeds(data);
                    return;//quit here because all data is always used
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        case 0xF0:
        switch(subCodeType)
        {
            //Result of the turn
            case 0x01:
            {
                quint8 returnCode;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                    return;
                }
                in >> returnCode;
                switch(returnCode)
                {
                    case 0x01:
                        captureCityYourAreNotLeader();
                    break;
                    case 0x02:
                    {
                        QString city;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                            return;
                        in >> city;
                        captureCityYourLeaderHaveStartInOtherCity(city);
                    }
                    break;
                    case 0x03:
                        captureCityPreviousNotFinished();
                    break;
                    case 0x04:
                    {
                        quint16 player_count,clan_count;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_count;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> clan_count;
                        if((in.device()->size()-in.device()->pos())==0)
                            captureCityStartBattle(player_count,clan_count);
                        else if((in.device()->size()-in.device()->pos())==(int)(sizeof(quint32)))
                        {
                            quint32 fightId;
                            in >> fightId;
                            captureCityStartBotFight(player_count,clan_count,fightId);
                        }
                    }
                    break;
                    case 0x05:
                    {
                        quint16 player_count,clan_count;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_count;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> clan_count;
                        captureCityDelayedStart(player_count,clan_count);
                    }
                    break;
                    case 0x06:
                        captureCityWin();
                    break;
                    default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, returnCode: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(returnCode).arg(__LINE__));
                    return;
                }
            }
            break;
            default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
            return;
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident main code: %1").arg(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseFullMessage(%1,%2,%3 %4) line %5")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      .arg(__LINE__)
                      );
        return;
    }
}
