// test/testingoverworld/main.cpp — monster overworld sheet unit test.
//
// Exercises the REAL client/libqtcatchchallenger/maprender/MonsterSheet.cpp, which
// owns the two accepted layouts of monsters/<id>/overworld.png:
//
//   skin layout    48x96, 3 columns x 4 rows of 16x24 — the same sheet as
//                  skin/<x>/trainer.png, so a trainer.png can be copied over as-is;
//                  rows top/right/bottom/left, columns walk/idle/walk (3 frames)
//   monster layout 64x128, 2 columns x 4 rows of 32x32 — top-walk/left-walk,
//                  top-idle/left-idle, bottom-walk/right-walk, bottom-idle/right-idle
//                  (2 frames)
//
// For each layout it asserts the geometry, then walks every direction and every
// animation step exactly like the renderer does (MapVisualiserPlayer::moveStepSlot,
// MapControllerMPMove) and checks that EVERY frame of the sheet is reached — a
// dropped frame is the regression this test exists for.
//
// The tiles it selects are blitted the way MapRenderer::drawTile does (the source
// rect is Tile::imageRect(), Tile::image() is the WHOLE sheet) into one contact
// sheet, which testingoverworld.py then compares pixel by pixel against the
// reference test/overworld-test.png. So the test catches BOTH a wrong index and a
// wrong sprite on screen.
//
// No network, no event loop exec, no DB: it reads two PNGs and exits.

#include "../../client/libqtcatchchallenger/maprender/MonsterSheet.hpp"
#include "../../client/libqtcatchchallenger/libtiled/tileset.h"
#include "../../client/libqtcatchchallenger/libtiled/tile.h"

#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QRect>
#include <QString>
#include <iostream>
#include <string>
#include <vector>

//background of the contact sheet: opaque so the comparison never depends on how a
//semi-transparent pixel happens to be composited
#define BACKGROUND_COLOR qRgb(72,72,72)
//columns drawn per direction: idle, then the two walk frames (the monster layout
//only has one, it is simply drawn twice)
#define DRAWN_COLUMNS 3

static int failures=0;

//the harness (testingoverworld.py) turns every "PASS <name> <detail>" / "FAIL <name>
//<detail>" line into one test case, so <name> must not contain a space
static void check(const bool &ok,const std::string &name,const std::string &detail)
{
    if(ok)
        std::cout << "PASS " << name << " " << detail << std::endl;
    else
    {
        std::cout << "FAIL " << name << " " << detail << std::endl;
        failures++;
    }
}

struct DirectionEntry
{
    CatchChallenger::Direction look;
    CatchChallenger::Direction move;
    const char *name;
};

static const DirectionEntry directions[4]={
    {CatchChallenger::Direction_look_at_top,   CatchChallenger::Direction_move_at_top,   "top"},
    {CatchChallenger::Direction_look_at_right, CatchChallenger::Direction_move_at_right, "right"},
    {CatchChallenger::Direction_look_at_bottom,CatchChallenger::Direction_move_at_bottom,"bottom"},
    {CatchChallenger::Direction_look_at_left,  CatchChallenger::Direction_move_at_left,  "left"}
};

//Tile::image() is the WHOLE sheet and the source rect is Tile::imageRect(): blit the
//way MapRenderer::drawTile() does, a bare drawPixmap() would paint the whole sheet
static void drawTile(QPainter &painter,const int &x,const int &y,const Tiled::Tile * const tile)
{
    const QRect source=tile->imageRect();
    painter.drawPixmap(x,y,tile->image(),source.x(),source.y(),source.width(),source.height());
}

//load one sheet, check it, and paint its contact block at blockX of the destination
static void verifySheet(QPainter &painter,const int &blockX,const std::string &prefix,
                        const QString &path,
                        const int &expectedColumns,const int &expectedTileWidth,
                        const int &expectedTileHeight,const qreal &expectedXOffset)
{
    std::cout << "== " << path.toStdString() << std::endl;
    const QImage image(path);
    check(!image.isNull(),prefix+"_load",path.toStdString());
    if(!image.isNull())
    {
        Tiled::SharedTileset tileset=MonsterSheet::create(QStringLiteral("overworld"),image);
        check(tileset->loadFromImage(image,path),prefix+"_loadFromImage","");
        check(tileset->tileWidth()==expectedTileWidth && tileset->tileHeight()==expectedTileHeight,
              prefix+"_tilesize",
              std::to_string(tileset->tileWidth())+"x"+std::to_string(tileset->tileHeight())+
              " expected "+std::to_string(expectedTileWidth)+"x"+std::to_string(expectedTileHeight));
        check(tileset->columnCount()==expectedColumns,prefix+"_columns",
              std::to_string(tileset->columnCount())+" expected "+std::to_string(expectedColumns));
        check(MonsterSheet::isSkinLayout(tileset.data())==(expectedColumns==DRAWN_COLUMNS),
              prefix+"_layout",expectedColumns==DRAWN_COLUMNS?"skin":"monster");
        check(MonsterSheet::xOffset(tileset.data())==expectedXOffset,prefix+"_xoffset",
              std::to_string(MonsterSheet::xOffset(tileset.data()))+
              " expected "+std::to_string(expectedXOffset));

        const int tileCount=tileset->tileCount();
        std::vector<bool> used(tileCount,false);
        int d=0;
        while(d<4)
        {
            //a look and the matching move must land on the same standing frame
            const int base=MonsterSheet::baseTile(tileset.data(),directions[d].look);
            check(base==MonsterSheet::baseTile(tileset.data(),directions[d].move),
                  prefix+"_"+directions[d].name+"_lookeqmove","");
            check(base>=0 && base<tileCount,prefix+"_"+directions[d].name+"_idle",
                  "tile "+std::to_string(base));
            if(base>=0 && base<tileCount)
            {
                used[base]=true;
                drawTile(painter,blockX,d*tileset->tileHeight(),tileset->tileAt(base));

                //the renderer flips stepAlternance on every transition step
                bool stepAlternance=false;
                int column=1;
                while(column<DRAWN_COLUMNS)
                {
                    const int walk=MonsterSheet::walkTile(tileset.data(),base,stepAlternance);
                    check(walk>=0 && walk<tileCount,
                          prefix+"_"+directions[d].name+"_walk"+std::to_string(column),
                          "tile "+std::to_string(walk));
                    check(walk!=base,prefix+"_"+directions[d].name+"_walk"+std::to_string(column)+"_noteqidle","");
                    if(walk>=0 && walk<tileCount)
                    {
                        used[walk]=true;
                        drawTile(painter,blockX+column*tileset->tileWidth(),
                                 d*tileset->tileHeight(),tileset->tileAt(walk));
                    }
                    stepAlternance=!stepAlternance;
                    column++;
                }
            }
            d++;
        }

        //the whole point: no frame of the sheet may be unreachable
        int reached=0;
        int i=0;
        while(i<tileCount)
        {
            if(used.at(i))
                reached++;
            i++;
        }
        check(reached==tileCount,prefix+"_allframesused",
              std::to_string(reached)+"/"+std::to_string(tileCount)+" frames reachable");
    }
}

int main(int argc,char *argv[])
{
    if(argc!=3)
    {
        std::cerr << "usage: " << argv[0] << " <datapack-root> <output.png>" << std::endl;
        return 2;
    }
    //no display needed, this only paints into a QImage
    qputenv("QT_QPA_PLATFORM","offscreen");
    QGuiApplication app(argc,argv);
    const QString datapack=QString::fromUtf8(argv[1]);
    const QString output=QString::fromUtf8(argv[2]);

    //skin block is 3x16 wide and 4x24 tall, monster block 3x32 and 4x32
    QImage sheet(DRAWN_COLUMNS*16+DRAWN_COLUMNS*32,4*32,QImage::Format_RGB32);
    sheet.fill(BACKGROUND_COLOR);
    QPainter painter(&sheet);
    //a skin sheet used as-is for a monster: this is the layout that must keep 3 frames
    verifySheet(painter,0,"skin",datapack+"/skin/animals/conileaf/trainer.png",3,16,24,0.0);
    //the historic monster sheet: must behave exactly as it always did
    verifySheet(painter,DRAWN_COLUMNS*16,"monster",datapack+"/monsters/57/overworld.png",2,32,32,-0.5);
    painter.end();

    if(!sheet.save(output))
    {
        std::cerr << "cannot write " << output.toStdString() << std::endl;
        return 2;
    }
    std::cout << "contact sheet written: " << output.toStdString() << std::endl;
    if(failures==0)
        std::cout << "ALL PASS" << std::endl;
    else
        std::cout << "FAILURES: " << failures << std::endl;
    return failures==0?0:1;
}
