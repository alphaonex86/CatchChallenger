#include "../base/Client.hpp"
#include <cstring>

using namespace CatchChallenger;

bool Client::sendInventory()
{
    if(sizeof(ProtocolParsingBase::tempBigBufferForOutput)<=(1+4+
                                                            (2+4)*public_and_private_informations.items.size()
                                                            ))
    {
        errorOutput("Too many items to be send");
        return false;
    }
    //send the network message
    uint32_t posOutput=0;
    ProtocolParsingBase::tempBigBufferForOutput[posOutput]=0x54;
    posOutput+=1+4;

    {const uint16_t _tmp_le=(htole16(public_and_private_informations.items.size()));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

    posOutput+=2;
    {
        std::map<CATCHCHALLENGER_TYPE_ITEM,CATCHCHALLENGER_TYPE_ITEM_QUANTITY>::iterator i=public_and_private_informations.items.begin();
        while(i!=public_and_private_informations.items.cend())
        {
            {const uint16_t _tmp_le=(htole16(i->first));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            posOutput+=2;
            {const uint32_t _tmp_le=(htole32(i->second));memcpy(ProtocolParsingBase::tempBigBufferForOutput+posOutput,&_tmp_le,sizeof(_tmp_le));}

            posOutput+=4;
            ++i;
        }
    }

    {const uint32_t _tmp_le=(htole32(posOutput-1-4));memcpy(ProtocolParsingBase::tempBigBufferForOutput+1,&_tmp_le,sizeof(_tmp_le));}//set the dynamic size
    return sendRawBlock(ProtocolParsingBase::tempBigBufferForOutput,posOutput);
}
