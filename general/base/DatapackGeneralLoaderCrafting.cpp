#include "DatapackGeneralLoader.h"
#include "../../general/base/CommonDatapack.h"
#include <iostream>

using namespace CatchChallenger;

#ifndef CATCHCHALLENGER_CLASS_MASTER
std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> > DatapackGeneralLoader::loadCraftingRecipes(const std::string &file,const std::unordered_map<uint16_t, Item> &items,uint16_t &crafingRecipesMaxId)
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
    std::unordered_map<uint16_t,CraftingRecipe> crafingRecipes;
    std::unordered_map<uint16_t,uint16_t> itemToCrafingRecipes;
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
            return std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
        }
        #ifndef EPOLLCATCHCHALLENGERSERVER
    }
    #endif
    const tinyxml2::XMLElement * root = domDocument->RootElement();
    if(root==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", no root balise found for the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }
    if(root->Name()==NULL)
    {
        std::cerr << "Unable to open the file: " << file << ", \"recipes\" root balise not found 2 for reputation of the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }
    if(strcmp(root->Name(),"recipes")!=0)
    {
        std::cerr << "Unable to open the file: " << file << ", \"recipes\" root balise not found for reputation of the xml file" << std::endl;
        return std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
    }

    //load the content
    bool ok,ok2,ok3;
    const tinyxml2::XMLElement * recipeItem = root->FirstChildElement("recipe");
    while(recipeItem!=NULL)
    {
        if(recipeItem->Attribute("id")!=NULL && recipeItem->Attribute("itemToLearn")!=NULL && recipeItem->Attribute("doItemId")!=NULL)
        {
            uint8_t success=100;
            if(recipeItem->Attribute("success")!=NULL)
            {
                const uint8_t &tempShort=stringtouint8(recipeItem->Attribute("success"),&ok);
                if(ok)
                {
                    if(tempShort>100)
                        std::cerr << "preload_crafting_recipes() success can't be greater than 100 for crafting recipe file: " << file << ", child->Name(): " << recipeItem->Name() << std::endl;
                    else
                        success=tempShort;
                }
                else
                    std::cerr << "preload_crafting_recipes() success in not an number for crafting recipe file: " << file << ", child->Name(): " << recipeItem->Name() << std::endl;
            }
            uint16_t quantity=1;
            if(recipeItem->Attribute("quantity")!=NULL)
            {
                const uint32_t &tempShort=stringtouint32(recipeItem->Attribute("quantity"),&ok);
                if(ok)
                {
                    if(tempShort>65535)
                        std::cerr << "preload_crafting_recipes() quantity can't be greater than 65535 for crafting recipe file: " << file << ", child->Name(): " << recipeItem->Name() << std::endl;
                    else
                        quantity=static_cast<uint16_t>(tempShort);
                }
                else
                    std::cerr << "preload_crafting_recipes() quantity in not an number for crafting recipe file: " << file << ", child->Name(): " << recipeItem->Name() << std::endl;
            }

            const uint16_t &id=stringtouint16(recipeItem->Attribute("id"),&ok);
            const uint16_t &itemToLearn=stringtouint16(recipeItem->Attribute("itemToLearn"),&ok2);
            const uint16_t &doItemId=stringtouint16(recipeItem->Attribute("doItemId"),&ok3);
            if(ok && ok2 && ok3)
            {
                if(crafingRecipes.find(id)==crafingRecipes.cend())
                {
                    ok=true;
                    CatchChallenger::CraftingRecipe recipe;
                    recipe.doItemId=doItemId;
                    recipe.itemToLearn=itemToLearn;
                    recipe.quantity=quantity;
                    recipe.success=success;
                    {
                        const tinyxml2::XMLElement * requirementsItem = recipeItem->FirstChildElement("requirements");
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
                                            recipe.requirements.reputation.push_back(reputationRequirements);
                                        }
                                    }
                                    else
                                        std::cerr << "Reputation type not found: " << reputationItem->Attribute("type") << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                }
                                else
                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                reputationItem = reputationItem->NextSiblingElement("reputation");
                            }
                        }
                    }
                    {
                        const tinyxml2::XMLElement * rewardsItem = recipeItem->FirstChildElement("rewards");
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
                                            recipe.rewards.reputation.push_back(reputationRewards);
                                        }
                                    }
                                }
                                else
                                    std::cerr << "Unable to open the industries link file: " << file << ", have not the id, child->Name(): " << reputationItem->Name() << std::endl;
                                reputationItem = reputationItem->NextSiblingElement("reputation");
                            }
                        }
                    }
                    const tinyxml2::XMLElement * material = recipeItem->FirstChildElement("material");
                    while(material!=NULL && ok)
                    {
                        if(material->Attribute("itemId")!=NULL)
                        {
                            const uint16_t &itemId=stringtouint16(material->Attribute("itemId"),&ok2);
                            if(!ok2)
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() material attribute itemId is not a number for crafting recipe file: " << file << ": child->Name(): " << material->Name() << std::endl;
                                break;
                            }
                            uint16_t quantity=1;
                            if(material->Attribute("quantity")!=NULL)
                            {
                                const uint32_t &tempShort=stringtouint32(material->Attribute("quantity"),&ok2);
                                if(ok2)
                                {
                                    if(tempShort>65535)
                                    {
                                        ok=false;
                                        std::cerr << "preload_crafting_recipes() material quantity can't be greater than 65535 for crafting recipe file: " << file << ": child->Name(): " << material->Name() << std::endl;
                                        break;
                                    }
                                    else
                                        quantity=static_cast<uint16_t>(tempShort);
                                }
                                else
                                {
                                    ok=false;
                                    std::cerr << "preload_crafting_recipes() material quantity in not an number for crafting recipe file: " << file << ": child->Name(): " << material->Name() << std::endl;
                                    break;
                                }
                            }
                            if(items.find(itemId)==items.cend())
                            {
                                ok=false;
                                std::cerr << "preload_crafting_recipes() material itemId in not into items list for crafting recipe file: " << file << ": child->Name(): " << material->Name() << std::endl;
                                break;
                            }
                            CatchChallenger::CraftingRecipe::Material newMaterial;
                            newMaterial.item=itemId;
                            newMaterial.quantity=quantity;
                            unsigned int index=0;
                            while(index<recipe.materials.size())
                            {
                                if(recipe.materials.at(index).item==newMaterial.item)
                                    break;
                                index++;
                            }
                            if(index<recipe.materials.size())
                            {
                                ok=false;
                                std::cerr << "id of item already into resource or product list: %1: child->Name(): " << material->Name() << std::endl;
                            }
                            else
                            {
                                if(recipe.doItemId==newMaterial.item)
                                {
                                    std::cerr << "id of item already into resource or product list: %1: child->Name(): " << material->Name() << std::endl;
                                    ok=false;
                                }
                                else
                                    recipe.materials.push_back(newMaterial);
                            }
                        }
                        else
                            std::cerr << "preload_crafting_recipes() material have not attribute itemId for crafting recipe file: " << file << ": child->Name(): " << material->Name() << std::endl;
                        material = material->NextSiblingElement("material");
                    }
                    if(ok)
                    {
                        if(items.find(recipe.itemToLearn)==items.cend())
                        {
                            ok=false;
                            std::cerr << "preload_crafting_recipes() itemToLearn is not into items list for crafting recipe file: " << file << ": child->Name(): " << recipeItem->Name() << std::endl;
                        }
                    }
                    if(ok)
                    {
                        if(items.find(recipe.doItemId)==items.cend())
                        {
                            ok=false;
                            std::cerr << "preload_crafting_recipes() doItemId is not into items list for crafting recipe file: " << file << ": child->Name(): " << recipeItem->Name() << std::endl;
                        }
                    }
                    if(ok)
                    {
                        if(itemToCrafingRecipes.find(recipe.itemToLearn)!=itemToCrafingRecipes.cend())
                        {
                            ok=false;
                            std::cerr << "preload_crafting_recipes() itemToLearn already used to learn another recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << " child->Name(): " << recipeItem->Name() << std::endl;
                        }
                    }
                    if(ok)
                    {
                        if(recipe.itemToLearn==recipe.doItemId)
                        {
                            ok=false;
                            std::cerr << "preload_crafting_recipes() the product of the recipe can't be them self: " << id << ": file: " << file << ": child->Name(): " << recipeItem->Name() << std::endl;
                        }
                    }
                    if(ok)
                    {
                        if(itemToCrafingRecipes.find(recipe.doItemId)!=itemToCrafingRecipes.cend())
                        {
                            ok=false;
                            std::cerr << "preload_crafting_recipes() the product of the recipe can't be a recipe: " << itemToCrafingRecipes.at(recipe.doItemId) << ": file: " << file << ": child->Name(): " << recipeItem->Name() << std::endl;
                        }
                    }
                    if(ok)
                    {
                        if(crafingRecipesMaxId<id)
                            crafingRecipesMaxId=id;
                        crafingRecipes[id]=recipe;
                        itemToCrafingRecipes[recipe.itemToLearn]=id;
                    }
                }
                else
                    std::cerr << "Unable to open the crafting recipe file: " << file << ", id number already set: child->Name(): " << recipeItem->Name() << std::endl;
            }
            else
                std::cerr << "Unable to open the crafting recipe file: " << file << ", id is not a number: child->Name(): " << recipeItem->Name() << std::endl;
        }
        else
            std::cerr << "Unable to open the crafting recipe file: " << file << ", have not the crafting recipe id: child->Name(): " << recipeItem->Name() << std::endl;
        recipeItem = recipeItem->NextSiblingElement("recipe");
    }
    #ifdef EPOLLCATCHCHALLENGERSERVER
    delete domDocument;
    #endif
    return std::pair<std::unordered_map<uint16_t,CraftingRecipe>,std::unordered_map<uint16_t,uint16_t> >(crafingRecipes,itemToCrafingRecipes);
}
#endif
