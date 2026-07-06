#include "FullWriter.hpp"
#include "OverworldSprite.hpp"
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

static bool ensureDir(const std::string &path)
{
    if(QDir().mkpath(QString::fromStdString(path)))
        return true;
    std::cerr << "FullWriter: cannot create " << path << std::endl;
    return false;
}

static bool writeTextFile(const std::string &path, const std::string &content)
{
    std::ofstream o(path);
    if(!o)
    {
        std::cerr << "FullWriter: cannot write " << path << std::endl;
        return false;
    }
    o << content;
    o.flush();
    if(!o)
    {
        std::cerr << "FullWriter: write failed: " << path << std::endl;
        return false;
    }
    return true;
}

// ── monsters/type.xml ──
bool FullWriter::writeTypes()
{
    if(!ensureDir(outRoot_ + "/monsters"))
        return false;
    const std::string path = outRoot_ + "/monsters/type.xml";
    std::ofstream o(path);
    if(!o)
    {
        std::cerr << "FullWriter: cannot write " << path << std::endl;
        return false;
    }
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
    o.flush();
    if(!o)
    {
        std::cerr << "FullWriter: write failed: " << path << std::endl;
        return false;
    }
    return true;
}

// ── monsters/skill/skill.xml ──
bool FullWriter::writeSkills()
{
    if(!ensureDir(outRoot_ + "/monsters/skill"))
        return false;
    const std::string path = outRoot_ + "/monsters/skill/skill.xml";
    std::ofstream o(path);
    if(!o)
    {
        std::cerr << "FullWriter: cannot write " << path << std::endl;
        return false;
    }
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
        if(!mv.description.empty())
            o << "        <description>" << xmlEscape(mv.description) << "</description>\n";
        o << "        <effect>\n            <level endurance=\"" << pp << "\" number=\"1\">\n";
        o << "                <life success=\"" << acc << "%\" quantity=\"-" << dmg << "\" applyOn=\"aloneEnemy\"/>\n";
        o << "            </level>\n        </effect>\n    </skill>\n";
    }
    o << "</skills>\n";
    o.flush();
    if(!o)
    {
        std::cerr << "FullWriter: write failed: " << path << std::endl;
        return false;
    }
    return true;
}

// ── monsters/monster.xml ──
bool FullWriter::writeMonsters()
{
    std::unordered_set<int> validSpecies;
    std::size_t i = 0;
    while(i < data_.species().size()) { validSpecies.insert(data_.species()[i].id); ++i; }

    if(!ensureDir(outRoot_ + "/monsters"))
        return false;
    const std::string path = outRoot_ + "/monsters/monster.xml";
    std::ofstream o(path);
    if(!o)
    {
        std::cerr << "FullWriter: cannot write " << path << std::endl;
        return false;
    }
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
        // TM/HM-taught skills: byitem = the teaching item (engine itemToLearn);
        // separate from the level list, so no dedup against it.
        std::unordered_set<int> seenItem;
        a = 0;
        while(a < s.tmLearn.size())
        {
            const int itemId = s.tmLearn[a].first;
            const int mv = s.tmLearn[a].second;
            if(seenItem.find(itemId) == seenItem.end())
            {
                seenItem.insert(itemId);
                o << "            <attack id=\"" << mv << "\" skill_level=\"1\" byitem=\"" << itemId << "\"/>\n";
            }
            ++a;
        }
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
        if(!s.kind.empty())
            o << "        <kind>" << xmlEscape(s.kind) << "</kind>\n";
        if(!s.description.empty())
            o << "        <description>" << xmlEscape(s.description) << "</description>\n";
        o << "    </monster>\n";
    }
    o << "</monsters>\n";
    o.flush();
    if(!o)
    {
        std::cerr << "FullWriter: write failed: " << path << std::endl;
        return false;
    }
    return true;
}

// ── items/items.xml ──
bool FullWriter::writeItems()
{
    if(!ensureDir(outRoot_ + "/items"))
        return false;
    SpriteRipper ripper;
    ripper.locate(rom_);
    const std::string path = outRoot_ + "/items/items.xml";
    std::ofstream o(path);
    if(!o)
    {
        std::cerr << "FullWriter: cannot write " << path << std::endl;
        return false;
    }
    o << "<!-- Generated from gItems + gItemIconTable -->\n<items>\n";
    int icons = 0;
    int firstIcon = -1; // fallback image for the few items past the icon table
    // Ruby/Sapphire have NO per-item icon data (bag icons arrived with
    // FRLG/Emerald): give every item the ripped item-ball sprite instead so
    // the client's mandatory image attribute resolves.
    if(!ripper.haveItemIcons())
    {
        const QImage ball = OverworldSprite::renderStatic(rom_, rom_.game().itemBallGfx);
        if(!ball.isNull() && ball.copy(0, 0, 16, 16).save(QString::fromStdString(outRoot_ + "/items/icon-0.png"), "PNG"))
            firstIcon = 0;
    }
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
        o << ">\n        <name>" << xmlEscape(it.name) << "</name>\n";
        if(!it.description.empty())
            o << "        <description>" << xmlEscape(it.description) << "</description>\n";
        o << "    </item>\n";
    }
    o << "</items>\n";
    o.flush();
    if(!o)
    {
        std::cerr << "FullWriter: write failed: " << path << std::endl;
        return false;
    }
    std::cerr << "FullWriter: " << icons << " item icons extracted." << std::endl;
    return true;
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
bool FullWriter::writePlayerSkin()
{
    const std::string dir = outRoot_ + "/skin/fighter/player";
    if(!ensureDir(dir))
        return false;
    const QString base = QString::fromStdString(dir) + "/";
    QImage front(80, 80, QImage::Format_ARGB32);
    front.fill(Qt::transparent);
    QPainter p(&front);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setBrush(QColor(70, 110, 200, 255)); p.setPen(Qt::NoPen);
    p.drawEllipse(24, 8, 32, 32);                 // head
    p.drawRoundedRect(20, 36, 40, 40, 8, 8);      // body
    p.end();
    bool ok = front.save(base + "front.png", "PNG");
    ok = front.mirrored(true, false).save(base + "back.png", "PNG") && ok;
    QImage trainer(48, 96, QImage::Format_ARGB32);
    trainer.fill(Qt::transparent);
    QPainter t(&trainer);
    t.setRenderHint(QPainter::Antialiasing, true);
    int r = 0;
    while(r < 4) { t.setBrush(QColor(70,110,200,255)); t.setPen(Qt::NoPen); t.drawEllipse(16, r*24+2, 16, 10); t.drawRect(14, r*24+12, 20, 11); ++r; }
    t.end();
    ok = trainer.save(base + "trainer.png", "PNG") && ok;
    ok = trainer.save(base + "swim.png", "PNG") && ok;
    if(!ok)
        std::cerr << "FullWriter: cannot write player skin under " << dir << std::endl;
    return ok;
}

bool FullWriter::writeCompleteness()
{
    if(!writePlayerSkin())
        return false;
    // The game's REAL starter trio, matched by NAME (no offsets); every ROM
    // holds all 386 species, so the ENGINE picks which trio comes first (Kanto
    // on FRLG, Hoenn on RSE — the other stays as a hack fallback).  Each is one
    // <monstergroup> CHOICE like the base datapack.  Fallback: first species.
    std::vector<int> starters;
    {
        const bool frlg = (rom_.game().engine == Engine::Frlg);
        const char *trios[2][3] = {{frlg?"Bulbasaur":"Treecko", frlg?"Charmander":"Torchic", frlg?"Squirtle":"Mudkip"},
                                   {frlg?"Treecko":"Bulbasaur", frlg?"Torchic":"Charmander", frlg?"Mudkip":"Squirtle"}};
        int tr = 0;
        while(tr < 2 && starters.empty())
        {
            std::vector<int> got;
            int n = 0;
            while(n < 3)
            {
                std::size_t si = 0;
                while(si < data_.species().size())
                {
                    if(data_.species()[si].name == trios[tr][n])
                    {
                        got.push_back(data_.species()[si].id);
                        break;
                    }
                    ++si;
                }
                ++n;
            }
            if(got.size() == 3)
                starters = got;
            ++tr;
        }
        if(starters.empty())
            starters.push_back(firstSpeciesId());
    }

    if(!ensureDir(outRoot_ + "/player"))
        return false;
    std::string start;
    start += "<profile>\n    <start id=\"Normal\">\n";
    start += "        <name>Trainer</name>\n        <description>Start a new adventure</description>\n";
    start += "        <forcedskin value=\"player\"/>\n        <cash value=\"3000\"/>\n";
    std::size_t sg = 0;
    while(sg < starters.size())
    {
        start += "        <monstergroup>\n            <monster id=\"" + std::to_string(starters[sg]) + "\" level=\"5\" captured_with=\"4\"/>\n        </monstergroup>\n";
        ++sg;
    }
    start += "        <reputation type=\"nation\" level=\"1\"/>\n";
    start += "        <item quantity=\"5\" id=\"4\"/>\n";   // Poke Ball
    start += "    </start>\n</profile>\n";
    if(!writeTextFile(outRoot_ + "/player/start.xml", start))
        return false;
    if(!writeTextFile(outRoot_ + "/player/reputation.xml",
            "<reputations>\n    <reputation type=\"nation\">\n        <name>Nation</name>\n"
            "        <level point=\"-500\"><text>Outlaw</text></level>\n"
            "        <level point=\"0\"><text>Citizen</text></level>\n"
            "        <level point=\"500\"><text>Champion</text></level>\n"
            "    </reputation>\n</reputations>\n"))
        return false;
    if(!writeTextFile(outRoot_ + "/player/event.xml",
            "<events>\n    <event id=\"day\">\n        <value>day</value>\n        <value>night</value>\n    </event>\n</events>\n"))
        return false;
    if(!ensureDir(outRoot_ + "/map"))
        return false;
    // monsterType names match the base datapack's existing conventions
    // (waterRod/waterSuperRod); rockSmash is new, bound to the RockSmash
    // actionOn layer the map writer emits.  Item ids are the ROM's own:
    // 262 Old Rod, 263 Good Rod, 264 Super Rod, 344 HM06 Rock Smash.
    if(!writeTextFile(outRoot_ + "/map/layers.xml",
            "<layers zoom=\"4\">\n"
            "    <monstersCollision monsterType=\"grass\" background=\"fight/grass/\" layer=\"Grass\" type=\"walkOn\"/>\n"
            "    <monstersCollision monsterType=\"cave\" background=\"fight/cave/\" type=\"walkOn\"/>\n"
            "    <monstersCollision item=\"4\" tile=\"swim\" background=\"fight/water/\" layer=\"Water\" type=\"walkOn\" monsterType=\"water\"/>\n"
            "    <monstersCollision item=\"262\" tile=\"fish\" background=\"fight/water/\" layer=\"Water\" type=\"actionOn\" monsterType=\"waterRod\"/>\n"
            "    <monstersCollision item=\"263\" tile=\"fish\" background=\"fight/water/\" layer=\"Water\" type=\"actionOn\" monsterType=\"waterRod\"/>\n"
            "    <monstersCollision item=\"264\" tile=\"fish\" background=\"fight/water/\" layer=\"Water\" type=\"actionOn\" monsterType=\"waterSuperRod\"/>\n"
            "    <monstersCollision item=\"344\" background=\"fight/cave/\" layer=\"RockSmash\" type=\"actionOn\" monsterType=\"rockSmash\"/>\n"
            "</layers>\n"))
        return false;
    if(!writeTextFile(outRoot_ + "/map/visualcategory.xml",
            "<visual>\n    <category id=\"indoor\"/>\n    <category id=\"outdoor\">\n"
            "        <event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/>\n    </category>\n"
            "    <category id=\"city\"><event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/></category>\n"
            "    <category id=\"cave\" color=\"#000000\" alpha=\"60\"/>\n</visual>\n"))
        return false;
    // fight backgrounds (placeholder platforms; background is solid)
    const char *terr[] = {"grass","cave","water"};
    int ti = 0;
    while(ti < 3)
    {
        const std::string d = outRoot_ + "/map/fight/" + terr[ti];
        if(!ensureDir(d))
            return false;
        QImage bg(240, 160, QImage::Format_ARGB32);
        bg.fill(ti == 1 ? QColor(40,30,50) : (ti == 2 ? QColor(60,110,170) : QColor(110,170,90)));
        bool ok = bg.save(QString::fromStdString(d) + "/background.png", "PNG");
        QImage plat(160, 48, QImage::Format_ARGB32); plat.fill(Qt::transparent);
        QPainter pp(&plat); pp.setRenderHint(QPainter::Antialiasing,true); pp.setPen(Qt::NoPen);
        pp.setBrush(QColor(0,0,0,70)); pp.drawEllipse(2,2,156,44); pp.end();
        ok = plat.save(QString::fromStdString(d) + "/plateform-background.png", "PNG") && ok;
        ok = plat.save(QString::fromStdString(d) + "/plateform-front.png", "PNG") && ok;
        if(!ok)
        {
            std::cerr << "FullWriter: cannot write fight background under " << d << std::endl;
            return false;
        }
        ++ti;
    }
    if(!ensureDir(outRoot_ + "/plants"))
        return false;
    // RSE berries -> plants: itemUsed = the berry item itself (plant one, grow
    // more).  1 Gen3 hour = 1 CC minute (4 equal stages -> fruits = 4*stage,
    // engine defaults sprouted/taller/flowering to the quarters); quantity =
    // average yield, the fraction being the engine's extra-fruit luck.
    // A berry-less game (FRLG) must NOT clobber plants a sibling --all run
    // already wrote into the shared root: keep the existing file.
    std::ifstream existingPlants(outRoot_ + "/plants/plants.xml");
    const bool keepPlants = data_.berries().empty() &&
        existingPlants && existingPlants.peek() != std::ifstream::traits_type::eof();
    if(!keepPlants)
    {
        std::string plants = "<plants>\n";
        std::size_t bi = 0;
        while(bi < data_.berries().size())
        {
            const Gen3Berry &b = data_.berries()[bi];
            ++bi;
            int fruits = b.stageHours * 4;
            if(fruits < 1) fruits = 1;
            if(fruits > 108) fruits = 108; // engine 16-bit seconds cap
            const int avg2 = b.minYield + b.maxYield; // 2*average
            plants += "    <plant id=\"" + std::to_string(b.index + 1) + "\" itemUsed=\"" + std::to_string(133 + b.index) + "\">\n";
            plants += "        <!-- " + b.name + " -->\n";
            plants += "        <grow><fruits>" + std::to_string(fruits) + "</fruits></grow>\n";
            plants += "        <quantity>" + std::to_string(avg2 / 2) + (avg2 % 2 ? ".5" : "") + "</quantity>\n";
            plants += "    </plant>\n";
        }
        plants += "</plants>\n";
        if(!writeTextFile(outRoot_ + "/plants/plants.xml", plants))
            return false;
    }
    if(!ensureDir(outRoot_ + "/crafting"))
        return false;
    if(!writeTextFile(outRoot_ + "/crafting/recipes.xml", "<recipes>\n</recipes>\n"))
        return false;
    if(!ensureDir(outRoot_ + "/monsters/buff"))
        return false;
    if(!writeTextFile(outRoot_ + "/monsters/buff/buff.xml", "<buffs>\n</buffs>\n"))
        return false;

    std::cerr << "FullWriter: wrote completeness files (start/reputation/event/layers/visualcategory/fight-bg)" << std::endl;
    return true;
}

bool FullWriter::writeAll()
{
    if(!writeTypes())
        return false;
    if(!writeSkills())
        return false;
    if(!writeMonsters())
        return false;
    if(!writeItems())
        return false;
    writeSprites();
    if(!writeCompleteness())
        return false;
    std::cerr << "FullWriter: wrote " << data_.species().size() << " monsters, "
              << data_.moves().size() << " skills, " << data_.items().size()
              << " items, " << Gen3Data::typeCount() << " types at " << outRoot_ << std::endl;
    return true;
}
