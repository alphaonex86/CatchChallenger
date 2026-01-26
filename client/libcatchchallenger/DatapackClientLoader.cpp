#include "DatapackClientLoader.hpp"
#include "DatapackChecksum.hpp"
#include "../../general/base/GeneralVariable.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonSettingsCommon.hpp"
#include "../../general/base/CommonSettingsServer.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/Map_loader.hpp"
#include "../../general/tinyXML2/customtinyxml2.hpp"
#ifdef CATCHCHALLENGER_CACHE_HPS
#include "../../general/hps/hps.h"
#include <fstream>
#endif
#include <sys/stat.h>
#include <iostream>
#include <chrono>

std::string DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN;
std::string DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1 "na" DATAPACK_BASE_PATH_MAPSUB2;

bool operator!=(const DatapackClientLoader::CCColor& a, const DatapackClientLoader::CCColor& b)
{
    if(a.a != b.a)
        return true;
    if(a.r != b.r)
        return true;
    if(a.g != b.g)
        return true;
    if(a.b != b.b)
        return true;
    return false;
}

DatapackClientLoader::DatapackClientLoader()
{
    inProgress=false;
}

DatapackClientLoader::~DatapackClientLoader()
{
}

bool DatapackClientLoader::isParsingDatapack() const
{
    return inProgress;
}

void DatapackClientLoader::parseDatapack(const std::string &datapackPath,const std::string &cacheHash,const std::string &language)
{
    (void)language;
    if(inProgress)
    {
        std::cerr << "already in progress" << std::endl;
        return;
    }

    if(!CommonSettingsCommon::commonSettingsCommon.httpDatapackMirrorBase.empty())
    {
        std::vector<char> hash;
        if(!cacheHash.empty())
        {
            hash=std::vector<char>(cacheHash.begin(),cacheHash.end());
            if(hash.size()!=28)
                hash=CatchChallenger::DatapackChecksum::doChecksumBase(datapackPath);
        }
        else
            hash=CatchChallenger::DatapackChecksum::doChecksumBase(datapackPath);
        if(hash.empty())
        {
            std::cerr << "DatapackClientLoader::parseDatapack(): hash is empty" << std::endl;
            emitdatapackChecksumError();
            inProgress=false;
            return;
        }

        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase.size()!=28)
        {
            std::cerr << "CommonSettingsCommon::commonSettingsCommon.datapackHashBase size is not 28: "
                      << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase) << std::endl;
            emitdatapackChecksumError();
            inProgress=false;
            abort();
            return;
        }
        if(hash.size()!=28)
        {
            std::cerr << "hash size is not 28: "
                      << binarytoHexa(hash) << std::endl;
            emitdatapackChecksumError();
            inProgress=false;
            abort();
            return;
        }

        if(CommonSettingsCommon::commonSettingsCommon.datapackHashBase!=hash)
        {
            std::cerr << "DatapackClientLoader::parseDatapack() CommonSettingsCommon::commonSettingsCommon.datapackHashBase!=hash.result(): "
                      << binarytoHexa(CommonSettingsCommon::commonSettingsCommon.datapackHashBase) << "!="
                      << binarytoHexa(hash) << std::endl;
            emitdatapackChecksumError();
            inProgress=false;
            return;
        }
    }

    this->datapackPath=datapackPath;
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN "na/";
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=std::string(DATAPACK_BASE_PATH_MAPSUB1)+"na/"+std::string(DATAPACK_BASE_PATH_MAPSUB2)+"nabis/";
    #ifndef BOTTESTCONNECT
    auto start_time = std::chrono::high_resolution_clock::now();
    CatchChallenger::CommonDatapack::commonDatapack.parseDatapack(datapackPath);

    auto end_time = std::chrono::high_resolution_clock::now();
    auto time = end_time - start_time;
    std::cout << "CatchChallenger::CommonDatapack::commonDatapack.parseDatapack() took " << time/std::chrono::milliseconds(1) << "ms to parse " << datapackPath << std::endl;

    auto start_time2 = std::chrono::high_resolution_clock::now();
    this->language=language;
    parseVisualCategory();
    parseTypesExtra();
    parseItemsExtra();
    parseSkins();
    parseSkillsExtra();
    parseAudioAmbiance();
    parseReputationExtra();
    auto end_time2 = std::chrono::high_resolution_clock::now();
    auto time2 = end_time2 - start_time2;
    std::cout << "CatchChallenger::CommonDatapack::commonDatapack.parseDatapack() other took " << time2/std::chrono::milliseconds(1) << "ms to parse " << datapackPath << std::endl;
    #endif
    /*do into child class
     * inProgress=false;
    emitdatapackParsed();*/
}

void DatapackClientLoader::parseDatapackMainSub(const std::string &mainDatapackCode, const std::string &subDatapackCode, const std::string &cacheHashMain, const std::string &cacheHashBase)
{
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN=DATAPACK_BASE_PATH_MAPMAIN+mainDatapackCode+"/";
    DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB=DATAPACK_BASE_PATH_MAPSUB1+mainDatapackCode+DATAPACK_BASE_PATH_MAPSUB2+subDatapackCode+"/";

    if(inProgress)
    {
        std::cerr << "already in progress" << std::endl;
        return;
    }
    inProgress=true;
    this->mainDatapackCode=mainDatapackCode;
    this->subDatapackCode=subDatapackCode;

    if(!CommonSettingsServer::commonSettingsServer.httpDatapackMirrorServer.empty())
    {
        {
            std::vector<char> hash;
            if(!cacheHashMain.empty())
                hash=std::vector<char>(cacheHashMain.begin(),cacheHashMain.end());
            else
                hash=CatchChallenger::DatapackChecksum::doChecksumMain((datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN));
            if(hash.empty())
            {
                std::cerr << "DatapackClientLoader::parseDatapackMainSub(): hash is empty" << std::endl;
                emitdatapackChecksumError();
                inProgress=false;
                return;
            }

            if(CommonSettingsServer::commonSettingsServer.datapackHashServerMain!=hash)
            {
                 std::cerr << "DatapackClientLoader::parseDatapack() Main CommonSettingsServer::commonSettingsServer.datapackHashServerMain!=hash.result(): "
                           << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerMain) << "!="
                           << binarytoHexa(hash) << std::endl;
                emitdatapackChecksumError();
                inProgress=false;
                return;
            }
        }
        if(!CommonSettingsServer::commonSettingsServer.subDatapackCode.empty())
        {
            std::vector<char> hash;
            if(!cacheHashBase.empty())
                hash=std::vector<char>(cacheHashBase.begin(),cacheHashBase.end());
            else
                hash=CatchChallenger::DatapackChecksum::doChecksumSub(
                            (datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB));
            if(hash.empty())
            {
                std::cerr << "DatapackClientLoader::parseDatapackSub(): hash is empty" << std::endl;
                emitdatapackChecksumError();
                inProgress=false;
                return;
            }

            if(CommonSettingsServer::commonSettingsServer.datapackHashServerSub!=hash)
            {
                std::cerr << "DatapackClientLoader::parseDatapack() Sub CommonSettingsServer::commonSettingsServer.datapackHashServerSub!=hash.result(): "
                          << binarytoHexa(CommonSettingsServer::commonSettingsServer.datapackHashServerSub) << "!="
                          << binarytoHexa(hash) << std::endl;
                emitdatapackChecksumError();
                inProgress=false;
                return;
            }
        }
    }

    parseMaps();
    parseZoneExtra();
    CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.parseDatapackAfterZoneAndMap(datapackPath,mainDatapackCode,subDatapackCode);

    parseQuestsLink();
    parseQuestsExtra();
    parseQuestsText();
    parseBotFightsExtra();
    parseTopLib();

    inProgress=false;

    emitdatapackParsedMainSub();
}

std::string DatapackClientLoader::getDatapackPath() const
{
    return datapackPath;
}

std::string DatapackClientLoader::getMainDatapackPath() const
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
}

std::string DatapackClientLoader::getSubDatapackPath() const
{
    return DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPSUB;
}

DatapackClientLoader::CCColor DatapackClientLoader::namedColorToCCColor(const std::string &str,bool *ok)
{
    CCColor color;
    color.a=255;
    color.r=0;
    color.g=0;
    color.b=0;

    if(str.empty())
    {
        if(ok!=nullptr)
            *ok=false;
        return color;
    }
    std::string strclean=str;
    if(str[0]=='#')
        strclean=str.substr(1);
    if(strclean.size()!=6)
    {
        if(ok!=nullptr)
            *ok=false;
        return color;
    }

    bool returnVar=false;
    std::vector<char> data=hexatoBinary(strclean,&returnVar);
    if(!returnVar)
    {
        if(ok!=nullptr)
            *ok=false;
        return color;
    }

    color.r=data.data()[0];
    color.g=data.data()[1];
    color.b=data.data()[2];
    if(ok!=nullptr)
        *ok=true;
    return color;
}

void DatapackClientLoader::parseVisualCategory()
{
    tinyxml2::XMLDocument domDocument;
    //open and quick check the file
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_MAPBASE+"visualcategory.xml";
    const auto loadOkay = domDocument.LoadFile(file.c_str());
    if(loadOkay!=0)
    {
        std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
        return;
    }
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"visual")!=0)
    {
        std::cerr << "Unable to open the file, \"visual\" root balise not found for the xml file: " << file << std::endl;
        return;
    }

    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("category");
    while(item!=NULL)
    {
        if(item->Attribute("id")!=NULL)
        {
            if(strcmp(item->Attribute("id"),"")!=0)
            {
                if(visualCategories.find(item->Attribute("id"))==
                        visualCategories.cend())
                {
                    bool ok;
                    CCColor color;
                    color.a=0;
                    color.r=0;
                    color.g=0;
                    color.b=0;
                    visualCategories[item->Attribute("id")].defaultColor=color;
                    int alpha=255;
                    if(item->Attribute("alpha")!=NULL)
                    {
                        alpha=stringtouint8(item->Attribute("alpha"),&ok);
                        if(!ok || alpha>255)
                        {
                            alpha=255;
                            std::cerr << "Unable to open the file: alpha is not number or greater than 255: child.Name()"
                                      << file << std::endl;
                        }
                    }
                    if(item->Attribute("color")!=NULL)
                    {
                        bool ok=false;
                        CCColor color=namedColorToCCColor(item->Attribute("color"),&ok);
                        if(ok)
                        {
                            color.a=alpha;
                            visualCategories[item->Attribute("id")].defaultColor=color;
                        }
                        else
                            std::cerr << "Unable to open the file, color is not valid: "
                                      << file << std::endl;
                    }
                    const tinyxml2::XMLElement *event = item->FirstChildElement("event");
                    while(event!=NULL)
                    {
                        if(event->Attribute("id")!=NULL && event->Attribute("value")!=NULL)
                        {
                            unsigned int index=0;
                            while(index<CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
                            {
                                if(CatchChallenger::CommonDatapack::commonDatapack.get_events().at(index).name==
                                        event->Attribute("id"))
                                {
                                    unsigned int sub_index=0;
                                    while(sub_index<CatchChallenger::CommonDatapack::commonDatapack.get_events().at(index).values.size())
                                    {
                                        if(CatchChallenger::CommonDatapack::commonDatapack.get_events().at(index).values.at(sub_index)==
                                                event->Attribute("value"))
                                        {
                                            VisualCategory::VisualCategoryCondition visualCategoryCondition;
                                            visualCategoryCondition.event=static_cast<uint8_t>(index);
                                            visualCategoryCondition.eventValue=static_cast<uint8_t>(sub_index);
                                            visualCategoryCondition.color=visualCategories.at(
                                                        item->Attribute("id")).defaultColor;
                                            int alpha=255;
                                            if(event->Attribute("alpha")!=NULL)
                                            {
                                                alpha=stringtouint32(event->Attribute("alpha"),&ok);
                                                if(!ok || alpha>255)
                                                {
                                                    alpha=255;
                                                    std::cerr << "alpha is not number or greater than 255:" << file << std::endl;
                                                }
                                            }
                                            if(event->Attribute("color")!=NULL)
                                            {
                                                bool ok=false;
                                                CCColor color=namedColorToCCColor(event->Attribute("color"),&ok);
                                                color.a=alpha;
                                                if(ok)
                                                    visualCategoryCondition.color=color;
                                                else
                                                    std::cerr << "color is not valid: " << file << std::endl;
                                            }
                                            if(visualCategoryCondition.color!=visualCategories.at(
                                                        item->Attribute("id")).defaultColor)
                                                visualCategories[item->Attribute("id")]
                                                        .conditions.push_back(visualCategoryCondition);
                                            else
                                                std::cerr << "color same than the default color" << file << std::endl;
                                            break;
                                        }
                                        sub_index++;
                                    }
                                    if(sub_index==CatchChallenger::CommonDatapack::commonDatapack.get_events().at(index).values.size())
                                        std::cerr << "event value not found: " << file << std::endl;
                                    break;
                                }
                                index++;
                            }
                            if(index==CatchChallenger::CommonDatapack::commonDatapack.get_events().size())
                                std::cerr << "event not found: " << file << std::endl;
                        }
                        else
                            std::cerr << "attribute id is already found" << file << std::endl;
                        event = event->NextSiblingElement("event");
                    }
                }
                else
                    std::cerr << "attribute id is already found: " << file << std::endl;
            }
            else
                std::cerr << "attribute id can't be empty: " << file << std::endl;
        }
        else
            std::cerr << "have not the attribute id: " << file << std::endl;
        item = item->NextSiblingElement("category");
    }

    std::cout << std::to_string(visualCategories.size()) << " visual cat loaded" << std::endl;
}

void DatapackClientLoader::parseReputationExtra()
{
    {
        uint8_t index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.get_reputation().size())
        {
            reputationNameToId[CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(index).name]=index;
            index++;
        }
    }
    tinyxml2::XMLDocument domDocument;
    //open and quick check the file
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_PLAYERBASE+"reputation.xml";
    const auto loadOkay = domDocument.LoadFile(file.c_str());
    if(loadOkay!=0)
    {
        std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
        return;
    }
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"reputations")!=0)
    {
        std::cerr << "\"reputations\" root balise not found for the xml file: " << file << std::endl;
        return;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement *item = root->FirstChildElement("reputation");
    while(item!=NULL)
    {
        if(item->Attribute("type")!=NULL)
        {
            ok=true;

            //load the name
            {
                bool name_found=false;
                const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                if(!language.empty() && language!="en")
                    while(name!=NULL)
                    {
                        if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                        {
                            reputationExtra[item->Attribute("type")].name=name->GetText();
                            name_found=true;
                            break;
                        }
                        name = name->NextSiblingElement("name");
                    }
                if(!name_found)
                {
                    name = item->FirstChildElement("name");
                    while(name!=NULL)
                    {
                        if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                            if(name->GetText()!=NULL)
                            {
                                reputationExtra[item->Attribute("type")].name=name->GetText();
                                name_found=true;
                                break;
                            }
                        name = name->NextSiblingElement("name");
                    }
                }
                if(!name_found)
                {
                    reputationExtra[item->Attribute("type")].name="Unknown";
                    std::cerr << "English name not found for the reputation with id: "
                              << item->Attribute("type") << std::endl;
                }
            }

            std::vector<int32_t> point_list_positive,point_list_negative;
            std::vector<std::string> text_positive,text_negative;
            const tinyxml2::XMLElement *level = item->FirstChildElement("level");
            ok=true;
            while(level!=NULL && ok)
            {
                if(level->Attribute("point")!=NULL)
                {
                    const int32_t &point=stringtoint32(level->Attribute("point"),&ok);
                    //std::string text_val;
                    if(ok)
                    {
                        ok=true;

                        std::string text;
                        //load the name
                        {
                            bool name_found=false;
                            const tinyxml2::XMLElement *name = level->FirstChildElement("text");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang") && name->Attribute("lang")==language && name->GetText()!=NULL)
                                    {
                                        text=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("text");
                                }
                            if(!name_found)
                            {
                                name = level->FirstChildElement("text");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                        if(name->GetText()!=NULL)
                                        {
                                            text=name->GetText();
                                            name_found=true;
                                            break;
                                        }
                                    name = name->NextSiblingElement("text");
                                }
                            }
                            if(!name_found)
                            {
                                text="Unknown";
                                std::cerr << "English name not found for the reputation with id: " << item->Attribute("type") << std::endl;
                            }
                        }

                        bool found=false;
                        unsigned int index=0;
                        if(point>=0)
                        {
                            while(index<point_list_positive.size())
                            {
                                if(point_list_positive.at(index)==point)
                                {
                                    std::cerr << "reputation level with same number of point found!: child.Name(): " << file << " "
                                              << item->Name() << std::endl;
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_positive.at(index)>point)
                                {
                                    point_list_positive.insert(point_list_positive.cbegin()+index,point);
                                    text_positive.insert(text_positive.cbegin()+index,text);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_positive.push_back(point);
                                text_positive.push_back(text);
                            }
                        }
                        else
                        {
                            while(index<point_list_negative.size())
                            {
                                if(point_list_negative.at(index)==point)
                                {
                                    std::cerr << "reputation level with same number of point found!: child.Name(): " << file << " " << item->Name() << std::endl;
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_negative.at(index)<point)
                                {
                                    point_list_negative.insert(point_list_negative.cbegin()+index,point);
                                    text_negative.insert(text_negative.cbegin()+index,text);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_negative.push_back(point);
                                text_negative.push_back(text);
                            }
                        }
                    }
                    else
                        std::cerr << "point is not number: child.Name(): " << file << " " << item->Name() << std::endl;
                }
                level = level->NextSiblingElement("level");
            }
            std::sort(point_list_positive.begin(),point_list_positive.end());
            std::sort(point_list_negative.begin(),point_list_negative.end());
            std::reverse(point_list_positive.begin(), point_list_positive.end());
            if(ok)
                if(point_list_positive.size()<2)
                {
                    std::cerr << "reputation have to few level: child.Name(): " << file << " " << item->Name() << std::endl;
                    ok=false;
                }
            if(ok)
                if(!vectorcontainsAtLeastOne(point_list_positive,0))
                {
                    std::cerr << "no starting level for the positive: child.Name(): " << file << " " << item->Name() << std::endl;
                    ok=false;
                }
            if(ok)
                if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                {
                    //std::cerr << "Unable to open the file: %1, no starting level for the negative client: child.Name(): " << file << " " << item->Name() << std::endl;
                    std::vector<int32_t> point_list_negative_new;
                    int lastValue=-1;
                    unsigned int index=0;
                    while(index<point_list_negative.size())
                    {
                        point_list_negative_new.push_back(lastValue);
                        lastValue=point_list_negative.at(index);//(1 less to negative value)
                        index++;
                    }
                    point_list_negative=point_list_negative_new;
                }
            if(ok)
            {
                std::regex regex("[a-z]{1,32}");
                const std::string type(item->Attribute("type"));
                std::smatch base_match;
                if(!std::regex_match(type, base_match, regex))
                {
                    std::cerr << "the type don't match wiuth the regex: ^[a-z]{1,32}$: "
                              << file << " "
                              << item->Name() << " "
                              << item->Attribute("type") << std::endl;
                    ok=false;
                }
            }
            if(ok)
            {
                reputationExtra[item->Attribute("type")].reputation_positive=text_positive;
                reputationExtra[item->Attribute("type")].reputation_negative=text_negative;
            }
        }
        else
            std::cerr << "Unable to open the file: %1, have not the item id: child.Name(): " << file << " " << item->Name() << std::endl;
        item = item->NextSiblingElement("reputation");
    }
    {
        unsigned int index=0;
        while(index<CatchChallenger::CommonDatapack::commonDatapack.get_reputation().size())
        {
            const CatchChallenger::Reputation &reputation=CatchChallenger::CommonDatapack::commonDatapack.get_reputation().at(index);
            if(reputationExtra.find(reputation.name)==reputationExtra.cend())
                reputationExtra[reputation.name].name="Unknown";
            while((uint32_t)reputationExtra[reputation.name].reputation_negative.size()<reputation.reputation_negative.size())
                reputationExtra[reputation.name].reputation_negative.push_back("Unknown");
            while((uint32_t)reputationExtra[reputation.name].reputation_positive.size()<reputation.reputation_positive.size())
                reputationExtra[reputation.name].reputation_positive.push_back("Unknown");
            index++;
        }
    }

    std::cout << std::to_string(reputationExtra.size()) << " reputation(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseItemsExtra()
{
    const std::vector<std::string> &fileList=CatchChallenger::FacilityLibGeneral::listFolder(datapackPath+DATAPACK_BASE_PATH_ITEM+"/");
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        const std::string &file=datapackPath+DATAPACK_BASE_PATH_ITEM+fileList.at(file_index);
        if(!CatchChallenger::FacilityLibGeneral::isFile(file))
        {
            file_index++;
            continue;
        }
        tinyxml2::XMLDocument domDocument;
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        //open and quick check the file
        const auto loadOkay = domDocument.LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
            file_index++;
            continue;
        }
        const tinyxml2::XMLElement *root = domDocument.RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"items")!=0)
        {
            //std::cerr << "Unable to open the file: %1, \"items\" root balise not found for the xml file").arg(QString::fromStdString(file));
            file_index++;
            continue;
        }

        std::string folder=CatchChallenger::FacilityLibGeneral::getFolderFromFile(file);
        if(!stringEndsWith(folder,"/") && !stringEndsWith(folder,"\\"))
            folder+="/";
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("item");
        while(item!=NULL)
        {
            if(item->Attribute("id")!=NULL)
            {
                const uint32_t &tempid=stringtouint32(item->Attribute("id"),&ok);
                if(ok && tempid<65536)
                {
                    const uint16_t &id=static_cast<uint16_t>(tempid);
                    if(DatapackClientLoader::itemsExtra.find(id)==DatapackClientLoader::itemsExtra.cend())
                    {
                        ItemExtra itemExtra;
                        //load the image
                        if(item->Attribute("image")!=NULL)
                            itemExtra.imagePath=folder+item->Attribute("image");
                        else
                        {
                            std::cerr << "For parse item: Have not image attribute: child.Name(): "
                                        << item->Name() << " "
                                        << file << std::endl;
                            itemExtra.imagePath=":/CC/images/inventory/unknown-object.png";
                        }

                        //load the name
                        {
                            bool name_found=false;
                            const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                            if(!language.empty() && language!="en")
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang") && name->Attribute("lang")==language && name->GetText()!=NULL)
                                    {
                                        itemExtra.name=name->GetText();
                                        name_found=true;
                                        break;
                                    }
                                    name = name->NextSiblingElement("name");
                                }
                            if(!name_found)
                            {
                                name = item->FirstChildElement("name");
                                while(name!=NULL)
                                {
                                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                        if(name->GetText()!=NULL)
                                        {
                                            itemExtra.name=name->GetText();
                                            name_found=true;
                                            break;
                                        }
                                    name = name->NextSiblingElement("name");
                                }
                            }
                            if(!name_found)
                            {
                                itemExtra.name="Unknown object";
                                std::cerr << "English name not found for the item with id: " << std::to_string(id) << std::endl;
                            }
                        }

                        //load the description
                        {
                            bool description_found=false;
                            const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                            if(!language.empty() && language!="en")
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang") && description->Attribute("lang")==language && description->GetText()!=NULL)
                                    {
                                        itemExtra.description=description->GetText();
                                        description_found=true;
                                        break;
                                    }
                                    description = description->NextSiblingElement("description");
                                }
                            if(!description_found)
                            {
                                description = item->FirstChildElement("description");
                                while(description!=NULL)
                                {
                                    if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                        if(description->GetText()!=NULL)
                                        {
                                            itemExtra.description=description->GetText();
                                            description_found=true;
                                            break;
                                        }
                                    description = description->NextSiblingElement("description");
                                }
                            }
                            if(!description_found)
                            {
                                itemExtra.description="This object is not listed as know object. The information can't be found.";
                                //std::cerr << "English description not found for the item with id: %1").arg(id);
                            }
                        }
                        DatapackClientLoader::itemsExtra[id]=itemExtra;
                    }
                    else
                        std::cerr << "id number already set: child.Name(): " << file << " " << item->Name() << std::endl;
                }
                else
                    std::cerr << "id is not a number: child.Name(): " << file << " " << item->Name() << std::endl;
            }
            else
                std::cerr << "have not the item id: child.Name(): " << file << " " << item->Name() << std::endl;
            item = item->NextSiblingElement("item");
        }
        file_index++;
    }

    std::cout << std::to_string(DatapackClientLoader::itemsExtra.size()) << " item(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseMaps()
{
    /// \todo do a sub overlay
    const std::vector<std::string> &returnList=
                CatchChallenger::FacilityLibGeneral::listFolder(datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN);

    //load the map
    uint16_t pointOnMapIndexItem=0;
    uint16_t pointOnMapIndexPlant=0;
    unsigned int index=0;
    std::unordered_map<std::string,std::string> sortToFull;
    std::vector<std::string> tempMapList;
    while(index<returnList.size())
    {
        const std::string &fileName=returnList.at(index);
        std::string sortFileName=fileName;
        if(stringEndsWith(fileName,".tmx") &&
                fileName.find("\"") == std::string::npos &&
                fileName.find("'\\'") == std::string::npos
                )
        {
            sortFileName.resize(sortFileName.size()-4);
            sortToFull[sortFileName]=fileName;
            tempMapList.push_back(sortFileName);
        }
        index++;
    }
    std::sort(tempMapList.begin(),tempMapList.end());
    const std::string &basePath=datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN;
    index=0;
    while(index<tempMapList.size())
    {
        std::string entry=tempMapList.at(index);
        mapToId[sortToFull.at(entry)]=index;
        std::string temp=basePath+sortToFull.at(entry);
        stringreplaceAll(temp,"\\\\","\\");
        stringreplaceAll(temp,"//","/");
        fullMapPathToId[temp]=index;
        maps.push_back(sortToFull.at(entry));

        const std::string &file=sortToFull.at(entry);
        {
            tinyxml2::XMLDocument domDocument;
            const auto loadOkay = domDocument.LoadFile((basePath+file).c_str());
            if(loadOkay!=0)
            {
                std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
                index++;
                continue;
            }

            const tinyxml2::XMLElement *root = domDocument.RootElement();
            if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"map")!=0)
            {
                std::cerr << "\"map\" root balise not found for the xml file" << file << std::endl;
                index++;
                continue;
            }
            bool ok;
            int tilewidth=16;
            int tileheight=16;
            if(root->Attribute("tilewidth")!=NULL)
            {
                tilewidth=stringtouint8(root->Attribute("tilewidth"),&ok);
                if(!ok)
                {
                    std::cerr << "tilewidth is not a number" << file << std::endl;
                    tilewidth=16;
                }
            }
            if(root->Attribute("tileheight")!=NULL)
            {
                tileheight=stringtouint8(root->Attribute("tileheight"),&ok);
                if(!ok)
                {
                    std::cerr << "tileheight is not a number" << file << std::endl;
                    tileheight=16;
                }
            }

            bool haveDirtLayer=false;
            {
                const tinyxml2::XMLElement *layergroup = root->FirstChildElement("layer");
                while(layergroup!=NULL)
                {
                    if(layergroup->Attribute("name") &&
                            strcmp(layergroup->Attribute("name"),"Dirt")==0)
                    {
                        haveDirtLayer=true;
                        break;
                    }
                    layergroup = layergroup->NextSiblingElement("layer");
                }
            }
            if(haveDirtLayer)
            {
                CatchChallenger::Map_loader mapLoader;
                if(mapLoader.tryLoadMap(basePath+file))
                {
                    unsigned int index=0;
                    while(index<mapLoader.map_to_send.dirts.size())
                    {
                        const CatchChallenger::Map_to_send::DirtOnMap_Semi &dirt=mapLoader.map_to_send.dirts.at(index);
                        plantOnMap[basePath+file][std::pair<uint8_t,uint8_t>(dirt.point.x,dirt.point.y)]=pointOnMapIndexPlant;
                        PlantIndexContent plantIndexContent;
                        plantIndexContent.map=basePath+file;
                        plantIndexContent.x=dirt.point.x;
                        plantIndexContent.y=dirt.point.y;
                        plantIndexOfOnMap[pointOnMapIndexPlant]=plantIndexContent;
                        pointOnMapIndexPlant++;
                        index++;
                    }
                }
            }

            //load name
            {
                const tinyxml2::XMLElement *objectgroup = root->FirstChildElement("objectgroup");
                while(objectgroup!=NULL)
                {
                    if(objectgroup->Attribute("name")!=NULL &&
                            strcmp(objectgroup->Attribute("name"),"Object")==0)
                    {
                        const tinyxml2::XMLElement *object = objectgroup->FirstChildElement("object");
                        while(object!=NULL)
                        {
                            if(
                                    object->Attribute("type")!=NULL && strcmp(object->Attribute("type"),"object")==0
                                    && object->Attribute("x")!=NULL
                                    && object->Attribute("y")!=NULL
                                    )
                            {
                                // the -1 is important to fix object layer bug into tiled!!!
                                // Don't remove!
                                const uint32_t &object_y=stringtouint32(object->Attribute("y"),&ok)/tileheight-1;
                                if(ok && object_y<256)
                                {
                                    const uint32_t &object_x=stringtouint32(object->Attribute("x"),&ok)/tilewidth;
                                    if(ok && object_x<256)
                                    {
                                        itemOnMap[datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+file]
                                                [std::pair<uint8_t,uint8_t>(static_cast<uint8_t>(object_x),static_cast<uint8_t>(object_y))]=
                                                pointOnMapIndexItem;
                                        pointOnMapIndexItem++;
                                    }
                                    else
                                        std::cerr << "object_y too big or not number" << object->Attribute("y") << " " << file << std::endl;
                                }
                                else
                                    std::cerr << "object_x too big or not number" << object->Attribute("x") << " " << file << std::endl;
                            }
                            object = object->NextSiblingElement("object");
                        }
                    }
                    objectgroup = objectgroup->NextSiblingElement("objectgroup");
                }
            }
        }
        index++;
    }

    std::cout << std::to_string(maps.size()) << " map(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseSkins()
{
    skins=CatchChallenger::FacilityLibGeneral::skinIdList(datapackPath+DATAPACK_BASE_PATH_SKIN);

    std::cout << std::to_string(skins.size()) << " skin(s) loaded" << std::endl;
}

void DatapackClientLoader::resetAll()
{
    CatchChallenger::CommonDatapack::commonDatapack.unload();
    language.clear();
    mapToId.clear();
    fullMapPathToId.clear();
    visualCategories.clear();
    datapackPath.clear();
    itemsExtra.clear();
    maps.clear();
    skins.clear();

    itemOnMap.clear();
    plantOnMap.clear();
    itemToPlants.clear();
    questsExtra.clear();
    botToQuestStart.clear();
    botFightsExtra.clear();
    monsterExtra.clear();
    monsterBuffsExtra.clear();
    monsterSkillsExtra.clear();
    audioAmbiance.clear();
    zonesExtra.clear();
    typeExtra.clear();
}

void DatapackClientLoader::parseQuestsExtra()
{
    //open and quick check the file
    std::string temp=datapackPath+DATAPACK_BASE_PATH_QUESTS1+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+DATAPACK_BASE_PATH_QUESTS2;
    stringreplaceAll(temp,"\\\\","\\");
    stringreplaceAll(temp,"//","/");
    const std::vector<std::string> &returnList=listFolderNotRecursive(temp, std::string());
    unsigned int index=0;
    while(index<returnList.size())
    {
        bool showRewards=false;
        bool autostep=false;
        std::string folder_path = temp + returnList.at(index);
        if(!CatchChallenger::FacilityLibGeneral::isDir(folder_path))
        {
            index++;
            continue;
        }
        bool ok;
        std::string inode=folder_path.substr(temp.size());
        if(stringEndsWith(inode,"/") || stringEndsWith(inode,"\\"))
            inode=inode.substr(0,inode.size()-1);
        //inode=inode.substr(temp.size());
        const uint32_t &tempid=stringtouint32(inode,&ok);
        if(!ok)
        {
            std::cerr << "because is folder name is not a number" << returnList.at(index) << std::endl;
            index++;
            continue;
        }
        if(tempid>=256)
        {
            std::cerr << "parseQuestsExtra too big: " << returnList.at(index) << std::endl;
            index++;
            continue;
        }
        const uint16_t &id=static_cast<uint16_t>(tempid);

        tinyxml2::XMLDocument domDocument;
        const std::string &file=folder_path+"definition.xml";
        const auto loadOkay = domDocument.LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
            index++;
            continue;
        }
        const tinyxml2::XMLElement *root = domDocument.RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"quest")!=0)
        {
            std::cerr << "\"quest\" root balise not found for the xml file" << file << std::endl;
            index++;
            continue;
        }

        DatapackClientLoader::QuestExtra quest;

        //load name
        {
            const tinyxml2::XMLElement *name = root->FirstChildElement("name");
            bool found=false;
            if(!language.empty() && language!="en")
                while(name!=NULL)
                {
                    if(name->Attribute("lang") && name->Attribute("lang")==language && name->GetText()!=NULL)
                    {
                        quest.name=name->GetText();
                        found=true;
                        break;
                    }
                    else
                        std::cerr << "is not an element: child.Name(): " << file << std::endl;
                    name = name->NextSiblingElement("name");
                }
            if(!found)
            {
                name = root->FirstChildElement("name");
                while(name!=NULL)
                {
                    if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                    {
                        if(name->GetText()!=NULL)
                        {
                            quest.name=name->GetText();
                            break;
                        }
                    }
                    else
                        std::cerr << "is not an element: child.Name(): " << file << std::endl;
                    name = name->NextSiblingElement("name");
                }
            }
        }

        //load showRewards
        {
            const tinyxml2::XMLElement *rewards = root->FirstChildElement("rewards");
            if(rewards!=NULL)
            {
                if(rewards->Attribute("show")!=NULL)
                    if(strcmp(rewards->Attribute("show"),"true")==0)
                        showRewards=true;
            }
        }
        //load autostep
        {
            if(root->Attribute("autostep")!=NULL)
                if(strcmp(root->Attribute("autostep"),"yes")==0)
                    autostep=true;
        }

        std::unordered_map<uint32_t,std::string> steps;
        {
            //load step
            const tinyxml2::XMLElement *step = root->FirstChildElement("step");
            while(step!=NULL)
            {
                int16_t tempid=1;
                if(step->Attribute("id")!=NULL)
                {
                    tempid=stringtouint8(step->Attribute("id"),&ok);
                    if(!ok)
                    {
                        std::cerr << "Unable to open the file: %1, id is not a number: child.Name(): " << file << " " << step->Name() << std::endl;
                        tempid=-1;
                    }
                }
                if(tempid>=0)
                {
                    const uint16_t &id=static_cast<uint16_t>(tempid);
                    CatchChallenger::Quest::Step stepObject;
                    std::string text;

                    if(step->Attribute("bot")!=NULL)
                    {
                        const std::vector<std::string> &tempStringList=
                                stringsplit(step->Attribute("bot"),';');
                        unsigned int index=0;
                        while(index<tempStringList.size())
                        {
                            const uint32_t tempInt=stringtouint32(tempStringList.at(index),&ok);
                            if(ok && tempInt<65536)
                                stepObject.bots.push_back(static_cast<uint16_t>(tempInt));
                            index++;
                        }
                    }
                    const tinyxml2::XMLElement *stepItem = step->FirstChildElement("text");
                    bool found=false;
                    if(!language.empty() && language!="en")
                    {
                        while(stepItem!=NULL)
                        {
                            if(stepItem->Attribute("lang")!=NULL && stepItem->Attribute("lang")==language)
                                if(stepItem->GetText()!=NULL)
                                {
                                    found=true;
                                    text=stepItem->GetText();
                                }
                            stepItem = stepItem->NextSiblingElement("text");
                        }
                    }
                    if(!found)
                    {
                        stepItem = step->FirstChildElement("text");
                        while(stepItem!=NULL)
                        {
                            if(stepItem->Attribute("lang")==NULL || strcmp(stepItem->Attribute("lang"),"en")==0)
                                if(stepItem->GetText()!=NULL)
                                    text=stepItem->GetText();
                            stepItem = stepItem->NextSiblingElement("text");
                        }
                        if(text.empty())
                            text="No text";
                    }
                    steps[id]=text;
                }
                step = step->NextSiblingElement("step");
            }
        }

        //sort the step
        unsigned int indexLoop=1;
        while(indexLoop<(steps.size()+1))
        {
            if(steps.find(indexLoop)==steps.cend())
                break;
            quest.steps.push_back(steps.at(indexLoop));
            indexLoop++;
        }
        if(indexLoop>=(steps.size()+1))
        {
            //add it, all seam ok
            questsExtra[id]=quest;
            questsExtra[id].showRewards=showRewards;
            questsExtra[id].autostep=autostep;
        }
        questsPathToId[inode]=id;

        index++;
    }

    std::cout << std::to_string(questsExtra.size()) << " quest(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseQuestsText()
{
    //open and quick check the file
    std::string temp=datapackPath+DATAPACK_BASE_PATH_QUESTS1+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            DATAPACK_BASE_PATH_QUESTS2;
    stringreplaceAll(temp,"\\\\","\\");
    stringreplaceAll(temp,"//","/");
    //const std::vector<std::string> &returnList=CatchChallenger::FacilityLibGeneral::listFolder(temp);
    const std::vector<std::string> &returnList=listFolderNotRecursive(temp, std::string());
    unsigned int index=0;
    while(index<(unsigned int)returnList.size())
    {
        std::string folder_path= temp + returnList.at(index);
        if(!CatchChallenger::FacilityLibGeneral::isDir(folder_path))
        {
            index++;
            continue;
        }
        const std::string &file=folder_path+"text.xml";

        tinyxml2::XMLDocument domDocument;
        const auto loadOkay = domDocument.LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
            index++;
            continue;
        }

        const tinyxml2::XMLElement *root = domDocument.RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"text")!=0)
        {
            std::cerr << "\"quest\" root balise not found for the xml file" << file << std::endl;
            index++;
            continue;
        }

        //load the content
        std::string inode=returnList.at(index);
        if(stringEndsWith(inode,"/") || stringEndsWith(inode,"\\"))
            inode=inode.substr(0,inode.size()-1);
        if(questsPathToId.find(inode)!=questsPathToId.cend())
        {
            const uint16_t questId=questsPathToId.at(inode);
            bool ok;
            //load text
            const tinyxml2::XMLElement *client_logic = root->FirstChildElement("client_logic");
            while(client_logic!=NULL)
            {
                if(client_logic->Attribute("id"))
                {
                    const uint16_t &tempid=stringtouint16(client_logic->Attribute("id"),&ok);
                    if(ok)
                    {
                        //load the condition
                        QuestStepWithConditionExtra questStepWithConditionExtra;
                        questStepWithConditionExtra.text="No text";
                        const tinyxml2::XMLElement *condition = client_logic->FirstChildElement("contion");
                        while(condition!=NULL)
                        {
                            if(condition->Attribute("queststep")!=NULL)
                            {
                                const uint8_t queststep=stringtouint8(condition->Attribute("queststep"),&ok);
                                if(ok)
                                {
                                    QuestConditionExtra questConditionExtra;
                                    questConditionExtra.type=QuestCondition_queststep;
                                    questConditionExtra.value=queststep;
                                    questStepWithConditionExtra.conditions.push_back(questConditionExtra);
                                }
                            }
                            if(condition->Attribute("haverequirements")!=NULL)
                            {
                                QuestConditionExtra questConditionExtra;
                                questConditionExtra.type=QuestCondition_haverequirements;
                                questConditionExtra.value=strcmp(condition->Attribute("haverequirements"),"true")==0;
                                questStepWithConditionExtra.conditions.push_back(questConditionExtra);
                            }
                            condition = condition->NextSiblingElement("condition");
                        }

                        const tinyxml2::XMLElement *text = client_logic->FirstChildElement("text");
                        bool found=false;
                        if(!language.empty() && language!="en")
                            while(text!=NULL)
                            {
                                if(text->Attribute("lang") && text->Attribute("lang")==language && text->GetText()!=NULL)
                                {
                                    questStepWithConditionExtra.text=text->GetText();
                                    found=true;
                                }
                                text = text->NextSiblingElement("text");
                            }
                        if(!found)
                        {
                            text = client_logic->FirstChildElement("text");
                            while(text!=NULL)
                            {
                                if(text->Attribute("lang")==NULL || strcmp(text->Attribute("lang"),"en")==0)
                                    if(text->GetText()!=NULL)
                                        questStepWithConditionExtra.text=text->GetText();
                                text = text->NextSiblingElement("text");
                            }
                        }
                        questsExtra[questId].text[tempid].texts.push_back(questStepWithConditionExtra);
                    }
                    else
                        std::cerr << "Unable to open the file: %1, id is not a number: child.Name(): " << file << client_logic->Name() << std::endl;
                }
                else
                    std::cerr << "Has attribute: %1, have not id attribute: child.Name(): " << file << client_logic->Name() << std::endl;
                client_logic = client_logic->NextSiblingElement("client_logic");
            }
        }
        else
            std::cerr << "!questsPathToId find(): %1, have not id attribute: child.Name(): " << file << inode << std::endl;
        #ifdef DEBUG_CLIENT_QUEST
        std::cerr << "%1 quest(s) text loaded for quest %2").arg(client_logic_texts.size()).arg(questsPathToId.value(entryList.at(index).absoluteFilePath()));
        #endif
        index++;
    }

    std::cout << std::to_string(questsExtra.size()) << " quest(s) text loaded" << std::endl;
}

void DatapackClientLoader::parseAudioAmbiance()
{
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_MAPBASE+"music.xml";
    tinyxml2::XMLDocument domDocument;
    //open and quick check the file
    const auto loadOkay = domDocument.LoadFile(file.c_str());
    if(loadOkay!=0)
    {
        std::cerr << file+", "+tinyxml2errordoc(&domDocument) << std::endl;
        return;
    }
    const tinyxml2::XMLElement *root = domDocument.RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"musics")!=0)
    {
        std::cerr << "Unable to open the file: %1, \"musics\" root balise not found for the xml file" << file << std::endl;
        return;
    }

    //load the content
    const tinyxml2::XMLElement *item = root->FirstChildElement("map");
    while(item!=NULL)
    {
        if(item->Attribute("type")!=NULL)
        {
            const std::string &type=item->Attribute("type");
            if(audioAmbiance.find(type)==audioAmbiance.cend() && item->GetText()!=NULL)
                audioAmbiance[type]=/*datapackPath+DatapackClientLoader::text_DATAPACK_BASE_PATH_MAPMAIN+*/item->GetText();
            else
                std::cerr << "Unable to open the file: %1, id number already set: child.Name(): %2" << file << " " << item->Name() << file << std::endl;
        }
        else
            std::cerr << "Unable to open the file: %1, have not the item id: child.Name(): %2" << file << " " << item->Name() << file << std::endl;
        item = item->NextSiblingElement("map");
    }

    std::cout << std::to_string(audioAmbiance.size()) << " audio ambiance(s) link loaded" << std::endl;
}

void DatapackClientLoader::parseQuestsLink()
{
    auto i=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().cbegin();
    while(i!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests().cend())
    {
        if(!i->second.steps.empty())
        {
            std::vector<uint16_t> bots=i->second.steps.front().bots;
            unsigned int index=0;
            while(index<bots.size())
            {
                botToQuestStart[bots.at(index)].push_back(i->first);
                index++;
            }
        }
        ++i;
    }
    std::cout << std::to_string(botToQuestStart.size()) << " bot linked with quest(s) loaded" << std::endl;
}

void DatapackClientLoader::parseZoneExtra()
{
    //open and quick check the file
    std::string temp=datapackPath+DATAPACK_BASE_PATH_ZONE1+
            CommonSettingsServer::commonSettingsServer.mainDatapackCode+
            DATAPACK_BASE_PATH_ZONE2;
    stringreplaceAll(temp,"\\\\","\\");
    stringreplaceAll(temp,"//","/");
    std::vector<std::string> returnList=CatchChallenger::FacilityLibGeneral::listFolder(temp);
    std::sort(returnList.begin(),returnList.end());
    unsigned int index=0;
    std::regex xmlFilter("[a-zA-Z0-9\\- _]+\\.xml");
    while(index<returnList.size())
    {
        const std::string &fileWithoutPath=returnList.at(index);
        const std::string &fullPath=temp+fileWithoutPath;
        if(!CatchChallenger::FacilityLibGeneral::isFile(fullPath))
        {
            index++;
            continue;
        }
        std::smatch m;
        if(!std::regex_match(fileWithoutPath, m, xmlFilter))
        {
            std::cerr << "%1 the zone file name not match" << fullPath << std::endl;
            index++;
            continue;
        }
        std::string zoneCodeName=fileWithoutPath;
        stringreplaceAll(zoneCodeName,"/","");
        stringreplaceAll(zoneCodeName,"\\","");
        const std::string &file=fullPath;
        if(stringEndsWith(zoneCodeName,".xml"))
            zoneCodeName.resize(zoneCodeName.size()-4);

        tinyxml2::XMLDocument domDocument;
        const auto loadOkay = domDocument.LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file << ", " << tinyxml2errordoc(&domDocument) << std::endl;
            index++;
            continue;
        }

        const tinyxml2::XMLElement *root = domDocument.RootElement();
        if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"zone")!=0)
        {
            std::cerr << "Unable to open the file: %1, \"zone\" root balise not found for the xml file" << file << std::endl;
            index++;
            continue;
        }

        //load the content
        bool haveName=false;
        DatapackClientLoader::ZoneExtra zone;

        //load name
        const tinyxml2::XMLElement *name = root->FirstChildElement("name");
        bool found=false;
        if(!language.empty() && language!="en")
            while(name!=NULL)
            {
                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                {
                    if(name->GetText()!=NULL)
                    {
                        haveName=true;
                        zone.name=name->GetText();
                        break;
                    }
                }
                else
                    std::cerr << "Unable to open the file: %1, is not an element: child.Name(): " << file << name->Name() << std::endl;
                name = name->NextSiblingElement("name");
            }
        if(!found)
        {
            name = root->FirstChildElement("name");
            while(name!=NULL)
            {
                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                {
                    if(name->GetText()!=NULL)
                    {
                        haveName=true;
                        zone.name=name->GetText();
                        break;
                    }
                }
                else
                    std::cerr << "Unable to open the file: %1, is not an element: child.Name(): " << file << name->Name() << std::endl;
                name = name->NextSiblingElement("name");
            }
        }
        zone.id=zonesExtra.size();
        if(haveName)
            zonesExtra[zoneCodeName]=zone;

        //load the audio ambiance
        {
            const tinyxml2::XMLElement *item = root->FirstChildElement("music");
            while(item!=NULL)
            {
                if(item->Attribute("type")!=NULL && item->Attribute("backgroundsound")!=NULL)
                {
                    const std::string &type=item->Attribute("type");
                    const std::string &backgroundsound=item->Attribute("backgroundsound");
                    zonesExtra[zoneCodeName].audioAmbiance[type]=backgroundsound;
                }
                else
                    std::cerr << "Unable to open the file: %1, have not the music attribute: child.Name(): %2" << file << " " << item->Name() << std::endl;
                item = item->NextSiblingElement("music");
            }
        }

        index++;
    }

    std::cout << std::to_string(zonesExtra.size()) << " zone(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseSkillsExtra()
{
    //open and quick check the file
    std::string temp=datapackPath+DATAPACK_BASE_PATH_SKILL;
    stringreplaceAll(temp,"\\\\","\\");
    stringreplaceAll(temp,"//","/");
    const std::vector<std::string> &returnList=CatchChallenger::FacilityLibGeneral::listFolder(temp);
    unsigned int file_index=0;
    while(file_index<returnList.size())
    {
        const std::string &file=temp+returnList.at(file_index);
        if(!CatchChallenger::FacilityLibGeneral::isFile(file))
        {
            file_index++;
            continue;
        }
        if(!stringEndsWith(file,".xml"))
        {
            file_index++;
            continue;
        }
        //open and quick check the file
        tinyxml2::XMLDocument *domDocument;
        if(CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=
                CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().at(file);
        else
        {
            domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
            const auto loadOkay = domDocument->LoadFile(file.c_str());
            if(loadOkay!=0)
            {
                std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                file_index++;
                continue;
            }
        }
        const tinyxml2::XMLElement *root = domDocument->RootElement();
        if(root==NULL)
        {
            std::cerr << "\"skills\" root balise not found for the xml file: " << file << std::endl;
            file_index++;
            continue;
        }
        if(strcmp(root->Name(),"skills")!=0)
        {
            std::cerr << "Unable to open the xml file: %1, \"skills\" root balise not found for the xml file" << file << std::endl;
            file_index++;
            continue;
        }

        const std::string &language=getLanguage();
        bool found;
        //load the content
        bool ok;
        const tinyxml2::XMLElement *item = root->FirstChildElement("skill");
        while(item!=NULL)
        {
            if(item->Attribute("id"))
            {
                const uint32_t tempid=stringtouint16(item->Attribute("id"),&ok);
                if(!ok || tempid>65535)
                    std::cerr << "id not a number: child->Name(): " << file << std::endl;
                else
                {
                    const uint16_t &id=static_cast<uint16_t>(tempid);
                    if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().find(id)==CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().cend())
                    {}
                    else
                    {
                        DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
                        #ifdef DEBUG_MESSAGE_MONSTER_LOAD
                        std::cerr << (QStringLiteral("monster extra loading: %1").arg(id)) << std::endl;
                        #endif
                        found=false;
                        const tinyxml2::XMLElement *name = item->FirstChildElement("name");
                        if(!language.empty() && language!="en")
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")!=NULL && name->Attribute("lang")==language && name->GetText()!=NULL)
                                {
                                    monsterSkillExtraEntry.name=name->GetText();
                                    found=true;
                                    break;
                                }
                                name = name->NextSiblingElement("name");
                            }
                        if(!found)
                        {
                            name = item->FirstChildElement("name");
                            while(name!=NULL)
                            {
                                if(name->Attribute("lang")==NULL || strcmp(name->Attribute("lang"),"en")==0)
                                    if(name->GetText()!=NULL)
                                    {
                                        monsterSkillExtraEntry.name=name->GetText();
                                        break;
                                    }
                                name = name->NextSiblingElement("name");
                            }
                        }
                        found=false;
                        const tinyxml2::XMLElement *description = item->FirstChildElement("description");
                        if(!language.empty() && language!="en")
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang")!=NULL && description->Attribute("lang")==language && description->GetText()!=NULL)
                                {
                                    monsterSkillExtraEntry.description=description->GetText();
                                    found=true;
                                    break;
                                }
                                description = description->NextSiblingElement("description");
                            }
                        if(!found)
                        {
                            description = item->FirstChildElement("description");
                            while(description!=NULL)
                            {
                                if(description->Attribute("lang")==NULL || strcmp(description->Attribute("lang"),"en")==0)
                                    if(description->GetText()!=NULL)
                                    {
                                        monsterSkillExtraEntry.description=description->GetText();
                                        break;
                                    }
                                description = description->NextSiblingElement("description");
                            }
                        }
                        if(monsterSkillExtraEntry.name.empty())
                            monsterSkillExtraEntry.name="Unknown";
                        if(monsterSkillExtraEntry.description.empty())
                            monsterSkillExtraEntry.description="Unknown";
                        monsterSkillsExtra[id]=monsterSkillExtraEntry;
                    }
                }
            }
            else
                std::cerr << "have not the skill id: child->Name(): " << file << std::endl;
            item = item->NextSiblingElement("skill");
        }

        file_index++;
    }

    auto i=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().cbegin();
    while(i!=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkills().cend())
    {
        if(monsterSkillsExtra.find(i->first)==monsterSkillsExtra.cend())
        {
            std::cerr << "Strange, have entry into monster list, but not into monster skill extra for id: " << i->first << std::endl;
            DatapackClientLoader::MonsterExtra::Skill monsterSkillExtraEntry;
            monsterSkillExtraEntry.name="Unknown";
            monsterSkillExtraEntry.description="Unknown";
            monsterSkillsExtra[i->first]=monsterSkillExtraEntry;
        }
        ++i;
    }

    std::cerr << std::to_string(monsterSkillsExtra.size()) << " skill(s) extra loaded" << std::endl;
}

void DatapackClientLoader::parseTypesExtra()
{
    const std::string &file=datapackPath+DATAPACK_BASE_PATH_MONSTERS+"type.xml";
    tinyxml2::XMLDocument *domDocument;
    //open and quick check the file
    if(CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=
            CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().at(file);
    else
    {
        domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
            return;
        }
    }
    const tinyxml2::XMLElement *root = domDocument->RootElement();
    if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"types")!=0)
    {
        std::cerr << "\"types\" root balise not found for the xml file" << file << std::endl;
        return;
    }

    //load the content
    {
        const std::string &language=getLanguage();
        std::unordered_set<std::string> duplicate;
        const tinyxml2::XMLElement *typeItem = root->FirstChildElement("type");
        while(typeItem!=NULL)
        {
            if(typeItem->Attribute("name")!=NULL)
            {
                std::string name=typeItem->Attribute("name");
                if(duplicate.find(name)==duplicate.cend())
                {
                    TypeExtra type;

                    duplicate.insert(name);
                    if(typeItem->Attribute("color")!=NULL)
                    {
                        bool ok=false;
                        CCColor c=namedColorToCCColor(typeItem->Attribute("color"),&ok);
                        if(ok)
                            type.color=c;
                        else
                            std::cerr << "color is not valid: " << file << std::endl;
                    }

                    bool found=false;
                    const tinyxml2::XMLElement *nameItem = typeItem->FirstChildElement("name");
                    if(!language.empty() && language!="en")
                        while(nameItem!=NULL)
                        {
                            if(nameItem->Attribute("lang")!=NULL && nameItem->Attribute("lang")==language && nameItem->GetText()!=NULL)
                            {
                                type.name=nameItem->GetText();
                                found=true;
                                break;
                            }
                            nameItem = nameItem->NextSiblingElement("name");
                        }
                    if(!found)
                    {
                        nameItem = typeItem->FirstChildElement("name");
                        while(nameItem!=NULL)
                        {
                            if(nameItem->Attribute("lang")==NULL || strcmp(nameItem->Attribute("lang"),"en")==0)
                                if(nameItem->GetText()!=NULL)
                                {
                                    type.name=nameItem->GetText();
                                    break;
                                }
                            nameItem = nameItem->NextSiblingElement("name");
                        }
                    }
                    if(typeExtra.size()>255)
                    {
                        std::cerr << "QtDatapackClientLoader::datapackLoader.typeExtra.size()>255" << std::endl;
                        abort();
                    }
                    typeExtra[static_cast<uint8_t>(typeExtra.size())]=type;
                }
                else
                    std::cerr << "name is already set for type: " << file << std::endl;
            }
            else
                std::cerr << "have not the item id: " << file << std::endl;
            typeItem = typeItem->NextSiblingElement("type");
        }
    }

    std::cerr << std::to_string(typeExtra.size()) << " type(s) extra loaded" << std::endl;
}
void DatapackClientLoader::parseBotFightsExtra()
{
    const std::string &language=getLanguage();
    bool found;

    std::string tempbase=datapackPath+
            DATAPACK_BASE_PATH_FIGHT1+
            mainDatapackCode+
            DATAPACK_BASE_PATH_FIGHT2;
    stringreplaceAll(tempbase,"\\\\","\\");
    stringreplaceAll(tempbase,"//","/");
    const std::vector<std::string> &returnList=CatchChallenger::FacilityLibGeneral::listFolder(tempbase);
    unsigned int file_index=0;
    while(file_index<returnList.size())
    {
        const std::string &file=tempbase+returnList.at(file_index);
        if(CatchChallenger::FacilityLibGeneral::isFile(file))
        {
            //std::cout << "file: " << tempbase << "+" << returnList.at(file_index) << std::endl;
            //const std::string &file=returnList.at(file_index);
            tinyxml2::XMLDocument *domDocument;
            //open and quick check the file
            if(CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().find(file)!=
                    CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile().cend())
                domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw().at(file);
            else
            {
                domDocument=&CatchChallenger::CommonDatapack::commonDatapack.get_xmlLoadedFile_rw()[file];
                const auto loadOkay = domDocument->LoadFile(file.c_str());
                if(loadOkay!=0)
                {
                    std::cerr << file << ", " << tinyxml2errordoc(domDocument) << std::endl;
                    file_index++;
                    continue;
                }
            }
            const tinyxml2::XMLElement *root = domDocument->RootElement();
            if(root==NULL || root->Name()==NULL || strcmp(root->Name(),"fights")!=0)
            {
                std::cerr << "\"fights\" root balise not found for the xml file: "
                          << file << ", " << root->Name() << "!=" << "fights" << std::endl;
                file_index++;
                continue;
            }

            //load the content
            bool ok;
            const tinyxml2::XMLElement *item = root->FirstChildElement("fight");
            while(item!=NULL)
            {
                if(item->Attribute("id"))
                {
                    const uint32_t tempid=stringtouint16(item->Attribute("id"),&ok);
                    if(ok && tempid<65535)
                    {
                        const uint16_t id=static_cast<uint16_t>(tempid);
                        if(CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().find(id)!=
                                CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
                        {
                            if(botFightsExtra.find(id)==botFightsExtra.cend())
                            {
                                BotFightExtra botFightExtra;
                                botFightExtra.start="Ready for the fight?";
                                botFightExtra.win="You are so strong for me!";
                                {
                                    found=false;
                                    const tinyxml2::XMLElement *start = item->FirstChildElement("start");
                                    if(!language.empty() && language!="en")
                                        while(start!=NULL)
                                        {
                                            if(start->Attribute("lang")!=NULL && start->Attribute("lang")==language && start->GetText()!=NULL)
                                            {
                                                botFightExtra.start=start->GetText();
                                                found=true;
                                                break;
                                            }
                                            start = start->NextSiblingElement("start");
                                        }
                                    if(!found)
                                    {
                                        start = item->FirstChildElement("start");
                                        while(start!=NULL)
                                        {
                                            if(start->Attribute("lang")==NULL || strcmp(start->Attribute("lang"),"en")==0)
                                                if(start->GetText()!=NULL)
                                                {
                                                    botFightExtra.start=start->GetText();
                                                    break;
                                                }
                                            start = start->NextSiblingElement("start");
                                        }
                                    }
                                    found=false;
                                    const tinyxml2::XMLElement *win = item->FirstChildElement("win");
                                    if(!language.empty() && language!="en")
                                        while(win!=NULL)
                                        {
                                            if(win->Attribute("lang") && win->Attribute("lang")==language && win->GetText()!=NULL)
                                            {
                                                botFightExtra.win=win->GetText();
                                                found=true;
                                                break;
                                            }
                                            win = win->NextSiblingElement("win");
                                        }
                                    if(!found)
                                    {
                                        win = item->FirstChildElement("win");
                                        while(win!=NULL)
                                        {
                                            if(win->Attribute("lang")==NULL || strcmp(win->Attribute("lang"),"en")==0)
                                                if(win->GetText()!=NULL)
                                                {
                                                    botFightExtra.win=win->GetText();
                                                    break;
                                                }
                                            win = win->NextSiblingElement("win");
                                        }
                                    }
                                }
                                botFightsExtra[id]=botFightExtra;
                            }
                            else
                                std::cerr << "id is already into the botFight extra: " << file << std::endl;
                        }
                        else
                            std::cerr << "bot fights have not the id " << std::to_string(id) << ", " << file << std::endl;
                    }
                    else
                        std::cerr << "id is not a number to parse bot fight extra: " << file << std::endl;
                }
                item = item->NextSiblingElement("fight");
            }
        }
        else
            std::cerr << "file not found bot fight extra: " << file << std::endl;
        file_index++;
    }

    auto i=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().begin();
    while(i!=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_botFights().cend())
    {
        if(botFightsExtra.find(i->first)==botFightsExtra.cend())
        {
            std::cerr << "Strange, have entry into monster list, but not into bot fight extra for id: " << std::to_string(i->first) << std::endl;
            BotFightExtra botFightExtra;
            botFightExtra.start="Ready for the fight?";
            botFightExtra.win="You are so strong for me!";
            botFightsExtra[i->first]=botFightExtra;
        }
        ++i;
    }

    std::cerr << std::to_string(botFightsExtra.size()) << " fight extra(s) loaded" << std::endl;
}

std::vector<std::string> DatapackClientLoader::listFolderNotRecursive(const std::string& folder,const std::string& suffix)
{
    std::vector<std::string> returnList;
    std::vector<CatchChallenger::FacilityLibGeneral::InodeDescriptor> entryList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder+suffix, CatchChallenger::FacilityLibGeneral::Dirs);//possible wait time here
    for (unsigned int index=0;index<entryList.size();++index)
    {
        const CatchChallenger::FacilityLibGeneral::InodeDescriptor &fileInfo=entryList.at(index);
        if(fileInfo.type==CatchChallenger::FacilityLibGeneral::InodeDescriptor::Type::Dir)
            returnList.push_back(suffix+fileInfo.name + "/");
    }
    return returnList;
}

const std::unordered_map<uint8_t,DatapackClientLoader::TypeExtra> &DatapackClientLoader::get_typeExtra() const
{
    return typeExtra;
}

const std::unordered_map<uint16_t,DatapackClientLoader::MonsterExtra> &DatapackClientLoader::get_monsterExtra() const
{
    return monsterExtra;
}

const std::unordered_map<CATCHCHALLENGER_TYPE_MONSTER,DatapackClientLoader::MonsterExtra::Buff> &DatapackClientLoader::get_monsterBuffsExtra() const
{
    return monsterBuffsExtra;
}

const std::unordered_map<CATCHCHALLENGER_TYPE_SKILL,DatapackClientLoader::MonsterExtra::Skill> &DatapackClientLoader::get_monsterSkillsExtra() const
{
    return monsterSkillsExtra;
}

const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,DatapackClientLoader::ItemExtra> &DatapackClientLoader::get_itemsExtra() const
{
    return itemsExtra;
}

const std::unordered_map<std::string,DatapackClientLoader::ReputationExtra> &DatapackClientLoader::get_reputationExtra() const
{
    return reputationExtra;
}

const std::unordered_map<std::string,uint8_t> &DatapackClientLoader::get_reputationNameToId() const
{
    return reputationNameToId;
}

const std::unordered_map<CATCHCHALLENGER_TYPE_ITEM,uint8_t> &DatapackClientLoader::get_itemToPlants() const
{
    return itemToPlants;
}

const std::unordered_map<CATCHCHALLENGER_TYPE_QUEST,DatapackClientLoader::QuestExtra> &DatapackClientLoader::get_questsExtra() const
{
    return questsExtra;
}

const std::unordered_map<std::string,uint16_t> &DatapackClientLoader::get_questsPathToId() const
{
    return questsPathToId;
}

const std::unordered_map<uint16_t,std::vector<CATCHCHALLENGER_TYPE_QUEST> > &DatapackClientLoader::get_botToQuestStart() const
{
    return botToQuestStart;
}

const std::unordered_map<uint16_t,DatapackClientLoader::BotFightExtra> &DatapackClientLoader::get_botFightsExtra() const
{
    return botFightsExtra;
}

const std::unordered_map<std::string,DatapackClientLoader::ZoneExtra> &DatapackClientLoader::get_zonesExtra() const
{
    return zonesExtra;
}

const std::unordered_map<std::string,std::string> &DatapackClientLoader::get_audioAmbiance() const
{
    return audioAmbiance;
}

const std::unordered_map<uint32_t,DatapackClientLoader::ProfileText> &DatapackClientLoader::get_profileTextList() const
{
    return profileTextList;
}

const std::unordered_map<std::string,DatapackClientLoader::VisualCategory> &DatapackClientLoader::get_visualCategories() const
{
    return visualCategories;
}

const std::string &DatapackClientLoader::get_language() const
{
    return language;
}

const std::vector<std::string> &DatapackClientLoader::get_maps() const
{
    return maps;
}

const std::vector<std::string> &DatapackClientLoader::get_skins() const
{
    return skins;
}

const std::unordered_map<std::string,uint32_t> &DatapackClientLoader::get_mapToId() const
{
    return mapToId;
}

const std::unordered_map<std::string,uint32_t> &DatapackClientLoader::get_fullMapPathToId() const
{
    return fullMapPathToId;
}

const std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > &DatapackClientLoader::get_itemOnMap() const
{
    return itemOnMap;
}

const std::unordered_map<std::string,std::unordered_map<std::pair<uint8_t,uint8_t>,CATCHCHALLENGER_TYPE_ITEM,pairhash> > &DatapackClientLoader::get_plantOnMap() const
{
    return plantOnMap;
}

const std::unordered_map<uint16_t,DatapackClientLoader::PlantIndexContent> &DatapackClientLoader::get_plantIndexOfOnMap() const
{
    return plantIndexOfOnMap;
}

