#include "MainWindow.hpp"
#include "TagModel.hpp"
#include "TilesetView.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
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
      << "water" << "water-edge" << "waterfall"
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
    categoryBox_(nullptr),
    hRepeat_(nullptr),
    hMidRepeat_(nullptr),
    vRepeat_(nullptr),
    vMidRepeat_(nullptr),
    selLabel_(nullptr),
    guardLabel_(nullptr),
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

    lay->addWidget(new QLabel(tr("Category:"),panel));
    categoryBox_=new QComboBox(panel);
    categoryBox_->addItems(tagCategories());     // fixed list, NOT editable
    lay->addWidget(categoryBox_);

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

    connect(openBtn,&QPushButton::clicked,this,&MainWindow::onOpen);
    connect(tagBtn,&QPushButton::clicked,this,&MainWindow::onTag);
    connect(clearBtn,&QPushButton::clicked,this,&MainWindow::onClearTag);
    connect(saveBtn,&QPushButton::clicked,this,&MainWindow::onSave);
    connect(nextBtn,&QPushButton::clicked,this,&MainWindow::onNextUntagged);
    connect(showUntagged,&QCheckBox::toggled,this,&MainWindow::onToggleUntagged);
    connect(view_,&TilesetView::selectionChanged,this,&MainWindow::onSelection);

    resize(1100,820);
    refreshGuard();
    updateTitle();
}

MainWindow::~MainWindow()
{
    delete model_;
}

bool MainWindow::openTsx(const QString &path)
{
    if(!model_->load(path))
    {
        QMessageBox::warning(this,tr("Open failed"),model_->error());
        return false;
    }
    view_->setModel(model_);
    selC0_=selR0_=selC1_=selR1_=-1;
    selLabel_->setText(tr("no selection"));
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
    attrs["name"]=category+"@"+std::to_string(selC0_)+","+std::to_string(selR0_);
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
