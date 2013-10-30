#include "Api_protocol.h"

using namespace CatchChallenger;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

#include "GeneralStructures.h"
#include "GeneralVariable.h"
#include "CommonDatapack.h"
#include "CommonSettings.h"
#include "FacilityLib.h"

//need host + port here to have datapack base

QSet<QString> Api_protocol::extensionAllowed;

Api_protocol::Api_protocol(ConnectedSocket *socket,bool tolerantMode) :
    ProtocolParsingInput(socket,PacketModeTransmission_Client)
{
    output=new ProtocolParsingOutput(socket,PacketModeTransmission_Client);
    this->tolerantMode=tolerantMode;

    if(extensionAllowed.isEmpty())
    {
        QStringList extensionAllowedTemp=QString(CATCHCHALLENGER_EXTENSION_ALLOWED).split(";");
        extensionAllowed=extensionAllowedTemp.toSet();
    }

    connect(this,&Api_protocol::newInputQuery,output,&ProtocolParsingOutput::newInputQuery);
    connect(this,&Api_protocol::newFullInputQuery,output,&ProtocolParsingOutput::newFullInputQuery);
    connect(output,&ProtocolParsingOutput::newOutputQuery,this,&Api_protocol::newOutputQuery);
    connect(output,&ProtocolParsingOutput::newFullOutputQuery,this,&Api_protocol::newFullOutputQuery);
    connect(socket,&ConnectedSocket::destroyed,this,&Api_protocol::socketDestroyed,Qt::DirectConnection);

    resetAll();
}

Api_protocol::~Api_protocol()
{
    delete output;
    output=NULL;
}

void Api_protocol::socketDestroyed()
{
    delete output;
    output=NULL;
    socket=NULL;
}

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("is not logged with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
        return;
    }
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        //Insert player on map
        case 0xC0:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }

                quint16 playerSizeList;
                if(max_player<=255)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    Player_public_informations public_informations;
                    if(max_player<=255)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 playerSmallId;
                        in >> playerSmallId;
                        public_informations.simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> public_informations.simplifiedId;
                    }

                    //x and y
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 directionAndPlayerType;
                    in >> directionAndPlayerType;
                    quint8 directionInt,playerTypeInt;
                    directionInt=directionAndPlayerType & 0x0F;
                    playerTypeInt=directionAndPlayerType & 0xF0;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2, directionAndPlayerType: %3, line: %4").arg(directionInt).arg(mainCodeType).arg(directionAndPlayerType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;
                    Player_type playerType=(Player_type)playerTypeInt;
                    if(playerType!=Player_type_normal && playerType!=Player_type_premium && playerType!=Player_type_gm && playerType!=Player_type_dev)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("playerType have wrong value: %1, at main ident: %2, directionAndPlayerType: %3, line: %4").arg(playerTypeInt).arg(mainCodeType).arg(directionAndPlayerType).arg(__LINE__));
                        return;
                    }
                    public_informations.type=playerType;

                    //the speed
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> public_informations.speed;

                    //the pseudo
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                    public_informations.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(public_informations.pseudo.isEmpty())
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed for pseudo: %1, rawData: %2, line: %3").arg(mainCodeType).arg(QString(rawText.toHex())).arg(__LINE__));
                        return;
                    }

                    //the skin
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 skinId;
                    in >> skinId;
                    public_informations.skinId=skinId;

                    if(public_informations.simplifiedId==player_informations.public_informations.simplifiedId)
                    {
                        setLastDirection(direction);
                        player_informations.public_informations=public_informations;
                        emit have_current_player_info(player_informations);
                    }
                    emit insert_player(public_informations,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
        }
        break;
        //Move player on map
        case 0xC7:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }
            //move the player
            quint8 directionInt,moveListSize;
            quint16 playerSizeList;
            if(max_player<=255)
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                quint8 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("move size == 0 with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
                        new_movement.second=(Direction)directionInt;
                        movement << new_movement;
                        index_sub_loop++;
                    }
                    emit move_player(playerId,movement);
                    index++;
                }
            }
            else
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("move size == 0 with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> new_movement.first;
                        in >> directionInt;
                        new_movement.second=(Direction)directionInt;
                        movement << new_movement;
                        index_sub_loop++;
                    }
                    emit move_player(playerId,movement);
                    index++;
                }
            }
        }
        break;
        //Remove player from map
        case 0xC8:
        {
            //remove player
            quint16 playerSizeList;
            if(max_player<=255)
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
                quint8 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    emit remove_player(playerId);
                    index++;
                }
            }
            else
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerId;
                    emit remove_player(playerId);
                    index++;
                }
            }
        }
        break;
        //Player number
        case 0xC3:
        {
            if(max_player<=255)
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 current_player_connected_8Bits;
                in >> current_player_connected_8Bits;
                emit number_of_player(current_player_connected_8Bits,max_player);
            }
            else
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint16 current_player_connected_16Bits;
                in >> current_player_connected_16Bits;
                emit number_of_player(current_player_connected_16Bits,max_player);
            }
        }
        break;
        //drop all player on the map
        case 0xC4:
            emit dropAllPlayerOnTheMap();
        break;
        //Reinser player on same map
        case 0xC5:
        {
            quint16 playerSizeList;
            if(max_player<=255)
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 numberOfPlayer;
                in >> numberOfPlayer;
                playerSizeList=numberOfPlayer;
            }
            else
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                in >> playerSizeList;
            }
            int index_sub_loop=0;
            while(index_sub_loop<playerSizeList)
            {
                //player id
                quint16 simplifiedId;
                if(max_player<=255)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 playerSmallId;
                    in >> playerSmallId;
                    simplifiedId=playerSmallId;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> simplifiedId;
                }

                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;

                //direction and player type
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 directionInt;
                in >> directionInt;
                if(directionInt<1 || directionInt>8)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                    return;
                }
                Direction direction=(Direction)directionInt;

                emit reinsert_player(simplifiedId,x,y,direction);
                index_sub_loop++;
            }
        }
        break;
        //Reinser player on other map
        case 0xC6:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                quint16 playerSizeList;
                if(max_player<=255)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 numberOfPlayer;
                    in >> numberOfPlayer;
                    playerSizeList=numberOfPlayer;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> playerSizeList;
                }
                int index_sub_loop=0;
                while(index_sub_loop<playerSizeList)
                {
                    //player id
                    quint16 simplifiedId;
                    if(max_player<=255)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 playerSmallId;
                        in >> playerSmallId;
                        simplifiedId=playerSmallId;
                    }
                    else
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> simplifiedId;
                    }

                    //x and y
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 directionInt;
                    in >> directionInt;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    emit full_reinsert_player(simplifiedId,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                return;
            }

        }
        break;
        //Insert plant on map
        case 0xD1:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;
                //plant
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 plant;
                in >> plant;
                //seconds to mature
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint16 seconds_to_mature;
                in >> seconds_to_mature;

                emit insert_plant(mapId,x,y,plant,seconds_to_mature);
                index++;
            }
        }
        break;
        //Remove plant on map
        case 0xD2:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else if(number_of_map<=65535)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    quint16 mapTempId;
                    in >> mapTempId;
                    mapId=mapTempId;
                }
                else
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;

                emit remove_plant(mapId,x,y);
                index++;
            }
        }
        break;
        default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow ident main code: %1").arg(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("remaining data: parseMessage(%1,%2 %3)")
                      .arg(mainCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

void Api_protocol::parseFullMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("is not logged with main ident: %1").arg(mainCodeType));
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
                case 0x0003:
                case 0x0004:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 fileListSize;
                    in >> fileListSize;
                    int index=0;
                    while(index<fileListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 fileNameSize;
                        in >> fileNameSize;
                        if((in.device()->size()-in.device()->pos())<(int)fileNameSize)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                        in.device()->seek(in.device()->pos()+rawText.size());
                        QString fileName=QString::fromUtf8(rawText.data(),rawText.size());
                        if(!extensionAllowed.contains(QFileInfo(fileName).suffix()))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("extension not allowed: %4 with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__).arg(QFileInfo(fileName).suffix()));
                            if(!tolerantMode)
                                return;
                        }
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint32 size;
                        in >> size;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint64 mtime;
                        in >> mtime;
                        QDateTime date;
                        date.setTime_t(mtime);
                        if((in.device()->size()-in.device()->pos())<size)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong file data size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        QByteArray dataFile=data.mid(in.device()->pos(),size);
                        in.device()->seek(in.device()->pos()+size);
                        emit newFile(fileName,dataFile,mtime);
                        index++;
                    }
                    return;//no remaining data, because all remaing is used as file data
                }
                break;
                //chat as input
                case 0x0005:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 chat_type_int;
                    in >> chat_type_int;
                    if(chat_type_int<1 || chat_type_int>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong chat type with main ident: %1, subCodeType: %2, chat_type_int: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(chat_type_int).arg(__LINE__));
                        return;
                    }
                    Chat_type chat_type=(Chat_type)chat_type_int;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString text;
                    in >> text;
                    if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
                        emit new_system_text(chat_type,text);
                    else
                    {
                        quint8 pseudoSize;
                        in >> pseudoSize;
                        if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
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
                            parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(rawText.toHex()))
                                          .arg(rawText.size())
                                          .arg(__LINE__)
                                          );
                            return;
                        }
                        quint8 player_type_int;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_type_int;
                        Player_type player_type=(Player_type)player_type_int;
                        if(player_type!=Player_type_normal && player_type!=Player_type_premium && player_type!=Player_type_gm && player_type!=Player_type_dev)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong player type with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(player_type_int).arg(__LINE__));
                            return;
                        }
                        emit new_chat_text(chat_type,text,pseudo,player_type);
                    }
                }
                break;
                //kicked/ban and reason
                case 0x0008:
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 code;
                    in >> code;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString text;
                    in >> text;
                    switch(code)
                    {
                        case 0x01:
                            parseError(tr("Disconnected by the server"),QString("You have been kicked by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        case 0x02:
                            parseError(tr("Disconnected by the server"),QString("You have been ban by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        case 0x03:
                            parseError(tr("Disconnected by the server"),QString("You have been disconnected by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                        default:
                            parseError(tr("Disconnected by the server"),QString("You have been disconnected by strange way by the server with the reason: %1, line: %2").arg(text).arg(__LINE__));
                        return;
                    }
                }
                break;
                //clan dissolved
                case 0x0009:
                    emit clanDissolved();
                break;
                //clan info
                case 0x000A:
                {
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString name;
                    in >> name;
                    emit clanInformations(name);
                }
                break;
                //clan invite
                case 0x000B:
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint32 clanId;
                    in >> clanId;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QString name;
                    in >> name;
                    emit clanInvite(clanId,name);
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint32,quint32> items;
                    quint32 inventorySize,id,quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint32,quint32> warehouse_items;
                    in >> inventorySize;
                    index=0;
                    while(index<inventorySize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(warehouse_items.contains(id))
                            warehouse_items[id]+=quantity;
                        else
                            warehouse_items[id]=quantity;
                        index++;
                    }
                    emit have_inventory(items,warehouse_items);
                }
                break;
                //Add object
                case 0x0002:
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint32,quint32> items;
                    quint32 inventorySize,id,quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    emit add_to_inventory(items);
                }
                break;
                //Remove object
                case 0x0003:
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    QHash<quint32,quint32> items;
                    quint32 inventorySize,id,quantity;
                    in >> inventorySize;
                    quint32 index=0;
                    while(index<inventorySize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2; for the id, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, for the quantity, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> quantity;
                        if(items.contains(id))
                            items[id]+=quantity;
                        else
                            items[id]=quantity;
                        index++;
                    }
                    emit remove_to_inventory(items);
                }
                break;
                //the other player have put object
                case 0x0004:
                {
                    if(!isInTrade)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("not in trade trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),"wrong remaining size for trade add cash");
                                return;
                            }
                            quint64 cash;
                            in >> cash;
                            emit tradeAddTradeCash(cash);
                        }
                        break;
                        //item
                        case 0x02:
                        {
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),"wrong remaining size for trade add item id");
                                return;
                            }
                            quint32 item;
                            in >> item;
                            if((data.size()-in.device()->pos())<((int)sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),"wrong remaining size for trade add item quantity");
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            emit tradeAddTradeObject(item,quantity);
                        }
                        break;
                        //monster
                        case 0x03:
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.captured_with;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
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
                                    parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint32 sub_size;
                            in >> sub_size;
                            quint32 sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                monster.skills << skill;
                                sub_index++;
                            }
                            emit tradeAddTradeMonster(monster);
                        }
                        break;
                        default:
                            parseError(tr("Procotol wrong or corrupted"),"wrong type for trade add");
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
                        parseError(tr("Internal error"),QString("request is running, skip this trade exchange"));
                        return;
                    }
                    if(isInTrade)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("already in trade trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
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
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    quint8 skinId;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinId;
                    isInTrade=true;
                    emit tradeAcceptedByOther(pseudo,skinId);
                }
                break;
                //the other player have canceled
                case 0x0006:
                {
                    isInTrade=false;
                    emit tradeCanceledByOther();
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
                        parseError(tr("Procotol wrong or corrupted"),QString("not in trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    emit tradeFinishedByOther();
                }
                break;
                //the server have validated the transaction
                case 0x0008:
                {
                    if(!isInTrade)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("not in trade with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    isInTrade=false;
                    emit tradeValidatedByTheServer();
                }
                break;
                //random seeds as input
                case 0x0009:
                {
                    emit random_seeds(data);
                    return;//quit here because all data is always used
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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

                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> attackReturnListSize;
                    indexAttackReturn=0;
                    while(indexAttackReturn<attackReturnListSize)
                    {
                        Skill::AttackReturn tempAttackReturn;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> tempuint;
                        if(tempuint>1)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("code to bool with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        tempAttackReturn.doByTheCurrentMonster=(tempuint!=0);
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> tempuint;
                        if(tempuint>1)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("code to bool with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        tempAttackReturn.success=(tempuint!=0);
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> tempAttackReturn.attack;
                        //add buff
                        index=0;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buffEffect.buff;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            buffEffect.on=(ApplyOn)tempuint;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buffEffect.level;
                            tempAttackReturn.addBuffEffectMonster << buffEffect;
                            index++;
                        }
                        //remove buff
                        index=0;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::BuffEffect buffEffect;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buffEffect.buff;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            buffEffect.on=(ApplyOn)tempuint;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buffEffect.level;
                            tempAttackReturn.removeBuffEffectMonster << buffEffect;
                            index++;
                        }
                        //life effect
                        index=0;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> lifeEffectReturn.quantity;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            lifeEffectReturn.on=(ApplyOn)tempuint;
                            tempAttackReturn.lifeEffectMonster << lifeEffectReturn;
                            index++;
                        }
                        //buff effect
                        index=0;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> listSizeShort;
                        while(index<listSizeShort)
                        {
                            Skill::LifeEffectReturn lifeEffectReturn;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> lifeEffectReturn.quantity;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            lifeEffectReturn.on=(ApplyOn)tempuint;
                            tempAttackReturn.buffLifeEffectMonster << lifeEffectReturn;
                            index++;
                        }
                        attackReturn << tempAttackReturn;
                        indexAttackReturn++;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> monsterPlace;
                    if(monsterPlace!=0)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> publicPlayerMonster.monster;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> publicPlayerMonster.level;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> publicPlayerMonster.hp;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> publicPlayerMonster.captured_with;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(genderInt));
                                return;
                            break;
                        }
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> buffListSize;
                        index=0;
                        while(index<buffListSize)
                        {
                            PlayerBuff buff;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buff.buff;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                                return;
                            }
                            in >> buff.level;
                            publicPlayerMonster.buffs << buff;
                            index++;
                        }
                    }


                    // -------------------------
                    if(monsterPlace==0)
                        emit sendBattleReturn(attackReturn);
                    else
                        emit sendFullBattleReturn(attackReturn,monsterPlace,publicPlayerMonster);
                }
                break;
                //The other player have declined you battle request
                case 0x0007:
                    emit battleCanceledByOther();
                break;
                //The other player have accepted you battle request
                case 0x0008:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
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
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    quint8 skinId;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinId;
                    PublicPlayerMonster publicPlayerMonster;
                    QList<quint8> stat;
                    quint8 genderInt;
                    int buffListSize;
                    quint8 monsterPlace;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    quint8 statListSize;
                    in >> statListSize;
                    int index=0;
                    while(index<statListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 statEntry;
                        in >> statEntry;
                        stat << statEntry;
                        index++;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> monsterPlace;
                    if(monsterPlace<=0 || monsterPlace>stat.size())
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("monster place wrong range with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.monster;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.level;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.hp;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> publicPlayerMonster.captured_with;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(genderInt));
                            return;
                        break;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                        return;
                    }
                    in >> buffListSize;
                    index=0;
                    while(index<buffListSize)
                    {
                        PlayerBuff buff;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> buff.buff;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> buff.level;
                        publicPlayerMonster.buffs << buff;
                        index++;
                    }
                    emit battleAcceptedByOther(pseudo,skinId,stat,monsterPlace,publicPlayerMonster);
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                    return;
                }
                in >> returnCode;
                switch(returnCode)
                {
                    case 0x01:
                        emit captureCityYourAreNotLeader();
                    break;
                    case 0x02:
                    {
                        QString city;
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                            return;
                        in >> city;
                        emit captureCityYourLeaderHaveStartInOtherCity(city);
                    }
                    break;
                    case 0x03:
                        emit captureCityPreviousNotFinished();
                    break;
                    case 0x04:
                    {
                        quint16 player_count,clan_count;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_count;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> clan_count;
                        if((in.device()->size()-in.device()->pos())==0)
                            emit captureCityStartBattle(player_count,clan_count);
                        else if((in.device()->size()-in.device()->pos())==(int)(sizeof(quint32)))
                        {
                            quint32 fightId;
                            in >> fightId;
                            emit captureCityStartBotFight(player_count,clan_count,fightId);
                        }
                    }
                    break;
                    case 0x05:
                    {
                        quint16 player_count,clan_count;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> player_count;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint16)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                            return;
                        }
                        in >> clan_count;
                        emit captureCityDelayedStart(player_count,clan_count);
                    }
                    break;
                    case 0x06:
                        emit captureCityWin();
                    break;
                    default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, returnCode: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(returnCode).arg(__LINE__));
                    return;
                }
            }
            break;
            default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
            return;
        }
        break;
        default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow ident main code: %1").arg(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("remaining data: parseMessage(%1,%2,%3 %4)")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

//have query with reply
void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(tr("Procotol wrong or corrupted"),QString("have not query of this type, mainCodeType: %1, queryNumber: %2").arg(mainCodeType).arg(queryNumber));
}

void Api_protocol::parseFullQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    if(!is_logged)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("is not logged with main ident: %1").arg(mainCodeType));
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
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint8 mapTempId;
                        in >> mapTempId;
                        mapId=mapTempId;
                    }
                    else if(number_of_map<=65535)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        quint16 mapTempId;
                        in >> mapTempId;
                        mapId=mapTempId;
                    }
                    else
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                            return;
                        }
                        in >> mapId;
                    }
                    quint8 x,y;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> x;
                    in >> y;
                    quint8 directionInt;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> directionInt;
                    if(directionInt<1 || directionInt>4)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2, line: %3").arg(directionInt).arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    teleportList << queryNumber;
                    emit teleportTo(mapId,x,y,direction);
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("Already on battle"));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
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
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    quint8 skinInt;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinInt;
                    tradeRequestId << queryNumber;
                    emit tradeRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("Already on trade"));
                        return;
                    }
                    if(!battleRequestId.isEmpty() || isInBattle)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("Already on battle"));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4, line: %5")
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
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4, line: %5")
                                      .arg(mainCodeType)
                                      .arg(subCodeType)
                                      .arg(QString(rawText.toHex()))
                                      .arg(rawText.size())
                                      .arg(__LINE__)
                                      );
                        return;
                    }
                    quint8 skinInt;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, line: %2").arg(mainCodeType).arg(__LINE__));
                        return;
                    }
                    in >> skinInt;
                    battleRequestId << queryNumber;
                    emit battleRequested(pseudo,skinInt);
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2, line: %3").arg(mainCodeType).arg(subCodeType).arg(__LINE__));
                return;
            }
        }
        break;
        default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow ident main code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("remaining data: parseMessage(%1,%2,%3 %4)")
                      .arg(mainCodeType)
                      .arg(subCodeType)
                      .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                      .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                      );
        return;
    }
}

//send reply
void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(tr("Procotol wrong or corrupted"),QString("have not reply of this type, mainCodeType: %1, queryNumber: %2").arg(mainCodeType).arg(queryNumber));
}

void Api_protocol::parseFullReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    switch(mainCodeType)
    {
        case 0x02:
        {
            //local the query number to get the type
            switch(subCodeType)
            {
                //Protocol initialization
                case 0x0001:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        quint8 compressionCode;
                        in >> compressionCode;
                        switch(compressionCode)
                        {
                            case 0x00:
                                ProtocolParsing::compressionType=ProtocolParsing::CompressionType_None;
                            break;
                            case 0x01:
                                ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Zlib;
                            break;
                            case 0x02:
                                ProtocolParsing::compressionType=ProtocolParsing::CompressionType_Xz;
                            break;
                            default:
                                emit newError(tr("Procotol wrong or corrupted"),QString("compression type wrong with main ident: %1, subCodeType:%2, and queryNumber: %3, type: query_type_protocol").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }

                        have_receive_protocol=true;
                        emit protocol_is_good();
                    }
                    else if(returnCode==0x02)
                    {
                        emit newError(tr("Procotol wrong or corrupted"),QString("the server have returned: protocol wrong"));
                        return;
                    }
                    else if(returnCode==0x03)
                    {
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            emit newError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainCodeType));
                            return;
                        }
                        QString string;
                        in >> string;
                        DebugClass::debugConsole("disconnect with reason: "+string);
                        emit disconnected(string);
                        return;
                    }
                    else
                    {
                        emit newError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                //Get first data and send the login
                case 0x0002:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                    {
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        QString string;
                        in >> string;
                        DebugClass::debugConsole("is not logged, reason: "+string);
                        emit notLogged(string);
                        return;
                    }
                    else if(returnCode==0x02)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the max player, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> max_player;
                        setMaxPlayers(max_player);
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id, line: %1").arg(__LINE__));
                            return;
                        }
                        else if(max_player<=255)
                        {
                            quint8 tempSize;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> tempSize;
                            player_informations.public_informations.simplifiedId=tempSize;
                        }
                        else
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> player_informations.public_informations.simplifiedId;
                        }

                        quint32 captureRemainingTime;
                        quint8 captureFrequencyType;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the city capture remainingTime, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> captureRemainingTime;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the city capture type, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> captureFrequencyType;
                        switch(captureFrequencyType)
                        {
                            case 0x01:
                            case 0x02:
                            break;
                            default:
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong captureFrequencyType, line: %1").arg(__LINE__));
                            return;
                        }
                        emit cityCapture(captureRemainingTime,captureFrequencyType);

                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the factoryPriceChange, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> CommonSettings::commonSettings.factoryPriceChange;

                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        QString tempAllow;
                        in >> tempAllow;
                        player_informations.allow=FacilityLib::QStringToAllow(tempAllow);
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player clan, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.clan;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player clan leader, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 tempClanLeader;
                        in >> tempClanLeader;
                        if(tempClanLeader==0x01)
                            player_informations.clan_leader=true;
                        else
                            player_informations.clan_leader=false;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player cash, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.cash;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint64))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.warehouse_cash;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(double))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player cash ware house, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> player_informations.bitcoin;
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                            return;
                        }
                        in >> player_informations.bitcoinAddress;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the number of map, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> number_of_map;

                        //recipes
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the recipe list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint32 recipe_list_size;
                        in >> recipe_list_size;
                        quint32 recipeId;
                        quint32 index=0;
                        while(index<recipe_list_size)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player local recipe, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> recipeId;
                            player_informations.recipes << recipeId;
                            index++;
                        }

                        //monsters
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint8 gender;
                        quint32 monster_list_size;
                        in >> monster_list_size;
                        index=0;
                        quint32 sub_size,sub_index;
                        while(index<monster_list_size)
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.captured_with;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
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
                                    parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
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
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster list size, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster_list_size;
                        index=0;
                        while(index<monster_list_size)
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.captured_with;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
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
                                    parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
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
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerReputation playerReputation;
                        QString type;
                        index=0;
                        quint8 sub_size8;
                        in >> sub_size8;
                        while(index<sub_size8)
                        {
                            if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> type;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.level;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(qint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerReputation.point;
                            player_informations.reputation[type]=playerReputation;
                            index++;
                        }
                        //quest
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        PlayerQuest playerQuest;
                        quint32 playerQuestId;
                        index=0;
                        in >> sub_size8;
                        while(index<sub_size8)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(qint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> playerQuestId;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.step;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(qint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation point, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> playerQuest.finish_one_time;
                            if(playerQuest.step<=0 && !playerQuest.finish_one_time)
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("can't be to step 0 if have never finish the quest, line: %1").arg(__LINE__));
                                return;
                            }
                            player_informations.quests[playerQuestId]=playerQuest;
                            index++;
                        }
                        //bot_already_beaten
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the reputation list size, line: %1").arg(__LINE__));
                            return;
                        }
                        quint32 bot_already_beaten,sub_size32;
                        index=0;
                        in >> sub_size32;
                        while(index<sub_size32)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType:%2, and queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                                return;
                            }
                            in >> bot_already_beaten;
                            player_informations.bot_already_beaten << bot_already_beaten;
                            index++;
                        }

                        is_logged=true;
                        emit logged();
                    }
                    else
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("bad return code: %1, line: %2").arg(returnCode).arg(__LINE__));
                        return;
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
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                emit clanActionSuccess(0);
                            else
                            {
                                quint32 clanId;
                                in >> clanId;
                                emit clanActionSuccess(clanId);
                            }
                        break;
                        case 0x02:
                            emit clanActionFailed();
                        break;
                        default:
                            parseError(tr("Procotol wrong or corrupted"),QString("bad return code: %1, line: %2").arg(returnCode).arg(__LINE__));
                        break;
                    }
                }
                break;
                default:
                    parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
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
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                        emit seed_planted(true);
                    else if(returnCode==0x02)
                        emit seed_planted(false);
                    else
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                //Collect mature plant
                case 0x0007:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
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
                            emit plant_collected((Plant_collect)returnCode);
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Usage of recipe
                case 0x0008:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x03:
                            emit recipeUsed((RecipeUsage)returnCode);
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                }
                break;
                //Use object
                case 0x0009:
                {
                    quint32 item=lastObjectUsed.first();
                    lastObjectUsed.removeFirst();
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(CommonDatapack::commonDatapack.items.trap.contains(item))
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        quint32 newMonsterId;
                        in >> newMonsterId;
                        emit monsterCatch(newMonsterId);
                    }
                    else
                    {
                        switch(returnCode)
                        {
                            case 0x01:
                            case 0x02:
                            case 0x03:
                                emit objectUsed((ObjectUsage)returnCode);
                            break;
                            default:
                            parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                    }
                }
                break;
                //Get shop list
                case 0x000A:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 shopListSize;
                    in >> shopListSize;
                    quint32 index=0;
                    QList<ItemToSellOrBuy> items;
                    while(index<shopListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*3))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        items << item;
                        index++;
                    }
                    haveShopAction=false;
                    emit haveShopList(items);
                }
                break;
                //Buy object
                case 0x000B:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x04:
                            emit haveBuyObject((BuyStat)returnCode,0);
                        break;
                        case 0x03:
                        {
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            emit haveBuyObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    haveShopAction=false;
                }
                break;
                //Sell object
                case 0x000C:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x02:
                        case 0x04:
                            emit haveSellObject((SoldStat)returnCode,0);
                        break;
                        case 0x03:
                        {
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            emit haveSellObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    haveShopAction=false;
                }
                break;
                case 0x000D:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 remainingProductionTime;
                    in >> remainingProductionTime;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint32 shopListSize;
                    quint32 index;
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> resources;
                    while(index<shopListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*3))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        resources << item;
                        index++;
                    }
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> shopListSize;
                    index=0;
                    QList<ItemToSellOrBuy> products;
                    while(index<shopListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)*3))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        ItemToSellOrBuy item;
                        in >> item.object;
                        in >> item.price;
                        in >> item.quantity;
                        products << item;
                        index++;
                    }
                    haveFactoryAction=false;
                    emit haveFactoryList(remainingProductionTime,resources,products);
                }
                break;
                case 0x000E:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x03:
                        case 0x04:
                            emit haveBuyFactoryObject((BuyStat)returnCode,0);
                        break;
                        case 0x02:
                        {
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            emit haveBuyFactoryObject((BuyStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    haveFactoryAction=false;
                }
                break;
                case 0x000F:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        case 0x03:
                        case 0x04:
                            emit haveSellFactoryObject((SoldStat)returnCode,0);
                        break;
                        case 0x02:
                        {
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 newPrice;
                            in >> newPrice;
                            emit haveSellFactoryObject((SoldStat)returnCode,newPrice);
                        }
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("unknow return code with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    haveFactoryAction=false;
                }
                break;
                case 0x0010:
                {
                    quint32 listSize,index;
                    QList<MarketObject> marketObjectList;
                    QList<MarketMonster> marketMonsterList;
                    QList<MarketObject> marketOwnObjectList;
                    QList<MarketMonster> marketOwnMonsterList;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    double bitcoin;
                    in >> bitcoin;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.bitcoin;
                        marketObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.bitcoin;
                        marketMonsterList << marketMonster;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketObject marketObject;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.marketObjectId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.objectId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.quantity;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.price;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketObject.bitcoin;
                        marketOwnObjectList << marketObject;
                        index++;
                    }
                    in >> listSize;
                    index=0;
                    while(index<listSize)
                    {
                        MarketMonster marketMonster;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monsterId;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.monster;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.level;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.price;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                            return;
                        }
                        in >> marketMonster.bitcoin;
                        marketOwnMonsterList << marketMonster;
                        index++;
                    }
                    emit marketList(cash,bitcoin,marketObjectList,marketMonsterList,marketOwnObjectList,marketOwnMonsterList);
                }
                break;
                case 0x0011:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            if((in.device()->size()-in.device()->pos())==0)
                                emit marketBuy(true);
                        break;
                        case 0x02:
                        case 0x03:
                            emit marketBuy(false);
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01 && (in.device()->size()-in.device()->pos())>0)
                    {
                        PlayerMonster monster;
                        PlayerBuff buff;
                        PlayerMonster::PlayerSkill skill;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.monster;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster level, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.level;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.remaining_xp;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster hp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.hp;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster sp, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.sp;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.captured_with;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
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
                                parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                return;
                            break;
                        }
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> monster.egg_step;

                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                            return;
                        }
                        quint32 sub_size,sub_index;
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> buff.buff;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> buff.level;
                            monster.buffs << buff;
                            sub_index++;
                        }

                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                            return;
                        }
                        in >> sub_size;
                        sub_index=0;
                        while(sub_index<sub_size)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> skill.skill;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> skill.level;
                            monster.skills << skill;
                            sub_index++;
                        }
                        emit marketBuyMonster(monster);
                    }
                }
                break;
                case 0x0012:
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                            emit marketPut(true);
                        break;
                        case 0x02:
                            emit marketPut(false);
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                break;
                case 0x0013:
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint64 cash;
                    in >> cash;
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(double)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    double bitcoin;
                    in >> bitcoin;
                    emit marketGetCash(cash,bitcoin);
                break;
                case 0x0014:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    switch(returnCode)
                    {
                        case 0x01:
                        break;
                        case 0x02:
                            emit marketWithdrawCanceled();
                        break;
                        default:
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong return code, line: %1").arg(__LINE__));
                        return;
                    }
                    if(returnCode==0x01)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong return code, line: %1").arg(__LINE__));
                            return;
                        }
                        if(returnType==0x01)
                        {
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 objectId;
                            in >> objectId;
                            if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint32)))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType:%2, and queryNumber: %3, line: %4").arg(mainCodeType).arg(subCodeType).arg(queryNumber).arg(__LINE__));
                                return;
                            }
                            quint32 quantity;
                            in >> quantity;
                            emit marketWithdrawObject(objectId,quantity);
                        }
                        else
                        {
                            PlayerMonster monster;
                            PlayerBuff buff;
                            PlayerMonster::PlayerSkill skill;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id bd, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.id;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster id, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.monster;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster level, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.level;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster remaining_xp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.remaining_xp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster hp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.hp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster sp, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.sp;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.captured_with;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster captured_with, line: %1").arg(__LINE__));
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
                                    parseError(tr("Procotol wrong or corrupted"),QString("gender code wrong: %2, line: %1").arg(__LINE__).arg(gender));
                                    return;
                                break;
                            }
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster egg_step, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> monster.egg_step;

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the buff monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            quint32 sub_size,sub_index;
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.buff;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster buff level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> buff.level;
                                monster.buffs << buff;
                                sub_index++;
                            }

                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster size of list of the skill monsters, line: %1").arg(__LINE__));
                                return;
                            }
                            in >> sub_size;
                            sub_index=0;
                            while(sub_index<sub_size)
                            {
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.skill;
                                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                                {
                                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the monster skill level, line: %1").arg(__LINE__));
                                    return;
                                }
                                in >> skill.level;
                                monster.skills << skill;
                                sub_index++;
                            }
                            emit marketWithdrawMonster(monster);
                        }
                    }
                }
                break;
                default:
                    parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType code: %1, with mainCodeType: %2, line: %3").arg(subCodeType).arg(mainCodeType).arg(__LINE__));
                    return;
                break;
            }
        }
        break;
        default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1, line: %2").arg(mainCodeType).arg(__LINE__));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("error: remaining data: parseReplyData(%1,%2,%3), line: %4, data: %5 %6")
                   .arg(mainCodeType).arg(subCodeType).arg(queryNumber)
                   .arg(__LINE__)
                   .arg(QString(data.mid(0,in.device()->pos()).toHex()))
                   .arg(QString(data.mid(in.device()->pos(),(in.device()->size()-in.device()->pos())).toHex()))
                   );
        return;
    }
}

bool Api_protocol::getHaveShopAction()
{
    return haveShopAction;
}

bool Api_protocol::getHaveFactoryAction()
{
    return haveFactoryAction;
}

void Api_protocol::parseError(const QString &userMessage,const QString &errorString)
{
    if(tolerantMode)
        DebugClass::debugConsole(QString("packet ignored due to: %1").arg(errorString));
    else
        emit newError(userMessage,errorString);
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

quint64 Api_protocol::getTXSize()
{
    if(output!=NULL)
        return output->getTXSize();
    return 0;
}

quint8 Api_protocol::queryNumber()
{
    if(lastQueryNumber>=254)
        lastQueryNumber=1;
    return lastQueryNumber++;
}

bool Api_protocol::sendProtocol()
{
    if(have_send_protocol)
    {
        emit newError(tr("Internal problem"),QString("Have already send the protocol"));
        return false;
    }
    have_send_protocol=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << QString(PROTOCOL_HEADER);
    if(output==NULL)
        return false;
    output->packFullOutcommingQuery(0x02,0x0001,queryNumber(),outputData);
    return true;
}

bool Api_protocol::tryLogin(const QString &login, const QString &pass)
{
    if(!have_send_protocol)
    {
        emit newError(tr("Internal problem"),QString("Have not send the protocol"));
        return false;
    }
    if(is_logged)
    {
        emit newError(tr("Internal problem"),QString("Is already logged"));
        return false;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    quint8 query_number=queryNumber();
    out << login;
    QCryptographicHash hash(QCryptographicHash::Sha512);
    hash.addData(pass.toUtf8());
    outputData+=hash.result();
    if(output==NULL)
        return false;
    output->packFullOutcommingQuery(0x02,0x0002,query_number,outputData);
    return true;
}

void Api_protocol::send_player_move(const quint8 &moved_unit,const Direction &direction)
{
    quint8 directionInt=static_cast<quint8>(direction);
    if(directionInt<1 || directionInt>8)
    {
        DebugClass::debugConsole(QString("direction given wrong: %1").arg(directionInt));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << moved_unit;
    out << directionInt;
    if(output==NULL)
        return;
    output->packOutcommingData(0x40,outputData);
}

void Api_protocol::send_player_direction(const Direction &the_direction)
{
    newDirection(the_direction);
}

void Api_protocol::sendChatText(const Chat_type &chatType, const QString &text)
{
    if(chatType!=Chat_type_local && chatType!=Chat_type_all && chatType!=Chat_type_clan && chatType!=Chat_type_aliance && chatType!=Chat_type_system && chatType!=Chat_type_system_important)
    {
        DebugClass::debugConsole("chatType wrong: "+QString::number(chatType));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)chatType;
    out << text;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x42,0x0003,outputData);
    if(!text.startsWith('/'))
        emit new_chat_text(chatType,text,player_informations.public_informations.pseudo,player_informations.public_informations.type);
}

void Api_protocol::sendPM(const QString &text,const QString &pseudo)
{
    emit new_chat_text(Chat_type_pm,text,tr("To: ")+pseudo,Player_type_normal);
    if(this->player_informations.public_informations.pseudo==pseudo)
        return;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)Chat_type_pm;
    out << text;
    out << pseudo;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x42,0x003,outputData);
}

void Api_protocol::teleportDone()
{
    if(output==NULL)
        return;
    output->postReplyData(teleportList.first(),QByteArray());
    teleportList.removeFirst();
}

void Api_protocol::useSeed(const quint8 &plant_id)
{
    QByteArray outputData;
    outputData[0]=plant_id;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0006,queryNumber(),outputData);
}

void Api_protocol::monsterMoveUp(const quint8 &number)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << number;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0008,outputData);
}

void Api_protocol::monsterMoveDown(const quint8 &number)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << number;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0008,outputData);
}

//inventory
void Api_protocol::destroyObject(const quint32 &object, const quint32 &quantity)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << object;
    out << quantity;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0002,outputData);
}

void Api_protocol::useObject(const quint32 &object)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << object;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0009,queryNumber(),outputData);
    lastObjectUsed << object;
}

void Api_protocol::wareHouseStore(const qint64 &cash, const QList<QPair<quint32,qint32> > &items, const QList<quint32> &withdrawMonsters, const QList<quint32> &depositeMonsters)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << cash;

    out << (quint32)items.size();
    int index=0;
    while(index<items.size())
    {
        out << (quint32)items.at(index).first;
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

    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0006,outputData);
}

void Api_protocol::getShopList(const quint32 &shopId)
{
    if(haveShopAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveShopAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)shopId;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x000A,queryNumber(),outputData);
}

void Api_protocol::buyObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(haveShopAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveShopAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)shopId;
    out << (quint32)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x000B,queryNumber(),outputData);
}

void Api_protocol::sellObject(const quint32 &shopId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(haveShopAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveShopAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)shopId;
    out << (quint32)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x000C,queryNumber(),outputData);
}

void Api_protocol::getFactoryList(const quint32 &factoryId)
{
    if(haveFactoryAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveFactoryAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)factoryId;
    output->packFullOutcommingQuery(0x10,0x000D,queryNumber(),outputData);
}

void Api_protocol::buyFactoryProduct(const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(haveFactoryAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveFactoryAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)factoryId;
    out << (quint32)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x000E,queryNumber(),outputData);
}

void Api_protocol::sellFactoryResource(const quint32 &factoryId,const quint32 &objectId,const quint32 &quantity,const quint32 &price)
{
    if(haveFactoryAction)
    {
        DebugClass::debugConsole("already have shop action");
        return;
    }
    haveFactoryAction=true;
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)factoryId;
    out << (quint32)objectId;
    out << (quint32)quantity;
    out << (quint32)price;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x000F,queryNumber(),outputData);
}

void Api_protocol::tryEscape()
{
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0002,QByteArray());
}

void Api_protocol::heal()
{
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0006,QByteArray());
}

void Api_protocol::requestFight(const quint32 &fightId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)fightId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0007,outputData);
}

void Api_protocol::changeOfMonsterInFight(const quint32 &monsterId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)monsterId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0009,outputData);
}

void Api_protocol::useSkill(const quint32 &skill)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)skill;
    if(output==NULL)
        return;
    output->packOutcommingData(0x61,outputData);
}

void Api_protocol::learnSkill(const quint32 &monsterId,const quint32 &skill)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)monsterId;
    out << (quint32)skill;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x60,0x0004,outputData);
}

void Api_protocol::startQuest(const quint32 &questId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)questId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x6a,0x0001,outputData);
}

void Api_protocol::finishQuest(const quint32 &questId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)questId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x6a,0x0002,outputData);
}

void Api_protocol::cancelQuest(const quint32 &questId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)questId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x6a,0x0003,outputData);
}

void Api_protocol::nextQuestStep(const quint32 &questId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)questId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x6a,0x0004,outputData);
}

void Api_protocol::createClan(const QString &name)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << name;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData);
}

void Api_protocol::leaveClan()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData);
}

void Api_protocol::dissolveClan()
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x03;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData);
}

void Api_protocol::inviteClan(const QString &pseudo)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x04;
    out << pseudo;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData);
}

void Api_protocol::ejectClan(const QString &pseudo)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x05;
    out << pseudo;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x02,0x000D,queryNumber(),outputData);
}

void Api_protocol::inviteAccept(const bool &accept)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(accept)
        out << (quint8)0x01;
    else
        out << (quint8)0x02;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x42,0x0004,outputData);
}

void Api_protocol::waitingForCityCapture(const bool &cancel)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    if(!cancel)
        out << (quint8)0x00;
    else
        out << (quint8)0x01;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x6a,0x0005,outputData);
}

//market
void Api_protocol::getMarketList()
{
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0010,queryNumber(),QByteArray());
}

void Api_protocol::buyMarketObject(const quint32 &marketObjectId, const quint32 &quantity)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << marketObjectId;
    out << quantity;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0011,queryNumber(),outputData);
}

void Api_protocol::buyMarketMonster(const quint32 &monsterId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0011,queryNumber(),outputData);
}

void Api_protocol::putMarketObject(const quint32 &objectId,const quint32 &quantity,const quint32 &price,const double &bitcoin)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    out << price;
    out << bitcoin;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0012,queryNumber(),outputData);
}

void Api_protocol::putMarketMonster(const quint32 &monsterId,const quint32 &price,const double &bitcoin)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    out << price;
    out << bitcoin;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0012,queryNumber(),outputData);
}

void Api_protocol::recoverMarketCash()
{
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0013,queryNumber(),QByteArray());
}

void Api_protocol::withdrawMarketObject(const quint32 &objectId,const quint32 &quantity)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << objectId;
    out << quantity;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0014,queryNumber(),outputData);
}

void Api_protocol::withdrawMarketMonster(const quint32 &monsterId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << monsterId;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0014,queryNumber(),outputData);
}

void Api_protocol::collectMaturePlant()
{
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0007,queryNumber(),QByteArray());
}

//crafting
void Api_protocol::useRecipe(const quint32 &recipeId)
{
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint32)recipeId;
    if(output==NULL)
        return;
    output->packFullOutcommingQuery(0x10,0x0008,queryNumber(),outputData);
}

void Api_protocol::addRecipe(const quint32 &recipeId)
{
    player_informations.recipes << recipeId;
}

void Api_protocol::battleRefused()
{
    if(battleRequestId.isEmpty())
    {
        emit newError(tr("Internal problem"),QString("no battle request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    if(output==NULL)
        return;
    output->postReplyData(battleRequestId.first(),outputData);
    battleRequestId.removeFirst();
}

void Api_protocol::battleAccepted()
{
    if(battleRequestId.isEmpty())
    {
        emit newError(tr("Internal problem"),QString("no battle request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    if(output==NULL)
        return;
    output->postReplyData(battleRequestId.first(),outputData);
    battleRequestId.removeFirst();
}

//trade
void Api_protocol::tradeRefused()
{
    if(tradeRequestId.isEmpty())
    {
        emit newError(tr("Internal problem"),QString("no trade request to refuse"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    if(output==NULL)
        return;
    output->postReplyData(tradeRequestId.first(),outputData);
    tradeRequestId.removeFirst();
}

void Api_protocol::tradeAccepted()
{
    if(tradeRequestId.isEmpty())
    {
        emit newError(tr("Internal problem"),QString("no trade request to accept"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    if(output==NULL)
        return;
    output->postReplyData(tradeRequestId.first(),outputData);
    tradeRequestId.removeFirst();
    isInTrade=true;
}

void Api_protocol::tradeCanceled()
{
    if(!isInTrade)
    {
        emit newError(tr("Internal problem"),QString("in not in trade"));
        return;
    }
    isInTrade=false;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0005,QByteArray());
}

void Api_protocol::tradeFinish()
{
    if(!isInTrade)
    {
        emit newError(tr("Internal problem"),QString("in not in trade"));
        return;
    }
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0004,QByteArray());
}

void Api_protocol::addTradeCash(const quint64 &cash)
{
    if(cash==0)
    {
        emit newError(tr("Internal problem"),QString("can't send 0 for the cash"));
        return;
    }
    if(!isInTrade)
    {
        emit newError(tr("Internal problem"),QString("no in trade to send cash"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x01;
    out << cash;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0003,outputData);
}

void Api_protocol::addObject(const quint32 &item,const quint32 &quantity)
{
    if(quantity==0)
    {
        emit newError(tr("Internal problem"),QString("can't send a quantity of 0"));
        return;
    }
    if(!isInTrade)
    {
        emit newError(tr("Internal problem"),QString("no in trade to send object"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x02;
    out << item;
    out << quantity;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0003,outputData);
}

void Api_protocol::addMonster(const quint32 &monsterId)
{
    if(!isInTrade)
    {
        emit newError(tr("Internal problem"),QString("no in trade to send monster"));
        return;
    }
    QByteArray outputData;
    QDataStream out(&outputData, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_4);
    out << (quint8)0x03;
    out << monsterId;
    if(output==NULL)
        return;
    output->packFullOutcommingData(0x50,0x0003,outputData);
}

//to reset all
void Api_protocol::resetAll()
{
    //status for the query
    is_logged=false;
    have_send_protocol=false;
    have_receive_protocol=false;
    max_player=65535;
    number_of_map=0;
    player_informations.recipes.clear();
    player_informations.playerMonster.clear();
    player_informations.items.clear();
    player_informations.reputation.clear();
    player_informations.quests.clear();
    haveShopAction=false;
    haveFactoryAction=false;
    isInTrade=false;
    tradeRequestId.clear();
    isInBattle=false;
    battleRequestId.clear();

    //to send trame
    lastQueryNumber=1;
}

void Api_protocol::startReadData()
{
    canStartReadData=true;
}

QString Api_protocol::get_datapack_base_name() const
{
    return datapack_base_name;
}
