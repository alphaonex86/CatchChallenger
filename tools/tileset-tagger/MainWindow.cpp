#include "MainWindow.hpp"
#include "MapUsageView.hpp"
#include "TagModel.hpp"
#include "TilesetView.hpp"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
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
    layerBox_(nullptr),
    mapCombo_(nullptr),
    walkable_(nullptr),
    animated_(nullptr),
    hRepeat_(nullptr),
    hMidRepeat_(nullptr),
    vRepeat_(nullptr),
    vMidRepeat_(nullptr),
    selLabel_(nullptr),
    guardLabel_(nullptr),
    currentUsages_(),
    selC0_(-1),
    selR0_(-1),
    selC1_(-1),
    selR1_(-1)
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

    lay->addWidget(new QLabel(tr("Category (what it is):"),panel));
    categoryBox_=new QComboBox(panel);
    categoryBox_->addItems(tagCategories());     // fixed list, NOT editable
    lay->addWidget(categoryBox_);

    lay->addWidget(new QLabel(tr("Engine layer:"),panel));
    layerBox_=new QComboBox(panel);
    layerBox_->addItem("walkable");
    layerBox_->addItem("grass");
    layerBox_->addItem("water");
    layerBox_->addItem("lava");
    layerBox_->addItem("ledge");
    layerBox_->addItem("collision");
    layerBox_->addItem("over");
    lay->addWidget(layerBox_);

    walkable_=new QCheckBox(tr("walkable (player can stand on it)"),panel);
    animated_=new QCheckBox(tr("animated"),panel);
    lay->addWidget(walkable_);
    lay->addWidget(animated_);

    QFrame *line=new QFrame(panel);
    line->setFrameShape(QFrame::HLine);
    lay->addWidget(line);
    lay->addWidget(new QLabel(tr("Repeat behaviour:"),panel));
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
    QPushButton *clearBtn=new QPushButton(tr("Clear tag on selection"),panel);
    lay->addWidget(tagBtn);
    lay->addWidget(clearBtn);

    QFrame *line3=new QFrame(panel);
    line3->setFrameShape(QFrame::HLine);
    lay->addWidget(line3);

    QCheckBox *showUntagged=new QCheckBox(tr("highlight untagged (red)"),panel);
    showUntagged->setChecked(true);
    lay->addWidget(showUntagged);
    guardLabel_=new QLabel(panel);
    lay->addWidget(guardLabel_);
    QPushButton *nextBtn=new QPushButton(tr("Jump to next untagged"),panel);
    lay->addWidget(nextBtn);

    QPushButton *saveBtn=new QPushButton(tr("Save .tsx"),panel);
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
    connect(clearBtn,&QPushButton::clicked,this,&MainWindow::onClearTag);
    connect(saveBtn,&QPushButton::clicked,this,&MainWindow::onSave);
    connect(nextBtn,&QPushButton::clicked,this,&MainWindow::onNextUntagged);
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
    selLabel_->setText(tr("no selection"));
    statusBar()->showMessage(tr("%1 map(s) reference this tileset").arg(usage_->candidateCount()),4000);
    refreshGuard();
    updateTitle();
    return true;
}

void MainWindow::onOpen()
{
    const QString path=QFileDialog::getOpenFileName(this,tr("Open tileset"),QString(),tr("Tiled tileset (*.tsx)"));
    if(!path.isEmpty())
        openTsx(path);
}

void MainWindow::onTag()
{
    if(selC0_<0)
    {
        statusBar()->showMessage(tr("select a rectangle first"),3000);
        return;
    }
    const std::string category=categoryBox_->currentText().toStdString();
    const std::vector<int> ids=model_->tilesInRect(selC0_,selR0_,selC1_,selR1_);
    if(ids.empty())
        return;
    // size + item-group name are DERIVED from the rectangle (no typing).
    const int w=selC1_-selC0_+1;
    const int h=selR1_-selR0_+1;
    std::map<std::string,std::string> attrs;
    attrs["size"]=std::to_string(w)+"x"+std::to_string(h);
    attrs["group"]=category+"@"+std::to_string(selC0_)+","+std::to_string(selR0_);
    attrs["layer"]=layerBox_->currentText().toStdString();
    attrs["walkable"]= walkable_->isChecked() ? "true" : "false";
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
    model_->tagTiles(ids,category,attrs);
    view_->refresh();
    refreshGuard();
    updateTitle();
    statusBar()->showMessage(tr("tagged %1 tile(s) as '%2'").arg((int)ids.size()).arg(QString::fromStdString(category)),3000);
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

void MainWindow::onSave()
{
    if(!model_->save())
    {
        QMessageBox::warning(this,tr("Save failed"),model_->error());
        return;
    }
    statusBar()->showMessage(tr("saved"),3000);
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

void MainWindow::prefillFromUsage(const std::vector<int> &ids)
{
    const MapUsageIndex::GroupStats &st=usage_->lastStats();
    if(st.totalCells<=0)
        return;     // not used anywhere: leave the controls as the user left them

    // dominant engine layer across all maps
    QString domLayer;
    int best=-1;
    std::map<std::string,int>::const_iterator it=st.layerCounts.begin();
    while(it!=st.layerCounts.cend())
    {
        if(it->second>best) { best=it->second; domLayer=QString::fromStdString(it->first); }
        ++it;
    }
    QString layerVal;
    QString category;
    bool walkable=true;
    layerToGuess(domLayer,layerVal,category,walkable);
    setComboTo(categoryBox_,category);
    setComboTo(layerBox_,layerVal);
    walkable_->setChecked(walkable);

    // repeat flags: the SAME tile recurs in >=30% of the group's cells
    hRepeat_->setChecked(st.horizontalRepeatCells*100 >= st.totalCells*30);
    vRepeat_->setChecked(st.verticalRepeatCells*100 >= st.totalCells*30);

    // animated if any selected tile carries an animation property
    bool anim=false;
    size_t k=0;
    while(k<ids.size() && !anim) { if(model_->tileAnimated(ids.at(k))) anim=true; k++; }
    animated_->setChecked(anim);

    statusBar()->showMessage(tr("pre-filled from '%1' layer — validate or edit, then Tag")
                             .arg(domLayer.isEmpty()?tr("?"):domLayer),5000);
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
    const std::vector<int> untagged=model_->untaggedNonEmpty();
    if(untagged.empty())
    {
        statusBar()->showMessage(tr("no untagged tiles — all done"),3000);
        return;
    }
    const int cols=model_->columns();
    const int id=untagged.at(0);
    const int col=id%cols;
    const int row=id/cols;
    view_->selectCell(col,row);
    QScrollArea *scroll=qobject_cast<QScrollArea*>(centralWidget());
    if(scroll!=nullptr)
    {
        const QRect cell=view_->cellPixelRect(col,row);
        scroll->ensureVisible(cell.center().x(),cell.center().y(),cell.width(),cell.height());
    }
    statusBar()->showMessage(tr("%1 untagged tile(s) left — jumped to tile %2").arg((int)untagged.size()).arg(id),4000);
}

void MainWindow::refreshGuard()
{
    const int n=(int)model_->untaggedNonEmpty().size();
    if(n==0)
    {
        guardLabel_->setText(tr("✓ every drawn tile is tagged"));
        guardLabel_->setStyleSheet("color:#2a8a2a;");
    }
    else
    {
        guardLabel_->setText(tr("⚠ %1 tile(s) draw pixels but are UNTAGGED").arg(n));
        guardLabel_->setStyleSheet("color:#c02020; font-weight:bold;");
    }
}

void MainWindow::updateTitle()
{
    const int n=(int)model_->untaggedNonEmpty().size();
    QString name=tr("(no file)");
    if(model_->tileCount()>0 && !model_->image().isNull())
        name=QString("%1 tiles").arg(model_->tileCount());
    setWindowTitle(tr("tileset-tagger — %1 — %2 untagged").arg(name).arg(n));
}
