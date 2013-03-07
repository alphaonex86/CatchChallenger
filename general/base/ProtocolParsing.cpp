#include "ProtocolParsing.h"
#include "DebugClass.h"
#include "GeneralVariable.h"

using namespace CatchChallenger;

QSet<quint8>				ProtocolParsing::mainCodeWithoutSubCodeTypeClientToServer;//if need sub code or not
//if is a query
QSet<quint8>				ProtocolParsing::mainCode_IsQueryClientToServer;
quint8					ProtocolParsing::replyCodeClientToServer;
//predefined size
QHash<quint8,quint16>			ProtocolParsing::sizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketClientToServer;
QHash<quint8,quint16>			ProtocolParsing::replySizeOnlyMainCodePacketClientToServer;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketClientToServer;

QSet<quint8>				ProtocolParsing::mainCodeWithoutSubCodeTypeServerToClient;//if need sub code or not
//if is a query
QSet<quint8>				ProtocolParsing::mainCode_IsQueryServerToClient;
quint8					ProtocolParsing::replyCodeServerToClient;
//predefined size
QHash<quint8,quint16>			ProtocolParsing::sizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::sizeMultipleCodePacketServerToClient;
QHash<quint8,quint16>			ProtocolParsing::replySizeOnlyMainCodePacketServerToClient;
QHash<quint8,QHash<quint16,quint16> >	ProtocolParsing::replySizeMultipleCodePacketServerToClient;

QHash<quint8,QSet<quint16> > ProtocolParsing::compressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> > ProtocolParsing::compressionMultipleCodePacketServerToClient;
QHash<quint8,QSet<quint16> > ProtocolParsing::replyComressionMultipleCodePacketClientToServer;
QHash<quint8,QSet<quint16> > ProtocolParsing::replyComressionMultipleCodePacketServerToClient;
QSet<quint8> ProtocolParsing::replyComressionOnlyMainCodePacketClientToServer;
QSet<quint8> ProtocolParsing::replyComressionOnlyMainCodePacketServerToClient;

ProtocolParsing::ProtocolParsing(ConnectedSocket * socket)
{
    this->socket=socket;
    connect(socket,SIGNAL(disconnected()),this,SLOT(reset()),Qt::QueuedConnection);
}

void ProtocolParsing::initialiseTheVariable()
{
    //def query without the sub code
    mainCodeWithoutSubCodeTypeServerToClient << 0xC0 << 0xC1 << 0xC3 << 0xC4 << 0xC5 << 0xC6 << 0xC7 << 0xC8 << 0xD1 << 0xD2;
    mainCodeWithoutSubCodeTypeClientToServer << 0x40 << 0x41 << 0x61;

    //define the size of direct query
    {
        //default like is was more than 255 players
        sizeOnlyMainCodePacketServerToClient[0xC3]=2;
    }
    sizeOnlyMainCodePacketServerToClient[0xC4]=0;
    sizeOnlyMainCodePacketClientToServer[0x40]=2;
    sizeOnlyMainCodePacketClientToServer[0x61]=4;
    sizeMultipleCodePacketClientToServer[0x10][0x0006]=1;
    sizeMultipleCodePacketClientToServer[0x10][0x0007]=0;
    sizeMultipleCodePacketClientToServer[0x50][0x0002]=8;
    sizeMultipleCodePacketClientToServer[0x50][0x0004]=0;
    sizeMultipleCodePacketClientToServer[0x50][0x0005]=0;
    sizeMultipleCodePacketClientToServer[0x60][0x0002]=0;
    sizeMultipleCodePacketClientToServer[0x60][0x0003]=1;
    sizeMultipleCodePacketClientToServer[0x60][0x0004]=8;
    sizeMultipleCodePacketServerToClient[0xD0][0x0006]=0;
    sizeMultipleCodePacketServerToClient[0xD0][0x0007]=0;
    sizeMultipleCodePacketServerToClient[0xD0][0x0008]=0;
    //define the size of the reply
    /** \note previously send by: sizeMultipleCodePacketServerToClient */
    replySizeMultipleCodePacketClientToServer[0x79][0x0001]=0;
    /** \note previously send by: sizeMultipleCodePacketClientToServer */
    replySizeMultipleCodePacketServerToClient[0x10][0x0006]=1;
    replySizeMultipleCodePacketServerToClient[0x10][0x0007]=1;
    replySizeMultipleCodePacketServerToClient[0x80][0x0001]=1;

    compressionMultipleCodePacketClientToServer[0x02] << 0x000C;
    //define the compression of the reply
    /** \note previously send by: sizeMultipleCodePacketClientToServer */
    replyComressionMultipleCodePacketServerToClient[0x02] << 0x000C;
    replyComressionMultipleCodePacketServerToClient[0x02] << 0x0002;

    //main code for query with reply
    ProtocolParsing::mainCode_IsQueryClientToServer << 0x02 << 0x10 << 0x20 << 0x30;
    ProtocolParsing::mainCode_IsQueryServerToClient << 0x79 << 0x80 << 0x90 << 0xA0;

    //reply code
    ProtocolParsing::replyCodeServerToClient=0xC1;
    ProtocolParsing::replyCodeClientToServer=0x41;

    //register meta type
    qRegisterMetaType<CatchChallenger::PlayerMonster >("CatchChallenger::PlayerMonster");//for Api_protocol::tradeAddTradeMonster()
}

void ProtocolParsing::setMaxPlayers(const quint16 &maxPlayers)
{
    if(maxPlayers<=255)
    {
        ProtocolParsing::sizeOnlyMainCodePacketServerToClient[0xC3]=1;
    }
    else
    {
        //this case do into initialiseTheVariable()
    }
}

ProtocolParsingInput::ProtocolParsingInput(ConnectedSocket * socket,PacketModeTransmission packetModeTransmission) :
    ProtocolParsing(socket)
{
    canStartReadData=false;
    RXSize=0;
    if(!connect(socket,SIGNAL(readyRead()),this,SLOT(parseIncommingData())))
        DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::ProtocolParsingInput(): can't connect the object"));
    isClient=(packetModeTransmission==PacketModeTransmission_Client);
    dataClear();
}

bool ProtocolParsingInput::checkStringIntegrity(const QByteArray & data)
{
    if(data.size()<(int)sizeof(qint32))
    {
        emit error("header size not suffisient");
        return false;
    }
    qint32 stringSize;
    QDataStream in(data);
    in.setVersion(QDataStream::Qt_4_4);
    in >> stringSize;
    if(stringSize>65535)
    {
        emit error(QString("String size is wrong: %1").arg(stringSize));
        return false;
    }
    if(data.size()<stringSize)
    {
        emit error(QString("String size is greater than the data: %1>%2").arg(data.size()).arg(stringSize));
        return false;
    }
    return true;
}

quint64 ProtocolParsingInput::getRXSize()
{
    return RXSize;
}

ProtocolParsingOutput::ProtocolParsingOutput(ConnectedSocket * socket,PacketModeTransmission packetModeTransmission) :
    ProtocolParsing(socket)
{
    TXSize=0;
    isClient=(packetModeTransmission==PacketModeTransmission_Client);
}

void ProtocolParsingInput::parseIncommingData()
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): socket->bytesAvailable(): %1").arg(socket->bytesAvailable()));
    #endif
    if(!canStartReadData)
        return;

    //put this as general
    QDataStream in(socket);
    in.setVersion(QDataStream::Qt_4_4);

    while(socket->bytesAvailable()>0)
    {
        if(!haveData)
        {
            if(socket->bytesAvailable()<(int)sizeof(quint8))//ignore because first int is cuted!
                return;
            in >> mainCodeType;
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !haveData, mainCodeType: %1").arg(mainCodeType));
            #endif
            haveData=true;
            haveData_dataSize=false;
            have_subCodeType=false;
            have_query_number=false;
            is_reply=false;
            if(isClient)
                need_subCodeType=!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType);
            else
                need_subCodeType=!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType);
            need_query_number=false;
            data_size.clear();
        }

        if(!have_subCodeType)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !have_subCodeType"));
            #endif
            if(!need_subCodeType)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_subCodeType"));
                #endif
                //if is a reply
                if((isClient && mainCodeType==replyCodeServerToClient) || (!isClient && mainCodeType==replyCodeClientToServer))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true"));
                    #endif
                    is_reply=true;
                    need_query_number=true;
                    //the size with be resolved later
                }
                else
                {

                    if(isClient)
                    {
                        //if is query with reply
                        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                        {
                            #ifdef PROTOCOLPARSINGDEBUG
                            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply"));
                            #endif
                            need_query_number=true;
                        }

                        //check if have defined size
                        if(sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
                        {
                            //have fixed size
                            dataSize=sizeOnlyMainCodePacketServerToClient[mainCodeType];
                            haveData_dataSize=true;
                        }
                    }
                    else
                    {
                        //if is query with reply
                        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
                            need_query_number=true;

                        //check if have defined size
                        if(sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
                        {
                            //have fixed size
                            dataSize=sizeOnlyMainCodePacketClientToServer[mainCodeType];
                            haveData_dataSize=true;
                        }
                    }
                }
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_subCodeType"));
                #endif
                if(socket->bytesAvailable()<(int)sizeof(quint16))//ignore because first int is cuted!
                {
                    RXSize+=in.device()->pos();
                    return;
                }
                in >> subCodeType;

                if(isClient)
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): isClient"));
                    #endif
                    //if is query with reply
                    if(mainCode_IsQueryServerToClient.contains(mainCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryServerToClient)"));
                        #endif
                        need_query_number=true;
                    }

                    //check if have defined size
                    if(sizeMultipleCodePacketServerToClient.contains(mainCodeType))
                    {
                        if(sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                        {
                            //have fixed size
                            dataSize=sizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
                            haveData_dataSize=true;
                        }
                    }
                }
                else
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !isClient"));
                    #endif
                    //if is query with reply
                    if(mainCode_IsQueryClientToServer.contains(mainCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryClientToServer)"));
                        #endif
                        need_query_number=true;
                    }

                    //check if have defined size
                    if(sizeMultipleCodePacketClientToServer.contains(mainCodeType))
                    {
                        if(sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                        {
                            //have fixed size
                            dataSize=sizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
                            haveData_dataSize=true;
                        }
                    }
                }
            }

            //set this parsing step is done
            have_subCodeType=true;
        }
        #ifdef PROTOCOLPARSINGDEBUG
        else
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have_subCodeType"));
        #endif
        if(!have_query_number && need_query_number)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number"));
            #endif
            if(socket->bytesAvailable()<(int)sizeof(quint8))
            {
                RXSize+=in.device()->pos();//todo, write message: need more bytes
                return;
            }
            in >> queryNumber;

            // it's reply
            if(is_reply)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): it's reply, resolv size"));
                #endif
                if(replySize.contains(queryNumber))
                {
                    dataSize=replySize[queryNumber];
                    haveData_dataSize=true;
                }
            }
            else // it's query with reply
            {
                //size resolved before, into subCodeType step
            }

            //set this parsing step is done
            have_query_number=true;
        }
        #ifdef PROTOCOLPARSINGDEBUG
        else
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not need_query_number"));
        #endif
        if(!haveData_dataSize)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !haveData_dataSize"));
            #endif
            //temp data
            quint8 temp_size_8Bits;
            quint16 temp_size_16Bits;
            quint32 temp_size_32Bits;
            while(!haveData_dataSize)
            {
                switch(data_size.size())
                {
                    case 0:
                    {
                        if(socket->bytesAvailable()<(int)sizeof(quint8))
                        {
                            RXSize+=in.device()->pos();
                            return;
                        }
                        data_size+=socket->read(sizeof(quint8));
                        QDataStream in_size(data_size);
                        in_size.setVersion(QDataStream::Qt_4_4);
                        in_size >> temp_size_8Bits;
                        if(temp_size_8Bits!=0x00)
                        {
                            dataSize=temp_size_8Bits;
                            haveData_dataSize=true;
                            #ifdef PROTOCOLPARSINGDEBUG
                            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have 8Bits data size"));
                            #endif
                        }
                        else
                        {
                            #ifdef PROTOCOLPARSINGDEBUG
                            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have not 8Bits data size: %1, temp_size_8Bits: %2").arg(QString(data_size.toHex())).arg(temp_size_8Bits));
                            #endif
                            if(data_size.size()==0)
                            {
                                RXSize+=in.device()->pos();
                                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): internal infinity packet read prevent"));
                                return;
                            }
                        }
                    }
                    break;
                    case sizeof(quint8):
                    {
                        if(socket->bytesAvailable()<(int)sizeof(quint16))
                        {
                            RXSize+=in.device()->pos();
                            return;
                        }
                        data_size+=socket->read(sizeof(quint8));
                        {
                            QDataStream in_size(data_size);
                            in_size.setVersion(QDataStream::Qt_4_4);
                            in_size >> temp_size_16Bits;
                        }
                        if(temp_size_16Bits!=0x0000)
                        {
                            data_size+=socket->read(sizeof(quint8));
                            QDataStream in_size(data_size);
                            in_size.setVersion(QDataStream::Qt_4_4);
                            //in_size.device()->seek(sizeof(quint8)); or in_size >> temp_size_8Bits;, not both
                            in_size >> temp_size_8Bits;
                            in_size >> temp_size_16Bits;
                            dataSize=temp_size_16Bits;
                            haveData_dataSize=true;
                            #ifdef PROTOCOLPARSINGDEBUG
                            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have 16Bits data size: %1, temp_size_16Bits: %2").arg(QString(data_size.toHex())).arg(dataSize));
                            #endif
                        }
                        else
                        {
                            #ifdef PROTOCOLPARSINGDEBUG
                            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): have not 16Bits data size"));
                            #endif
                            if(data_size.size()==sizeof(quint8))
                            {
                                RXSize+=in.device()->pos();
                                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): internal infinity packet read prevent"));
                                return;
                            }
                        }
                    }
                    break;
                    case sizeof(quint16):
                    {
                        if(socket->bytesAvailable()<(int)sizeof(quint32))
                        {
                            RXSize+=in.device()->pos();
                            return;
                        }
                        data_size+=socket->read(sizeof(quint32));
                        QDataStream in_size(data_size);
                        in_size.setVersion(QDataStream::Qt_4_4);
                        in_size >> temp_size_16Bits;
                        in_size >> temp_size_32Bits;
                        if(temp_size_32Bits!=0x00000000)
                        {
                            dataSize=temp_size_32Bits;
                            haveData_dataSize=true;
                        }
                        else
                        {
                            emit error("size is null");
                            return;
                        }
                    }
                    break;
                    default:
                    emit error(QString("size not understand, internal bug: %1").arg(data_size.size()));
                    return;
                }
            }
        }
        #ifdef PROTOCOLPARSINGDEBUG
        else
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): haveData_dataSize"));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!haveData_dataSize)
        {
            emit error("have not the size here!");
            return;
        }
        #endif
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): header informations is ready, dataSize: %1").arg(dataSize));
        #endif
        if(dataSize>16*1024*1024)
        {
            emit error("packet size too big");
            return;
        }
        RXSize+=in.device()->pos();
        if(dataSize>0)
        {
            do
            {
                //if have too many data, or just the size
                if((dataSize-data.size())<=socket->bytesAvailable())
                {
                    RXSize+=dataSize-data.size();
                    data.append(socket->read(dataSize-data.size()));
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): remaining data: %1").arg(socket->bytesAvailable()));
                    #endif
                }
                else //if need more data
                {
                    if(socket->bytesAvailable()<CATCHCHALLENGER_MIN_PACKET_SIZE && (dataSize-data.size())>CATCHCHALLENGER_MIN_PACKET_SIZE)
                    {
                        if(!need_query_number)
                        {
                            if(!need_subCodeType)
                                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data corruption: !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
                            else
                                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data corruption: !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                        }
                        else
                        {
                            if(!is_reply)
                            {
                                if(!need_subCodeType)
                                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data corruption: need_query_number && !is_reply, mainCodeType: %1").arg(mainCodeType));
                                else
                                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data corruption: need_query_number && !is_reply, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                            }
                            else
                                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data corruption: need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
                        }
                        dataClear();
                        return;
                    }
                    RXSize+=socket->bytesAvailable();
                    data.append(socket->readAll());
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need more to recompose: %1").arg(dataSize-data.size()));
                    #endif
                }
            } while(
                //need more data
                (dataSize-data.size())>0 &&
                //and have more data
                socket->bytesAvailable()>0
                );

            if((dataSize-data.size())>0)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                if(!need_subCodeType)
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not suffisent data: %1, wait more tcp packet").arg(mainCodeType));
                else
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): not suffisent data: %1,%2, wait more tcp packet").arg(mainCodeType).arg(subCodeType));
                #endif

                return;
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): no need data"));
            #endif
        }
        #ifdef PROTOCOLPARSINGDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data.size(): %1").arg(data.size()));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(dataSize!=(quint32)data.size())
        {
            emit error("wrong data size here");
            return;
        }
        #endif
        #ifdef PROTOCOLPARSINGINPUTDEBUG
        if(isClient)
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): parse message as client"));
        else
            DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): parse message as server"));
        #endif
        #ifdef PROTOCOLPARSINGINPUTDEBUG
        DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): data: %1").arg(QString(data.toHex())));
        #endif
        //message
        if(!need_query_number)
        {
            if(!need_subCodeType)
            {
                #ifdef PROTOCOLPARSINGINPUTDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
                #endif
                parseMessage(mainCodeType,data);
            }
            else
            {
                #ifdef PROTOCOLPARSINGINPUTDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                #endif
                if(isClient)
                {
                    if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                        if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                            data=qUncompress(data);
                }
                else
                {
                    if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                        if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                            data=qUncompress(data);
                }
                parseMessage(mainCodeType,subCodeType,data);
            }
        }
        else
        {
            //query
            if(!is_reply)
            {
                if(!need_subCodeType)
                {
                    #ifdef PROTOCOLPARSINGINPUTDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1").arg(mainCodeType));
                    #endif
                    emit newInputQuery(mainCodeType,queryNumber);
                    parseQuery(mainCodeType,queryNumber,data);
                }
                else
                {
                    #ifdef PROTOCOLPARSINGINPUTDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                    #endif
                    if(isClient)
                    {
                        if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                            if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                                data=qUncompress(data);
                    }
                    else
                    {
                        if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                            if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                                data=qUncompress(data);
                    }
                    emit newInputQuery(mainCodeType,subCodeType,queryNumber);
                    parseQuery(mainCodeType,subCodeType,queryNumber,data);
                }
            }
            else
            {
                //reply
                if(!reply_subCodeType.contains(queryNumber))
                {
                    if(!reply_mainCodeType.contains(queryNumber))
                    {
                        emit error("reply to a query not send");
                        return;
                    }
                    mainCodeType=reply_mainCodeType[queryNumber];
                    reply_mainCodeType.remove(queryNumber);
                    #ifdef PROTOCOLPARSINGINPUTDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
                    #endif
                    if(isClient)
                    {
                        if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                                data=qUncompress(data);
                    }
                    else
                    {
                        if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                                data=qUncompress(data);
                    }
                    parseReplyData(mainCodeType,queryNumber,data);
                }
                else
                {
                    mainCodeType=reply_mainCodeType[queryNumber];
                    subCodeType=reply_subCodeType[queryNumber];
                    reply_mainCodeType.remove(queryNumber);
                    reply_subCodeType.remove(queryNumber);
                    #ifdef PROTOCOLPARSINGINPUTDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
                    #endif
                    if(isClient)
                    {
                        if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                            if(replyComressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                                data=qUncompress(data);
                    }
                    else
                    {
                        if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                            if(replyComressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                                data=qUncompress(data);
                    }
                    parseReplyData(mainCodeType,subCodeType,queryNumber,data);
                }
            }
        }
        dataClear();
    }
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QString(" parseIncommingData(): finish parse the input"));
    #endif

}

void ProtocolParsingInput::reset()
{
    RXSize=0;
    replySize.clear();
    reply_mainCodeType.clear();
    reply_subCodeType.clear();

    dataClear();
}

void ProtocolParsingInput::dataClear()
{
    data.clear();
    dataSize=0;
    haveData=false;
}

void ProtocolParsingInput::newOutputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    if(reply_mainCodeType.contains(queryNumber))
    {
        emit error("Query with this query number already found");
        return;
    }
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            replySize[queryNumber]=replySizeOnlyMainCodePacketServerToClient[mainCodeType];
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            replySize[queryNumber]=replySizeOnlyMainCodePacketClientToServer[mainCodeType];
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType));
            #endif
        }
    }
    #ifdef PROTOCOLPARSINGINPUTDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
    #endif
    reply_mainCodeType[queryNumber]=mainCodeType;
}

void ProtocolParsingInput::newOutputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
    if(reply_mainCodeType.contains(queryNumber))
    {
        emit error("Query with this query number already found");
        return;
    }
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                replySize[queryNumber]=replySizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                replySize[queryNumber]=replySizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3 compression disabled because have fixed size").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            #endif
        }
    }
    #ifdef PROTOCOLPARSINGINPUTDEBUG
    DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::newOutputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
    #endif
    reply_mainCodeType[queryNumber]=mainCodeType;
    reply_subCodeType[queryNumber]=subCodeType;
}

bool ProtocolParsingOutput::postReplyData(const quint8 &queryNumber,QByteArray data)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!queryReceived.contains(queryNumber))
    {
        DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingInput::postReplyData(): try reply to queryNumber: %1, but this query is not into the list").arg(queryNumber));
        return false;
    }
    else
        queryReceived.remove(queryNumber);
    #endif

    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if(isClient)
        out << replyCodeClientToServer;
    else
        out << replyCodeServerToClient;
    out << queryNumber;

    if(!replySize.contains(queryNumber))
    {
        if(replyCompression.contains(queryNumber))
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" postReplyData(%1) is now compressed").arg(queryNumber));
            #endif
            data=qCompress(data,9);
        }
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(data.size()==0)
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" postReplyData(%1,{}) dropped because can be size==0 if not fixed size").arg(queryNumber));
            return false;
        }
        #endif
        block+=encodeSize(data.size());
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(replyCompression.contains(queryNumber))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" postReplyData(%1,{}) compression disabled because have fixed size").arg(queryNumber));
        }
        if(data.size()!=replySize[queryNumber])
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" postReplyData(%1,{}) dropped because can be size!=fixed size").arg(queryNumber));
            return false;
        }
        #endif
        replySize.remove(queryNumber);
    }

    return internalPackOutcommingData(block+data);
}

quint64 ProtocolParsingOutput::getTXSize()
{
    return TXSize;
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint8 &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryReceived.contains(queryNumber))
        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2) query with same id previously say").arg(mainCodeType).arg(queryNumber));
    queryReceived << queryNumber;
    #endif
    if(replySize.contains(queryNumber))
    {
        emit error("Query with this query number already found");
        return;
    }
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::newInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            replySize[queryNumber]=replySizeOnlyMainCodePacketClientToServer[mainCodeType];
        }
        else
        {
            if(replyComressionOnlyMainCodePacketClientToServer.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                replyCompression << queryNumber;
            }
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::newInputQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2) compression can't be enabled with fixed size").arg(mainCodeType).arg(queryNumber));
            #endif
            replySize[queryNumber]=replySizeOnlyMainCodePacketServerToClient[mainCodeType];
        }
        else
        {
            if(replyComressionOnlyMainCodePacketServerToClient.contains(mainCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                replyCompression << queryNumber;
            }
        }
    }
}

void ProtocolParsingOutput::newInputQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(queryReceived.contains(queryNumber))
        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) query with same id previously say").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
    queryReceived << queryNumber;
    #endif
    if(replySize.contains(queryNumber))
    {
        emit error("Query with this query number already found");
        return;
    }
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::newInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                replySize[queryNumber]=replySizeMultipleCodePacketClientToServer[mainCodeType][subCodeType];
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        replyCompression << queryNumber;
                    }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(replyComressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(replyComressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    replyCompression << queryNumber;
                }
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::newInputQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(replySizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression can't be enabled with fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                replySize[queryNumber]=replySizeMultipleCodePacketServerToClient[mainCodeType][subCodeType];
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                    if(replyComressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        replyCompression << queryNumber;
                    }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            if(replyComressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(replyComressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" newInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    replyCompression << queryNumber;
               }
        }
    }
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const quint16 &subCodeType,QByteArray data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return false;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return false;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else if(!sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression enabled").arg(mainCodeType).arg(subCodeType));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketClientToServer[mainCodeType][subCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingData(): mainCodeType: %1, subCodeType: %2, try send with sub code, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return false;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): mainCodeType: %1, subCodeType: %2, try send as normal data, but not registred as is").arg(mainCodeType).arg(subCodeType));
            return false;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else if(!sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType));
            if(data.size()!=sizeMultipleCodePacketServerToClient[mainCodeType][subCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
        }
    }

    return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingData(const quint8 &mainCodeType,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return false;
        }
        if(mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return false;
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketClientToServer[mainCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return false;
            }
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingData(): mainCodeType: %1, try send without sub code, but not registred as is").arg(mainCodeType));
            return false;
        }
        if(mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): mainCodeType: %1, try send as normal data, but not registred as is").arg(mainCodeType));
            return false;
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketServerToClient[mainCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingData(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return false;
            }
            #endif
        }
    }

    return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint8 &queryNumber,const QByteArray &data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << queryNumber;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return false;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return false;
        }
        #endif
        if(!sizeOnlyMainCodePacketClientToServer.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketClientToServer[mainCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return false;
            }
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(!mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send without sub code, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return false;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType));
            return false;
        }
        #endif
        if(!sizeOnlyMainCodePacketServerToClient.contains(mainCodeType))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()!=sizeOnlyMainCodePacketServerToClient[mainCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,{}) dropped because can be size!=fixed size").arg(mainCodeType));
                return false;
            }
            #endif
        }
    }

    emit newOutputQuery(mainCodeType,queryNumber);
    return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::packOutcommingQuery(const quint8 &mainCodeType,const quint16 &subCodeType,const quint8 &queryNumber,QByteArray data)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    out << mainCodeType;
    out << subCodeType;
    out << queryNumber;

    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return false;
        }
        if(!mainCode_IsQueryClientToServer.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return false;
        }
        #endif
        if(!sizeMultipleCodePacketClientToServer.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else if(!sizeMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
        {
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketClientToServer.contains(mainCodeType))
                if(compressionMultipleCodePacketClientToServer[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketClientToServer[mainCodeType][subCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
        }
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send with sub code, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return false;
        }
        if(!mainCode_IsQueryServerToClient.contains(mainCodeType))
        {
            DebugClass::debugConsole(QString::number(isClient)+QString(" ProtocolParsingOutput::packOutcommingQuery(): queryNumber: %1, mainCodeType: %2, subCodeType: %3, try send as query, but not registred as is").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
            return false;
        }
        #endif
        if(!sizeMultipleCodePacketServerToClient.contains(mainCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else if(!sizeMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
        {
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    data=qCompress(data,9);
                }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(data.size()==0)
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size==0 if not fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
            block+=encodeSize(data.size());
        }
        else
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(compressionMultipleCodePacketServerToClient.contains(mainCodeType))
                if(compressionMultipleCodePacketServerToClient[mainCodeType].contains(subCodeType))
                    DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,%3) compression can't be enabled due to fixed size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            if(data.size()!=sizeMultipleCodePacketServerToClient[mainCodeType][subCodeType])
            {
                DebugClass::debugConsole(QString::number(isClient)+QString(" packOutcommingQuery(%1,%2,{}) dropped because can be size!=fixed size").arg(mainCodeType).arg(subCodeType));
                return false;
            }
            #endif
        }
    }

    emit newOutputQuery(mainCodeType,subCodeType,queryNumber);
    //doit étre bloquer avant, car e main code devrai pas avoir de sub code
    return internalPackOutcommingData(block+data);
}

bool ProtocolParsingOutput::internalPackOutcommingData(const QByteArray &data)
{
    #ifdef PROTOCOLPARSINGDEBUG
    DebugClass::debugConsole("internalPackOutcommingData(): start");
    #endif
    #ifdef DEBUG_PROTOCOLPARSING_RAW_NETWORK
    emit message(QString("Sended packet size: %1: %2").arg(data.size()).arg(QString(data.toHex())));
    #endif // DEBUG_PROTOCOLPARSING_RAW_NETWORK
    TXSize+=data.size();
    byteWriten = socket->write(data);
    if(data.size()!=byteWriten)
    {
        DebugClass::debugConsole(QString("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        emit error(QString("All the bytes have not be written: %1, byteWriten: %2").arg(socket->errorString()).arg(byteWriten));
        return false;
    }
    return true;
}

QByteArray ProtocolParsingOutput::encodeSize(quint32 size)
{
    QByteArray block;
    QDataStream out(&block, QIODevice::WriteOnly);
    if(size<=0xFF)
        out << quint8(size);
    else if(size<=0xFFFF)
        out << quint8(0x00) << quint16(size);
    else
        out << quint16(0x0000) << quint32(size);
    return block;
}

void ProtocolParsingOutput::reset()
{
    TXSize=0;
    replySize.clear();
    replyCompression.clear();
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    queryReceived.clear();
    #endif
}
