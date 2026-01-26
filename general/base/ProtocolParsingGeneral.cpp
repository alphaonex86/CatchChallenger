#include "ProtocolParsing.hpp"
#include "GeneralVariable.hpp"
#include "ProtocolParsingCheck.hpp"
#include <iostream>
#include <cstring>
//#include <openssl/sha.h>



using namespace CatchChallenger;

#if ! defined (ONLYMAPRENDER)
char ProtocolParsingBase::tempBigBufferForOutput[];
char ProtocolParsingBase::tempBigBufferForInput[];//to store the input buffer on linux READ() interface or with Qt
#ifndef DYNAMICPACKETFIXEDSIZE
uint8_t ProtocolParsing::packetFixedSize[];
#else
uint8_t ProtocolParsing::packetFixedSize8[];
uint8_t ProtocolParsing::packetFixedSize16[];
#endif

ProtocolParsing::ProtocolParsing()
{
    #ifdef DYNAMICPACKETFIXEDSIZE
    packetFixedSize=packetFixedSize16;
    #endif
}

/// \note Nomination is: function, direction
void ProtocolParsing::initialiseTheVariable(const InitialiseTheVariableType &initialiseTheVariableType)
{
    //test the sha224 lib
    /*{
        static const unsigned char ibuf[]={0xBA,0xD9,0x39,0xE0,0x62,0x4C,0x48,0x1B,0x6B,0x60,0x49,0x63,0x18,0x77,0x01,0xBA,0x0A,0x37,0x2C,0x15,0x4D,0xA4,0x0C,0x1D,0x82,0x8A,0xE8,0xF2};
        static const unsigned char requiredResult[]={0x1c,0x82,0x16,0x18,0xa8,0xaa,0xd1,0x00,0xf7,0x41,0xba,0xfc,0x84,0x0f,0xcd,0x61,0x3a,0x9d,0xee,0x51,0x84,0xe0,0x5e,0xfd,0x45,0x8c,0x8f,0x9d};
        unsigned char obuf[sizeof(requiredResult)];
        SHA224(ibuf,sizeof(ibuf),obuf);
        if(memcmp(requiredResult,obuf,sizeof(requiredResult))!=0)
        {
            std::cerr << "Sha224 lib don't return the correct result" << std::endl;
            abort();
        }
    }*/

    switch(initialiseTheVariableType)
    {
        case InitialiseTheVariableType::MasterServer:
        case InitialiseTheVariableType::LoginServer:
        case InitialiseTheVariableType::AllInOne:
        default:
            #ifdef DYNAMICPACKETFIXEDSIZE
            if(ProtocolParsingBase::packetFixedSize8[0x02]==2)
                return;
            uint8_t *packetFixedSize=packetFixedSize8;
            #else
            if(ProtocolParsingBase::packetFixedSize[0x02]==2)
                return;
            #endif

            memset(ProtocolParsingBase::tempBigBufferForOutput,0,sizeof(ProtocolParsingBase::tempBigBufferForOutput));
            #ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
            memset(CompressionProtocol::tempBigBufferForCompressedOutput,0,sizeof(CompressionProtocol::tempBigBufferForCompressedOutput));
            memset(CompressionProtocol::tempBigBufferForUncompressedInput,0,sizeof(CompressionProtocol::tempBigBufferForUncompressedInput));
            CompressionProtocol::compressionTypeServer=CompressionProtocol::CompressionType::Zstandard;
            #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
            CompressionProtocol::compressionTypeClient=CompressionProtocol::CompressionType::Zstandard;
            #endif
            #endif

            #ifdef DYNAMICPACKETFIXEDSIZE
            memset(packetFixedSize8,0xFF,sizeof(packetFixedSize8));
            #else
            memset(packetFixedSize,0xFF,sizeof(packetFixedSize));
            #endif

            //Value: 0xFF not found (blocked), 0xFE not fixed size, fixed size used to prevent DDOS attack before login (can't send 4G size packet before login and saturate the bandwith)
            packetFixedSize[0x02]=2;
            packetFixedSize[0x03]=0xFE;
            packetFixedSize[0x04]=1;
            packetFixedSize[0x05]=0xFE;
            packetFixedSize[0x06]=0xFE;
            packetFixedSize[0x07]=0;
            packetFixedSize[0x08]=1;
            packetFixedSize[0x09]=1+2;
            packetFixedSize[0x0A]=0xFE;
            packetFixedSize[0x0B]=0;
            packetFixedSize[0x0C]=0;
            packetFixedSize[0x0D]=2;
            packetFixedSize[0x0E]=1;//monster position
            packetFixedSize[0x0F]=0xFE;
            packetFixedSize[0x10]=0xFE;
            packetFixedSize[0x11]=2;
            packetFixedSize[0x12]=0xFE;
            packetFixedSize[0x13]=2+4;
            packetFixedSize[0x14]=0xFE;
            packetFixedSize[0x15]=0;
            packetFixedSize[0x16]=0;
            packetFixedSize[0x17]=0xFE;
            packetFixedSize[0x18]=0;
            packetFixedSize[0x19]=1;
            packetFixedSize[0x1A]=0;
            packetFixedSize[0x1B]=2;
            packetFixedSize[0x1C]=2;
            packetFixedSize[0x1D]=2;
            packetFixedSize[0x1E]=2;
            packetFixedSize[0x1F]=1;
            packetFixedSize[0x3E]=4;
            packetFixedSize[0x3F]=2;

            packetFixedSize[0x40]=0xFE;
            packetFixedSize[0x44]=0xFE;
            packetFixedSize[0x45]=0xFE;
            packetFixedSize[0x46]=0xFE;
            packetFixedSize[0x47]=0xFE;
            packetFixedSize[0x48]=0xFE;
            packetFixedSize[0x4D]=4;
            packetFixedSize[0x50]=0xFE;
            packetFixedSize[0x51]=0;
            packetFixedSize[0x52]=0xFE;
            packetFixedSize[0x53]=0xFE;
            packetFixedSize[0x54]=0xFE;
            packetFixedSize[0x55]=0xFE;
            packetFixedSize[0x56]=0xFE;
            packetFixedSize[0x57]=0xFE;
            packetFixedSize[0x58]=0xFE;
            packetFixedSize[0x59]=0;
            packetFixedSize[0x5A]=0;
            packetFixedSize[0x5B]=0;

            packetFixedSize[0x5C]=0xFE;
            packetFixedSize[0x5D]=0xFE;
            packetFixedSize[0x5E]=0;
            packetFixedSize[0x5F]=0xFE;
            packetFixedSize[0x60]=0xFE;
            packetFixedSize[0x61]=0xFE;
            packetFixedSize[0x62]=0xFE;
            packetFixedSize[0x63]=0xFE;
            packetFixedSize[0x64]=2;//default like is was more than 255 players
            packetFixedSize[0x65]=0;
            packetFixedSize[0x66]=0xFE;
            packetFixedSize[0x67]=0xFE;
            packetFixedSize[0x68]=0xFE;
            packetFixedSize[0x69]=0xFE;
            packetFixedSize[0x6A]=0;
            packetFixedSize[0x6B]=0xFE;
            packetFixedSize[0x6C]=1;

            packetFixedSize[0x75]=4+4;
            packetFixedSize[0x76]=0xFE;
            packetFixedSize[0x77]=0xFE;
            packetFixedSize[0x78]=0xFE;
            packetFixedSize[0x7F]=0xFE;

            packetFixedSize[0x80]=0xFE;
            packetFixedSize[0x81]=0xFE;
            packetFixedSize[0x82]=0xFE;
            packetFixedSize[0x83]=0xFE;
            packetFixedSize[0x84]=0;
            packetFixedSize[0x85]=2;
            packetFixedSize[0x86]=2;
            packetFixedSize[0x87]=0;
            packetFixedSize[0x88]=2+4+4;
            packetFixedSize[0x89]=2+4+4;
            packetFixedSize[0x8A]=2;
            packetFixedSize[0x8B]=2*2+2*4;
            packetFixedSize[0x8C]=2*2+2*4;
            packetFixedSize[0x92]=0xFE;
            packetFixedSize[0x93]=CATCHCHALLENGER_TOKENSIZE_CONNECTGAMESERVER;
            packetFixedSize[0xA0]=5;
            packetFixedSize[0xA1]=0xFE;
            packetFixedSize[0xA8]=CATCHCHALLENGER_SHA224HASH_SIZE+CATCHCHALLENGER_SHA224HASH_SIZE;
            packetFixedSize[0xA9]=CATCHCHALLENGER_SHA224HASH_SIZE+CATCHCHALLENGER_SHA224HASH_SIZE;
            packetFixedSize[0xAA]=0xFE;
            packetFixedSize[0xAB]=1+4;
            packetFixedSize[0xAC]=1+4+4;
            packetFixedSize[0xAD]=CATCHCHALLENGER_SHA224HASH_SIZE;
            packetFixedSize[0xB0]=0;
            packetFixedSize[0xB1]=0;
            packetFixedSize[0xB2]=0xFE;
            packetFixedSize[0xB8]=9;
            packetFixedSize[0xBD]=CATCHCHALLENGER_SHA224HASH_SIZE;
            packetFixedSize[0xBE]=1+4+4+4;
            packetFixedSize[0xBF]=0;
            packetFixedSize[0xC0]=1;
            packetFixedSize[0xC1]=1;

            packetFixedSize[0xDF]=0xFE;
            packetFixedSize[0xE0]=0xFE;
            packetFixedSize[0xE1]=0xFE;
            packetFixedSize[0xE2]=2;
            packetFixedSize[0xE3]=0;
            packetFixedSize[0xF8]=4+4;
            packetFixedSize[0xF9]=0;

            //define the size of the reply
            packetFixedSize[128+0x80]=0xFE;
            packetFixedSize[128+0x81]=0xFE;
            packetFixedSize[128+0x82]=0xFE;
            packetFixedSize[128+0x83]=1;
            packetFixedSize[128+0x84]=0xFE;
            packetFixedSize[128+0x85]=0xFE;
            packetFixedSize[128+0x86]=0xFE;
            packetFixedSize[128+0x87]=0xFE;
            packetFixedSize[128+0x88]=0xFE;
            packetFixedSize[128+0x89]=0xFE;
            packetFixedSize[128+0x8A]=0xFE;
            packetFixedSize[128+0x8B]=0xFE;
            packetFixedSize[128+0x8C]=0xFE;
            packetFixedSize[128+0x92]=0xFE;
            packetFixedSize[128+0x93]=0xFE;
            packetFixedSize[128+0xA0]=0xFE;
            packetFixedSize[128+0xA1]=0xFE;
            packetFixedSize[128+0xA8]=0xFE;
            packetFixedSize[128+0xA9]=0xFE;
            packetFixedSize[128+0xAA]=1+4;/*drop dynamic size to improve the packet size overhead*/
            packetFixedSize[128+0xAB]=1;
            packetFixedSize[128+0xAC]=0xFE;
            packetFixedSize[128+0xAD]=0x01;
            packetFixedSize[128+0xB0]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            packetFixedSize[128+0xB1]=5*4;
            packetFixedSize[128+0xB2]=0xFE;
            packetFixedSize[128+0xB8]=0xFE;
            packetFixedSize[128+0xBD]=0xFE;
            packetFixedSize[128+0xBE]=0xFE;
            packetFixedSize[128+0xBF]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            packetFixedSize[128+0xC0]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;
            packetFixedSize[128+0xC1]=CATCHCHALLENGER_SERVER_MAXIDBLOCK*4;

            packetFixedSize[128+0xDF]=1;
            packetFixedSize[128+0xE0]=1;
            packetFixedSize[128+0xE1]=0;
            packetFixedSize[128+0xE2]=0;
            packetFixedSize[128+0xE3]=0;
            packetFixedSize[128+0xF8]=0xFE;
            packetFixedSize[128+0xF9]=0;

            #ifdef DYNAMICPACKETFIXEDSIZE
            memcpy(packetFixedSize16,packetFixedSize8,sizeof(packetFixedSize8));
            packetFixedSize8[0x64]=1;
            packetFixedSize16[0x64]=2;
            #endif

        break;
    }
}

ProtocolParsingBase::ProtocolParsingBase(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        PacketModeTransmission packetModeTransmission
        #endif
        ) :
    #ifdef EPOLLCATCHCHALLENGERSERVER
        #ifdef SERVERSSL
            ProtocolParsing(),
        #else
            ProtocolParsing(),
        #endif
    #else
    ProtocolParsing(),
    #endif
    flags(0),
    // for data
    dataSize(0),
    //to parse the netwrok stream
    packetCode(0),
    queryNumber(0)
{
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //if(!connect(socket,&ConnectedSocket::readyRead,this,&ProtocolParsingInputOutput::parseIncommingData,Qt::QueuedConnection/*to virtual socket*/))
    //    messageParsingLayer(std::to_string(isClient)+std::stringLiteral(" ProtocolParsingInputOutput::ProtocolParsingInputOutput(): can't connect the object"));
    #endif
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(packetModeTransmission==PacketModeTransmission_Client)
        flags |= 0x10;
    #endif
    memset(outputQueryNumberToPacketCode,0x00,sizeof(outputQueryNumberToPacketCode));
}

void ProtocolParsing::setMaxPlayers(const uint16_t &maxPlayers)
{
    #ifndef DYNAMICPACKETFIXEDSIZE
    if(maxPlayers<=255)
        ProtocolParsingBase::packetFixedSize[0x64]=1;
    else
        //NO: this case do into initialiseTheVariable()
        //YES: reinitialise because the initialise already done, but this object can be reused
        ProtocolParsingBase::packetFixedSize[0x64]=2;
    #else
    if(maxPlayers<=255)
        packetFixedSize=packetFixedSize8;
    else
        packetFixedSize=packetFixedSize16;
    #endif
}

ProtocolParsingBase::~ProtocolParsingBase()
{
}

#ifndef EPOLLCATCHCHALLENGERSERVER
uint64_t ProtocolParsingInputOutput::getRXSize() const
{
    return RXSize;
}

uint64_t ProtocolParsingInputOutput::getTXSize() const
{
    return TXSize;
}
#endif

void ProtocolParsingBase::reset()
{
    //outputQueryNumberToPacketCode.clear();

    dataClear();
}

void ProtocolParsingBase::resetForReconnect()
{
    flags &= 0x18;
    // ProtocolParsingBase
    // for data
    dataSize=0;
    //to parse the netwrok stream
    packetCode=0;
    queryNumber=0;
    memset(outputQueryNumberToPacketCode,0x00,sizeof(outputQueryNumberToPacketCode));
}

void ProtocolParsingInputOutput::resetForReconnect()
{
    ProtocolParsingBase::resetForReconnect();

    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    parseIncommingDataCount=0;
    #endif
    //ProtocolParsingInputOutput
    #ifndef EPOLLCATCHCHALLENGERSERVER
    RXSize=0;
    TXSize=0;
    #endif
    memset(inputQueryNumberToPacketCode,0x00,sizeof(inputQueryNumberToPacketCode));
}

#ifdef DYNAMICPACKETFIXEDSIZE
void ProtocolParsingInputOutput::setMaxPlayers(const uint16_t &maxPlayers)
{
    ProtocolParsing::setMaxPlayers(maxPlayers);
    protocolParsingCheck->setMaxPlayers(maxPlayers);
}
#endif

ProtocolParsingInputOutput::ProtocolParsingInputOutput(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        const PacketModeTransmission &packetModeTransmission
        #endif
        ) :
    ProtocolParsingBase(
        #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        packetModeTransmission
        #endif
        ),
      #ifdef CATCHCHALLENGER_EXTRA_CHECK
      parseIncommingDataCount(0)
      #endif
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
        if(packetModeTransmission==PacketModeTransmission_Client)
            flags |= 0x10;
        if(packetModeTransmission==PacketModeTransmission_Client)
            protocolParsingCheck=new ProtocolParsingCheck(PacketModeTransmission_Server);
        else
            protocolParsingCheck=new ProtocolParsingCheck(PacketModeTransmission_Client);
    #else
        #error "Can't have both CATCHCHALLENGERSERVERDROPIFCLENT and CATCHCHALLENGER_EXTRA_CHECK enabled because ProtocolParsingCheck work as client"
    #endif
    #endif
    #ifndef EPOLLCATCHCHALLENGERSERVER
    RXSize=0;
    TXSize=0;
    #endif
    memset(inputQueryNumberToPacketCode,0x00,sizeof(inputQueryNumberToPacketCode));
}

ProtocolParsingInputOutput::~ProtocolParsingInputOutput()
{
    #ifdef CATCHCHALLENGER_EXTRA_CHECK
    if(protocolParsingCheck!=NULL)
    {
        delete protocolParsingCheck;
        protocolParsingCheck=NULL;
    }
    #endif
}

#ifndef EPOLLCATCHCHALLENGERSERVERNOCOMPRESSION
CompressionProtocol::CompressionType ProtocolParsingInputOutput::getCompressType() const
{
    #ifndef CATCHCHALLENGERSERVERDROPIFCLENT
    if(flags & 0x10)
        return CompressionProtocol::compressionTypeClient;
    else
    #endif
        return CompressionProtocol::compressionTypeServer;
}
#endif
#endif
