// tileset-tagger — tag a Tiled .tsx with semantic categories (table, tree-canopy,
// building-wall, ledge-down, ...) used by the learn-from-examples map generator.
// Tags live as per-tile <property> entries inside the .tsx.  GUARD: any tile that
// draws pixels but has no category is reported, so nothing is left untagged.
//
//   tileset-tagger --guard   <x.tsx>
//   tileset-tagger --tag     <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]
//   tileset-tagger --selftest <x.tsx>
//   tileset-tagger [<x.tsx>]                  # launch the GUI (default)
//
// The GUI (rectangle-select tagging) is built on this exact TagModel core.

#include "TagModel.hpp"
#include "MainWindow.hpp"
#include "MapUsageIndex.hpp"

#include <QApplication>
#include <QFile>
#include <QString>
#include <QStringList>
#include <iostream>
#include <map>
#include <string>
#include <vector>

static void printUntagged(const TagModel &model)
{
    const std::vector<int> untagged=model.untaggedNonEmpty();
    std::cout << "GUARD untagged-tiles: " << untagged.size() << " tile(s) draw pixels but have no category";
    if(!untagged.empty())
    {
        std::cout << " ->";
        size_t i=0;
        while(i<untagged.size() && i<40) { std::cout << " " << untagged.at(i); i++; }
        if(untagged.size()>40)
            std::cout << " ...";
    }
    std::cout << std::endl;
}

static int runGuard(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    std::cout << tsx.toStdString() << ": " << model.tileCount() << " tiles, "
              << model.columns() << " columns, " << model.tileWidth() << "x" << model.tileHeight() << std::endl;
    printUntagged(model);
    return 0;
}

static int runTag(const QStringList &args)
{
    // args: <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]
    if(args.size()<6) { std::cerr << "tag needs: <x.tsx> <category> <col0> <row0> <col1> <row1> [name] [size]" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const std::string category=args.at(1).toStdString();
    const int c0=args.at(2).toInt(),r0=args.at(3).toInt(),c1=args.at(4).toInt(),r1=args.at(5).toInt();
    std::map<std::string,std::string> attrs;
    if(args.size()>6 && !args.at(6).isEmpty()) attrs["group"]=args.at(6).toStdString();
    if(args.size()>7 && !args.at(7).isEmpty()) attrs["size"]=args.at(7).toStdString();
    const std::vector<int> ids=model.tilesInRect(c0,r0,c1,r1);
    model.tagTiles(ids,category,attrs);
    if(!model.save()) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    std::cout << "tagged " << ids.size() << " tile(s) as '" << category << "' in " << args.at(0).toStdString() << std::endl;
    printUntagged(model);
    return 0;
}

static int runSelftest(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << "selftest load FAIL: " << model.error().toStdString() << std::endl; return 1; }
    const size_t before=model.untaggedNonEmpty().size();
    std::cout << "selftest: loaded " << model.tileCount() << " tiles, untagged-non-empty=" << before << std::endl;
    // tag 4 currently-UNTAGGED non-empty tiles so the guard count drops by exactly 4
    const std::vector<int> untagged=model.untaggedNonEmpty();
    std::vector<int> pick;
    size_t pi=0;
    while(pi<untagged.size() && pick.size()<4) { pick.push_back(untagged.at(pi)); pi++; }
    if(pick.empty()) { std::cout << "selftest: no untagged tiles with pixels (skip tag round-trip)" << std::endl; return 0; }
    std::map<std::string,std::string> attrs;
    attrs["group"]="table-0";
    attrs["size"]="2x2";
    attrs["horizontalRepeat"]="true";
    // tags go to the SIDECAR (out of the datapack) — back up any real tag file and
    // restore it so the selftest never disturbs the user's tags.
    const QString tagFile=model.tagFilePath();
    const bool had=QFile::exists(tagFile);
    QByteArray backup;
    if(had) { QFile f(tagFile); if(f.open(QIODevice::ReadOnly)) { backup=f.readAll(); f.close(); } }
    std::cout << "selftest: sidecar " << tagFile.toStdString() << std::endl;

    model.tagTiles(pick,"selftest-table",attrs);
    if(!model.save()) { std::cerr << "selftest save FAIL: " << model.error().toStdString() << std::endl; return 1; }
    // reload from the sidecar and verify tags persisted + the guard count dropped
    TagModel reload;
    if(!reload.load(tsx)) { std::cerr << "selftest reload FAIL: " << reload.error().toStdString() << std::endl; return 1; }
    bool ok=true;
    size_t k=0;
    while(k<pick.size())
    {
        if(reload.tagOf(pick.at(k)).category!="selftest-table") { ok=false; std::cerr << "  tile " << pick.at(k) << " lost its tag" << std::endl; }
        k++;
    }
    const size_t after=reload.untaggedNonEmpty().size();

    // restore the datapack sidecar to its pre-test state
    if(had) { QFile f(tagFile); if(f.open(QIODevice::WriteOnly|QIODevice::Truncate)) { f.write(backup); f.close(); } }
    else QFile::remove(tagFile);

    const bool dropOk = pick.size()<=before && after==before-pick.size();
    std::cout << "selftest: round-trip tags " << (ok?"PERSIST":"LOST") << "; untagged " << before << " -> " << after
              << " (expected -" << pick.size() << "); .tsx untouched" << std::endl;
    return (ok && dropOk) ? 0 : 1;
}

static int runUsage(const QStringList &args)
{
    // args: <x.tsx> <col0> <row0> <col1> <row1>
    if(args.size()<5) { std::cerr << "usage needs: <x.tsx> <c0> <r0> <c1> <r1>" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const std::vector<int> ids=model.tilesInRect(args.at(1).toInt(),args.at(2).toInt(),args.at(3).toInt(),args.at(4).toInt());
    MapUsageIndex index;
    index.build(args.at(0));
    std::cout << index.candidateCount() << " map(s) reference " << args.at(0).toStdString() << std::endl;
    const std::vector<MapUsageIndex::Usage> usages=index.findUsages(ids);
    std::cout << "group of " << ids.size() << " tile(s) is used on " << usages.size() << " map(s):" << std::endl;
    size_t i=0;
    while(i<usages.size() && i<30)
    {
        const MapUsageIndex::Usage &u=usages.at(i);
        std::cout << "  " << u.mapLabel.toStdString() << "  " << u.cells.size() << " cell(s)  ("
                  << u.mapW << "x" << u.mapH << ")" << std::endl;
        i++;
    }
    // PRE-FILL inference data: which engine layer dominates + same-tile runs.
    const MapUsageIndex::GroupStats &st=index.lastStats();
    std::string domLayer;
    int best=-1;
    std::map<std::string,int>::const_iterator li=st.layerCounts.begin();
    while(li!=st.layerCounts.cend())
    {
        if(li->second>best) { best=li->second; domLayer=li->first; }
        ++li;
    }
    if(st.totalCells>0)
    {
        std::cout << "prefill: drawn-on layer '" << domLayer << "'  ("
                  << (best*100/st.totalCells) << "% of " << st.totalCells << " cells)"
                  << "  hRepeat=" << (st.horizontalRepeatCells*100/st.totalCells) << "%"
                  << "  vRepeat=" << (st.verticalRepeatCells*100/st.totalCells) << "%" << std::endl;
        std::cout << "effective (engine precedence): walkable=" << (st.walkableCells*100/st.totalCells)
                  << "%  blocked=" << (st.blockedCells*100/st.totalCells)
                  << "%  ledge=" << (st.ledgeCells*100/st.totalCells) << "%"
                  << "  -> " << ((st.walkableCells+st.ledgeCells>=st.blockedCells)?"WALKABLE":"BLOCKED") << std::endl;
    }
    if(!usages.empty())
    {
        // exercise the compositor on the top map (what the GUI would render)
        const QImage img=index.render(usages.at(0).mapPath);
        std::cout << "render(" << usages.at(0).mapLabel.toStdString() << ") -> "
                  << img.width() << "x" << img.height() << " px"
                  << (img.isNull() ? " [NULL]" : "") << std::endl;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    QStringList args;
    int i=1;
    while(i<argc) { args.append(QString::fromUtf8(argv[i])); i++; }
    const bool cli = !args.isEmpty()
        && (args.at(0)=="--guard" || args.at(0)=="--tag" || args.at(0)=="--selftest" || args.at(0)=="--usage");
    if(cli)
        qputenv("QT_QPA_PLATFORM","offscreen"); //headless: no display needed

    QApplication app(argc,argv);

    if(!args.isEmpty() && args.at(0)=="--guard" && args.size()>=2)
        return runGuard(args.at(1));
    if(!args.isEmpty() && args.at(0)=="--tag")
        return runTag(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--selftest" && args.size()>=2)
        return runSelftest(args.at(1));
    if(!args.isEmpty() && args.at(0)=="--usage")
        return runUsage(args.mid(1));

    // GUI: optional first arg is a .tsx to open.
    MainWindow window;
    if(!args.isEmpty() && !args.at(0).startsWith("--"))
        window.openTsx(args.at(0));
    window.show();
    return app.exec();
}
