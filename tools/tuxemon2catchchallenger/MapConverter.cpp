#include "MapConverter.hpp"

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

// ── tileset copy ────────────────────────────────────────────────────────────
void MapConverter::copyTileset(const std::string &tsxBasename)
{
    if(copiedTilesets_.find(tsxBasename) != copiedTilesets_.end())
        return;
    copiedTilesets_.insert(tsxBasename);

    const std::string srcTsx = modRoot_ + "/gfx/tilesets/" + tsxBasename;
    tinyxml2::XMLDocument doc;
    if(doc.LoadFile(srcTsx.c_str()) != tinyxml2::XML_SUCCESS)
    {
        std::cerr << "  warning: cannot read tileset " << srcTsx << std::endl;
        return;
    }
    tinyxml2::XMLElement *ts = doc.RootElement();
    if(ts == NULL)
        return;

    QDir().mkpath(QString::fromStdString(tilesetDir_));

    // Copy every referenced image (single <image> and per-tile <image>),
    // rewriting the source attribute to the bare basename next to the .tsx.
    tinyxml2::XMLElement *img = ts->FirstChildElement("image");
    while(img != NULL)
    {
        const char *src = img->Attribute("source");
        if(src != NULL)
        {
            const QString base = QFileInfo(QString::fromUtf8(src)).fileName();
            const QString from = QString::fromStdString(modRoot_ + "/gfx/tilesets/") + QString::fromUtf8(src);
            const QString to = QString::fromStdString(tilesetDir_) + "/" + base;
            QFile::remove(to);
            QFile::copy(QFileInfo(from).absoluteFilePath(), to);
            img->SetAttribute("source", base.toStdString().c_str());
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
                const QString base = QFileInfo(QString::fromUtf8(src)).fileName();
                const QString from = QString::fromStdString(modRoot_ + "/gfx/tilesets/") + QString::fromUtf8(src);
                const QString to = QString::fromStdString(tilesetDir_) + "/" + base;
                QFile::remove(to);
                QFile::copy(QFileInfo(from).absoluteFilePath(), to);
                ti->SetAttribute("source", base.toStdString().c_str());
            }
            ti = ti->NextSiblingElement("image");
        }
        tile = tile->NextSiblingElement("tile");
    }

    doc.SaveFile((tilesetDir_ + "/" + tsxBasename).c_str());
}

std::string MapConverter::materializeInline(void *tilesetElement, const std::string &mapDirAbs)
{
    tinyxml2::XMLElement *ts = reinterpret_cast<tinyxml2::XMLElement*>(tilesetElement);
    const char *name = ts->Attribute("name");
    tinyxml2::XMLElement *img = ts->FirstChildElement("image");
    if(name == NULL || img == NULL || img->Attribute("source") == NULL)
        return std::string();

    const std::string tsxBase = std::string(name) + ".tsx";
    const QString imgSrc = QString::fromUtf8(img->Attribute("source"));
    const QString imgBase = QFileInfo(imgSrc).fileName();

    if(copiedTilesets_.find(tsxBase) == copiedTilesets_.end())
    {
        copiedTilesets_.insert(tsxBase);
        QDir().mkpath(QString::fromStdString(tilesetDir_));
        // image source is relative to the source map's directory
        const QString from = QString::fromStdString(mapDirAbs) + "/" + imgSrc;
        const QString to = QString::fromStdString(tilesetDir_) + "/" + imgBase;
        QFile::remove(to);
        QFile::copy(QFileInfo(from).absoluteFilePath(), to);

        std::ofstream t(tilesetDir_ + "/" + tsxBase);
        t << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        t << "<tileset version=\"1.10\" name=\"" << name << "\" tilewidth=\""
          << ts->IntAttribute("tilewidth", 16) << "\" tileheight=\"" << ts->IntAttribute("tileheight", 16)
          << "\" tilecount=\"" << ts->IntAttribute("tilecount") << "\" columns=\"" << ts->IntAttribute("columns") << "\">\n";
        t << " <image source=\"" << imgBase.toStdString() << "\" width=\"" << img->IntAttribute("width")
          << "\" height=\"" << img->IntAttribute("height") << "\"/>\n";
        t << "</tileset>\n";
    }
    return tsxBase;
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
            tilesets.push_back(std::make_pair(firstgid, base));
            copyTileset(base);
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
                        warps.push_back(wp);
                    }
                    else if(verb == "random_encounter" && !a.empty())
                    {
                        EncZone z;
                        z.x0 = ox / 16; z.y0 = oy / 16;
                        z.x1 = (ox + ow + 15) / 16; z.y1 = (oy + oh + 15) / 16;
                        z.slug = a[0];
                        encZones.push_back(z);
                    }
                    else if(verb == "create_npc" && a.size() >= 3)
                    {
                        NpcPlace &np = npcMap[a[0]];
                        np.slug = a[0];
                        np.x = QString::fromStdString(a[1]).toInt();
                        np.y = QString::fromStdString(a[2]).toInt();
                        if(a.size() >= 4) np.facing = a[3];
                        if(subject.empty()) subject = a[0];
                    }
                    else if(verb == "char_face" && a.size() >= 2)
                        npcMap[a[0]].facing = a[1];
                    else if(verb == "set_economy" && a.size() >= 2)
                    {
                        npcMap[a[0]].economy = a[1];
                        npcMap[a[0]].shop = true;
                    }
                    else if(verb == "open_shop" && !a.empty())
                        npcMap[a[0]].shop = true;
                    else if(verb == "add_monster" && a.size() >= 3)
                        npcMap[a[2]].party.push_back(std::make_pair(a[0], QString::fromStdString(a[1]).toInt()));
                    else if(verb == "char_talk" && !a.empty())
                        subject = a[0];
                    else if((verb == "translated_dialog" || verb == "translated_translation" || verb == "dialog") && !a.empty())
                        pendingDialog.push_back(a[0]);
                }
                ++it;
            }
            if(!subject.empty() && !pendingDialog.empty())
            {
                NpcPlace &np = npcMap[subject];
                if(np.slug.empty()) np.slug = subject;
                std::size_t di = 0;
                while(di < pendingDialog.size()) { np.dialog.push_back(pendingDialog[di]); ++di; }
            }
            obj = obj->NextSiblingElement("object");
        }
        og = og->NextSiblingElement("objectgroup");
    }

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
            startMap_ = slug;
            startX_ = bestCell % w;
            startY_ = bestCell / w;
        }
    }

    // ── emit the CatchChallenger .tmx ──
    QDir().mkpath(QString::fromStdString(mapDir_));
    std::ofstream o(mapDir_ + "/" + slug + ".tmx");
    o << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    o << "<map version=\"1.10\" orientation=\"orthogonal\" renderorder=\"right-down\" width=\""
      << w << "\" height=\"" << h << "\" tilewidth=\"16\" tileheight=\"16\" infinite=\"0\">\n";
    std::size_t ti = 0;
    while(ti < tilesets.size())
    {
        o << " <tileset firstgid=\"" << tilesets[ti].first << "\" source=\"tileset/"
          << tilesets[ti].second << "\"/>\n";
        ++ti;
    }

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
        std::size_t wi = 0;
        while(wi < warps.size())
        {
            const Warp &wp = warps[wi];
            const int px = wp.srcTx * 16;
            const int py = (wp.srcTy + 1) * 16; // loader does y/16 - 1
            o << "  <object type=\"teleport on it\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
            o << "   <properties>\n";
            o << "    <property name=\"map\" value=\"" << wp.dest << "\"/>\n";
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
            o << "  <object type=\"bot\" x=\"" << px << "\" y=\"" << py << "\" width=\"16\" height=\"16\">\n";
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
    tinyxml2::XMLElement *mprops = map->FirstChildElement("properties");
    if(mprops != NULL)
    {
        tinyxml2::XMLElement *p = mprops->FirstChildElement("property");
        std::map<std::string,std::string> mp;
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
    std::ofstream ox(mapDir_ + "/" + slug + ".xml");
    ox << "<map type=\"" << type << "\">\n";
    ox << " <name>" << slug << "</name>\n";
    writeGrass(ox, encZones, db_, dw_);
    writeBots(ox, bots, botSkin, db_, dw_, l10n_);
    ox << "</map>\n";
    return true;
}

int MapConverter::convertAll()
{
    QDir mdir(QString::fromStdString(modRoot_ + "/maps"));
    QStringList filters;
    filters << "*.tmx";
    const QFileInfoList files = mdir.entryInfoList(filters, QDir::Files, QDir::Name);
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
              << copiedTilesets_.size() << " tilesets, " << warpsTotal_ << " warps, "
              << collisionCells_ << " collision cells, " << encounterZones_ << " maps with encounters, "
              << botsTotal_ << " bots." << std::endl;
    return ok;
}

} // namespace tuxemon
