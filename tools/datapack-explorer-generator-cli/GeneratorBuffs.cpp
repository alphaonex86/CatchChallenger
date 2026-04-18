#include "GeneratorBuffs.hpp"
#include "GeneratorMonsters.hpp"
#include "GeneratorTypes.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <sstream>
#include <map>
#include <vector>

namespace GeneratorBuffs {

std::string relativePathForBuff(uint8_t id)
{
    std::string stem;
    if(QtDatapackClientLoader::datapackLoader->has_monsterBuffExtra(id) && !QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra(id).name.empty())
        stem=Helper::textForUrl(QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra(id).name);
    if(stem.empty())
        stem=Helper::toStringUint(id);
    return "buffs/"+stem+".html";
}

static std::string formatEffectValue(int32_t quantity, CatchChallenger::QuantityType type)
{
    std::string v=Helper::toStringInt(quantity);
    if(type==CatchChallenger::QuantityType_Percent)
        v+="%";
    return v;
}

void generate()
{
    const std::vector<CatchChallenger::Type> &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();

    // Build buff_to_monster: buff_id -> buff_level -> list of monster_ids
    std::map<uint8_t, std::map<uint8_t, std::vector<uint16_t>>> buff_to_monster;
    for(CATCHCHALLENGER_TYPE_MONSTER monsterId=1;monsterId<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();monsterId++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
            continue;
        const CatchChallenger::Monster &monster=CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId);
        for(const CatchChallenger::Monster::AttackToLearn &atk : monster.learn)
        {
            if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterSkill(atk.learnSkill))
                continue;
            const CatchChallenger::Skill &skill=CatchChallenger::CommonDatapack::commonDatapack.get_monsterSkill(atk.learnSkill);
            for(size_t lvlIdx=0; lvlIdx<skill.level.size(); ++lvlIdx)
            {
                for(const CatchChallenger::Skill::Buff &be : skill.level[lvlIdx].buff)
                {
                    uint8_t buffId=be.effect.buff;
                    uint8_t buffLevel=be.effect.level;
                    if(CatchChallenger::CommonDatapack::commonDatapack.has_monsterBuff(buffId))
                    {
                        std::vector<uint16_t> &vec=buff_to_monster[buffId][buffLevel];
                        bool found=false;
                        for(uint16_t mid : vec)
                            if(mid==monsterId) { found=true; break; }
                        if(!found)
                            vec.push_back(monsterId);
                    }
                }
            }
        }
    }

    // --- Individual buff pages ---
    for(CATCHCHALLENGER_TYPE_BUFF id=1;id<=255;id++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterBuff(id))
        {if(id==255)break;continue;}
        const CatchChallenger::Buff &b=CatchChallenger::CommonDatapack::commonDatapack.get_monsterBuff(id);

        std::string name;
        if(QtDatapackClientLoader::datapackLoader->has_monsterBuffExtra(id))
            name=QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra(id).name;
        if(name.empty())
            name="Buff #"+Helper::toStringUint(id);

        std::string rel=relativePathForBuff(id);
        Helper::setCurrentPage(rel);

        std::ostringstream body;
        body << "<div class=\"map monster_type_normal\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(name) << "</h1></div>\n";

        unsigned level=1;
        for(const CatchChallenger::Buff::GeneralEffect &lvl : b.level)
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">\n";

            // Buff icon
            std::string buffIconRel="monsters/buff/"+Helper::toStringUint(id)+".png";
            if(Helper::fileExists(Helper::datapackPath()+buffIconRel))
            {
                std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(buffIconRel));
                body << "<center><img src=\"" << imgUrl << "\" alt=\"\" width=\"16\" height=\"16\" /></center>\n";
            }

            if(b.level.size()>1)
                body << "Level " << level;
            body << "</div><div class=\"value\">\n";

            if(lvl.capture_bonus!=1.0f)
                body << "Capture bonus: " << lvl.capture_bonus << "<br />\n";

            switch(lvl.duration)
            {
                case CatchChallenger::Buff::Duration_ThisFight:
                    body << "This buff is only valid for this fight<br />\n";
                    break;
                case CatchChallenger::Buff::Duration_Always:
                    body << "This buff is always valid<br />\n";
                    break;
                case CatchChallenger::Buff::Duration_NumberOfTurn:
                    body << "This buff is valid during " << (unsigned)lvl.durationNumberOfTurn << " turns<br />\n";
                    break;
            }

            // In fight effects
            if(!lvl.fight.empty())
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">In fight</div><div class=\"value\">\n";
                for(const CatchChallenger::Buff::Effect &eff : lvl.fight)
                {
                    std::string val=formatEffectValue(eff.quantity, eff.type);
                    switch(eff.on)
                    {
                        case CatchChallenger::Buff::Effect::EffectOn_HP:
                            body << "The hp change of " << val << "<br />\n";
                            break;
                        case CatchChallenger::Buff::Effect::EffectOn_Defense:
                            body << "The defense change of " << val << "<br />\n";
                            break;
                        case CatchChallenger::Buff::Effect::EffectOn_Attack:
                            body << "The attack change of " << val << "<br />\n";
                            break;
                    }
                }
                body << "</div></div>\n";
            }

            // In walk effects
            if(!lvl.walk.empty())
            {
                body << "<div class=\"subblock\"><div class=\"valuetitle\">In walk</div><div class=\"value\">\n";
                for(const CatchChallenger::Buff::EffectInWalk &w : lvl.walk)
                {
                    std::string val=formatEffectValue(w.effect.quantity, w.effect.type);
                    std::string steps=Helper::toStringUint(w.steps);
                    switch(w.effect.on)
                    {
                        case CatchChallenger::Buff::Effect::EffectOn_HP:
                            body << "The hp change of <b>" << val << "</b> during <b>" << steps << " steps</b><br />\n";
                            break;
                        case CatchChallenger::Buff::Effect::EffectOn_Defense:
                            body << "The defense change of <b>" << val << "</b> during <b>" << steps << " steps</b><br />\n";
                            break;
                        case CatchChallenger::Buff::Effect::EffectOn_Attack:
                            body << "The attack change of <b>" << val << "</b> during <b>" << steps << " steps</b><br />\n";
                            break;
                    }
                }
                body << "</div></div>\n";
            }

            body << "</div></div>\n";
            ++level;
        }
        body << "</div>\n";

        // Monster table (buff_to_monster)
        std::map<uint8_t, std::map<uint8_t, std::vector<uint16_t>>>::const_iterator btmIt=buff_to_monster.find(id);
        if(btmIt!=buff_to_monster.end() && !btmIt->second.empty())
        {
            bool multiLevel=(btmIt->second.size()>1);
            body << "<table class=\"item_list item_list_type_normal\">\n"
                 << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
                 << "\t<th colspan=\"2\">Monster</th>\n"
                 << "\t<th>Type</th>\n";
            if(multiLevel)
                body << "\t<th>Skill level</th>\n";
            body << "</tr>\n";

            uint8_t buff_level_displayed=0;
            for(const std::pair<const uint8_t, std::vector<uint16_t>> &lvlPair : btmIt->second)
            {
                uint8_t buffLevel=lvlPair.first;
                const std::vector<uint16_t> &monsterList=lvlPair.second;

                if(buff_level_displayed!=buffLevel && multiLevel)
                {
                    body << "<tr class=\"item_list_title_type_normal\"><th colspan=\"4\">\n"
                         << "Level " << (unsigned)buffLevel
                         << "</th></tr>\n";
                    buff_level_displayed=buffLevel;
                }

                for(uint16_t monsterId : monsterList)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId))
                        continue;

                    if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
                        continue;

                    const std::string &mname=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId).name;
                    std::string link=Helper::relUrl(GeneratorMonsters::relativePathForMonster(monsterId));

                    body << "<tr class=\"value\">\n\t<td>\n";

                    // Monster icon
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

                    body << "</td>\n\t<td><a href=\"" << Helper::htmlEscape(link) << "\">" << Helper::htmlEscape(mname) << "</a></td>\n";

                    // Type labels
                    body << "<td><div class=\"type_label_list\">";
                    bool firstType=true;
                    for(uint8_t tId : CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId).type)
                    {
                        if(tId<types.size())
                        {
                            if(!firstType) body << " ";
                            body << "<span class=\"type_label type_label_" << Helper::toLowerCase(types[tId].name) << "\">"
                                 << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(GeneratorTypes::relativePathForType(tId))) << "\">"
                                 << Helper::htmlEscape(Helper::firstLetterUpper(types[tId].name)) << "</a></span>\n";
                            firstType=false;
                        }
                    }
                    body << "</div></td>\n";

                    if(multiLevel)
                        body << "<td>" << (unsigned)buffLevel << "</td>\n";

                    body << "</tr>\n";
                }
            }

            body << "<tr>\n\t<td colspan=\"" << (multiLevel ? "4" : "3")
                 << "\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";
        }

        Helper::writeHtml(rel,name,body.str());
        if(id==255)break;
    }

    // --- Index page (buffs.html at root) ---
    Helper::setCurrentPage("buffs.html");
    std::ostringstream indexBody;
    indexBody << "<table class=\"item_list item_list_type_normal\">\n"
              << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
              << "\t<th colspan=\"2\">Buff</th>\n"
              << "</tr>\n";

    for(CATCHCHALLENGER_TYPE_BUFF id=1;id<=255;id++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monsterBuff(id))
        {if(id==255)break;continue;}

        std::string name;
        if(QtDatapackClientLoader::datapackLoader->has_monsterBuffExtra(id))
            name=QtDatapackClientLoader::datapackLoader->get_monsterBuffExtra(id).name;
        if(name.empty())
            name="Buff #"+Helper::toStringUint(id);

        indexBody << "<tr class=\"value\">\n";
        indexBody << "<td>\n";

        std::string buffIconRel="monsters/buff/"+Helper::toStringUint(id)+".png";
        if(Helper::fileExists(Helper::datapackPath()+buffIconRel))
        {
            std::string imgUrl=Helper::relUrl(Helper::publishDatapackFile(buffIconRel));
            indexBody << "<img src=\"" << imgUrl << "\" alt=\"\" width=\"16\" height=\"16\" />\n";
        }
        else
            indexBody << "&nbsp;\n";

        indexBody << "</td>\n";
        indexBody << "<td><a href=\"" << Helper::htmlEscape(Helper::relUrl(relativePathForBuff(id)))
                  << "\">" << Helper::htmlEscape(name) << "</a></td>\n";
        indexBody << "</tr>\n";
        if(id==255)break;
    }

    indexBody << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

    Helper::writeHtml("buffs.html","Buffs list",indexBody.str());
}

} // namespace GeneratorBuffs
