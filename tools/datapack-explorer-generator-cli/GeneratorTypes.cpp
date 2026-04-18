#include "GeneratorTypes.hpp"
#include "GeneratorMonsters.hpp"
#include "Helper.hpp"

#include "../../client/libqtcatchchallenger/QtDatapackClientLoader.hpp"
#include "../../general/base/CommonDatapack.hpp"

#include <sstream>
#include <vector>
#include <map>

namespace GeneratorTypes {

static std::string typeNameStr(uint8_t id)
{
    const std::vector<CatchChallenger::Type> &types=CatchChallenger::CommonDatapack::commonDatapack.get_types();
    if(id<types.size())
        return types[id].name;
    return Helper::toStringUint(id);
}

static std::string typeNameLower(uint8_t id)
{
    return Helper::toLowerCase(typeNameStr(id));
}

std::string relativePathForType(uint8_t id)
{
    return "monsters/type-"+typeNameLower(id)+".html";
}

static std::string typeLabelHtml(uint8_t id, const std::string &prefix=std::string())
{
    std::ostringstream oss;
    oss << "<span class=\"type_label type_label_" << typeNameLower(id) << "\">"
        << prefix
        << "<a href=\"" << Helper::htmlEscape(Helper::relUrl(relativePathForType(id)))
        << "\">" << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr(id))) << "</a></span>";
    return oss.str();
}

// Convert C++ encoded int8_t multiplier to PHP float string
static std::string multiplierToString(int8_t m)
{
    if(m==0) return "0";
    if(m>0) return Helper::toStringInt(m);
    // negative: 1/abs(m)
    int abs_m=-m;
    if(abs_m==2) return "0.5";
    if(abs_m==4) return "0.25";
    return "1/"+Helper::toStringInt(abs_m);
}

// Get PHP-style float multiplier for display
static float multiplierToFloat(int8_t m)
{
    if(m==0) return 0.0f;
    if(m>0) return (float)m;
    return 1.0f/(float)(-m);
}

void generate()
{
    const std::vector<CatchChallenger::Type> &commonTypes=CatchChallenger::CommonDatapack::commonDatapack.get_types();

    // Build type_to_monster: primary_type -> secondary_type -> monster_ids
    std::map<uint8_t, std::map<uint8_t, std::vector<uint16_t>>> type_to_monster;
    for(CATCHCHALLENGER_TYPE_MONSTER mId=1;mId<=CatchChallenger::CommonDatapack::commonDatapack.get_monstersMaxId();mId++)
    {
        if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(mId))
            continue;
        const CatchChallenger::Monster &monEntry=CatchChallenger::CommonDatapack::commonDatapack.get_monster(mId);
        const std::vector<uint8_t> &mtypes=monEntry.type;
        if(mtypes.empty()) continue;
        uint8_t type1=mtypes[0];
        uint8_t type2=(mtypes.size()>1) ? mtypes[1] : type1;
        if(type2>=commonTypes.size())
            type2=type1;
        type_to_monster[type1][type2].push_back(mId);
        if(type1!=type2)
            type_to_monster[type2][type1].push_back(mId);
    }

    // --- Individual type pages ---
    for(uint16_t id=0; id<commonTypes.size(); ++id)
    {
        const CatchChallenger::Type &t=commonTypes[id];
        std::string tname=typeNameStr((uint8_t)id);
        std::string tc=typeNameLower((uint8_t)id);
        std::string rel=relativePathForType((uint8_t)id);
        Helper::setCurrentPage(rel);

        std::ostringstream body;
        body << "<div class=\"map monster_type_" << tc << "\">\n";
        body << "<div class=\"subblock\"><h1>" << Helper::htmlEscape(Helper::firstLetterUpper(tname)) << "</h1></div>\n";

        // Type label in subblock
        body << "<div class=\"subblock\"><div class=\"valuetitle\">" << Helper::firstLetterUpper("type")
             << "</div><div class=\"value\">\n";
        body << "<div class=\"type_label_list\">" << typeLabelHtml((uint8_t)id) << "</div></div></div>\n";

        // DEFENSIVE: what types are effective against this type
        // Look at all other types' multiplicators targeting this type
        std::map<std::string, std::vector<uint8_t>> def_effectiveness;
        for(uint16_t a=0; a<commonTypes.size(); ++a)
        {
            const std::unordered_map<uint8_t, int8_t> &mp=commonTypes[a].multiplicator;
            std::unordered_map<uint8_t, int8_t>::const_iterator mit=mp.find((uint8_t)id);
            if(mit!=mp.cend())
            {
                float eff=multiplierToFloat(mit->second);
                if(eff!=1.0f)
                    def_effectiveness[multiplierToString(mit->second)].push_back((uint8_t)a);
            }
        }

        // Weak to (2x and 4x)
        if(def_effectiveness.count("2") || def_effectiveness.count("4"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Weak to</div><div class=\"value\">\n";
            bool first=true;
            if(def_effectiveness.count("2"))
                for(uint8_t tid : def_effectiveness["2"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"2x: ") << "\n"; first=false; }
            if(def_effectiveness.count("4"))
                for(uint8_t tid : def_effectiveness["4"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"4x: ") << "\n"; first=false; }
            body << "</div></div>\n";
        }

        // Resistant to (0.25x and 0.5x)
        if(def_effectiveness.count("0.25") || def_effectiveness.count("0.5"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Resistant to</div><div class=\"value\">\n";
            bool first=true;
            if(def_effectiveness.count("0.25"))
                for(uint8_t tid : def_effectiveness["0.25"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"0.25x: ") << "\n"; first=false; }
            if(def_effectiveness.count("0.5"))
                for(uint8_t tid : def_effectiveness["0.5"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"0.5x: ") << "\n"; first=false; }
            body << "</div></div>\n";
        }

        // Immune to (0)
        if(def_effectiveness.count("0"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Immune to</div><div class=\"value\">\n";
            bool first=true;
            for(uint8_t tid : def_effectiveness["0"])
            { if(!first) body << " "; body << typeLabelHtml(tid) << "\n"; first=false; }
            body << "</div></div>\n";
        }

        // OFFENSIVE: this type's own multiplicators
        std::map<std::string, std::vector<uint8_t>> off_effectiveness;
        for(const std::pair<const uint8_t, int8_t> &mp : t.multiplicator)
            off_effectiveness[multiplierToString(mp.second)].push_back(mp.first);

        // Effective against (2x, 4x)
        if(off_effectiveness.count("2") || off_effectiveness.count("4"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Effective against</div><div class=\"value\">\n";
            bool first=true;
            if(off_effectiveness.count("2"))
                for(uint8_t tid : off_effectiveness["2"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"2x: ") << "\n"; first=false; }
            if(off_effectiveness.count("4"))
                for(uint8_t tid : off_effectiveness["4"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"4x: ") << "\n"; first=false; }
            body << "</div></div>\n";
        }

        // Not effective against (0.25x, 0.5x)
        if(off_effectiveness.count("0.25") || off_effectiveness.count("0.5"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Not effective against</div><div class=\"value\">\n";
            bool first=true;
            if(off_effectiveness.count("0.25"))
                for(uint8_t tid : off_effectiveness["0.25"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"0.25x: ") << "\n"; first=false; }
            if(off_effectiveness.count("0.5"))
                for(uint8_t tid : off_effectiveness["0.5"])
                { if(!first) body << " "; body << typeLabelHtml(tid,"0.5x: ") << "\n"; first=false; }
            body << "</div></div>\n";
        }

        // Useless against (0)
        if(off_effectiveness.count("0"))
        {
            body << "<div class=\"subblock\"><div class=\"valuetitle\">Useless against</div><div class=\"value\">\n";
            bool first=true;
            for(uint8_t tid : off_effectiveness["0"])
            { if(!first) body << " "; body << typeLabelHtml(tid) << "\n"; first=false; }
            body << "</div></div>\n";
        }

        body << "</div>\n";

        // Monster list table grouped by secondary type
        std::map<uint8_t, std::map<uint8_t, std::vector<uint16_t>>>::const_iterator ttmIt=type_to_monster.find((uint8_t)id);
        if(ttmIt!=type_to_monster.end() && !ttmIt->second.empty())
        {
            std::function<void()> writeMonsterTableHeader=[&]() {
                body << "<table class=\"monster_list item_list item_list_type_" << tc << "\">\n"
                     << "<tr class=\"item_list_title item_list_title_type_" << tc << "\">\n"
                     << "\t<th colspan=\"2\">Monster</th>\n"
                     << "\t<th>Type</th>\n"
                     << "</tr>\n";
            };

            writeMonsterTableHeader();
            std::string second_type_displayed;
            unsigned typetomonster_count=0;

            for(const std::pair<const uint8_t, std::vector<uint16_t>> &stPair : ttmIt->second)
            {
                uint8_t secondType=stPair.first;
                const std::vector<uint16_t> &monsterList=stPair.second;
                std::string stName=typeNameLower(secondType);

                if(second_type_displayed!=stName)
                {
                    typetomonster_count++;

                    if(typetomonster_count%10==0)
                    {
                        body << "<tr>\n\t\t\t\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_" << tc << "\"></td>\n"
                             << "\t\t\t\t\t</tr>\n\t\t\t\t\t</table>\n";
                        writeMonsterTableHeader();
                    }

                    if(secondType==(uint8_t)id)
                        body << "<tr class=\"item_list_title_type_" << stName << "\"><th colspan=\"3\">"
                             << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr(secondType))) << "</th></tr>\n";
                    else
                        body << "<tr class=\"item_list_title_type_" << stName << "\"><th colspan=\"3\">"
                             << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr((uint8_t)id))) << " - "
                             << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr(secondType))) << "</th></tr>\n";
                    second_type_displayed=stName;
                }

                for(uint16_t monsterId : monsterList)
                {
                    if(!QtDatapackClientLoader::datapackLoader->has_monsterExtra(monsterId))
                        continue;
                    if(!CatchChallenger::CommonDatapack::commonDatapack.has_monster(monsterId))
                        continue;

                    const DatapackClientLoader::MonsterExtra &meit=QtDatapackClientLoader::datapackLoader->get_monsterExtra(monsterId);
                    const CatchChallenger::Monster &mit=CatchChallenger::CommonDatapack::commonDatapack.get_monster(monsterId);

                    typetomonster_count++;
                    const std::string &mname=meit.name;
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
                    for(uint8_t tId : mit.type)
                    {
                        if(tId<commonTypes.size())
                        {
                            if(!firstType) body << " ";
                            body << typeLabelHtml(tId) << "\n";
                            firstType=false;
                        }
                    }
                    body << "</div></td>\n";
                    body << "</tr>\n";

                    if(typetomonster_count%10==0)
                    {
                        body << "<tr>\n\t\t\t\t\t\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_" << tc << "\"></td>\n"
                             << "\t\t\t\t\t\t\t</tr>\n\t\t\t\t\t\t\t</table>\n";
                        writeMonsterTableHeader();
                        // Re-emit secondary type header if needed
                        if(second_type_displayed!=stName)
                        {
                            if(secondType==(uint8_t)id)
                                body << "<tr class=\"item_list_title_type_" << stName << "\"><th colspan=\"3\">"
                                     << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr(secondType))) << "</th></tr>\n";
                            else
                                body << "<tr class=\"item_list_title_type_" << stName << "\"><th colspan=\"3\">"
                                     << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr((uint8_t)id))) << " - "
                                     << Helper::htmlEscape(Helper::firstLetterUpper(typeNameStr(secondType))) << "</th></tr>\n";
                            second_type_displayed=stName;
                            typetomonster_count++;
                        }
                    }
                }
            }

            body << "<tr>\n\t\t\t<td colspan=\"3\" class=\"item_list_endline item_list_title_type_" << tc << "\"></td>\n"
                 << "\t\t</tr>\n\t\t</table>\n";
            body << "<br style=\"clear:both;\" />\n";
        }

        Helper::writeHtml(rel,Helper::firstLetterUpper(tname),body.str());
    }

    // --- Index page (types.html) ---
    Helper::setCurrentPage("types.html");
    std::ostringstream indexBody;

    // Summary table
    indexBody << "<table class=\"item_list item_list_type_normal\">\n"
              << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
              << "\t<th>Type</th>\n"
              << "\t<th>Monster</th>\n"
              << "</tr>\n";

    for(uint16_t id=0; id<commonTypes.size(); ++id)
    {
        indexBody << "<tr class=\"value\">\n";
        indexBody << "<td><div class=\"type_label_list\">" << typeLabelHtml((uint8_t)id) << "</div></td>\n";

        // Count monsters of this type
        unsigned count=0;
        std::map<uint8_t, std::map<uint8_t, std::vector<uint16_t>>>::const_iterator ttmIt=type_to_monster.find((uint8_t)id);
        if(ttmIt!=type_to_monster.end())
            for(const std::pair<const uint8_t, std::vector<uint16_t>> &stPair : ttmIt->second)
                count+=stPair.second.size();

        indexBody << "<td>" << count << "</td>\n";
        indexBody << "</tr>\n";
    }

    indexBody << "<tr>\n\t<td colspan=\"2\" class=\"item_list_endline item_list_title_type_normal\"></td>\n</tr>\n</table>\n";

    // Effectiveness matrix
    indexBody << "<table class=\"item_list item_list_type_normal\">\n"
              << "<tr class=\"item_list_title item_list_title_type_normal\">\n"
              << "\t<th class=\"item_list_title_corner\">Effective against</th>\n";

    for(uint16_t d=0; d<commonTypes.size(); ++d)
        indexBody << "<th><div class=\"type_label_list\">" << typeLabelHtml((uint8_t)d) << "</div></th>\n";

    for(uint16_t a=0; a<commonTypes.size(); ++a)
    {
        indexBody << "<tr class=\"value\"><td class=\"item_list_title_left item_list_title_type_normal\"><div class=\"type_label_list\">"
                  << typeLabelHtml((uint8_t)a) << "</div></td>\n";

        const std::unordered_map<uint8_t, int8_t> &mp=commonTypes[a].multiplicator;
        for(uint16_t d=0; d<commonTypes.size(); ++d)
        {
            std::unordered_map<uint8_t, int8_t>::const_iterator mit=mp.find((uint8_t)d);
            int8_t m=1;
            if(mit!=mp.cend())
                m=mit->second;

            float eff=multiplierToFloat(m);
            std::string effStr=multiplierToString(m);
            if(m==1) effStr="1";

            std::string cls;
            if(eff>1.0f)
                cls="very_effective";
            else if(eff==1.0f)
                cls="normal_effective";
            else if(eff==0.0f)
                cls="no_effective";
            else
                cls="not_very_effective";

            indexBody << "<td class=\"" << cls << "\">" << effStr << "</td>\n";
        }
        indexBody << "</tr>\n";
    }

    indexBody << "</table>\n";

    Helper::writeHtml("types.html","Monsters types",indexBody.str());
}

} // namespace GeneratorTypes
