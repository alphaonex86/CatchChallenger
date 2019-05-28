#include "DatapackGeneralLoader.h"
#ifndef EPOLLCATCHCHALLENGERSERVER
#include "../../general/base/CommonDatapack.h"
#endif
#include <iostream>

using namespace CatchChallenger;

std::vector<Reputation> DatapackGeneralLoader::loadReputation(const std::string &file)
{
    std::regex excludeFilterRegex("[\"']");
    std::regex typeRegex("^[a-z]{1,32}$");
    std::vector<Reputation> reputation;
    tinyxml2::XMLDocument *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    if(CommonDatapack::commonDatapack.xmlLoadedFile.find(file)!=CommonDatapack::commonDatapack.xmlLoadedFile.cend())
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
    else
    {
        domDocument=&CommonDatapack::commonDatapack.xmlLoadedFile[file];
        #else
        domDocument=new tinyxml2::XMLDocument();
        #endif
        const auto loadOkay = domDocument->LoadFile(file.c_str());
        if(loadOkay!=0)
        {
            std::cerr << file+", "+tinyxml2errordoc(domDocument) << std::endl;
            return reputation;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return reputation;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found 2 for reputation of the xml file" << std::endl;
        return reputation;
    }
    if(strcmp(root->Name(),"reputations")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"reputations\" root balise not found for reputation of the xml file" << std::endl;
        return reputation;
    }

    //load the content
    bool ok;
    const tinyxml2::XMLElement * item = root->FirstChildElement("reputation");
    while(item!=NULL)
    {
        if(item->Attribute("type")!=NULL)
        {
            std::vector<int32_t> point_list_positive,point_list_negative;
            std::vector<std::string> text_positive,text_negative;
            const tinyxml2::XMLElement * level = item->FirstChildElement("level");
            ok=true;
            while(level!=NULL && ok)
            {
                if(level->Attribute("point")!=NULL)
                {
                    const int32_t &point=stringtoint32(level->Attribute("point"),&ok);
                    std::string text_val;
                    if(ok)
                    {
                        ok=true;
                        bool found=false;
                        unsigned int index=0;
                        if(point>=0)
                        {
                            while(index<point_list_positive.size())
                            {
                                if(point_list_positive.at(index)==point)
                                {
                                    std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->Name(): "
                                              << item->Name() << std::endl;
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_positive.at(index)>point)
                                {
                                    point_list_positive.insert(point_list_positive.begin()+index,point);
                                    text_positive.insert(text_positive.begin()+index,text_val);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_positive.push_back(point);
                                text_positive.push_back(text_val);
                            }
                        }
                        else
                        {
                            while(index<point_list_negative.size())
                            {
                                if(point_list_negative.at(index)==point)
                                {
                                    std::cerr << "Unable to open the file: " << file << ", reputation level with same number of point found!: child->Name(): "
                                              << item->Name() << std::endl;
                                    found=true;
                                    ok=false;
                                    break;
                                }
                                if(point_list_negative.at(index)<point)
                                {
                                    point_list_negative.insert(point_list_negative.begin()+index,point);
                                    text_negative.insert(text_negative.begin()+index,text_val);
                                    found=true;
                                    break;
                                }
                                index++;
                            }
                            if(!found)
                            {
                                point_list_negative.push_back(point);
                                text_negative.push_back(text_val);
                            }
                        }
                    }
                    else
                        std::cerr << "Unable to open the file: " << file << ", point is not number: child->Name(): " << item->Name() << std::endl;
                }
                level = level->NextSiblingElement("level");
            }
            std::sort(point_list_positive.begin(),point_list_positive.end());
            std::sort(point_list_negative.begin(),point_list_negative.end());
            std::reverse(point_list_negative.begin(),point_list_negative.end());
            if(ok)
                if(point_list_positive.size()<2)
                {
                    std::cerr << "Unable to open the file: " << file << ", reputation have to few level: child->Name(): " << item->Name() << std::endl;
                    ok=false;
                }
            if(ok)
                if(!vectorcontainsAtLeastOne(point_list_positive,0))
                {
                    std::cerr << "Unable to open the file: " << file << ", no starting level for the positive: child->Name(): " << item->Name() << std::endl;
                    ok=false;
                }
            if(ok)
                if(!point_list_negative.empty() && !vectorcontainsAtLeastOne(point_list_negative,-1))
                {
                    //std::cerr << "Unable to open the file: " << file << ", no starting level for the negative, first level need start with -1, fix by change range: child->Name(): " << item->Name() << std::endl;
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
                const std::string &type=item->Attribute("type");
                if(!std::regex_match(type,typeRegex))
                {
                    std::cerr << "Unable to open the file: " << file << ", the type " << item->Attribute("type")
                              << " don't match wiuth the regex: ^[a-z]{1,32}$: child->Name(): " << item->Name() << std::endl;
                    ok=false;
                }
                else
                {
                    if(!std::regex_match(type,excludeFilterRegex))
                    {
                        Reputation reputationTemp;
                        reputationTemp.name=type;
                        reputationTemp.reputation_positive=point_list_positive;
                        reputationTemp.reputation_negative=point_list_negative;
                        if(reputation.size()>=255)
                        {
                            std::cerr << "You can't have mopre than 255 reputation " << file << ": " << item->Name() << std::endl;
                            abort();
                        }
                        reputation.push_back(reputationTemp);
                    }
                }
            }
        }
        else
            std::cerr << "Unable to open the file: " << file << ", have not the item id: child->Name(): " << item->Name() << std::endl;
        item = item->NextSiblingElement("reputation");
    }

    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return reputation;
}
