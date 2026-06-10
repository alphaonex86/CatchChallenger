#include "DatapackWriter.hpp"
#include "SpriteExtractor.hpp"

#include <yaml-cpp/yaml.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QString>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

namespace tuxemon {

DatapackWriter::DatapackWriter(const TuxemonDb &db, const Localization &l10n,
                               const std::string &modRoot, const std::string &outRoot)
    : db_(db), l10n_(l10n), modRoot_(modRoot), outRoot_(outRoot)
{
}

// ── helpers ─────────────────────────────────────────────────────────────────
static std::string xmlEscape(const std::string &s)
{
    std::string out;
    out.reserve(s.size());
    std::size_t i = 0;
    while(i < s.size())
    {
        const char c = s[i];
        if(c == '&')       out += "&amp;";
        else if(c == '<')  out += "&lt;";
        else if(c == '>')  out += "&gt;";
        else if(c == '"')  out += "&quot;";
        else               out.push_back(c);
        ++i;
    }
    return out;
}

static int iround(double v)
{
    return (int)std::floor(v + 0.5);
}

// comparison functions (project style forbids lambdas)
static bool monsterByIdLess(const Monster *a, const Monster *b)
{
    return a->txmnId < b->txmnId;
}
static bool itemBySlugLess(const Item *a, const Item *b)
{
    // ids are assigned in slug-sorted order, so slug order == id order
    return a->slug < b->slug;
}

static bool approx(double a, double b)
{
    return std::fabs(a - b) < 0.01;
}

// Make a YAML key a valid XML element name.
static std::string sanitizeTag(const std::string &k)
{
    std::string t;
    std::size_t i = 0;
    while(i < k.size())
    {
        const char c = k[i];
        if(std::isalnum((unsigned char)c) || c == '_' || c == '-')
            t.push_back(c);
        else
            t.push_back('_');
        ++i;
    }
    if(t.empty())
        t = "x";
    if(std::isdigit((unsigned char)t[0]))
        t = "_" + t;
    return t;
}

// Recursively dump a YAML node as nested XML.  Generic: every Tuxemon field
// (including ones added in future Tuxemon versions) is preserved automatically.
// The engine ignores the enclosing <tuxemon> element entirely.
static void dumpYaml(std::ofstream &o, const std::string &indent,
                     const std::string &key, const YAML::Node &n)
{
    const std::string tag = sanitizeTag(key);
    if(!n.IsDefined() || n.IsNull())
    {
        o << indent << "<" << tag << "/>\n";
        return;
    }
    if(n.IsScalar())
    {
        o << indent << "<" << tag << ">" << xmlEscape(n.Scalar()) << "</" << tag << ">\n";
        return;
    }
    if(n.IsSequence())
    {
        o << indent << "<" << tag << ">\n";
        std::size_t i = 0;
        while(i < n.size()) { dumpYaml(o, indent + "  ", "item", n[i]); ++i; }
        o << indent << "</" << tag << ">\n";
        return;
    }
    if(n.IsMap())
    {
        o << indent << "<" << tag << ">\n";
        YAML::const_iterator it = n.begin();
        while(it != n.end())
        {
            dumpYaml(o, indent + "  ", it->first.Scalar(), it->second);
            ++it;
        }
        o << indent << "</" << tag << ">\n";
    }
}

// Emit the whole raw Tuxemon record under a <tuxemon> element (engine-ignored).
static void writeTuxemon(std::ofstream &o, const std::string &indent, const YAML::Node &raw)
{
    if(!raw.IsDefined() || !raw.IsMap())
        return;
    o << indent << "<tuxemon>\n";
    YAML::const_iterator it = raw.begin();
    while(it != raw.end())
    {
        dumpYaml(o, indent + "    ", it->first.Scalar(), it->second);
        ++it;
    }
    o << indent << "</tuxemon>\n";
}

// A display colour for each Tuxemon element (cosmetic; the engine only needs
// the type names to match).
static const char *colorFor(const std::string &slug)
{
    if(slug == "fire")      return "#f38233";
    if(slug == "water")     return "#2E9EF3";
    if(slug == "earth")     return "#926E3B";
    if(slug == "metal")     return "#B5B5B5";
    if(slug == "wood")      return "#71DA5E";
    if(slug == "normal")    return "#C8C8A8";
    if(slug == "frost")     return "#9AD9D9";
    if(slug == "lightning") return "#FFEC10";
    if(slug == "venom")     return "#A040A0";
    if(slug == "cosmic")    return "#7038F8";
    if(slug == "shadow")    return "#4A3A5A";
    if(slug == "sky")       return "#8AB8E8";
    if(slug == "heroic")    return "#E8C84A";
    return "#9090A0";
}

void DatapackWriter::writeNameDesc(std::ofstream &o, const char *indent,
                                   const std::string &slug, bool withDescription)
{
    const std::string nameEn = l10n_.nameEn(slug);
    const std::string nameFr = l10n_.nameFr(slug);
    o << indent << "<name>" << xmlEscape(nameEn) << "</name>\n";
    if(!nameFr.empty() && nameFr != nameEn)
        o << indent << "<name lang=\"fr\">" << xmlEscape(nameFr) << "</name>\n";
    if(withDescription)
    {
        const std::string descEn = l10n_.descEn(slug);
        const std::string descFr = l10n_.descFr(slug);
        if(!descEn.empty())
            o << indent << "<description>" << xmlEscape(descEn) << "</description>\n";
        if(!descFr.empty() && descFr != descEn)
            o << indent << "<description lang=\"fr\">" << xmlEscape(descFr) << "</description>\n";
    }
}

int DatapackWriter::skillId(const std::string &slug) const
{
    std::unordered_map<std::string,int>::const_iterator it = techToId_.find(slug);
    return it == techToId_.end() ? -1 : it->second;
}
int DatapackWriter::buffId(const std::string &slug) const
{
    std::unordered_map<std::string,int>::const_iterator it = statusToId_.find(slug);
    return it == statusToId_.end() ? -1 : it->second;
}
int DatapackWriter::monsterId(const std::string &slug) const
{
    std::unordered_map<std::string,int>::const_iterator it = monsterToId_.find(slug);
    return it == monsterToId_.end() ? -1 : it->second;
}
int DatapackWriter::idForItem(const std::string &slug) const
{
    std::unordered_map<std::string,int>::const_iterator it = itemToId_.find(slug);
    return it == itemToId_.end() ? -1 : it->second;
}
int DatapackWriter::idForMonster(const std::string &slug) const
{
    return monsterId(slug);
}

// ── id maps ─────────────────────────────────────────────────────────────────
void DatapackWriter::buildIdMaps()
{
    // Monsters keep their Tuxemon txmn_id.
    std::size_t i = 0;
    while(i < db_.monsters().size())
    {
        monsterToId_[db_.monsters()[i].slug] = db_.monsters()[i].txmnId;
        ++i;
    }

    // Techniques: sequential ids from 1 (0 is reserved for "Last luck").
    std::vector<std::string> tslugs;
    i = 0;
    while(i < db_.techniques().size())
    {
        tslugs.push_back(db_.techniques()[i].slug);
        ++i;
    }
    std::sort(tslugs.begin(), tslugs.end());
    int next = 1;
    i = 0;
    while(i < tslugs.size())
    {
        techToId_[tslugs[i]] = next++;
        ++i;
    }

    // Statuses: sequential buff ids from 1.
    std::vector<std::string> sslugs;
    i = 0;
    while(i < db_.statuses().size())
    {
        sslugs.push_back(db_.statuses()[i].slug);
        ++i;
    }
    std::sort(sslugs.begin(), sslugs.end());
    next = 1;
    i = 0;
    while(i < sslugs.size())
    {
        statusToId_[sslugs[i]] = next++;
        ++i;
    }

    // Items: sequential ids from 1.
    std::vector<std::string> islugs;
    i = 0;
    while(i < db_.items().size())
    {
        islugs.push_back(db_.items()[i].slug);
        ++i;
    }
    std::sort(islugs.begin(), islugs.end());
    next = 1;
    i = 0;
    while(i < islugs.size())
    {
        itemToId_[islugs[i]] = next++;
        ++i;
    }
}

// ── informations.xml ────────────────────────────────────────────────────────
void DatapackWriter::writeInformations()
{
    std::ofstream o(outRoot_ + "/informations.xml");
    o << "<informations>\n";
    o << "    <author pseudo=\"Tuxemon\" email=\"\" name=\"Tuxemon project\"/>\n";
    o << "    <author pseudo=\"tuxemon2catchchallenger\" email=\"\" name=\"converter\"/>\n";
    o << "    <name>Tuxemon datapack</name>\n";
    o << "    <name lang=\"fr\">Datapack Tuxemon</name>\n";
    o << "    <description>Converted from the Tuxemon game data (https://www.tuxemon.org/). "
         "Monsters, techniques, types and items only.</description>\n";
    o << "</informations>\n";
}

// ── monsters/type.xml ────────────────────────────────────────────────────────
void DatapackWriter::writeTypes()
{
    std::ofstream o(outRoot_ + "/monsters/type.xml");
    o << "<!-- Generated from Tuxemon db/element; multiplicator 2/0.5/0, default 1 omitted -->\n";
    o << "<types>\n";
    std::size_t e = 0;
    while(e < db_.elements().size())
    {
        const Element &el = db_.elements()[e];
        o << "    <type name=\"" << el.slug << "\" color=\"" << colorFor(el.slug) << "\">\n";
        writeNameDesc(o, "        ", el.slug, false);

        // Group matchups by multiplier value.
        std::string sup, weak, immune;
        std::size_t m = 0;
        while(m < el.matchups.size())
        {
            const Matchup &mu = el.matchups[m];
            if(approx(mu.mult, 2.0))      { if(!sup.empty()) sup += ";"; sup += mu.against; }
            else if(approx(mu.mult, 0.5)) { if(!weak.empty()) weak += ";"; weak += mu.against; }
            else if(approx(mu.mult, 0.0)) { if(!immune.empty()) immune += ";"; immune += mu.against; }
            ++m;
        }
        if(!sup.empty())
            o << "        <multiplicator number=\"2\" to=\"" << sup << "\"/>\n";
        if(!weak.empty())
            o << "        <multiplicator number=\"0.5\" to=\"" << weak << "\"/>\n";
        if(!immune.empty())
            o << "        <multiplicator number=\"0\" to=\"" << immune << "\"/>\n";
        writeTuxemon(o, "        ", el.raw);
        o << "    </type>\n";
        ++e;
    }
    o << "</types>\n";
}

// ── monsters/buff/buff.xml ───────────────────────────────────────────────────
void DatapackWriter::writeBuffs()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters/buff"));
    std::ofstream o(outRoot_ + "/monsters/buff/buff.xml");
    o << "<!-- Generated from Tuxemon db/status -->\n";
    o << "<buffs>\n";

    std::size_t s = 0;
    while(s < db_.statuses().size())
    {
        const Status &st = db_.statuses()[s];
        const int id = buffId(st.slug);
        if(id < 0) { ++s; continue; }

        // duration: persistent statuses last "Always", others a few turns.
        std::string durAttr;
        if(st.persists)
            durAttr = "duration=\"Always\"";
        else
            durAttr = "duration=\"NumberOfTurn\" durationNumberOfTurn=\"3\"";

        // Per-turn damage if the status is a damaging negative one.
        std::string hp;
        if(st.category == "negative" && !st.effects.empty())
        {
            // first numeric effect parameter = damage amount
            std::size_t k = 0;
            while(k < st.effects.size() && hp.empty())
            {
                if(!st.effects[k].params.empty())
                {
                    const std::string &p0 = st.effects[k].params[0];
                    bool numeric = !p0.empty();
                    std::size_t c = 0;
                    while(c < p0.size()) { if(!std::isdigit((unsigned char)p0[c])) numeric = false; ++c; }
                    if(numeric)
                        hp = "-" + p0;
                }
                ++k;
            }
            if(hp.empty() && st.stepType == "percent_damage")
                hp = "-5%";
        }

        o << "    <buff id=\"" << id << "\">\n";
        writeNameDesc(o, "        ", st.slug, true);
        o << "        <effect " << durAttr << ">\n";
        o << "            <level number=\"1\">\n";
        if(!hp.empty())
        {
            o << "                <inFight hp=\"" << hp << "\"/>\n";
            if(st.persists)
                o << "                <inWalk steps=\"50\" hp=\"" << hp << "\"/>\n";
        }
        o << "            </level>\n";
        o << "        </effect>\n";
        writeTuxemon(o, "        ", st.raw);
        o << "    </buff>\n";
        ++s;
    }
    o << "</buffs>\n";
}

// ── monsters/skill/skill.xml ─────────────────────────────────────────────────
void DatapackWriter::writeSkills()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters/skill"));
    std::ofstream o(outRoot_ + "/monsters/skill/skill.xml");
    o << "<!-- Generated from Tuxemon db/technique. id=0 is the mandatory fallback -->\n";
    o << "<skills>\n";

    // Mandatory fallback attack.
    o << "    <skill type=\"normal\" category=\"Physical\" id=\"0\">\n";
    o << "        <name>Last luck</name>\n";
    o << "        <name lang=\"fr\">Derniére chance</name>\n";
    o << "        <description>Small attack when can fight anymore</description>\n";
    o << "        <effect>\n";
    o << "            <level endurance=\"40\" number=\"1\">\n";
    o << "                <life success=\"100%\" quantity=\"-15%\" applyOn=\"aloneEnemy\"/>\n";
    o << "            </level>\n";
    o << "        </effect>\n";
    o << "    </skill>\n";

    std::size_t t = 0;
    while(t < db_.techniques().size())
    {
        const Technique &tech = db_.techniques()[t];
        const int id = skillId(tech.slug);
        if(id < 0) { ++t; continue; }

        std::string type = tech.types.empty() ? std::string("normal") : tech.types[0];

        // accuracy -> success %, power -> flat damage, power -> PP.
        int acc = iround(tech.accuracy * 100.0);
        if(acc < 1) acc = 1; if(acc > 100) acc = 100;
        int dmg = iround(tech.power * 60.0);
        if(dmg < 5) dmg = 5;
        int pp = iround(40.0 / std::max(0.5, tech.power));
        if(pp < 5) pp = 5; if(pp > 50) pp = 50;

        bool hasDamage = false;
        bool hasGive = false;
        std::size_t k = 0;
        while(k < tech.effects.size())
        {
            if(tech.effects[k].type == "damage") hasDamage = true;
            if(tech.effects[k].type == "give")   hasGive = true;
            ++k;
        }
        // No explicit effects -> assume a plain damage move.
        if(tech.effects.empty())
            hasDamage = true;

        const char *applyOn = tech.targetSelf ? "themself" : "aloneEnemy";

        o << "    <skill id=\"" << id << "\" type=\"" << type << "\" category=\"Physical\">\n";
        writeNameDesc(o, "        ", tech.slug, true);
        o << "        <effect>\n";
        o << "            <level endurance=\"" << pp << "\" number=\"1\">\n";

        bool emitted = false;
        if(hasDamage)
        {
            o << "                <life success=\"" << acc << "%\" quantity=\"-" << dmg
              << "\" applyOn=\"" << applyOn << "\"/>\n";
            emitted = true;
        }
        if(hasGive)
        {
            // emit a buff for each give effect whose status is known
            k = 0;
            while(k < tech.effects.size())
            {
                if(tech.effects[k].type == "give" && !tech.effects[k].params.empty())
                {
                    const int bid = buffId(tech.effects[k].params[0]);
                    if(bid >= 0)
                    {
                        int succ = tech.potency > 0.0 ? iround(tech.potency * 100.0)
                                                      : (hasDamage ? 20 : 100);
                        if(succ < 1) succ = 1; if(succ > 100) succ = 100;
                        o << "                <buff id=\"" << bid << "\" success=\"" << succ << "%\"/>\n";
                        emitted = true;
                    }
                }
                ++k;
            }
        }
        if(!emitted)
            o << "                <life success=\"" << acc << "%\" quantity=\"-" << dmg
              << "\" applyOn=\"aloneEnemy\"/>\n";

        o << "            </level>\n";
        o << "        </effect>\n";
        writeTuxemon(o, "        ", tech.raw);
        o << "    </skill>\n";
        ++t;
    }
    o << "</skills>\n";
}

// Map a Tuxemon shape + evolution stage to the 6 CatchChallenger stats
// (interpreted as the level-100 values).  Shape attributes are 0..10.
struct CcStats { int hp, attack, defense, spatk, spdef, speed; };
static CcStats computeStats(const Shape &sh, const std::string &stage)
{
    double mul = 1.0;
    if(stage == "stage1") mul = 1.18;
    else if(stage == "stage2") mul = 1.35;
    CcStats s;
    s.hp     = iround((60 + sh.hp     * 24) * mul);
    s.attack = iround((30 + sh.melee  * 17) * mul);
    s.spatk  = iround((30 + sh.ranged * 17) * mul);
    s.defense= iround((30 + sh.armour * 17) * mul);
    s.spdef  = iround((30 + sh.dodge  * 17) * mul);
    s.speed  = iround((30 + sh.speed  * 17) * mul);
    if(s.hp < 1) s.hp = 1;
    if(s.attack < 1) s.attack = 1;
    if(s.spatk < 1) s.spatk = 1;
    if(s.defense < 1) s.defense = 1;
    if(s.spdef < 1) s.spdef = 1;
    if(s.speed < 1) s.speed = 1;
    return s;
}

// ── monsters/monster.xml ─────────────────────────────────────────────────────
void DatapackWriter::writeMonsters()
{
    // sort by id for a stable, readable file
    std::vector<const Monster*> list;
    std::size_t i = 0;
    while(i < db_.monsters().size()) { list.push_back(&db_.monsters()[i]); ++i; }
    std::sort(list.begin(), list.end(), monsterByIdLess);

    std::ofstream o(outRoot_ + "/monsters/monster.xml");
    o << "<!-- Generated from Tuxemon db/monster. Stats derived from the body shape. -->\n";
    o << "<monsters>\n";

    i = 0;
    while(i < list.size())
    {
        const Monster &m = *list[i];
        ++i;

        Shape sh;
        std::map<std::string,Shape>::const_iterator sit = db_.shapes().find(m.shape);
        if(sit != db_.shapes().end())
            sh = sit->second;
        const CcStats st = computeStats(sh, m.stage);
        const long total = (long)st.hp + st.attack + st.defense + st.spatk + st.spdef + st.speed;
        long giveXp = total * 12;
        if(giveXp < 100) giveXp = 100;
        const long giveSp = giveXp / 30;

        int catchRate = iround(m.catchRate);
        if(catchRate < 0) catchRate = 0; if(catchRate > 100) catchRate = 100;

        // gender ratio = % female; -1 when genderless
        std::string ratio;
        const double gtot = m.genderMale + m.genderFemale;
        if(gtot <= 0.0)
            ratio = "-1";
        else
            ratio = std::to_string(iround(m.genderFemale / gtot * 100.0)) + "%";

        o << "    <monster id=\"" << m.txmnId << "\" type=\"" << (m.types.empty() ? "normal" : m.types[0]) << "\"";
        if(m.types.size() >= 2 && m.types[1] != m.types[0])
            o << " type2=\"" << m.types[1] << "\"";
        o << " hp=\"" << st.hp << "\" attack=\"" << st.attack << "\" defense=\"" << st.defense
          << "\" special_attack=\"" << st.spatk << "\" special_defense=\"" << st.spdef
          << "\" speed=\"" << st.speed << "\" pow=\"2\" catch_rate=\"" << catchRate
          << "\" ratio_gender=\"" << ratio << "\" egg_step=\"5000\" xp_max=\"1000000\""
          << " give_xp=\"" << giveXp << "\" give_sp=\"" << giveSp << "\"";
        if(m.heightCm > 0)
            o << " height=\"" << (m.heightCm / 100.0) << "m\"";
        if(m.weightKg > 0)
            o << " weight=\"" << m.weightKg << "kg\"";
        o << ">\n";

        // attack_list.  The engine keys a learned move on (skill, attack_level);
        // since every generated skill has a single level (attack_level 1), a
        // monster learns each skill at most once.  Tuxemon movesets are level-
        // ordered, so keeping the first occurrence keeps the earliest level.
        o << "        <attack_list>\n";
        bool anyAttack = false;
        std::vector<int> seenAttacks;
        std::size_t a = 0;
        while(a < m.moveset.size())
        {
            const int sid = skillId(m.moveset[a].technique);
            if(sid >= 0)
            {
                bool dup = false;
                std::size_t q = 0;
                while(q < seenAttacks.size()) { if(seenAttacks[q] == sid) dup = true; ++q; }
                if(!dup)
                {
                    seenAttacks.push_back(sid);
                    o << "            <attack id=\"" << sid << "\" level=\"" << m.moveset[a].level << "\"/>\n";
                    anyAttack = true;
                }
            }
            ++a;
        }
        if(!anyAttack)
            o << "            <attack id=\"0\" level=\"0\"/>\n";
        o << "        </attack_list>\n";

        // evolutions (engine allows a single level evolution)
        std::size_t ev = 0;
        bool wroteEvo = false;
        while(ev < m.evolutions.size() && !wroteEvo)
        {
            const Evolution &e = m.evolutions[ev];
            const int to = monsterId(e.toSlug);
            if(to >= 0 && e.atLevel > 0)
            {
                o << "        <evolutions>\n";
                o << "            <evolution type=\"level\" level=\"" << e.atLevel
                  << "\" evolveTo=\"" << to << "\"/>\n";
                o << "        </evolutions>\n";
                wroteEvo = true;
            }
            ++ev;
        }

        writeNameDesc(o, "        ", m.slug, true);
        // Full raw Tuxemon record (engine-ignored), preserved for later support.
        writeTuxemon(o, "        ", m.raw);
        o << "    </monster>\n";
    }
    o << "</monsters>\n";
}

// ── items/items.xml ──────────────────────────────────────────────────────────
void DatapackWriter::writeItems()
{
    const std::string itemsDir = outRoot_ + "/items";
    const std::string imgDir = itemsDir + "/tuxemon";
    QDir().mkpath(QString::fromStdString(imgDir));

    // stable order by id
    std::vector<const Item*> list;
    std::size_t i = 0;
    while(i < db_.items().size()) { list.push_back(&db_.items()[i]); ++i; }
    std::sort(list.begin(), list.end(), itemBySlugLess);

    std::ofstream o(itemsDir + "/items.xml");
    o << "<!-- Generated from Tuxemon db/item -->\n";
    o << "<items>\n";

    i = 0;
    while(i < list.size())
    {
        const Item &it = *list[i];
        ++i;
        const int id = itemToId_[it.slug];
        const long price = it.cost >= 0 ? it.cost : 0;

        // copy the icon, if present
        std::string imageAttr;
        if(!it.sprite.empty())
        {
            const QString src = QString::fromStdString(modRoot_ + "/" + it.sprite);
            const QString name = QFileInfo(src).fileName();
            const QString dst = QString::fromStdString(imgDir) + "/" + name;
            if(QFile::exists(src))
            {
                QFile::remove(dst);
                if(QFile::copy(src, dst))
                    imageAttr = " image=\"tuxemon/" + name.toStdString() + "\"";
            }
        }

        // decide the (single) effect element
        bool capture = (it.category == "capture");
        bool heal = false;
        std::string healAmount;
        bool removeStatus = false;
        std::size_t k = 0;
        while(k < it.effects.size())
        {
            const std::string &ty = it.effects[k].type;
            if(ty == "capture") capture = true;
            else if(ty == "heal")
            {
                heal = true;
                if(!it.effects[k].params.empty())
                    healAmount = it.effects[k].params[0];
            }
            else if(ty.rfind("remove", 0) == 0 || ty == "cure")
                removeStatus = true;
            ++k;
        }

        o << "    <item price=\"" << price << "\" id=\"" << id << "\"" << imageAttr;
        if(!it.consumable)
            o << " consumeAtUse=\"false\"";
        o << ">\n";
        writeNameDesc(o, "        ", it.slug, true);

        if(capture)
        {
            double rate = 0.2 + (it.cost > 0 ? it.cost / 500.0 : 0.3);
            if(rate < 0.2) rate = 0.2; if(rate > 3.0) rate = 3.0;
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", rate);
            o << "        <trap bonus_rate=\"" << buf << "\"/>\n";
        }
        else if(heal)
        {
            // numeric "fixed" amount -> add="N"; anything else -> full heal
            bool numeric = !healAmount.empty();
            std::size_t c = 0;
            while(c < healAmount.size()) { if(!std::isdigit((unsigned char)healAmount[c])) numeric = false; ++c; }
            if(numeric)
                o << "        <hp add=\"" << healAmount << "\"/>\n";
            else
                o << "        <hp add=\"all\"/>\n";
        }
        else if(removeStatus)
            o << "        <buff remove=\"all\"/>\n";
        // else: a plain item (stone, key item, berry) with no effect element.

        writeTuxemon(o, "        ", it.raw);
        o << "    </item>\n";
    }
    o << "</items>\n";
}

// ── sprites ──────────────────────────────────────────────────────────────────
void DatapackWriter::writeSprites()
{
    const std::string battleDir = modRoot_ + "/gfx/sprites/battle";
    int ok = 0, miss = 0;
    std::size_t i = 0;
    while(i < db_.monsters().size())
    {
        const Monster &m = db_.monsters()[i];
        ++i;
        const std::string sheet = battleDir + "/" + m.slug + "-sheet.png";
        const std::string outDir = outRoot_ + "/monsters/" + std::to_string(m.txmnId);
        if(SpriteExtractor::extract(sheet, outDir))
            ++ok;
        else
            ++miss;
    }
    std::cerr << "Sprites: " << ok << " extracted, " << miss << " missing." << std::endl;
}

// ── orchestration ────────────────────────────────────────────────────────────
bool DatapackWriter::writeAll()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/monsters"));
    QDir().mkpath(QString::fromStdString(outRoot_ + "/items"));

    buildIdMaps();
    writeInformations();
    writeTypes();
    writeBuffs();
    writeSkills();
    writeMonsters();
    writeItems();
    writeSprites();

    std::cerr << "Datapack written to " << outRoot_ << std::endl;
    return true;
}

} // namespace tuxemon
