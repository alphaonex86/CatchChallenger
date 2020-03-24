#include "DatapackGeneralLoader.hpp"
#include "../../general/base/FacilityLibGeneral.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include <iostream>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::unordered_map<uint16_t,Industry> DatapackGeneralLoader::loadIndustries(const std::string &folder,const std::unordered_map<uint16_t, Item> &items)
{
    std::unordered_map<uint16_t,Industry> industries;
    const std::vector<FacilityLibGeneral::InodeDescriptor> &fileList=CatchChallenger::FacilityLibGeneral::listFolderNotRecursive(folder,CatchChallenger::FacilityLibGeneral::ListFolder::Files);
    unsigned int file_index=0;
    while(file_index<fileList.size())
    {
        const std::string &file=fileList.at(file_index).absoluteFilePath;
        tinyxml2::XMLDocument *domDocument;
        //open and quick check the file
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
                file_index++;
                continue;
            }
            #ifndef EPOLLCATCHCHALLENGERSERVER
        }
        #endif
        const tinyxml2::XMLElement * root = domDocument->RootElement();
        if(root==NULL)
        {
            std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
            file_index++;
            continue;
        }
        if(strcmp(root->Name(),"industries")!=0)
        {
            std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
            file_index++;
            continue;
        }

        //load the content
        bool ok,ok2,ok3;
        const tinyxml2::XMLElement * industryItem = root->FirstChildElement("industrialrecipe");
        while(industryItem!=NULL)
        {
            if(industryItem->Attribute("id")!=NULL && industryItem->Attribute("time")!=NULL && industryItem->Attribute("cycletobefull")!=NULL)
            {
                Industry industry;
                const uint16_t &id=stringtouint16(industryItem->Attribute("id"),&ok);
                industry.time=stringtouint32(industryItem->Attribute("time"),&ok2);
                industry.cycletobefull=stringtouint32(industryItem->Attribute("cycletobefull"),&ok3);
                if(ok && ok2 && ok3)
                {
                    if(industries.find(id)==industries.cend())
                    {
                        if(industry.time<60*5)
                        {
                            std::cerr << "the time need be greater than 5*60 seconds to not slow down the server: " << industry.time << ", " << file << ": child->Name(): " << industryItem->Name() << std::endl;
                            industry.time=60*5;
                        }
                        if(industry.cycletobefull<1)
                        {
                            std::cerr << "cycletobefull need be greater than 0: child->Name(): " << industryItem->Name() << std::endl;
                            industry.cycletobefull=1;
                        }
                        else if(industry.cycletobefull>65535)
                        {
                            std::cerr << "cycletobefull need be lower to 10 to not slow down the server, use the quantity: child->Name(): " << industryItem->Name() << std::endl;
                            industry.cycletobefull=10;
                        }
                        //resource
                        {
                            const tinyxml2::XMLElement * resourceItem = industryItem->FirstChildElement("resource");
                            ok=true;
                            while(resourceItem!=NULL && ok)
                            {
                                Industry::Resource resource;
                                resource.quantity=1;
                                if(resourceItem->Attribute("quantity")!=NULL)
                                {
                                    resource.quantity=stringtouint32(resourceItem->Attribute("quantity"),&ok);
                                    if(!ok)
                                        std::cerr << "quantity is not a number: child->Name(): " << industryItem->Name() << std::endl;
                                }
                                if(ok)
                                {
                                    if(resourceItem->Attribute("id")!=NULL)
                                    {
                                        resource.item=stringtouint16(resourceItem->Attribute("id"),&ok);
                                        if(!ok)
                                            std::cerr << "id is not a number: child->Name(): " << industryItem->Name() << std::endl;
                                        else if(items.find(resource.item)==items.cend())
                                        {
                                            ok=false;
                                            std::cerr << "id is not into the item list: child->Name(): " << industryItem->Name() << std::endl;
                                        }
                                        else
                                        {
                                            unsigned int index=0;
                                            while(index<industry.resources.size())
                                            {
                                                if(industry.resources.at(index).item==resource.item)
                                                    break;
                                                index++;
                                            }
                                            if(index<industry.resources.size())
                                            {
                                                ok=false;
                                                std::cerr << "id of item already into resource or product list: child->Name(): " << industryItem->Name() << std::endl;
                                            }
                                            else
                                            {
                                                index=0;
                                                while(index<industry.products.size())
                                                {
                                                    if(industry.products.at(index).item==resource.item)
                                                        break;
                                                    index++;
                                                }
                                                if(index<industry.products.size())
                                                {
                                                    ok=false;
                                                    std::cerr << "id of item already into resource or product list: child->Name(): " << industryItem->Name() << std::endl;
                                                }
                                                else
                                                    industry.resources.push_back(resource);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "have not the id attribute: child->Name(): " << industryItem->Name() << std::endl;
                                    }
                                }
                                resourceItem = resourceItem->NextSiblingElement("resource");
                            }
                        }

                        //product
                        if(ok)
                        {
                            const tinyxml2::XMLElement * productItem = industryItem->FirstChildElement("product");
                            ok=true;
                            while(productItem!=NULL && ok)
                            {
                                Industry::Product product;
                                product.quantity=1;
                                if(productItem->Attribute("quantity")!=NULL)
                                {
                                    product.quantity=stringtouint32(productItem->Attribute("quantity"),&ok);
                                    if(!ok)
                                        std::cerr << "quantity is not a number: child->Name(): " << industryItem->Name() << std::endl;
                                }
                                if(ok)
                                {
                                    if(productItem->Attribute("id")!=NULL)
                                    {
                                        product.item=stringtouint16(productItem->Attribute("id"),&ok);
                                        if(!ok)
                                            std::cerr << "id is not a number: child->Name(): " << industryItem->Name() << std::endl;
                                        else if(items.find(product.item)==items.cend())
                                        {
                                            ok=false;
                                            std::cerr << "id is not into the item list: child->Name(): " << industryItem->Name() << std::endl;
                                        }
                                        else
                                        {
                                            unsigned int index=0;
                                            while(index<industry.resources.size())
                                            {
                                                if(industry.resources.at(index).item==product.item)
                                                    break;
                                                index++;
                                            }
                                            if(index<industry.resources.size())
                                            {
                                                ok=false;
                                                std::cerr << "id of item already into resource or product list: child->Name(): " << industryItem->Name() << std::endl;
                                            }
                                            else
                                            {
                                                index=0;
                                                while(index<industry.products.size())
                                                {
                                                    if(industry.products.at(index).item==product.item)
                                                        break;
                                                    index++;
                                                }
                                                if(index<industry.products.size())
                                                {
                                                    ok=false;
                                                    std::cerr << "id of item already into resource or product list: child->Name(): " << industryItem->Name() << std::endl;
                                                }
                                                else
                                                    industry.products.push_back(product);
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ok=false;
                                        std::cerr << "have not the id attribute: child->Name(): " << industryItem->Name() << std::endl;
                                    }
                                }
                                productItem = productItem->NextSiblingElement("product");
                            }
                        }

                        //add
                        if(ok)
                        {
                            if(industry.products.empty() || industry.resources.empty())
                                std::cerr << "product or resources is empty: child->Name(): " << industryItem->Name() << std::endl;
                            else
                                industries[id]=industry;
                        }
                    }
                    else
                        std::cerr << "Unable to open the industries id number already set: file: " << file << ", child->Name(): " << industryItem->Name() << std::endl;
                }
                else
                    std::cerr << "Unable to open the industries id is not a number: file: " << file << ", child->Name(): " << industryItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the industries have not the id: file: " << file << ", child->Name(): " << industryItem->Name() << std::endl;
            industryItem = industryItem->NextSiblingElement("industrialrecipe");
        }
        #ifdef EPOLLCATCHCHALLENGERSERVER
        delete domDocument;
        #endif
        file_index++;
    }
    return industries;
}

std::unordered_map<uint16_t,IndustryLink> DatapackGeneralLoader::loadIndustriesLink(const std::string &file,const std::unordered_map<uint16_t,Industry> &industries)
{
    std::unordered_map<std::string,uint8_t> reputationNameToId;
    {
        uint8_t index=0;
        while(index<CommonDatapack::commonDatapack.reputation.size())
        {
            reputationNameToId[CommonDatapack::commonDatapack.reputation.at(index).name]=index;
            index++;
        }
    }
    std::unordered_map<uint16_t,IndustryLink> industriesLink;
    tinyxml2::XMLDocument *domDocument;
    #ifndef EPOLLCATCHCHALLENGERSERVER
    //open and quick check the file
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
            return industriesLink;
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return industriesLink;
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found 2 for reputation of the xml file" << std::endl;
        return industriesLink;
    }
    if(strcmp(root->Name(),"industries")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"industries\" root balise not found for reputation of the xml file" << std::endl;
        return industriesLink;
    }

    //load the content
    bool ok,ok2;
    const tinyxml2::XMLElement * linkItem = root->FirstChildElement("link");
    while(linkItem!=NULL)
    {
        if(linkItem->Attribute("industrialrecipe")!=NULL && linkItem->Attribute("industry")!=NULL)
        {
            const uint16_t &industry_id=stringtouint16(linkItem->Attribute("industrialrecipe"),&ok);
            const uint16_t &factory_id=stringtouint16(linkItem->Attribute("industry"),&ok2);
            if(ok && ok2)
            {
                if(industriesLink.find(factory_id)==industriesLink.cend())
                {
                    if(industries.find(industry_id)!=industries.cend())
                    {
                        industriesLink[factory_id].industry=industry_id;
                        IndustryLink *industryLink=&industriesLink[factory_id];
                        {
                            {
                                const tinyxml2::XMLElement * requirementsItem = linkItem->FirstChildElement("requirements");
                                if(requirementsItem!=NULL)
                                {
                                    const tinyxml2::XMLElement * reputationItem = requirementsItem->FirstChildElement("reputation");
                                    while(reputationItem!=NULL)
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("level")!=NULL)
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRequirements reputationRequirements;
                                                std::string stringLevel=reputationItem->Attribute("level");
                                                reputationRequirements.positif=!stringStartWith(stringLevel,"-");
                                                if(!reputationRequirements.positif)
                                                    stringLevel.erase(0,1);
                                                reputationRequirements.level=stringtouint8(stringLevel,&ok);
                                                if(ok)
                                                {
                                                    reputationRequirements.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    industryLink->requirements.reputation.push_back(reputationRequirements);
                                                }
                                            }
                                            else
                                                std::cerr << "Reputation type not found: have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->Name(): " << reputationItem->Name() << std::endl;
                                        reputationItem = reputationItem->NextSiblingElement("reputation");
                                    }
                                }
                            }
                            {
                                const tinyxml2::XMLElement * rewardsItem = linkItem->FirstChildElement("rewards");
                                if(rewardsItem!=NULL)
                                {
                                    const tinyxml2::XMLElement * reputationItem = rewardsItem->FirstChildElement("reputation");
                                    while(reputationItem!=NULL)
                                    {
                                        if(reputationItem->Attribute("type")!=NULL && reputationItem->Attribute("point")!=NULL)
                                        {
                                            if(reputationNameToId.find(reputationItem->Attribute("type"))!=reputationNameToId.cend())
                                            {
                                                ReputationRewards reputationRewards;
                                                reputationRewards.point=stringtoint32(reputationItem->Attribute("point"),&ok);
                                                if(ok)
                                                {
                                                    reputationRewards.reputationId=reputationNameToId.at(reputationItem->Attribute("type"));
                                                    industryLink->rewards.reputation.push_back(reputationRewards);
                                                }
                                            }
                                        }
                                        else
                                            std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->Name(): " << reputationItem->Name() << std::endl;
                                        reputationItem = reputationItem->NextSiblingElement("reputation");
                                    }
                                }
                            }
                        }
                    }
                    else
                        std::cerr << "Industry id for factory is not found: " << industry_id << ", file: " << file << ", child->Name(): " << linkItem->Name() << std::endl;
                }
                else
                    std::cerr << "Factory already found: " << factory_id << ", file: " << file << ", child->Name(): " << linkItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the industries link the attribute is not a number, file: " << file << ", child->Name(): " << linkItem->Name() << std::endl;
        }
        else
            std::cerr << "Unable to open the industries link have not the id, file: " << file << ", child->Name(): " << linkItem->Name() << std::endl;
        linkItem = linkItem->NextSiblingElement("link");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return industriesLink;
}
#endif
