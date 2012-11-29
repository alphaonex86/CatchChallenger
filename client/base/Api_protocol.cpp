#include "Api_protocol.h"

using namespace Pokecraft;

#ifdef Q_CC_GNU
//this next header is needed to change file time/date under gcc
#include <utime.h>
#include <sys/stat.h>
#endif

//need host + port here to have datapack base

Api_protocol::Api_protocol(ConnectedSocket *socket,bool tolerantMode) :
    ProtocolParsingInput(socket,PacketModeTransmission_Client)
{
    output=new ProtocolParsingOutput(socket,PacketModeTransmission_Client);
    this->tolerantMode=tolerantMode;

    connect(this,SIGNAL(newInputQuery(quint8,quint8)),output,SLOT(newInputQuery(quint8,quint8)));
    connect(this,SIGNAL(newInputQuery(quint8,quint16,quint8)),output,SLOT(newInputQuery(quint8,quint16,quint8)));
    connect(output,SIGNAL(newOutputQuery(quint8,quint8)),this,SLOT(newOutputQuery(quint8,quint8)));
    connect(output,SIGNAL(newOutputQuery(quint8,quint16,quint8)),this,SLOT(newOutputQuery(quint8,quint16,quint8)));

    resetAll();
}

Api_protocol::~Api_protocol()
{
    delete output;
}

//have message without reply
void Api_protocol::parseMessage(const quint8 &mainCodeType,const QByteArray &data)
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
        //Insert player on map
        case 0xC0:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> mapId;
                }

                quint16 playerSizeList;
                if(max_player<=255)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                            return;
                        }
                        in >> public_informations.simplifiedId;
                    }

                    //x and y
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    quint8 directionAndPlayerType;
                    in >> directionAndPlayerType;
                    quint8 directionInt,playerTypeInt;
                    directionInt=directionAndPlayerType & 0x0F;
                    playerTypeInt=directionAndPlayerType & 0xF0;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2, directionAndPlayerType: %3").arg(directionInt).arg(mainCodeType).arg(directionAndPlayerType));
                        return;
                    }
                    Direction direction=(Direction)directionInt;
                    Player_type playerType=(Player_type)playerTypeInt;
                    if(playerType!=Player_type_normal && playerType!=Player_type_premium && playerType!=Player_type_gm && playerType!=Player_type_dev)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("playerType have wrong value: %1, at main ident: %2, directionAndPlayerType: %3").arg(playerTypeInt).arg(mainCodeType).arg(directionAndPlayerType));
                        return;
                    }
                    public_informations.type=playerType;

                    //the speed
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> public_informations.speed;

                    //the clan
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> public_informations.clan;

                    //the pseudo
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    quint8 pseudoSize;
                    in >> pseudoSize;
                    if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                    public_informations.pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                    in.device()->seek(in.device()->pos()+rawText.size());
                    if(public_informations.pseudo.isEmpty())
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed for pseudo: %1, rawData: %2").arg(mainCodeType).arg(QString(rawText.toHex())));
                        return;
                    }

                    //the skin
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                return;
            }
            //move the player
            quint8 directionInt,moveListSize;
            quint16 playerSizeList;
            if(max_player<=255)
            {
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    in >> playerId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("move size == 0 with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    in >> playerId;
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    QList<QPair<quint8,Direction> > movement;
                    QPair<quint8,Direction> new_movement;
                    in >> moveListSize;
                    int index_sub_loop=0;
                    if(moveListSize==0)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("move size == 0 with main ident at move player: %1").arg(mainCodeType));
                        return;
                    }
                    while(index_sub_loop<moveListSize)
                    {
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at move player: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1").arg(mainCodeType));
                    return;
                }
                in >> playerSizeList;
                quint16 playerId;

                int index=0;
                while(index<playerSizeList)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident at remove player: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> simplifiedId;
                }

                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;

                //direction and player type
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                    return;
                }
                quint8 directionInt;
                in >> directionInt;
                if(directionInt<1 || directionInt>8)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2").arg(directionInt).arg(mainCodeType));
                    return;
                }
                Direction direction=(Direction)directionInt;

                emit reinsert_player(simplifiedId,x,y,direction);
                index_sub_loop++;
            }
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                return;
            }

        }
        break;
        //Reinser player on other map
        case 0xC6:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> mapId;
                }
                quint16 playerSizeList;
                if(max_player<=255)
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                            return;
                        }
                        in >> simplifiedId;
                    }

                    //x and y
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    quint8 x,y;
                    in >> x;
                    in >> y;

                    //direction and player type
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    quint8 directionInt;
                    in >> directionInt;
                    if(directionInt<1 || directionInt>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("direction have wrong value: %1, at main ident: %2").arg(directionInt).arg(mainCodeType));
                        return;
                    }
                    Direction direction=(Direction)directionInt;

                    emit reinsert_player(simplifiedId,mapId,x,y,direction);
                    index_sub_loop++;
                }
                index++;
            }
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                return;
            }

        }
        break;
        //Insert plant on map
        case 0xD1:
        {
            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
            {
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                    return;
                }
                quint8 x,y;
                in >> x;
                in >> y;
                //plant
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                    return;
                }
                quint8 plant;
                in >> plant;
                //seconds to mature
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
                        return;
                    }
                    in >> mapId;
                }
                //x and y
                if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8)*2)
                {
                    parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1").arg(mainCodeType));
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

void Api_protocol::parseMessage(const quint8 &mainCodeType,const quint16 &subCodeType,const QByteArray &data)
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
                {
                    if((in.device()->size()-in.device()->pos())<(int)(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    quint8 fileNameSize;
                    in >> fileNameSize;
                    if((in.device()->size()-in.device()->pos())<(int)fileNameSize)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    QByteArray rawText=data.mid(in.device()->pos(),fileNameSize);
                    in.device()->seek(in.device()->pos()+rawText.size());
                    QString fileName=QString::fromUtf8(rawText.data(),rawText.size());
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint64)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    quint64 mtime;
                    in >> mtime;
                    QDateTime date;
                    date.setTime_t(mtime);
                    DebugClass::debugConsole(QString("write the file: %1, with date: %2").arg(fileName).arg(date.toString("dd.MM.yyyy hh:mm:ss.zzz")));
                    QByteArray dataFile=data.right(data.size()-in.device()->pos());
                    emit newFile(fileName,dataFile,mtime);
                    return;//no remaining data, because all remaing is used as file data
                }
                break;
                //chat as input
                case 0x0005:
                {
                    DebugClass::debugConsole("chat as input");
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    quint8 chat_type_int;
                    in >> chat_type_int;
                    if(chat_type_int<1 || chat_type_int>8)
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong chat type with main ident: %1, subCodeType: %2, chat_type_int: %3").arg(mainCodeType).arg(subCodeType).arg(chat_type_int));
                        return;
                    }
                    Chat_type chat_type=(Chat_type)chat_type_int;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    QString text;
                    in >> text;
                    DebugClass::debugConsole("chat as input: "+text);
                    if(chat_type==Chat_type_system || chat_type==Chat_type_system_important)
                        emit new_system_text(chat_type,text);
                    else
                    {
                        quint8 pseudoSize;
                        in >> pseudoSize;
                        if((in.device()->size()-in.device()->pos())<(int)pseudoSize)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, pseudoSize: %3, data: %4")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(pseudoSize)
                                          .arg(QString(data.mid(in.device()->pos()).toHex()))
                                          );
                            return;
                        }
                        QByteArray rawText=data.mid(in.device()->pos(),pseudoSize);
                        QString pseudo=QString::fromUtf8(rawText.data(),rawText.size());
                        in.device()->seek(in.device()->pos()+rawText.size());
                        if(pseudo.isEmpty())
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("UTF8 decoding failed: mainCodeType: %1, subCodeType: %2, rawText.data(): %3, rawText.size(): %4")
                                          .arg(mainCodeType)
                                          .arg(subCodeType)
                                          .arg(QString(rawText.toHex()))
                                          .arg(rawText.size())
                                          );
                            return;
                        }
                        quint8 player_type_int;
                        if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                            return;
                        }
                        in >> player_type_int;
                        Player_type player_type=(Player_type)player_type_int;
                        if(player_type!=Player_type_normal && player_type!=Player_type_premium && player_type!=Player_type_gm && player_type!=Player_type_dev)
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong player type with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType).arg(player_type_int));
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    quint8 code;
                    in >> code;
                    if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong string for reason with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        return;
                    }
                    QString text;
                    in >> text;
                    switch(code)
                    {
                        case 0x01:
                            parseError(tr("Disconnected by the server"),QString("You have been kicked by the server with the reason: %1").arg(text));
                        return;
                        case 0x02:
                            parseError(tr("Disconnected by the server"),QString("You have been ban by the server with the reason: %1").arg(text));
                        return;
                        case 0x03:
                            parseError(tr("Disconnected by the server"),QString("You have been disconnected by the server with the reason: %1").arg(text));
                        return;
                        default:
                            parseError(tr("Disconnected by the server"),QString("You have been disconnected by strange way by the server with the reason: %1").arg(text));
                        return;
                    }
                }
                break;
                default:
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                return;
            }
        }
        break;
        case 0xD0:
        {
            switch(subCodeType)
            {
                //random seeds as input
                case 0x0001:
                {
                    if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2; for the id").arg(mainCodeType).arg(subCodeType));
                            return;
                        }
                        in >> id;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, subCodeType: %2, for the quantity").arg(mainCodeType).arg(subCodeType));
                            return;
                        }
                        in >> quantity;
                        items[id]=quantity;
                        index++;
                    }
                    emit have_inventory(items);
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
                parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType main code: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                return;
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

void Api_protocol::parseQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(tr("Procotol wrong or corrupted"),QString("have not query of this type, mainCodeType: %1, subCodeType: %2, queryNumber: %3").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
}

//send reply
void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
    Q_UNUSED(data);
    parseError(tr("Procotol wrong or corrupted"),QString("have not reply of this type, mainCodeType: %1, queryNumber: %2").arg(mainCodeType).arg(queryNumber));
}

void Api_protocol::parseReplyData(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,const QByteArray &data)
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
                        emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                    {
                        have_send_protocol=true;
                        emit protocol_is_good();
                        //DebugClass::debugConsole("the protocol is good");
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                    {
                        if(!checkStringIntegrity(data.right(data.size()-in.device()->pos())))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong text with main ident: %1").arg(mainCodeType));
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
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                            return;
                        }
                        in >> max_player;
                        setMaxPlayers(max_player);
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                            return;
                        }
                        else if(max_player<=255)
                        {
                            quint8 tempSize;
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint8))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                                return;
                            }
                            in >> tempSize;
                            player_informations.public_informations.simplifiedId=tempSize;
                        }
                        else
                        {
                            if((in.device()->size()-in.device()->pos())<(int)sizeof(quint16))
                            {
                                parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                                return;
                            }
                            in >> player_informations.public_informations.simplifiedId;
                        }
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                            return;
                        }
                        in >> player_informations.cash;
                        if((in.device()->size()-in.device()->pos())<(int)sizeof(quint32))
                        {
                            parseError(tr("Procotol wrong or corrupted"),QString("wrong size to get the player id"));
                            return;
                        }
                        in >> number_of_map;

                        is_logged=true;
                        emit logged();
                    }
                    else
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                default:
                    parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType code: %1, with mainCodeType: %2").arg(subCodeType).arg(mainCodeType));
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
                        emit newError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2, type: query_type_protocol").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    havePlantAction=false;
                    quint8 returnCode;
                    in >> returnCode;
                    if(returnCode==0x01)
                        emit seed_planted(true);
                    else if(returnCode==0x02)
                        emit seed_planted(false);
                    else
                    {
                        emit newError(tr("Procotol wrong or corrupted"),QString("bad return code: %1").arg(returnCode));
                        return;
                    }
                }
                break;
                //Collect mature plant
                case 0x0007:
                {
                    if((in.device()->size()-in.device()->pos())<(int)(sizeof(quint8)))
                    {
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                    havePlantAction=false;
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
                        parseError(tr("Procotol wrong or corrupted"),QString("wrong size with main ident: %1, and queryNumber: %2").arg(mainCodeType).arg(queryNumber));
                        return;
                    }
                }
                break;
                default:
                    parseError(tr("Procotol wrong or corrupted"),QString("unknow subCodeType code: %1, with mainCodeType: %2").arg(subCodeType).arg(mainCodeType));
                    return;
                break;
            }
        }
        break;
        default:
            parseError(tr("Procotol wrong or corrupted"),QString("unknow ident reply code: %1").arg(mainCodeType));
            return;
        break;
    }
    if((in.device()->size()-in.device()->pos())!=0)
    {
        parseError(tr("Procotol wrong or corrupted"),QString("error: remaining data: parseReplyData(%1,%2,%3)").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
        return;
    }
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
    output->packOutcommingQuery(0x02,0x0001,queryNumber(),outputData);
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
    QCryptographicHash hash(QCryptographicHash::Sha1);
    hash.addData(pass.toUtf8());
    outputData+=hash.result();
    output->packOutcommingQuery(0x02,0x0002,query_number,outputData);
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
    output->packOutcommingData(0x42,0x0003,outputData);
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
    output->packOutcommingData(0x42,0x003,outputData);
}

void Api_protocol::useSeed(const quint8 &plant_id)
{
    if(havePlantAction)
    {
        emit newError(tr("Internal problem"),QString("Is already in plant action"));
        return;
    }
    havePlantAction=true;
    QByteArray outputData;
    outputData[0]=plant_id;
    output->packOutcommingQuery(0x10,0x0006,queryNumber(),outputData);
    return;
}

void Api_protocol::collectMaturePlant()
{
    if(havePlantAction)
    {
        emit newError(tr("Internal problem"),QString("Is already in plant action"));
        return;
    }
    havePlantAction=true;
    output->packOutcommingQuery(0x10,0x0007,queryNumber(),QByteArray());
    return;
}

//to reset all
void Api_protocol::resetAll()
{
    //status for the query
    is_logged=false;
    have_send_protocol=false;
    max_player=65535;
    number_of_map=0;
    havePlantAction=false;

    //to send trame
    lastQueryNumber=1;
}

QString Api_protocol::get_datapack_base_name()
{
    return datapack_base_name;
}
