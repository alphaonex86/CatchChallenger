#include "WorldWriter.hpp"

#include <yaml-cpp/yaml.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileInfoList>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QPainter>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_set>
#include <vector>

namespace tuxemon {

WorldWriter::WorldWriter(const TuxemonDb &db, const Localization &l10n,
                         const std::string &modRoot, const std::string &outRoot,
                         SkinGen &skins, const DatapackWriter &dw,
                         const std::string &startMap, int startX, int startY)
    : db_(db), l10n_(l10n), modRoot_(modRoot), outRoot_(outRoot), skins_(skins),
      dw_(dw), startMap_(startMap), startX_(startX), startY_(startY)
{
}

// ── small helpers ───────────────────────────────────────────────────────────

// Same as DatapackWriter's (static there): escape XML text/attribute content.
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

// yaml helpers mirroring TuxemonDb.cpp's (static there): never throw outward.
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

static int nodeInt(const YAML::Node &n, int def)
{
    if(n && n.IsScalar())
    {
        try { return n.as<int>(); } catch(...) { return def; }
    }
    return def;
}

// mkpath with an error report; every writer checks it (ENOSPC/permission).
static bool ensureDir(const std::string &dir)
{
    if(!QDir().mkpath(QString::fromStdString(dir)))
    {
        std::cerr << "Cannot create directory: " << dir << std::endl;
        return false;
    }
    return true;
}

// Open-check for every std::ofstream writer.
static bool openOk(const std::ofstream &o, const std::string &path)
{
    if(!o.is_open())
    {
        std::cerr << "Cannot open for write: " << path << std::endl;
        return false;
    }
    return true;
}

// Flush+close and report any stream failure (short write on a full disk sets
// badbit only at flush time, so success must be judged AFTER close).
static bool closeOk(std::ofstream &o, const std::string &path)
{
    o.close();
    if(o.fail())
    {
        std::cerr << "Write failed (disk full or I/O error?): " << path << std::endl;
        return false;
    }
    return true;
}

// ── choices ─────────────────────────────────────────────────────────────────

int WorldWriter::chooseStarterMonster() const
{
    // prefer a well-known early monster, else the lowest-id basic-stage one
    const char *preferred[] = {"rockitten", "budaye", "eyenemy", "dollfin", "fruitera"};
    int p = 0;
    while(p < 5)
    {
        std::size_t i = 0;
        while(i < db_.monsters().size())
        {
            if(db_.monsters()[i].slug == preferred[p])
                return db_.monsters()[i].txmnId;
            ++i;
        }
        ++p;
    }
    int best = -1;
    std::size_t i = 0;
    while(i < db_.monsters().size())
    {
        const Monster &m = db_.monsters()[i];
        if(m.stage == "basic" && (best < 0 || m.txmnId < best))
            best = m.txmnId;
        ++i;
    }
    if(best < 0 && !db_.monsters().empty())
        best = db_.monsters()[0].txmnId;
    return best;
}

int WorldWriter::chooseCaptureItem() const
{
    std::size_t i = 0;
    while(i < db_.items().size())
    {
        if(db_.items()[i].category == "capture" || db_.items()[i].slug == "tuxeball")
        {
            const int id = dw_.idForItem(db_.items()[i].slug);
            if(id >= 0)
                return id;
        }
        ++i;
    }
    // fall back to any item
    if(!db_.items().empty())
    {
        const int id = dw_.idForItem(db_.items()[0].slug);
        if(id >= 0)
            return id;
    }
    return 1;
}

// ── writers ─────────────────────────────────────────────────────────────────

bool WorldWriter::writePlayerSkin()
{
    // player overworld+battle sprite per mod.yaml (sprite/combat_sheet = adventurer)
    if(!skins_.ensure("fighter", "player", "adventurer", "adventurer"))
    {
        std::cerr << "Cannot generate the player skin (skin/fighter/player)" << std::endl;
        return false;
    }
    return true;
}

bool WorldWriter::writePlayerStart(int starterId, int captureItemId)
{
    if(!ensureDir(outRoot_ + "/player"))
        return false;
    const std::string path = outRoot_ + "/player/start.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<profile>\n";
    o << "    <start id=\"Normal\">\n";
    o << "        <name>Starter</name>\n";
    o << "        <description>Start a new Tuxemon adventure</description>\n";
    o << "        <forcedskin value=\"player\"/>\n";
    o << "        <cash value=\"500\"/>\n";
    if(starterId >= 0)
    {
        o << "        <monstergroup>\n";
        o << "            <monster id=\"" << starterId << "\" level=\"5\" captured_with=\"" << captureItemId << "\"/>\n";
        o << "        </monstergroup>\n";
    }
    o << "        <reputation type=\"nation\" level=\"1\"/>\n";
    o << "        <item quantity=\"5\" id=\"" << captureItemId << "\"/>\n";
    o << "    </start>\n";
    o << "</profile>\n";
    return closeOk(o, path);
}

// One faction rank (db/faction/*.yaml "ranks": threshold -> title).
struct RankEntry
{
    int threshold;
    std::string title;
};

static bool rankLess(const RankEntry &a, const RankEntry &b)
{
    return a.threshold < b.threshold;
}

// DatapackGeneralLoaderReputation.cpp: type must match ^[a-z]{1,32}$ — keep
// only [a-z] of the lowercased faction slug, capped at 32 chars.
static std::string sanitizeReputationType(const std::string &slug)
{
    std::string out;
    std::size_t i = 0;
    while(i < slug.size() && out.size() < 32)
    {
        char c = slug[i];
        if(c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        if(c >= 'a' && c <= 'z')
            out.push_back(c);
        ++i;
    }
    return out;
}

bool WorldWriter::writeReputation()
{
    if(!ensureDir(outRoot_ + "/player"))
        return false;
    const std::string path = outRoot_ + "/player/reputation.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<reputations>\n";
    // The synthetic "nation" reputation stays: player/start.xml requires
    // <reputation type="nation" level="1"/> and loadProfileList rejects the
    // profile when that type does not resolve.
    o << "    <reputation type=\"nation\">\n";
    o << "        <name>Nation</name>\n";
    o << "        <name lang=\"fr\">Nation</name>\n";
    o << "        <level point=\"-500\"><text>Outlaw</text></level>\n";
    o << "        <level point=\"-100\"><text>Suspect</text></level>\n";
    o << "        <level point=\"0\"><text>Citizen</text></level>\n";
    o << "        <level point=\"100\"><text>Trusted</text></level>\n";
    o << "        <level point=\"500\"><text>Hero</text></level>\n";
    o << "    </reputation>\n";

    // One CC reputation per Tuxemon faction: db/faction/*.yaml "ranks" map 1:1
    // onto reputation levels (DatapackGeneralLoaderReputation.cpp: <reputation
    // type><level point><text>; needs >=2 positive levels including point 0,
    // unique points).  The client (DatapackClientLoader::parseReputation*)
    // additionally wants a <name> and per-level <text>.
    std::unordered_set<std::string> usedTypes;
    usedTypes.insert("nation");
    const QFileInfoList factionFiles =
            QDir(QString::fromStdString(modRoot_ + "/db/faction"))
            .entryInfoList(QStringList() << "*.yaml", QDir::Files, QDir::Name);
    int f = 0;
    while(f < factionFiles.size())
    {
        const std::string file = factionFiles.at(f).absoluteFilePath().toStdString();
        YAML::Node n;
        bool loaded = true;
        try { n = YAML::LoadFile(file); }
        catch(const std::exception &e)
        {
            std::cerr << "Cannot parse faction yaml: " << file << ": " << e.what() << std::endl;
            loaded = false;
        }
        if(loaded && n && n.IsMap())
        {
            const std::string slug = nodeStr(n["slug"], factionFiles.at(f).completeBaseName().toStdString());
            const std::string type = sanitizeReputationType(slug);
            if(type.empty() || usedTypes.find(type) != usedTypes.end())
                std::cerr << "Skip faction " << slug << ": empty or duplicate reputation type \"" << type << "\"" << std::endl;
            else
            {
                // collect the ranks, dedup by threshold (the loader rejects
                // duplicate points), sort ascending
                std::vector<RankEntry> ranks;
                std::unordered_set<int> seen;
                const YAML::Node rn = n["ranks"];
                if(rn && rn.IsSequence())
                {
                    std::size_t r = 0;
                    while(r < rn.size())
                    {
                        const YAML::Node e = rn[r];
                        if(e && e.IsMap() && e["title"] && e["threshold"])
                        {
                            RankEntry re;
                            re.threshold = nodeInt(e["threshold"], 0);
                            re.title = nodeStr(e["title"], "");
                            if(re.title.empty())
                                std::cerr << "Skip rank without title in faction " << slug << std::endl;
                            else
                            {
                                if(seen.find(re.threshold) != seen.end())
                                    std::cerr << "Skip duplicate rank threshold " << re.threshold
                                              << " in faction " << slug << std::endl;
                                else
                                {
                                    seen.insert(re.threshold);
                                    ranks.push_back(re);
                                }
                            }
                        }
                        else
                            std::cerr << "Skip malformed rank entry in faction " << slug << std::endl;
                        ++r;
                    }
                }
                std::sort(ranks.begin(), ranks.end(), rankLess);
                // pre-validate what the loader enforces, so the generated
                // datapack parses with 0 stderr warnings
                unsigned int positives = 0;
                bool hasZero = false;
                std::size_t r = 0;
                while(r < ranks.size())
                {
                    if(ranks[r].threshold >= 0)
                        ++positives;
                    if(ranks[r].threshold == 0)
                        hasZero = true;
                    ++r;
                }
                if(positives < 2 || !hasZero)
                    std::cerr << "Skip faction " << slug << ": needs >=2 ranks with thresholds >=0 "
                              << "including 0 (DatapackGeneralLoaderReputation requirement)" << std::endl;
                else
                {
                    const std::string nameEn = l10n_.nameEn(slug);
                    const std::string nameFr = l10n_.nameFr(slug);
                    o << "    <reputation type=\"" << type << "\">\n";
                    o << "        <name>" << xmlEscape(nameEn) << "</name>\n";
                    if(!nameFr.empty())
                        o << "        <name lang=\"fr\">" << xmlEscape(nameFr) << "</name>\n";
                    r = 0;
                    while(r < ranks.size())
                    {
                        o << "        <level point=\"" << ranks[r].threshold << "\"><text>"
                          << xmlEscape(ranks[r].title) << "</text></level>\n";
                        ++r;
                    }
                    o << "    </reputation>\n";
                    usedTypes.insert(type);
                }
            }
        }
        ++f;
    }
    o << "</reputations>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeEvent()
{
    if(!ensureDir(outRoot_ + "/player"))
        return false;
    const std::string path = outRoot_ + "/player/event.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<events>\n";
    o << "    <event id=\"day\">\n";
    o << "        <value>day</value>\n";
    o << "        <value>night</value>\n";
    o << "    </event>\n";
    o << "</events>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeLayers(int captureItemId)
{
    if(!ensureDir(outRoot_ + "/map"))
        return false;
    const std::string path = outRoot_ + "/map/layers.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<layers zoom=\"4\">\n";
    o << "    <monstersCollision monsterType=\"grass\" background=\"fight/grass/\" layer=\"Grass\" type=\"walkOn\">\n";
    o << "        <event id=\"day\" value=\"night\" monsterType=\"grassNight;grass\"/>\n";
    o << "    </monstersCollision>\n";
    o << "    <monstersCollision monsterType=\"cave\" background=\"fight/cave/\" type=\"walkOn\">\n";
    o << "        <event id=\"day\" value=\"night\" monsterType=\"caveNight;cave\"/>\n";
    o << "    </monstersCollision>\n";
    o << "    <monstersCollision item=\"" << captureItemId << "\" tile=\"swim\" background=\"fight/water/\" layer=\"Water\" type=\"walkOn\" monsterType=\"water\"/>\n";
    o << "</layers>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeVisualCategory()
{
    if(!ensureDir(outRoot_ + "/map"))
        return false;
    const std::string path = outRoot_ + "/map/visualcategory.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<visual>\n";
    o << "    <category id=\"indoor\"/>\n";
    o << "    <category id=\"outdoor\">\n";
    o << "        <event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/>\n";
    o << "    </category>\n";
    o << "    <category id=\"city\">\n";
    o << "        <event id=\"day\" value=\"night\" color=\"#000099\" alpha=\"50\"/>\n";
    o << "    </category>\n";
    o << "    <category id=\"cave\" color=\"#000000\" alpha=\"60\"/>\n";
    o << "</visual>\n";
    return closeOk(o, path);
}

// Resolve db/environment/<slug>.yaml battle_graphics.background (a mod-root-
// relative gfx path) to an existing absolute file; "" when absent/missing.
static QString envBackground(const std::string &modRoot, const char *envSlug)
{
    const std::string file = modRoot + "/db/environment/" + envSlug + ".yaml";
    if(!QFile::exists(QString::fromStdString(file)))
        return QString();
    YAML::Node n;
    try { n = YAML::LoadFile(file); }
    catch(const std::exception &e)
    {
        std::cerr << "Cannot parse environment yaml: " << file << ": " << e.what() << std::endl;
        return QString();
    }
    std::string rel;
    if(n && n.IsMap())
        rel = nodeStr(n["battle_graphics"]["background"], "");
    if(rel.empty())
        return QString();
    const QString abs = QString::fromStdString(modRoot + "/" + rel);
    if(!QFile::exists(abs))
    {
        std::cerr << "Environment " << envSlug << " references a missing background: " << rel << std::endl;
        return QString();
    }
    return abs;
}

// Pick a Tuxemon combat background for a terrain, falling back to any *.png.
static QString pickCombatBg(const QString &combatDir, const QStringList &preferred)
{
    int i = 0;
    while(i < preferred.size())
    {
        const QString p = combatDir + "/" + preferred.at(i);
        if(QFile::exists(p))
            return p;
        ++i;
    }
    QDir d(combatDir);
    QStringList f;
    f << "*_background.png" << "*.png";
    const QStringList all = d.entryList(f, QDir::Files, QDir::Name);
    if(!all.isEmpty())
        return combatDir + "/" + all.first();
    return QString();
}

// A soft semi-transparent ellipse used as a battle platform placeholder.
static bool writePlatform(const QString &path, int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawEllipse(2, 2, w - 4, h - 4);
    p.end();
    if(!img.save(path, "PNG"))
    {
        std::cerr << "Cannot write image: " << path.toStdString() << std::endl;
        return false;
    }
    return true;
}

bool WorldWriter::writeFightBackgrounds()
{
    // The engine consumes map/fight/<terrain>/background.png per the layers.xml
    // background= attribute (DatapackGeneralLoaderMap.cpp monstersCollision;
    // client qtopengl foreground/Battle.cpp loads background.png,
    // plateform-front.png, plateform-background.png, foreground.png from that
    // folder).  The background image comes from the matching Tuxemon biome:
    // db/environment/<slug>.yaml battle_graphics.background — with a filename-
    // preference fallback in gfx/ui/combat when the environment lacks one.
    const QString combat = QString::fromStdString(modRoot_ + "/gfx/ui/combat");
    // (terrain folder referenced by layers.xml, environment slugs to try,
    //  preferred fallback background names)
    struct Bg { const char *dir; const char *env1; const char *env2; QStringList pref; };
    Bg terrains[] = {
        {"grass", "grass", NULL,     QStringList() << "grass_background.png" << "forest_background.png" << "meadow_background.png"},
        {"cave",  "cave",  "cavern", QStringList() << "cave_background.png" << "cavern_background.png"},
        {"water", "ocean", "sea",    QStringList() << "beach_background.png" << "ocean_background.png" << "lake_background.png"},
    };
    bool allOk = true;
    int i = 0;
    while(i < 3)
    {
        const std::string dir = outRoot_ + "/map/fight/" + terrains[i].dir;
        if(!ensureDir(dir))
            allOk = false;
        else
        {
            QString bg = envBackground(modRoot_, terrains[i].env1);
            if(bg.isEmpty() && terrains[i].env2 != NULL)
                bg = envBackground(modRoot_, terrains[i].env2);
            if(bg.isEmpty())
                bg = pickCombatBg(combat, terrains[i].pref);
            if(bg.isEmpty())
                // degraded but valid: the client falls back to its built-in image
                std::cerr << "No battle background found for terrain " << terrains[i].dir << std::endl;
            else
            {
                const QString dst = QString::fromStdString(dir) + "/background.png";
                if(QFile::exists(dst) && !QFile::remove(dst))
                {
                    std::cerr << "Cannot remove stale file: " << dst.toStdString() << std::endl;
                    allOk = false;
                }
                else if(!QFile::copy(bg, dst))
                {
                    std::cerr << "Cannot copy " << bg.toStdString() << " -> " << dst.toStdString() << std::endl;
                    allOk = false;
                }
            }
            if(!writePlatform(QString::fromStdString(dir) + "/plateform-background.png", 160, 48))
                allOk = false;
            if(!writePlatform(QString::fromStdString(dir) + "/plateform-front.png", 160, 48))
                allOk = false;
        }
        ++i;
    }
    // grass also has a foreground overlay in the official datapack
    QImage fg(16, 16, QImage::Format_ARGB32);
    fg.fill(Qt::transparent);
    const std::string fgPath = outRoot_ + "/map/fight/grass/foreground.png";
    if(!fg.save(QString::fromStdString(fgPath), "PNG"))
    {
        std::cerr << "Cannot write image: " << fgPath << std::endl;
        allOk = false;
    }
    return allOk;
}

// ffmpeg-transcode one Tuxemon track to opus (best effort).
static bool transcode(const QString &in, const QString &out)
{
    if(!QFile::exists(in))
        return false;
    QStringList args;
    args << "-y" << "-i" << in << "-ac" << "2" << "-c:a" << "libopus" << "-b:a" << "96k" << out;
    QProcess p;
    p.start("ffmpeg", args);
    if(!p.waitForFinished(60000))
        return false;
    if(p.exitCode() != 0 || !QFile::exists(out))
    {
        // retry with the native opus encoder
        QStringList args2;
        args2 << "-y" << "-i" << in << "-ac" << "2" << "-c:a" << "opus" << "-strict" << "-2" << out;
        QProcess p2;
        p2.start("ffmpeg", args2);
        if(!p2.waitForFinished(60000))
            return false;
        return p2.exitCode() == 0 && QFile::exists(out);
    }
    return true;
}

std::string WorldWriter::sanitizeTrackSlug(const std::string &slug)
{
    std::string out;
    out.reserve(slug.size());
    std::size_t i = 0;
    while(i < slug.size())
    {
        char c = slug[i];
        if(c >= 'A' && c <= 'Z')
            c = (char)(c - 'A' + 'a');
        if((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9'))
            out.push_back(c);
        else
            out.push_back('-');
        ++i;
    }
    return out;
}

// One music track to transcode: the Tuxemon slug + its source file.
struct TrackRef
{
    std::string slug;
    std::string srcAbs;
};

bool WorldWriter::writeMusic()
{
    const std::string musicDir = outRoot_ + "/music";
    if(!ensureDir(musicDir))
        return false;
    const std::string src = modRoot_ + "/music/";

    // Every track db/music/*.yaml declares (slug -> file), so any play_music
    // slug a map references resolves to music/<sanitizeTrackSlug(slug)>.opus
    // (the map converter emits per-map backgroundsound with that exact name).
    std::vector<TrackRef> tracks;
    std::unordered_set<std::string> dbFiles;  // raw "file" fields (dedup + orphan scan)
    const QFileInfoList musicYamls =
            QDir(QString::fromStdString(modRoot_ + "/db/music"))
            .entryInfoList(QStringList() << "*.yaml", QDir::Files, QDir::Name);
    int y = 0;
    while(y < musicYamls.size())
    {
        const std::string file = musicYamls.at(y).absoluteFilePath().toStdString();
        YAML::Node n;
        bool loaded = true;
        try { n = YAML::LoadFile(file); }
        catch(const std::exception &e)
        {
            std::cerr << "Cannot parse music yaml: " << file << ": " << e.what() << std::endl;
            loaded = false;
        }
        if(loaded && n && n.IsSequence())
        {
            std::size_t e = 0;
            while(e < n.size())
            {
                const std::string f = nodeStr(n[e]["file"], "");
                const std::string slug = nodeStr(n[e]["slug"], "");
                if(f.empty() || slug.empty())
                    std::cerr << "Skip malformed music entry in " << file << std::endl;
                else
                {
                    TrackRef t;
                    t.slug = slug;
                    t.srcAbs = src + f;
                    tracks.push_back(t);
                    dbFiles.insert(f);
                }
                ++e;
            }
        }
        ++y;
    }
    // top-level music files without a db entry: slug = the filename stem
    const QFileInfoList loose =
            QDir(QString::fromStdString(modRoot_ + "/music"))
            .entryInfoList(QStringList() << "*.ogg" << "*.mp3", QDir::Files, QDir::Name);
    int l = 0;
    while(l < loose.size())
    {
        if(dbFiles.find(loose.at(l).fileName().toStdString()) == dbFiles.end())
        {
            TrackRef t;
            t.slug = loose.at(l).completeBaseName().toStdString();
            t.srcAbs = loose.at(l).absoluteFilePath().toStdString();
            tracks.push_back(t);
        }
        ++l;
    }

    std::unordered_set<std::string> produced;   // sanitized names that transcoded OK
    std::unordered_set<std::string> attempted;  // output-name dedup
    int okCount = 0;
    std::size_t ti = 0;
    while(ti < tracks.size())
    {
        const std::string outName = sanitizeTrackSlug(tracks[ti].slug);
        if(attempted.find(outName) != attempted.end())
            std::cerr << "Skip duplicate music output name " << outName
                      << " (slug " << tracks[ti].slug << ")" << std::endl;
        else
        {
            attempted.insert(outName);
            if(transcode(QString::fromStdString(tracks[ti].srcAbs),
                         QString::fromStdString(musicDir + "/" + outName + ".opus")))
            {
                produced.insert(outName);
                ++okCount;
            }
            else
                std::cerr << "Music transcode failed: " << tracks[ti].srcAbs
                          << " (slug " << tracks[ti].slug << ")" << std::endl;
        }
        ++ti;
    }
    std::cerr << "Music: " << okCount << "/" << tracks.size() << " tracks transcoded." << std::endl;
    if(!tracks.empty() && okCount == 0)
    {
        std::cerr << "No music track could be transcoded — is ffmpeg installed?" << std::endl;
        return false;
    }

    // map/music.xml: the per-map-type ambiance fallback the client reads via
    // DatapackClientLoader::parseAudioAmbiance (<map type=...>path</map>,
    // resolved against datapackPathMain then datapackPathBase — our music/
    // lives at the datapack root).  Curated real db/music slugs per CC type.
    const char *fallback[][2] = {
        {"city",    "music_cathedral_theme"},  // JRPG_royalCourt_loop.ogg
        {"indoor",  "music_the_princess"},     // JRPG_princess.ogg
        {"outdoor", "music_town_theme"},       // JRPG_town_loop.ogg
        {"cave",    "music_hhavok_main"},      // HHavok-main.ogg
    };
    const std::string path = outRoot_ + "/map/music.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<musics>\n";
    int fb = 0;
    while(fb < 4)
    {
        const std::string outName = sanitizeTrackSlug(fallback[fb][1]);
        if(produced.find(outName) != produced.end())
            o << "    <map type=\"" << fallback[fb][0] << "\">music/" << outName << ".opus</map>\n";
        else
            std::cerr << "music.xml: no transcoded track for type " << fallback[fb][0]
                      << " (" << fallback[fb][1] << ")" << std::endl;
        ++fb;
    }
    o << "</musics>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeRegionInfo()
{
    if(!ensureDir(outRoot_ + "/map/main/tuxemon"))
        return false;
    const std::string path = outRoot_ + "/map/main/tuxemon/informations.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<informations color=\"#4FD9FF\">\n";
    o << "    <author pseudo=\"Tuxemon\" email=\"\" name=\"Tuxemon project\"/>\n";
    o << "    <name>Tuxemon world</name>\n";
    o << "    <name lang=\"fr\">Monde Tuxemon</name>\n";
    o << "    <description>Maps converted from Tuxemon (https://www.tuxemon.org/)</description>\n";
    o << "    <initial>T</initial>\n";
    o << "</informations>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeRegionStart()
{
    if(!ensureDir(outRoot_ + "/map/main/tuxemon"))
        return false;
    const std::string path = outRoot_ + "/map/main/tuxemon/start.xml";
    std::ofstream o(path);
    if(!openOk(o, path))
        return false;
    o << "<profile>\n";
    o << "    <start id=\"Normal\">\n";
    o << "        <map x=\"" << startX_ << "\" y=\"" << startY_ << "\" file=\"" << startMap_ << "\"/>\n";
    o << "    </start>\n";
    o << "</profile>\n";
    return closeOk(o, path);
}

bool WorldWriter::writeEmptyPlantsAndCrafting()
{
    bool ok = true;
    if(!ensureDir(outRoot_ + "/plants"))
        ok = false;
    else
    {
        const std::string path = outRoot_ + "/plants/plants.xml";
        std::ofstream p(path);
        if(!openOk(p, path))
            ok = false;
        else
        {
            p << "<plants>\n</plants>\n";
            if(!closeOk(p, path))
                ok = false;
        }
    }
    if(!ensureDir(outRoot_ + "/crafting"))
        ok = false;
    else
    {
        const std::string path = outRoot_ + "/crafting/recipes.xml";
        std::ofstream c(path);
        if(!openOk(c, path))
            ok = false;
        else
        {
            c << "<recipes>\n</recipes>\n";
            if(!closeOk(c, path))
                ok = false;
        }
    }
    return ok;
}

bool WorldWriter::writeAll()
{
    const int starter = chooseStarterMonster();
    const int captureItem = chooseCaptureItem();

    // run every writer even after a failure (report all problems in one pass),
    // but propagate any failure so main exits non-zero
    bool ok = true;
    if(!writePlayerSkin())
        ok = false;
    if(!writePlayerStart(starter, captureItem))
        ok = false;
    if(!writeReputation())
        ok = false;
    if(!writeEvent())
        ok = false;
    if(!writeLayers(captureItem))
        ok = false;
    if(!writeVisualCategory())
        ok = false;
    if(!writeFightBackgrounds())
        ok = false;
    if(!writeMusic())
        ok = false;
    if(!writeRegionInfo())
        ok = false;
    if(!writeRegionStart())
        ok = false;
    if(!writeEmptyPlantsAndCrafting())
        ok = false;

    std::cerr << "World: starter monster=" << starter << " captureItem=" << captureItem
              << " start=" << startMap_ << " (" << startX_ << "," << startY_ << ")" << std::endl;
    return ok;
}

} // namespace tuxemon
