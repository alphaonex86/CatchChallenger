#include "MainWindow.hpp"
#include "MapUsageView.hpp"
#include "TagModel.hpp"
#include "TilesetView.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDockWidget>
#include <QEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
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
    c << "terrain"
      << "ground" << "path" << "sand" << "grass-short" << "grass-tall"
      << "water" << "water-edge" << "waterfall" << "lava"
      << "ledge-down" << "ledge-up" << "ledge-left" << "ledge-right"
      << "cliff" << "rock" << "tree-trunk" << "tree-canopy" << "bush" << "flower"
      << "building-wall" << "building-roof" << "door" << "window" << "stairs" << "sign" << "fence"
      << "interior-floor" << "interior-wall" << "carpet" << "counter"
      << "table" << "chair" << "bed" << "shelf" << "bookshelf" << "fridge"
      << "computer" << "tv" << "plant-pot" << "decoration";
    c.sort();   // alphabetical, easy to find
    return c;
}

// The "terrain" inner/outer texture vocabulary (a terrain tile is a transition
// from an INNER fill to an OUTER surround — a Wang/auto-tile block).
static QStringList terrainTextures()
{
    QStringList c;
    c << "grass" << "tall-grass" << "sand" << "dirt" << "gravel" << "brick"
      << "stone" << "path" << "wood" << "floor" << "carpet" << "water"
      << "snow" << "lava" << "cliff";
    return c;
}

// A single-tile object, or a MULTI-TILE block that scales (a terrain/auto-tile
// set)?  This is what the owner wanted spelled out so "grass-tall" isn't mistaken
// for one animated tile.  Shown live under the category combo.
static QString categoryDescription(const QString &c)
{
    if(c=="terrain")  return QObject::tr("MULTI-TILE auto-tiling block that SCALES to fill an area: a transition from an INNER fill to an OUTER surround. Set the inner & outer texture below. Select the whole block (use Ctrl/Shift+drag to add the border tiles).");
    if(c=="ground")   return QObject::tr("MULTI-TILE base ground terrain (walkable). Fill an area — select the whole block, not one tile.");
    if(c=="path")     return QObject::tr("MULTI-TILE path/road terrain (walkable), with edges & corners. Fill a strip.");
    if(c=="sand")     return QObject::tr("MULTI-TILE sand terrain (walkable), with edges to grass/water. Fill an area.");
    if(c=="grass-short") return QObject::tr("MULTI-TILE short-grass GROUND — decorative, walkable, NO encounters. Fill an area.");
    if(c=="grass-tall")  return QObject::tr("MULTI-TILE TALL grass = monster-encounter zone. Fill a rigid rectangle (base + grass border = ONE terrain). NOT a single animated tile.");
    if(c=="water")    return QObject::tr("MULTI-TILE water terrain (blocked surf zone), animated, with shore edges. Fill an area.");
    if(c=="lava")     return QObject::tr("MULTI-TILE lava terrain (blocked hazard), animated. Fill an area.");
    if(c=="interior-floor") return QObject::tr("MULTI-TILE indoor floor (walkable). Fill a room.");
    if(c=="carpet")   return QObject::tr("MULTI-TILE rug/carpet floor (walkable), with a border. Fill an area.");
    if(c=="water-edge") return QObject::tr("Border tiles where water meets land (edges & corners). A MULTI-TILE set, not a fill.");
    if(c=="waterfall")  return QObject::tr("Animated waterfall — a vertical strip (top/middle/bottom). MULTI-TILE.");
    if(c=="cliff")    return QObject::tr("Cliff face between two heights (blocked). A MULTI-TILE vertical/edge set.");
    if(c.startsWith("ledge")) return QObject::tr("One-way jump ledge. A row/column of edge tiles (MULTI-TILE); the engine makes it one-way.");
    if(c=="fence")    return QObject::tr("Fence/railing (blocked). A line of edge tiles (MULTI-TILE).");
    if(c=="tree-trunk")  return QObject::tr("Lower TRUNK of a tree (blocked). Usually under a tree-canopy. 1+ tiles.");
    if(c=="tree-canopy") return QObject::tr("Upper LEAVES of a tree, drawn ABOVE the player. 1+ tiles.");
    if(c=="bush")     return QObject::tr("Small bush/shrub object. 1+ tiles.");
    if(c=="flower")   return QObject::tr("Decorative flowers (walkable). Usually 1 tile, often animated.");
    if(c=="rock")     return QObject::tr("Rock/boulder object (blocked). 1+ tiles.");
    if(c=="building-wall") return QObject::tr("WALL/face of a building (blocked) — the solid body. MULTI-TILE. Not furniture/decoration.");
    if(c=="building-roof") return QObject::tr("ROOF of a building, often drawn above the player. MULTI-TILE.");
    if(c=="door")     return QObject::tr("A door (becomes an entrance/teleport). Usually 1 tile (sometimes 1×2).");
    if(c=="window")   return QObject::tr("A building window (decorative, blocked). Usually 1 tile.");
    if(c=="stairs")   return QObject::tr("Stairs/steps. 1+ tiles.");
    if(c=="sign")     return QObject::tr("A readable sign/board (blocked). Usually 1 tile.");
    if(c=="interior-wall") return QObject::tr("Indoor wall (blocked). MULTI-TILE.");
    if(c=="counter")  return QObject::tr("Shop/room counter (blocked). 1+ tiles.");
    if(c=="decoration") return QObject::tr("Generic decorative object — use when nothing more specific fits. 1+ tiles.");
    // table/chair/bed/shelf/bookshelf/fridge/computer/tv/plant-pot
    return QObject::tr("Furniture/appliance object — decorative & blocked, NOT a wall. Pick exactly its tile(s); 1+ tiles.");
}

MainWindow::MainWindow() :
    QMainWindow(),
    model_(new TagModel()),
    scroll_(nullptr),
    view_(new TilesetView()),
    usage_(new MapUsageIndex()),
    usageView_(new MapUsageView()),
    openBtn_(nullptr),
    categoryBox_(nullptr),
    categoryDescLabel_(nullptr),
    terrainGroup_(nullptr),
    innerBox_(nullptr),
    outerBox_(nullptr),
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
    scroll->setAlignment(Qt::AlignCenter);   // centre the sheet when smaller than the viewport
    scroll->setBackgroundRole(QPalette::Dark);
    view_->setModel(model_);
    setCentralWidget(scroll);
    scroll_=scroll;
    scroll_->viewport()->installEventFilter(this);   // refit exactly when the viewport resizes

    QDockWidget *dock=new QDockWidget(tr("Tag"),this);
    dock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    QWidget *panel=new QWidget(dock);
    QVBoxLayout *lay=new QVBoxLayout(panel);

    openBtn_=new QPushButton(tr("Open tileset folder…"),panel);
    lay->addWidget(openBtn_);

    // The tag is the VISUAL identity of the tile — what it looks like. The
    // logical role (walkable / collision / water) is NOT tagged by hand: it is
    // derived from the maps and shown below for reference only.
    lay->addWidget(new QLabel(tr("Category (what it looks like):"),panel));
    categoryBox_=new QComboBox(panel);
    categoryBox_->addItems(tagCategories());     // fixed list, NOT editable
    lay->addWidget(categoryBox_);

    // live description of the picked category (single-tile vs multi-tile terrain…)
    categoryDescLabel_=new QLabel(panel);
    categoryDescLabel_->setWordWrap(true);
    categoryDescLabel_->setStyleSheet("color:#bbb; font-style:italic;");
    lay->addWidget(categoryDescLabel_);

    // terrain inner/outer texture pickers — only meaningful for the "terrain"
    // category.  EDITABLE (free text) so you can name any terrain AND reuse an
    // existing name to LINK two terrains (terrain A outer == terrain B inner);
    // typed names are remembered in the dropdown so links stay typo-consistent.
    terrainGroup_=new QWidget(panel);
    QFormLayout *tForm=new QFormLayout(terrainGroup_);
    tForm->setContentsMargins(0,0,0,0);
    innerBox_=new QComboBox(terrainGroup_);
    innerBox_->setEditable(true);
    innerBox_->setInsertPolicy(QComboBox::InsertAlphabetically);
    innerBox_->addItems(terrainTextures());
    outerBox_=new QComboBox(terrainGroup_);
    outerBox_->setEditable(true);
    outerBox_->setInsertPolicy(QComboBox::InsertAlphabetically);
    outerBox_->addItems(terrainTextures());
    tForm->addRow(tr("inner / fill (free text, reuse to link):"),innerBox_);
    tForm->addRow(tr("outer / border (free text, reuse to link):"),outerBox_);
    lay->addWidget(terrainGroup_);
    terrainGroup_->setVisible(false);

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
    QPushButton *verifyBtn=new QPushButton(tr("✓ Verify selection → next  (apply + advance)"),panel);
    QPushButton *clearBtn=new QPushButton(tr("Clear tag on selection"),panel);
    lay->addWidget(verifyBtn);
    lay->addWidget(clearBtn);

    QFrame *line3=new QFrame(panel);
    line3->setFrameShape(QFrame::HLine);
    lay->addWidget(line3);

    QCheckBox *showUntagged=new QCheckBox(tr("show state colours (✗ red · ⚠ yellow · ✓ green)"),panel);
    showUntagged->setChecked(true);
    lay->addWidget(showUntagged);
    guardLabel_=new QLabel(panel);
    guardLabel_->setWordWrap(true);
    lay->addWidget(guardLabel_);
    lay->addStretch(1);

    dock->setWidget(panel);
    addDockWidget(Qt::RightDockWidgetArea,dock);

    // LEFT dock: "where is this group used on the maps" (step 2 of tagging).  Left
    // (tall) — NOT bottom (wide+short) — because the maps are mostly tall/narrow
    // routes; a tall panel shows the whole map readably, like Tiled's left map view.
    QDockWidget *usageDock=new QDockWidget(tr("Map usage"),this);
    QWidget *usagePanel=new QWidget(usageDock);
    QVBoxLayout *uLay=new QVBoxLayout(usagePanel);
    QHBoxLayout *uTop=new QHBoxLayout();
    uTop->addWidget(new QLabel(tr("Used on map:"),usagePanel));
    mapCombo_=new QComboBox(usagePanel);
    mapCombo_->setMinimumWidth(200);
    uTop->addWidget(mapCombo_,1);
    uLay->addLayout(uTop);
    // The view FITS THE WHOLE map into itself (so every usage mark shows at once),
    // so it goes straight in the tall LEFT dock — no scroll, no slice.
    uLay->addWidget(usageView_,1);
    usageDock->setWidget(usagePanel);
    addDockWidget(Qt::LeftDockWidgetArea,usageDock);

    connect(openBtn_,&QPushButton::clicked,this,&MainWindow::onOpen);
    connect(verifyBtn,&QPushButton::clicked,this,&MainWindow::onVerify);
    connect(clearBtn,&QPushButton::clicked,this,&MainWindow::onClearTag);
    connect(showUntagged,&QCheckBox::toggled,this,&MainWindow::onToggleUntagged);
    connect(view_,&TilesetView::selectionChanged,this,&MainWindow::onSelection);
    connect(view_,&TilesetView::selectionFinished,this,&MainWindow::onSelectionFinished);
    connect(mapCombo_,SIGNAL(currentIndexChanged(int)),this,SLOT(onMapPicked(int)));
    connect(categoryBox_,&QComboBox::currentTextChanged,this,&MainWindow::onCategoryChanged);
    onCategoryChanged(categoryBox_->currentText());   // initial description

    resize(1320,860);   // room for: left map dock + central tileset + right tag panel
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
    // position on the FIRST tile needing attention so the human reviews IN ORDER
    // (no jump button — verify auto-advances to the next from here).
    const std::vector<int> todo=model_->unverifiedTiles();
    QScrollArea *scroll=qobject_cast<QScrollArea*>(centralWidget());
    if(!todo.empty())
    {
        const int cols=model_->columns();
        const int id=todo.at(0);
        view_->selectCell(id%cols,id/cols);
        if(scroll!=nullptr)
        {
            const QRect c=view_->cellPixelRect(id%cols,id/cols);
            scroll->ensureVisible(c.center().x(),c.center().y(),c.width(),c.height());
        }
    }
    else if(scroll!=nullptr)
        scroll->ensureVisible(0,0,0,0);
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
    if(scroll_==nullptr || model_->image().isNull() || model_->image().width()<=0 || model_->image().height()<=0)
        return;
    // Fit the WHOLE sheet in the viewport (both dimensions) so every tile is visible
    // AND the sheet fills the space — a FRACTIONAL scale: a 512px sheet in a ~900px
    // viewport becomes ~1.7x (fills it), and if a dock steals height it shrinks below
    // 1x so nothing is cut.  A small margin keeps it off the scrollbars.
    const double m=8.0;
    const double sw=(scroll_->viewport()->width()-m)/(double)model_->image().width();
    const double sh=(scroll_->viewport()->height()-m)/(double)model_->image().height();
    double z = sw<sh ? sw : sh;
    if(z<0.1) z=0.1;
    view_->setZoom(z);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    fitViewToWindow();   // first pass; eventFilter does the authoritative refit
}

bool MainWindow::eventFilter(QObject *watched,QEvent *event)
{
    // The central area (scroll viewport) is laid out AFTER MainWindow::resizeEvent,
    // so the viewport size is only final here -> this is where the fit is correct.
    if(scroll_!=nullptr && watched==scroll_->viewport() && event->type()==QEvent::Resize)
        fitViewToWindow();
    return QMainWindow::eventFilter(watched,event);
}

void MainWindow::onZoomIn()  { view_->setZoom(view_->zoom()+1); }
void MainWindow::onZoomOut() { view_->setZoom(view_->zoom()-1); }

void MainWindow::hideOpenButton()
{
    if(openBtn_!=nullptr)
        openBtn_->hide();
}

void MainWindow::onOpen()
{
    const QString dir=QFileDialog::getExistingDirectory(this,tr("Open a tileset folder"));
    if(!dir.isEmpty())
        openPath(dir);
}

// Apply the current category + control settings to the selection as a VERIFIED
// tag (no auto=guess flag).  Returns the number of tiles tagged.
int MainWindow::applySelection()
{
    if(selC0_<0)
        return -1;
    const std::string category=categoryBox_->currentText().toStdString();
    const std::vector<int> ids=view_->selectedTiles();   // the exact (maybe non-rect) set
    if(ids.empty())
        return -1;   // only empty/transparent cells selected -> nothing to tag
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
    if(category=="terrain")   // a terrain records its inner fill + outer border texture
    {
        const QString in=innerBox_->currentText().trimmed();
        const QString out=outerBox_->currentText().trimmed();
        attrs["inner"]=in.toStdString();
        attrs["outer"]=out.toStdString();
        // remember both names in BOTH dropdowns so the next terrain can reuse them
        // to LINK (this terrain's outer == the neighbour terrain's inner).
        rememberTexture(in);
        rememberTexture(out);
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
    const std::vector<int> ids=view_->selectedTiles();
    if(ids.empty())
        return;
    const std::map<std::string,std::string> none;
    model_->tagTiles(ids,std::string(),none);
    model_->save();           // save on each change
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
    model_->save();                 // auto-persist so no work is lost (Save is optional)
    statusBar()->showMessage(tr("✓ verified %1 tile(s) as '%2' (saved) — moving to next").arg(n).arg(categoryBox_->currentText()),5000);
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

void MainWindow::onSelection(int tileCount)
{
    view_->selectionBounds(selC0_,selR0_,selC1_,selR1_);   // bbox of the (maybe non-rect) set
    if(selC0_<0)
        selLabel_->setText(tr("no selection — drag tiles (Ctrl/Shift+drag adds, Ctrl+click toggles)"));
    else
        selLabel_->setText(tr("selection: %1 tile(s)  (bbox %2x%3)")
                           .arg(tileCount).arg(selC1_-selC0_+1).arg(selR1_-selR0_+1));
}

void MainWindow::onCategoryChanged(const QString &category)
{
    if(categoryDescLabel_!=nullptr)
        categoryDescLabel_->setText(categoryDescription(category));
    if(terrainGroup_!=nullptr)
        terrainGroup_->setVisible(category=="terrain");   // inner/outer only for terrain
}

// Make a terrain-texture name available in BOTH dropdowns so the next terrain can
// reuse it to link (terrain A outer == terrain B inner).  Keeps the current text.
void MainWindow::rememberTexture(const QString &name)
{
    if(name.isEmpty())
        return;
    if(innerBox_!=nullptr && innerBox_->findText(name)<0)
        innerBox_->addItem(name);
    if(outerBox_!=nullptr && outerBox_->findText(name)<0)
        outerBox_->addItem(name);
}

void MainWindow::onSelectionFinished(int)
{
    // step 2: find where this group of tiles is used across the maps.
    const std::vector<int> ids=view_->selectedTiles();
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
    // search the map for the FIRST tile of the selection only — a whole group (or an
    // animated tile whose other frames aren't placed) still locates via its anchor.
    std::vector<int> searchIds;
    searchIds.push_back(ids.front());
    currentUsages_=usage_->findUsages(searchIds);
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
    // If the anchor tile is ALREADY tagged, show its SAVED values instead of the
    // guess — so re-selecting a tile you fixed doesn't look like it was lost.
    {
        const TagModel::TileTag &existing=model_->tagOf(ids.front());
        if(!existing.category.empty())
        {
            const int ci=categoryBox_->findText(QString::fromStdString(existing.category));
            if(ci>=0)
                categoryBox_->setCurrentIndex(ci);
            animated_->setChecked(existing.attr("animated")=="true");
            hRepeat_->setChecked(existing.attr("horizontalRepeat")=="true");
            hMidRepeat_->setChecked(existing.attr("horizontalMiddleRepeat")=="true");
            vRepeat_->setChecked(existing.attr("verticalRepeat")=="true");
            vMidRepeat_->setChecked(existing.attr("verticalMiddleRepeat")=="true");
            if(existing.category=="terrain")   // restore saved inner/outer terrain link
            {
                const QString in=QString::fromStdString(existing.attr("inner"));
                const QString out=QString::fromStdString(existing.attr("outer"));
                rememberTexture(in);
                rememberTexture(out);
                innerBox_->setCurrentText(in);
                outerBox_->setCurrentText(out);
            }
        }
    }
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
                attrs["auto"]="guess";   // EVERY guess is to-review (human validates all)
                (void)high;
                std::vector<int> one;
                one.push_back(id);
                model_->tagTiles(one,cat,attrs);
                guess++;
            }
        }
        ++it;
    }
    (void)sure;
    QApplication::restoreOverrideCursor();
    view_->refresh();
    refreshGuard();
    updateTitle();
    statusBar()->showMessage(tr("auto-suggested %1 tile(s) to review — validate each one").arg(guess),7000);
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
    usageView_->setUsage(img,u.cells,u.tileW,u.tileH);   // fits the whole map; marching ants mark every cell
    statusBar()->showMessage(tr("'%1' uses the group in %2 cell(s)").arg(u.mapLabel).arg((int)u.cells.size()),4000);
}

void MainWindow::onToggleUntagged(bool on)
{
    view_->setShowStates(on);
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
