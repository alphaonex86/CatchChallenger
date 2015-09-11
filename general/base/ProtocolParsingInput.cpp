#include "ProtocolParsing.h"
#include "ProtocolParsingCheck.h"
#include "GeneralVariable.h"
#include <iostream>

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
#include <lzma.h>
#include "lz4/lz4.h"
#endif

using namespace CatchChallenger;

ssize_t ProtocolParsingInputOutput::read(char * data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(std::stringLiteral("Socket open in read only!"));
        disconnectClient();
        return false;
    }
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
        #ifdef EPOLLCATCHCHALLENGERSERVER
        const int &temp_size=epollSocket.read(data,size);
        #else
        const int &temp_size=socket->read(data,size);
        #endif
        RXSize+=temp_size;
        return temp_size;
    #else
        #ifdef EPOLLCATCHCHALLENGERSERVER
        return epollSocket.read(data,size);
        #else
        return socket->read(data,size);
        #endif
    #endif
}

ssize_t ProtocolParsingInputOutput::write(const char * const data, const size_t &size)
{
    #if defined (CATCHCHALLENGER_EXTRA_CHECK) && ! defined (EPOLLCATCHCHALLENGERSERVER)
    if(socket->openMode()|QIODevice::WriteOnly)
    {}
    else
    {
        messageParsingLayer(std::stringLiteral("Socket open in write only!"));
        disconnectClient();
        return false;
    }
    #endif
    #ifdef EPOLLCATCHCHALLENGERSERVER
    const ssize_t &byteWriten=epollSocket.write(data,size);
    #else
    const ssize_t &byteWriten=socket->write(data,size);
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    TXSize+=byteWriten;
    #endif
    if(Q_UNLIKELY((ssize_t)size!=byteWriten))
    {
        #ifdef EPOLLCATCHCHALLENGERSERVER
        messageParsingLayer("All the bytes have not be written byteWriten: "+std::to_string(byteWriten)+", size: "+std::to_string(size));
        #else
        messageParsingLayer("All the bytes have not be written: "+socket->errorString()+", byteWriten: "+std::to_string(byteWriten));
        #endif
    }
    else
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        {
            uint32_t cursor=0;
            do
            {
                if(!protocolParsingCheck->parseIncommingDataRaw(data,size,cursor))
                {
                    std::cerr << "Bug at data-sending: " << QString(QByteArray(data,size).toHex()).toStdString() << std::endl;
                    abort();
                }
                if(!protocolParsingCheck->valid)
                {
                    qDebug() << "Bug at data-sending not tigger the function";
                    abort();
                }
                protocolParsingCheck->valid=false;
            } while(cursor!=(uint32_t)size);
            /*if(cursor!=(uint32_t)size)
            {
                // Muliple concatened packet
                qDebug() << "Bug at data-sending cursor != size:" << cursor << "!=" << size;
                qDebug() << "raw write control bug:" << std::string(QByteArray(data,size).toHex());
                //abort();
            }*/
        }
        #endif
    }
    return byteWriten;
}

void ProtocolParsingInputOutput::closeSocket()
{
    #ifdef EPOLLCATCHCHALLENGERSERVER
    return epollSocket.close();
    #else
    if(socket!=NULL)
        return socket->disconnectFromHost();
    #endif
}

void ProtocolParsingInputOutput::parseIncommingData()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(parseIncommingDataCount>0)
    {
        qDebug() << "Multiple client on same section";
        abort();
    }
    parseIncommingDataCount++;
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
                std::stringLiteral(" parseIncommingData(): socket->bytesAvailable(): %1, header_cut: %2").arg(socket->bytesAvailable()).arg(header_cut.size()));
    #endif
    #endif

    while(1)
    {
        int32_t size;
        uint32_t cursor=0;
        if(!header_cut.isEmpty())
        {
            const unsigned int &size_to_get=CATCHCHALLENGER_COMMONBUFFERSIZE-header_cut.size();
            memcpy(ProtocolParsingInputOutput::commonBuffer,header_cut.constData(),header_cut.size());
            size=read(ProtocolParsingInputOutput::commonBuffer,size_to_get)+header_cut.size();
            if(size>0)
            {
                //QByteArray tempDataToDebug(ProtocolParsingInputOutput::commonBuffer+header_cut.size(),size-header_cut.size());
                //qDebug() << "with header cut" << header_cut << tempDataToDebug.toHex() << "and size" << size;
            }
            header_cut.clear();
        }
        else
        {
            size=read(ProtocolParsingInputOutput::commonBuffer,CATCHCHALLENGER_COMMONBUFFERSIZE);
            if(size>0)
            {
                //QByteArray tempDataToDebug(ProtocolParsingInputOutput::commonBuffer,size);
                //qDebug() << "without header cut" << tempDataToDebug.toHex() << "and size" << size;
            }
        }
        if(size<=0)
        {
            /*messageParsingLayer(
#ifndef CATCHCHALLENGERSERVERDROPIFCLENT
std::to_string(isClient)+
#endif
std::stringLiteral(" parseIncommingData(): size returned is 0!"));*/
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            parseIncommingDataCount--;
            #endif
            return;
        }

        do
        {
            if(!parseIncommingDataRaw(ProtocolParsingInputOutput::commonBuffer,size,cursor))
                break;
        } while(cursor<(uint32_t)size);

        if(size<CATCHCHALLENGER_COMMONBUFFERSIZE)
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            parseIncommingDataCount--;
            #endif
            return;
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
    std::stringLiteral(" parseIncommingData(): finish parse the input"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount--;
    #endif
}

bool ProtocolParsingBase::parseIncommingDataRaw(const char * const commonBuffer, const uint32_t &size, uint32_t &cursor)
{
    if(!parseHeader(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        qDebug() << "Break due to need more in header";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseQueryNumber(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        qDebug() << "Break due to need more in query number";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseDataSize(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //qDebug() << "Break due to need more in parse data size";
        #endif
        return false;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    if(!parseData(commonBuffer,size,cursor))
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        //qDebug() << "Break due to need more in parse data";
        #endif
        return false;
    }
    //parseDispatch(); do into above function
    dataClear();
    return true;
}

bool ProtocolParsingBase::parseHeader(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(!haveData)
    {
        if((size-cursor)<sizeof(uint8_t))//ignore because first int is cuted!
            return false;
        packetCode=*(commonBuffer+cursor);
        cursor+=sizeof(uint8_t);
        //def query without the sub code
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(isClient)
        {
            if(mainCodeWithoutSubCodeTypeServerToClient.find(packetCode)==mainCodeWithoutSubCodeTypeServerToClient.cend() && toDebugValidMainCodeServerToClient.find(packetCode)==toDebugValidMainCodeServerToClient.cend() && sizeMultipleCodePacketServerToClient.find(packetCode)==sizeMultipleCodePacketServerToClient.cend())
            {
                //errorParsingLayer("Critical bug, mainCodeType not valid");//flood when web browser try connect on it
                return false;
            }
        }
        else
        #endif
        {
            if(mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)==mainCodeWithoutSubCodeTypeClientToServer.cend() && toDebugValidMainCodeClientToServer.find(packetCode)==toDebugValidMainCodeClientToServer.cend() && sizeMultipleCodePacketClientToServer.find(packetCode)==sizeMultipleCodePacketClientToServer.cend())
            {
                //errorParsingLayer("Critical bug, mainCodeType not valid");//flood when web browser try connect on it
                return false;
            }
        }
        if(cursor==0)
        {
            qDebug() << "Critical bug";
            abort();
        }
        #endif
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): !haveData, mainCodeType: %1").arg(mainCodeType));
        #endif
        haveData=true;
        haveData_dataSize=false;
        have_subCodeType=false;
        have_query_number=false;
        is_reply=false;
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(isClient)
            need_subCodeType=mainCodeWithoutSubCodeTypeServerToClient.find(packetCode)==mainCodeWithoutSubCodeTypeServerToClient.cend();
        else
        #endif
            need_subCodeType=mainCodeWithoutSubCodeTypeClientToServer.find(packetCode)==mainCodeWithoutSubCodeTypeClientToServer.cend();
        need_query_number=false;
        /// \todo remplace by char*
        data_size_size=0;
    }
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif

    if(!have_subCodeType)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): !have_subCodeType"));
        #endif
        if(!need_subCodeType)
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" parseIncommingData(): !need_subCodeType"));
            #endif
            //if is a reply
            if(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    (isClient && packetCode==replyCodeServerToClient) || (!isClient && packetCode==replyCodeClientToServer)
                    #else
                    mainCodeType==replyCodeClientToServer
                    #endif
            )
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): need_query_number=true"));
                #endif
                is_reply=true;
                need_query_number=true;
                //the size with be resolved later
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
            else
            {
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    //if is query with reply
                    if(mainCode_IsQueryServerToClient.find(packetCode)!=mainCode_IsQueryServerToClient.cend())
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                        std::stringLiteral(" parseIncommingData(): need_query_number=true, query with reply"));
                        #endif
                        need_query_number=true;
                    }

                    //check if have defined size
                    if(sizeOnlyMainCodePacketServerToClient.find(packetCode)!=sizeOnlyMainCodePacketServerToClient.cend())
                    {
                        //have fixed size
                        dataSize=sizeOnlyMainCodePacketServerToClient.at(packetCode);
                        haveData_dataSize=true;
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(cursor==0 && !haveData)
                    {
                        qDebug() << "Critical bug";
                        abort();
                    }
                    #endif
                }
                else
                #endif
                {
                    //if is query with reply
                    if(mainCode_IsQueryClientToServer.find(packetCode)!=mainCode_IsQueryClientToServer.cend())
                        need_query_number=true;

                    //check if have defined size
                    if(sizeOnlyMainCodePacketClientToServer.find(packetCode)!=sizeOnlyMainCodePacketClientToServer.cend())
                    {
                        //have fixed size
                        dataSize=sizeOnlyMainCodePacketClientToServer.at(packetCode);
                        haveData_dataSize=true;
                    }
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(cursor==0 && !haveData)
                    {
                        qDebug() << "Critical bug";
                        abort();
                    }
                    #endif
                }
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" parseIncommingData(): need_subCodeType"));
            #endif
            subCodeType=commonBuffer[cursor];
            cursor+=sizeof(uint8_t);
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !haveData)
            {
                qDebug() << "Critical bug";
                abort();
            }
            #endif

            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            if(isClient)
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryServerToClient.find(packetCode)!=mainCode_IsQueryServerToClient.cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                    std::stringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryClientToServer)"));
                    #endif
                    need_query_number=true;
                }

                //check if have defined size
                if(sizeMultipleCodePacketServerToClient.find(packetCode)!=sizeMultipleCodePacketServerToClient.cend())
                {
                    if(sizeMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=sizeMultipleCodePacketServerToClient.at(packetCode).cend())
                    {
                        //have fixed size
                        dataSize=sizeMultipleCodePacketServerToClient.at(packetCode).at(subCodeType);
                        haveData_dataSize=true;
                    }
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
            else
            #endif
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): !isClient"));
                #endif
                //if is query with reply
                if(mainCode_IsQueryClientToServer.find(packetCode)!=mainCode_IsQueryClientToServer.cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                    std::stringLiteral(" parseIncommingData(): need_query_number=true, query with reply (mainCode_IsQueryServerToClient)"));
                    #endif
                    need_query_number=true;
                }

                //check if have defined size
                if(sizeMultipleCodePacketClientToServer.find(packetCode)!=sizeMultipleCodePacketClientToServer.cend())
                {
                    if(sizeMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=sizeMultipleCodePacketClientToServer.at(packetCode).cend())
                    {
                        //have fixed size
                        dataSize=sizeMultipleCodePacketClientToServer.at(packetCode).at(subCodeType);
                        haveData_dataSize=true;
                    }
                }
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(cursor==0 && !haveData)
                {
                    qDebug() << "Critical bug";
                    abort();
                }
                #endif
            }
        }

        //set this parsing step is done
        have_subCodeType=true;
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): have_subCodeType"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(cursor==0 && !haveData)
    {
        qDebug() << "Critical bug";
        abort();
    }
    #endif
    return true;
}

bool ProtocolParsingBase::parseQueryNumber(const char * const commonBuffer,const uint32_t &size,uint32_t &cursor)
{
    if(!have_query_number && need_query_number)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): need_query_number"));
        #endif
        if((size-cursor)<sizeof(uint8_t))
        {
            //todo, write message: need more bytes
            return false;
        }
        queryNumber=*(commonBuffer+cursor);
        cursor+=sizeof(uint8_t);

        // it's reply
        if(is_reply)
        {
            if(waitedReply_packetCode.find(queryNumber)!=waitedReply_packetCode.cend())
            {
                packetCode=waitedReply_packetCode.at(queryNumber);
                if(waitedReply_subCodeType.find(queryNumber)==waitedReply_subCodeType.cend())
                {
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    if(isClient)
                    {
                        if(replySizeOnlyMainCodePacketServerToClient.find(packetCode)!=replySizeOnlyMainCodePacketServerToClient.cend())
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketServerToClient.find(packetCode)!=replyComressionOnlyMainCodePacketServerToClient.cend())
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            #endif
                            dataSize=replySizeOnlyMainCodePacketServerToClient.at(packetCode);
                            haveData_dataSize=true;
                        }
                    }
                    else
                    #endif
                    {
                        if(replySizeOnlyMainCodePacketClientToServer.find(packetCode)!=replySizeOnlyMainCodePacketClientToServer.cend())
                        {
                            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                            #ifdef CATCHCHALLENGER_EXTRA_CHECK
                            if(replyComressionOnlyMainCodePacketClientToServer.find(packetCode)!=replyComressionOnlyMainCodePacketClientToServer.cend())
                            {
                                errorParsingLayer("Can't be compressed and fixed size");
                                return false;
                            }
                            #endif
                            #endif
                            dataSize=replySizeOnlyMainCodePacketClientToServer.at(packetCode);
                            haveData_dataSize=true;
                        }
                    }
                }
                else
                {
                    subCodeType=waitedReply_subCodeType.at(queryNumber);
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    if(isClient)
                    {
                        if(replySizeMultipleCodePacketServerToClient.find(packetCode)!=replySizeMultipleCodePacketServerToClient.cend())
                        {
                            if(replySizeMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=replySizeMultipleCodePacketServerToClient.at(packetCode).cend())
                            {
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketServerToClient.find(packetCode)!=replyComressionMultipleCodePacketServerToClient.cend())
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                #endif
                                dataSize=replySizeMultipleCodePacketServerToClient.at(packetCode).at(subCodeType);
                                haveData_dataSize=true;
                            }
                        }
                    }
                    else
                    #endif
                    {
                        if(replySizeMultipleCodePacketClientToServer.find(packetCode)!=replySizeMultipleCodePacketClientToServer.cend())
                        {
                            if(replySizeMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=replySizeMultipleCodePacketClientToServer.at(packetCode).cend())
                            {
                                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                                if(replyComressionMultipleCodePacketClientToServer.find(packetCode)!=replyComressionMultipleCodePacketClientToServer.cend())
                                {
                                    errorParsingLayer("Can't be compressed and fixed size");
                                    return false;
                                }
                                #endif
                                #endif
                                dataSize=replySizeMultipleCodePacketClientToServer.at(packetCode).at(subCodeType);
                                haveData_dataSize=true;
                            }
                        }
                    }
                }
            }
            else
            {
                errorParsingLayer("It's not reply to a query");
                return false;
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
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): not need_query_number"));
    #endif
    return true;
}

bool ProtocolParsingBase::parseDataSize(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(!haveData_dataSize)
    {
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): !haveData_dataSize"));
        #endif
        //temp data
        uint8_t temp_size_8Bits;
        uint16_t temp_size_16Bits;
        uint32_t temp_size_32Bits;
        if(!haveData_dataSize)
        {
            switch(data_size_size)
            {
                case 0:
                {
                    if((size-cursor)<sizeof(uint8_t))
                        return false;
                    temp_size_8Bits=*(commonBuffer+cursor);
                    data_size_size+=sizeof(uint8_t);
                    cursor+=sizeof(uint8_t);
                    if(temp_size_8Bits!=0xFF)
                    {
                        dataSize=temp_size_8Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                        std::stringLiteral(" parseIncommingData(): have 8Bits data size"));
                        #endif
                    }
                }
                case sizeof(uint8_t):
                {
                    if((size-cursor)<sizeof(uint16_t))
                    {
                        if((size-cursor)>0)
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    temp_size_8Bits=*(commonBuffer+cursor);
                    if(temp_size_8Bits!=0xFF)
                    {
                        temp_size_16Bits=le16toh(*(reinterpret_cast<const uint16_t *>(commonBuffer+cursor)));
                        cursor+=sizeof(uint16_t);

                        dataSize=temp_size_16Bits;
                        haveData_dataSize=true;
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                        std::stringLiteral(" parseIncommingData(): have 16Bits data size: %1, temp_size_16Bits: %2").arg(dataSize).arg(data_size_size));
                        #endif
                    }
                    else
                    {
                        data_size_size+=sizeof(uint8_t);
                        cursor+=sizeof(uint8_t);//2x 0xFF when 32Bits size
                    }
                }
                case sizeof(uint16_t):
                {
                    if((size-cursor)<sizeof(uint32_t))
                    {
                        if((size-cursor)>0)
                            header_cut.append(commonBuffer+cursor,(size-cursor));
                        return false;
                    }
                    temp_size_32Bits=le32toh(*reinterpret_cast<const uint32_t *>(commonBuffer+cursor));
                    cursor+=sizeof(uint32_t);

                    dataSize=temp_size_32Bits;
                    haveData_dataSize=true;
                }
                break;
                default:
                errorParsingLayer("size not understand, internal bug: "+std::to_string(data_size_size));
                return false;
            }
        }
    }
    #ifdef PROTOCOLPARSINGDEBUG
    else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): haveData_dataSize"));
    #endif
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(!haveData_dataSize)
    {
        errorParsingLayer("have not the size here!");
        return false;
    }
    #endif
    #ifdef PROTOCOLPARSINGDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
    std::stringLiteral(" parseIncommingData(): header informations is ready, dataSize: %1").arg(dataSize));
    #endif
    if(dataSize>16*1024*1024)
    {
        errorParsingLayer("packet size too big");
        return false;
    }
    return true;
}

bool ProtocolParsingBase::parseData(const char * const commonBuffer, const uint32_t &size,uint32_t &cursor)
{
    if(dataSize==0)
        return parseDispatch(NULL,0);
    if(dataToWithoutHeader.isEmpty())
    {
        //if have too many data, or just the size
        if(dataSize<=(size-cursor))
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(cursor==0 && !haveData)
            {
                qDebug() << "Critical bug";
                abort();
            }
            #endif
            const bool &returnVal=parseDispatch(commonBuffer+cursor,dataSize);
            cursor+=dataSize;
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" parseIncommingData(): remaining data: %1").arg((size-cursor)));
            #endif
            return returnVal;
        }
    }
    //if have too many data, or just the size
    if((dataSize-dataToWithoutHeader.size())<=(size-cursor))
    {
        const uint32_t &size_to_append=dataSize-dataToWithoutHeader.size();
        dataToWithoutHeader.append(commonBuffer+cursor,size_to_append);
        cursor+=size_to_append;
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): remaining data: %1, buffer data: %2").arg((size-cursor)).arg(std::string(QByteArray(commonBuffer,sizeof(commonBuffer)).toHex())));
        #endif
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(dataSize!=(uint32_t)dataToWithoutHeader.size())
        {
            errorParsingLayer("wrong data size here");
            return false;
        }
        #endif
        return parseDispatch(dataToWithoutHeader.constData(),dataToWithoutHeader.size());
    }
    else //if need more data
    {
        dataToWithoutHeader.append(commonBuffer+cursor,(size-cursor));
        #ifdef PROTOCOLPARSINGDEBUG
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): need more to recompose: %1").arg(dataSize-dataToWithoutHeader.size()));
        #endif
        cursor=size;
        return false;
    }
}

bool ProtocolParsingBase::parseDispatch(const char * const data, const int &size)
{
    #ifdef ProtocolParsingInputOutputDEBUG
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): parse message as client"));
    else
    #else
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        std::stringLiteral(" parseIncommingData(): parse message as server"));
    #endif
    #endif
    #ifdef ProtocolParsingInputOutputDEBUG
    messageParsingLayer(
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                std::to_string(isClient)+
                #endif
    std::stringLiteral(" parseIncommingData(): data: %1").arg(std::string(data.toHex())));
    #endif
    //message
    if(!need_query_number)
    {
        if(!need_subCodeType)
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" parseIncommingData(): !need_query_number && !need_subCodeType, mainCodeType: %1").arg(mainCodeType));
            #endif
            return parseMessage(packetCode,data,size);
        }
        else
        {
            #ifdef ProtocolParsingInputOutputDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" parseIncommingData(): !need_query_number && need_subCodeType, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
            #endif
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            if(isClient)
            {
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(compressionMultipleCodePacketServerToClient.find(packetCode)!=compressionMultipleCodePacketServerToClient.cend())
                    if(compressionMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketServerToClient.at(packetCode).cend())
                    {
                        switch(getCompressType())
                        {
                            case CompressionType::None:
                                parseFullMessage(packetCode,subCodeType,data,size);
                            break;
                            case CompressionType::Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Lz4:
                            {
                                QByteArray newData;
                                newData.resize(size*4);
                                const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                if(newSize<0)
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                newData.resize(newSize);
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                        }
                        return true;
                    }
                #endif
                parseFullMessage(packetCode,subCodeType,data,size);
                return true;
            }
            else
            #endif
            {
                #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                    parseFullMessage(mainCodeType,subCodeType,data,size);
                #else
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(compressionMultipleCodePacketClientToServer.find(packetCode)!=compressionMultipleCodePacketClientToServer.cend())
                    if(compressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketClientToServer.at(packetCode).cend())
                    {
                        switch(getCompressType())
                        {
                            case CompressionType::None:
                                parseFullMessage(packetCode,subCodeType,data,size);
                            break;
                            case CompressionType::Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Lz4:
                            {
                                QByteArray newData;
                                newData.resize(size*4);
                                const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                if(newSize<0)
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                newData.resize(newSize);
                                parseFullMessage(packetCode,subCodeType,newData.constData(),newData.size());
                            }
                            break;
                        }
                        return true;
                    }
                #endif
                parseFullMessage(packetCode,subCodeType,data,size);
                return true;
                #endif
            }
        }
    }
    else
    {
        //query
        if(!is_reply)
        {
            if(!need_subCodeType)
            {
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1").arg(mainCodeType));
                #endif
                storeInputQuery(packetCode,queryNumber);
                return parseQuery(packetCode,queryNumber,data,size);
            }
            else
            {
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): need_query_number && !is_reply, mainCodeType: %1, subCodeType: %2").arg(mainCodeType).arg(subCodeType));
                #endif
                storeFullInputQuery(packetCode,subCodeType,queryNumber);
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(compressionMultipleCodePacketServerToClient.find(packetCode)!=compressionMultipleCodePacketServerToClient.cend())
                        if(compressionMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketServerToClient.at(packetCode).cend())
                        {
                            switch(getCompressType())
                            {
                                case CompressionType::None:
                                    parseFullQuery(packetCode,subCodeType,queryNumber,data,size);
                                break;
                                case CompressionType::Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Lz4:
                                {
                                    QByteArray newData;
                                    newData.resize(size*4);
                                    const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                    if(newSize<0)
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    newData.resize(newSize);
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullQuery(packetCode,subCodeType,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        parseFullQuery(mainCodeType,subCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(compressionMultipleCodePacketClientToServer.find(packetCode)!=compressionMultipleCodePacketClientToServer.cend())
                        if(compressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=compressionMultipleCodePacketClientToServer.at(packetCode).cend())
                        {
                            switch(getCompressType())
                            {
                                case CompressionType::None:
                                    parseFullQuery(packetCode,subCodeType,queryNumber,data,size);
                                break;
                                case CompressionType::Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Lz4:
                                {
                                    QByteArray newData;
                                    newData.resize(size*4);
                                    const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                    if(newSize<0)
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    newData.resize(newSize);
                                    parseFullQuery(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullQuery(packetCode,subCodeType,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
        }
        else
        {
            //reply
            if(waitedReply_subCodeType.find(queryNumber)==waitedReply_subCodeType.cend())
            {
                waitedReply_packetCode.erase(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2").arg(queryNumber).arg(mainCodeType));
                #endif
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionOnlyMainCodePacketServerToClient.find(packetCode)!=replyComressionOnlyMainCodePacketServerToClient.cend())
                    {
                        switch(getCompressType())
                        {
                            case CompressionType::None:
                                return parseReplyData(packetCode,queryNumber,data,size);
                            break;
                            case CompressionType::Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Lz4:
                            {
                                QByteArray newData;
                                newData.resize(size*4);
                                const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                if(newSize<0)
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                newData.resize(newSize);
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                        }
                        return true;
                    }
                    #endif
                    return parseReplyData(packetCode,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        return parseReplyData(mainCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionOnlyMainCodePacketClientToServer.find(packetCode)!=replyComressionOnlyMainCodePacketClientToServer.cend())
                    {
                        switch(getCompressType())
                        {
                            case CompressionType::None:
                                return parseReplyData(packetCode,queryNumber,data,size);
                            break;
                            case CompressionType::Zlib:
                            default:
                            {
                                const QByteArray &newData=qUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Xz:
                            {
                                const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                if(newData.isEmpty())
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                            case CompressionType::Lz4:
                            {
                                QByteArray newData;
                                newData.resize(size*4);
                                const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                if(newSize<0)
                                {
                                    errorParsingLayer("Compressed data corrupted");
                                    return false;
                                }
                                newData.resize(newSize);
                                return parseReplyData(packetCode,queryNumber,newData.constData(),newData.size());
                            }
                            break;
                        }
                        return true;
                    }
                    #endif
                    return parseReplyData(packetCode,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
            else
            {
                waitedReply_packetCode.erase(queryNumber);
                waitedReply_subCodeType.erase(queryNumber);
                #ifdef ProtocolParsingInputOutputDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" parseIncommingData(): need_query_number && is_reply && reply_subCodeType.contains(queryNumber), queryNumber: %1, mainCodeType: %2, subCodeType: %3").arg(queryNumber).arg(mainCodeType).arg(subCodeType));
                #endif
                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                if(isClient)
                {
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionMultipleCodePacketServerToClient.find(packetCode)!=replyComressionMultipleCodePacketServerToClient.cend())
                        if(replyComressionMultipleCodePacketServerToClient.at(packetCode).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(packetCode).cend())
                        {
                            switch(getCompressType())
                            {
                                case CompressionType::None:
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,data,size);
                                break;
                                case CompressionType::Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Lz4:
                                {
                                    QByteArray newData;
                                    newData.resize(size*4);
                                    const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                    if(newSize<0)
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    newData.resize(newSize);
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullReplyData(packetCode,subCodeType,queryNumber,data,size);
                    return true;
                }
                else
                #endif
                {
                    #ifdef CATCHCHALLENGERSERVERBLOCKCLIENTTOSERVERPACKETDECOMPRESSION
                        parseFullReplyData(mainCodeType,subCodeType,queryNumber,data,size);
                    #else
                    #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                    if(replyComressionMultipleCodePacketClientToServer.find(packetCode)!=replyComressionMultipleCodePacketClientToServer.cend())
                        if(replyComressionMultipleCodePacketClientToServer.at(packetCode).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(packetCode).cend())
                        {
                            switch(getCompressType())
                            {
                                case CompressionType::None:
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,data,size);
                                break;
                                case CompressionType::Zlib:
                                default:
                                {
                                    const QByteArray &newData=qUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Xz:
                                {
                                    const QByteArray &newData=lzmaUncompress(QByteArray(data,size));
                                    if(newData.isEmpty())
                                    {
                                        errorParsingLayer("Compressed data is buggy");
                                        return false;
                                    }
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                                case CompressionType::Lz4:
                                {
                                    QByteArray newData;
                                    newData.resize(size*4);
                                    const int &newSize=LZ4_decompress_safe(data,newData.data(),size,newData.size());
                                    if(newSize<0)
                                    {
                                        errorParsingLayer("Compressed data corrupted");
                                        return false;
                                    }
                                    newData.resize(newSize);
                                    parseFullReplyData(packetCode,subCodeType,queryNumber,newData.constData(),newData.size());
                                }
                                break;
                            }
                            return true;
                        }
                    #endif
                    parseFullReplyData(packetCode,subCodeType,queryNumber,data,size);
                    return true;
                    #endif
                }
            }
        }
    }
    return true;
}

void ProtocolParsingBase::dataClear()
{
    dataToWithoutHeader.clear();
    dataSize=0;
    haveData=false;
}

#ifndef EPOLLCATCHCHALLENGERSERVER
std::vector<std::string> ProtocolParsingBase::getQueryRunningList()
{
    std::vector<std::string> returnedList;
    std::unordered_mapIterator<uint8_t,uint8_t> i(waitedReply_mainCodeType);
    while (i.hasNext()) {
        i.next();
        if(waitedReply_subCodeType.contains(i.key()))
            returnedList << std::stringLiteral("%1 %2").arg(i.value()).arg(waitedReply_subCodeType.value(i.key()));
        else
            returnedList << std::to_string(i.value());
    }
    return returnedList;
}
#endif

void ProtocolParsingBase::storeInputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(queryNumber);
}

void ProtocolParsingBase::storeFullInputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber)
{
    Q_UNUSED(mainCodeType);
    Q_UNUSED(subCodeType);
    Q_UNUSED(queryNumber);
}

void ProtocolParsingInputOutput::storeInputQuery(const uint8_t &mainCodeType,const uint8_t &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->waitedReply_packetCode[queryNumber]=mainCodeType;
    if(queryReceived.find(queryNumber)!=queryReceived.cend())
    {
        messageParsingLayer(
                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                    std::to_string(isClient)+
                    #endif
        " storeInputQuery("+std::to_string(mainCodeType)+","+std::to_string(queryNumber)+") query with same id previously say");
        return;
    }
    queryReceived.insert(queryNumber);
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)==mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::storeInputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+", try send without sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketServerToClient.find(mainCodeType)!=replySizeOnlyMainCodePacketServerToClient.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketServerToClient.find(mainCodeType)!=replyComressionOnlyMainCodePacketServerToClient.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                            " storeInputQuery("+std::to_string(mainCodeType)+
                            ","+std::to_string(queryNumber)+") compression can't be enabled with fixed size");
            #endif
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketServerToClient.at(mainCodeType);
        }
        else
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionOnlyMainCodePacketServerToClient.find(mainCodeType)!=replyComressionOnlyMainCodePacketServerToClient.cend())
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression.insert(queryNumber);
            }
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)==mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::storeInputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", try send without sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeOnlyMainCodePacketClientToServer.find(mainCodeType)!=replySizeOnlyMainCodePacketClientToServer.cend())
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(replyComressionOnlyMainCodePacketClientToServer.find(mainCodeType)!=replyComressionOnlyMainCodePacketClientToServer.cend())
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                            " storeInputQuery("+std::to_string(mainCodeType)+
                            ","+std::to_string(queryNumber)+
                            ") compression can't be enabled with fixed size");
            #endif
            #endif
            //register the size of the reply to send
            replyOutputSize[queryNumber]=replySizeOnlyMainCodePacketClientToServer.at(mainCodeType);
        }
        else
        {
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionOnlyMainCodePacketClientToServer.find(mainCodeType)!=replyComressionOnlyMainCodePacketClientToServer.cend())
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2) compression enabled").arg(mainCodeType).arg(queryNumber));
                #endif
                //register the compression of the reply to send
                replyOutputCompression.insert(queryNumber);
            }
            #endif
        }
    }
}

void ProtocolParsingInputOutput::storeFullInputQuery(const uint8_t &mainCodeType,const uint8_t &subCodeType,const uint8_t &queryNumber)
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    protocolParsingCheck->waitedReply_packetCode[queryNumber]=mainCodeType;
    protocolParsingCheck->waitedReply_subCodeType[queryNumber]=subCodeType;
    if(queryReceived.find(queryNumber)!=queryReceived.cend())
    {
        errorParsingLayer(
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            std::to_string(isClient)+
            #endif
            " storeInputQuery("+std::to_string(mainCodeType)+
            ","+std::to_string(subCodeType)+
            ","+std::to_string(queryNumber)+
            ") query with same id previously say");
        return;
    }
    queryReceived.insert(queryNumber);
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(isClient)
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeServerToClient.find(mainCodeType)!=mainCodeWithoutSubCodeTypeServerToClient.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::storeInputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", subCodeType: "+std::to_string(subCodeType)+
                        ", try send with sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeMultipleCodePacketClientToServer.find(mainCodeType)!=replySizeMultipleCodePacketClientToServer.cend())
        {
            if(replySizeMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=replySizeMultipleCodePacketClientToServer.at(mainCodeType).cend())
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketClientToServer.find(mainCodeType)!=replyComressionMultipleCodePacketClientToServer.cend())
                    if(replyComressionMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(mainCodeType).cend())
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                                    " storeInputQuery("+std::to_string(mainCodeType)+
                                    ","+std::to_string(subCodeType)+
                                    ","+std::to_string(queryNumber)+
                                    ") compression can't be enabled with fixed size");
                #endif
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketClientToServer.at(mainCodeType).at(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(replyComressionMultipleCodePacketClientToServer.find(mainCodeType)!=replyComressionMultipleCodePacketClientToServer.cend())
                    if(replyComressionMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(mainCodeType).cend())
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                        std::stringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression.insert(queryNumber);
                    }
                #endif
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionMultipleCodePacketClientToServer.find(mainCodeType)!=replyComressionMultipleCodePacketClientToServer.cend())
                if(replyComressionMultipleCodePacketClientToServer.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketClientToServer.at(mainCodeType).cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                    std::stringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression.insert(queryNumber);
                }
            #endif
        }
    }
    else
    #endif
    {
        #ifdef CATCHCHALLENGER_EXTRA_CHECK
        if(mainCodeWithoutSubCodeTypeClientToServer.find(mainCodeType)!=mainCodeWithoutSubCodeTypeClientToServer.cend())
        {
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
                        " ProtocolParsingInputOutput::storeInputQuery(): queryNumber: "+std::to_string(queryNumber)+
                        ", mainCodeType: "+std::to_string(mainCodeType)+
                        ", subCodeType: "+std::to_string(subCodeType)+
                        ", try send with sub code, but not registred as is");
            return;
        }
        #endif
        if(replySizeMultipleCodePacketServerToClient.find(mainCodeType)!=replySizeMultipleCodePacketServerToClient.cend())
        {
            if(replySizeMultipleCodePacketServerToClient.at(mainCodeType).find(subCodeType)!=replySizeMultipleCodePacketServerToClient.at(mainCodeType).cend())
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2,%3) fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                #ifdef CATCHCHALLENGER_EXTRA_CHECK
                if(replyComressionMultipleCodePacketServerToClient.find(mainCodeType)!=replyComressionMultipleCodePacketServerToClient.cend())
                    if(replyComressionMultipleCodePacketServerToClient.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(mainCodeType).cend())
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                                    " storeInputQuery("+std::to_string(mainCodeType)+
                                    ","+std::to_string(subCodeType)+
                                    ","+std::to_string(queryNumber)+
                                    ") compression can't be enabled with fixed size");
                #endif
                #endif
                replyOutputSize[queryNumber]=replySizeMultipleCodePacketServerToClient.at(mainCodeType).at(subCodeType);
            }
            else
            {
                #ifdef PROTOCOLPARSINGDEBUG
                messageParsingLayer(
                            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                            std::to_string(isClient)+
                            #endif
                std::stringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                #endif
                #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
                if(replyComressionMultipleCodePacketServerToClient.find(mainCodeType)!=replyComressionMultipleCodePacketServerToClient.cend())
                    if(replyComressionMultipleCodePacketServerToClient.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(mainCodeType).cend())
                    {
                        #ifdef PROTOCOLPARSINGDEBUG
                        messageParsingLayer(
                                    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                    std::to_string(isClient)+
                                    #endif
                        std::stringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                        #endif
                        //register the compression of the reply to send
                        replyOutputCompression.insert(queryNumber);
                    }
                #endif
            }
        }
        else
        {
            #ifdef PROTOCOLPARSINGDEBUG
            messageParsingLayer(
                        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                        std::to_string(isClient)+
                        #endif
            std::stringLiteral(" storeInputQuery(%1,%2,%3) 1) not fixed reply size").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
            #endif
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            if(replyComressionMultipleCodePacketServerToClient.find(mainCodeType)!=replyComressionMultipleCodePacketServerToClient.cend())
                if(replyComressionMultipleCodePacketServerToClient.at(mainCodeType).find(subCodeType)!=replyComressionMultipleCodePacketServerToClient.at(mainCodeType).cend())
                {
                    #ifdef PROTOCOLPARSINGDEBUG
                    messageParsingLayer(
                                #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
                                std::to_string(isClient)+
                                #endif
                    std::stringLiteral(" storeInputQuery(%1,%2,%3) compression enabled").arg(mainCodeType).arg(subCodeType).arg(queryNumber));
                    #endif
                    //register the compression of the reply to send
                    replyOutputCompression.insert(queryNumber);
               }
            #endif
        }
    }
}
