#include "WorldWriter.hpp"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QImage>
#include <QPainter>

#include <fstream>
#include <iostream>

namespace tuxemon {

WorldWriter::WorldWriter(const TuxemonDb &db, const Localization &l10n,
                         const std::string &modRoot, const std::string &outRoot,
                         SkinGen &skins, const DatapackWriter &dw,
                         const std::string &startMap, int startX, int startY)
    : db_(db), l10n_(l10n), modRoot_(modRoot), outRoot_(outRoot), skins_(skins),
      dw_(dw), startMap_(startMap), startX_(startX), startY_(startY)
{
}

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

void WorldWriter::writePlayerSkin()
{
    // player overworld+battle sprite per mod.yaml (sprite/combat_sheet = adventurer)
    skins_.ensure("fighter", "player", "adventurer", "adventurer");
}

void WorldWriter::writePlayerStart(int starterId, int captureItemId)
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/player"));
    std::ofstream o(outRoot_ + "/player/start.xml");
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
}

void WorldWriter::writeReputation()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/player"));
    std::ofstream o(outRoot_ + "/player/reputation.xml");
    o << "<reputations>\n";
    o << "    <reputation type=\"nation\">\n";
    o << "        <name>Nation</name>\n";
    o << "        <name lang=\"fr\">Nation</name>\n";
    o << "        <level point=\"-500\"><text>Outlaw</text></level>\n";
    o << "        <level point=\"-100\"><text>Suspect</text></level>\n";
    o << "        <level point=\"0\"><text>Citizen</text></level>\n";
    o << "        <level point=\"100\"><text>Trusted</text></level>\n";
    o << "        <level point=\"500\"><text>Hero</text></level>\n";
    o << "    </reputation>\n";
    o << "</reputations>\n";
}

void WorldWriter::writeEvent()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/player"));
    std::ofstream o(outRoot_ + "/player/event.xml");
    o << "<events>\n";
    o << "    <event id=\"day\">\n";
    o << "        <value>day</value>\n";
    o << "        <value>night</value>\n";
    o << "    </event>\n";
    o << "</events>\n";
}

void WorldWriter::writeLayers(int captureItemId)
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map"));
    std::ofstream o(outRoot_ + "/map/layers.xml");
    o << "<layers zoom=\"4\">\n";
    o << "    <monstersCollision monsterType=\"grass\" background=\"fight/grass/\" layer=\"Grass\" type=\"walkOn\">\n";
    o << "        <event id=\"day\" value=\"night\" monsterType=\"grassNight;grass\"/>\n";
    o << "    </monstersCollision>\n";
    o << "    <monstersCollision monsterType=\"cave\" background=\"fight/cave/\" type=\"walkOn\"/>\n";
    o << "    <monstersCollision item=\"" << captureItemId << "\" tile=\"swim\" background=\"fight/water/\" layer=\"Water\" type=\"walkOn\" monsterType=\"water\"/>\n";
    o << "</layers>\n";
}

void WorldWriter::writeVisualCategory()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map"));
    std::ofstream o(outRoot_ + "/map/visualcategory.xml");
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
static void writePlatform(const QString &path, int w, int h)
{
    QImage img(w, h, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawEllipse(2, 2, w - 4, h - 4);
    p.end();
    img.save(path, "PNG");
}

void WorldWriter::writeFightBackgrounds()
{
    const QString combat = QString::fromStdString(modRoot_ + "/gfx/ui/combat");
    // (terrain folder, preferred Tuxemon background names)
    struct Bg { const char *dir; QStringList pref; };
    Bg terrains[] = {
        {"grass",      QStringList() << "grass_background.png" << "forest_background.png" << "meadow_background.png"},
        {"grassnight", QStringList() << "grass_background.png" << "forest_background.png"},
        {"cave",       QStringList() << "cave_background.png" << "cavern_background.png"},
        {"water",      QStringList() << "beach_background.png" << "ocean_background.png" << "lake_background.png"},
    };
    int i = 0;
    while(i < 4)
    {
        const std::string dir = outRoot_ + "/map/fight/" + terrains[i].dir;
        QDir().mkpath(QString::fromStdString(dir));
        const QString bg = pickCombatBg(combat, terrains[i].pref);
        if(!bg.isEmpty())
        {
            const QString dst = QString::fromStdString(dir) + "/background.png";
            QFile::remove(dst);
            QFile::copy(bg, dst);
        }
        writePlatform(QString::fromStdString(dir) + "/plateform-background.png", 160, 48);
        writePlatform(QString::fromStdString(dir) + "/plateform-front.png", 160, 48);
        ++i;
    }
    // grass also has a foreground overlay in the official datapack
    QImage fg(16, 16, QImage::Format_ARGB32);
    fg.fill(Qt::transparent);
    fg.save(QString::fromStdString(outRoot_ + "/map/fight/grass/foreground.png"), "PNG");
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

void WorldWriter::writeMusic()
{
    const std::string musicDir = outRoot_ + "/music";
    QDir().mkpath(QString::fromStdString(musicDir));
    const std::string src = modRoot_ + "/music/";

    // (CatchChallenger type, Tuxemon source, output)
    const char *map[][3] = {
        {"outdoor", "JRPG_town_loop.ogg", "outdoor.opus"},
        {"city",    "JRPG_royalCourt_loop.ogg", "city.opus"},
        {"indoor",  "JRPG_princess.ogg", "indoor.opus"},
        {"cave",    "HHavok-main.ogg", "cave.opus"},
        {"battle",  "JRPG_winBattle.ogg", "battle.opus"},
    };
    int ok = 0;
    int i = 0;
    while(i < 5)
    {
        if(transcode(QString::fromStdString(src + map[i][1]),
                     QString::fromStdString(musicDir + "/" + map[i][2])))
            ++ok;
        ++i;
    }
    std::cerr << "Music: " << ok << "/5 tracks transcoded." << std::endl;

    std::ofstream o(outRoot_ + "/map/music.xml");
    o << "<musics>\n";
    o << "    <map type=\"city\">music/city.opus</map>\n";
    o << "    <map type=\"indoor\">music/indoor.opus</map>\n";
    o << "    <map type=\"outdoor\">music/outdoor.opus</map>\n";
    o << "    <map type=\"cave\">music/cave.opus</map>\n";
    o << "</musics>\n";
}

void WorldWriter::writeRegionInfo()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map/main/tuxemon"));
    std::ofstream o(outRoot_ + "/map/main/tuxemon/informations.xml");
    o << "<informations color=\"#4FD9FF\">\n";
    o << "    <author pseudo=\"Tuxemon\" email=\"\" name=\"Tuxemon project\"/>\n";
    o << "    <name>Tuxemon world</name>\n";
    o << "    <name lang=\"fr\">Monde Tuxemon</name>\n";
    o << "    <description>Maps converted from Tuxemon (https://www.tuxemon.org/)</description>\n";
    o << "    <initial>T</initial>\n";
    o << "</informations>\n";
}

void WorldWriter::writeRegionStart()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/map/main/tuxemon"));
    std::ofstream o(outRoot_ + "/map/main/tuxemon/start.xml");
    o << "<profile>\n";
    o << "    <start id=\"Normal\">\n";
    o << "        <map x=\"" << startX_ << "\" y=\"" << startY_ << "\" file=\"" << startMap_ << "\"/>\n";
    o << "    </start>\n";
    o << "</profile>\n";
}

void WorldWriter::writeEmptyPlantsAndCrafting()
{
    QDir().mkpath(QString::fromStdString(outRoot_ + "/plants"));
    std::ofstream p(outRoot_ + "/plants/plants.xml");
    p << "<plants>\n</plants>\n";
    QDir().mkpath(QString::fromStdString(outRoot_ + "/crafting"));
    std::ofstream c(outRoot_ + "/crafting/recipes.xml");
    c << "<recipes>\n</recipes>\n";
}

void WorldWriter::writeAll()
{
    const int starter = chooseStarterMonster();
    const int captureItem = chooseCaptureItem();

    writePlayerSkin();
    writePlayerStart(starter, captureItem);
    writeReputation();
    writeEvent();
    writeLayers(captureItem);
    writeVisualCategory();
    writeFightBackgrounds();
    writeMusic();
    writeRegionInfo();
    writeRegionStart();
    writeEmptyPlantsAndCrafting();

    std::cerr << "World: starter monster=" << starter << " captureItem=" << captureItem
              << " start=" << startMap_ << " (" << startX_ << "," << startY_ << ")" << std::endl;
}

} // namespace tuxemon
