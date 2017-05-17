#include <QApplication>
#include <QCoreApplication>

#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/base/render/MapVisualiserThread.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
#include "../../client/base/Map_client.h"
#include "../../general/base/Map_loader.h"

#include "../../client/tiled/tiled_isometricrenderer.h"
#include "../../client/tiled/tiled_map.h"
#include "../../client/tiled/tiled_mapobject.h"
#include "../../client/tiled/tiled_mapreader.h"
#include "../../client/tiled/tiled_mapwriter.h"
#include "../../client/tiled/tiled_objectgroup.h"
#include "../../client/tiled/tiled_orthogonalrenderer.h"
#include "../../client/tiled/tiled_tilelayer.h"
#include "../../client/tiled/tiled_tileset.h"

#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
extern "C" {
const char* __asan_default_options() { return "alloc_dealloc_mismatch=0:detect_container_overflow=0"; }
}  // extern "C"
#  endif
#endif

int main(int argc, char *argv[])
{
    // Avoid performance issues with X11 engine when rendering objects
#ifdef Q_WS_X11
    QApplication::setGraphicsSystem(QStringLiteral("raster"));
#endif

    QApplication a(argc, argv);

    a.setOrganizationDomain(QStringLiteral("catchchallenger"));
    a.setApplicationName(QStringLiteral("map2png"));
    a.setApplicationVersion(QStringLiteral("1.0"));

    Tiled::Map tiledMap(Tiled::Map::Orientation::Orthogonal,42,42,16,16);
    Tiled::MapWriter maprwriter;
    maprwriter.writeMap(&tiledMap,"test.tmx");

    return 0;
}
