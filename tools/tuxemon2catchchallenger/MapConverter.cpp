#include "MapConverter.hpp"
#include "InvisibleAsset.hpp"

#include <yaml-cpp/yaml.h>

#include <tinyxml2.h>
#include <zlib.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <vector>

namespace tuxemon {

MapConverter::MapConverter(const std::string &modRoot, const std::string &outRoot,
                           const TuxemonDb &db, const DatapackWriter &dw,
                           const Localization &l10n, SkinGen &skins)
    : db_(db), dw_(dw), l10n_(l10n), skins_(skins), modRoot_(modRoot), outRoot_(outRoot),
      warpsTotal_(0), collisionCells_(0), botsTotal_(0), encounterZones_(0),
      startX_(0), startY_(0), startScore_(-1)
{
    mapDir_ = outRoot_ + "/map/main/tuxemon";
    tilesetDir_ = mapDir_ + "/tileset";
}

// ── encode/decode helpers ───────────────────────────────────────────────────
static const uint32_t kGidMask = 0x1FFFFFFFu; // strip the 3 Tiled flip flags

static bool zlibInflate(const QByteArray &in, int expectedBytes, QByteArray &out)
{
    out.resize(expectedBytes);
    uLongf destLen = (uLongf)expectedBytes;
    const int r = uncompress(reinterpret_cast<Bytef*>(out.data()), &destLen,
                             reinterpret_cast<const Bytef*>(in.constData()), (uLong)in.size());
    if(r != Z_OK)
        return false;
    out.resize((int)destLen);
    return true;
}

static QByteArray zlibDeflate(const QByteArray &in)
{
    uLongf bound = compressBound((uLong)in.size());
    QByteArray out;
    out.resize((int)bound);
    if(compress2(reinterpret_cast<Bytef*>(out.data()), &bound,
                 reinterpret_cast<const Bytef*>(in.constData()), (uLong)in.size(), 9) != Z_OK)
        return QByteArray();
    out.resize((int)bound);
    return out;
}

// Decode a Tiled <data> body (csv OR base64+zlib) to a w*h gid grid.
static bool decodeLayer(const char *encoding, const char *compression, const char *text,
                        int w, int h, std::vector<uint32_t> &gids)
{
    gids.assign((std::size_t)w * h, 0);
    if(text == NULL)
        return false;

    if(encoding != NULL && std::strcmp(encoding, "csv") == 0)
    {
        std::size_t idx = 0;
        const char *p = text;
        while(*p && idx < gids.size())
        {
            while(*p && (*p < '0' || *p > '9'))
                ++p;
            if(!*p)
                break;
            uint64_t v = 0;
            while(*p >= '0' && *p <= '9') { v = v * 10 + (uint64_t)(*p - '0'); ++p; }
            gids[idx++] = (uint32_t)v;
        }
        return true;
    }

    if(encoding != NULL && std::strcmp(encoding, "base64") == 0)
    {
        QByteArray b64 = QByteArray(text);
        const QByteArray packed = QByteArray::fromBase64(b64.simplified().replace(" ", ""));
        QByteArray raw;
        if(compression != NULL && std::strcmp(compression, "zlib") == 0)
        {
            if(!zlibInflate(packed, w * h * 4, raw))
                return false;
        }
        else if(compression == NULL || compression[0] == '\0')
            raw = packed; // uncompressed base64
        else
            return false; // gzip/zstd not used by the Tuxemon maps
        if(raw.size() < w * h * 4)
            return false;
        std::size_t i = 0;
        while(i < gids.size())
        {
            const unsigned char *b = reinterpret_cast<const unsigned char*>(raw.constData()) + i * 4;
            gids[i] = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
            ++i;
        }
        return true;
    }
    return false;
}

// Encode a gid grid as base64+zlib (the datapack convention).
static std::string encodeLayer(const std::vector<uint32_t> &gids)
{
    QByteArray raw;
    raw.resize((int)(gids.size() * 4));
    std::size_t i = 0;
    while(i < gids.size())
    {
        unsigned char *b = reinterpret_cast<unsigned char*>(raw.data()) + i * 4;
        const uint32_t g = gids[i];
        b[0] = (unsigned char)(g & 0xFF);
        b[1] = (unsigned char)((g >> 8) & 0xFF);
        b[2] = (unsigned char)((g >> 16) & 0xFF);
        b[3] = (unsigned char)((g >> 24) & 0xFF);
        ++i;
    }
    return zlibDeflate(raw).toBase64().toStdString();
}

static bool anyTile(const std::vector<uint32_t> &g)
{
    std::size_t i = 0;
    while(i < g.size()) { if((g[i] & kGidMask) != 0) return true; ++i; }
    return false;
}

// ── tileset filename sanitisation ────────────────────────────────────────────
// Datapack files are synced/checksummed only when their path is [a-z0-9._/-]
// (FacilityLibGeneral::getSuffixAndValidatePathFromFS).  Tuxemon tileset files
// have uppercase/spaces/punctuation, so rename them to a valid form.
static std::string sanitizeFsBase(const std::string &name)
{
    const std::size_t dot = name.rfind('.');
    const std::string base = dot == std::string::npos ? name : name.substr(0, dot);
    const std::string ext  = dot == std::string::npos ? std::string() : name.substr(dot + 1);
    std::string out;
    std::size_t i = 0;
    while(i < base.size())
    {
        char c = base[i];
        if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-')
            out.push_back(c);
        else
            out.push_back('_');
        ++i;
    }
    if(out.empty()) out = "t";
    std::string e;
    i = 0;
    while(i < ext.size()) { char c = ext[i]; if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a'); if(c >= 'a' && c <= 'z') e.push_back(c); ++i; }
    return e.empty() ? out : out + "." + e;
}

std::string MapConverter::uniqueTilesetFile(const std::string &origBasename)
{
    std::string san = sanitizeFsBase(origBasename);
    if(usedTilesetFiles_.find(san) == usedTilesetFiles_.end())
    {
        usedTilesetFiles_.insert(san);
        return san;
    }
    const std::size_t dot = san.rfind('.');
    const std::string base = dot == std::string::npos ? san : san.substr(0, dot);
    const std::string ext  = dot == std::string::npos ? std::string() : san.substr(dot);
    int n = 2;
    while(true)
    {
        const std::string cand = base + "-" + std::to_string(n) + ext;
        if(usedTilesetFiles_.find(cand) == usedTilesetFiles_.end())
        {
            usedTilesetFiles_.insert(cand);
            return cand;
        }
        ++n;
    }
}

// Copy one referenced image (resolved against baseDir) under a sanitised name,
// deduped by original basename; returns the written name.
static std::string copyImageSan(const std::string &baseDir, const std::string &srcRel,
                                const std::string &tilesetDir,
                                std::unordered_map<std::string,std::string> &imgSan,
                                MapConverter *self,
                                std::string (MapConverter::*uniq)(const std::string&))
{
    const std::string orig = QFileInfo(QString::fromUtf8(srcRel.c_str())).fileName().toStdString();
    std::unordered_map<std::string,std::string>::const_iterator it = imgSan.find(orig);
    if(it != imgSan.end())
        return it->second;
    const std::string san = (self->*uniq)(orig);
    imgSan[orig] = san;
    const QString from = QString::fromStdString(baseDir) + "/" + QString::fromUtf8(srcRel.c_str());
    const QString to = QString::fromStdString(tilesetDir) + "/" + QString::fromStdString(san);
    QFile::remove(to);
    QFile::copy(QFileInfo(from).absoluteFilePath(), to);
    return san;
}

// ── tileset copy ────────────────────────────────────────────────────────────
std::string MapConverter::copyTileset(const std::string &tsxBasename)
{
    std::unordered_map<std::string,std::string>::const_iterator done = tsxSan_.find(tsxBasename);
    if(done != tsxSan_.end())
        return done->second;

    const std::string srcTsx = modRoot_ + "/gfx/tilesets/" + tsxBasename;
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(srcTsx.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "  warning: cannot read tileset " << srcTsx << std::endl;
        tsxSan_[tsxBasename] = std::string();
        return std::string();
    }
    tinyxml2::XMLElement *ts = doc.RootElement();
    if(ts == NULL)
    {
        tsxSan_[tsxBasename] = std::string();
        return std::string();
    }

    QDir().mkpath(QString::fromStdString(tilesetDir_));
    const std::string sanTsx = uniqueTilesetFile(tsxBasename);
    tsxSan_[tsxBasename] = sanTsx;
    const std::string imgBaseDir = modRoot_ + "/gfx/tilesets";

    // tilecount, so the invisible marker tileset can be placed after the last gid
    {
        int count = ts->IntAttribute("tilecount");
        if(count <= 0)
        {
            tinyxml2::XMLElement *cimg = ts->FirstChildElement("image");
            const int tw = ts->IntAttribute("tilewidth", 16);
            const int th = ts->IntAttribute("tileheight", 16);
            if(cimg != NULL && tw > 0 && th > 0)
                count = (cimg->IntAttribute("width") / tw) * (cimg->IntAttribute("height") / th);
        }
        if(count <= 0)
        {
            tinyxml2::XMLElement *ct = ts->FirstChildElement("tile");
            while(ct != NULL) { ++count; ct = ct->NextSiblingElement("tile"); }
        }
        tsxCount_[sanTsx] = count;
    }

    // Copy every referenced image (single <image> and per-tile <image>) under a
    // sanitised name and point the source at it.
    tinyxml2::XMLElement *img = ts->FirstChildElement("image");
    while(img != NULL)
    {
        const char *src = img->Attribute("source");
        if(src != NULL)
        {
            const std::string san = copyImageSan(imgBaseDir, src, tilesetDir_, imgSan_, this, &MapConverter::uniqueTilesetFile);
            img->SetAttribute("source", san.c_str());
        }
        img = img->NextSiblingElement("image");
    }
    tinyxml2::XMLElement *tile = ts->FirstChildElement("tile");
    while(tile != NULL)
    {
        tinyxml2::XMLElement *ti = tile->FirstChildElement("image");
        while(ti != NULL)
        {
            const char *src = ti->Attribute("source");
            if(src != NULL)
            {
                const std::string san = copyImageSan(imgBaseDir, src, tilesetDir_, imgSan_, this, &MapConverter::uniqueTilesetFile);
                ti->SetAttribute("source", san.c_str());
            }
            ti = ti->NextSiblingElement("image");
        }
        tile = tile->NextSiblingElement("tile");
    }

    doc.SaveFile((tilesetDir_ + "/" + sanTsx).c_str());
    return sanTsx;
}

std::string MapConverter::materializeInline(void *tilesetElement, const std::string &mapDirAbs)
{
    tinyxml2::XMLElement *ts = reinterpret_cast<tinyxml2::XMLElement*>(tilesetElement);
    const char *name = ts->Attribute("name");
    tinyxml2::XMLElement *img = ts->FirstChildElement("image");
    if(name == NULL || img == NULL || img->Attribute("source") == NULL)
        return std::string();

    // dedup by the inline tileset's own (original) name
    const std::string origKey = std::string("inline:") + name;
    std::unordered_map<std::string,std::string>::const_iterator done = tsxSan_.find(origKey);
    if(done != tsxSan_.end())
        return done->second;

    const std::string imgSrc = img->Attribute("source");
    QDir().mkpath(QString::fromStdString(tilesetDir_));
    const std::string sanTsx = uniqueTilesetFile(std::string(name) + ".tsx");
    tsxSan_[origKey] = sanTsx;
    {
        int count = ts->IntAttribute("tilecount");
        if(count <= 0)
        {
            const int tw = ts->IntAttribute("tilewidth", 16);
            const int th = ts->IntAttribute("tileheight", 16);
            if(tw > 0 && th > 0)
                count = (img->IntAttribute("width") / tw) * (img->IntAttribute("height") / th);
        }
        tsxCount_[sanTsx] = count;
    }
    // image source is relative to the source map's directory
    const std::string sanImg = copyImageSan(mapDirAbs, imgSrc, tilesetDir_, imgSan_, this, &MapConverter::uniqueTilesetFile);

    std::ofstream t(tilesetDir_ + "/" + sanTsx);
    t << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    t << "<tileset version=\"1.10\" name=\"" << name << "\" tilewidth=\""
      << ts->IntAttribute("tilewidth", 16) << "\" tileheight=\"" << ts->IntAttribute("tileheight", 16)
      << "\" tilecount=\"" << ts->IntAttribute("tilecount") << "\" columns=\"" << ts->IntAttribute("columns") << "\">\n";
    t << " <image source=\"" << sanImg << "\" width=\"" << img->IntAttribute("width")
      << "\" height=\"" << img->IntAttribute("height") << "\"/>\n";
    t << "</tileset>\n";
    return sanTsx;
}

// Read an object's <properties> into a flat name->value map.
static void readProps(tinyxml2::XMLElement *obj, std::map<std::string,std::string> &out)
{
    tinyxml2::XMLElement *props = obj->FirstChildElement("properties");
    if(props == NULL)
        return;
    tinyxml2::XMLElement *p = props->FirstChildElement("property");
    while(p != NULL)
    {
        const char *n = p->Attribute("name");
        const char *v = p->Attribute("value");
        if(n != NULL && v != NULL)
            out[n] = v;
        p = p->NextSiblingElement("property");
    }
}

// ── output layout (region/location folders, gen2-style) ────────────────────
// One pre-pass record per source map.
struct MapMeta {
    int w, h;
    bool inside;
    std::string scenario;
    std::vector<std::string> warpDests;
    MapMeta() : w(0), h(0), inside(false) {}
};

typedef std::map<std::string, std::set<std::string> > AdjGraph; // sorted = deterministic

// BFS from `start` through the warp graph to the nearest map that is outdoor
// (wantOutdoor) or that carries a scenario (!wantOutdoor); "" when none is
// reachable.  visitedOut (optional) receives the whole connected component.
static std::string bfsNearest(const std::string &start,
                              const std::map<std::string,MapMeta> &meta,
                              const AdjGraph &adj, bool wantOutdoor,
                              std::set<std::string> *visitedOut)
{
    std::set<std::string> seen;
    std::vector<std::string> queue;
    queue.push_back(start);
    seen.insert(start);
    std::string found;
    std::size_t qi = 0;
    while(qi < queue.size())
    {
        const std::string cur = queue[qi];
        ++qi;
        if(found.empty())
        {
            std::map<std::string,MapMeta>::const_iterator m = meta.find(cur);
            if(m != meta.end())
            {
                if(wantOutdoor ? !m->second.inside : !m->second.scenario.empty())
                {
                    found = cur;
                    if(visitedOut == NULL)
                        break; // component not requested, done
                }
            }
        }
        AdjGraph::const_iterator a = adj.find(cur);
        if(a != adj.end())
        {
            std::set<std::string>::const_iterator n = a->second.begin();
            while(n != a->second.end())
            {
                if(seen.find(*n) == seen.end())
                {
                    seen.insert(*n);
                    queue.push_back(*n);
                }
                ++n;
            }
        }
    }
    if(visitedOut != NULL)
        *visitedOut = seen;
    return found;
}

// "_"-token helpers for the gen2-style short dashed names — everything is
// derived from the source data (scenario + anchor slug), nothing hardcoded.
static std::vector<std::string> splitTokens(const std::string &s)
{
    std::vector<std::string> out;
    std::string cur;
    std::size_t i = 0;
    while(i < s.size())
    {
        if(s[i] == '_')
        {
            if(!cur.empty()) { out.push_back(cur); cur.clear(); }
        }
        else
            cur.push_back(s[i]);
        ++i;
    }
    if(!cur.empty())
        out.push_back(cur);
    return out;
}

static std::string joinDash(const std::vector<std::string> &t)
{
    std::string out;
    std::size_t i = 0;
    while(i < t.size())
    {
        if(!out.empty()) out += "-";
        out += t[i];
        ++i;
    }
    return out;
}

static std::size_t commonPrefixLen(const std::vector<std::string> &a,
                                   const std::vector<std::string> &b)
{
    std::size_t n = 0;
    while(n < a.size() && n < b.size() && a[n] == b[n])
        ++n;
    return n;
}

// Reserve `base` (else base-2, base-3, …) within the per-folder name set.
static std::string uniqueInDir(const std::string &dir, const std::string &base,
                               std::map<std::string,std::set<std::string> > &used)
{
    std::set<std::string> &names = used[dir];
    if(names.find(base) == names.end())
    {
        names.insert(base);
        return base;
    }
    int n = 2;
    while(true)
    {
        const std::string cand = base + "-" + std::to_string(n);
        if(names.find(cand) == names.end())
        {
            names.insert(cand);
            return cand;
        }
        ++n;
    }
}

// Path of map `slug` living in `toDir`, relative to a map living in `fromDir`
// (both dirs relative to map/main/tuxemon/, no trailing slash, no extension).
static std::string relMapPath(const std::string &fromDir, const std::string &toDir,
                              const std::string &slug)
{
    if(fromDir == toDir)
        return slug;
    const QStringList f = QString::fromStdString(fromDir).split('/');
    const QStringList t = QString::fromStdString(toDir).split('/');
    int common = 0;
    while(common < f.size() && common < t.size() && f.at(common) == t.at(common))
        ++common;
    std::string out;
    int i = common;
    while(i < f.size()) { out += "../"; ++i; }
    i = common;
    while(i < t.size()) { out += t.at(i).toStdString() + "/"; ++i; }
    return out + slug;
}

// ── per-map conversion ──────────────────────────────────────────────────────
struct Warp { int srcTx, srcTy; std::string dest; int dx, dy; };
struct EncZone { int x0, y0, x1, y1; std::string slug; };
struct NpcPlace {
    std::string slug;
    int x, y;
    std::string facing;
    std::string economy;
    bool shop;
    std::vector<std::pair<std::string,int> > party; // monster slug, level
    std::vector<std::string> dialog;                // l18n msgids
    NpcPlace() : x(0), y(0), shop(false) {}
};

static std::string facingToLookAt(const std::string &f)
{
    if(f == "up")    return "top";
    if(f == "down")  return "bottom";
    if(f == "left")  return "left";
    if(f == "right") return "right";
    return "bottom";
}

static std::string sanitizeName(const std::string &s)
{
    std::string t;
    std::size_t i = 0;
    while(i < s.size())
    {
        const char c = s[i];
        if(std::isalnum((unsigned char)c) || c == '_' || c == '-')
            t.push_back(c);
        ++i;
    }
    if(t.empty()) t = "npc";
    return t;
}

// Escape text for a CDATA block (only "]]>" is illegal inside).
static std::string cdataSafe(std::string s)
{
    std::size_t p;
    while((p = s.find("]]>")) != std::string::npos)
        s.replace(p, 3, "]] >");
    return s;
}

// "create_npc foo,3,4" -> verb="create_npc", args=["foo","3","4"]
static void splitAction(const std::string &value, std::string &verb, std::vector<std::string> &args)
{
    const std::size_t sp = value.find(' ');
    verb = sp == std::string::npos ? value : value.substr(0, sp);
    args.clear();
    if(sp != std::string::npos)
    {
        const QStringList parts = QString::fromStdString(value.substr(sp + 1)).split(',');
        int i = 0;
        while(i < parts.size()) { args.push_back(parts.at(i).trimmed().toStdString()); ++i; }
    }
}

static std::string xmlEsc(const std::string &s)
{
    std::string out;
    std::size_t i = 0;
    while(i < s.size())
    {
        const char c = s[i];
        if(c == '&') out += "&amp;";
        else if(c == '<') out += "&lt;";
        else if(c == '>') out += "&gt;";
        else if(c == '"') out += "&quot;";
        else out.push_back(c);
        ++i;
    }
    return out;
}

struct GrassAgg { int minL, maxL; double weight; };

// Emit one wild-encounter list, normalising luck to sum exactly 100.
static void emitGrassList(std::ofstream &ox, const char *tag, const std::map<int,GrassAgg> &m)
{
    if(m.empty())
        return;
    double sum = 0;
    std::map<int,GrassAgg>::const_iterator it = m.begin();
    while(it != m.end()) { sum += it->second.weight > 0 ? it->second.weight : 1.0; ++it; }
    if(sum <= 0)
        return;
    std::vector<std::pair<int,int> > luck; // id, luck
    int total = 0, maxIdx = 0, maxLuck = -1;
    it = m.begin();
    int k = 0;
    while(it != m.end())
    {
        const double wgt = it->second.weight > 0 ? it->second.weight : 1.0;
        int l = (int)(wgt / sum * 100.0 + 0.5);
        if(l < 1) l = 1;
        luck.push_back(std::make_pair(it->first, l));
        total += l;
        if(l > maxLuck) { maxLuck = l; maxIdx = k; }
        ++k; ++it;
    }
    luck[maxIdx].second += (100 - total);
    if(luck[maxIdx].second < 1) luck[maxIdx].second = 1;
    ox << " <" << tag << ">\n";
    std::size_t j = 0;
    it = m.begin();
    while(j < luck.size())
    {
        std::map<int,GrassAgg>::const_iterator e = m.find(luck[j].first);
        ox << "  <monster id=\"" << luck[j].first << "\" minLevel=\"" << e->second.minL
           << "\" maxLevel=\"" << e->second.maxL << "\" luck=\"" << luck[j].second << "\"/>\n";
        ++j;
    }
    ox << " </" << tag << ">\n";
}

static void aggAdd(std::map<int,GrassAgg> &m, int id, int minL, int maxL, double w)
{
    std::map<int,GrassAgg>::iterator it = m.find(id);
    if(it == m.end())
    {
        GrassAgg g; g.minL = minL; g.maxL = maxL; g.weight = w;
        m[id] = g;
    }
    else
    {
        if(minL < it->second.minL) it->second.minL = minL;
        if(maxL > it->second.maxL) it->second.maxL = maxL;
        it->second.weight += w;
    }
}

static void writeGrass(std::ofstream &ox, const std::vector<EncZone> &zones,
                       const TuxemonDb &db, const DatapackWriter &dw)
{
    std::map<int,GrassAgg> day, night;
    std::size_t z = 0;
    while(z < zones.size())
    {
        const Encounter *enc = db.encounter(zones[z].slug);
        ++z;
        if(enc == NULL)
            continue;
        std::size_t i = 0;
        while(i < enc->monsters.size())
        {
            const EncounterMonster &em = enc->monsters[i];
            ++i;
            const int id = dw.idForMonster(em.slug);
            if(id < 0)
                continue;
            if(em.daytime != 0) aggAdd(day, id, em.minLevel, em.maxLevel, em.rate);   // day or both
            if(em.daytime != 1) aggAdd(night, id, em.minLevel, em.maxLevel, em.rate); // night or both
        }
    }
    emitGrassList(ox, "grass", day);
    emitGrassList(ox, "grassNight", night);
}

static void writeBots(std::ofstream &ox, const std::vector<const NpcPlace*> &bots,
                      const std::vector<std::string> &skins,
                      const TuxemonDb &db, const DatapackWriter &dw, const Localization &l10n)
{
    std::size_t b = 0;
    while(b < bots.size())
    {
        const NpcPlace &np = *bots[b];
        const int id = (int)b + 1;
        ++b;
        ox << " <bot id=\"" << id << "\">\n";
        ox << "  <name>" << xmlEsc(l10n.nameEn(np.slug)) << "</name>\n";

        const Economy *eco = np.economy.empty() ? NULL : db.economy(np.economy);
        if(np.shop && eco != NULL && !eco->items.empty())
        {
            ox << "  <step id=\"1\" type=\"text\">\n";
            ox << "   <text><![CDATA[Welcome!<br /><a href=\"2\">Buy</a><br /><a href=\"3\">Sell</a>]]></text>\n";
            ox << "  </step>\n";
            ox << "  <step shop=\"" << id << "\" id=\"2\" type=\"shop\">\n";
            std::size_t i = 0;
            while(i < eco->items.size())
            {
                const int iid = dw.idForItem(eco->items[i].slug);
                if(iid >= 0)
                    ox << "   <product item=\"" << iid << "\" overridePrice=\"" << eco->items[i].price << "\"/>\n";
                ++i;
            }
            ox << "  </step>\n";
            ox << "  <step shop=\"" << id << "\" id=\"3\" type=\"sell\"/>\n";
        }
        else if(!np.party.empty())
        {
            ox << "  <step type=\"fight\" id=\"1\">\n";
            ox << "   <start><![CDATA[Let's battle!]]></start>\n";
            ox << "   <win><![CDATA[You won!]]></win>\n";
            long cash = 0;
            std::size_t i = 0;
            while(i < np.party.size())
            {
                const int mid = dw.idForMonster(np.party[i].first);
                if(mid >= 0)
                {
                    ox << "   <monster id=\"" << mid << "\" level=\"" << np.party[i].second << "\"/>\n";
                    cash += 30L * np.party[i].second;
                }
                ++i;
            }
            ox << "   <gain cash=\"" << cash << "\"/>\n";
            ox << "  </step>\n";
        }
        else
        {
            std::string text;
            std::size_t i = 0;
            while(i < np.dialog.size())
            {
                if(!text.empty()) text += "<br />";
                text += l10n.nameEn(np.dialog[i]);
                ++i;
            }
            if(text.empty())
                text = "...";
            ox << "  <step type=\"text\" id=\"1\">\n";
            ox << "   <text><![CDATA[" << cdataSafe(text) << "]]></text>\n";
            ox << "  </step>\n";
        }
        ox << " </bot>\n";
    }
}

// ── shared event interpreter ───────────────────────────────────────────────
// One Tuxemon event = a property set (actN/condN/behavN) anchored at a pixel
// rect.  Used by BOTH encodings: TMX object properties AND the sidecar
// <map>.yaml events (newer Tuxemon moved 62 maps' events there).
struct EventSink {
    std::vector<Warp> *warps;
    std::vector<EncZone> *encZones;
    std::map<std::string,NpcPlace> *npcMap;
    const std::unordered_map<std::string,std::pair<int,int> > *mapDims;
};

static void processEventProps(const std::map<std::string,std::string> &props,
                              int ox, int oy, int ow, int oh, EventSink &sink)
{
    // determine the event's "subject" NPC (for dialogue) from a
    // behav "talk <slug>" or a create_npc/char_talk action
    std::string subject;
    std::vector<std::string> pendingDialog;
    std::map<std::string,std::string>::const_iterator it = props.begin();
    while(it != props.end())
    {
        const std::string &key = it->first;
        const std::string &val = it->second;
        if(key.rfind("behav", 0) == 0 && val.rfind("talk ", 0) == 0)
            subject = val.substr(5);
        if(key.rfind("act", 0) == 0)
        {
            std::string verb;
            std::vector<std::string> a;
            splitAction(val, verb, a);
            if(verb == "transition_teleport" && a.size() >= 4)
            {
                Warp wp;
                wp.srcTx = ox / 16; wp.srcTy = oy / 16;
                std::string dest = a[1];
                const std::size_t dot = dest.rfind(".tmx");
                if(dot != std::string::npos) dest = dest.substr(0, dot);
                wp.dest = dest;
                wp.dx = QString::fromStdString(a[2]).toInt();
                wp.dy = QString::fromStdString(a[3]).toInt();
                // clamp destination into the target map (Tuxemon source
                // sometimes points past the edge -> engine "out of range")
                std::unordered_map<std::string,std::pair<int,int> >::const_iterator dim = sink.mapDims->find(dest);
                if(dim != sink.mapDims->cend())
                {
                    if(wp.dx >= dim->second.first)  wp.dx = dim->second.first - 1;
                    if(wp.dy >= dim->second.second) wp.dy = dim->second.second - 1;
                }
                if(wp.dx < 0) wp.dx = 0;
                if(wp.dy < 0) wp.dy = 0;
                sink.warps->push_back(wp);
            }
            else if(verb == "random_encounter" && !a.empty())
            {
                EncZone z;
                z.x0 = ox / 16; z.y0 = oy / 16;
                z.x1 = (ox + ow + 15) / 16; z.y1 = (oy + oh + 15) / 16;
                z.slug = a[0];
                sink.encZones->push_back(z);
            }
            else if(verb == "create_npc" && a.size() >= 3)
            {
                NpcPlace &np = (*sink.npcMap)[a[0]];
                np.slug = a[0];
                np.x = QString::fromStdString(a[1]).toInt();
                np.y = QString::fromStdString(a[2]).toInt();
                if(a.size() >= 4) np.facing = a[3];
                if(subject.empty()) subject = a[0];
            }
            else if(verb == "char_face" && a.size() >= 2)
                (*sink.npcMap)[a[0]].facing = a[1];
            else if(verb == "set_economy" && a.size() >= 2)
            {
                (*sink.npcMap)[a[0]].economy = a[1];
                (*sink.npcMap)[a[0]].shop = true;
            }
            else if(verb == "open_shop" && !a.empty())
                (*sink.npcMap)[a[0]].shop = true;
            else if(verb == "add_monster" && a.size() >= 3)
                (*sink.npcMap)[a[2]].party.push_back(std::make_pair(a[0], QString::fromStdString(a[1]).toInt()));
            else if(verb == "char_talk" && !a.empty())
                subject = a[0];
            else if((verb == "translated_dialog" || verb == "translated_translation" || verb == "dialog") && !a.empty())
                pendingDialog.push_back(a[0]);
        }
        ++it;
    }
    if(!subject.empty() && !pendingDialog.empty())
    {
        NpcPlace &np = (*sink.npcMap)[subject];
        if(np.slug.empty()) np.slug = subject;
        std::size_t di = 0;
        while(di < pendingDialog.size()) { np.dialog.push_back(pendingDialog[di]); ++di; }
    }
}

// Feed the sidecar <map>.yaml events (x/y/width/height in TILES, actions/
// conditions/behav as lists) through the same interpreter.
static void loadYamlEvents(const std::string &tmxPath, EventSink &sink)
{
    std::string ypath = tmxPath;
    const std::size_t dot = ypath.rfind(".tmx");
    if(dot == std::string::npos)
        return;
    ypath = ypath.substr(0, dot) + ".yaml";
    if(!QFile::exists(QString::fromStdString(ypath)))
        return;
    YAML::Node root;
    try { root = YAML::LoadFile(ypath); }
    catch(...) { std::cerr << "  warning: unreadable map yaml " << ypath << std::endl; return; }
    YAML::Node evs = root["events"];
    if(!evs || !evs.IsMap())
        return;
    YAML::Node::const_iterator e = evs.begin();
    while(e != evs.end())
    {
        const YAML::Node &ev = e->second;
        ++e;
        if(!ev.IsMap())
            continue;
        const int x = ev["x"] ? ev["x"].as<int>(0) : 0;
        const int y = ev["y"] ? ev["y"].as<int>(0) : 0;
        const int w = ev["width"] ? ev["width"].as<int>(1) : 1;
        const int h = ev["height"] ? ev["height"].as<int>(1) : 1;
        std::map<std::string,std::string> props;
        int n = 1;
        if(ev["actions"] && ev["actions"].IsSequence())
        {
            YAML::Node::const_iterator a = ev["actions"].begin();
            while(a != ev["actions"].end())
            {
                props["act" + std::to_string(n)] = a->as<std::string>(std::string());
                ++n; ++a;
            }
        }
        if(ev["behav"])
        {
            if(ev["behav"].IsSequence())
            {
                n = 1;
                YAML::Node::const_iterator b = ev["behav"].begin();
                while(b != ev["behav"].end())
                {
                    props["behav" + std::to_string(n)] = b->as<std::string>(std::string());
                    ++n; ++b;
                }
            }
            else if(ev["behav"].IsScalar())
                props["behav1"] = ev["behav"].as<std::string>(std::string());
        }
        processEventProps(props, x * 16, y * 16, w * 16, h * 16, sink);
    }
}

bool MapConverter::convertOne(const std::string &tmxPath)
{
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(tmxPath.c_str()) != tinyxml2::XML_SUCCESS)
        return false;
    tinyxml2::XMLElement *map = doc.RootElement();
    if(map == NULL || std::strcmp(map->Name(), "map") != 0)
        return false;

    const int w = map->IntAttribute("width");
    const int h = map->IntAttribute("height");
    if(w < 1 || h < 1 || w > 127 || h > 127)
    {
        std::cerr << "  skip " << QFileInfo(QString::fromStdString(tmxPath)).fileName().toStdString()
                  << " (size " << w << "x" << h << " out of 1..127)" << std::endl;
        return false;
    }

    const std::string slug = QFileInfo(QString::fromStdString(tmxPath)).completeBaseName().toStdString();

    // output folder + short file name: map/main/tuxemon/<region>/<location>/
    // <base>.tmx (from the pre-pass)
    std::string relDir = "other/" + slug;
    std::string fileBase = slug;
    {
        std::unordered_map<std::string,std::string>::const_iterator rd = relDir_.find(slug);
        if(rd != relDir_.end())
            relDir = rd->second;
        std::unordered_map<std::string,std::string>::const_iterator fb = fileBase_.find(slug);
        if(fb != fileBase_.end())
            fileBase = fb->second;
    }
    const std::string outDir = mapDir_ + "/" + relDir;

    // tilesets (firstgid, basename) — external <tileset source> and inline ones
    const std::string mapDirAbs = QFileInfo(QString::fromStdString(tmxPath)).absolutePath().toStdString();
    std::vector<std::pair<uint32_t,std::string> > tilesets;
    tinyxml2::XMLElement *tsx = map->FirstChildElement("tileset");
    while(tsx != NULL)
    {
        const char *src = tsx->Attribute("source");
        const uint32_t firstgid = (uint32_t)tsx->UnsignedAttribute("firstgid");
        if(src != NULL)
        {
            const std::string base = QFileInfo(QString::fromUtf8(src)).fileName().toStdString();
            const std::string san = copyTileset(base);
            if(!san.empty())
                tilesets.push_back(std::make_pair(firstgid, san));
        }
        else
        {
            const std::string base = materializeInline(tsx, mapDirAbs);
            if(!base.empty())
                tilesets.push_back(std::make_pair(firstgid, base));
        }
        tsx = tsx->NextSiblingElement("tileset");
    }

    // tile layers
    std::vector<std::vector<uint32_t> > groundLayers;
    std::vector<std::vector<uint32_t> > overLayers;
    tinyxml2::XMLElement *layer = map->FirstChildElement("layer");
    while(layer != NULL)
    {
        tinyxml2::XMLElement *data = layer->FirstChildElement("data");
        if(data != NULL)
        {
            std::vector<uint32_t> gids;
            if(decodeLayer(data->Attribute("encoding"), data->Attribute("compression"),
                           data->GetText(), w, h, gids))
            {
                std::string name = layer->Attribute("name") != NULL ? layer->Attribute("name") : "";
                std::string lower = name;
                std::size_t c = 0;
                while(c < lower.size()) { lower[c] = (char)std::tolower((unsigned char)lower[c]); ++c; }
                if(lower.find("above") != std::string::npos || lower.find("over") != std::string::npos)
                    overLayers.push_back(gids);
                else
                    groundLayers.push_back(gids);
            }
        }
        layer = layer->NextSiblingElement("layer");
    }

    // collision rectangles, teleport warps, encounter zones and NPCs from the
    // object groups (Tuxemon encodes everything as cond/act event properties).
    std::vector<bool> blocked((std::size_t)w * h, false);
    std::vector<Warp> warps;
    std::vector<EncZone> encZones;
    std::map<std::string,NpcPlace> npcMap;
    EventSink sink;
    sink.warps = &warps;
    sink.encZones = &encZones;
    sink.npcMap = &npcMap;
    sink.mapDims = &mapDims_;
    tinyxml2::XMLElement *og = map->FirstChildElement("objectgroup");
    while(og != NULL)
    {
        std::string gname = og->Attribute("name") != NULL ? og->Attribute("name") : "";
        std::string glower = gname;
        std::size_t c = 0;
        while(c < glower.size()) { glower[c] = (char)std::tolower((unsigned char)glower[c]); ++c; }
        const bool isCollision = glower.find("collision") != std::string::npos;

        tinyxml2::XMLElement *obj = og->FirstChildElement("object");
        while(obj != NULL)
        {
            const int ox = (int)obj->IntAttribute("x");
            const int oy = (int)obj->IntAttribute("y");
            const int ow = obj->IntAttribute("width") > 0 ? (int)obj->IntAttribute("width") : 16;
            const int oh = obj->IntAttribute("height") > 0 ? (int)obj->IntAttribute("height") : 16;
            if(isCollision)
            {
                int ty = oy / 16;
                while(ty < (oy + oh + 15) / 16 && ty < h)
                {
                    int tx = ox / 16;
                    while(tx < (ox + ow + 15) / 16 && tx < w)
                    {
                        if(tx >= 0 && ty >= 0)
                            blocked[(std::size_t)tx + (std::size_t)ty * w] = true;
                        ++tx;
                    }
                    ++ty;
                }
            }

            std::map<std::string,std::string> props;
            readProps(obj, props);
            processEventProps(props, ox, oy, ow, oh, sink);
            obj = obj->NextSiblingElement("object");
        }
        og = og->NextSiblingElement("objectgroup");
    }
    // newer Tuxemon keeps part of the events in a sidecar <map>.yaml
    loadYamlEvents(tmxPath, sink);

    // Build the grass-encounter mask: cells inside a random_encounter zone that
    // are walkable become a "Grass" monster-collision layer (wild encounters).
    std::vector<bool> grass((std::size_t)w * h, false);
    std::size_t zi = 0;
    while(zi < encZones.size())
    {
        const EncZone &z = encZones[zi];
        ++zi;
        int ty = z.y0;
        while(ty < z.y1 && ty < h)
        {
            int tx = z.x0;
            while(tx < z.x1 && tx < w)
            {
                if(tx >= 0 && ty >= 0)
                    grass[(std::size_t)tx + (std::size_t)ty * w] = true;
                ++tx;
            }
            ++ty;
        }
    }
    if(!encZones.empty())
        ++encounterZones_;

    std::size_t bi = 0;
    while(bi < blocked.size()) { if(blocked[bi]) ++collisionCells_; ++bi; }
    warpsTotal_ += (int)warps.size();

    // Track a good walkable start location (prefer town/city maps with a
    // walkable cell near the centre).
    {
        int walkCount = 0, bestCell = -1, bestDist = 1 << 30;
        const int cx = w / 2, cy = h / 2;
        int yy = 0;
        while(yy < h)
        {
            int xx = 0;
            while(xx < w)
            {
                const std::size_t i = (std::size_t)xx + (std::size_t)yy * w;
                bool ground = false;
                std::size_t gl = 0;
                while(gl < groundLayers.size()) { if((groundLayers[gl][i] & kGidMask) != 0) ground = true; ++gl; }
                if(ground && !blocked[i])
                {
                    ++walkCount;
                    const int d = (xx - cx) * (xx - cx) + (yy - cy) * (yy - cy);
                    if(d < bestDist) { bestDist = d; bestCell = (int)i; }
                }
                ++xx;
            }
            ++yy;
        }
        int score = walkCount;
        if(slug.find("town") != std::string::npos || slug.find("city") != std::string::npos)
            score += 100000;
        if(bestCell >= 0 && score > startScore_)
        {
            startScore_ = score;
            startMap_ = relDir + "/" + fileBase;
            startX_ = bestCell % w;
            startY_ = bestCell / w;
        }
    }

    // ── emit the CatchChallenger .tmx ──
    QDir().mkpath(QString::fromStdString(outDir));
    std::ofstream o(outDir + "/" + fileBase + ".tmx");
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    o << "<map version=\"1.10\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\""
      << w << "\" height=\"" << h << "\" tilewidth=\"16\" tileheight=\"16\" infinite=\"0\">\n";
    std::size_t ti = 0;
    while(ti < tilesets.size())
    {
        o << " <tileset firstgid=\"" << tilesets[ti].first << "\" source=\"../../tileset/"
          << tilesets[ti].second << "\"/>\n";
        ++ti;
    }
    // marker tileset placed after the last used gid: bot/teleport objects carry
    // a gid into it so they are visible in Tiled (the gen2 reference scheme)
    uint32_t invisibleFirstGid = 1;
    ti = 0;
    while(ti < tilesets.size())
    {
        int count = 1024; // unknown tilecount: stay safely past it
        std::unordered_map<std::string,int>::const_iterator tc = tsxCount_.find(tilesets[ti].second);
        if(tc != tsxCount_.end() && tc->second > 0)
            count = tc->second;
        if(tilesets[ti].first + (uint32_t)count > invisibleFirstGid)
            invisibleFirstGid = tilesets[ti].first + (uint32_t)count;
        ++ti;
    }
    o << " <tileset firstgid=\"" << invisibleFirstGid << "\" source=\"../../../../invisible.tsx\"/>\n";

    int layerId = 1;
    // Split each ground layer by behaviour: Collisions (blocked), Grass (wild
    // encounter zone, walkable), else Walkable.
    std::size_t gl = 0;
    while(gl < groundLayers.size())
    {
        std::vector<uint32_t> walk((std::size_t)w * h, 0);
        std::vector<uint32_t> coll((std::size_t)w * h, 0);
        std::vector<uint32_t> grassL((std::size_t)w * h, 0);
        std::size_t i = 0;
        while(i < walk.size())
        {
            if((groundLayers[gl][i] & kGidMask) != 0)
            {
                if(blocked[i])      coll[i]   = groundLayers[gl][i];
                else if(grass[i])   grassL[i] = groundLayers[gl][i];
                else                walk[i]   = groundLayers[gl][i];
            }
            ++i;
        }
        if(anyTile(walk))
        {
            o << " <layer id=\"" << layerId++ << "\" name=\"Walkable\" width=\"" << w << "\" height=\"" << h << "\">\n";
            o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(walk) << "</data>\n";
            o << " </layer>\n";
        }
        if(anyTile(grassL))
        {
            o << " <layer id=\"" << layerId++ << "\" name=\"Grass\" width=\"" << w << "\" height=\"" << h << "\">\n";
            o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(grassL) << "</data>\n";
            o << " </layer>\n";
        }
        if(anyTile(coll))
        {
            o << " <layer id=\"" << layerId++ << "\" name=\"Collisions\" width=\"" << w << "\" height=\"" << h << "\">\n";
            o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(coll) << "</data>\n";
            o << " </layer>\n";
        }
        ++gl;
    }

    // A pure-collision cell with no ground tile still needs to block: ensure a
    // Collisions layer exists carrying those cells (use the first tileset gid).
    bool haveBareBlocked = false;
    bi = 0;
    while(bi < blocked.size())
    {
        if(blocked[bi])
        {
            bool covered = false;
            std::size_t gl2 = 0;
            while(gl2 < groundLayers.size()) { if((groundLayers[gl2][bi] & kGidMask) != 0) covered = true; ++gl2; }
            if(!covered) haveBareBlocked = true;
        }
        ++bi;
    }
    if(haveBareBlocked && !tilesets.empty())
    {
        std::vector<uint32_t> coll((std::size_t)w * h, 0);
        bi = 0;
        while(bi < blocked.size())
        {
            if(blocked[bi])
            {
                bool covered = false;
                std::size_t gl2 = 0;
                while(gl2 < groundLayers.size()) { if((groundLayers[gl2][bi] & kGidMask) != 0) covered = true; ++gl2; }
                if(!covered) coll[bi] = tilesets.front().first;
            }
            ++bi;
        }
        o << " <layer id=\"" << layerId++ << "\" name=\"Collisions\" width=\"" << w << "\" height=\"" << h << "\">\n";
        o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(coll) << "</data>\n";
        o << " </layer>\n";
    }

    // Over-tiles -> WalkBehind (last tile layer).
    std::size_t ol = 0;
    while(ol < overLayers.size())
    {
        if(anyTile(overLayers[ol]))
        {
            o << " <layer id=\"" << layerId++ << "\" name=\"WalkBehind\" width=\"" << w << "\" height=\"" << h << "\">\n";
            o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(overLayers[ol]) << "</data>\n";
            o << " </layer>\n";
        }
        ++ol;
    }

    // Warps in the "Moving" object group (object Y carries the -1 tile offset).
    if(!warps.empty())
    {
        o << " <objectgroup name=\"Moving\">\n";
        std::vector<std::pair<int,int> > seenWarp; // dedup by source tile
        std::size_t wi = 0;
        while(wi < warps.size())
        {
            const Warp &wp = warps[wi];
            const std::pair<int,int> key(wp.srcTx, wp.srcTy);
            bool dup = false;
            std::size_t q = 0;
            while(q < seenWarp.size()) { if(seenWarp[q] == key) dup = true; ++q; }
            if(dup) { ++wi; continue; }
            seenWarp.push_back(key);
            const int px = wp.srcTx * 16;
            const int py = (wp.srcTy + 1) * 16; // loader does y/16 - 1
            // destination path relative to this map's folder, short name
            std::string destPath = wp.dest;
            std::unordered_map<std::string,std::string>::const_iterator dd = relDir_.find(wp.dest);
            std::unordered_map<std::string,std::string>::const_iterator db = fileBase_.find(wp.dest);
            if(dd != relDir_.end() && db != fileBase_.end())
                destPath = relMapPath(relDir, dd->second, db->second);
            o << "  <object type=\"teleport on it\" gid=\"" << (invisibleFirstGid + 2)
              << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            o << "   <properties>\n";
            o << "    <property name=\"map\" value=\"" << destPath << "\"/>\n";
            o << "    <property name=\"x\" value=\"" << wp.dx << "\"/>\n";
            o << "    <property name=\"y\" value=\"" << wp.dy << "\"/>\n";
            o << "   </properties>\n";
            o << "  </object>\n";
            ++wi;
        }
        o << " </objectgroup>\n";
    }

    // Bots: assign ids 1..255 to the placed NPCs, generate a skin per NPC sprite.
    std::vector<const NpcPlace*> bots;
    {
        std::map<std::string,NpcPlace>::const_iterator it = npcMap.begin();
        while(it != npcMap.end())
        {
            if(!it->second.slug.empty() && bots.size() < 255)
                bots.push_back(&it->second);
            ++it;
        }
    }
    std::vector<std::string> botSkin(bots.size());
    if(!bots.empty())
    {
        o << " <objectgroup name=\"Object\">\n";
        std::size_t b = 0;
        while(b < bots.size())
        {
            const NpcPlace &np = *bots[b];
            std::string sprite = np.slug, sheet;
            const Npc *npd = db_.npc(np.slug);
            if(npd != NULL) { sprite = npd->spriteName; sheet = npd->combatSheet; }
            const std::string skinName = sanitizeName(sprite);
            skins_.ensure("bot", skinName, sprite, sheet);
            botSkin[b] = skinName;
            const int px = np.x * 16;
            const int py = (np.y + 1) * 16;
            o << "  <object type=\"bot\" gid=\"" << invisibleFirstGid
              << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            o << "   <properties>\n";
            o << "    <property name=\"id\" type=\"int\" value=\"" << (b + 1) << "\"/>\n";
            o << "    <property name=\"skin\" value=\"" << skinName << "\"/>\n";
            o << "    <property name=\"lookAt\" value=\"" << facingToLookAt(np.facing) << "\"/>\n";
            o << "   </properties>\n";
            o << "  </object>\n";
            ++b;
        }
        o << " </objectgroup>\n";
    }
    o << "</map>\n";
    o.close();
    botsTotal_ += (int)bots.size();

    // sidecar <map>.xml: type + name + grass encounters + bot definitions
    std::string type = "outdoor";
    std::map<std::string,std::string> mp;
    tinyxml2::XMLElement *mprops = map->FirstChildElement("properties");
    if(mprops != NULL)
    {
        tinyxml2::XMLElement *p = mprops->FirstChildElement("property");
        while(p != NULL)
        {
            if(p->Attribute("name") != NULL && p->Attribute("value") != NULL)
                mp[p->Attribute("name")] = p->Attribute("value");
            p = p->NextSiblingElement("property");
        }
        if(mp.find("inside") != mp.end() && mp["inside"] == "true")
            type = "indoor";
        if(mp.find("map_type") != mp.end())
        {
            const std::string &mt = mp["map_type"];
            if(mt.find("cave") != std::string::npos || mt.find("dungeon") != std::string::npos)
                type = "cave";
            else if(mt.find("interior") != std::string::npos || mt.find("inside") != std::string::npos)
                type = "indoor";
        }
    }
    std::ofstream ox(outDir + "/" + fileBase + ".xml");
    ox << "<map type=\"" << type << "\">\n";
    // display name from the Tuxemon catalogue (nameEn title-cases the slug
    // when untranslated), keyed by the map's own slug property when present
    std::string dispSlug = slug;
    if(mp.find("slug") != mp.end() && !mp["slug"].empty())
        dispSlug = mp["slug"];
    ox << " <name>" << xmlEsc(l10n_.nameEn(dispSlug)) << "</name>\n";
    writeGrass(ox, encZones, db_, dw_);
    writeBots(ox, bots, botSkin, db_, dw_, l10n_);
    ox << "</map>\n";
    return true;
}

// Pre-pass: read every source map's size, scenario/inside properties and warp
// targets, then derive relDir_ ("region/location") — region from the scenario
// (else the nearest one through the warp graph, else "other"), location from
// the nearest outdoor map so a town and its interiors share one folder.
void MapConverter::computeLayout()
{
    std::map<std::string,MapMeta> meta;
    QDir mdir(QString::fromStdString(modRoot_ + "/maps"));
    QStringList filters;
    filters << "*.tmx";
    const QFileInfoList files = mdir.entryInfoList(filters, QDir::Files, QDir::Name);
    int fi = 0;
    while(fi < files.size())
    {
        const std::string slug = files.at(fi).completeBaseName().toStdString();
        ++fi;
        tinyxml2::XMLDocument d;
        if(d.LoadFile(mdir.absoluteFilePath(QString::fromStdString(slug) + ".tmx").toStdString().c_str()) != tinyxml2::XML_SUCCESS)
            continue;
        tinyxml2::XMLElement *r = d.RootElement();
        if(r == NULL)
            continue;
        MapMeta mm;
        mm.w = (int)r->IntAttribute("width");
        mm.h = (int)r->IntAttribute("height");
        tinyxml2::XMLElement *props = r->FirstChildElement("properties");
        if(props != NULL)
        {
            tinyxml2::XMLElement *p = props->FirstChildElement("property");
            while(p != NULL)
            {
                const char *n = p->Attribute("name");
                const char *v = p->Attribute("value");
                if(n != NULL && v != NULL)
                {
                    if(std::strcmp(n, "scenario") == 0)
                        mm.scenario = v;
                    else if(std::strcmp(n, "inside") == 0 && std::strcmp(v, "true") == 0)
                        mm.inside = true;
                }
                p = p->NextSiblingElement("property");
            }
        }
        // warp targets (transition_teleport actions on any object)
        tinyxml2::XMLElement *og = r->FirstChildElement("objectgroup");
        while(og != NULL)
        {
            tinyxml2::XMLElement *obj = og->FirstChildElement("object");
            while(obj != NULL)
            {
                std::map<std::string,std::string> oprops;
                readProps(obj, oprops);
                std::map<std::string,std::string>::const_iterator it = oprops.begin();
                while(it != oprops.end())
                {
                    if(it->first.rfind("act", 0) == 0)
                    {
                        std::string verb;
                        std::vector<std::string> a;
                        splitAction(it->second, verb, a);
                        if(verb == "transition_teleport" && a.size() >= 4)
                        {
                            std::string dest = a[1];
                            const std::size_t dot = dest.rfind(".tmx");
                            if(dot != std::string::npos) dest = dest.substr(0, dot);
                            mm.warpDests.push_back(dest);
                        }
                    }
                    ++it;
                }
                obj = obj->NextSiblingElement("object");
            }
            og = og->NextSiblingElement("objectgroup");
        }
        // warps from the sidecar <map>.yaml events too (the location grouping
        // follows the warp graph, so the yaml-event maps must contribute)
        {
            std::vector<Warp> yw;
            std::vector<EncZone> yz;
            std::map<std::string,NpcPlace> yn;
            EventSink ysink;
            ysink.warps = &yw;
            ysink.encZones = &yz;
            ysink.npcMap = &yn;
            ysink.mapDims = &mapDims_;
            loadYamlEvents(mdir.absoluteFilePath(QString::fromStdString(slug) + ".tmx").toStdString(), ysink);
            std::size_t yi = 0;
            while(yi < yw.size())
            {
                mm.warpDests.push_back(yw[yi].dest);
                ++yi;
            }
        }
        mapDims_[slug] = std::make_pair(mm.w, mm.h);
        meta[slug] = mm;
    }

    // undirected warp graph between existing maps
    AdjGraph adj;
    std::map<std::string,MapMeta>::const_iterator mi = meta.begin();
    while(mi != meta.end())
    {
        std::size_t wi = 0;
        while(wi < mi->second.warpDests.size())
        {
            const std::string &dest = mi->second.warpDests[wi];
            if(dest != mi->first && meta.find(dest) != meta.end())
            {
                adj[mi->first].insert(dest);
                adj[dest].insert(mi->first);
            }
            ++wi;
        }
        ++mi;
    }

    // phase 1: anchor of every map (itself when outdoor, else the nearest
    // outdoor map through the warp graph; indoor-only component -> its
    // smallest slug)
    std::map<std::string,std::string> anchorOf; // slug -> anchor slug
    mi = meta.begin();
    while(mi != meta.end())
    {
        const std::string &slug = mi->first;
        std::set<std::string> component;
        std::string anchor = bfsNearest(slug, meta, adj, true, &component);
        if(anchor.empty())
        {
            anchor = slug;
            std::set<std::string>::const_iterator v = component.begin();
            while(v != component.end()) { if(*v < anchor) anchor = *v; ++v; }
        }
        anchorOf[slug] = anchor;
        ++mi;
    }

    // phase 2: one folder per anchor — region/location, both dashed; the
    // anchor map itself is named after the location (gen2: violet-city/
    // violet-city.tmx).  Token prefixes are stripped, never hardcoded names.
    std::map<std::string,std::string> anchorDir;                       // anchor -> "region/location"
    std::map<std::string,std::vector<std::string> > anchorRegionTok;   // anchor -> region tokens
    std::map<std::string,std::vector<std::string> > anchorLocTok;      // anchor -> location tokens
    std::set<std::string> locUsed;                                     // taken "region/location" dirs
    std::map<std::string,std::set<std::string> > usedNames;            // dir -> taken file basenames
    std::map<std::string,std::string>::const_iterator ai = anchorOf.begin();
    while(ai != anchorOf.end())
    {
        const std::string &anchor = ai->second;
        ++ai;
        if(anchorDir.find(anchor) != anchorDir.end())
            continue;
        std::string regionRaw = meta[anchor].scenario;
        if(regionRaw.empty())
        {
            const std::string near = bfsNearest(anchor, meta, adj, false, NULL);
            if(!near.empty())
                regionRaw = meta[near].scenario;
        }
        if(regionRaw.empty())
            regionRaw = "other";
        const std::vector<std::string> regionTok = splitTokens(regionRaw);
        const std::string region = sanitizeFsBase(joinDash(regionTok));
        // location: the anchor slug minus the region token prefix
        std::vector<std::string> locTok = splitTokens(anchor);
        if(commonPrefixLen(locTok, regionTok) == regionTok.size() && locTok.size() > regionTok.size())
            locTok.erase(locTok.begin(), locTok.begin() + (long)regionTok.size());
        std::string dir = region + "/" + sanitizeFsBase(joinDash(locTok));
        if(locUsed.find(dir) != locUsed.end())
        {
            // stripped name collides with another location: keep the full slug
            locTok = splitTokens(anchor);
            dir = region + "/" + sanitizeFsBase(joinDash(locTok));
            int n = 2;
            while(locUsed.find(dir) != locUsed.end())
            {
                dir = region + "/" + sanitizeFsBase(joinDash(locTok)) + "-" + std::to_string(n);
                ++n;
            }
        }
        locUsed.insert(dir);
        anchorDir[anchor] = dir;
        anchorRegionTok[anchor] = regionTok;
        anchorLocTok[anchor] = locTok;
        relDir_[anchor] = dir;
        fileBase_[anchor] = uniqueInDir(dir, sanitizeFsBase(joinDash(locTok)), usedNames);
    }

    // phase 3: short file name for every non-anchor map — strip the region
    // token prefix, then the tokens shared with the location (gen2:
    // violet-city/gym.tmx, not violet-city/violet-city-gym.tmx)
    std::map<std::string,std::string>::const_iterator si = anchorOf.begin();
    while(si != anchorOf.end())
    {
        const std::string &slug = si->first;
        const std::string &anchor = si->second;
        ++si;
        if(slug == anchor)
            continue;
        const std::string &dir = anchorDir[anchor];
        relDir_[slug] = dir;
        std::vector<std::string> t = splitTokens(slug);
        const std::vector<std::string> &rt = anchorRegionTok[anchor];
        if(commonPrefixLen(t, rt) == rt.size() && t.size() > rt.size())
            t.erase(t.begin(), t.begin() + (long)rt.size());
        const std::vector<std::string> &lt = anchorLocTok[anchor];
        const std::size_t c = commonPrefixLen(t, lt);
        if(c > 0 && c < t.size())
            t.erase(t.begin(), t.begin() + (long)c);
        fileBase_[slug] = uniqueInDir(dir, sanitizeFsBase(joinDash(t)), usedNames);
    }
}

// Install the engine marker tileset (same bytes as the official datapack's
// map/invisible.*) so warp/bot objects can carry a visible gid in Tiled.
void MapConverter::writeInvisibleTileset()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map"));
    std::ofstream png(outRoot_ + "/map/invisible.png", std::ios::binary);
    png.write(reinterpret_cast<const char*>(kInvisiblePng), kInvisiblePngLen);
    png.close();
    std::ofstream tsx(outRoot_ + "/map/invisible.tsx");
    tsx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    tsx << "<tileset name=\"invisible.tsx\" tilewidth=\"16\" tileheight=\"16\">\n";
    tsx << " <image source=\"invisible.png\" trans=\"000000\" width=\"64\" height=\"64\"/>\n";
    tsx << "</tileset>\n";
}

int MapConverter::convertAll()
{
    QDir mdir(QString::fromStdString(modRoot_ + "/maps"));
    QStringList filters;
    filters << "*.tmx";
    const QFileInfoList files = mdir.entryInfoList(filters, QDir::Files, QDir::Name);

    // pre-pass: map sizes (warp clamping) + the region/location output layout
    computeLayout();
    writeInvisibleTileset();

    int ok = 0, fail = 0;
    int i = 0;
    while(i < files.size())
    {
        if(convertOne(files.at(i).absoluteFilePath().toStdString()))
            ++ok;
        else
            ++fail;
        ++i;
    }
    std::cerr << "Maps: " << ok << " converted, " << fail << " skipped; "
              << usedTilesetFiles_.size() << " tileset files, " << warpsTotal_ << " warps, "
              << collisionCells_ << " collision cells, " << encounterZones_ << " maps with encounters, "
              << botsTotal_ << " bots." << std::endl;
    return ok;
}

} // namespace tuxemon
