#include "FullWriter.hpp"
#include "SpriteRipper.hpp"

#include <QDir>
#include <QString>
#include <QImage>
#include <QPainter>

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
    SpriteRipper ripper;
    ripper.locate(rom_);
    std::ofstream o(outRoot_ + "/items/items.xml");
    o << "<!-- Generated from gItems + gItemIconTable -->\n<items>\n";
    int icons = 0;
    int firstIcon = -1; // fallback image for the few items past the icon table
    std::size_t i = 0;
    while(i < data_.items().size())
    {
        const Gen3Item &it = data_.items()[i];
        ++i;
        const bool haveIcon = ripper.haveItemIcons() && ripper.writeItemIcon(rom_, outRoot_, it.id);
        if(haveIcon)
        {
            ++icons;
            if(firstIcon < 0) firstIcon = it.id;
        }
        o << "    <item price=\"" << it.price << "\" id=\"" << it.id << "\"";
        if(haveIcon)
            o << " image=\"icon-" << it.id << ".png\"";
        else if(firstIcon >= 0)
            o << " image=\"icon-" << firstIcon << ".png\""; // the client wants an image attribute
        o << ">\n        <name>" << xmlEscape(it.name) << "</name>\n    </item>\n";
    }
    o << "</items>\n";
    std::cerr << "FullWriter: " << icons << " item icons extracted." << std::endl;
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

int FullWriter::firstSpeciesId() const
{
    return data_.species().empty() ? 1 : data_.species().front().id;
}

// A placeholder player skin (valid dimensions) so the start profile resolves.
// front/back are the species' starter sprite tinted; trainer/swim a simple figure.
void FullWriter::writePlayerSkin()
{
    const std::string dir = outRoot_ + "/skin/fighter/player";
    QDir().mkpath(QString::fromStdString(dir));
    const QString base = QString::fromStdString(dir) + "/";
    QImage front(80, 80, QImage::Format_ARGB32);
    front.fill(Qt::transparent);
    QPainter p(&front);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QColor(70, 110, 200, 255)); p.setPen(Qt::NoPen);
    p.drawEllipse(24, 8, 32, 32);                 // head
    p.drawRoundedRect(20, 36, 40, 40, 8, 8);      // body
    p.end();
    front.save(base + "front.png", "PNG");
    front.mirrored(true, false).save(base + "back.png", "PNG");
    QImage trainer(48, 96, QImage::Format_ARGB32);
    trainer.fill(Qt::transparent);
    QPainter t(&trainer);
    t.setRenderHint(QPainter::Antialiasing, true);
    int r = 0;
    while(r < 4) { t.setBrush(QColor(70,110,200,255)); t.setPen(Qt::NoPen); t.drawEllipse(16, r*24+2, 16, 10); t.drawRect(14, r*24+12, 20, 11); ++r; }
    t.end();
    trainer.save(base + "trainer.png", "PNG");
    trainer.save(base + "swim.png", "PNG");
}

void FullWriter::writeCompleteness()
{
    writePlayerSkin();
    const int starter = firstSpeciesId();

    QDir().mkpath(QString::fromStdString(outRoot_ + "/player"));
    {
        std::ofstream o(outRoot_ + "/player/start.xml");
        o << "<profile>\n    <start id=\"Normal\">\n";
        o << "        <name>Trainer</name>\n        <description>Start a new adventure</description>\n";
        o << "        <forcedskin value=\"player\"/>\n        <cash value=\"3000\"/>\n";
        o << "        <monstergroup>\n            <monster id=\"" << starter << "\" level=\"5\" captured_with=\"4\"/>\n        </monstergroup>\n";
        o << "        <reputation type=\"nation\" level=\"1\"/>\n";
        o << "        <item quantity=\"5\" id=\"4\"/>\n";   // Poke Ball
        o << "    </start>\n</profile>\n";
    }
    {
        std::ofstream o(outRoot_ + "/player/reputation.xml");
        o << "<reputations>\n    <reputation type=\"nation\">\n        <name>Nation</name>\n";
        o << "        <level point=\"-500\"><text>Outlaw</text></level>\n";
        o << "        <level point=\"0\"><text>Citizen</text></level>\n";
        o << "        <level point=\"500\"><text>Champion</text></level>\n";
        o << "    </reputation>\n</reputations>\n";
    }
    {
        std::ofstream o(outRoot_ + "/player/event.xml");
        o << "<events>\n    <event id=\"day\">\n        <value>day</value>\n        <value>night</value>\n    </event>\n</events>\n";
    }
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map"));
    {
        std::ofstream o(outRoot_ + "/map/layers.xml");
        o << "<layers zoom=\"4\">\n";
        o << "    <monstersCollision monsterType=\"grass\" background=\"fight/grass/\" layer=\"Grass\" type=\"walkOn\"/>\n";
        o << "    <monstersCollision monsterType=\"cave\" background=\"fight/cave/\" type=\"walkOn\"/>\n";
        o << "    <monstersCollision item=\"4\" tile=\"swim\" background=\"fight/water/\" layer=\"Water\" type=\"walkOn\" monsterType=\"water\"/>\n";
        o << "</layers>\n";
    }
    {
        std::ofstream o(outRoot_ + "/map/visualcategory.xml");
        o << "<visual>\n    <category id=\"indoor\"/>\n    <category id=\"outdoor\">\n";
        o << "        <event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/>\n    </category>\n";
        o << "    <category id=\"city\"><event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/></category>\n";
        o << "    <category id=\"cave\" color=\"#000000\" alpha=\"60\"/>\n</visual>\n";
    }
    // fight backgrounds (placeholder platforms; background is solid)
    const char *terr[] = {"grass","cave","water"};
    int ti = 0;
    while(ti < 3)
    {
        const std::string d = outRoot_ + "/map/fight/" + terr[ti];
        QDir().mkpath(QString::fromStdString(d));
        QImage bg(240, 160, QImage::Format_ARGB32);
        bg.fill(ti == 1 ? QColor(40,30,50) : (ti == 2 ? QColor(60,110,170) : QColor(110,170,90)));
        bg.save(QString::fromStdString(d) + "/background.png", "PNG");
        QImage plat(160, 48, QImage::Format_ARGB32); plat.fill(Qt::transparent);
        QPainter pp(&plat); pp.setRenderHint(QPainter::Antialiasing,true); pp.setPen(Qt::NoPen);
        pp.setBrush(QColor(0,0,0,70)); pp.drawEllipse(2,2,156,44); pp.end();
        plat.save(QString::fromStdString(d) + "/plateform-background.png", "PNG");
        plat.save(QString::fromStdString(d) + "/plateform-front.png", "PNG");
        ++ti;
    }
    QDir().mkpath(QString::fromStdString(outRoot_ + "/plants"));
    { std::ofstream o(outRoot_ + "/plants/plants.xml"); o << "<plants>\n</plants>\n"; }
    QDir().mkpath(QString::fromStdString(outRoot_ + "/crafting"));
    { std::ofstream o(outRoot_ + "/crafting/recipes.xml"); o << "<recipes>\n</recipes>\n"; }
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters/buff"));
    { std::ofstream o(outRoot_ + "/monsters/buff/buff.xml"); o << "<buffs>\n</buffs>\n"; }

    std::cerr << "FullWriter: wrote completeness files (start/reputation/event/layers/visualcategory/fight-bg)" << std::endl;
}

bool FullWriter::writeAll()
{
    writeTypes();
    writeSkills();
    writeMonsters();
    writeItems();
    writeSprites();
    writeCompleteness();
    std::cerr << "FullWriter: wrote " << data_.species().size() << " monsters, "
              << data_.moves().size() << " skills, " << data_.items().size()
              << " items, " << Gen3Data::typeCount() << " types at " << outRoot_ << std::endl;
    return true;
}
