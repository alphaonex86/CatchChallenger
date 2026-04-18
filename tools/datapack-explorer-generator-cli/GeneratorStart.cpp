#include "GeneratorStart.hpp"
#include "GeneratorMonsters.hpp"
#include "GeneratorItems.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/tinyXML2/tinyxml2.hpp"

#include <sstream>
#include <vector>
#include <algorithm>

namespace GeneratorStart {

void generate()
{
    const std::string page="start.html";
    Helper::setCurrentPage(page);

    const std::string startXml=Helper::datapackPath()+"player/start.xml";
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(startXml.c_str())!=tinyxml2::XML_SUCCESS)
        return;
    const tinyxml2::XMLElement *root=doc.RootElement();
    if(root==nullptr)
        return;

    // Using per-key has/get API instead of whole-container access

    std::ostringstream body;
    int profileIndex=0;

    for(const tinyxml2::XMLElement *startEl=root->FirstChildElement("start");
        startEl!=nullptr; startEl=startEl->NextSiblingElement("start"))
    {
        profileIndex++;
        // Name
        std::string name;
        const tinyxml2::XMLElement *nameEl=startEl->FirstChildElement("name");
        if(nameEl!=nullptr && nameEl->GetText()!=nullptr)
            name=nameEl->GetText();

        // Description
        std::string description;
        const tinyxml2::XMLElement *descEl=startEl->FirstChildElement("description");
        if(descEl!=nullptr && descEl->GetText()!=nullptr)
            description=descEl->GetText();

        // Forced skins
        std::vector<std::string> skins;
        const tinyxml2::XMLElement *skinEl=startEl->FirstChildElement("forcedskin");
        if(skinEl!=nullptr)
        {
            const char *val=skinEl->Attribute("value");
            if(val!=nullptr)
            {
                std::string sv=val;
                size_t pos=0;
                while(pos<sv.size())
                {
                    size_t sc=sv.find(';',pos);
                    if(sc==std::string::npos) sc=sv.size();
                    std::string s=sv.substr(pos,sc-pos);
                    if(!s.empty())
                        skins.push_back(s);
                    pos=sc+1;
                }
            }
        }
        std::sort(skins.begin(),skins.end());

        // Cash
        int cash=0;
        const tinyxml2::XMLElement *cashEl=startEl->FirstChildElement("cash");
        if(cashEl!=nullptr)
            cashEl->QueryIntAttribute("value",&cash);

        // Monster groups
        struct MonsterEntry { uint16_t id; int level; };
        std::vector<std::vector<MonsterEntry>> monsterGroups;
        for(const tinyxml2::XMLElement *mgEl=startEl->FirstChildElement("monstergroup");
            mgEl!=nullptr; mgEl=mgEl->NextSiblingElement("monstergroup"))
        {
            std::vector<MonsterEntry> group;
            for(const tinyxml2::XMLElement *mEl=mgEl->FirstChildElement("monster");
                mEl!=nullptr; mEl=mEl->NextSiblingElement("monster"))
            {
                MonsterEntry e;
                e.id=0;
                e.level=1;
                mEl->QueryUnsignedAttribute("id",(unsigned int*)&e.id);
                mEl->QueryIntAttribute("level",&e.level);
                if(e.id>0)
                    group.push_back(e);
            }
            if(!group.empty())
                monsterGroups.push_back(group);
        }

        // Reputations
        struct RepEntry { std::string type; int level; };
        std::vector<RepEntry> reputations;
        for(const tinyxml2::XMLElement *repEl=startEl->FirstChildElement("reputation");
            repEl!=nullptr; repEl=repEl->NextSiblingElement("reputation"))
        {
            const char *t=repEl->Attribute("type");
            int lv=0;
            repEl->QueryIntAttribute("level",&lv);
            if(t!=nullptr)
                reputations.push_back({t,lv});
        }

        // Items
        struct ItemEntry { uint16_t id; int quantity; };
        std::vector<ItemEntry> items;
        for(const tinyxml2::XMLElement *itemEl=startEl->FirstChildElement("item");
            itemEl!=nullptr; itemEl=itemEl->NextSiblingElement("item"))
        {
            ItemEntry e;
            e.id=0;
            e.quantity=1;
            itemEl->QueryUnsignedAttribute("id",(unsigned int*)&e.id);
            itemEl->QueryIntAttribute("quantity",&e.quantity);
            if(e.id>0)
                items.push_back(e);
        }

        // --- Build HTML ---
        body << "<fieldset><legend><h2><strong>"
             << Helper::htmlEscape(name)
             << "</strong></h2></legend>";
        body << "<b>" << Helper::htmlEscape(description) << "</b><br />";

        // Skins
        if(!skins.empty())
        {
            body << "Skin: <div id=\"skin_preview_" << profileIndex << "\">";
            for(const std::string &sk : skins)
            {
                std::string skinRel="skin/fighter/"+sk+"/front.png";
                std::string skinAbs=Helper::datapackPath()+skinRel;
                if(!Helper::fileExists(skinAbs))
                {
                    skinRel="skin/fighter/"+sk+"/front.gif";
                    skinAbs=Helper::datapackPath()+skinRel;
                }
                std::string published=Helper::publishDatapackFile(skinRel);
                std::string url=Helper::relUrlFrom(page,published);
                body << "<img src=\"" << Helper::htmlEscape(url)
                     << "\" width=\"80\" height=\"80\" alt=\"Front\" style=\"float:left\" />";
            }
            body << "</div><br style=\"clear:both\" />";
        }

        // Cash
        if(cash>0)
            body << "Cash: <i>" << cash << "$</i><br />";

        // Monsters
        if(!monsterGroups.empty())
        {
            body << "Monster: <ul style=\"margin:0px;list-style-type:none;\">";
            for(const auto &group : monsterGroups)
            {
                body << "<li>";
                for(const MonsterEntry &m : group)
                {
                    // Monster name and description
                    std::string mname;
                    std::string mdesc;
                    bool hasMonsterEx=QtDatapackClientLoader::datapackLoader->has_monsterExtra(m.id);
                    if(hasMonsterEx)
                    {
                        const DatapackClientLoader::MonsterExtra &mEx=QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.id);
                        mname=mEx.name;
                        mdesc=mEx.description;
                    }
                    if(mname.empty())
                        mname=Helper::toStringUint(m.id);

                    // Monster front image
                    std::string frontUrl;
                    {
                        std::string absPath;
                        if(hasMonsterEx && !QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.id).frontPath.empty())
                            absPath=QtDatapackClientLoader::datapackLoader->get_monsterExtra(m.id).frontPath;
                        else
                        {
                            std::string base=Helper::datapackPath()+"monsters/"+Helper::toStringUint(m.id)+"/";
                            if(Helper::fileExists(base+"front.png"))
                                absPath=base+"front.png";
                            else if(Helper::fileExists(base+"front.gif"))
                                absPath=base+"front.gif";
                        }
                        if(!absPath.empty())
                        {
                            std::string rel=Helper::relativeFromDatapack(absPath);
                            std::string pub=Helper::publishDatapackFile(rel);
                            frontUrl=Helper::relUrlFrom(page,pub);
                        }
                    }

                    // Monster page URL
                    std::string monsterPage=Helper::relUrlFrom(page,
                        GeneratorMonsters::relativePathForMonster(m.id));

                    body << "<div style=\"float:left;margin:5px;\">"
                         << "<a href=\"" << Helper::htmlEscape(monsterPage) << "\">";
                    if(!frontUrl.empty())
                        body << "<img src=\"" << Helper::htmlEscape(frontUrl)
                             << "\" width=\"80\" height=\"80\" alt=\""
                             << Helper::htmlEscape(mname) << "\" title=\""
                             << Helper::htmlEscape(mdesc) << "\" />";
                    body << "<br /><b>" << Helper::htmlEscape(mname) << "</b>"
                         << " level <i>" << m.level << "</i></a></div>"
                         << "<br style=\"clear:both\" />";
                }
                body << "</li>";
            }
            body << "</ul>";
        }

        // Reputations
        if(!reputations.empty())
        {
            body << "Reputations: <ul style=\"margin:0px;\">";
            for(const RepEntry &r : reputations)
            {
                std::string repText;
                if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(r.type))
                {
                    const DatapackClientLoader::ReputationExtra &rEx=QtDatapackClientLoader::datapackLoader->get_reputationExtra(r.type);
                    if(r.level>=0 && (size_t)r.level<rEx.reputation_positive.size())
                        repText=rEx.reputation_positive[r.level];
                    else if(r.level<0 && (size_t)(-r.level)<rEx.reputation_negative.size())
                        repText=rEx.reputation_negative[-r.level];
                }
                if(repText.empty())
                    repText=r.type+" level "+Helper::toStringInt(r.level);
                body << "<li>" << Helper::htmlEscape(repText) << "</li>";
            }
            body << "</ul>";
        }

        // Items
        {
            // Collect items that have extras
            std::vector<ItemEntry> validItems;
            for(const ItemEntry &item : items)
            {
                if(QtDatapackClientLoader::datapackLoader->has_itemExtra(item.id))
                    validItems.push_back(item);
            }
            if(!validItems.empty())
            {
                body << "Items: <ul style=\"margin:0px;\">";
                for(const ItemEntry &item : validItems)
                {
                    const DatapackClientLoader::ItemExtra &iEx=QtDatapackClientLoader::datapackLoader->get_itemExtra(item.id);
                    const std::string &iname=iEx.name;
                    const std::string &idesc=iEx.description;
                    const std::string &iimage=iEx.imagePath;

                    std::string itemPage=Helper::relUrlFrom(page,
                        GeneratorItems::relativePathForItem(item.id));

                    std::string imgUrl;
                    if(!iimage.empty())
                    {
                        std::string rel=Helper::relativeFromDatapack(iimage);
                        std::string pub=Helper::publishDatapackFile(rel);
                        imgUrl=Helper::relUrlFrom(page,pub);
                    }

                    body << "<li><a href=\"" << Helper::htmlEscape(itemPage)
                         << "\" title=\"" << Helper::htmlEscape(iname) << "\">";
                    if(!imgUrl.empty())
                        body << "<img src=\"" << Helper::htmlEscape(imgUrl)
                             << "\" width=\"24\" height=\"24\" alt=\""
                             << Helper::htmlEscape(idesc) << "\" title=\""
                             << Helper::htmlEscape(idesc) << "\" />";
                    if(item.quantity>1)
                        body << item.quantity << " ";
                    body << Helper::htmlEscape(iname) << "</a></li>";
                }
                body << "</ul>";
            }
        }

        body << "</fieldset>";
    }

    Helper::writeHtml(page,"Starters",body.str());
}

} // namespace GeneratorStart
