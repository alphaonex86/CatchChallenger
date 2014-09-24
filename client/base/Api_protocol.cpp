#include "Api_protocol.h"

using namespace CatchChallenger;

const unsigned char protocolHeaderToMatch[] = PROTOCOL_HEADER;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/GeneralVariable.h"
#include "../../general/base/CommonDatapack.h"
#include "../../general/base/CommonSettings.h"
#include "../../general/base/FacilityLib.h"

#include <QCoreApplication>

#ifdef BENCHMARKMUTIPLECLIENT
char Api_protocol::hurgeBufferForBenchmark[4096];
bool Api_protocol::precomputeDone=false;
char Api_protocol::hurgeBufferMove[4];

#include <iostream>
#include <fstream>
#include <unistd.h>
#endif

//need host + port here to have datapack base

QSet<QString> Api_protocol::extensionAllowed;

Api_protocol* Api_protocol::client=NULL;
bool Api_protocol::internalVersionDisplayed=false;

Api_protocol::Api_protocol(ConnectedSocket *socket,bool tolerantMode) :
    ProtocolParsingInputOutput(socket,PacketModeTransmission_Client),
    tolerantMode(tolerantMode)
{
    #ifdef BENCHMARKMUTIPLECLIENT
    if(!Api_protocol::precomputeDone)
    {
        Api_protocol::precomputeDone=true;
        hurgeBufferMove[0]=0x40;
    }
    #endif
    if(extensionAllowed.isEmpty())
        extensionAllowed=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";").toSet();

    connect(socket,&ConnectedSocket::destroyed,this,&Api_protocol::socketDestroyed,Qt::DirectConnection);
    //connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::DirectConnection);-> why direct?
    connect(socket,&ConnectedSocket::readyRead,this,&Api_protocol::parseIncommingData,Qt::QueuedConnection);//put queued to don't have circular loop Client -> Server -> Client

    resetAll();

    if(!Api_protocol::internalVersionDisplayed)
    {
        Api_protocol::internalVersionDisplayed=true;
        #if defined(Q_CC_GNU)
            qDebug() << QStringLiteral("GCC %1.%2.%3 build: ").arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__)+__DATE__+" "+__TIME__;
        #else
            #if defined(__DATE__) && defined(__TIME__)
                qDebug() << QStringLiteral("Unknown compiler: ")+__DATE__+" "+__TIME__;
            #else
                qDebug() << QStringLiteral("Unknown compiler");
            #endif
        #endif
        qDebug() << QStringLiteral("Qt version: %1 (%2)").arg(qVersion()).arg(QT_VERSION);
    }
}

Api_protocol::~Api_protocol()
{
}

void Api_protocol::disconnectClient()
{
    if(socket!=NULL)
        socket->disconnect();
    is_logged=false;
    character_selected=false;
}

void Api_protocol::socketDestroyed()
{
    socket=NULL;
}

QMap<quint8,QTime> Api_protocol::getQuerySendTimeList() const
{
    return querySendTime;
}

void Api_protocol::parseIncommingData()
{
    ProtocolParsingInputOutput::parseIncommingData();
}

void Api_protocol::errorParsingLayer(const QString &error)
{
    emit newError(tr("Internal error"),error);
}

void Api_protocol::messageParsingLayer(const QString &message) const
{
    qDebug() << message;
}

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const char *data,const int &size)
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
    in.setVersion(QDataStream::Qt_4_4);
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
                    if(CommonSettings::commonSettings.forcedSpeed==0)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> public_informations.speed;
                    }
                    else
                        public_informations.speed=CommonSettings::commonSettings.forcedSpeed;

                    if(!CommonSettings::commonSettings.dontSendPseudo)
                    {
                        //the pseudo
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 pseudoSize;
                        in >> pseudoSize;
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
            quint8 textSize;
            in >> textSize;
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
            const QString &text=QString::fromUtf8(rawText.data(),rawText.size());
            in.device()->seek(in.device()->pos()+rawText.size());
            if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
                new_system_text(chat_type,text);
            else
            {
                quint8 pseudoSize;
                in >> pseudoSize;
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
                QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
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
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseMessage(%1,%2 %3)")
                      .arg(mainCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

void Api_protocol::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const char *data,const int &size)
{
    parseFullMessage(mainCodeType,subCodeType,QByteArray(data,size));
}

void Api_protocol::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
        return;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0xC2:
        {
            switch(subCodeType)
            {
                //file as input
                case 0x0003://raw
                case 0x0004://compressed
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 fileListSize;
                    in >> fileListSize;
                    int index=0;
                    while(index<fileListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 fileNameSize;
                        in >> fileNameSize;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)fileNameSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                        in.device()->seek(in.device()->pos()+rawText.size());
                        QString fileName=QString::fromUtf8(rawText.data(),rawText.size());
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
                        if(subCodeType==0x0003)
                            DebugClass::debugConsole(QStringLiteral("Raw file to create: %1").arg(fileName));
                        else
                            DebugClass::debugConsole(QStringLiteral("Compressed file to create: %1").arg(fileName));
                        newFile(fileName,dataFile);
                        index++;
                    }
                    return;//no remaining data, because all remaing is used as file data
                }
                break;
                //kicked/ban and reason
                case 0x0008:
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
                case 0x0009:
                    clanDissolved();
                break;
                //clan info
                case 0x000A:
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
                case 0x000B:
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
                case 0x000C:
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
                    datapackSize(datapckFileNumber,datapckFileSize);
                }
                break;
                //Update file http
                case 0x000D:
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
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 fileNameSize;
                        in >> fileNameSize;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)fileNameSize)
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                        in.device()->seek(in.device()->pos()+rawText.size());
                        QString fileName=QString::fromUtf8(rawText.data(),rawText.size());
                        if(!extensionAllowed.contains(QFileInfo(fileName).suffix()))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("extension not allowed: %4 with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__).arg(QFileInfo(fileName).suffix()));
                            if(!tolerantMode)
                                return;
                        }
                        newHttpFile(baseHttp+fileName,fileName);

                        index++;
                    }
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
                case 0x0001:
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
                case 0x0002:
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
                case 0x0003:
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
                case 0x0004:
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
                            if((data.size()-in.device()->pos())<((int)sizeof(quint16)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),"wrong remaining size for trade add item quantity");
                                return;
                            }
                            quint16 quantity;
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                case 0x0005:
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
                    quint8 pseudoSize;
                    in >> pseudoSize;
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
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
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
                case 0x0006:
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
                case 0x0007:
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
                case 0x0008:
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
                case 0x0006:
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                case 0x0007:
                    battleCanceledByOther();
                break;
                //The other player have accepted you battle request
                case 0x0008:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
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
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
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
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
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
                case 0x0009:
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
            case 0x0001:
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
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseFullMessage(%1,%2,%3 %4)")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

//have query with reply
void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    parseQuery(mainCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("have not query of this type, mainCodeType: %1, queryNumber: %2").arg(mainCodeType).arg(queryNumber));
}

void Api_protocol::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    parseFullQuery(mainCodeType,subCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("is not logged with main ident: %1").arg(mainCodeType));
        return;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x79:
        {
            switch(subCodeType)
            {
                //Teleport the player
                case 0x0001:
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
                    quint8 x,y;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> x;
                    in >> y;
                    quint8 directionInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> directionInt;
                    if(directionInt<1 || directionInt>4)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    teleportList << queryNumber;
                    teleportTo(mapId,x,y,direction);
                }
                break;
                //Event change
                case 0x0002:
                {
                    quint8 event,event_value;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> event;
                    in >> event_value;
                    newEvent(event,event_value);
                    postReplyData(queryNumber,NULL,0);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        case 0x80:
        {
            switch(subCodeType)
            {
                //Another player request a trade
                case 0x0001:
                {
                    if(!tradeRequestId.isEmpty() || isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
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
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
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
                    quint8 skinInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinInt;
                    tradeRequestId << queryNumber;
                    tradeRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        case 0x90:
        {
            switch(subCodeType)
            {
                //Another player request a trade
                case 0x0001:
                {
                    if(!tradeRequestId.isEmpty() || isInTrade)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("Already on battle"));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
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
                    QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
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
                    quint8 skinInt;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinInt;
                    battleRequestId << queryNumber;
                    battleRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident main code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("remaining data: parseFullQuery(%1,%2,%3 %4)")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

//send reply
void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    parseReplyData(mainCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x03:
        {
            //Protocol initialization
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                return;
            }
            quint8 returnCode;
            in >> returnCode;
            if(returnCode>=0x04 && returnCode<=0x06)
            {
                switch(returnCode)
                {
                    case 0x04:
                        ProtocolParsing::compressionType=ProtocolParsing::CompressionType_None;
                    break;
                    case 0x05:
                        ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Zlib;
                    break;
                    case 0x06:
                        ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Xz;
                    break;
                    default:
                        newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(queryNumber));
                    return;
                }
                if(data.size()!=(sizeof(quint8)+CATCHCHALLENGER_TOKENSIZE))
                {
                    newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("compression type wrong with main ident: %1 and queryNumber: %2, type: query_type_protocol").arg(queryNumber));
                    return;
                }
                token=data.right(CATCHCHALLENGER_TOKENSIZE);
                have_receive_protocol=true;
                protocol_is_good();
                return;
            }
            else
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Protocol not supported");
                else if(returnCode==0x03)
                    string=tr("Server full");
                else
                    string=tr("Unknown error %1").arg(returnCode);
                newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("the server have returned: %1").arg(string));
                disconnected(QStringLiteral("the server have returned: %1").arg(string));
                return;
            }
        }
        break;
        //Get first data and send the login
        case 0x04:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                return;
            }
            quint8 returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Bad login");
                else if(returnCode==0x03)
                    string=tr("Wrong login/pass");
                else if(returnCode==0x04)
                    string=tr("Server internal error");
                else if(returnCode==0x05)
                    string=tr("Can't create character and don't have character");
                else if(returnCode==0x06)
                    string=tr("Login already in progress");
                else if(returnCode==0x07)
                {
                    tryCreate();
                    return;
                }
                else
                    string=tr("Unknown error %1").arg(returnCode);
                DebugClass::debugConsole("is not logged, reason: "+string);
                notLogged(string);
                return;
            }
            else
            {
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max player, line: %1").arg(__LINE__));
                    return;
                }
                in >> max_players;
                setMaxPlayers(max_players);

                quint32 captureRemainingTime;
                quint8 captureFrequencyType;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture remainingTime, line: %1").arg(__LINE__));
                    return;
                }
                in >> captureRemainingTime;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the city capture type, line: %1").arg(__LINE__));
                    return;
                }
                in >> captureFrequencyType;
                switch(captureFrequencyType)
                {
                    case 0x01:
                    case 0x02:
                    break;
                    default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong captureFrequencyType, line: %1").arg(__LINE__));
                    return;
                }
                cityCapture(captureRemainingTime,captureFrequencyType);

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.waitBeforeConnectAfterKick;
                {
                    quint8 tempForceClientToSendAtBorder;
                    in >> tempForceClientToSendAtBorder;
                    if(tempForceClientToSendAtBorder!=0 && tempForceClientToSendAtBorder!=1)
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("forceClientToSendAtBorder have wrong value, line: %1").arg(__LINE__));
                        return;
                    }
                    CommonSettings::commonSettings.forceClientToSendAtMapChange=(tempForceClientToSendAtBorder==1);
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.forcedSpeed;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the forcedSpeed, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.useSP;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the tcpCork, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.tcpCork;
                {
                    socket->setTcpCork(CommonSettings::commonSettings.tcpCork);
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.autoLearn;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.dontSendPseudo;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.max_character;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the min_character, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.min_character;
                if(CommonSettings::commonSettings.max_character<CommonSettings::commonSettings.min_character)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("max_character<min_character, line: %1").arg(__LINE__));
                    return;
                }
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.max_pseudo_size;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_pseudo_size, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.character_delete_time;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_xp, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.rates_xp;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.rates_gold;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.rates_xp_pow;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(float))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the rates_gold, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.rates_drop;

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.maxPlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.maxWarehousePlayerMonsters;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.maxPlayerItems;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.maxWarehousePlayerItems;

                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_all, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.chat_allow_all;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_local, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.chat_allow_local;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_private, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.chat_allow_private;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the chat_allow_clan, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.chat_allow_clan;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the factoryPriceChange, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.factoryPriceChange;
                if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the httpDatapackMirror, line: %1").arg(__LINE__));
                    return;
                }
                in >> CommonSettings::commonSettings.httpDatapackMirror;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<28)
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the datapack checksum, line: %1").arg(__LINE__));
                    return;
                }
                CommonSettings::commonSettings.datapackHash=data.mid(in.device()->pos(),28);
                in.device()->seek(in.device()->pos()+CommonSettings::commonSettings.datapackHash.size());
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the number of map, line: %1").arg(__LINE__));
                    return;
                }
                /// \todo, change on login, why transmit it?
                in >> number_of_map;
                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the characterListSize, line: %1").arg(__LINE__));
                    return;
                }
                quint8 characterListSize;
                in >> characterListSize;

                QList<CharacterEntry> characterEntryList;
                int index=0;
                while(index<characterListSize)
                {
                    CharacterEntry characterEntry;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the character_id, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.character_id;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.pseudo;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.skinId;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the delete_time_left, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.delete_time_left;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the played_time, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.played_time;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the last_connect, line: %1").arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.last_connect;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> characterEntry.mapId;
                    characterEntryList << characterEntry;
                    index++;
                }
                is_logged=true;
                logged(characterEntryList);
            }
        }
        break;
        //Account creation
        case 0x05:
        {
            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
            {
                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1 and queryNumber: %2, line: %3").arg(mainCodeType).arg(queryNumber).arg(__LINE__));
                return;
            }
            quint8 returnCode;
            in >> returnCode;
            if(returnCode!=0x01)
            {
                QString string;
                if(returnCode==0x02)
                    string=tr("Login already used");
                else if(returnCode==0x03)
                    string=tr("Not created");
                else
                    string=tr("Unknown error %1").arg(returnCode);
                DebugClass::debugConsole("is not logged, reason: "+string);
                notLogged(string);
                return;
            }
            else
            {
                QByteArray outputData;
                //reemit the login try
                outputData+=loginHash;
                QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
                hashAndToken.addData(passHash+token);
                outputData+=hashAndToken.result();
                const quint8 &query_number=Api_protocol::queryNumber();
                packOutcommingQuery(0x04,query_number,outputData.constData(),outputData.size());
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown sort ident reply code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseReplyData(%1,%2), line: %3, data: %4 %5")
                   .arg(mainCodeType).arg(queryNumber)
                   .arg(__LINE__)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void Api_protocol::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const char *data,const int &size)
{
    parseFullReplyData(mainCodeType,subCodeType,queryNumber,QByteArray(data,size));
}

void Api_protocol::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(querySendTime.contains(queryNumber))
    {
        lastReplyTime(querySendTime.value(queryNumber).elapsed());
        querySendTime.remove(queryNumber);
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Get the character id return
                case 0x0003:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 characterId;
                    in >> characterId;
                    newCharacterId(returnCode,characterId);
                }
                break;
                //Get the character id return
                case 0x0004:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    //just don't emited and used
                }
                break;
                //get the character selection return
                case 0x0005:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode!=0x01)
                    {
                        QString string;
                        if(returnCode==0x02)
                            string=tr("Character not found");
                        else if(returnCode==0x03)
                            string=tr("Already logged");
                        else if(returnCode==0x04)
                            string=tr("Server internal problem");
                        else
                            string=tr("Unknown error: %1").arg(returnCode);
                        DebugClass::debugConsole("Selected character not found, reason: "+string);
                        notLogged(string);
                        return;
                    }
                    else
                    {
                        if(max_players<=255)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 simplifiedId;
                            in >> simplifiedId;
                            player_informations.public_informations.simplifiedId=simplifiedId;
                        }
                        else
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> player_informations.public_informations.simplifiedId;
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || !checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_informations.public_informations.pseudo;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 tempAllowSize;
                        in >> tempAllowSize;
                        {
                            int index=0;
                            while(index<tempAllowSize)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    newError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                                    return;
                                }
                                quint8 tempAllow;
                                in >> tempAllow;
                                player_informations.allow << static_cast<ActionAllow>(tempAllow);
                                index++;
                            }
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.clan;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player clan leader, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 tempClanLeader;
                        in >> tempClanLeader;
                        if(tempClanLeader==0x01)
                            player_informations.clan_leader=true;
                        else
                            player_informations.clan_leader=false;
                        {
                            QList<QPair<quint8,quint8> > events;
                            quint8 tempListSize;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the max_character, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> tempListSize;
                            quint8 event,value;
                            int index=0;
                            while(index<tempListSize)
                            {

                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event id, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> event;
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the event value, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> value;
                                index++;
                                events << QPair<quint8,quint8>(event,value);
                            }
                            setEvents(events);
                        }
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.cash;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.warehouse_cash;

                        //item on map
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 itemOnMapSize;
                            in >> itemOnMapSize;
                            quint8 index=0;
                            while(index<itemOnMapSize)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player item on map, line: %1").arg(__LINE__));
                                    return;
                                }
                                quint8 itemOnMap;
                                in >> itemOnMap;
                                player_informations.itemOnMap << itemOnMap;
                                index++;
                            }
                        }

                        //recipes
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the recipe list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint16 recipe_list_size;
                        in >> recipe_list_size;
                        quint16 recipeId;
                        quint32 index=0;
                        while(index<recipe_list_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the player local recipe, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> recipeId;
                            player_informations.recipes << recipeId;
                            index++;
                        }

                        //monsters
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 gender;
                        quint8 monster_list_size;
                        in >> monster_list_size;
                        index=0;
                        quint32 sub_index;
                        while(index<monster_list_size)
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
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

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 sub_size8;
                            in >> sub_size8;
                            sub_index=0;
                            while(sub_index<sub_size8)
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

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint16 sub_size16;
                            in >> sub_size16;
                            sub_index=0;
                            while(sub_index<sub_size16)
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
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
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster_list_size;
                        index=0;
                        while(index<monster_list_size)
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
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

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint8 sub_size8;
                            in >> sub_size8;
                            sub_index=0;
                            while(sub_index<sub_size8)
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

                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint16 sub_size16;
                            in >> sub_size16;
                            sub_index=0;
                            while(sub_index<sub_size16)
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster skill level, line: %1").arg(__LINE__));
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
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerReputation playerReputation;
                        quint8 type;
                        index=0;
                        quint8 sub_size8;
                        in >> sub_size8;
                        while(index<sub_size8)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong reputation code: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> type;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.level;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.point;
                            player_informations.reputation[type]=playerReputation;
                            index++;
                        }
                        //quest
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerQuest playerQuest;
                        quint16 playerQuestId;
                        index=0;
                        quint16 sub_size16;
                        in >> sub_size16;
                        while(index<sub_size16)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint16))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> playerQuestId;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.step;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.finish_one_time;
                            if(playerQuest.step<=0 && !playerQuest.finish_one_time)
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("can't be to step 0 if have never finish the quest, line: %1").arg(__LINE__));
                                return;
                            }
                            player_informations.quests[playerQuestId]=playerQuest;
                            index++;
                        }
                        //bot_already_beaten
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint16 bot_already_beaten;
                        index=0;
                        in >> sub_size16;
                        while(index<sub_size16)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
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
                }
                break;
                //Clan action
                case 0x000D:
                {
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                clanActionSuccess(0);
                            else
                            {
                                quint32 clanId;
                                in >> clanId;
                                clanActionSuccess(clanId);
                            }
                        break;
                        case 0x02:
                            clanActionFailed();
                        break;
                        default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code at clan action: %1, line: %2").arg(returnCode).arg(__LINE__));
                        break;
                    }
                }
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
                    return;
                break;
            }
        }
        break;
        case 0x10:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Use seed into dirt
                case 0x0006:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                        seed_planted(true);
                    else if(returnCode==0x02)
                        seed_planted(false);
                    else
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("bad return code to use seed: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                //Collect mature plant
                case 0x0007:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                        case 0x04:
                            plant_collected((Plant_collect)returnCode);
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Usage of recipe
                case 0x0008:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                            recipeUsed((RecipeUsage)returnCode);
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Use object
                case 0x0009:
                {
                    quint16 item=lastObjectUsed.first();
                    lastObjectUsed.removeFirst();
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(CommonDatapack::commonDatapack.items.trap.contains(item))
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        quint32 newMonsterId;
                        in >> newMonsterId;
                        monsterCatch(newMonsterId);
                    }
                    else
                    {
                        switch(returnCode)
                        {
                            case 0x01:
                            case 0x02:
                            case 0x03:
                                objectUsed((ObjectUsage)returnCode);
                            break;
                            default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                    }
                }
                break;
                //Get shop list
                case 0x000A:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint16 shopListSize;
                    in >> shopListSize;
                    quint32 index=0;
                    QList<ItemToSellOrBuy> items;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        items << item;
                        index++;
                    }
                    haveShopList(items);
                }
                break;
                //Buy object
                case 0x000B:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x04:
                            haveBuyObject((BuyStat)returnCode,0);
                        break;
                        case 0x03:
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Sell object
                case 0x000C:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x04:
                            haveSellObject((SoldStat)returnCode,0);
                        break;
                        case 0x03:
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x000D:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 remainingProductionTime;
                    in >> remainingProductionTime;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint16 shopListSize;
                    quint32 index;
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> resources;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        resources << item;
                        index++;
                    }
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> products;
                    while(index<shopListSize)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*2+sizeof(quint16)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        products << item;
                        index++;
                    }
                    haveFactoryList(remainingProductionTime,resources,products);
                }
                break;
                case 0x000E:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x03:
                        case 0x04:
                            haveBuyFactoryObject((BuyStat)returnCode,0);
                        break;
                        case 0x02:
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x000F:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x03:
                        case 0x04:
                            haveSellFactoryObject((SoldStat)returnCode,0);
                        break;
                        case 0x02:
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            haveSellFactoryObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                case 0x0010:
                {
                    quint32 listSize,index;
                    QList<MarketObject> marketObjectList;
                    QList<MarketMonster> marketMonsterList;
                    QList<MarketObject> marketOwnObjectList;
                    QList<MarketMonster> marketOwnMonsterList;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        marketObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        marketMonsterList << marketMonster;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        marketOwnObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        marketOwnMonsterList << marketMonster;
                        index++;
                    }
                    marketList(cash,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
                }
                break;
                case 0x0011:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            if((in.device()->size()-in.device()->pos())==0)
                                marketBuy(true);
                        break;
                        case 0x02:
                        case 0x03:
                            marketBuy(false);
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
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
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.sp;
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                        quint32 sub_size,sub_index;
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                        marketBuyMonster(monster);
                    }
                }
                break;
                case 0x0012:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            marketPut(true);
                        break;
                        case 0x02:
                            marketPut(false);
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                break;
                case 0x0013:
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    marketGetCash(cash);
                break;
                case 0x0014:
                {
                    if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        break;
                        case 0x02:
                            marketWithdrawCanceled();
                        break;
                        default:
                        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01)
                    {
                        if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        quint8 returnType;
                        in >> returnType;
                        switch(returnType)
                        {
                            case 0x01:
                            case 0x02:
                            break;
                            default:
                            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong return code, line: %1").arg(__LINE__));
                            return;
                        }
                        if(returnType==0x01)
                        {
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 objectId;
                            in >> objectId;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            marketWithdrawObject(objectId,quantity);
                        }
                        else
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                            quint32 sub_size,sub_index;
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                                if(in.device()->pos()<0 || !in.device()->isOpen() || (in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
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
                            marketWithdrawMonster(monster);
                        }
                    }
                }
                break;
                default:
                    parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
                    return;
                break;
            }
        }
        break;
        default:
            parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("unknown ident reply code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(QStringLiteral("Procotol wrong or corrupted"),QStringLiteral("error: remaining data: parseFullReplyData(%1,%2,%3), line: %4, data: %5 %6")
                   .arg(mainCodeType).arg(subCodeType).arg(queryNumber)
                   .arg(__LINE__)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

void Api_protocol::parseError(const QString &userMessage,const QString &errorString)
{
    if(tolerantMode)
        DebugClass::debugConsole(QStringLiteral("packet ignored due to: %1").arg(errorString));
    else
        newError(userMessage,errorString);
}

Player_private_and_public_informations Api_protocol::get_player_informations()
{
    return player_informations;
}

QString Api_protocol::getPseudo()
{
    return player_informations.public_informations.pseudo;
}

quint16 Api_protocol::getId()
{
    return player_informations.public_informations.simplifiedId;
}

quint8 Api_protocol::queryNumber()
{
    if(lastQueryNumber>=254)
        lastQueryNumber=1;
    querySendTime[lastQueryNumber].start();
    return lastQueryNumber++;
}

bool Api_protocol::sendProtocol()
{
    if(have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have already send the protocol"));
        return false;
    }
    have_send_protocol=true;
    QByteArray outputData(reinterpret_cast<char *>(const_cast<unsigned char *>(protocolHeaderToMatch)),sizeof(protocolHeaderToMatch));
    packOutcommingQuery(0x03,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::tryLogin(const QString &login, const QString &pass)
{
    if(!have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Is already logged"));
        return false;
    }
    if(token.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Token is empty"));
        return false;
    }
    QByteArray outputData;
    {
        QCryptographicHash hashLogin(QCryptographicHash::Sha224);
        hashLogin.addData((login+/*salt*/"RtR3bm9Z1DFMfAC3").toLatin1());
        loginHash=hashLogin.result();
        outputData+=loginHash;
    }
    {
        QCryptographicHash hashPass(QCryptographicHash::Sha224);
        hashPass.addData((pass+/*salt*/"AwjDvPIzfJPTTgHs").toLatin1());
        passHash=hashPass.result();

        QCryptographicHash hashAndToken(QCryptographicHash::Sha224);
        hashAndToken.addData(passHash);
        hashAndToken.addData(token);
        outputData+=hashAndToken.result();
    }
    const quint8 &query_number=queryNumber();
    packOutcommingQuery(0x04,query_number,outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::tryCreate()
{
    if(!have_send_protocol)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("Is already logged"));
        return false;
    }
    QByteArray outputData;
    outputData+=loginHash;
    outputData+=passHash;
    const quint8 &query_number=queryNumber();
    packOutcommingQuery(0x05,query_number,outputData.constData(),outputData.size());
    return true;
}

void Api_protocol::send_player_move(const quint8 &moved_unit,const Direction &direction)
{
    #ifdef BENCHMARKMUTIPLECLIENT
    hurgeBufferMove[1]=moved_unit;
    hurgeBufferMove[2]=direction;
    const int &infd=socket->sslSocket->socketDescriptor();
    if(infd!=-1)
        ::write(infd,hurgeBufferMove,3);
    else
        internalSendRawSmallPacket(hurgeBufferMove,3);
    return;
    #endif
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    quint8 directionInt=static_cast<quint8>(direction);
    if(directionInt<1 || directionInt>8)
    {
        DebugClass::debugConsole(QStringLiteral("direction given wrong: %1").arg(directionInt));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << moved_unit;
    out << directionInt;
    is_logged=character_selected=packOutcommingData(0x40,outputData.constData(),outputData.size());
}

void Api_protocol::send_player_direction(const Direction &the_direction)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    newDirection(the_direction);
}

void Api_protocol::sendChatText(const Chat_type &chatType, const QString &text)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_aliance && chatType!=Chat_type_system && chatType!=Chat_type_system_important)
    {
        DebugClass::debugConsole("chatType wrong: "+QString::number(chatType));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)chatType;
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
    is_logged=character_selected=packOutcommingData(0x43,outputData.constData(),outputData.size());
}

void Api_protocol::sendPM(const QString &text,const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(this->player_informations.public_informations.pseudo==pseudo)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)Chat_type_pm;
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
    {
        const QByteArray &tempText=pseudo.toUtf8();
        if(tempText.size()>255)
        {
            DebugClass::debugConsole(QStringLiteral("text in Utf8 too big, line: %1").arg(__LINE__));
            return;
        }
        out << (quint8)tempText.size();
        outputData+=tempText;
        out.device()->seek(out.device()->pos()+tempText.size());
    }
    is_logged=character_selected=packOutcommingData(0x43,outputData.constData(),outputData.size());
}

void Api_protocol::teleportDone()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=postReplyData(teleportList.first(),NULL,0);
    teleportList.removeFirst();
}

bool Api_protocol::addCharacter(const quint8 &profileIndex, const QString &pseudo, const quint8 &skinId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    if(skinId>=CommonDatapack::commonDatapack.skins.size())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("skin provided: %1 is not into skin listed").arg(skinId));
        return false;
    }
    const Profile &profile=CommonDatapack::commonDatapack.profileList.at(profileIndex);
    if(!profile.forcedskin.isEmpty() && !profile.forcedskin.contains(skinId))
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("skin provided: %1 is not into profile %2 forced skin list").arg(skinId).arg(profileIndex));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)profileIndex;
    out << pseudo;
    out << (quint8)skinId;
    is_logged=packFullOutcommingQuery(0x02,0x0003,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::removeCharacter(const quint32 &characterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << characterId;
    is_logged=packFullOutcommingQuery(0x02,0x0004,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

bool Api_protocol::selectCharacter(const quint32 &characterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << characterId;
    is_logged=packFullOutcommingQuery(0x02,0x0005,queryNumber(),outputData.constData(),outputData.size());
    return true;
}

void Api_protocol::useSeed(const quint8 &plant_id)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    outputData[0]=plant_id;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0006,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveUp(const quint8 &number)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << number;
    is_logged=character_selected=packFullOutcommingData(0x60,0x0008,outputData.constData(),outputData.size());
}

void Api_protocol::confirmEvolution(const quint32 &monterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)monterId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x000A,outputData.constData(),outputData.size());
}

void Api_protocol::monsterMoveDown(const quint8 &number)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << number;
    is_logged=character_selected=packFullOutcommingData(0x60,0x0008,outputData.constData(),outputData.size());
}

//inventory
void Api_protocol::destroyObject(const quint16 &object, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << object;
    out << quantity;
    is_logged=character_selected=packFullOutcommingData(0x50,0x0002,outputData.constData(),outputData.size());
}

void Api_protocol::useObject(const quint16 &object)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << object;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0009,queryNumber(),outputData.constData(),outputData.size());
    lastObjectUsed << object;
}

void Api_protocol::useObjectOnMonster(const quint16 &object,const quint32 &monster)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << object;
    out << monster;
    is_logged=character_selected=packFullOutcommingData(0x60,0x000B,outputData.constData(),outputData.size());
}


void Api_protocol::wareHouseStore(const qint64 &cash, const QList<QPair<quint16,qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << cash;

    out << (quint16)items.size();
    int index=0;
    while(index<items.size())
    {
        out << (quint16)items.at(index).first;
        out << (qint32)items.at(index).second;
        index++;
    }

    out << (quint32)withdrawMonsters.size();
    index=0;
    while(index<withdrawMonsters.size())
    {
        out << (quint32)withdrawMonsters.at(index);
        index++;
    }
    out << (quint32)depositeMonsters.size();
    index=0;
    while(index<depositeMonsters.size())
    {
        out << (quint32)depositeMonsters.at(index);
        index++;
    }

    is_logged=character_selected=packFullOutcommingData(0x50,0x0006,outputData.constData(),outputData.size());
}

void Api_protocol::takeAnObjectOnMap()
{
    packFullOutcommingData(0x50,0x0007,NULL,0);
}

void Api_protocol::getShopList(const quint32 &shopId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)shopId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000A,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)shopId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000B,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)shopId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000C,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::getFactoryList(const quint16 &factoryId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)factoryId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyFactoryProduct(const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)factoryId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000E,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::sellFactoryResource(const quint16 &factoryId,const quint16 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)factoryId;
    out << (quint16)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x000F,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::tryEscape()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x60,0x0002,NULL,0);
}

void Api_protocol::heal()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x60,0x0006,NULL,0);
}

void Api_protocol::requestFight(const quint32 &fightId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)fightId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x0007,outputData.constData(),outputData.size());
}

void Api_protocol::changeOfMonsterInFight(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)monsterId;
    is_logged=character_selected=packFullOutcommingData(0x60,0x0009,outputData.constData(),outputData.size());
}

void Api_protocol::useSkill(const quint16 &skill)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)skill;
    is_logged=character_selected=packOutcommingData(0x61,outputData.constData(),outputData.size());
}

void Api_protocol::learnSkill(const quint32 &monsterId,const quint16 &skill)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)monsterId;
    out << (quint16)skill;
    is_logged=character_selected=packFullOutcommingData(0x60,0x0004,outputData.constData(),outputData.size());
}

void Api_protocol::startQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x0001,outputData.constData(),outputData.size());
}

void Api_protocol::finishQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x0002,outputData.constData(),outputData.size());
}

void Api_protocol::cancelQuest(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x0003,outputData.constData(),outputData.size());
}

void Api_protocol::nextQuestStep(const quint16 &questId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)questId;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x0004,outputData.constData(),outputData.size());
}

void Api_protocol::createClan(const QString &name)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << name;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::leaveClan()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::dissolveClan()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x03;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteClan(const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << pseudo;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::ejectClan(const QString &pseudo)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x05;
    out << pseudo;
    is_logged=character_selected=packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::inviteAccept(const bool &accept)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(accept)
        out << (quint8)0x01;
    else
        out << (quint8)0x02;
    is_logged=character_selected=packFullOutcommingData(0x42,0x0004,outputData.constData(),outputData.size());
}

void Api_protocol::waitingForCityCapture(const bool &cancel)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(!cancel)
        out << (quint8)0x00;
    else
        out << (quint8)0x01;
    is_logged=character_selected=packFullOutcommingData(0x6a,0x0005,outputData.constData(),outputData.size());
}

//market
void Api_protocol::getMarketList()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0010,queryNumber(),NULL,0);
}

void Api_protocol::buyMarketObject(const quint32 &marketObjectId, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << marketObjectId;
    out << quantity;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0011,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::buyMarketMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0011,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketObject(const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    out << price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0012,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::putMarketMonster(const quint32 &monsterId,const quint32 &price)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    out << price;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0012,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::recoverMarketCash()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0013,queryNumber(),NULL,0);
}

void Api_protocol::withdrawMarketObject(const quint32 &objectId,const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0014,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::withdrawMarketMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0014,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::collectMaturePlant()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0007,queryNumber(),NULL,0);
}

//crafting
void Api_protocol::useRecipe(const quint16 &recipeId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint16)recipeId;
    is_logged=character_selected=packFullOutcommingQuery(0x10,0x0008,queryNumber(),outputData.constData(),outputData.size());
}

void Api_protocol::addRecipe(const quint16 &recipeId)
{
    player_informations.recipes << recipeId;
}

void Api_protocol::battleRefused()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(battleRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no battle request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    is_logged=character_selected=postReplyData(battleRequestId.first(),outputData.constData(),outputData.size());
    battleRequestId.removeFirst();
}

void Api_protocol::battleAccepted()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(battleRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no battle request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    is_logged=character_selected=postReplyData(battleRequestId.first(),outputData.constData(),outputData.size());
    battleRequestId.removeFirst();
}

//trade
void Api_protocol::tradeRefused()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(tradeRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no trade request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    is_logged=character_selected=postReplyData(tradeRequestId.first(),outputData.constData(),outputData.size());
    tradeRequestId.removeFirst();
}

void Api_protocol::tradeAccepted()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(tradeRequestId.isEmpty())
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no trade request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    is_logged=character_selected=postReplyData(tradeRequestId.first(),outputData.constData(),outputData.size());
    tradeRequestId.removeFirst();
    isInTrade=true;
}

void Api_protocol::tradeCanceled()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("in not in trade"));
        return;
    }
    isInTrade=false;
    is_logged=character_selected=packFullOutcommingData(0x50,0x0005,NULL,0);
}

void Api_protocol::tradeFinish()
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("in not in trade"));
        return;
    }
    is_logged=character_selected=packFullOutcommingData(0x50,0x0004,NULL,0);
}

void Api_protocol::addTradeCash(const quint64 &cash)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(cash==0)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("can't send 0 for the cash"));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send cash"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << cash;
    is_logged=character_selected=packFullOutcommingData(0x50,0x0003,outputData.constData(),outputData.size());
}

void Api_protocol::addObject(const quint16 &item, const quint32 &quantity)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(quantity==0)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("can't send a quantity of 0"));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send object"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    is_logged=character_selected=packFullOutcommingData(0x50,0x0003,outputData.constData(),outputData.size());
}

void Api_protocol::addMonster(const quint32 &monsterId)
{
    if(!is_logged)
    {
        DebugClass::debugConsole(QStringLiteral("is not logged, line: %1").arg(__LINE__));
        return;
    }
    if(!character_selected)
    {
        DebugClass::debugConsole(QStringLiteral("character not selected, line: %1").arg(__LINE__));
        return;
    }
    if(!isInTrade)
    {
        newError(QStringLiteral("Internal problem"),QStringLiteral("no in trade to send monster"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x03;
    out << monsterId;
    is_logged=character_selected=packFullOutcommingData(0x50,0x0003,outputData.constData(),outputData.size());
}

//to reset all
void Api_protocol::resetAll()
{
    //status for the query
    token.clear();
    is_logged=false;
    character_selected=false;
    have_send_protocol=false;
    have_receive_protocol=false;
    max_players=65535;
    number_of_map=0;
    player_informations.allow.clear();
    player_informations.bot_already_beaten.clear();
    player_informations.cash=0;
    player_informations.clan=0;
    player_informations.clan_leader=false;
    player_informations.warehouse_cash=0;
    player_informations.warehouse_items.clear();
    player_informations.warehouse_playerMonster.clear();
    player_informations.public_informations.pseudo.clear();
    player_informations.public_informations.simplifiedId=0;
    player_informations.public_informations.skinId=0;
    player_informations.public_informations.speed=0;
    player_informations.public_informations.type=Player_type_normal;
    player_informations.repel_step=0;
    player_informations.recipes.clear();
    player_informations.playerMonster.clear();
    player_informations.items.clear();
    player_informations.reputation.clear();
    player_informations.quests.clear();
    player_informations.itemOnMap.clear();
    isInTrade=false;
    tradeRequestId.clear();
    isInBattle=false;
    battleRequestId.clear();
    mDatapack=QStringLiteral("%1/datapack/").arg(QCoreApplication::applicationDirPath());

    //to send trame
    lastQueryNumber=1;

    ProtocolParsingInputOutput::reset();
}

QString Api_protocol::datapackPath() const
{
    return mDatapack;
}

void Api_protocol::setDatapackPath(const QString &datapack_path)
{
    if(datapack_path.endsWith(QLatin1Literal("/")))
        mDatapack=datapack_path;
    else
        mDatapack=datapack_path+QLatin1Literal("/");
}

bool Api_protocol::getIsLogged() const
{
    return is_logged;
}

bool Api_protocol::getCaracterSelected() const
{
    return character_selected;
}
