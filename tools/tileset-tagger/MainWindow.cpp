#include "MainWindow.hpp"
#include "MapUsageView.hpp"
#include "TagModel.hpp"
#include "TilesetView.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QResizeEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QStringList>
#include <QVBoxLayout>
#include <QWidget>

// Curated category vocabulary (NO free-text input — the owner requests new
// entries and we add them here).  Grouped outdoor -> structure -> interior so
// the combo reads top to bottom like a map is built.
static QStringList tagCategories()
{
    QStringList c;
    c << "ground" << "path" << "sand" << "grass-short" << "grass-tall"
      << "water" << "water-edge" << "waterfall" << "lava"
      << "ledge-down" << "ledge-up" << "ledge-left" << "ledge-right"
      << "cliff" << "rock" << "tree-trunk" << "tree-canopy" << "bush" << "flower"
      << "building-wall" << "building-roof" << "door" << "window" << "stairs" << "sign" << "fence"
      << "interior-floor" << "interior-wall" << "carpet" << "counter"
      << "table" << "chair" << "bed" << "shelf" << "bookshelf" << "fridge"
      << "computer" << "tv" << "plant-pot" << "decoration";
    return c;
}

MainWindow::MainWindow() :
    QMainWindow(),
    model_(new TagModel()),
    view_(new TilesetView()),
    usage_(new MapUsageIndex()),
    usageView_(new MapUsageView()),
    categoryBox_(nullptr),
    mapCombo_(nullptr),
    animated_(nullptr),
    hRepeat_(nullptr),
    hMidRepeat_(nullptr),
    vRepeat_(nullptr),
    vMidRepeat_(nullptr),
    selLabel_(nullptr),
    detectedLabel_(nullptr),
    guardLabel_(nullptr),
    currentUsages_(),
    derivedLayer_(),
    derivedWalkable_(-1),
    derivedFromMaps_(false),
    selC0_(-1),
    selR0_(-1),
    selC1_(-1),
    selR1_(-1),
    tsxQueue_(),
    tsxIndex_(0)
{
    QScrollArea *scroll=new QScrollArea(this);
    scroll->setWidget(view_);
    scroll->setBackgroundRole(QPalette::Dark);
    view_->setModel(model_);
    setCentralWidget(scroll);

    QDockWidget *dock=new QDockWidget(tr("Tag"),this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *panel=new QWidget(dock);
    QVBoxLayout *lay=new QVBoxLayout(panel);

    QPushButton *openBtn=new QPushButton(tr("Open .tsx…"),panel);
    lay->addWidget(openBtn);

    // The tag is the VISUAL identity of the tile — what it looks like. The
    // logical role (walkable / collision / water) is NOT tagged by hand: it is
    // derived from the maps and shown below for reference only.
    lay->addWidget(new QLabel(tr("Category (what it looks like):"),panel));
    categoryBox_=new QComboBox(panel);
    categoryBox_->addItems(tagCategories());     // fixed list, NOT editable
    lay->addWidget(categoryBox_);

    animated_=new QCheckBox(tr("animated"),panel);
    lay->addWidget(animated_);

    detectedLabel_=new QLabel(panel);
    detectedLabel_->setWordWrap(true);
    detectedLabel_->setStyleSheet("color:#888;");
    lay->addWidget(detectedLabel_);

    QFrame *line=new QFrame(panel);
    line->setFrameShape(QFrame::HLine);
    lay->addWidget(line);
    lay->addWidget(new QLabel(tr("Repeat behaviour (visual):"),panel));
    hRepeat_=new QCheckBox(tr("horizontalRepeat"),panel);
    hMidRepeat_=new QCheckBox(tr("horizontalMiddleRepeat"),panel);
    vRepeat_=new QCheckBox(tr("verticalRepeat"),panel);
    vMidRepeat_=new QCheckBox(tr("verticalMiddleRepeat"),panel);
    lay->addWidget(hRepeat_);
    lay->addWidget(hMidRepeat_);
    lay->addWidget(vRepeat_);
    lay->addWidget(vMidRepeat_);

    QFrame *line2=new QFrame(panel);
    line2->setFrameShape(QFrame::HLine);
    lay->addWidget(line2);

    selLabel_=new QLabel(tr("no selection"),panel);
    lay->addWidget(selLabel_);
    QPushButton *tagBtn=new QPushButton(tr("Tag selection"),panel);
    QPushButton *verifyBtn=new QPushButton(tr("Mark selection verified (accept guesses)"),panel);
    QPushButton *clearBtn=new QPushButton(tr("Clear tag on selection"),panel);
    lay->addWidget(tagBtn);
    lay->addWidget(verifyBtn);
    lay->addWidget(clearBtn);

    QFrame *line3=new QFrame(panel);
    line3->setFrameShape(QFrame::HLine);
    lay->addWidget(line3);

    QCheckBox *showUntagged=new QCheckBox(tr("highlight untagged (red)"),panel);
    showUntagged->setChecked(true);
    lay->addWidget(showUntagged);
    guardLabel_=new QLabel(panel);
    lay->addWidget(guardLabel_);
    QPushButton *suggestBtn=new QPushButton(tr("Suggest terrain (auto-tag water/grass/ledge)"),panel);
    lay->addWidget(suggestBtn);
    QPushButton *nextBtn=new QPushButton(tr("Jump to next to-verify (red/yellow)"),panel);
    lay->addWidget(nextBtn);

    QHBoxLayout *zoomRow=new QHBoxLayout();
    QPushButton *zoomOutBtn=new QPushButton(tr("Zoom −"),panel);
    QPushButton *zoomInBtn=new QPushButton(tr("Zoom +"),panel);
    zoomRow->addWidget(new QLabel(tr("Tile size:"),panel));
    zoomRow->addWidget(zoomOutBtn);
    zoomRow->addWidget(zoomInBtn);
    lay->addLayout(zoomRow);

    QPushButton *saveBtn=new QPushButton(tr("Save tags"),panel);
    lay->addWidget(saveBtn);
    lay->addStretch(1);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea,dock);

    // bottom dock: "where is this group used on the maps" (step 2 of tagging)
    QDockWidget *usageDock=new QDockWidget(tr("Map usage"),this);
    QWidget *usagePanel=new QWidget(usageDock);
    QVBoxLayout *uLay=new QVBoxLayout(usagePanel);
    QHBoxLayout *uTop=new QHBoxLayout();
    uTop->addWidget(new QLabel(tr("Used on map:"),usagePanel));
    mapCombo_=new QComboBox(usagePanel);
    mapCombo_->setMinimumWidth(280);
    uTop->addWidget(mapCombo_);
    uTop->addStretch(1);
    uLay->addLayout(uTop);
    uLay->addWidget(usageView_,1);
    usageDock->setWidget(usagePanel);
    addDockWidget(Qt::BottomDockWidgetArea,usageDock);

    connect(openBtn,&QPushButton::clicked,this,&MainWindow::onOpen);
    connect(tagBtn,&QPushButton::clicked,this,&MainWindow::onTag);
    connect(verifyBtn,&QPushButton::clicked,this,&MainWindow::onVerify);
    connect(clearBtn,&QPushButton::clicked,this,&MainWindow::onClearTag);
    connect(saveBtn,&QPushButton::clicked,this,&MainWindow::onSave);
    connect(suggestBtn,&QPushButton::clicked,this,&MainWindow::onSuggest);
    connect(nextBtn,&QPushButton::clicked,this,&MainWindow::onNextUntagged);
    connect(zoomInBtn,&QPushButton::clicked,this,&MainWindow::onZoomIn);
    connect(zoomOutBtn,&QPushButton::clicked,this,&MainWindow::onZoomOut);
    connect(showUntagged,&QCheckBox::toggled,this,&MainWindow::onToggleUntagged);
    connect(view_,&TilesetView::selectionChanged,this,&MainWindow::onSelection);
    connect(view_,&TilesetView::selectionFinished,this,&MainWindow::onSelectionFinished);
    connect(mapCombo_,SIGNAL(currentIndexChanged(int)),this,SLOT(onMapPicked(int)));

    resize(1100,820);
    refreshGuard();
    updateTitle();
}

MainWindow::~MainWindow()
{
    delete model_;
    delete usage_;
}

bool MainWindow::openTsx(const QString &path)
{
    if(!model_->load(path))
    {
        QMessageBox::warning(this,tr("Open failed"),model_->error());
        return false;
    }
    view_->setModel(model_);
    usage_->build(path);
    mapCombo_->clear();
    currentUsages_.clear();
    usageView_->clearUsage();
    selC0_=selR0_=selC1_=selR1_=-1;
    derivedFromMaps_=false;
    derivedLayer_.clear();
    derivedWalkable_=-1;
    selLabel_->setText(tr("no selection"));
    detectedLabel_->clear();
    // auto-suggest on open so there is no separate bootstrap step: fills only the
    // untagged tiles (idempotent), leaving the human to fix the yellow guesses.
    if(usage_->candidateCount()>0 && !model_->untaggedNonEmpty().empty())
        onSuggest();
    else
    {
        statusBar()->showMessage(tr("%1 map(s) reference this tileset").arg(usage_->candidateCount()),4000);
        refreshGuard();
        updateTitle();
    }
    fitViewToWindow();
    QScrollArea *scroll=qobject_cast<QScrollArea*>(centralWidget());
    if(scroll!=nullptr)
        scroll->ensureVisible(0,0,0,0);   // show the top-left of the sheet
    return true;
}

void MainWindow::openPath(const QString &path)
{
    QFileInfo fi(path);
    if(fi.isDir())
    {
        QDir d(path);
        const QStringList names=d.entryList(QStringList()<<"*.tsx",QDir::Files,QDir::Name);
        tsxQueue_.clear();
        int i=0;
        while(i<names.size()) { tsxQueue_.append(d.absoluteFilePath(names.at(i))); i++; }
        tsxIndex_=0;
        if(tsxQueue_.isEmpty())
        {
            statusBar()->showMessage(tr("no .tsx files in %1").arg(path),5000);
            return;
        }
        openNextIncomplete();
    }
    else
    {
        tsxQueue_.clear();
        tsxIndex_=0;
        openTsx(path);
    }
}

void MainWindow::openNextIncomplete()
{
    while(tsxIndex_<tsxQueue_.size())
    {
        if(openTsx(tsxQueue_.at(tsxIndex_)))
        {
            const TagModel::Counts c=model_->progress();
            if(c.untagged==0 && c.toReview==0)
            {
                tsxIndex_++;          // already fully verified -> skip
                continue;
            }
            statusBar()->showMessage(tr("tileset %1/%2: %3 — %4 to review/tag")
                                     .arg(tsxIndex_+1).arg(tsxQueue_.size())
                                     .arg(QFileInfo(tsxQueue_.at(tsxIndex_)).fileName())
                                     .arg(c.toReview+c.untagged),0);
            return;
        }
        tsxIndex_++;                  // failed to load -> skip
    }
    QMessageBox::information(this,tr("Done"),tr("All %1 tileset(s) in the folder are fully verified.").arg(tsxQueue_.size()));
}

void MainWindow::fitViewToWindow()
{
    QScrollArea *scroll=qobject_cast<QScrollArea*>(centralWidget());
    if(scroll==nullptr || model_->image().isNull() || model_->image().width()<=0)
        return;
    int z=scroll->viewport()->width()/model_->image().width();   // fill the width
    if(z<2) z=2;
    view_->setZoom(z);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    fitViewToWindow();   // expand the tileset when the window grows/maximises
}

void MainWindow::onZoomIn()  { view_->setZoom(view_->zoom()+1); }
void MainWindow::onZoomOut() { view_->setZoom(view_->zoom()-1); }

void MainWindow::onOpen()
{
    const QString path=QFileDialog::getOpenFileName(this,tr("Open tileset"),QString(),tr("Tiled tileset (*.tsx)"));
    if(!path.isEmpty())
        openTsx(path);
}

// Apply the current category + control settings to the selection as a VERIFIED
// tag (no auto=guess flag).  Returns the number of tiles tagged.
int MainWindow::applySelection()
{
    if(selC0_<0)
        return -1;
    const std::string category=categoryBox_->currentText().toStdString();
    const std::vector<int> ids=model_->tilesInRect(selC0_,selR0_,selC1_,selR1_);
    if(ids.empty())
        return 0;
    const int w=selC1_-selC0_+1;
    const int h=selR1_-selR0_+1;
    std::map<std::string,std::string> attrs;
    attrs["size"]=std::to_string(w)+"x"+std::to_string(h);
    attrs["group"]=category+"@"+std::to_string(selC0_)+","+std::to_string(selR0_);
    if(derivedFromMaps_)
    {
        if(!derivedLayer_.isEmpty())
            attrs["layer"]=derivedLayer_.toStdString();
        if(derivedWalkable_>=0)
            attrs["walkable"]= derivedWalkable_==1 ? "true" : "false";
    }
    if(animated_->isChecked())
        attrs["animated"]="true";
    if(hRepeat_->isChecked())
        attrs["horizontalRepeat"]="true";
    if(hMidRepeat_->isChecked())
        attrs["horizontalMiddleRepeat"]="true";
    if(vRepeat_->isChecked())
        attrs["verticalRepeat"]="true";
    if(vMidRepeat_->isChecked())
        attrs["verticalMiddleRepeat"]="true";
    model_->tagTiles(ids,category,attrs);   // tagTiles writes no auto flag => verified
    view_->refresh();
    refreshGuard();
    updateTitle();
    return (int)ids.size();
}

void MainWindow::onTag()
{
    const int n=applySelection();
    if(n<0)
        statusBar()->showMessage(tr("select a rectangle first"),5000);
    else if(n>0)
        statusBar()->showMessage(tr("tagged %1 tile(s) as '%2' (verified)").arg(n).arg(categoryBox_->currentText()),5000);
}

void MainWindow::onClearTag()
{
    if(selC0_<0)
        return;
    const std::vector<int> ids=model_->tilesInRect(selC0_,selR0_,selC1_,selR1_);
    const std::map<std::string,std::string> none;
    model_->tagTiles(ids,std::string(),none);
    view_->refresh();
    refreshGuard();
    updateTitle();
}

void MainWindow::onVerify()
{
    const int n=applySelection();   // apply the current category + checkboxes, verified
    if(n<0)
    {
        statusBar()->showMessage(tr("select a tile/rectangle first"),5000);
        return;
    }
    statusBar()->showMessage(tr("✓ verified %1 tile(s) as '%2' — moving to next").arg(n).arg(categoryBox_->currentText()),5000);
    onNextUntagged();               // auto-advance to the next tile needing attention
}

void MainWindow::onSave()
{
    if(!model_->save())
    {
        QMessageBox::warning(this,tr("Save failed"),model_->error());
        return;
    }
    statusBar()->showMessage(tr("saved tags to %1 (datapack untouched)").arg(model_->tagFilePath()),5000);
    updateTitle();
}

void MainWindow::onSelection(int col0,int row0,int col1,int row1,int tileCount)
{
    selC0_=col0;
    selR0_=row0;
    selC1_=col1;
    selR1_=row1;
    selLabel_->setText(tr("selection: cols %1..%2 rows %3..%4  (%5x%6 = %7 tiles)")
                       .arg(col0).arg(col1).arg(row0).arg(row1)
                       .arg(col1-col0+1).arg(row1-row0+1).arg(tileCount));
}

void MainWindow::onSelectionFinished(int col0,int row0,int col1,int row1,int)
{
    // step 2: find where this group of tiles is used across the maps.
    const std::vector<int> ids=model_->tilesInRect(col0,row0,col1,row1);
    mapCombo_->blockSignals(true);
    mapCombo_->clear();
    currentUsages_.clear();
    if(usage_->candidateCount()==0 || ids.empty())
    {
        mapCombo_->blockSignals(false);
        usageView_->clearUsage();
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    currentUsages_=usage_->findUsages(ids);
    QApplication::restoreOverrideCursor();
    size_t i=0;
    while(i<currentUsages_.size())
    {
        const MapUsageIndex::Usage &u=currentUsages_.at(i);
        mapCombo_->addItem(tr("%1  (%2 cells)").arg(u.mapLabel).arg((int)u.cells.size()));
        i++;
    }
    mapCombo_->blockSignals(false);
    prefillFromUsage(ids);   // guess category/layer/walkable/repeats for validation
    if(currentUsages_.empty())
    {
        usageView_->clearUsage();
        statusBar()->showMessage(tr("this group is not used on any map"),3000);
    }
    else
    {
        mapCombo_->setCurrentIndex(0);
        onMapPicked(0);
    }
}

// Map a CatchChallenger engine layer name to (layer-enum, category-guess,
// walkable) — used to PRE-FILL the controls from where the tiles really sit.
static void layerToGuess(const QString &rawLayer,QString &layerVal,QString &category,bool &walkable)
{
    const QString L=rawLayer.toLower();
    walkable=true;
    if(L.contains("water")) { layerVal="water"; category="water"; walkable=false; }
    else if(L.contains("lava")) { layerVal="lava"; category="lava"; walkable=false; }
    else if(L.contains("collision")) { layerVal="collision"; category="building-wall"; walkable=false; }
    else if(L.contains("walkbehind") || L=="over" || L=="back") { layerVal="over"; category="tree-canopy"; walkable=true; }
    else if(L.contains("ledge"))
    {
        layerVal="ledge";
        walkable=true;
        if(L.contains("down")) category="ledge-down";
        else if(L.contains("up")) category="ledge-up";
        else if(L.contains("left")) category="ledge-left";
        else if(L.contains("right")) category="ledge-right";
        else category="ledge-down";
    }
    else if(L.contains("grass")) { layerVal="grass"; category="grass-tall"; walkable=true; }
    else if(L.contains("dirt")) { layerVal="walkable"; category="path"; walkable=true; }
    else { layerVal="walkable"; category="ground"; walkable=true; }
}

static void setComboTo(QComboBox *box,const QString &text)
{
    const int i=box->findText(text);
    if(i>=0)
        box->setCurrentIndex(i);
}

void MainWindow::onSuggest()
{
    if(usage_->candidateCount()==0)
    {
        statusBar()->showMessage(tr("no maps reference this tileset — nothing to suggest"),3000);
        return;
    }
    QApplication::setOverrideCursor(Qt::WaitCursor);
    const std::map<int,MapUsageIndex::TileStat> stats=usage_->analyzeAllTiles();
    int sure=0,guess=0;
    std::map<int,MapUsageIndex::TileStat>::const_iterator it=stats.begin();
    while(it!=stats.cend())
    {
        const int id=it->first;
        const MapUsageIndex::TileStat &s=it->second;
        if(model_->tagOf(id).category.empty())
        {
            std::string norm;
            bool walk=true,high=true;
            const std::string cat=MapUsageIndex::suggestCategory(s,model_->tileGreenish(id),norm,walk,high);
            if(!cat.empty())
            {
                std::map<std::string,std::string> attrs;
                attrs["layer"]=norm;
                attrs["walkable"]= walk ? "true" : "false";
                if(!high)
                    attrs["auto"]="guess";   // low-confidence -> shown yellow for review
                std::vector<int> one;
                one.push_back(id);
                model_->tagTiles(one,cat,attrs);
                if(high) sure++; else guess++;
            }
        }
        ++it;
    }
    QApplication::restoreOverrideCursor();
    view_->refresh();
    refreshGuard();
    updateTitle();
    statusBar()->showMessage(tr("auto-tagged %1 sure + %2 to-review (yellow) — fix the wrong guesses, then Save").arg(sure).arg(guess),7000);
}

void MainWindow::prefillFromUsage(const std::vector<int> &ids)
{
    const MapUsageIndex::GroupStats &st=usage_->lastStats();

    // animated is a property of the tileset art, known regardless of map usage
    bool anim=false;
    size_t k=0;
    while(k<ids.size() && !anim) { if(model_->tileAnimated(ids.at(k))) anim=true; k++; }
    animated_->setChecked(anim);

    if(st.totalCells<=0)
    {
        // not used on any map: no logical info to derive, no category hint.
        derivedFromMaps_=false;
        derivedLayer_.clear();
        derivedWalkable_=-1;
        detectedLabel_->setText(tr("from maps: not used anywhere — pick the category by eye"));
        return;
    }

    // dominant engine layer across all maps -> the LOGICAL role (derived, not tagged)
    QString domLayer;
    int best=-1;
    std::map<std::string,int>::const_iterator it=st.layerCounts.begin();
    while(it!=st.layerCounts.cend())
    {
        if(it->second>best) { best=it->second; domLayer=QString::fromStdString(it->first); }
        ++it;
    }
    QString layerVal;
    QString categoryGuess;
    bool layerWalkHint=true;
    layerToGuess(domLayer,layerVal,categoryGuess,layerWalkHint);

    // WALKABLE comes from the engine's EFFECTIVE per-cell precedence (Collisions
    // cancels Walkable), NOT from the placement layer — that is the whole point
    // of Map_loaderMain.cpp's else-if chain. Ledges count as (one-way) walkable.
    const int walkN=st.walkableCells+st.ledgeCells;
    const bool walkable= walkN>=st.blockedCells;

    derivedFromMaps_=true;
    derivedLayer_=layerVal;
    derivedWalkable_= walkable ? 1 : 0;

    // category is the VISUAL identity: the layer gives only a soft starting guess
    // (Collisions could be a wall, a cliff or a tree-trunk) — the human confirms.
    setComboTo(categoryBox_,categoryGuess);

    // repeat flags are visual: the SAME tile recurs in >=30% of the group's cells
    hRepeat_->setChecked(st.horizontalRepeatCells*100 >= st.totalCells*30);
    vRepeat_->setChecked(st.verticalRepeatCells*100 >= st.totalCells*30);

    detectedLabel_->setText(tr("from maps: drawn on '%1' · effective %2 (walk %3% / blocked %4% / ledge %5%)")
                            .arg(domLayer)
                            .arg(walkable?tr("WALKABLE"):tr("BLOCKED"))
                            .arg(st.walkableCells*100/st.totalCells)
                            .arg(st.blockedCells*100/st.totalCells)
                            .arg(st.ledgeCells*100/st.totalCells));
    statusBar()->showMessage(tr("category pre-guessed from layer '%1' — set what it LOOKS like, then Tag").arg(domLayer),5000);
}

void MainWindow::onMapPicked(int index)
{
    if(index<0 || index>=(int)currentUsages_.size())
        return;
    const MapUsageIndex::Usage &u=currentUsages_.at(index);
    const QImage img=usage_->render(u.mapPath);
    usageView_->setUsage(img,u.cells,u.tileW,u.tileH);
    statusBar()->showMessage(tr("'%1' uses the group in %2 cell(s)").arg(u.mapLabel).arg((int)u.cells.size()),4000);
}

void MainWindow::onToggleUntagged(bool on)
{
    view_->setShowUntagged(on);
}

void MainWindow::onNextUntagged()
{
    // walk everything needing attention: untagged (red) then to-review (yellow)
    const std::vector<int> todo=model_->unverifiedTiles();
    if(todo.empty())
    {
        if(!tsxQueue_.isEmpty())
        {
            onSave();              // persist this finished tileset
            tsxIndex_++;
            openNextIncomplete();  // move on to the next tileset in the folder
        }
        else
            statusBar()->showMessage(tr("all tiles verified — nothing left to review"),5000);
        return;
    }
    const int cols=model_->columns();
    int id=todo.at(0);
    // prefer the next one AFTER the current selection, so repeated clicks advance
    if(selC0_>=0)
    {
        const int cur=selR0_*cols+selC0_;
        size_t i=0;
        while(i<todo.size()) { if(todo.at(i)>cur) { id=todo.at(i); break; } i++; }
    }
    const int col=id%cols;
    const int row=id/cols;
    view_->selectCell(col,row);
    QScrollArea *scroll=qobject_cast<QScrollArea*>(centralWidget());
    if(scroll!=nullptr)
    {
        const QRect cell=view_->cellPixelRect(col,row);
        scroll->ensureVisible(cell.center().x(),cell.center().y(),cell.width(),cell.height());
    }
    statusBar()->showMessage(tr("%1 tile(s) to review left — jumped to tile %2").arg((int)todo.size()).arg(id),4000);
}

void MainWindow::refreshGuard()
{
    const TagModel::Counts c=model_->progress();
    const int total=c.verified+c.toReview+c.untagged;
    const int pct = total>0 ? c.verified*100/total : 100;
    if(c.untagged==0 && c.toReview==0)
    {
        guardLabel_->setText(tr("✓ all %1 tiles verified (100%)").arg(c.verified));
        guardLabel_->setStyleSheet("color:#2a8a2a; font-weight:bold;");
    }
    else
    {
        guardLabel_->setText(tr("progress %1%  —  ✓ verified %2 · ⚠ review %3 · ✗ untagged %4")
                             .arg(pct).arg(c.verified).arg(c.toReview).arg(c.untagged));
        guardLabel_->setStyleSheet(c.untagged>0 ? "color:#c02020;" : "color:#b08000;");
    }
}

void MainWindow::updateTitle()
{
    const TagModel::Counts c=model_->progress();
    const int total=c.verified+c.toReview+c.untagged;
    QString name=tr("(no file)");
    if(model_->tileCount()>0 && !model_->image().isNull())
        name=QString("%1 tiles").arg(model_->tileCount());
    setWindowTitle(tr("tileset-tagger — %1 — verified %2/%3").arg(name).arg(c.verified).arg(total));
}
