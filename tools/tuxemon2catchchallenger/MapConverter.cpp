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

MapConverter::MapConverter(const std::string &modRoot, const std::string &outRoot)
    : modRoot_(modRoot), outRoot_(outRoot), warpsTotal_(0), collisionCells_(0)
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

    // collision rectangles + teleport warps from the object groups
    std::vector<bool> blocked((std::size_t)w * h, false);
    std::vector<Warp> warps;
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
            if(isCollision)
            {
                const int ox = (int)obj->IntAttribute("x");
                const int oy = (int)obj->IntAttribute("y");
                const int ow = obj->IntAttribute("width") > 0 ? (int)obj->IntAttribute("width") : 16;
                const int oh = obj->IntAttribute("height") > 0 ? (int)obj->IntAttribute("height") : 16;
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
            // teleport: any property act* == "transition_teleport player,<map>,<x>,<y>,..."
            std::map<std::string,std::string> props;
            readProps(obj, props);
            std::map<std::string,std::string>::const_iterator it = props.begin();
            while(it != props.end())
            {
                if(it->first.rfind("act", 0) == 0 &&
                   it->second.rfind("transition_teleport", 0) == 0)
                {
                    const QStringList parts = QString::fromStdString(it->second).split(',');
                    if(parts.size() >= 4)
                    {
                        Warp wp;
                        wp.srcTx = (int)obj->IntAttribute("x") / 16;
                        wp.srcTy = (int)obj->IntAttribute("y") / 16;
                        std::string dest = parts.at(1).trimmed().toStdString();
                        const std::size_t dot = dest.rfind(".tmx");
                        if(dot != std::string::npos)
                            dest = dest.substr(0, dot);
                        wp.dest = dest;
                        wp.dx = parts.at(2).trimmed().toInt();
                        wp.dy = parts.at(3).trimmed().toInt();
                        warps.push_back(wp);
                        break; // one teleport per object
                    }
                }
                ++it;
            }
            obj = obj->NextSiblingElement("object");
        }
        og = og->NextSiblingElement("objectgroup");
    }

    std::size_t bi = 0;
    while(bi < blocked.size()) { if(blocked[bi]) ++collisionCells_; ++bi; }
    warpsTotal_ += (int)warps.size();

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
    // Split each ground layer into Walkable (passable) + Collisions (blocked).
    std::size_t gl = 0;
    while(gl < groundLayers.size())
    {
        std::vector<uint32_t> walk((std::size_t)w * h, 0);
        std::vector<uint32_t> coll((std::size_t)w * h, 0);
        std::size_t i = 0;
        while(i < walk.size())
        {
            if((groundLayers[gl][i] & kGidMask) != 0)
            {
                if(blocked[i]) coll[i] = groundLayers[gl][i];
                else           walk[i] = groundLayers[gl][i];
            }
            ++i;
        }
        if(anyTile(walk))
        {
            o << " <layer id=\"" << layerId++ << "\" name=\"Walkable\" width=\"" << w << "\" height=\"" << h << "\">\n";
            o << "  <data encoding=\"base64\" compression=\"zlib\">" << encodeLayer(walk) << "</data>\n";
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
    o << "</map>\n";
    o.close();

    // sidecar <map>.xml (type + name)
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
              << collisionCells_ << " collision cells." << std::endl;
    return ok;
}

} // namespace tuxemon
