#include "Client.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include <cstring>

using namespace CatchChallenger;

//have query with reply
bool Client::parseQuery(const uint8_t &packetCode,const uint8_t &queryNumber,const char * const data,const unsigned int &size)
{
    if(stopIt)
        return false;
    const bool goodQueryBeforeLoginLoaded=
            //before be logged, accept:
            packetCode==0xA0 || //protocol header
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            packetCode==0xA8 || //login by login/pass
            packetCode==0xA9 || //Create account
            packetCode==0xAD //Stat client
            #else
            packetCode==0x93 //login by token + Select character (Get first data and send the login)
            #endif
            ;
    if(stat==ClientStat::None || goodQueryBeforeLoginLoaded)
        return parseInputBeforeLogin(packetCode,queryNumber,data,size);
    const bool goodQueryBeforeCharacterLoaded=
            //before character selected (but after login), accept:
            #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
            packetCode==0xAA || //Add character
            packetCode==0xAB || //Remove character
            packetCode==0xAC || //Select character
            #endif
            packetCode==0xA1;
    if(stat!=ClientStat::CharacterSelected && !goodQueryBeforeCharacterLoaded)
    {
        errorOutput("charaters is not logged, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+")");
        return false;
    }
    switch(packetCode)
    {
        //Usage of recipe
        case 0x85:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &recipe_id=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            useRecipe(queryNumber,recipe_id);
            return true;
        }
        break;
        //Use object
        case 0x86:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            useObject(queryNumber,objectId);
            return true;
        }
        break;
        //Get shop list
        case 0x87:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const SHOP_TYPE &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            getShopList(queryNumber,shopId);
            return true;
        }
        break;
        //Buy object
        case 0x88:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const SHOP_TYPE &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            buyObject(queryNumber,shopId,objectId,quantity,price);
            return true;
        }
        break;
        //Sell object
        case 0x89:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const SHOP_TYPE &shopId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            sellObject(queryNumber,shopId,objectId,quantity,price);
            return true;
        }
        break;
        //Get factory list
        case 0x8A:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            //getFactoryList(queryNumber,factoryId);
            return true;
        }
        break;
        //Buy factory object
        case 0x8B:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            //buyFactoryProduct(queryNumber,factoryId,objectId,quantity,price);
            return true;
        }
        break;
        //Sell factory object
        case 0x8C:
        {
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint16_t)*2+sizeof(uint32_t)*2)
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            const uint16_t &factoryId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data)));
            const uint16_t &objectId=le16toh(*reinterpret_cast<uint16_t *>(const_cast<char *>(data+sizeof(uint16_t))));
            const uint32_t &quantity=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2)));
            const uint32_t &price=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+sizeof(uint16_t)*2+sizeof(uint32_t))));
            //sellFactoryResource(queryNumber,factoryId,objectId,quantity,price);
            return true;
        }
        break;

        //Clan action
        case 0x92:
        {
            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &clanActionId=data[pos];
            pos+=1;
            switch(clanActionId)
            {
                case 0x01:
                case 0x04:
                case 0x05:
                {
                    std::string tempString;
                    const uint8_t &textSize=data[pos];
                    pos+=1;
                    if(textSize>0)
                    {
                        if((size-pos)<textSize)
                        {
                            errorOutput("wrong utf8 to std::string size in clan action for text");
                            return false;
                        }
                        tempString=std::string(data+pos,textSize);
                        pos+=textSize;
                    }
                    clanAction(queryNumber,clanActionId,tempString);
                }
                break;
                case 0x02:
                case 0x03:
                    clanAction(queryNumber,clanActionId,std::string());
                break;
                default:
                    errorOutput("unknown clan action code");
                return false;
            }
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Send datapack file list
        case 0xA1:
        #ifndef CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR
        {
            switch(datapackStatus)
            {
                #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
                case DatapackStatus::Base:
                    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
                    {
                        if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
                        {
                            errorOutput("Can't use because mirror is defined");
                            return false;
                        }
                        else
                            datapackStatus=DatapackStatus::Main;
                    }
                break;
                #endif
                case DatapackStatus::Sub:
                    if(CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
                    {
                        errorOutput("CommonSettingsServer::commonSettingsServer.subDatapackCode.isEmpty()");
                        return false;
                    }
                    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
                    {
                        errorOutput("Can't use because mirror is defined");
                        return false;
                    }
                break;
                case DatapackStatus::Main:
                    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
                    {
                        errorOutput("Can't use because mirror is defined");
                        return false;
                    }
                break;
                default:
                    errorOutput("Double datapack send list");
                return false;
            }

            uint32_t pos=0;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &stepToSkip=data[pos];
            pos+=1;
            switch(stepToSkip)
            {
                case 0x01:
                    if(datapackStatus==DatapackStatus::Base)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                case 0x02:
                    if(datapackStatus==DatapackStatus::Base)
                        datapackStatus=DatapackStatus::Main;
                    else if(datapackStatus==DatapackStatus::Main)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                case 0x03:
                    if(datapackStatus==DatapackStatus::Base)
                        datapackStatus=DatapackStatus::Sub;
                    else if(datapackStatus==DatapackStatus::Main)
                        datapackStatus=DatapackStatus::Sub;
                    else if(datapackStatus==DatapackStatus::Sub)
                    {}
                    else
                    {
                        errorOutput("step out of range to get datapack base but already in highter level");
                        return false;
                    }
                break;
                default:
                    errorOutput("step out of range to get datapack: "+std::to_string(stepToSkip));
                return false;
            }

            if((size-pos)<(int)sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint32_t &number_of_file=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
            pos+=sizeof(uint32_t);
            std::vector<std::string> files;
            files.reserve(number_of_file);
            std::vector<uint32_t> partialHashList;
            partialHashList.reserve(number_of_file);
            std::string tempFileName;
            uint32_t index=0;
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            std::regex datapack_rightFileName = std::regex(DATAPACK_FILE_REGEX);
            #endif
            if(number_of_file>1000000)
            {
                errorOutput("number_of_file > million: "+std::to_string(number_of_file)+": parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            while(index<number_of_file)
            {
                {
                    if((size-pos)<(int)sizeof(uint8_t))
                    {
                        errorOutput("wrong utf8 to std::string size "+std::to_string(size)+" pos "+std::to_string(pos)+": parseQuery("+
                            std::to_string(packetCode)+
                            ","+
                            std::to_string(queryNumber)+
                            "): "+
                            binarytoHexa(data,pos)+
                            " "+
                            binarytoHexa(data+pos,size-pos)
                            );
                        return false;
                    }
                    const uint8_t &textSize=data[pos];
                    pos+=1;
                    //control the regex file into Client::datapackList()
                    if(textSize>0)
                    {
                        if((size-pos)<textSize)
                        {
                            errorOutput("wrong utf8 to std::string size for file name: parseQuery("+
                                std::to_string(packetCode)+
                                ","+
                                std::to_string(queryNumber)+
                                "): "+
                                binarytoHexa(data,pos)+
                                " "+
                                binarytoHexa(data+pos,size-pos)
                                );
                            return false;
                        }
                        tempFileName=std::string(data+pos,textSize);
                        pos+=textSize;
                    }
                    else
                        tempFileName.clear();
                    #ifdef CATCHCHALLENGER_EXTRA_CHECK
                    if(!std::regex_match(tempFileName,datapack_rightFileName))
                    {
                        errorOutput("wrong file name \""+tempFileName+"\": parseQuery("+
                            std::to_string(packetCode)+
                            ","+
                            std::to_string(queryNumber)+
                            "): "+
                            binarytoHexa(data,pos)+
                            " "+
                            binarytoHexa(data+pos,size-pos)
                            );
                        return false;
                    }
                    #endif
                }
                files.push_back(tempFileName);
                index++;
            }
            index=0;
            while(index<number_of_file)
            {
                if((size-pos)<(int)sizeof(uint32_t))
                {
                    errorOutput("wrong size for id with main ident: "+
                                          std::to_string(packetCode)+
                                          ", remaining: "+
                                          std::to_string(size-pos)+
                                          ", lower than: "+
                                          std::to_string((int)sizeof(uint32_t))
                                          );
                    return false;
                }
                const uint32_t &partialHash=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+pos)));
                pos+=sizeof(uint32_t);
                partialHashList.push_back(partialHash);
                index++;
            }
            datapackList(queryNumber,files,partialHashList);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        #else
        errorOutput("CATCHCHALLENGER_SERVER_DATAPACK_ONLYBYMIRROR");
        return false;
        #endif
        break;
        #ifndef CATCHCHALLENGER_CLASS_ONLYGAMESERVER
        //Add character
        case 0xAA:
        {
            uint32_t pos=0;
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }

            std::string pseudo;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size)+"");
                return false;
            }
            //const uint8_t &charactersGroupIndex=data[pos];
            pos+=1;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size)+"");
                return false;
            }
            const uint8_t &profileIndex=data[pos];
            pos+=1;
            //pseudo
            {
                if((size-pos)<(int)sizeof(uint8_t))
                {
                    errorOutput("wrong utf8 to std::string size in PM for text size");
                    return false;
                }
                const uint8_t &textSize=data[pos];
                pos+=1;
                if(textSize>0)
                {
                    if(textSize>CommonSettingsCommon::commonSettingsCommon.max_pseudo_size)
                    {
                        errorOutput("pseudo size is too big: "+std::to_string(pseudo.size())+" because is greater than "+std::to_string(CommonSettingsCommon::commonSettingsCommon.max_pseudo_size));
                        return false;
                    }
                    if((size-pos)<textSize)
                    {
                        errorOutput("wrong utf8 to std::string size in PM for text");
                        return false;
                    }
                    pseudo=std::string(data+pos,textSize);
                    pos+=textSize;
                }
            }
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("error to get skin with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &monsterGroupId=data[pos];
            pos+=1;
            if((size-pos)<(int)sizeof(uint8_t))
            {
                errorOutput("error to get skin with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            const uint8_t &skinId=data[pos];
            pos+=1;
            addCharacter(queryNumber,profileIndex,pseudo,monsterGroupId,skinId);
            if((size-pos)!=0)
            {
                errorOutput("remaining data: parseQuery("+
                    std::to_string(packetCode)+
                    ","+
                    std::to_string(queryNumber)+
                    "): "+
                    binarytoHexa(data,pos)+
                    " "+
                    binarytoHexa(data+pos,size-pos)
                    );
                return false;
            }
            return true;
        }
        break;
        //Remove character
        case 0xAB:
        {
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)+sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            //skip charactersGroupIndex with data+1
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1)));
            removeCharacterLater(queryNumber,characterId);
        }
        break;
        //Select character
        case 0xAC:
        {
            if(stat==ClientStat::CharacterSelected)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            if(stat!=ClientStat::Logged)
            {
                errorOutput("charaters is logged, deny charaters add/select/delete, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+") with stat: "+std::to_string(stat));
                return false;
            }
            #ifdef CATCHCHALLENGER_EXTRA_CHECK
            if(size!=(int)sizeof(uint8_t)+sizeof(uint32_t)+sizeof(uint32_t))
            {
                errorOutput("wrong size with the main ident: "+std::to_string(packetCode)+", data: "+binarytoHexa(data,size));
                return false;
            }
            #endif
            //skip charactersGroupIndex with data+4+1
            const uint32_t &characterId=le32toh(*reinterpret_cast<uint32_t *>(const_cast<char *>(data+1+4)));
            selectCharacter(queryNumber,characterId);
            return true;
        }
        break;
        #endif
        default:
        errorOutput("no query with only the main code for now, parseQuery("+std::to_string(packetCode)+","+std::to_string(queryNumber)+")");
        return false;
    }
    return true;
}
