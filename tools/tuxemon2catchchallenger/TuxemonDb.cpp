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
Evolution::Evolution() : atLevel(0) {}
Monster::Monster() : txmnId(0), heightCm(0), weightKg(0), catchRate(100.0), genderMale(0.5), genderFemale(0.5) {}
Technique::Technique() : techId(0), power(1.0), accuracy(1.0), potency(0.0), targetSelf(false) {}
Item::Item() : cost(-1), consumable(true) {}
Status::Status() : condId(0), persists(false), stepValue(0.0), stepInterval(0) {}
TuxemonDb::TuxemonDb() {}

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
        if(m.slug.empty() || m.txmnId <= 0)
            continue;
        if(!seenSlug.insert(m.slug).second || !seenId.insert(m.txmnId).second)
        {
            std::cerr << "Skipping duplicate monster slug/id: " << m.slug << " (" << m.txmnId << ")" << std::endl;
            continue;
        }
        m.shape = nodeStr(n["shape"], "");
        m.stage = nodeStr(n["stage"], "basic");
        m.heightCm = nodeInt(n["height"], 0);
        m.weightKg = nodeInt(n["weight"], 0);
        m.catchRate = nodeDouble(n["catch_rate"], 100.0);

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
                if(!ev.toSlug.empty())
                    m.evolutions.push_back(ev);
                ++i;
            }
        }

        monsters_.push_back(m);
    }
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

        it.effects = readEffects(n["effects"]);
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
        s.stepType = nodeStr(n["step_effect_type"], "");
        s.stepValue = nodeDouble(n["step_effect_value"], 0.0);
        s.stepInterval = nodeInt(n["step_interval"], 0);

        const YAML::Node beh = n["behaviors"];
        if(beh && beh["persists_after_combat"])
            s.persists = (nodeStr(beh["persists_after_combat"], "false") == "true");

        s.effects = readEffects(n["effects"]);
        statuses_.push_back(s);
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

    std::cerr << "Loaded: " << elements_.size() << " elements, "
              << statuses_.size() << " statuses, "
              << techniques_.size() << " techniques, "
              << items_.size() << " items, "
              << monsters_.size() << " monsters." << std::endl;

    return !monsters_.empty() && !techniques_.empty() && !elements_.empty();
}

} // namespace tuxemon
