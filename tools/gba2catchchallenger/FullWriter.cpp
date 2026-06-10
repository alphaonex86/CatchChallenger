#include "FullWriter.hpp"
#include "SpriteRipper.hpp"

#include <QDir>
#include <QString>

#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <cctype>

FullWriter::FullWriter(const GbaRom &rom, const Gen3Data &data, const std::string &outRoot)
    : rom_(rom), data_(data), outRoot_(outRoot)
{
}

static std::string xmlEscape(const std::string &s)
{
    std::string o;
    std::size_t i = 0;
    while(i < s.size())
    {
        const char c = s[i];
        if(c == '&') o += "&amp;";
        else if(c == '<') o += "&lt;";
        else if(c == '>') o += "&gt;";
        else if(c == '"') o += "&quot;";
        else o.push_back(c);
        ++i;
    }
    return o;
}

// ── monsters/type.xml ──
void FullWriter::writeTypes()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters"));
    std::ofstream o(outRoot_ + "/monsters/type.xml");
    o << "<!-- Generated from the Gen3 type chart -->\n<types>\n";
    int t = 0;
    while(t < Gen3Data::typeCount())
    {
        const std::string name = Gen3Data::typeName(t);
        o << "    <type name=\"" << name << "\">\n";
        o << "        <name>" << (char)std::toupper(name[0]) << name.substr(1) << "</name>\n";
        std::string sup, weak, immune;
        std::size_t m = 0;
        while(m < data_.typeChart().size())
        {
            const Gen3TypeMatch &tm = data_.typeChart()[m];
            if(tm.atk == t)
            {
                const std::string d = Gen3Data::typeName(tm.def);
                if(tm.mult == 2.0)      { if(!sup.empty()) sup += ";"; sup += d; }
                else if(tm.mult == 0.5) { if(!weak.empty()) weak += ";"; weak += d; }
                else if(tm.mult == 0.0) { if(!immune.empty()) immune += ";"; immune += d; }
            }
            ++m;
        }
        if(!sup.empty())    o << "        <multiplicator number=\"2\" to=\"" << sup << "\"/>\n";
        if(!weak.empty())   o << "        <multiplicator number=\"0.5\" to=\"" << weak << "\"/>\n";
        if(!immune.empty()) o << "        <multiplicator number=\"0\" to=\"" << immune << "\"/>\n";
        o << "    </type>\n";
        ++t;
    }
    o << "</types>\n";
}

// ── monsters/skill/skill.xml ──
void FullWriter::writeSkills()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters/skill"));
    std::ofstream o(outRoot_ + "/monsters/skill/skill.xml");
    o << "<!-- Generated from gBattleMoves. id=0 is the mandatory fallback -->\n<skills>\n";
    o << "    <skill type=\"normal\" category=\"Physical\" id=\"0\">\n";
    o << "        <name>Struggle</name>\n";
    o << "        <effect>\n            <level endurance=\"40\" number=\"1\">\n";
    o << "                <life success=\"100%\" quantity=\"-15%\" applyOn=\"aloneEnemy\"/>\n";
    o << "            </level>\n        </effect>\n    </skill>\n";

    std::size_t i = 0;
    while(i < data_.moves().size())
    {
        const Gen3Move &mv = data_.moves()[i];
        ++i;
        int acc = mv.accuracy;
        if(acc <= 0 || acc > 100) acc = 100;     // 0 = always-hit moves (Swift)
        int dmg = mv.power > 0 ? mv.power : 1;    // status moves -> token damage
        int pp = mv.pp > 0 ? mv.pp : 5;
        o << "    <skill id=\"" << mv.id << "\" type=\"" << Gen3Data::typeName(mv.type) << "\" category=\"Physical\">\n";
        o << "        <name>" << xmlEscape(mv.name) << "</name>\n";
        o << "        <effect>\n            <level endurance=\"" << pp << "\" number=\"1\">\n";
        o << "                <life success=\"" << acc << "%\" quantity=\"-" << dmg << "\" applyOn=\"aloneEnemy\"/>\n";
        o << "            </level>\n        </effect>\n    </skill>\n";
    }
    o << "</skills>\n";
}

// ── monsters/monster.xml ──
void FullWriter::writeMonsters()
{
    std::unordered_set<int> validSpecies;
    std::size_t i = 0;
    while(i < data_.species().size()) { validSpecies.insert(data_.species()[i].id); ++i; }

    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters"));
    std::ofstream o(outRoot_ + "/monsters/monster.xml");
    o << "<!-- Generated from gBaseStats/gLevelUpLearnsets/gEvolutionTable -->\n<monsters>\n";

    i = 0;
    while(i < data_.species().size())
    {
        const Gen3Species &s = data_.species()[i];
        ++i;
        const int hp = 2 * s.hp + 110;
        const int atk = 2 * s.attack + 5, def = 2 * s.defense + 5;
        const int spa = 2 * s.spAttack + 5, spd = 2 * s.spDefense + 5, spe = 2 * s.speed + 5;
        int catchR = (int)(s.catchRate / 2.55 + 0.5);
        if(catchR < 1) catchR = 1; if(catchR > 100) catchR = 100;
        long giveXp = (long)s.baseExp * 40;
        if(giveXp < 100) giveXp = 100;
        const int eggStep = s.eggCycles > 0 ? s.eggCycles * 256 : 5120;
        std::string ratio;
        if(s.genderRatio == 255) ratio = "-1";
        else ratio = std::to_string((int)(s.genderRatio / 256.0 * 100.0 + 0.5)) + "%";

        o << "    <monster id=\"" << s.id << "\" type=\"" << Gen3Data::typeName(s.type1) << "\"";
        if(s.type2 != s.type1) o << " type2=\"" << Gen3Data::typeName(s.type2) << "\"";
        o << " hp=\"" << hp << "\" attack=\"" << atk << "\" defense=\"" << def
          << "\" special_attack=\"" << spa << "\" special_defense=\"" << spd
          << "\" speed=\"" << spe << "\" pow=\"2\" catch_rate=\"" << catchR
          << "\" ratio_gender=\"" << ratio << "\" egg_step=\"" << eggStep
          << "\" xp_max=\"1000000\" give_xp=\"" << giveXp << "\">\n";

        // attack_list (dedup by skill; engine keys learned moves on skill+attack_level)
        o << "        <attack_list>\n";
        std::unordered_set<int> seen;
        bool any = false;
        std::size_t a = 0;
        while(a < s.learnset.size())
        {
            const int lvl = s.learnset[a].first;
            const int mv = s.learnset[a].second;
            if(seen.find(mv) == seen.end())
            {
                seen.insert(mv);
                o << "            <attack id=\"" << mv << "\" level=\"" << lvl << "\"/>\n";
                any = true;
            }
            ++a;
        }
        if(!any) o << "            <attack id=\"0\" level=\"0\"/>\n";
        o << "        </attack_list>\n";

        // a single evolution (engine constraint), to a valid species
        std::size_t e = 0;
        bool wrote = false;
        while(e < s.evolutions.size() && !wrote)
        {
            const int kind = s.evolutions[e].first;        // 1 level, 2 item
            const int param = s.evolutions[e].second.first;
            const int target = s.evolutions[e].second.second;
            if(validSpecies.find(target) != validSpecies.end())
            {
                o << "        <evolutions>\n";
                if(kind == 2)
                    o << "            <evolution type=\"item\" item=\"" << param << "\" evolveTo=\"" << target << "\"/>\n";
                else
                    o << "            <evolution type=\"level\" level=\"" << param << "\" evolveTo=\"" << target << "\"/>\n";
                o << "        </evolutions>\n";
                wrote = true;
            }
            ++e;
        }

        o << "        <name>" << xmlEscape(s.name) << "</name>\n";
        o << "    </monster>\n";
    }
    o << "</monsters>\n";
}

// ── items/items.xml ──
void FullWriter::writeItems()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/items"));
    std::ofstream o(outRoot_ + "/items/items.xml");
    o << "<!-- Generated from gItems -->\n<items>\n";
    std::size_t i = 0;
    while(i < data_.items().size())
    {
        const Gen3Item &it = data_.items()[i];
        ++i;
        o << "    <item price=\"" << it.price << "\" id=\"" << it.id << "\">\n";
        o << "        <name>" << xmlEscape(it.name) << "</name>\n";
        o << "    </item>\n";
    }
    o << "</items>\n";
}

// ── monster battle sprites ──
void FullWriter::writeSprites()
{
    SpriteRipper ripper;
    if(!ripper.locate(rom_))
    {
        std::cerr << "FullWriter: could not locate sprite tables; monsters have no sprites." << std::endl;
        return;
    }
    int ok = 0, miss = 0;
    std::size_t i = 0;
    while(i < data_.species().size())
    {
        if(ripper.writeSpecies(rom_, outRoot_, data_.species()[i].id))
            ++ok;
        else
            ++miss;
        ++i;
    }
    std::cerr << "FullWriter: " << ok << " sprites extracted, " << miss << " missing." << std::endl;
}

bool FullWriter::writeAll()
{
    writeTypes();
    writeSkills();
    writeMonsters();
    writeItems();
    writeSprites();
    std::cerr << "FullWriter: wrote " << data_.species().size() << " monsters, "
              << data_.moves().size() << " skills, " << data_.items().size()
              << " items, " << Gen3Data::typeCount() << " types at " << outRoot_ << std::endl;
    return true;
}
