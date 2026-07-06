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
#include <cstdlib>
#include <cstring>
#include <algorithm>
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
      itemsTotal_(0), ioErrors_(0),
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
std::string MapConverter::copyImageSan(const std::string &baseDir, const std::string &srcRel)
{
    const std::string orig = QFileInfo(QString::fromUtf8(srcRel.c_str())).fileName().toStdString();
    std::unordered_map<std::string,std::string>::const_iterator it = imgSan_.find(orig);
    if(it != imgSan_.end())
        return it->second;
    const std::string san = uniqueTilesetFile(orig);
    imgSan_[orig] = san;
    const QString from = QString::fromStdString(baseDir) + "/" + QString::fromUtf8(srcRel.c_str());
    const QString to = QString::fromStdString(tilesetDir_) + "/" + QString::fromStdString(san);
    QFile::remove(to);
    if(!QFile::copy(QFileInfo(from).absoluteFilePath(), to))
    {
        // an unchecked copy loses the tileset image silently -> broken map
        std::cerr << "  error: cannot copy tileset image " << baseDir << "/" << srcRel
                  << " -> " << to.toStdString() << std::endl;
        ++ioErrors_;
    }
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
            const std::string san = copyImageSan(imgBaseDir, src);
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
                const std::string san = copyImageSan(imgBaseDir, src);
                ti->SetAttribute("source", san.c_str());
            }
            ti = ti->NextSiblingElement("image");
        }
        tile = tile->NextSiblingElement("tile");
    }

    if(doc.SaveFile((tilesetDir_ + "/" + sanTsx).c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "  error: cannot write tileset " << tilesetDir_ << "/" << sanTsx << std::endl;
        ++ioErrors_;
    }
    return sanTsx;
}

std::string MapConverter::materializeInline(void *tilesetElement, const std::string &mapDirAbs)
{
    tinyxml2::XMLElement *ts = reinterpret_cast<tinyxml2::XMLElement*>(tilesetElement);
    const char *name = ts->Attribute("name");
    tinyxml2::XMLElement *img = ts->FirstChildElement("image");
    if(name == NULL || img == NULL || img->Attribute("source") == NULL)
        return std::string();

    const std::string imgSrc = img->Attribute("source");
    // dedup by name + image source + geometry, NOT by name alone: citypark.tmx
    // and route5.tmx both embed an inline tileset called "Tileset" with
    // DIFFERENT images (Superpowers_Tilesheet.png vs Tileset.png) — a name-only
    // key bound the second map to the first map's image and rendered garbage
    const std::string origKey = std::string("inline:") + name + "|"
            + QFileInfo(QString::fromUtf8(imgSrc.c_str())).fileName().toStdString() + "|"
            + std::to_string(ts->IntAttribute("tilecount")) + "|"
            + std::to_string(ts->IntAttribute("columns"));
    std::unordered_map<std::string,std::string>::const_iterator done = tsxSan_.find(origKey);
    if(done != tsxSan_.end())
        return done->second;

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
    const std::string sanImg = copyImageSan(mapDirAbs, imgSrc);

    std::ofstream t(tilesetDir_ + "/" + sanTsx);
    t << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    t << "<tileset version=\"1.10\" name=\"" << name << "\" tilewidth=\""
      << ts->IntAttribute("tilewidth", 16) << "\" tileheight=\"" << ts->IntAttribute("tileheight", 16)
      << "\" tilecount=\"" << ts->IntAttribute("tilecount") << "\" columns=\"" << ts->IntAttribute("columns") << "\">\n";
    t << " <image source=\"" << sanImg << "\" width=\"" << img->IntAttribute("width")
      << "\" height=\"" << img->IntAttribute("height") << "\"/>\n";
    t << "</tileset>\n";
    t.close();
    if(t.fail())
    {
        std::cerr << "  error: cannot write tileset " << tilesetDir_ << "/" << sanTsx << std::endl;
        ++ioErrors_;
    }
    return sanTsx;
}

// ── polyline/polygon collision rasterisation ────────────────────────────────
// A Tuxemon collision object may be a <polyline>/<polygon> (fences, walls —
// 111 objects in 27 maps); reading only x/y/width/height blocked just the
// 16x16 origin tile and players walked through the rest of the shape.
// Points are PIXELS relative to the object's x/y.
static void parsePoints(const char *s, double bx, double by,
                        std::vector<std::pair<double,double> > &pts)
{
    pts.clear();
    if(s == NULL)
        return;
    const char *p = s;
    while(*p)
    {
        char *end = NULL;
        const double x = std::strtod(p, &end);
        if(end == p)
            ++p;
        else
        {
            p = end;
            if(*p == ',')
            {
                ++p;
                const double y = std::strtod(p, &end);
                if(end != p)
                {
                    p = end;
                    pts.push_back(std::make_pair(bx + x, by + y));
                }
            }
        }
    }
}

// Mark every 16px tile crossed by the segment (x0,y0)-(x1,y1) as blocked
// (walked in small sub-tile steps so no crossed tile is skipped).
static void markLineTiles(double x0, double y0, double x1, double y1,
                          int w, int h, std::vector<bool> &blocked)
{
    const double dx = x1 - x0, dy = y1 - y0;
    double len = dx >= 0 ? dx : -dx;
    const double ady = dy >= 0 ? dy : -dy;
    if(ady > len) len = ady;
    int steps = (int)(len / 4.0) + 1;
    int i = 0;
    while(i <= steps)
    {
        const double t = (double)i / (double)steps;
        const double px = x0 + dx * t;
        const double py = y0 + dy * t;
        if(px >= 0 && py >= 0)
        {
            const int tx = (int)(px / 16.0);
            const int ty = (int)(py / 16.0);
            if(tx < w && ty < h)
                blocked[(std::size_t)tx + (std::size_t)ty * w] = true;
        }
        ++i;
    }
}

// Even-odd point-in-polygon test (for the <polygon> interior fill).
static bool pointInPoly(double cx, double cy,
                        const std::vector<std::pair<double,double> > &pts)
{
    bool inside = false;
    std::size_t i = 0, j = pts.size() - 1;
    while(i < pts.size())
    {
        const double xi = pts[i].first, yi = pts[i].second;
        const double xj = pts[j].first, yj = pts[j].second;
        if((yi > cy) != (yj > cy) &&
           cx < (xj - xi) * (cy - yi) / (yj - yi) + xi)
            inside = !inside;
        j = i;
        ++i;
    }
    return inside;
}

// Rasterise one polyline (open) or polygon (closed + interior) collision
// object into the blocked grid.
static void rasterisePoly(const std::vector<std::pair<double,double> > &pts,
                          bool closed, int w, int h, std::vector<bool> &blocked)
{
    if(pts.size() < 2)
        return;
    std::size_t i = 0;
    while(i + 1 < pts.size())
    {
        markLineTiles(pts[i].first, pts[i].second, pts[i + 1].first, pts[i + 1].second, w, h, blocked);
        ++i;
    }
    if(closed)
    {
        markLineTiles(pts.back().first, pts.back().second, pts.front().first, pts.front().second, w, h, blocked);
        // interior fill: even-odd test at every tile centre of the bbox
        double minX = pts[0].first, maxX = pts[0].first;
        double minY = pts[0].second, maxY = pts[0].second;
        i = 1;
        while(i < pts.size())
        {
            if(pts[i].first < minX)  minX = pts[i].first;
            if(pts[i].first > maxX)  maxX = pts[i].first;
            if(pts[i].second < minY) minY = pts[i].second;
            if(pts[i].second > maxY) maxY = pts[i].second;
            ++i;
        }
        int ty = minY > 0 ? (int)(minY / 16.0) : 0;
        while(ty < h && ty * 16 <= (int)maxY)
        {
            int tx = minX > 0 ? (int)(minX / 16.0) : 0;
            while(tx < w && tx * 16 <= (int)maxX)
            {
                if(pointInPoly(tx * 16 + 8.0, ty * 16 + 8.0, pts))
                    blocked[(std::size_t)tx + (std::size_t)ty * w] = true;
                ++tx;
            }
            ++ty;
        }
    }
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
// A floor item / chest (add_item action anchored at a tile).
struct ItemPlace { int tx, ty; std::string slug; };
// One dialog "page": lines shown together; a translated_dialog_choice ends the
// page and its options become <a href> links to the next step.
struct DialogPage {
    std::vector<std::string> lines;   // l18n msgids
    std::vector<std::string> choices; // l18n msgids of the options ("" = none)
};
struct NpcPlace {
    std::string slug;
    int x, y;
    std::string facing;
    std::string economy;
    bool shop;
    bool wander;   // create_npc ...,wander / char_wander -> lookAt="move"
    bool heal;     // set_monster_health / (set_)teleport_faint -> heal step
    bool skinless; // synthetic sign/computer/scenery dialog bot (no skin)
    bool placed;   // got real coordinates (create_npc args or event tile);
                   // a merely talked-about NPC must NOT land at (0,0)
    std::vector<std::pair<std::string,int> > party; // monster slug, level
    std::vector<DialogPage> pages;                  // dialog, split at choices
    NpcPlace() : x(0), y(0), shop(false), wander(false), heal(false),
                 skinless(false), placed(false) {}
};

// Append a dialog line to the (possibly new) last open page.
static void addDialogLine(std::vector<DialogPage> &pages, const std::string &msgid)
{
    if(pages.empty() || !pages.back().choices.empty())
        pages.push_back(DialogPage());
    pages.back().lines.push_back(msgid);
}

static std::string facingToLookAt(const std::string &f)
{
    if(f == "up")    return "top";
    if(f == "down")  return "bottom";
    if(f == "left")  return "left";
    if(f == "right") return "right";
    return "bottom";
}

// Skin names feed skin/bot/<name>/ paths: the network sync path
// (FacilityLibGeneral::getSuffixAndValidatePathFromFS) drops anything outside
// [a-z0-9._/-], so lowercase like sanitizeFsBase does.
static std::string sanitizeName(const std::string &s)
{
    std::string t;
    std::size_t i = 0;
    while(i < s.size())
    {
        char c = s[i];
        if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if(std::isalnum((unsigned char)c) || c == '_' || c == '-')
            t.push_back(c);
        ++i;
    }
    if(t.empty()) t = "npc";
    return t;
}

// Per-map music track name -> datapack file base (shared convention with the
// WorldWriter music transcoder): tolower, every char not in [a-z0-9] -> '-'.
static std::string sanitizeMusicTrack(const std::string &s)
{
    std::string t;
    std::size_t i = 0;
    while(i < s.size())
    {
        char c = s[i];
        if(c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
        if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            t.push_back(c);
        else
            t.push_back('-');
        ++i;
    }
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

// Emit one day+night wild list from encounter-table slugs.  dayTag/nightTag =
// "grass"/"grassNight" (layer-bound) or "cave"/"caveNight": the layer-LESS
// monstersCollision entry of map/layers.xml gives the walk-anywhere cave
// monsterType (Map_loader.cpp loadMonsterMap, caveName lookup ~l.202) — used
// for dungeon maps and no-rect random_encounter events.
static void writeWild(std::ofstream &ox, const std::vector<std::string> &slugs,
                      const TuxemonDb &db, const DatapackWriter &dw,
                      const char *dayTag, const char *nightTag)
{
    std::map<int,GrassAgg> day, night;
    std::size_t z = 0;
    while(z < slugs.size())
    {
        const Encounter *enc = db.encounter(slugs[z]);
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
    emitGrassList(ox, dayTag, day);
    emitGrassList(ox, nightTag, night);
}

// Tuxemon dialog templates (${{name}}, ${{currency}}, ${{today}}, …) would
// otherwise leak literally into the bot text (47 source lines).  Known tokens
// get a neutral replacement, every other ${{...}} is scrubbed away.
static std::string scrubTemplates(std::string s)
{
    std::size_t p = 0;
    while((p = s.find("${{", p)) != std::string::npos)
    {
        const std::size_t e = s.find("}}", p + 3);
        if(e == std::string::npos)
        {
            s.erase(p); // unterminated token: drop the tail
            break;
        }
        const std::string token = s.substr(p + 3, e - (p + 3));
        std::string rep;
        if(token == "name")          rep = "traveler";
        else if(token == "currency") rep = "$";
        else if(token == "today")    rep = "today";
        // any other token (map_name, north, var:..., …) is dropped
        s.replace(p, e + 2 - p, rep);
        p += rep.size();
    }
    return s;
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
        ox << "  <name>" << xmlEsc(scrubTemplates(l10n.nameEn(np.slug))) << "</name>\n";

        const Economy *eco = np.economy.empty() ? NULL : db.economy(np.economy);
        if(np.shop && eco != NULL && !eco->items.empty())
        {
            ox << "  <step id=\"1\" type=\"text\">\n";
            ox << "   <text><![CDATA[Welcome!<br /><a href=\"2\">Buy</a><br /><a href=\"3\">Sell</a>";
            if(np.heal) // café/clinic counter that both sells and heals
                ox << "<br /><a href=\"4\">Heal</a>";
            ox << "]]></text>\n";
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
            if(np.heal) // MapServer::parseUnknownBotStep registers the heal spot
                ox << "  <step id=\"4\" type=\"heal\"/>\n";
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
        else if(np.pages.empty())
        {
            if(np.heal)
            {
                // official heal-bot pattern (e.g. castel-town/indoor/heal.xml):
                // yes/no text step linking to a <step type="heal"> that
                // MapServer::parseUnknownBotStep (server/base/MapServer.cpp
                // ~l.234) registers at the bot's tile
                ox << "  <step id=\"1\" type=\"text\">\n";
                ox << "   <text><![CDATA[Do you want to be healed?<br /><a href=\"2\">Yes</a><br /><a href=\"close\">No</a>]]></text>\n";
                ox << "  </step>\n";
                ox << "  <step id=\"2\" type=\"heal\"/>\n";
            }
            else
            {
                ox << "  <step type=\"text\" id=\"1\">\n";
                ox << "   <text><![CDATA[...]]></text>\n";
                ox << "  </step>\n";
            }
        }
        else
        {
            // dialog pages -> chained text steps; a translated_dialog_choice
            // ends its page with one <a href> link per option (the same step
            // navigation the shop Buy/Sell menu uses); the heal step (when
            // any) is the final step of the chain
            const int healStep = np.heal ? (int)np.pages.size() + 1 : 0;
            std::size_t k = 0;
            while(k < np.pages.size())
            {
                const DialogPage &pg = np.pages[k];
                const int nextId = (k + 1 < np.pages.size()) ? (int)k + 2 : healStep;
                std::string text;
                std::size_t i = 0;
                while(i < pg.lines.size())
                {
                    const std::string line = scrubTemplates(l10n.nameEn(pg.lines[i]));
                    if(!line.empty())
                    {
                        if(!text.empty()) text += "<br />";
                        text += line;
                    }
                    ++i;
                }
                if(text.empty())
                    text = "...";
                i = 0;
                while(i < pg.choices.size())
                {
                    std::string label = scrubTemplates(l10n.nameEn(pg.choices[i]));
                    if(label.empty())
                        label = "...";
                    // a "no" option (or a choice with nothing after it) ends
                    // the conversation like the official heal bots do
                    std::string href = "close";
                    if(pg.choices[i] != "no" && nextId != 0)
                        href = std::to_string(nextId);
                    text += "<br /><a href=\"" + href + "\">" + label + "</a>";
                    ++i;
                }
                if(pg.choices.empty() && nextId != 0)
                    text += "<br /><a href=\"" + std::to_string(nextId) + "\">(continue)</a>";
                ox << "  <step type=\"text\" id=\"" << (k + 1) << "\">\n";
                ox << "   <text><![CDATA[" << cdataSafe(text) << "]]></text>\n";
                ox << "  </step>\n";
                ++k;
            }
            if(np.heal)
                ox << "  <step id=\"" << healStep << "\" type=\"heal\"/>\n";
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
    std::vector<ItemPlace> *items;        // add_item floor items/chests
    std::vector<std::string> *caveSlugs;  // no-rect random_encounter tables
    std::vector<std::string> *music;      // play_music track slugs
};

// actN/behavN keys must be interpreted in NUMERIC order: the lexicographic
// std::map order puts act10 before act2 (spyder_cotton_scoop.tmx etc.) and
// concatenated 6 bots' dialogs out of order.
struct OrderedProp {
    int num;
    std::string key;
    std::string val;
};
static bool orderedPropLess(const OrderedProp &a, const OrderedProp &b)
{
    if(a.num != b.num)
        return a.num < b.num;
    return a.key < b.key;
}
static void collectOrdered(const std::map<std::string,std::string> &props,
                           const char *prefix, std::vector<OrderedProp> &out)
{
    const std::size_t plen = std::strlen(prefix);
    out.clear();
    std::map<std::string,std::string>::const_iterator it = props.begin();
    while(it != props.end())
    {
        if(it->first.rfind(prefix, 0) == 0)
        {
            OrderedProp op;
            op.num = 0;
            std::size_t i = plen;
            while(i < it->first.size())
            {
                const char c = it->first[i];
                if(c >= '0' && c <= '9')
                    op.num = op.num * 10 + (c - '0');
                ++i;
            }
            op.key = it->first;
            op.val = it->second;
            out.push_back(op);
        }
        ++it;
    }
    std::sort(out.begin(), out.end(), orderedPropLess);
}

// positioned = the event carries a real x/y anchor.  A yaml event WITHOUT one
// is a Tuxemon GLOBAL scripted event (cutscene), not a placed trigger: its
// teleports/encounters/items must NOT be emitted at tile (0,0) — 31 cutscene
// teleports used to become unconditional corner warps.  NPC creation
// (create_npc carries its own coordinates) still applies.
static void processEventProps(const std::map<std::string,std::string> &props,
                              int ox, int oy, int ow, int oh, EventSink &sink,
                              bool positioned)
{
    // determine the event's "subject" NPC (for dialogue) from a
    // behav "talk <slug>" or a create_npc/char_talk action
    std::string subject;
    std::vector<DialogPage> pendingPages;
    bool pendingHeal = false;
    std::vector<OrderedProp> ordered;
    collectOrdered(props, "behav", ordered);
    std::size_t oi = 0;
    while(oi < ordered.size())
    {
        if(ordered[oi].val.rfind("talk ", 0) == 0)
            subject = ordered[oi].val.substr(5);
        ++oi;
    }
    collectOrdered(props, "act", ordered);
    oi = 0;
    while(oi < ordered.size())
    {
        const std::string &val = ordered[oi].val;
        ++oi;
        {
            std::string verb;
            std::vector<std::string> a;
            splitAction(val, verb, a);
            if(verb == "transition_teleport" && a.size() >= 4)
            {
                if(positioned)
                {
                    Warp wp;
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
                    // the event rect may span several tiles (2-wide stairs,
                    // 3-wide doorways, whole-edge transitions): a CC teleport is
                    // per-tile, so emit one warp on EVERY covered tile
                    const int tx0 = ox / 16, ty0 = oy / 16;
                    const int tx1 = (ox + ow + 15) / 16, ty1 = (oy + oh + 15) / 16;
                    int ty = ty0;
                    while(ty < ty1)
                    {
                        int tx = tx0;
                        while(tx < tx1)
                        {
                            wp.srcTx = tx;
                            wp.srcTy = ty;
                            sink.warps->push_back(wp);
                            ++tx;
                        }
                        ++ty;
                    }
                }
                // else: scripted cutscene teleport, not a tile warp — skip
            }
            else if(verb == "random_encounter" && !a.empty())
            {
                if(positioned)
                {
                    EncZone z;
                    z.x0 = ox / 16; z.y0 = oy / 16;
                    z.x1 = (ox + ow + 15) / 16; z.y1 = (oy + oh + 15) / 16;
                    z.slug = a[0];
                    sink.encZones->push_back(z);
                }
                else if(sink.caveSlugs != NULL)
                    // no-rect table -> map-wide walk-anywhere ("cave") list
                    sink.caveSlugs->push_back(a[0]);
            }
            else if(verb == "create_npc" && a.size() >= 3)
            {
                NpcPlace &np = (*sink.npcMap)[a[0]];
                np.slug = a[0];
                np.x = QString::fromStdString(a[1]).toInt();
                np.y = QString::fromStdString(a[2]).toInt();
                np.placed = true;
                if(a.size() >= 4)
                {
                    // 4th arg = behaviour: wander -> a moving bot
                    // (lookAt="move", Map_loaderMain keeps the cell walkable)
                    if(a[3] == "wander")
                        np.wander = true;
                    else
                        np.facing = a[3];
                }
                if(subject.empty()) subject = a[0];
            }
            else if(verb == "char_wander" && !a.empty())
                (*sink.npcMap)[a[0]].wander = true;
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
                addDialogLine(pendingPages, a[0]);
            else if(verb == "translated_dialog_choice" && !a.empty())
            {
                // "translated_dialog_choice yes:no,variable" — the options
                // close the current dialog page and become <a href> links
                if(pendingPages.empty() || !pendingPages.back().choices.empty())
                    pendingPages.push_back(DialogPage());
                const QStringList opts = QString::fromStdString(a[0]).split(':');
                int qo = 0;
                while(qo < opts.size())
                {
                    const std::string opt = opts.at(qo).trimmed().toStdString();
                    if(!opt.empty())
                        pendingPages.back().choices.push_back(opt);
                    ++qo;
                }
            }
            else if(verb == "set_monster_health" || verb == "teleport_faint" ||
                    verb == "set_teleport_faint")
                // hospital/café heal (full heal / faint-respawn setup) ->
                // <step type="heal"> (server/base/MapServer.cpp ~l.234)
                pendingHeal = true;
            else if(verb == "add_item" && !a.empty() && positioned && sink.items != NULL)
            {
                // floor item / chest: negative quantity is a REMOVE, a
                // non-numeric quantity (variable) counts as one give
                bool numeric = false;
                const int qty = a.size() >= 2 ? QString::fromStdString(a[1]).toInt(&numeric) : 1;
                if(a.size() < 2 || !numeric || qty > 0)
                {
                    ItemPlace ip;
                    ip.tx = ox / 16;
                    ip.ty = oy / 16;
                    ip.slug = a[0];
                    sink.items->push_back(ip);
                }
            }
            else if(verb == "play_music" && !a.empty() && sink.music != NULL)
                sink.music->push_back(a[0]);
        }
    }
    if(!pendingPages.empty() || pendingHeal)
    {
        if(!subject.empty())
        {
            NpcPlace &np = (*sink.npcMap)[subject];
            if(np.slug.empty()) np.slug = subject;
            if(pendingHeal) np.heal = true;
            std::size_t di = 0;
            while(di < pendingPages.size()) { np.pages.push_back(pendingPages[di]); ++di; }
        }
        else if(positioned)
        {
            // sign/computer/scenery dialog (641 translated_dialog events had
            // no NPC subject and were dropped): a skinless TEXT bot at the
            // event tile — without a "skin" property Map_loaderMain (~l.881)
            // keeps the cell walkable, the client just shows the dialog
            const int tx = ox / 16, ty = oy / 16;
            NpcPlace &np = (*sink.npcMap)["sign:" + std::to_string(tx) + "," + std::to_string(ty)];
            np.slug = "sign";
            np.x = tx;
            np.y = ty;
            np.skinless = true;
            np.placed = true;
            if(pendingHeal) np.heal = true;
            std::size_t di = 0;
            while(di < pendingPages.size()) { np.pages.push_back(pendingPages[di]); ++di; }
        }
        // else: global cutscene text with no anchor — skip
    }
}

// Zero-padded synthetic property index ("0001") so keys sort numerically.
static std::string padNum(int n)
{
    std::string s = std::to_string(n);
    while(s.size() < 4)
        s = "0" + s;
    return s;
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
        // COPY the node: e->second is a member of the iterator's TEMPORARY
        // proxy pair — a reference to it dangles once ++e runs (crashed as
        // YAML::InvalidNode with newer yaml-cpp).  A Node copy is a cheap
        // shared handle and stays valid.
        YAML::Node ev = e->second;
        ++e;
        if(!ev.IsMap())
            continue;
        // an event with neither x nor y is a Tuxemon GLOBAL scripted event
        // (cutscene/init), not a placed trigger — see processEventProps
        const bool positioned = (bool)ev["x"] || (bool)ev["y"];
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
                // zero-padded so even a lexicographic walk keeps act10 > act2
                // (the interpreter additionally sorts numerically)
                props["act" + padNum(n)] = a->as<std::string>(std::string());
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
                    props["behav" + padNum(n)] = b->as<std::string>(std::string());
                    ++n; ++b;
                }
            }
            else if(ev["behav"].IsScalar())
                props["behav" + padNum(1)] = ev["behav"].as<std::string>(std::string());
        }
        processEventProps(props, x * 16, y * 16, w * 16, h * 16, sink, positioned);
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

    // map properties + CC map type — needed EARLY: a "cave"-type map (Tuxemon
    // map_type dungeon) emits its wild list as walk-anywhere <cave> instead of
    // a Grass layer
    std::string type = "outdoor";
    std::map<std::string,std::string> mp;
    {
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
    }

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
    std::vector<ItemPlace> floorItems;
    std::vector<std::string> caveSlugs;
    std::vector<std::string> musicTracks;
    EventSink sink;
    sink.warps = &warps;
    sink.encZones = &encZones;
    sink.npcMap = &npcMap;
    sink.mapDims = &mapDims_;
    sink.items = &floorItems;
    sink.caveSlugs = &caveSlugs;
    sink.music = &musicTracks;
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
                tinyxml2::XMLElement *pl = obj->FirstChildElement("polyline");
                tinyxml2::XMLElement *pg = obj->FirstChildElement("polygon");
                if(pl != NULL || pg != NULL)
                {
                    // fences/walls drawn as polylines/polygons (points are
                    // pixels relative to the object x/y) — a rect read blocked
                    // only the origin tile and players walked through
                    std::vector<std::pair<double,double> > pts;
                    parsePoints(pl != NULL ? pl->Attribute("points") : pg->Attribute("points"),
                                obj->DoubleAttribute("x"), obj->DoubleAttribute("y"), pts);
                    rasterisePoly(pts, pg != NULL, w, h, blocked);
                }
                else
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
            }

            std::map<std::string,std::string> props;
            readProps(obj, props);
            processEventProps(props, ox, oy, ow, oh, sink, true);
            obj = obj->NextSiblingElement("object");
        }
        og = og->NextSiblingElement("objectgroup");
    }
    // newer Tuxemon keeps part of the events in a sidecar <map>.yaml
    loadYamlEvents(tmxPath, sink);

    // Two-tile stair/door graphics: the Tuxemon author often marks only ONE
    // tile of a two-tile staircase as the teleport (eclipse banks).  Extend a
    // warp onto the adjacent tile when it continues the SAME multi-tile
    // graphic: same layer, consecutive gid, bounded run of exactly two, drawn
    // over a non-empty lower layer (an overlay, not the base floor), the
    // neighbour inside the map, walkable, and not already a warp source.
    {
        std::vector<const std::vector<uint32_t>*> stack;
        std::size_t si = 0;
        while(si < groundLayers.size()) { stack.push_back(&groundLayers[si]); ++si; }
        si = 0;
        while(si < overLayers.size()) { stack.push_back(&overLayers[si]); ++si; }
        const std::size_t warpCount = warps.size(); // do not extend extensions
        std::size_t wi2 = 0;
        while(wi2 < warpCount)
        {
            const Warp wp = warps[wi2];
            ++wi2;
            const int x = wp.srcTx, y = wp.srcTy;
            if(x < 0 || y < 0 || x >= (int)w || y >= (int)h)
                continue;
            // first layer with a tile here that sits ABOVE another tiled layer
            std::size_t li = 0;
            while(li < stack.size())
            {
                const std::vector<uint32_t> &g = *stack[li];
                const uint32_t gid = g[(std::size_t)y * w + x] & kGidMask;
                if(gid != 0)
                {
                    bool lower = false;
                    std::size_t lj = 0;
                    while(lj < li) { if(((*stack[lj])[(std::size_t)y * w + x] & kGidMask) != 0) lower = true; ++lj; }
                    if(lower)
                    {
                        int d = -1;
                        while(d <= 1)
                        {
                            if(d != 0)
                            {
                                const int nx = x + d;
                                const uint32_t ngid = gid + (uint32_t)d;
                                const uint32_t atN  = (nx >= 0 && nx < (int)w) ? (g[(std::size_t)y * w + nx] & kGidMask) : 0;
                                // run must be exactly the two tiles (bounded)
                                const int bx = x - d, bn = nx + d;
                                const uint32_t atB  = (bx >= 0 && bx < (int)w) ? (g[(std::size_t)y * w + bx] & kGidMask) : 0;
                                const uint32_t atBn = (bn >= 0 && bn < (int)w) ? (g[(std::size_t)y * w + bn] & kGidMask) : 0;
                                if(nx >= 0 && nx < (int)w && ngid != 0 &&
                                   atN == ngid && atB != gid - (uint32_t)d && atBn != ngid + (uint32_t)d &&
                                   !blocked[(std::size_t)y * w + nx])
                                {
                                    bool taken = false;
                                    std::size_t q = 0;
                                    while(q < warps.size()) { if(warps[q].srcTx == nx && warps[q].srcTy == y) taken = true; ++q; }
                                    if(!taken)
                                    {
                                        Warp ext = wp;
                                        ext.srcTx = nx;
                                        warps.push_back(ext);
                                    }
                                }
                            }
                            ++d;
                        }
                        break; // only the first overlay layer decides
                    }
                }
                ++li;
            }
        }
    }

    // Build the grass-encounter mask: cells inside a random_encounter zone that
    // are walkable become a "Grass" monster-collision layer (wild encounters).
    // A cave-type map instead routes its zones to the layer-less <cave> list
    // (sidecar emission below) — no Grass layer there.
    std::vector<bool> grass((std::size_t)w * h, false);
    if(type != "cave")
    {
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
    }
    if(!encZones.empty() || !caveSlugs.empty())
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
    // Per-map music as a MAP-LEVEL <property>: the client reads it from the
    // .tmx (MapVisualiserPlayer::currentBackgroundsound() -> tiledMap
    // properties).  Value is LABEL-ROOT-relative ("music/<track>.opus",
    // resolved as datapackPathMain()+value); the WorldWriter transcodes the
    // referenced tracks with the same sanitize.  Must precede <tileset> per
    // the Tiled child order.  Most-played track wins, first-seen on a tie.
    if(!musicTracks.empty())
    {
        std::string best = musicTracks[0];
        int bestCount = 0;
        std::map<std::string,int> counts;
        std::size_t mi = 0;
        while(mi < musicTracks.size())
        {
            const int c = ++counts[musicTracks[mi]];
            if(c > bestCount)
            {
                bestCount = c;
                best = musicTracks[mi];
            }
            ++mi;
        }
        o << " <properties>\n  <property name=\"backgroundsound\" value=\"music/"
          << sanitizeMusicTrack(best) << ".opus\"/>\n </properties>\n";
    }
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
            // only NPCs that got REAL coordinates (create_npc / event tile):
            // a merely talked-about NPC would land on the (0,0) corner.
            // A Tuxemon scripted event can carry a NOMINAL out-of-bounds tile
            // (e.g. player_house_bedroom "Intro No" at x=9 on a 9-wide map) —
            // the engine warns "bot out of map" and drops it, so skip here.
            if(!it->second.slug.empty() && it->second.placed && bots.size() < 255 &&
               it->second.x >= 0 && it->second.y >= 0 &&
               it->second.x < w && it->second.y < h)
                bots.push_back(&it->second);
            ++it;
        }
    }
    std::vector<std::string> botSkin(bots.size());
    if(!bots.empty() || !floorItems.empty())
    {
        o << " <objectgroup name=\"Object\">\n";
        std::size_t b = 0;
        while(b < bots.size())
        {
            const NpcPlace &np = *bots[b];
            const int px = np.x * 16;
            const int py = (np.y + 1) * 16;
            o << "  <object type=\"bot\" gid=\"" << invisibleFirstGid
              << "\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            o << "   <properties>\n";
            o << "    <property name=\"id\" type=\"int\" value=\"" << (b + 1) << "\"/>\n";
            if(np.skinless)
            {
                // sign/computer/scenery dialog: no skin — Map_loaderMain
                // (~l.881) then keeps the cell walkable and the client only
                // shows the dialog steps
            }
            else
            {
                std::string sprite = np.slug, sheet;
                const Npc *npd = db_.npc(np.slug);
                if(npd != NULL) { sprite = npd->spriteName; sheet = npd->combatSheet; }
                const std::string skinName = sanitizeName(sprite);
                skins_.ensure("bot", skinName, sprite, sheet);
                botSkin[b] = skinName;
                o << "    <property name=\"skin\" value=\"" << skinName << "\"/>\n";
                // wander behaviour -> lookAt="move": Map_loaderMain (~l.881)
                // keeps the bot cell walkable and the client animates it
                if(np.wander)
                    o << "    <property name=\"lookAt\" value=\"move\"/>\n";
                else
                    o << "    <property name=\"lookAt\" value=\"" << facingToLookAt(np.facing) << "\"/>\n";
            }
            o << "   </properties>\n";
            o << "  </object>\n";
            ++b;
        }
        // Floor items/chests (gen2 object scheme): the engine resolves a
        // non-numeric "item" property through tempNameToItemId[str_tolower]
        // (Map_loaderMain ~l.485), so the value is the lowercased English
        // display name — matching the items.xml <name> DatapackWriter emits.
        std::vector<std::pair<int,int> > seenItem; // one item per tile
        std::size_t fi = 0;
        while(fi < floorItems.size())
        {
            const ItemPlace &ip = floorItems[fi];
            ++fi;
            if(ip.tx >= 0 && ip.ty >= 0 && ip.tx < w && ip.ty < h)
            {
                const std::pair<int,int> key(ip.tx, ip.ty);
                bool dup = false;
                std::size_t q = 0;
                while(q < seenItem.size()) { if(seenItem[q] == key) dup = true; ++q; }
                if(!dup)
                {
                    if(dw_.idForItem(ip.slug) < 0)
                        // variable reference or unknown slug: the name would
                        // dangle (engine silently drops it), note and skip
                        std::cerr << "  note: unresolved add_item \"" << ip.slug
                                  << "\" at " << ip.tx << "," << ip.ty << " (" << slug << ")" << std::endl;
                    else
                    {
                        seenItem.push_back(key);
                        std::string iname = l10n_.nameEn(ip.slug);
                        std::size_t ci = 0;
                        while(ci < iname.size())
                        {
                            if(iname[ci] >= 'A' && iname[ci] <= 'Z')
                                iname[ci] = (char)(iname[ci] - 'A' + 'a');
                            ++ci;
                        }
                        o << "  <object type=\"object\" gid=\"" << (invisibleFirstGid + 1)
                          << "\" x=\"" << (ip.tx * 16) << "\" y=\"" << ((ip.ty + 1) * 16)
                          << "\" width=\"16\" height=\"16\">\n";
                        o << "   <properties>\n";
                        o << "    <property name=\"item\" value=\"" << xmlEsc(iname) << "\"/>\n";
                        o << "   </properties>\n";
                        o << "  </object>\n";
                        ++itemsTotal_;
                    }
                }
            }
        }
        o << " </objectgroup>\n";
    }
    o << "</map>\n";
    o.close();
    if(o.fail())
    {
        std::cerr << "  error: cannot write " << outDir << "/" << fileBase << ".tmx" << std::endl;
        ++ioErrors_;
        return false;
    }
    botsTotal_ += (int)bots.size();

    // sidecar <map>.xml: type + name + wild encounters + bot definitions
    std::ofstream ox(outDir + "/" + fileBase + ".xml");
    ox << "<map type=\"" << type << "\">\n";
    // display name from the Tuxemon catalogue (nameEn title-cases the slug
    // when untranslated), keyed by the map's own slug property when present
    std::string dispSlug = slug;
    if(mp.find("slug") != mp.end() && !mp["slug"].empty())
        dispSlug = mp["slug"];
    ox << " <name>" << xmlEsc(l10n_.nameEn(dispSlug)) << "</name>\n";
    // wild lists: a cave-type map puts EVERY table in the layer-less
    // walk-anywhere <cave> list (tunnels had 0 encounters when their zones
    // went to a Grass layer that was never emitted); positioned zones on
    // other maps stay on the Grass layer; no-rect tables are map-wide cave
    {
        std::vector<std::string> grassSlugs;
        std::size_t z = 0;
        while(z < encZones.size())
        {
            if(type == "cave")
                caveSlugs.push_back(encZones[z].slug);
            else
                grassSlugs.push_back(encZones[z].slug);
            ++z;
        }
        writeWild(ox, grassSlugs, db_, dw_, "grass", "grassNight");
        writeWild(ox, caveSlugs, db_, dw_, "cave", "caveNight");
    }
    writeBots(ox, bots, botSkin, db_, dw_, l10n_);
    ox << "</map>\n";
    ox.close();
    if(ox.fail())
    {
        std::cerr << "  error: cannot write " << outDir << "/" << fileBase << ".xml" << std::endl;
        ++ioErrors_;
        return false;
    }
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
            std::vector<ItemPlace> yitems;
            std::vector<std::string> ycave, ymusic;
            EventSink ysink;
            ysink.warps = &yw;
            ysink.encZones = &yz;
            ysink.npcMap = &yn;
            ysink.mapDims = &mapDims_;
            ysink.items = &yitems;
            ysink.caveSlugs = &ycave;
            ysink.music = &ymusic;
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
    if(png.fail())
    {
        std::cerr << "  error: cannot write " << outRoot_ << "/map/invisible.png" << std::endl;
        ++ioErrors_;
    }
    std::ofstream tsx(outRoot_ + "/map/invisible.tsx");
    tsx << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    tsx << "<tileset name=\"invisible.tsx\" tilewidth=\"16\" tileheight=\"16\">\n";
    tsx << " <image source=\"invisible.png\" trans=\"000000\" width=\"64\" height=\"64\"/>\n";
    tsx << "</tileset>\n";
    tsx.close();
    if(tsx.fail())
    {
        std::cerr << "  error: cannot write " << outRoot_ << "/map/invisible.tsx" << std::endl;
        ++ioErrors_;
    }
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
              << botsTotal_ << " bots, " << itemsTotal_ << " floor items." << std::endl;
    if(ioErrors_ > 0)
        std::cerr << "ERROR: " << ioErrors_ << " asset write/copy failures (output incomplete)" << std::endl;
    // 0 = full success; else the number of failures (skipped maps + I/O errors).
    return fail + ioErrors_;
}

} // namespace tuxemon
