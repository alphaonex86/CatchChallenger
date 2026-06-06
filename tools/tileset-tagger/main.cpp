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
#include "MapDecoder.hpp"
#include "MapUsageIndex.hpp"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QString>
#include <QStringList>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>
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

static int suggestOne(const QString &tsx)
{
    TagModel model;
    if(!model.load(tsx)) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    MapUsageIndex index;
    index.build(tsx);
    const std::map<int,MapUsageIndex::TileStat> stats=index.analyzeAllTiles();
    int sure=0,guess=0,alreadyTagged=0;
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        const int id=it->first;
        const MapUsageIndex::TileStat &s=it->second;
        if(!model.tagOf(id).category.empty()) { alreadyTagged++; ++it; continue; }
        std::string norm;
        bool walk=true,high=true;
        const std::string cat=MapUsageIndex::suggestCategory(s,model.tileGreenish(id),norm,walk,high);
        if(cat.empty()) { ++it; continue; }
        std::map<std::string,std::string> attrs;
        attrs["layer"]=norm;
        attrs["walkable"]= walk ? "true" : "false";
        if(!high)
            attrs["auto"]="guess";        // low-confidence -> flag for human review
        std::vector<int> one;
        one.push_back(id);
        model.tagTiles(one,cat,attrs);
        if(high) sure++; else guess++;
        ++it;
    }
    if(!model.save()) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    const size_t remaining=model.untaggedNonEmpty().size();
    std::cout << QFileInfo(tsx).fileName().toStdString() << ": " << sure << " sure + " << guess
              << " to-review auto-tagged (" << index.candidateCount() << " maps), "
              << remaining << " untagged" << std::endl;
    return 0;
}

static int runClassify(const QStringList &args)
{
    // dry-run: show what suggest WOULD tag, without writing anything.
    if(args.isEmpty()) { std::cerr << "classify needs: <x.tsx>" << std::endl; return 1; }
    TagModel model;
    if(!model.load(args.at(0))) { std::cerr << model.error().toStdString() << std::endl; return 1; }
    MapUsageIndex index;
    index.build(args.at(0));
    const std::map<int,MapUsageIndex::TileStat> stats=index.analyzeAllTiles();
    std::map<std::string,int> hist;
    int sure=0,guess=0;
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        std::string norm;
        bool walk=true,high=true;
        const std::string cat=MapUsageIndex::suggestCategory(it->second,model.tileGreenish(it->first),norm,walk,high);
        if(!cat.empty()) { hist[cat]++; if(high) sure++; else guess++; }
        ++it;
    }
    std::cout << QFileInfo(args.at(0)).fileName().toStdString() << ": " << sure << " sure + " << guess << " to-review" << std::endl;
    std::map<std::string,int>::const_iterator h=hist.begin();
    while(h!=hist.cend()) { std::cout << "  " << h->first << ": " << h->second << std::endl; ++h; }
    return 0;
}

// top-N (count,name) pairs of a histogram, descending.
static std::vector<std::pair<long,std::string> > topOf(const std::map<std::string,long> &h,size_t n)
{
    std::vector<std::pair<long,std::string> > v;
    std::map<std::string,long>::const_iterator it=h.begin();
    while(it!=h.cend()) { v.push_back(std::make_pair(it->second,it->first)); ++it; }
    std::sort(v.rbegin(),v.rend());
    if(v.size()>n)
        v.resize(n);
    return v;
}

static int runLearn(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "learn needs: <map-dir> <rules.json>" << std::endl; return 1; }
    std::map<std::string,long> catCount;
    std::map<std::string,std::map<std::string,long> > rightOf,downOf;   // [A][B] = B is right/below A
    long total=0;
    int nmaps=0;
    MapDecoder dec;
    QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
    while(it.hasNext())
    {
        const QString tmx=it.next();
        MapDecoder::Result r;
        QString err;
        if(!dec.decode(tmx,r,err))
            continue;
        nmaps++;
        int y=0;
        while(y<r.h)
        {
            int x=0;
            while(x<r.w)
            {
                const std::string &a=r.categoryGrid.at(x+y*r.w);
                if(!a.empty())
                {
                    catCount[a]++;
                    total++;
                    if(x+1<r.w) { const std::string &b=r.categoryGrid.at((x+1)+y*r.w); if(!b.empty()) rightOf[a][b]++; }
                    if(y+1<r.h) { const std::string &b=r.categoryGrid.at(x+(y+1)*r.w); if(!b.empty()) downOf[a][b]++; }
                }
                x++;
            }
            y++;
        }
    }
    if(total==0) { std::cerr << "learn: no decodable maps under " << args.at(0).toStdString() << std::endl; return 1; }

    // write rules.json
    {
        QJsonObject cats;
        std::map<std::string,long>::const_iterator c=catCount.begin();
        while(c!=catCount.cend()) { cats.insert(QString::fromStdString(c->first),(double)c->second); ++c; }
        QJsonObject jright,jdown;
        std::map<std::string,std::map<std::string,long> >::const_iterator a=rightOf.begin();
        while(a!=rightOf.cend())
        {
            QJsonObject row;
            std::map<std::string,long>::const_iterator b=a->second.begin();
            while(b!=a->second.cend()) { row.insert(QString::fromStdString(b->first),(double)b->second); ++b; }
            jright.insert(QString::fromStdString(a->first),row);
            ++a;
        }
        a=downOf.begin();
        while(a!=downOf.cend())
        {
            QJsonObject row;
            std::map<std::string,long>::const_iterator b=a->second.begin();
            while(b!=a->second.cend()) { row.insert(QString::fromStdString(b->first),(double)b->second); ++b; }
            jdown.insert(QString::fromStdString(a->first),row);
            ++a;
        }
        QJsonObject root;
        root.insert("maps",nmaps);
        root.insert("cells",(double)total);
        root.insert("categories",cats);
        root.insert("rightOf",jright);
        root.insert("downOf",jdown);
        QFile out(args.at(1));
        if(!out.open(QIODevice::WriteOnly|QIODevice::Truncate)) { std::cerr << "cannot write " << args.at(1).toStdString() << std::endl; return 1; }
        out.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        out.close();
    }

    // human-readable summary (sanity check the learned rules)
    std::cout << "learned from " << nmaps << " maps, " << total << " cells, "
              << catCount.size() << " categories -> " << args.at(1).toStdString() << std::endl;
    const std::vector<std::pair<long,std::string> > top=topOf(catCount,12);
    std::cout << "top categories:";
    size_t i=0;
    while(i<top.size()) { std::cout << " " << top.at(i).second << "(" << (top.at(i).first*100/total) << "%)"; i++; }
    std::cout << std::endl;
    std::cout << "dominant neighbours (right → / down ↓):" << std::endl;
    i=0;
    while(i<top.size())
    {
        const std::string &cat=top.at(i).second;
        const std::vector<std::pair<long,std::string> > r=topOf(rightOf[cat],3);
        const std::vector<std::pair<long,std::string> > d=topOf(downOf[cat],3);
        std::cout << "  " << cat << "  → ";
        size_t k=0; while(k<r.size()) { std::cout << r.at(k).second << " "; k++; }
        std::cout << "  ↓ ";
        k=0; while(k<d.size()) { std::cout << d.at(k).second << " "; k++; }
        std::cout << std::endl;
        i++;
    }
    // hard constraints: pairs of common categories that are NEVER adjacent
    std::cout << "never-adjacent (hard constraints) among the top categories:" << std::endl;
    int shown=0;
    size_t ai=0;
    while(ai<top.size() && shown<14)
    {
        size_t bi=ai+1;
        while(bi<top.size() && shown<14)
        {
            const std::string &A=top.at(ai).second;
            const std::string &B=top.at(bi).second;
            const long n = rightOf[A][B]+rightOf[B][A]+downOf[A][B]+downOf[B][A];
            if(n==0) { std::cout << "  " << A << " | " << B << std::endl; shown++; }
            bi++;
        }
        ai++;
    }
    return 0;
}

static int decodeOne(const QString &mapPath,const QString &outPng)
{
    MapDecoder dec;
    MapDecoder::Result r;
    QString err;
    if(!dec.decode(mapPath,r,err)) { std::cerr << "decode FAIL " << mapPath.toStdString() << ": " << err.toStdString() << std::endl; return 1; }
    // side-by-side: real | category
    const int gap=8;
    QImage combined(r.realRender.width()+gap+r.categoryRender.width(),
                    r.realRender.height(),QImage::Format_ARGB32);
    combined.fill(QColor(0,0,0));
    {
        QPainter p(&combined);
        p.drawImage(0,0,r.realRender);
        p.drawImage(r.realRender.width()+gap,0,r.categoryRender);
    }
    if(!combined.save(outPng)) { std::cerr << "cannot write " << outPng.toStdString() << std::endl; return 1; }
    const int cov = r.totalDrawn>0 ? r.tagged*100/r.totalDrawn : 0;
    std::cout << QFileInfo(mapPath).fileName().toStdString() << ": " << r.w << "x" << r.h
              << "  coverage " << cov << "% (" << r.tagged << " tagged / " << r.untagged << " untagged of "
              << r.totalDrawn << " drawn)  -> " << outPng.toStdString() << std::endl;
    // top categories by cell count, to cross-check the render
    std::map<std::string,int> hist;
    size_t gi=0;
    while(gi<r.categoryGrid.size()) { if(!r.categoryGrid.at(gi).empty()) hist[r.categoryGrid.at(gi)]++; gi++; }
    std::vector<std::pair<int,std::string> > top;
    std::map<std::string,int>::const_iterator hit=hist.begin();
    while(hit!=hist.cend()) { top.push_back(std::make_pair(hit->second,hit->first)); ++hit; }
    std::sort(top.rbegin(),top.rend());
    std::cout << "   ";
    size_t k=0;
    while(k<top.size() && k<8) { std::cout << top.at(k).second << "=" << top.at(k).first << "  "; k++; }
    std::cout << std::endl;
    return 0;
}

static int runDecode(const QStringList &args)
{
    if(args.size()<2) { std::cerr << "decode needs: <map.tmx | map-dir> <out.png | out-dir>" << std::endl; return 1; }
    const QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDir().mkpath(args.at(1));
        QDirIterator it(args.at(0),QStringList()<<"*.tmx",QDir::Files,QDirIterator::Subdirectories);
        int rc=0,n=0;
        while(it.hasNext())
        {
            const QString tmx=it.next();
            const QString png=args.at(1)+"/"+QFileInfo(tmx).completeBaseName()+".png";
            if(decodeOne(tmx,png)!=0) rc=1;
            n++;
        }
        std::cout << "decoded " << n << " map(s) -> " << args.at(1).toStdString() << std::endl;
        return rc;
    }
    return decodeOne(args.at(0),args.at(1));
}

static int runSuggest(const QStringList &args)
{
    if(args.isEmpty()) { std::cerr << "suggest needs: <x.tsx | tileset-dir>" << std::endl; return 1; }
    const QFileInfo fi(args.at(0));
    if(fi.isDir())
    {
        QDir d(args.at(0));
        const QStringList tsxs=d.entryList(QStringList()<<"*.tsx",QDir::Files,QDir::Name);
        std::cout << "suggest: " << tsxs.size() << " tileset(s) in " << args.at(0).toStdString() << std::endl;
        int rc=0,k=0;
        while(k<tsxs.size()) { if(suggestOne(d.absoluteFilePath(tsxs.at(k)))!=0) rc=1; k++; }
        std::cout << "done — open each in the GUI to set the visually-ambiguous categories." << std::endl;
        return rc;
    }
    return suggestOne(args.at(0));
}

int main(int argc, char *argv[])
{
    QStringList args;
    int i=1;
    while(i<argc) { args.append(QString::fromUtf8(argv[i])); i++; }
    const bool cli = !args.isEmpty()
        && (args.at(0)=="--guard" || args.at(0)=="--tag" || args.at(0)=="--selftest"
            || args.at(0)=="--usage" || args.at(0)=="--suggest" || args.at(0)=="--classify"
            || args.at(0)=="--decode" || args.at(0)=="--learn");
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
    if(!args.isEmpty() && args.at(0)=="--suggest")
        return runSuggest(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--classify")
        return runClassify(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--decode")
        return runDecode(args.mid(1));
    if(!args.isEmpty() && args.at(0)=="--learn")
        return runLearn(args.mid(1));

    // GUI: optional first arg is a .tsx to open.
    MainWindow window;
    if(!args.isEmpty() && !args.at(0).startsWith("--"))
        window.openTsx(args.at(0));
    window.show();
    return app.exec();
}
