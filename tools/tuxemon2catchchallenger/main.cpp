// tuxemon2catchchallenger — convert the Tuxemon game data into a CatchChallenger
// datapack.  Usage:
//   tuxemon2catchchallenger <mods/tuxemon dir> <output datapack dir>
//
// Reads the Tuxemon YAML db + sprites + gettext catalogues and writes the
// CatchChallenger types/skills/buffs/monsters/items XML and monster sprites.

#include "TuxemonDb.hpp"
#include "Localization.hpp"
#include "DatapackWriter.hpp"
#include "MapConverter.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QString>

#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    if(argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <mods/tuxemon dir> <output datapack dir>" << std::endl;
        std::cerr << "  e.g. " << argv[0]
                  << " /home/user/Desktop/CatchChallenger/old/Tuxemon/mods/tuxemon"
                  << " /home/user/Desktop/CatchChallenger/Tuxemon-datapack" << std::endl;
        return 1;
    }

    const std::string modRoot = QDir(QString::fromUtf8(argv[1])).absolutePath().toStdString();
    const std::string outRoot = QDir(QString::fromUtf8(argv[2])).absolutePath().toStdString();

    if(!QDir(QString::fromStdString(modRoot)).exists())
    {
        std::cerr << "Input mod directory does not exist: " << modRoot << std::endl;
        return 1;
    }
    if(!QDir(QString::fromStdString(modRoot + "/db")).exists())
    {
        std::cerr << "Input directory has no db/ subfolder: " << modRoot << std::endl;
        return 1;
    }

    tuxemon::TuxemonDb db;
    if(!db.load(modRoot))
    {
        std::cerr << "Failed to load the Tuxemon db." << std::endl;
        return 1;
    }

    tuxemon::Localization l10n;
    l10n.load(modRoot);

    QDir().mkpath(QString::fromStdString(outRoot));
    tuxemon::DatapackWriter writer(db, l10n, modRoot, outRoot);
    if(!writer.writeAll())
    {
        std::cerr << "Failed to write the datapack." << std::endl;
        return 1;
    }

    tuxemon::MapConverter maps(modRoot, outRoot);
    maps.convertAll();

    std::cout << "Done." << std::endl;
    return 0;
}
