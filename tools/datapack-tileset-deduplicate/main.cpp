// datapack-tileset-deduplicate — find identical/similar 16x16 tiles across a
// datapack's tilesets, merge them, and rewrite every .tmx so its cells point
// at the kept tile.
//
// Usage: datapack-tileset-deduplicate <datapack_path>
// With no argument a directory picker is shown.

#include "TileDeduplicator.hpp"

#include <QApplication>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QStringList>
#include <iostream>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("datapack-tileset-deduplicate"));

    QStringList arguments = QCoreApplication::arguments();
    if(arguments.contains(QStringLiteral("--help")) || arguments.contains(QStringLiteral("-h")))
    {
        std::cout << "Usage: " << argv[0] << " [--batch] [--check-all] [--reset-skips] <datapack_path>" << std::endl;
        std::cout << "Compares 16x16 tiles ACROSS tilesets; auto-merges identical tiles," << std::endl;
        std::cout << "asks about similar ones, then rewrites all .tmx references." << std::endl;
        std::cout << "  --batch        headless: auto-merge identical, keep similar, no window." << std::endl;
        std::cout << "  --check-all    also compare tiles within the same tileset file (can generate problem with animation, then where into same tileset the tile are very similar)." << std::endl;
        std::cout << "  --reset-skips  forget all previously SKIP-ped tiles (clears the SKIP lines in <datapack>/tile-dedup.log) so they are asked about again." << std::endl;
        std::cout << "  --migrate-from <a.tsx> --migrate-to <b.tsx>  rewrite every map to use b.tsx instead of a.tsx (same tile ids), then delete a.tsx and its image. No deduplication is run." << std::endl;
        std::cout << "  --stat         list every .tsx under the path with how many tile cells use it (repeats counted) and in how many maps; prints to stdout and exits." << std::endl;
        std::cout << "  --remove <a.tsx>  delete a.tsx and its image, clear every cell that used it from all maps, and drop its <tileset> entry. No deduplication is run." << std::endl;
        return 0;
    }

    bool batch = arguments.removeAll(QStringLiteral("--batch")) > 0;
    bool checkAll = arguments.removeAll(QStringLiteral("--check-all")) > 0;
    bool resetSkips = arguments.removeAll(QStringLiteral("--reset-skips")) > 0;
    bool stat = arguments.removeAll(QStringLiteral("--stat")) > 0;

    // --migrate-from <path> / --migrate-to <path> (value is the following argument).
    QString migrateFrom;
    QString migrateTo;
    {
        int idx = arguments.indexOf(QStringLiteral("--migrate-from"));
        if(idx >= 0 && idx + 1 < arguments.size())
        {
            migrateFrom = arguments.at(idx + 1);
            arguments.removeAt(idx + 1);
            arguments.removeAt(idx);
        }
    }
    {
        int idx = arguments.indexOf(QStringLiteral("--migrate-to"));
        if(idx >= 0 && idx + 1 < arguments.size())
        {
            migrateTo = arguments.at(idx + 1);
            arguments.removeAt(idx + 1);
            arguments.removeAt(idx);
        }
    }
    const bool migrate = !migrateFrom.isEmpty() || !migrateTo.isEmpty();
    if(migrate && (migrateFrom.isEmpty() || migrateTo.isEmpty()))
    {
        std::cerr << "Both --migrate-from and --migrate-to are required." << std::endl;
        return 1;
    }

    QString removeTsx;
    {
        int idx = arguments.indexOf(QStringLiteral("--remove"));
        if(idx >= 0 && idx + 1 < arguments.size())
        {
            removeTsx = arguments.at(idx + 1);
            arguments.removeAt(idx + 1);
            arguments.removeAt(idx);
        }
    }

    QString datapackPath;
    if(arguments.size() > 1)
        datapackPath = arguments.at(1);
    else if(stat)
        datapackPath = QDir::currentPath();   // --stat with no path: the current directory
    else if(migrate)
        // No datapack path given: scan from the parent of the migrate-from tileset's
        // directory (e.g. .../datapack/map/x.tsx -> .../datapack), which contains the maps.
        datapackPath = QFileInfo(QFileInfo(migrateFrom).absolutePath()).absolutePath();
    else if(!removeTsx.isEmpty())
        datapackPath = QFileInfo(QFileInfo(removeTsx).absolutePath()).absolutePath();
    else if(!batch)
        datapackPath = QFileDialog::getExistingDirectory(nullptr, QStringLiteral("Select the datapack folder"));

    if(datapackPath.isEmpty())
        return 0;
    if(!QFileInfo(datapackPath).isDir())
    {
        std::cerr << "Not a directory: " << datapackPath.toStdString() << std::endl;
        return 1;
    }

    TileDeduplicator w(datapackPath, batch, checkAll, resetSkips, stat, migrateFrom, migrateTo, removeTsx);
    if(!batch && !migrate && !stat && removeTsx.isEmpty())
        w.show();
    return a.exec();
}
