#include "GeneratorItems.hpp"
#include "GeneratorMonsters.hpp"
#include "GeneratorCrafting.hpp"
#include "GeneratorMaps.hpp"
#include "GeneratorPlants.hpp"
#include "GeneratorSkills.hpp"
#include "GeneratorBuffs.hpp"
#include "GeneratorTypes.hpp"
#include "GeneratorQuests.hpp"
#include "Helper.hpp"
#include "MapStore.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"
#include "../../general/base/CommonDatapackServerSpec.hpp"
#include "../../general/base/CommonMap/CommonMap.hpp"

#include <sstream>
#include <map>
#include <set>
#include <cstdlib>

#include <sys/types.h>
#include <dirent.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>

namespace GeneratorItems {

// ── Image/Path helpers ──

static std::string toRelativeItemImage(const std::string &imagePath)
{
    if(imagePath.empty())
        return std::string();
    std::string rel=Helper::relativeFromDatapack(imagePath);
    if(rel.compare(0,6,"items/")==0)
        return rel;
    size_t pos=rel.find("/items/");
    if(pos!=std::string::npos)
        return rel.substr(pos+1);
    size_t slash=rel.find_last_of('/');
    if(slash!=std::string::npos)
        return std::string("items/")+rel.substr(slash+1);
    return std::string("items/")+rel;
}

static void splitImagePath(const std::string &imagePath, std::string &folder, std::string &stem)
{
    folder.clear();
    stem.clear();
    std::string rel=toRelativeItemImage(imagePath);
    if(rel.empty())
        return;
    if(rel.compare(0,6,"items/")==0)
        rel=rel.substr(6);
    size_t slash=rel.find_last_of('/');
    if(slash!=std::string::npos)
    {
        folder=rel.substr(0,slash);
        stem=rel.substr(slash+1);
    }
    else
        stem=rel;
    size_t dot=stem.find_last_of('.');
    if(dot!=std::string::npos)
        stem=stem.substr(0,dot);
}

static std::unordered_map<uint16_t,std::string> s_itemPath;

std::string relativePathForItem(uint16_t id)
{
    auto it=s_itemPath.find(id);
    if(it!=s_itemPath.end())
        return it->second;
    std::string folder, stem;
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(id))
    {
        const DatapackClientLoader::ItemExtra &ex=QtDatapackClientLoader::datapackLoader->get_itemExtra(id);
        splitImagePath(ex.imagePath,folder,stem);
        if(!ex.name.empty())
            stem=Helper::textForUrl(ex.name);
    }
    if(folder.empty())
        folder="other";
    if(stem.empty())
        stem=Helper::toStringUint(id);
    return "items/"+folder+"/"+stem+".html";
}

static void buildItemPaths()
{
    s_itemPath.clear();
    std::set<std::string> used;
    std::vector<uint16_t> ids;
    for(CATCHCHALLENGER_TYPE_ITEM id=1;id<=CatchChallenger::CommonDatapack::commonDatapack.get_itemMaxId();id++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_item(id))
            continue;
        ids.push_back(id);
    }
    std::sort(ids.begin(),ids.end());
    for(uint16_t id : ids)
    {
        std::string folder, imgStem;
        std::string name;
        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(id))
        {
            const DatapackClientLoader::ItemExtra &ex=QtDatapackClientLoader::datapackLoader->get_itemExtra(id);
            splitImagePath(ex.imagePath,folder,imgStem);
            name=ex.name;
        }
        if(folder.empty())
            folder="other";
        std::string stem=imgStem;
        if(stem.empty())
        {
            std::string nStem=Helper::textForUrl(name);
            if(!nStem.empty() && nStem!="untitled")
                stem=nStem;
        }
        if(stem.empty())
            stem=Helper::toStringUint(id);
        std::string base="items/"+folder+"/"+stem;
        std::string rel=base+".html";
        int n=2;
        while(used.count(rel)>0)
        {
            rel=base+"-"+Helper::toStringInt(n)+".html";
            n++;
        }
        used.insert(rel);
        s_itemPath[id]=rel;
    }
}

static std::string itemImageUrlFrom(const std::string &fromPage, const std::string &imagePath)
{
    if(imagePath.empty())
        return std::string();
    std::string rel=toRelativeItemImage(imagePath);
    std::string published=Helper::publishDatapackFile(rel);
    return Helper::relUrlFrom(fromPage,published);
}

// ── Reverse lookup caches ──

struct ShopOnMap  { size_t setIdx; size_t mapId; uint32_t price; };
struct PickOnMap  { size_t setIdx; size_t mapId; };
struct DropOnMap  { size_t setIdx; size_t mapId; uint32_t qmin; uint32_t qmax; uint8_t luck; };
struct FightReward{ size_t setIdx; size_t mapId; uint8_t botId; uint32_t qty; };

static std::unordered_map<uint16_t,std::vector<ShopOnMap>>   s_itemInShops;
static std::unordered_map<uint16_t,std::vector<PickOnMap>>   s_itemOnMap;
static std::unordered_map<uint16_t,std::vector<DropOnMap>>   s_itemAsDrop;
static std::unordered_map<uint16_t,std::vector<FightReward>> s_itemAsFightReward;
static std::unordered_map<uint16_t,std::vector<uint16_t>>    s_itemAsCraftMaterial;
static std::unordered_map<uint16_t,std::vector<uint16_t>>    s_itemProducedByCraft;
static std::unordered_map<uint16_t,std::unordered_map<uint16_t,uint16_t>> s_itemEvolution;

struct SkillOfMonsterEntry { uint16_t monsterId; uint16_t skillId; uint8_t skillLevel; };
static std::unordered_map<uint16_t, std::vector<SkillOfMonsterEntry>> s_itemToSkillOfMonster;

struct MonsterDropEntry { uint16_t monsterId; uint32_t qmin; uint32_t qmax; uint8_t luck; };
static std::unordered_map<uint16_t, std::vector<MonsterDropEntry>> s_itemToMonsterDrop;

struct QuestRewardEntry { uint16_t questId; uint32_t quantity; };
static std::unordered_map<uint16_t, std::vector<QuestRewardEntry>> s_itemToQuestReward;

struct QuestStepEntry { uint16_t questId; uint32_t quantity; uint16_t monsterId; uint8_t rate; bool hasMonster; };
static std::unordered_map<uint16_t, std::vector<QuestStepEntry>> s_itemToQuestStep;

static std::unordered_map<uint16_t, uint16_t> s_itemIsRecipe;
static std::unordered_map<uint16_t, std::vector<uint16_t>> s_doItemToRecipeItems;
static std::unordered_map<uint16_t, std::vector<uint16_t>> s_materialToRecipeItems;

struct IndustryRef { size_t setIdx; size_t mapIdx; size_t industryIdx; uint32_t quantity; };
static std::unordered_map<uint16_t, std::vector<IndustryRef>> s_itemConsumedByIndustry;
static std::unordered_map<uint16_t, std::vector<IndustryRef>> s_itemProducedByIndustryMap;

static void buildReverseLookups()
{
    s_itemInShops.clear();
    s_itemOnMap.clear();
    s_itemAsDrop.clear();
    s_itemAsFightReward.clear();
    s_itemAsCraftMaterial.clear();
    s_itemProducedByCraft.clear();
    s_itemEvolution.clear();
    s_itemToSkillOfMonster.clear();
    s_itemToMonsterDrop.clear();
    s_itemToQuestReward.clear();
    s_itemToQuestStep.clear();
    s_itemIsRecipe.clear();
    s_doItemToRecipeItems.clear();
    s_materialToRecipeItems.clear();
    s_itemConsumedByIndustry.clear();
    s_itemProducedByIndustryMap.clear();

    const auto &sets=MapStore::sets();
    for(size_t si=0; si<sets.size(); ++si)
    {
        const auto &set=sets[si];
        for(size_t mi=0; mi<set.mapList.size(); ++mi)
        {
            const CatchChallenger::CommonMap &m=set.mapList[mi];
            for(const auto &sp : m.shops)
                for(const auto &ip : sp.second.items)
                    s_itemInShops[ip.first].push_back({si,mi,ip.second});
            for(const auto &ip : m.items)
                s_itemOnMap[ip.second.item].push_back({si,mi});
            for(const auto &d : m.monsterDrops)
                s_itemAsDrop[d.item].push_back({si,mi,d.quantity_min,d.quantity_max,d.luck});
            for(const auto &bp : m.botFights)
                for(const auto &reward : bp.second.items)
                    s_itemAsFightReward[reward.id].push_back({si,mi,bp.first,reward.quantity});
            for(size_t ii=0; ii<m.industries.size(); ++ii)
            {
                const auto &ind=m.industries[ii];
                for(const auto &res : ind.resources)
                    s_itemConsumedByIndustry[res.item].push_back({si,mi,ii,res.quantity});
                for(const auto &prod : ind.products)
                    s_itemProducedByIndustryMap[prod.item].push_back({si,mi,ii,prod.quantity});
            }
        }
    }

    for(uint16_t rid=1;rid<=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipes_size()+1;rid++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_craftingRecipe(rid))
            continue;
        const CatchChallenger::CraftingRecipe &r=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipe(rid);
        s_itemProducedByCraft[r.doItemId].push_back(rid);
        for(const auto &mat : r.materials)
            s_itemAsCraftMaterial[mat.item].push_back(rid);
        s_itemIsRecipe[r.itemToLearn]=rid;
        s_doItemToRecipeItems[r.doItemId].push_back(r.itemToLearn);
        for(const auto &mat : r.materials)
            s_materialToRecipeItems[mat.item].push_back(r.itemToLearn);
    }

    for(CATCHCHALLENGER_TYPE_ITEM evoItemId=1;evoItemId<=CatchChallenger::CommonDatapack::commonDatapack.get_itemMaxId();evoItemId++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_evolutionItem(evoItemId))
            continue;
        // iterate monster IDs that this item can evolve
        for(CATCHCHALLENGER_TYPE_MONSTER evoMonsterId=1;evoMonsterId<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();evoMonsterId++) {
            if(!CatchChallenger::CommonDatapack::commonDatapack.has_evolutionItemForMonster(evoItemId,evoMonsterId))
                continue;
            s_itemEvolution[evoItemId][evoMonsterId]=CatchChallenger::CommonDatapack::commonDatapack.get_evolutionItemForMonster(evoItemId,evoMonsterId);
        }
    }

    for(CATCHCHALLENGER_TYPE_MONSTER monsterId=1;monsterId<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();monsterId++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
            continue;
        const CatchChallenger::Monster &monsterEntry=CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId);
        for(const std::pair<const uint16_t,CatchChallenger::Monster::AttackToLearnByItem> &lbi : monsterEntry.learnByItem)
            s_itemToSkillOfMonster[lbi.first].push_back({monsterId,lbi.second.learnSkill,lbi.second.learnSkillLevel});
    }

    for(CATCHCHALLENGER_TYPE_MONSTER dropMonsterId=1;dropMonsterId<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();dropMonsterId++) {
        if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_monsterDrop(dropMonsterId))
            continue;
        const std::vector<CatchChallenger::MonsterDrops> &drops=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_monsterDrop(dropMonsterId);
        for(const CatchChallenger::MonsterDrops &d : drops)
            s_itemToMonsterDrop[d.item].push_back({dropMonsterId,d.quantity_min,d.quantity_max,d.luck});
    }

    for(CATCHCHALLENGER_TYPE_QUEST questId=1;questId<=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quests_size()+1;questId++) {
        if(!CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.has_quest(questId))
            continue;
        const CatchChallenger::Quest &questEntry=CatchChallenger::CommonDatapackServerSpec::commonDatapackServerSpec.get_quest(questId);
        for(const CatchChallenger::Quest::Item &ri : questEntry.rewards.items)
            s_itemToQuestReward[ri.item].push_back({questEntry.id,ri.quantity});
        for(const CatchChallenger::Quest::Step &step : questEntry.steps)
        {
            for(const CatchChallenger::Quest::Item &si : step.requirements.items)
                s_itemToQuestStep[si.item].push_back({questEntry.id,si.quantity,0,0,false});
            for(const CatchChallenger::Quest::ItemMonster &im : step.itemsMonster)
                for(uint16_t mId : im.monsters)
                    s_itemToQuestStep[im.item].push_back({questEntry.id,1,mId,im.rate,true});
        }
    }
}

// ── HTML generation helpers ──

static std::string ucfirst(const std::string &s)
{
    if(s.empty()) return s;
    std::string r=s;
    r[0]=toupper((unsigned char)r[0]);
    return r;
}

static std::string monsterSmallImageUrl(const std::string &fromPage, uint16_t monsterId)
{
    std::string rel="monsters/"+Helper::toStringUint(monsterId)+"/small.png";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    rel="monsters/"+Helper::toStringUint(monsterId)+"/small.gif";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    return std::string();
}

static std::string monsterFrontImageUrl(const std::string &fromPage, uint16_t monsterId)
{
    std::string rel="monsters/"+Helper::toStringUint(monsterId)+"/front.png";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    rel="monsters/"+Helper::toStringUint(monsterId)+"/front.gif";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    return std::string();
}

static std::string plantSpriteUrl(const std::string &fromPage, uint8_t plantId)
{
    std::string rel="plants/"+Helper::toStringUint(plantId)+".png";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    rel="plants/"+Helper::toStringUint(plantId)+".gif";
    if(Helper::fileExists(Helper::datapackPath()+rel))
        return Helper::relUrlFrom(fromPage,Helper::publishDatapackFile(rel));
    return std::string();
}

static void writeItemLinkTable(std::ostringstream &body, const std::string &fromPage, uint16_t itemId, uint32_t quantity=0)
{
    std::string iname;
    std::string iimgUrl;
    if(QtDatapackClientLoader::datapackLoader->has_itemExtra(itemId)) {
        const DatapackClientLoader::ItemExtra &extra=QtDatapackClientLoader::datapackLoader->get_itemExtra(itemId);
        iname=extra.name;
        iimgUrl=itemImageUrlFrom(fromPage,extra.imagePath);
    } else {
        iname="Item #"+Helper::toStringUint(itemId);
    }

    body << "<a href=\"" << Helper::htmlEscape(Helper::relUrlFrom(fromPage,relativePathForItem(itemId)))
         << "\" title=\"" << Helper::htmlEscape(iname) << "\">\n";
    body << "<table><tr><td>\n";
    if(!iimgUrl.empty())
        body << "<img src=\"" << Helper::htmlEscape(iimgUrl) << "\" width=\"24\" height=\"24\" alt=\""
             << Helper::htmlEscape(iname) << "\" title=\"" << Helper::htmlEscape(iname) << "\" />\n";
    body << "</td><td>\n";
    if(quantity>1)
        body << quantity << "x ";
    body << Helper::htmlEscape(iname) << "</td></tr></table>\n";
    body << "</a>\n";
}

static void writeMonsterCells(std::ostringstream &body, const std::string &fromPage, uint16_t monsterId)
{
    std::string mname;
    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId))
        mname=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId).name;
    else
        mname="Monster #"+Helper::toStringUint(monsterId);
    std::string link=Helper::relUrlFrom(fromPage,GeneratorMonsters::relativePathForMonster(monsterId));
    std::string smallUrl=monsterSmallImageUrl(fromPage,monsterId);

    body << "<td>\n";
    if(!smallUrl.empty())
        body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link)
             << "\"><img src=\"" << Helper::htmlEscape(smallUrl) << "\" width=\"32\" height=\"32\" alt=\""
             << Helper::htmlEscape(mname) << "\" title=\"" << Helper::htmlEscape(mname)
             << "\" /></a></div>\n";
    body << "</td>\n<td><a href=\"" << Helper::htmlEscape(link) << "\">"
         << Helper::htmlEscape(mname) << "</a></td>\n";
}

static void writeMonsterTypeLabels(std::ostringstream &body, const std::string &fromPage, uint16_t monsterId)
{
    if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId)) { body << "<td>&nbsp;</td>\n"; return; }
    const CatchChallenger::Monster &monsterData=CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId);

    body << "<td><div class=\"type_label_list\">";
    bool first=true;
    for(uint8_t typeId : monsterData.type)
    {
        if(!first) body << " ";
        first=false;
        std::string tname;
        if(QtDatapackClientLoader::datapackLoader->has_typeExtra(typeId))
            tname=ucfirst(QtDatapackClientLoader::datapackLoader->get_typeExtra(typeId).name);
        else
            tname="Type "+Helper::toStringUint(typeId);
        body << "<span class=\"type_label type_label_" << (unsigned)typeId << "\"><a href=\""
             << Helper::htmlEscape(Helper::relUrlFrom(fromPage,GeneratorTypes::relativePathForType(typeId)))
             << "\">" << Helper::htmlEscape(tname) << "</a></span>\n";
    }
    body << "</div></td>\n";
}

static std::string reputationName(uint8_t repId)
{
    const auto &reps=CatchChallenger::CommonDatapack::commonDatapack.get_reputation();
    if(repId<reps.size())
        return reps[repId].name;
    return "Reputation #"+Helper::toStringUint(repId);
}

static std::string reputationLevelToText(uint8_t repId, uint8_t level, bool positif)
{
    const std::vector<CatchChallenger::Reputation> &reps=CatchChallenger::CommonDatapack::commonDatapack.get_reputation();
    std::string name=reputationName(repId);
    std::string levelText;
    if(repId<reps.size())
    {
        if(QtDatapackClientLoader::datapackLoader->has_reputationExtra(reps[repId].name))
        {
            const DatapackClientLoader::ReputationExtra &repExtraEntry=QtDatapackClientLoader::datapackLoader->get_reputationExtra(reps[repId].name);
            if(positif && level<repExtraEntry.reputation_positive.size())
                levelText=repExtraEntry.reputation_positive[level];
            else if(!positif && level<repExtraEntry.reputation_negative.size())
                levelText=repExtraEntry.reputation_negative[level];
        }
    }
    if(levelText.empty())
        return "Level "+Helper::toStringUint(level)+" in "+name;
    return "Level "+Helper::toStringUint(level)+" in "+name+" ("+levelText+")";
}

static void writeReputationRequirements(std::ostringstream &body, const std::vector<CatchChallenger::ReputationRequirements> &reqs)
{
    for(const auto &req : reqs)
        body << reputationLevelToText(req.reputationId,req.level,req.positif) << "<br />\n";
}

static void writeReputationRewards(std::ostringstream &body, const std::vector<CatchChallenger::ReputationRewards> &rews)
{
    for(const auto &rew : rews)
    {
        if(rew.point<0)
            body << "Less reputation in: " << reputationName(rew.reputationId);
        else
            body << "More reputation in: " << reputationName(rew.reputationId);
    }
}

static const std::string &mapPathOf(size_t si, size_t mi)
{
    static const std::string empty;
    const auto &sets=MapStore::sets();
    if(si>=sets.size()) return empty;
    if(mi>=sets[si].mapPaths.size()) return empty;
    return sets[si].mapPaths[mi];
}

static std::string mapDisplayName(size_t si, size_t mi)
{
    const std::string &path=mapPathOf(si,mi);
    if(path.empty()) return "Unknown map";
    size_t slash=path.find_last_of('/');
    std::string name=(slash!=std::string::npos)?path.substr(slash+1):path;
    if(name.size()>4 && name.substr(name.size()-4)==".tmx")
        name=name.substr(0,name.size()-4);
    return name;
}

// ── generate() ──

void generate()
{
    // Container references removed - using per-key accessors instead

    buildReverseLookups();
    buildItemPaths();

    // Scan items XML for <regeneration> tags
    std::unordered_set<uint16_t> s_itemHasRegeneration;
    {
        std::string itemsDir=Helper::datapackPath()+"items/";
        DIR *d=::opendir(itemsDir.c_str());
        if(d!=nullptr)
        {
            struct dirent *ent;
            while((ent=::readdir(d))!=nullptr)
            {
                std::string fname=ent->d_name;
                if(fname.size()<5 || fname.substr(fname.size()-4)!=".xml")
                    continue;
                std::string content;
                if(!Helper::readFile(itemsDir+fname,content)) continue;
                size_t pos=0;
                while((pos=content.find("<item ",pos))!=std::string::npos)
                {
                    size_t itemEnd=content.find("</item>",pos);
                    if(itemEnd==std::string::npos) itemEnd=content.size();
                    std::string block=content.substr(pos,itemEnd-pos);
                    if(block.find("<regeneration")!=std::string::npos)
                    {
                        size_t idPos=block.find("id=\"");
                        if(idPos!=std::string::npos)
                        {
                            uint16_t xid=(uint16_t)atoi(block.c_str()+idPos+4);
                            if(xid>0) s_itemHasRegeneration.insert(xid);
                        }
                    }
                    pos=itemEnd+1;
                }
            }
            ::closedir(d);
        }
    }

    // Build industry product lookup (for index gradient)
    std::unordered_map<uint16_t,bool> s_itemProducedByIndustry;
    {
        const auto &sets=MapStore::sets();
        for(size_t si=0;si<sets.size();++si)
            for(size_t mi=0;mi<sets[si].mapList.size();++mi)
                for(const auto &ind : sets[si].mapList[mi].industries)
                    for(const auto &prod : ind.products)
                        s_itemProducedByIndustry[prod.item]=true;
    }

    // Build craft recipe item lookup
    std::unordered_set<uint16_t> s_itemIsCraftRecipe;
    {
        for(uint16_t recipeId=1;recipeId<=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipes_size()+1;recipeId++) {
            if(!CatchChallenger::CommonDatapack::commonDatapack.has_craftingRecipe(recipeId))
                continue;
            if(CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipe(recipeId).itemToLearn>0)
                s_itemIsCraftRecipe.insert(CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipe(recipeId).itemToLearn);
        }
    }

    // Determine group for each item (matching PHP)
    /* lambda uses per-key accessors now */
    std::function<std::string(uint16_t)> getGroup=[&](uint16_t id) -> std::string {
        if(CatchChallenger::CommonDatapack::commonDatapack.has_itemToLearn(id))
            return "Learn";
        if(s_itemIsCraftRecipe.find(id)!=s_itemIsCraftRecipe.cend())
            return "Crafting";
        if(QtDatapackClientLoader::datapackLoader->has_itemToPlant(id))
            return "Plant";
        if(CatchChallenger::CommonDatapack::commonDatapack.has_monsterItemEffect(id) ||
           s_itemHasRegeneration.find(id)!=s_itemHasRegeneration.cend())
            return "Regeneration";
        if(CatchChallenger::CommonDatapack::commonDatapack.has_trap(id))
            return "Trap";
        if(s_itemEvolution.find(id)!=s_itemEvolution.cend())
            return "Evolution";
        return "Items";
    };

    // Group items for index
    std::map<std::string,std::vector<uint16_t>> grouped;
    std::vector<std::string> groupOrder;
    std::vector<uint16_t> sortedIds;
    for(CATCHCHALLENGER_TYPE_ITEM sortId=1;sortId<=CatchChallenger::CommonDatapack::commonDatapack.get_itemMaxId();sortId++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_item(sortId))
            continue;
        sortedIds.push_back(sortId);
    }
    std::sort(sortedIds.begin(),sortedIds.end());
    for(uint16_t id : sortedIds)
    {
        std::string g=getGroup(id);
        if(grouped.find(g)==grouped.end())
            groupOrder.push_back(g);
        grouped[g].push_back(id);
    }

    // Build gradient for index
    auto buildGradient=[&](uint16_t id) -> std::string {
        std::vector<std::string> colors;
        if(s_itemInShops.find(id)!=s_itemInShops.cend())
            colors.push_back("#e5eaff");
        bool fromWild=(s_itemToMonsterDrop.find(id)!=s_itemToMonsterDrop.cend());
        bool fromCraft=(s_itemProducedByCraft.find(id)!=s_itemProducedByCraft.cend());
        if(fromWild || fromCraft)
            colors.push_back("#e0ffdd");
        bool fromIndustry=(s_itemProducedByIndustry.find(id)!=s_itemProducedByIndustry.cend());
        if(fromIndustry)
            colors.push_back("#fbfdd3");
        if(s_itemOnMap.find(id)!=s_itemOnMap.cend())
            colors.push_back("#ffefdb");
        if(s_itemAsFightReward.find(id)!=s_itemAsFightReward.cend())
            colors.push_back("#ffe5e5");
        if(colors.empty())
            return std::string();
        std::ostringstream g;
        g << " style=\"background: repeating-linear-gradient(-45deg, ";
        int px=0;
        for(size_t i=0;i<colors.size();i++)
        {
            if(i>0) g << ", ";
            g << colors[i] << " " << px << "px, " << colors[i] << " " << (px+4) << "px";
            px+=5;
        }
        g << ");\"";
        return g.str();
    };

    std::ostringstream indexBody;

    // ── Per-item pages ──
    for(CATCHCHALLENGER_TYPE_ITEM id=1;id<=CatchChallenger::CommonDatapack::commonDatapack.get_itemMaxId();id++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_item(id))
            continue;
        const CatchChallenger::Item &item=CatchChallenger::CommonDatapack::commonDatapack.get_item(id);

        std::string name;
        std::string description;
        std::string imagePath;
        if(QtDatapackClientLoader::datapackLoader->has_itemExtra(id))
        {
            const DatapackClientLoader::ItemExtra &itxExtra=QtDatapackClientLoader::datapackLoader->get_itemExtra(id);
            name=itxExtra.name;
            description=itxExtra.description;
            imagePath=itxExtra.imagePath;
        }
        if(name.empty())
            name="Item #"+Helper::toStringUint(id);

        std::string rel=relativePathForItem(id);
        Helper::setCurrentPage(rel);
        std::string imgUrl=itemImageUrlFrom(rel,imagePath);

        std::ostringstream body;

        // ── 1. Header ──
        body << "<div class=\"map item_details\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name) << "</h1>\n";
        body << "<h2>#" << id << "</h2>\n";
        body << "</div>\n";

        // ── 2. Image ──
        body << "<div class=\"value datapackscreenshot\"><center>\n";
        if(!imgUrl.empty())
            body << "<img src=\"" << Helper::htmlEscape(imgUrl)
                 << "\" width=\"24\" height=\"24\" alt=\"" << Helper::htmlEscape(name)
                 << "\" title=\"" << Helper::htmlEscape(name) << "\" />\n";
        body << "</center></div>\n";

        // ── 3. Price ──
        if(item.price>0)
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Price</div><div class=\"value\">"
                 << item.price << "$</div></div>\n";
        else
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Price</div><div class=\"value\">Can't be sold</div></div>\n";

        // ── 4. Description ──
        if(!description.empty())
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Description</div><div class=\"value\">"
                 << Helper::htmlEscape(description) << "</div></div>\n";

        // ── 5. Trap ──
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.has_trap(id))
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Trap</div><div class=\"value\">Bonus rate: "
                     << CatchChallenger::CommonDatapack::commonDatapack.get_trap(id).bonus_rate << "x</div></div>\n";
        }

        // ── 6. Repel ──
        {
            if(CatchChallenger::CommonDatapack::commonDatapack.has_repel(id))
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Repel</div><div class=\"value\">Repel the monsters during "
                     << CatchChallenger::CommonDatapack::commonDatapack.get_repel(id) << " steps</div></div>\n";
        }

        // ── 7. Skill (if sameuniqueskill) ──
        bool sameuniqueskill=true;
        std::unordered_map<uint16_t, std::vector<SkillOfMonsterEntry>>::iterator skillOfMonsterIt=s_itemToSkillOfMonster.find(id);
        if(skillOfMonsterIt!=s_itemToSkillOfMonster.cend() && !skillOfMonsterIt->second.empty())
        {
            uint16_t lastskill=skillOfMonsterIt->second[0].skillId;
            for(size_t i=1;i<skillOfMonsterIt->second.size();++i)
            {
                if(skillOfMonsterIt->second[i].skillId!=lastskill)
                { sameuniqueskill=false; break; }
            }
            if(sameuniqueskill)
            {
                const auto &e=skillOfMonsterIt->second[0];
                if(QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(e.skillId))
                {
                    body << "<div class=\"subblock\"><div class=\"valuetitle\">Skill</div><div class=\"value\">\n";
                    body << "<table><td><a href=\""
                         << Helper::htmlEscape(Helper::relUrl(GeneratorSkills::relativePathForSkill(e.skillId)))
                         << "\">" << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(e.skillId).name);
                    if(e.skillLevel>1)
                        body << " at level " << (unsigned)e.skillLevel;
                    body << "</a></td>\n";
                    if(CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(e.skillId))
                    {
                        uint8_t typeId=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(e.skillId).type;
                        if(QtDatapackClientLoader::datapackLoader->has_typeExtra(typeId))
                            body << "<td><span class=\"type_label type_label_" << (unsigned)typeId
                                 << "\"><a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorTypes::relativePathForType(typeId)))
                                 << "\">" << Helper::htmlEscape(ucfirst(QtDatapackClientLoader::datapackLoader->get_typeExtra(typeId).name)) << "</a></span></td>\n";
                        else
                            body << "<td>&nbsp;</td>\n";
                    }
                    else
                        body << "<td>&nbsp;</td>\n";
                    body << "</table>\n";
                    body << "</div></div>\n";
                }
            }
        }

        // ── 8. Plant ──
        {
            if(QtDatapackClientLoader::datapackLoader->has_itemToPlant(id))
            {
                uint8_t plantId=QtDatapackClientLoader::datapackLoader->get_itemToPlant(id);
                if(CatchChallenger::CommonDatapack::commonDatapack.has_plant(plantId))
                {
                    std::string sprUrl=plantSpriteUrl(rel,plantId);
                    if(!sprUrl.empty())
                    {
                        uint32_t total=CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).sprouted_seconds+CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).taller_seconds
                                       +CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).flowering_seconds+CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).fruits_seconds;
                        body << "<div class=\"subblock\"><div class=\"valuetitle\">Plant</div><div class=\"value\">\n";
                        body << "After <b>" << (total/60) << "</b> minutes you will have <b>"
                             << CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).fix_quantity << "</b> fruits\n";
                        body << "<table class=\"item_list item_list_type_normal\">\n"
                             << "\t\t\t\t<tr class=\"item_list_title item_list_title_type_normal\">\n"
                             << "\t\t\t\t\t<th>Seed</th>\n"
                             << "\t\t\t\t\t<th>Sprouted</th>\n"
                             << "\t\t\t\t\t<th>Taller</th>\n"
                             << "\t\t\t\t\t<th>Flowering</th>\n"
                             << "\t\t\t\t\t<th>Fruits</th>\n"
                             << "\t\t\t\t</tr><tr class=\"value\">\n";
                        for(int stage=0;stage<5;++stage)
                        {
                            int xPos=-16*stage;
                            body << "<td><center><div style=\"width:16px;height:32px;background-image:url('"
                                 << Helper::htmlEscape(sprUrl)
                                 << "');background-repeat:no-repeat;background-position:"
                                 << xPos << "px 0px;\"></div></center></td>\n";
                        }
                        body << "</tr><tr>\n"
                             << "\t\t\t\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                             << "\t\t\t\t</tr>\n"
                             << "\t\t\t\t</table>\n";
                        body << "</div></div>\n";
                    }

                    // Plant requirements
                    if(!CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).requirements.reputation.empty())
                    {
                        body << "<div class=\"subblock\"><div class=\"valuetitle\">Requirements</div><div class=\"value\">\n";
                        writeReputationRequirements(body,CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).requirements.reputation);
                        body << "</div></div>\n";
                    }

                    // Plant rewards
                    if(!CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).rewards.reputation.empty())
                    {
                        body << "<div class=\"subblock\"><div class=\"valuetitle\">Rewards</div><div class=\"value\">\n";
                        writeReputationRewards(body,CatchChallenger::CommonDatapack::commonDatapack.get_plant(plantId).rewards.reputation);
                        body << "</div></div>\n";
                    }
                }
            }
        }

        // ── 9. Effect ──
        {
            bool hasRegen=false, hasBuff=false;
            uint32_t regenHp=0;
            uint8_t buffId=0;
            if(CatchChallenger::CommonDatapack::commonDatapack.has_monsterItemEffect(id))
            {
                for(const CatchChallenger::MonsterItemEffect &eff : CatchChallenger::CommonDatapack::commonDatapack.get_monsterItemEffect(id))
                {
                    if(eff.type==CatchChallenger::MonsterItemEffectType_AddHp)
                    { hasRegen=true; regenHp=eff.data.hp; }
                    else if(eff.type==CatchChallenger::MonsterItemEffectType_RemoveBuff)
                    { hasBuff=true; buffId=eff.data.buff; }
                }
            }
            if(!hasRegen && s_itemHasRegeneration.count(id)>0)
            { hasRegen=true; regenHp=0; }

            if(hasRegen || hasBuff)
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Effect</div><div class=\"value\"><ul>\n";
                if(hasRegen)
                {
                    if(regenHp==0)
                        body << "<li>Regenerate all the hp</li>\n";
                    else
                        body << "<li>Regenerate " << regenHp << " hp</li>\n";
                }
                if(hasBuff)
                {
                    if(QtDatapackClientLoader::datapackLoader->has_monsterBuffExtra((uint16_t)buffId))
                    {
                        body << "<li>Remove the buff:";
                        body << "<center><table><td>\n";
                        std::string buffImgRel="monsters/buff/"+Helper::toStringUint(buffId)+".png";
                        if(Helper::fileExists(Helper::datapackPath()+buffImgRel))
                            body << "<img src=\"" << Helper::htmlEscape(Helper::relUrlFrom(rel,Helper::publishDatapackFile(buffImgRel)))
                                 << "\" alt=\"\" width=\"16\" height=\"16\" />\n";
                        else
                            body << "&nbsp;\n";
                        body << "</td>\n";
                        body << "<td><a href=\""
                             << Helper::htmlEscape(Helper::relUrl(GeneratorBuffs::relativePathForBuff(buffId)))
                             << "\">" << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra((uint16_t)buffId).name) << "</a></td>\n";
                        body << "</table></center>\n";
                        body << "</li>\n";
                    }
                    else
                        body << "<li>Remove all the buff and debuff</li>\n";
                }
                body << "</ul></div></div>\n";
            }
        }

        // ── 10. Crafting (this item IS a recipe) ──
        {
            auto recipeIt=s_itemIsRecipe.find(id);
            if(recipeIt!=s_itemIsRecipe.cend())
            {
                if(CatchChallenger::CommonDatapack::commonDatapack.has_craftingRecipe(recipeIt->second))
                {
                    const CatchChallenger::CraftingRecipe &recipe=CatchChallenger::CommonDatapack::commonDatapack.get_craftingRecipe(recipeIt->second);

                    // "Do the item"
                    body << "<div class=\"subblock\"><div class=\"valuetitle\">Do the item</div><div class=\"value\">\n";
                    writeItemLinkTable(body,rel,recipe.doItemId);
                    body << "</div></div>\n";

                    // "Material"
                    body << "<div class=\"subblock\"><div class=\"valuetitle\">Material</div><div class=\"value\">\n";
                    for(const auto &mat : recipe.materials)
                        writeItemLinkTable(body,rel,mat.item,mat.quantity);
                    body << "</div></div>\n";

                    // Crafting requirements
                    if(!recipe.requirements.reputation.empty())
                    {
                        body << "<div class=\"subblock\"><div class=\"valuetitle\">Requirements</div><div class=\"value\">\n";
                        writeReputationRequirements(body,recipe.requirements.reputation);
                        body << "</div></div>\n";
                    }

                    // Crafting rewards
                    if(!recipe.rewards.reputation.empty())
                    {
                        body << "<div class=\"subblock\"><div class=\"valuetitle\">Rewards</div><div class=\"value\">\n";
                        writeReputationRewards(body,recipe.rewards.reputation);
                        body << "</div></div>\n";
                    }
                }
            }
        }

        // ── 11. Product by crafting ──
        {
            auto productByIt=s_doItemToRecipeItems.find(id);
            if(productByIt!=s_doItemToRecipeItems.cend() && !productByIt->second.empty())
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Product by crafting</div><div class=\"value\">\n";
                for(uint16_t recipeItemId : productByIt->second)
                    writeItemLinkTable(body,rel,recipeItemId);
                body << "</div></div>\n";
            }
        }

        // ── 12. Used into crafting ──
        {
            auto usedInIt=s_materialToRecipeItems.find(id);
            if(usedInIt!=s_materialToRecipeItems.cend() && !usedInIt->second.empty())
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">Used into crafting</div><div class=\"value\">\n";
                for(uint16_t recipeItemId : usedInIt->second)
                    writeItemLinkTable(body,rel,recipeItemId);
                body << "</div></div>\n";
            }
        }

        // ── 13. Evolution ──
        {
            auto evoIt=s_itemEvolution.find(id);
            if(evoIt!=s_itemEvolution.cend() && !evoIt->second.empty())
            {
                unsigned count_evol=0;
                for(const auto &mp : evoIt->second)
                    if(QtDatapackClientLoader::datapackLoader->has_monsterExtra(mp.first) && QtDatapackClientLoader::datapackLoader->has_monsterExtra(mp.second))
                        count_evol++;

                for(const auto &mp : evoIt->second)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(mp.first) || !QtDatapackClientLoader::datapackLoader->has_monsterExtra(mp.second))
                        continue;

                    std::string fromName=QtDatapackClientLoader::datapackLoader->get_monsterExtra(mp.first).name;
                    std::string toName=QtDatapackClientLoader::datapackLoader->get_monsterExtra(mp.second).name;
                    std::string fromLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(mp.first));
                    std::string toLink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(mp.second));
                    std::string fromFront=monsterFrontImageUrl(rel,mp.first);
                    std::string toFront=monsterFrontImageUrl(rel,mp.second);

                    body << "<table class=\"item_list item_list_type_normal map_list\">\n"
                         << "\t\t\t\t\t<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t\t\t\t\t\t<th colspan=\"" << count_evol << "\">Evolve from</th>\n"
                         << "\t\t\t\t\t</tr>\n";
                    body << "<tr class=\"value\">\n<td>\n";
                    body << "<table class=\"monsterforevolution\">\n";
                    if(!fromFront.empty())
                        body << "<tr><td><a href=\"" << Helper::htmlEscape(fromLink) << "\"><img src=\""
                             << Helper::htmlEscape(fromFront) << "\" width=\"80\" height=\"80\" alt=\""
                             << Helper::htmlEscape(fromName) << "\" title=\"" << Helper::htmlEscape(fromName)
                             << "\" /></a></td></tr>\n";
                    body << "<tr><td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(fromLink) << "\">"
                         << Helper::htmlEscape(fromName) << "</a></td></tr>\n";
                    body << "</table>\n</td>\n</tr>\n";

                    body << "<tr><td class=\"evolution_type\">Evolve with<br /><a href=\"#\" title=\""
                         << Helper::htmlEscape(name) << "\">\n";
                    if(!imgUrl.empty())
                        body << "<img src=\"" << Helper::htmlEscape(imgUrl) << "\" alt=\""
                             << Helper::htmlEscape(name) << "\" title=\"" << Helper::htmlEscape(name)
                             << "\" style=\"float:left;\" />\n";
                    body << Helper::htmlEscape(name) << "</a></td></tr>\n";

                    body << "<tr class=\"value\">\n<td>\n";
                    body << "<table class=\"monsterforevolution\">\n";
                    if(!toFront.empty())
                        body << "<tr><td><a href=\"" << Helper::htmlEscape(toLink) << "\"><img src=\""
                             << Helper::htmlEscape(toFront) << "\" width=\"80\" height=\"80\" alt=\""
                             << Helper::htmlEscape(toName) << "\" title=\"" << Helper::htmlEscape(toName)
                             << "\" /></a></td></tr>\n";
                    body << "<tr><td class=\"evolution_name\"><a href=\"" << Helper::htmlEscape(toLink) << "\">"
                         << Helper::htmlEscape(toName) << "</a></td></tr>\n";
                    body << "</table>\n</td>\n</tr>\n";

                    body << "<tr>\n\t\t\t\t\t\t<th colspan=\"" << count_evol
                         << "\" class=\"item_list_endline item_list_title item_list_title_type_normal\">Evolve to</th>\n"
                         << "\t\t\t\t\t</tr>\n\t\t\t\t\t</table>\n";
                }
                body << "<br style=\"clear:both\" />\n";
            }
        }

        // ── 14. Close item_details div ──
        body << "</div>\n";

        // ── 15. Shop ──
        {
            auto shopsIt=s_itemInShops.find(id);
            if(shopsIt!=s_itemInShops.cend() && !shopsIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th colspan=\"4\">Shop</th>\n"
                     << "</tr>\n";
                std::set<std::pair<size_t,size_t>> seen;
                for(const auto &e : shopsIt->second)
                {
                    if(!seen.insert({e.setIdx,e.mapId}).second) continue;
                    const std::string &mp=mapPathOf(e.setIdx,e.mapId);
                    if(mp.empty()) continue;
                    std::string mapLink=Helper::relUrl(GeneratorMaps::relativePathForMapRef(e.setIdx,e.mapId));
                    std::string mname=mapDisplayName(e.setIdx,e.mapId);
                    body << "<tr class=\"value\">\n";
                    body << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(mapLink) << "\" title=\""
                         << Helper::htmlEscape(mname) << "\">" << Helper::htmlEscape(mname) << "</a></td>\n";
                    body << "<td colspan=\"2\">&nbsp;</td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        // ── 16. Monster drops ──
        {
            auto monsterDropIt=s_itemToMonsterDrop.find(id);
            if(monsterDropIt!=s_itemToMonsterDrop.cend() && !monsterDropIt->second.empty())
            {
                bool only_one=true;
                for(const auto &e : monsterDropIt->second)
                    if(e.qmin!=1 || e.qmax!=1)
                    { only_one=false; break; }

                int monster_count=0;

                auto writeMonsterDropHeader=[&](){
                    body << "<table class=\"itemmonster_list item_list item_list_type_normal\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th colspan=\"2\">Monster</th>\n";
                    if(!only_one)
                        body << "<th>Quantity</th>\n";
                    body << "<th>Luck</th>\n"
                         << "</tr>\n";
                };

                writeMonsterDropHeader();

                for(const auto &e : monsterDropIt->second)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(e.monsterId))
                        continue;

                    if(monster_count%10==0 && monster_count!=0)
                    {
                        body << "<tr>\n";
                        if(!only_one)
                            body << "<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n";
                        else
                            body << "<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n";
                        body << "</tr>\n</table>\n";
                        writeMonsterDropHeader();
                    }
                    monster_count++;

                    std::string qtext;
                    if(e.qmin!=e.qmax)
                        qtext=Helper::toStringUint(e.qmin)+" to "+Helper::toStringUint(e.qmax);
                    else
                        qtext=Helper::toStringUint(e.qmin);

                    body << "<tr class=\"value\">\n";
                    writeMonsterCells(body,rel,e.monsterId);
                    if(!only_one)
                        body << "<td>" << qtext << "</td>\n";
                    body << "<td>" << (unsigned)e.luck << "%</td>\n";
                    body << "</tr>\n";
                }

                body << "<tr>\n";
                if(!only_one)
                    body << "<td colspan=\"4\" class=\"item_list_endline item_list_title_type_normal\"></td>\n";
                else
                    body << "<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n";
                body << "</tr>\n</table>\n";
            }
        }

        // ── 17. <br/> ──
        body << "<br style=\"clear:both;\" />\n";

        // ── 18. Quest rewards ──
        {
            auto questRewardIt=s_itemToQuestReward.find(id);
            if(questRewardIt!=s_itemToQuestReward.cend() && !questRewardIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th>Quests</th>\n"
                     << "\t<th>Quantity rewarded</th>\n"
                     << "</tr>\n";
                for(const auto &qr : questRewardIt->second)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_questExtra(qr.questId)) continue;
                    body << "<tr class=\"value\">\n";
                    body << "<td><a href=\""
                         << Helper::htmlEscape(Helper::relUrl(GeneratorQuests::relativePathForQuest(qr.questId)))
                         << "\" title=\"" << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_questExtra(qr.questId).name) << "\">\n";
                    body << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_questExtra(qr.questId).name) << "\n";
                    body << "</a></td>\n";
                    body << "<td>" << qr.quantity << "</td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        // ── 19. Quest step items ──
        {
            auto questStepIt=s_itemToQuestStep.find(id);
            if(questStepIt!=s_itemToQuestStep.cend() && !questStepIt->second.empty())
            {
                bool full_details=false;
                for(const auto &qs : questStepIt->second)
                    if(qs.hasMonster && QtDatapackClientLoader::datapackLoader->has_monsterExtra(qs.monsterId))
                    { full_details=true; break; }

                if(full_details)
                    body << "<table class=\"item_list item_list_type_normal\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th>Quests</th>\n"
                         << "\t<th>Quantity needed</th>\n"
                         << "\t<th colspan=\"2\">Monster</th>\n"
                         << "\t<th>Luck</th>\n"
                         << "</tr>\n";
                else
                    body << "<table class=\"item_list item_list_type_normal\">\n"
                         << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                         << "\t<th>Quests</th>\n"
                         << "\t<th>Quantity needed</th>\n"
                         << "</tr>\n";

                for(const auto &qs : questStepIt->second)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_questExtra(qs.questId)) continue;
                    body << "<tr class=\"value\">\n";
                    body << "<td><a href=\""
                         << Helper::htmlEscape(Helper::relUrl(GeneratorQuests::relativePathForQuest(qs.questId)))
                         << "\" title=\"" << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_questExtra(qs.questId).name) << "\">\n";
                    body << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_questExtra(qs.questId).name) << "\n";
                    body << "</a></td>\n";
                    body << "<td>" << qs.quantity << "</td>\n";
                    if(qs.hasMonster && QtDatapackClientLoader::datapackLoader->has_monsterExtra(qs.monsterId))
                    {
                        writeMonsterCells(body,rel,qs.monsterId);
                        body << "<td>" << (unsigned)qs.rate << "%</td>\n";
                    }
                    else if(full_details)
                        body << "<td></td><td></td><td></td>\n";
                    body << "</tr>\n";
                }

                if(full_details)
                    body << "<tr>\n\t<td colspan=\"5\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                         << "</tr>\n</table>\n";
                else
                    body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                         << "</tr>\n</table>\n";
            }
        }

        // ── 20. Industry resource ──
        {
            auto indResIt=s_itemConsumedByIndustry.find(id);
            if(indResIt!=s_itemConsumedByIndustry.cend() && !indResIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th>Resource of the industry</th>\n"
                     << "\t<th>Quantity</th>\n"
                     << "</tr>\n";
                for(const auto &ir : indResIt->second)
                {
                    std::string mname=mapDisplayName(ir.setIdx,ir.mapIdx);
                    body << "<tr class=\"value\">\n";
                    body << "<td>Industry on " << Helper::htmlEscape(mname) << "</td>\n";
                    body << "<td>" << ir.quantity << "</td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        // ── 21. Industry product ──
        {
            auto indProdIt=s_itemProducedByIndustryMap.find(id);
            if(indProdIt!=s_itemProducedByIndustryMap.cend() && !indProdIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th>Product of the industry</th>\n"
                     << "\t<th>Quantity</th>\n"
                     << "</tr>\n";
                for(const auto &ip : indProdIt->second)
                {
                    std::string mname=mapDisplayName(ip.setIdx,ip.mapIdx);
                    body << "<tr class=\"value\">\n";
                    body << "<td>Industry on " << Helper::htmlEscape(mname) << "</td>\n";
                    body << "<td>" << ip.quantity << "</td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        // ── 22. Monster able to learn ──
        if(skillOfMonsterIt!=s_itemToSkillOfMonster.cend() && !skillOfMonsterIt->second.empty())
        {
            int colCount2=sameuniqueskill?3:5;
            int itemskillmonster_count=0;

            auto writeLearnHeader=[&](){
                body << "<table class=\"itemskillmonster item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th colspan=\"3\">Monster able to learn</th>\n";
                if(!sameuniqueskill)
                    body << "<th>Skill</th>\n"
                         << "\t<th>Type</th>\n";
                body << "</tr>\n";
            };

            writeLearnHeader();

            for(const auto &entry : skillOfMonsterIt->second)
            {
                if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(entry.monsterId))
                    continue;

                if(itemskillmonster_count%10==0 && itemskillmonster_count!=0)
                {
                    body << "<tr>\n\t<td colspan=\"" << colCount2
                         << "\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                         << "</tr>\n</table>\n";
                    writeLearnHeader();
                }
                itemskillmonster_count++;

                body << "<tr class=\"value\">\n";
                writeMonsterCells(body,rel,entry.monsterId);
                writeMonsterTypeLabels(body,rel,entry.monsterId);

                if(!sameuniqueskill)
                {
                    if(QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(entry.skillId))
                    {
                        body << "<td><a href=\""
                             << Helper::htmlEscape(Helper::relUrl(GeneratorSkills::relativePathForSkill(entry.skillId)))
                             << "\">" << Helper::htmlEscape(QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(entry.skillId).name);
                        if(entry.skillLevel>1)
                            body << " at level " << (unsigned)entry.skillLevel;
                        body << "</a></td>\n";
                        if(CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(entry.skillId))
                        {
                            uint8_t typeId=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(entry.skillId).type;
                            if(QtDatapackClientLoader::datapackLoader->has_typeExtra(typeId))
                                body << "<td><span class=\"type_label type_label_" << (unsigned)typeId
                                     << "\"><a href=\""
                                     << Helper::htmlEscape(Helper::relUrl(GeneratorTypes::relativePathForType(typeId)))
                                     << "\">" << Helper::htmlEscape(ucfirst(QtDatapackClientLoader::datapackLoader->get_typeExtra(typeId).name))
                                     << "</a></span></td>\n";
                            else
                                body << "<td>&nbsp;</td>\n";
                        }
                        else
                            body << "<td>&nbsp;</td>\n";
                    }
                }
                body << "</tr>\n";
            }

            body << "<tr>\n\t<td colspan=\"" << colCount2
                 << "\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                 << "</tr>\n</table>\n";
            body << "<br style=\"clear:both;\" />\n";
        }

        // ── 23. On the map ──
        {
            auto pickIt=s_itemOnMap.find(id);
            if(pickIt!=s_itemOnMap.cend() && !pickIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th colspan=\"2\">On the map</th>\n"
                     << "</tr>\n";
                std::set<std::pair<size_t,size_t>> dedup;
                for(const auto &e : pickIt->second)
                    dedup.insert({e.setIdx,e.mapId});
                for(const auto &k : dedup)
                {
                    const std::string &mp=mapPathOf(k.first,k.second);
                    if(mp.empty()) continue;
                    std::string mapLink=Helper::relUrl(GeneratorMaps::relativePathForMapRef(k.first,k.second));
                    std::string mname=mapDisplayName(k.first,k.second);
                    body << "<tr class=\"value\">\n";
                    body << "<td colspan=\"2\"><a href=\"" << Helper::htmlEscape(mapLink) << "\" title=\""
                         << Helper::htmlEscape(mname) << "\">" << Helper::htmlEscape(mname) << "</a></td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        // ── 24. Fight ──
        {
            auto frIt=s_itemAsFightReward.find(id);
            if(frIt!=s_itemAsFightReward.cend() && !frIt->second.empty())
            {
                body << "<table class=\"item_list item_list_type_normal\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                     << "\t<th colspan=\"2\">Fight</th>\n"
                     << "\t<th>Monster</th>\n"
                     << "</tr>\n";
                for(const auto &fr : frIt->second)
                {
                    const std::string &mp=mapPathOf(fr.setIdx,fr.mapId);
                    if(mp.empty()) continue;

                    const auto &sets=MapStore::sets();
                    const CatchChallenger::CommonMap &m=sets[fr.setIdx].mapList[fr.mapId];
                    auto bfIt=m.botFights.find(fr.botId);

                    body << "<tr class=\"value\">\n";
                    body << "<td colspan=\"2\">Bot #" << (unsigned)fr.botId << "</td>\n";
                    body << "<td>\n";
                    if(bfIt!=m.botFights.cend())
                    {
                        for(const auto &bm : bfIt->second.monsters)
                        {
                            bool mexFound=QtDatapackClientLoader::datapackLoader->has_monsterExtra(bm.id);
                            std::string mname=(mexFound)?QtDatapackClientLoader::datapackLoader->get_monsterExtra(bm.id).name:("Monster #"+Helper::toStringUint(bm.id));
                            std::string mlink=Helper::relUrl(GeneratorMonsters::relativePathForMonster(bm.id));
                            body << "<a href=\"" << Helper::htmlEscape(mlink) << "\">"
                                 << Helper::htmlEscape(mname) << "</a> Lv." << (unsigned)bm.level << " ";
                        }
                    }
                    body << "</td>\n";
                    body << "</tr>\n";
                }
                body << "<tr>\n\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                     << "</tr>\n</table>\n";
            }
        }

        Helper::writeHtml(rel,name,body.str());
    }

    // ── Build items index (matching PHP format) ──
    Helper::setCurrentPage("items/index.html");
    const std::string indexPage="items/index.html";

    indexBody << "<div style=\"width:16px;height:16px;float:left;background-color:#e5eaff;\"></div>: Buy into shop<br style=\"clear:both\" />";
    indexBody << "<div style=\"width:16px;height:16px;float:left;background-color:#e0ffdd;\"></div>: Drop or crafting<br style=\"clear:both\" />";
    indexBody << "<div style=\"width:16px;height:16px;float:left;background-color:#fbfdd3;\"></div>: Quest or industry<br style=\"clear:both\" />";
    indexBody << "<div style=\"width:16px;height:16px;float:left;background-color:#ffefdb;\"></div>: Hidden on map<br style=\"clear:both\" />";
    indexBody << "<div style=\"width:16px;height:16px;float:left;background-color:#ffe5e5;\"></div>: Fight<br style=\"clear:both\" />";

    for(const std::string &groupName : groupOrder)
    {
        auto git=grouped.find(groupName);
        if(git==grouped.end() || git->second.empty())
            continue;

        const auto &gids=git->second;
        const int maxPerTable=15;

        auto writeTableHeader=[&](){
            indexBody << "<div class=\"divfixedwithlist\"><table class=\"item_list item_list_type_normal map_list\">\n"
                      << "\t<tr class=\"item_list_title item_list_title_type_normal\">\n"
                      << "\t\t<th colspan=\"3\">" << Helper::htmlEscape(groupName) << "</th>\n"
                      << "\t</tr>\n"
                      << "\t<tr class=\"item_list_title item_list_title_type_normal\">\n"
                      << "\t\t<th colspan=\"2\">Item</th>\n"
                      << "\t\t<th>Price</th>\n"
                      << "\t</tr>\n";
        };

        writeTableHeader();
        int count=0;

        for(uint16_t gid : gids)
        {
            count++;
            if(count>maxPerTable)
            {
                indexBody << "\t<tr>\n\t\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                          << "\t\t\t</tr>\n\t\t\t</table></div>\n";
                writeTableHeader();
                count=1;
            }

            std::string iname;
            std::string iimagePath;
            if(QtDatapackClientLoader::datapackLoader->has_itemExtra(gid))
            {
                iname=QtDatapackClientLoader::datapackLoader->get_itemExtra(gid).name;
                iimagePath=QtDatapackClientLoader::datapackLoader->get_itemExtra(gid).imagePath;
            }
            if(iname.empty())
                iname="Item #"+Helper::toStringUint(gid);

            std::string irel=relativePathForItem(gid);
            std::string ilink=Helper::relUrlFrom(indexPage,irel);
            std::string iimageUrl=itemImageUrlFrom(indexPage,iimagePath);
            std::string gradient=buildGradient(gid);

            uint32_t price=0;
            if(CatchChallenger::CommonDatapack::commonDatapack.has_item(gid))
                price=CatchChallenger::CommonDatapack::commonDatapack.get_item(gid).price;

            indexBody << "\t<tr class=\"value\"" << gradient << ">\n";

            indexBody << "\t\t<td>\n";
            if(!iimageUrl.empty())
            {
                indexBody << "<a href=\"" << Helper::htmlEscape(ilink) << "\">\n";
                indexBody << "<img src=\"" << Helper::htmlEscape(iimageUrl) << "\" width=\"24\" height=\"24\" alt=\""
                          << Helper::htmlEscape(iname) << "\" title=\"" << Helper::htmlEscape(iname) << "\" />\n";
                indexBody << "</a>\n";
            }
            indexBody << "</td>\n";

            indexBody << "\t\t<td>\n";
            indexBody << "<a href=\"" << Helper::htmlEscape(ilink) << "\">\n";
            indexBody << Helper::htmlEscape(iname);
            indexBody << "</a>\n";
            indexBody << "</td>\n";

            if(price>0)
                indexBody << "<td>" << price << "$</td>\n";
            else
                indexBody << "<td>&nbsp;</td>\n";

            indexBody << "\t</tr>\n";
        }

        indexBody << "\t<tr>\n\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_normal\"></td>\n"
                  << "\t</tr>\n\t</table></div>\n";
    }

    Helper::writeHtml("items/index.html","Items list",indexBody.str());
}

} // namespace GeneratorItems
