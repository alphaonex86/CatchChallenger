#include "GeneratorSkills.hpp"
#include "GeneratorBuffs.hpp"
#include "GeneratorMonsters.hpp"
#include "GeneratorTypes.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <sstream>
#include <map>
#include <vector>
#include <set>

namespace GeneratorSkills {

std::string relativePathForSkill(uint16_t id)
{
    std::string stem;
    if(QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(id) && !QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(id).name.empty())
        stem=Helper::textForUrl(QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(id).name);
    if(stem.empty())
        stem=Helper::toStringUint(id);
    return "skills/"+stem+".html";
}

static std::string typeName(uint8_t typeId)
{
    const std::vector<CatchChallenger::Type> &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();
    if(typeId<types.size())
        return types[typeId].name;
    return Helper::toStringUint(typeId);
}

static std::string typeNameLower(uint8_t typeId)
{
    return Helper::toLowerCase(typeName(typeId));
}

static std::string typeLabelHtml(uint8_t typeId, const std::string &prefix=std::string())
{
    std::ostringstream oss;
    oss << "<span class=\"type_label type_label_" << typeNameLower(typeId) << "\">"
        << prefix
        << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorTypes::relativePathForType(typeId)))
        << "\">" << Helper::htmlEscape(Helper::firstLetterUpper(typeName(typeId))) << "</a></span>";
    return oss.str();
}

static std::string formatLifeQuantity(int32_t quantity, CatchChallenger::QuantityType type)
{
    std::string v=Helper::toStringInt(quantity);
    if(type==CatchChallenger::QuantityType_Percent)
        v+="%";
    return v;
}

static void writeMonsterTableHeader(std::ostringstream &body, const std::string &typeClass, bool multiLevel)
{
    body << "<table class=\"item_list item_list_type_" << typeClass << " skill_list\">\n"
         << "<tr class=\"item_list_title item_list_title_type_" << typeClass << "\">\n"
         << "\t<th colspan=\"2\">Monster</th>\n"
         << "\t<th>Type</th>\n";
    if(multiLevel)
        body << "\t<th>Skill level</th>\n";
    body << "</tr>\n";
}

static void writeMonsterTableFooter(std::ostringstream &body, const std::string &typeClass, bool multiLevel)
{
    body << "<tr>\n\t\t\t<td colspan=\"" << (multiLevel ? "4" : "3")
         << "\" class=\"item_list_endline item_list_title_type_" << typeClass << "\"></td>\n\t\t</tr>\n\t\t</table>\n";
}

void generate()
{
    const std::vector<CatchChallenger::Type> &commonTypes=CatchChallenger::CommonDatapack::commonDatapack.get_types();

    // Build skill_to_monster: skill_id -> skill_level -> list of monster_ids
    std::map<uint16_t, std::map<uint8_t, std::vector<uint16_t>>> skill_to_monster;
    for(CATCHCHALLENGER_TYPE_MONSTER mpid=1;mpid<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();mpid++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(mpid))
            continue;
        const CatchChallenger::Monster &mpMonster=CatchChallenger::CommonDatapack::commonDatapack.get_monster(mpid);
        for(const CatchChallenger::Monster::AttackToLearn &atk : mpMonster.learn)
            skill_to_monster[atk.learnSkill][atk.learnSkillLevel].push_back(mpid);
    }

    // Check if all skills have only 1 level (for index column display)
    bool only_one_level=true;
    for(CATCHCHALLENGER_TYPE_SKILL skchk=1;skchk<=65535;skchk++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(skchk))
        { if(skchk==65535) break; continue; }
        if(CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(skchk).level.size()>1)
        {
            only_one_level=false;
            break;
        }
        if(skchk==65535) break;
    }

    // Group skills by type for the index
    std::map<uint8_t, std::vector<uint16_t>> skill_type_to_id;
    for(CATCHCHALLENGER_TYPE_SKILL skgrp=1;skgrp<=65535;skgrp++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(skgrp))
        { if(skgrp==65535) break; continue; }
        skill_type_to_id[CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(skgrp).type].push_back(skgrp);
        if(skgrp==65535) break;
    }

    // --- Individual skill pages ---
    for(CATCHCHALLENGER_TYPE_SKILL id=1;id<=65535;id++) {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(id))
        { if(id==65535) break; continue; }
        const CatchChallenger::Skill &s=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(id);

        std::string name;
        if(QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(id))
            name=QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(id).name;
        if(name.empty())
            name="Skill #"+Helper::toStringUint(id);

        std::string typeClass=typeNameLower(s.type);
        std::string rel=relativePathForSkill(id);
        Helper::setCurrentPage(rel);

        // Build effectiveness lists
        std::map<int8_t, std::vector<uint8_t>> effectiveness_list;
        if(s.type<commonTypes.size())
        {
            for(const std::pair<const uint8_t, int8_t> &mp : commonTypes[s.type].multiplicator)
                effectiveness_list[mp.second].push_back(mp.first);
        }

        std::ostringstream body;
        body << "<div class=\"map monster_type_" << typeClass << "\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name) << "</h1><h2>#" << id << "</h2></div>\n";

        // Type label
        body << "<div class=\"type_label_list\">" << typeLabelHtml(s.type) << "</div>\n";

        // Effective against (multiplier 2 and 4 → C++ stored as 2, 4)
        bool hasEffective=(effectiveness_list.count(2)>0 || effectiveness_list.count(4)>0);
        if(hasEffective)
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Effective against</div><div class=\"value\">\n";
            bool first=true;
            if(effectiveness_list.count(2))
                for(uint8_t tid : effectiveness_list[2])
                {
                    if(!first) body << " ";
                    body << typeLabelHtml(tid,"2x: ") << "\n";
                    first=false;
                }
            if(effectiveness_list.count(4))
                for(uint8_t tid : effectiveness_list[4])
                {
                    if(!first) body << " ";
                    body << typeLabelHtml(tid,"4x: ") << "\n";
                    first=false;
                }
            body << "</div></div>\n";
        }

        // Not effective against (multiplier -2 = 0.5x, -4 = 0.25x)
        bool hasNotEffective=(effectiveness_list.count(-2)>0 || effectiveness_list.count(-4)>0);
        if(hasNotEffective)
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Not effective against</div><div class=\"value\">\n";
            bool first=true;
            if(effectiveness_list.count(-4))
                for(uint8_t tid : effectiveness_list[-4])
                {
                    if(!first) body << " ";
                    body << typeLabelHtml(tid,"0.25x: ") << "\n";
                    first=false;
                }
            if(effectiveness_list.count(-2))
                for(uint8_t tid : effectiveness_list[-2])
                {
                    if(!first) body << " ";
                    body << typeLabelHtml(tid,"0.5x: ") << "\n";
                    first=false;
                }
            body << "</div></div>\n";
        }

        // Useless against (multiplier 0)
        if(effectiveness_list.count(0)>0)
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Useless against</div><div class=\"value\">\n";
            bool first=true;
            for(uint8_t tid : effectiveness_list[0])
            {
                if(!first) body << " ";
                body << typeLabelHtml(tid) << "\n";
                first=false;
            }
            body << "</div></div>\n";
        }

        // Levels
        unsigned level=1;
        for(const CatchChallenger::Skill::SkillList &lvl : s.level)
        {
            body << "<div class=\"subblock\">\n";
            if(s.level.size()>1)
                body << "<div class=\"valuetitle\">Level " << level << "</div>\n";
            body << "<div class=\"value\">\n";

            body << "Endurance: " << (unsigned)lvl.endurance << "<br />\n";

            // Life quantity
            for(const CatchChallenger::Skill::Life &li : lvl.life)
            {
                std::string lq=formatLifeQuantity(li.effect.quantity, li.effect.type);
                body << "Life quantity: " << lq << "<br />\n";
            }

            // Buff table
            if(!lvl.buff.empty())
            {
                body << "Add buff:";
                body << "<center><table class=\"item_list item_list_type_" << typeClass << "\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_" << typeClass << "\">\n"
                     << "\t<th colspan=\"2\">Buff</th>\n"
                     << "\t<th>Success</th>\n"
                     << "</tr>\n";

                for(const CatchChallenger::Skill::Buff &bf : lvl.buff)
                {
                    uint8_t buffId=bf.effect.buff;
                    body << "<tr class=\"value\"><td>\n";

                    std::string buffIconRel="monsters/buff/"+Helper::toStringUint(buffId)+".png";
                    if(Helper::fileExists(Helper::datapackPath()+buffIconRel))
                    {
                        std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(buffIconRel));
                        body << "<img src=\"" << imgUrl << "\" alt=\"\" width=\"16\" height=\"16\" />\n";
                    }
                    else
                        body << "&nbsp;\n";
                    body << "</td>\n";

                    // Buff name linked
                    std::string buffName;
                    if(QtDatapackClientLoader::datapackLoader->has_monsterBuffExtra(buffId))
                        buffName=QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra(buffId).name;
                    if(buffName.empty())
                        buffName="Buff #"+Helper::toStringUint(buffId);

                    body << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorBuffs::relativePathForBuff(buffId)))
                         << "\">" << Helper::htmlEscape(buffName) << "</a></td>\n";
                    body << "<td>" << (unsigned)bf.success << "%</td>\n";
                    body << "</tr>\n";
                }

                body << "<tr>\n\t\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_" << typeClass << "\"></td>\n"
                     << "\t\t\t\t</tr>\n\t\t\t\t</table></center>\n";
            }

            body << "</div></div>\n";
            ++level;
        }
        body << "</div>\n";

        // Monster list table (skill_to_monster)
        std::map<uint16_t, std::map<uint8_t, std::vector<uint16_t>>>::const_iterator stmIt=skill_to_monster.find(id);
        if(stmIt!=skill_to_monster.end() && !stmIt->second.empty())
        {
            bool multiLevel=(stmIt->second.size()>1);
            std::set<uint16_t> skill_monster_duplicate;
            unsigned monster_count=0;

            writeMonsterTableHeader(body,typeClass,multiLevel);

            uint8_t skill_level_displayed=0;
            for(const std::pair<const uint8_t, std::vector<uint16_t>> &lvlPair : stmIt->second)
            {
                uint8_t skillLevel=lvlPair.first;
                const std::vector<uint16_t> &monsterList=lvlPair.second;

                if(skill_level_displayed!=skillLevel && multiLevel)
                {
                    body << "<tr class=\"item_list_title_type_" << typeClass << "\"><th colspan=\"4\">Level " << (unsigned)skillLevel << "</th></tr>\n";
                    skill_level_displayed=skillLevel;
                }

                for(uint16_t monsterId : monsterList)
                {
                    if(skill_monster_duplicate.count(monsterId))
                        continue;

                    if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId))
                        continue;

                    if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
                        continue;

                    monster_count++;
                    if(monster_count>20)
                    {
                        writeMonsterTableFooter(body,typeClass,multiLevel);
                        writeMonsterTableHeader(body,typeClass,multiLevel);
                        monster_count=1;
                    }

                    skill_monster_duplicate.insert(monsterId);
                    const std::string &mname=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId).name;
                    std::string link=Helper::relUrl(GeneratorMonsters::relativePathForMonster(monsterId));

                    body << "<tr class=\"value\">\n\t\t\t\t\t\t<td>\n";

                    std::string smallPng="monsters/"+Helper::toStringUint(monsterId)+"/small.png";
                    std::string smallGif="monsters/"+Helper::toStringUint(monsterId)+"/small.gif";
                    if(Helper::fileExists(Helper::datapackPath()+smallPng))
                    {
                        std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(smallPng));
                        body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link) << "\"><img src=\"" << imgUrl
                             << "\" width=\"32\" height=\"32\" alt=\"" << Helper::htmlEscape(mname)
                             << "\" title=\"" << Helper::htmlEscape(mname) << "\" /></a></div>\n";
                    }
                    else if(Helper::fileExists(Helper::datapackPath()+smallGif))
                    {
                        std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(smallGif));
                        body << "<div class=\"monstericon\"><a href=\"" << Helper::htmlEscape(link) << "\"><img src=\"" << imgUrl
                             << "\" width=\"32\" height=\"32\" alt=\"" << Helper::htmlEscape(mname)
                             << "\" title=\"" << Helper::htmlEscape(mname) << "\" /></a></div>\n";
                    }

                    body << "</td>\n\t\t\t\t\t\t<td><a href=\"" << Helper::htmlEscape(link) << "\">" << Helper::htmlEscape(mname) << "</a></td>\n";

                    // Type labels
                    body << "<td><div class=\"type_label_list\">";
                    bool firstType=true;
                    for(uint8_t tId : CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId).type)
                    {
                        if(tId<commonTypes.size())
                        {
                            if(!firstType) body << " ";
                            body << typeLabelHtml(tId) << "\n";
                            firstType=false;
                        }
                    }
                    body << "</div></td>\n";

                    if(multiLevel)
                        body << "<td>" << (unsigned)skillLevel << "</td>\n";
                    body << "</tr>\n";
                }
            }

            writeMonsterTableFooter(body,typeClass,multiLevel);
        }

        Helper::writeHtml(rel,name,body.str());
        if(id==65535) break;
    }

    // --- Index page (skills.html) grouped by type ---
    Helper::setCurrentPage("skills.html");
    std::ostringstream indexBody;

    for(const std::pair<const uint8_t, std::vector<uint16_t>> &typeGroup : skill_type_to_id)
    {
        uint8_t type=typeGroup.first;
        const std::vector<uint16_t> &id_list=typeGroup.second;
        std::string tc=typeNameLower(type);
        unsigned colCount=only_one_level ? 3 : 4;
        unsigned skill_count=0;

        std::function<void(std::ostringstream &)> writeIndexTableHeader=[&](std::ostringstream &out) {
            out << "<div class=\"divfixedwithlist\"><table class=\"item_list item_list_type_" << tc << " skill_list\">\n"
                << "<tr class=\"item_list_title item_list_title_type_" << tc << "\">\n"
                << "\t<th>Skill</th>\n"
                << "\t<th>Type</th>\n"
                << "\t<th>Endurance</th>\n";
            if(!only_one_level)
                out << "\t<th>Number of level</th>\n";
            out << "</tr>\n";
        };

        writeIndexTableHeader(indexBody);

        for(uint16_t skillId : id_list)
        {
            if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(skillId)) continue;
            const CatchChallenger::Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(skillId);

            if(skill.level.empty())
                continue;

            skill_count++;
            if(skill_count>20)
            {
                indexBody << "<tr>\n\t\t\t\t\t<td colspan=\"" << colCount
                          << "\" class=\"item_list_endline item_list_title_type_" << tc << "\"></td>\n"
                          << "\t\t\t\t</tr>\n\t\t\t\t</table></div>\n";
                writeIndexTableHeader(indexBody);
                skill_count=1;
            }

            std::string sname;
            if(QtDatapackClientLoader::datapackLoader->has_monsterSkillExtra(skillId))
                sname=QtDatapackClientLoader::datapackLoader->get_monsterSkillExtra(skillId).name;
            if(sname.empty())
                sname="Skill #"+Helper::toStringUint(skillId);

            indexBody << "<tr class=\"value\">\n";
            indexBody << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(relativePathForSkill(skillId)))
                      << "\">" << Helper::htmlEscape(sname) << "</a></td>\n";

            // Type
            if(skill.type<commonTypes.size())
                indexBody << "<td>" << typeLabelHtml(skill.type) << "</td>\n";
            else
                indexBody << "<td>&nbsp;</td>\n";

            // Endurance (from first level)
            if(!skill.level.empty())
                indexBody << "<td>" << (unsigned)skill.level[0].endurance << "</td>\n";
            else
                indexBody << "<td>&nbsp;</td>\n";

            // Number of levels
            if(!only_one_level)
                indexBody << "<td>" << skill.level.size() << "</td>\n";

            indexBody << "</tr>\n";
        }

        indexBody << "<tr>\n\t\t<td colspan=\"" << colCount
                  << "\" class=\"item_list_endline item_list_title_type_" << tc << "\"></td>\n\t</tr>\n\t</table></div>\n";
    }

    Helper::writeHtml("skills.html","Skills list",indexBody.str());
}

} // namespace GeneratorSkills
