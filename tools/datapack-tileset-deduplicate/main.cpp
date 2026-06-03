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
        std::cout << "Usage: " << argv[0] << " [--batch] [--check-all] <datapack_path>" << std::endl;
        std::cout << "Compares 16x16 tiles ACROSS tilesets; auto-merges identical tiles," << std::endl;
        std::cout << "asks about similar ones, then rewrites all .tmx references." << std::endl;
        std::cout << "  --batch      headless: auto-merge identical, keep similar, no window." << std::endl;
        std::cout << "  --check-all  also compare tiles within the same tileset file (can generate problem with animation, then where into same tileset the tile are very similar)." << std::endl;
        return 0;
    }

    bool batch = arguments.removeAll(QStringLiteral("--batch")) > 0;
    bool checkAll = arguments.removeAll(QStringLiteral("--check-all")) > 0;

    QString datapackPath;
    if(arguments.size() > 1)
        datapackPath = arguments.at(1);
    else if(!batch)
        datapackPath = QFileDialog::getExistingDirectory(nullptr, QStringLiteral("Select the datapack folder"));

    if(datapackPath.isEmpty())
        return 0;
    if(!QFileInfo(datapackPath).isDir())
    {
        std::cerr << "Not a directory: " << datapackPath.toStdString() << std::endl;
        return 1;
    }

    TileDeduplicator w(datapackPath, batch, checkAll);
    if(!batch)
        w.show();
    return a.exec();
}
