#include "TuxemonDb.hpp"

#include <yaml-cpp/yaml.h>

#include <QDir>
#include <QFileInfoList>

#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace tuxemon {

// ── struct constructors (init in .cpp per project style) ──────────────────
Shape::Shape() : hp(5), melee(5), ranged(5), armour(5), dodge(5), speed(5) {}
MoveEntry::MoveEntry() : level(0) {}
EvolutionItem::EvolutionItem() : prob(0.0) {}
Evolution::Evolution() : atLevel(0) {}
StatModifier::StatModifier() : value(1.0) {}
Monster::Monster() : txmnId(0), heightCm(0), weightKg(0), catchRate(100.0),
    lowerCatchResistance(1.0), upperCatchResistance(1.0), genderMale(0.5), genderFemale(0.5) {}
Technique::Technique() : techId(0), power(1.0), accuracy(1.0), potency(0.0), recharge(0), targetSelf(false) {}
Item::Item() : cost(-1), consumable(true), throwable(false), resellable(false) {}
Status::Status() : condId(0), persists(false), stepValue(0.0), stepInterval(0) {}
EncounterMonster::EncounterMonster() : minLevel(1), maxLevel(1), rate(1.0), daytime(-1) {}
TuxemonDb::TuxemonDb() {}

const Encounter *TuxemonDb::encounter(const std::string &slug) const
{
    std::map<std::string,Encounter>::const_iterator it = encounters_.find(slug);
    return it == encounters_.end() ? NULL : &it->second;
}
const Npc *TuxemonDb::npc(const std::string &slug) const
{
    std::map<std::string,Npc>::const_iterator it = npcs_.find(slug);
    return it == npcs_.end() ? NULL : &it->second;
}
const Economy *TuxemonDb::economy(const std::string &slug) const
{
    std::map<std::string,Economy>::const_iterator it = economies_.find(slug);
    return it == economies_.end() ? NULL : &it->second;
}

// ── small yaml helpers (avoid throwing on missing/odd keys) ───────────────
static std::string nodeStr(const YAML::Node &n, const std::string &def)
{
    if(n && n.IsScalar())
    {
        const std::string &s = n.Scalar();
        if(s == "null" || s == "~")
            return def;
        return s;
    }
    return def;
}

static double nodeDouble(const YAML::Node &n, double def)
{
    if(n && n.IsScalar())
    {
        try { return n.as<double>(); } catch(...) { return def; }
    }
    return def;
}

static int nodeInt(const YAML::Node &n, int def)
{
    if(n && n.IsScalar())
    {
        try { return n.as<int>(); } catch(...) { return def; }
    }
    return def;
}

// Read a YAML sequence of scalars into a string vector.
static void readStrList(const YAML::Node &node, std::vector<std::string> &out)
{
    if(node && node.IsSequence())
    {
        std::size_t i = 0;
        while(i < node.size())
        {
            const std::string s = nodeStr(node[i], "");
            if(!s.empty())
                out.push_back(s);
            ++i;
        }
    }
}

// Read a {type, parameters:[...]} list (techniques, items, statuses share it).
static std::vector<Effect> readEffects(const YAML::Node &node)
{
    std::vector<Effect> out;
    if(node && node.IsSequence())
    {
        std::size_t i = 0;
        while(i < node.size())
        {
            const YAML::Node e = node[i];
            Effect eff;
            eff.type = nodeStr(e["type"], "");
            const YAML::Node params = e["parameters"];
            if(params && params.IsSequence())
            {
                std::size_t j = 0;
                while(j < params.size())
                {
                    eff.params.push_back(nodeStr(params[j], ""));
                    ++j;
                }
            }
            if(!eff.type.empty())
                out.push_back(eff);
            ++i;
        }
    }
    return out;
}

// List every *.yaml in dir, sorted, returns absolute paths.
static QFileInfoList yamlFiles(const std::string &dir)
{
    QDir d(QString::fromStdString(dir));
    QStringList filters;
    filters << "*.yaml" << "*.yml";
    return d.entryInfoList(filters, QDir::Files, QDir::Name);
}

// ── shapes ────────────────────────────────────────────────────────────────
bool TuxemonDb::loadShapes(const std::string &file)
{
    YAML::Node root;
    try { root = YAML::LoadFile(file); }
    catch(const std::exception &e)
    {
        std::cerr << "Cannot load shapes " << file << ": " << e.what() << std::endl;
        return false;
    }
    if(!root.IsSequence())
        return false;
    std::size_t i = 0;
    while(i < root.size())
    {
        const YAML::Node n = root[i];
        const std::string slug = nodeStr(n["slug"], "");
        const YAML::Node a = n["attributes"];
        if(!slug.empty() && a)
        {
            Shape s;
            s.hp     = nodeInt(a["hp"], 5);
            s.melee  = nodeInt(a["melee"], 5);
            s.ranged = nodeInt(a["ranged"], 5);
            s.armour = nodeInt(a["armour"], 5);
            s.dodge  = nodeInt(a["dodge"], 5);
            s.speed  = nodeInt(a["speed"], 5);
            shapes_[slug] = s;
        }
        ++i;
    }
    return !shapes_.empty();
}

// ── monsters ────────────────────────────────────────────────────────────────
void TuxemonDb::loadMonsters(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    std::unordered_set<std::string> seenSlug;
    std::unordered_set<int> seenId;
    int maxId = 0;
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }

        Monster m;
        m.slug = nodeStr(n["slug"], "");
        m.txmnId = nodeInt(n["txmn_id"], 0);
        if(m.slug.empty())
            continue;
        if(!seenSlug.insert(m.slug).second)
        {
            std::cerr << "Skipping duplicate monster slug: " << m.slug << " (" << m.txmnId << ")" << std::endl;
            continue;
        }
        // txmn_id<=0 (the glitch set, 18 monsters) is kept: a free unique id is
        // allocated after the load loop so their encounter refs resolve.  The
        // engine only needs a unique u16 id.
        if(m.txmnId > 0)
        {
            if(!seenId.insert(m.txmnId).second)
            {
                std::cerr << "Skipping duplicate monster id: " << m.slug << " (" << m.txmnId << ")" << std::endl;
                continue;
            }
            if(m.txmnId > maxId)
                maxId = m.txmnId;
        }
        m.shape = nodeStr(n["shape"], "");
        m.stage = nodeStr(n["stage"], "basic");
        m.species = nodeStr(n["species"], "");
        m.heightCm = nodeInt(n["height"], 0);
        m.weightKg = nodeInt(n["weight"], 0);
        m.catchRate = nodeDouble(n["catch_rate"], 100.0);
        m.lowerCatchResistance = nodeDouble(n["lower_catch_resistance"], 1.0);
        m.upperCatchResistance = nodeDouble(n["upper_catch_resistance"], 1.0);
        readStrList(n["tags"], m.tags);
        readStrList(n["terrains"], m.terrains);

        const YAML::Node types = n["types"];
        if(types && types.IsSequence())
        {
            std::size_t i = 0;
            while(i < types.size())
            {
                const std::string t = nodeStr(types[i], "");
                if(!t.empty())
                    m.types.push_back(t);
                ++i;
            }
        }

        const YAML::Node gw = n["gender_weights"];
        if(gw)
        {
            m.genderMale = nodeDouble(gw["male"], 0.5);
            m.genderFemale = nodeDouble(gw["female"], 0.5);
        }

        const YAML::Node ms = n["moveset"];
        if(ms && ms.IsSequence())
        {
            std::size_t i = 0;
            while(i < ms.size())
            {
                const YAML::Node e = ms[i];
                MoveEntry mv;
                mv.level = nodeInt(e["level_learned"], 0);
                mv.technique = nodeStr(e["technique"], "");
                if(!mv.technique.empty())
                    m.moveset.push_back(mv);
                ++i;
            }
        }

        const YAML::Node evos = n["evolutions"];
        if(evos && evos.IsSequence())
        {
            std::size_t i = 0;
            while(i < evos.size())
            {
                const YAML::Node e = evos[i];
                Evolution ev;
                ev.atLevel = nodeInt(e["at_level"], 0);
                ev.toSlug = nodeStr(e["monster_slug"], "");
                // item-triggered evolution: `item:` is a {itemSlug: prob} map
                const YAML::Node itm = e["item"];
                if(itm && itm.IsMap())
                {
                    YAML::const_iterator iit = itm.begin();
                    while(iit != itm.end())
                    {
                        EvolutionItem ei;
                        ei.slug = nodeStr(iit->first, "");
                        ei.prob = nodeDouble(iit->second, 0.0);
                        if(!ei.slug.empty())
                            ev.items.push_back(ei);
                        ++iit;
                    }
                }
                if(!ev.toSlug.empty())
                    m.evolutions.push_back(ev);
                ++i;
            }
        }

        m.raw = n;
        monsters_.push_back(m);
    }

    // Allocate free unique ids above the current maximum for the monsters
    // shipped without a txmn_id (they were dropped silently before, leaving
    // the encounter/37707_town* refs dangling).
    int nextId = maxId;
    int assigned = 0;
    std::size_t mi = 0;
    while(mi < monsters_.size())
    {
        if(monsters_[mi].txmnId <= 0)
        {
            ++nextId;
            while(seenId.find(nextId) != seenId.end())
                ++nextId;
            monsters_[mi].txmnId = nextId;
            seenId.insert(nextId);
            ++assigned;
        }
        ++mi;
    }
    if(assigned > 0)
        std::cerr << "Assigned " << assigned << " free monster ids (missing txmn_id), up to " << nextId << std::endl;
}

// ── techniques ──────────────────────────────────────────────────────────────
void TuxemonDb::loadTechniques(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    std::unordered_set<std::string> seenSlug;
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }

        Technique t;
        t.slug = nodeStr(n["slug"], "");
        if(t.slug.empty() || !seenSlug.insert(t.slug).second)
            continue;
        t.techId = nodeInt(n["tech_id"], 0);
        t.power = nodeDouble(n["power"], 1.0);
        t.accuracy = nodeDouble(n["accuracy"], 1.0);
        t.potency = nodeDouble(n["potency"], 0.0);
        t.recharge = nodeInt(n["recharge"], 0);
        t.range = nodeStr(n["range"], "");
        t.sort = nodeStr(n["sort"], "");
        t.category = nodeStr(n["category"], "");

        const YAML::Node types = n["types"];
        if(types && types.IsSequence())
        {
            std::size_t i = 0;
            while(i < types.size())
            {
                const std::string e = nodeStr(types[i], "");
                if(!e.empty())
                    t.types.push_back(e);
                ++i;
            }
        }

        const YAML::Node target = n["target"];
        if(target)
            t.targetSelf = (nodeStr(target["own_monster"], "false") == "true");

        t.effects = readEffects(n["effects"]);
        t.raw = n;
        techniques_.push_back(t);
    }
}

// ── items ───────────────────────────────────────────────────────────────────
void TuxemonDb::loadItems(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    std::unordered_set<std::string> seenSlug;
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }

        Item it;
        it.slug = nodeStr(n["slug"], "");
        if(it.slug.empty() || !seenSlug.insert(it.slug).second)
            continue;
        it.category = nodeStr(n["category"], "");
        it.sort = nodeStr(n["sort"], "");
        it.sprite = nodeStr(n["sprite"], "");
        if(n["cost"] && n["cost"].IsScalar())
            it.cost = nodeInt(n["cost"], -1);

        const YAML::Node beh = n["behaviors"];
        if(beh && beh["consumable"])
            it.consumable = (nodeStr(beh["consumable"], "false") == "true");
        if(beh && beh["throwable"])
            it.throwable = (nodeStr(beh["throwable"], "false") == "true");
        if(beh && beh["resellable"])
            it.resellable = (nodeStr(beh["resellable"], "false") == "true");

        it.effects = readEffects(n["effects"]);
        it.raw = n;
        items_.push_back(it);
    }
}

// ── elements (type chart) ───────────────────────────────────────────────────
void TuxemonDb::loadElements(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }

        Element el;
        el.slug = nodeStr(n["slug"], "");
        if(el.slug.empty())
            continue;
        bool dupElem = false;
        std::size_t ei = 0;
        while(ei < elements_.size()) { if(elements_[ei].slug == el.slug) dupElem = true; ++ei; }
        if(dupElem)
            continue;
        const YAML::Node types = n["types"];
        if(types && types.IsSequence())
        {
            std::size_t i = 0;
            while(i < types.size())
            {
                const YAML::Node e = types[i];
                Matchup mu;
                mu.against = nodeStr(e["against"], "");
                mu.mult = nodeDouble(e["multiplier"], 1.0);
                if(!mu.against.empty())
                    el.matchups.push_back(mu);
                ++i;
            }
        }
        el.raw = n;
        elements_.push_back(el);
    }
}

// ── statuses (buffs) ────────────────────────────────────────────────────────
void TuxemonDb::loadStatuses(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    std::unordered_set<std::string> seenSlug;
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }

        Status s;
        s.slug = nodeStr(n["slug"], "");
        if(s.slug.empty() || !seenSlug.insert(s.slug).second)
            continue;
        s.condId = nodeInt(n["cond_id"], 0);
        s.category = nodeStr(n["category"], "neutral");
        s.sort = nodeStr(n["sort"], "");
        s.stepType = nodeStr(n["step_effect_type"], "");
        s.stepValue = nodeDouble(n["step_effect_value"], 0.0);
        s.stepInterval = nodeInt(n["step_interval"], 0);

        const YAML::Node beh = n["behaviors"];
        if(beh && beh["persists_after_combat"])
            s.persists = (nodeStr(beh["persists_after_combat"], "false") == "true");

        // stat_modifiers: {stat: {value: N, operation: '*'}} (blinded, hardshell, …)
        const YAML::Node sm = n["stat_modifiers"];
        if(sm && sm.IsMap())
        {
            YAML::const_iterator smit = sm.begin();
            while(smit != sm.end())
            {
                StatModifier mod;
                mod.stat = nodeStr(smit->first, "");
                if(smit->second && smit->second.IsMap())
                {
                    mod.op = nodeStr(smit->second["operation"], "*");
                    mod.value = nodeDouble(smit->second["value"], 1.0);
                }
                if(!mod.stat.empty())
                    s.statModifiers.push_back(mod);
                ++smit;
            }
        }

        s.effects = readEffects(n["effects"]);
        s.raw = n;
        statuses_.push_back(s);
    }
}

// ── encounters ──────────────────────────────────────────────────────────────
void TuxemonDb::loadEncounters(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }
        Encounter enc;
        enc.slug = nodeStr(n["slug"], "");
        if(enc.slug.empty())
            continue;
        const YAML::Node ms = n["monsters"];
        if(ms && ms.IsSequence())
        {
            std::size_t i = 0;
            while(i < ms.size())
            {
                const YAML::Node e = ms[i];
                EncounterMonster em;
                em.slug = nodeStr(e["monster"], "");
                const YAML::Node lr = e["level_range"];
                if(lr && lr.IsSequence() && lr.size() >= 2)
                {
                    em.minLevel = nodeInt(lr[0], 1);
                    em.maxLevel = nodeInt(lr[1], em.minLevel);
                }
                em.rate = nodeDouble(e["encounter_rate"], 1.0);
                const YAML::Node vars = e["variables"];
                if(vars && vars.IsSequence())
                {
                    std::size_t j = 0;
                    while(j < vars.size())
                    {
                        if(nodeStr(vars[j]["key"], "") == "daytime")
                            em.daytime = (nodeStr(vars[j]["value"], "") == "true") ? 1 : 0;
                        ++j;
                    }
                }
                if(!em.slug.empty())
                    enc.monsters.push_back(em);
                ++i;
            }
        }
        encounters_[enc.slug] = enc;
    }
}

// ── npcs ────────────────────────────────────────────────────────────────────
void TuxemonDb::loadNpcs(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }
        // an npc file may be a single mapping or a sequence of npc entries
        std::vector<YAML::Node> entries;
        if(n.IsSequence())
        {
            std::size_t i = 0;
            while(i < n.size()) { entries.push_back(n[i]); ++i; }
        }
        else
            entries.push_back(n);
        std::size_t k = 0;
        while(k < entries.size())
        {
            const YAML::Node e = entries[k];
            ++k;
            Npc np;
            np.slug = nodeStr(e["slug"], "");
            if(np.slug.empty())
                continue;
            const YAML::Node tpl = e["template"];
            if(tpl)
            {
                np.spriteName = nodeStr(tpl["sprite_name"], "");
                np.combatSheet = nodeStr(tpl["combat_sheet"], "");
            }
            if(np.spriteName.empty())
                np.spriteName = np.slug;
            npcs_[np.slug] = np;
        }
    }
}

// ── economy (shops) ──────────────────────────────────────────────────────────
void TuxemonDb::loadEconomy(const std::string &dir)
{
    const QFileInfoList files = yamlFiles(dir);
    int idx = 0;
    while(idx < files.size())
    {
        const std::string path = files.at(idx).absoluteFilePath().toStdString();
        ++idx;
        YAML::Node n;
        try { n = YAML::LoadFile(path); } catch(...) { continue; }
        std::vector<YAML::Node> entries;
        if(n.IsSequence()) { std::size_t i = 0; while(i < n.size()) { entries.push_back(n[i]); ++i; } }
        else entries.push_back(n);
        std::size_t k = 0;
        while(k < entries.size())
        {
            const YAML::Node e = entries[k];
            ++k;
            Economy ec;
            ec.slug = nodeStr(e["slug"], "");
            if(ec.slug.empty())
                continue;
            const YAML::Node items = e["items"];
            if(items && items.IsSequence())
            {
                std::size_t i = 0;
                while(i < items.size())
                {
                    EconomyItem ei;
                    ei.slug = nodeStr(items[i]["slug"], "");
                    ei.price = nodeInt(items[i]["price"], 0);
                    if(!ei.slug.empty())
                        ec.items.push_back(ei);
                    ++i;
                }
            }
            economies_[ec.slug] = ec;
        }
    }
}

// ── orchestration ───────────────────────────────────────────────────────────
bool TuxemonDb::load(const std::string &modRoot)
{
    const std::string db = modRoot + "/db";
    if(!loadShapes(db + "/shape/shapes.yaml"))
        std::cerr << "Warning: no shapes loaded (stats will use the default shape)." << std::endl;

    loadElements(db + "/element");
    loadStatuses(db + "/status");
    loadTechniques(db + "/technique");
    loadItems(db + "/item");
    loadMonsters(db + "/monster");
    loadEncounters(db + "/encounter");
    loadNpcs(db + "/npc");
    loadEconomy(db + "/economy");

    std::cerr << "Loaded: " << elements_.size() << " elements, "
              << statuses_.size() << " statuses, "
              << techniques_.size() << " techniques, "
              << items_.size() << " items, "
              << monsters_.size() << " monsters, "
              << encounters_.size() << " encounters, "
              << npcs_.size() << " npcs, "
              << economies_.size() << " economies." << std::endl;

    return !monsters_.empty() && !techniques_.empty() && !elements_.empty();
}

} // namespace tuxemon
