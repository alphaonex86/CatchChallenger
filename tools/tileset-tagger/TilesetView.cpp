#include "TilesetView.hpp"
#include "TagModel.hpp"

#include <QColor>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QWheelEvent>

TilesetView::TilesetView(QWidget *parent) :
    QWidget(parent),
    model_(nullptr),
    zoom_(3.0),
    showStates_(true),
    selecting_(false),
    additive_(false),
    dragC0_(0),
    dragR0_(0),
    dragC1_(0),
    dragR1_(0),
    selected_()
{
}

void TilesetView::setModel(TagModel *model)
{
    model_=model;
    selected_.clear();
    resize(sizeHint());   // a non-resizable scroll child must be resized explicitly
    updateGeometry();
    update();
}

void TilesetView::refresh() { update(); }

void TilesetView::selectCell(int col,int row)
{
    selected_.clear();
    if(model_!=nullptr)
    {
        const int id=row*model_->columns()+col;
        if(id>=0)
            selected_.insert(id);
    }
    selecting_=false;
    update();
    emitSelection(true);
}

std::vector<int> TilesetView::selectedTiles() const
{
    std::vector<int> ids;
    if(model_==nullptr)
        return ids;
    std::set<int>::const_iterator it=selected_.cbegin();
    while(it!=selected_.cend())
    {
        // EMPTY (transparent) tiles are not real tiles: exclude them so selecting
        // one tags nothing and shows no map.
        if(*it>=0 && *it<model_->tileCount() && model_->tileHasPixels(*it))
            ids.push_back(*it);   // std::set iterates ascending
        ++it;
    }
    return ids;
}

bool TilesetView::hasSelection() const { return !selected_.empty(); }

void TilesetView::selectionBounds(int &c0,int &r0,int &c1,int &r1) const
{
    if(selected_.empty() || model_==nullptr)
    {
        c0=r0=c1=r1=-1;
        return;
    }
    const int cols=model_->columns();
    c0=cols; r0=2147483647; c1=-1; r1=-1;
    std::set<int>::const_iterator it=selected_.cbegin();
    while(it!=selected_.cend())
    {
        const int c=(*it)%cols;
        const int r=(*it)/cols;
        if(c<c0) c0=c;
        if(c>c1) c1=c;
        if(r<r0) r0=r;
        if(r>r1) r1=r;
        ++it;
    }
}

// Fold the live rubber-band rectangle into the committed selection.  Ctrl/Shift+
// click on a single already-selected cell REMOVES it (toggle); otherwise the
// rectangle's cells are added (additive_) or replace the selection.
void TilesetView::commitDrag()
{
    if(model_==nullptr)
        return;
    int c0 = dragC0_<dragC1_ ? dragC0_ : dragC1_;
    int c1 = dragC0_<dragC1_ ? dragC1_ : dragC0_;
    int r0 = dragR0_<dragR1_ ? dragR0_ : dragR1_;
    int r1 = dragR0_<dragR1_ ? dragR1_ : dragR0_;
    const int cols=model_->columns();
    const bool singleCell=(c0==c1 && r0==r1);
    if(!additive_)
        selected_.clear();
    int r=r0;
    while(r<=r1)
    {
        int c=c0;
        while(c<=c1)
        {
            const int id=r*cols+c;
            if(c>=0 && c<cols && id>=0)
            {
                if(additive_ && singleCell && selected_.find(id)!=selected_.end())
                    selected_.erase(id);
                else
                    selected_.insert(id);
            }
            c++;
        }
        r++;
    }
}

QRect TilesetView::cellPixelRect(int col,int row) const
{
    if(model_==nullptr)
        return QRect();
    const double tw=model_->tileWidth()*zoom_;
    const double th=model_->tileHeight()*zoom_;
    return QRect(qRound(col*tw),qRound(row*th),qRound(tw),qRound(th));
}

void TilesetView::setShowStates(bool on) { showStates_=on; update(); }

void TilesetView::setZoom(double z)
{
    if(z<0.1)
        z=0.1;
    if(z>24.0)
        z=24.0;
    zoom_=z;
    resize(sizeHint());   // grow/shrink the widget so the scroll area shows the new zoom
    updateGeometry();
    update();
}

double TilesetView::zoom() const { return zoom_; }

QSize TilesetView::sizeHint() const
{
    if(model_==nullptr || model_->image().isNull())
        return QSize(320,320);
    return QSize(qRound(model_->image().width()*zoom_),qRound(model_->image().height()*zoom_));
}

void TilesetView::cellAt(const QPoint &pt,int &col,int &row) const
{
    const double tw=model_->tileWidth()*zoom_;
    const double th=model_->tileHeight()*zoom_;
    col = tw>0.0 ? (int)(pt.x()/tw) : 0;
    row = th>0.0 ? (int)(pt.y()/th) : 0;
    if(col<0)
        col=0;
    if(row<0)
        row=0;
    if(col>=model_->columns())
        col=model_->columns()-1;
    const int rows=(model_->tileCount()+model_->columns()-1)/model_->columns();
    if(row>=rows)
        row=rows-1;
}

void TilesetView::emitSelection(bool finished)
{
    const int n=(int)selectedTiles().size();
    emit selectionChanged(n);
    if(finished)
        emit selectionFinished(n);
}

void TilesetView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(),QColor(40,40,48));
    if(model_==nullptr || model_->image().isNull())
    {
        p.setPen(Qt::white);
        p.drawText(rect(),Qt::AlignCenter,tr("Open a .tsx (File > Open)"));
        return;
    }
    const QImage &img=model_->image();
    const int iw=qRound(img.width()*zoom_);
    const int ih=qRound(img.height()*zoom_);

    // transparency checkerboard (light/dark grey), squares = HALF the zoomed tile,
    // so the transparent parts of each tile read clearly against it.
    int sq=qRound(model_->tileWidth()*zoom_/2.0);
    if(sq<4)
        sq=4;
    int cy=0;
    while(cy*sq<ih)
    {
        int cx=0;
        while(cx*sq<iw)
        {
            const int sw = (cx*sq+sq>iw) ? iw-cx*sq : sq;
            const int sh = (cy*sq+sq>ih) ? ih-cy*sq : sq;
            p.fillRect(cx*sq,cy*sq,sw,sh,((cx+cy)&1) ? QColor(96,96,96) : QColor(168,168,168));
            cx++;
        }
        cy++;
    }
    p.drawImage(QRect(0,0,iw,ih),img);   // transparent pixels show the checkerboard

    const double tw=model_->tileWidth()*zoom_;
    const double th=model_->tileHeight()*zoom_;
    const int cols=model_->columns();
    const int n=model_->tileCount();

    // three-state overlay (toggleable): red=untagged, yellow=to-review, green=verified
    if(showStates_)
    {
        int id=0;
        while(id<n)
        {
            const int c=id%cols;
            const int r=id/cols;
            const QRect cell(qRound(c*tw),qRound(r*th),qRound(tw),qRound(th));
            const TagModel::TileTag &tag=model_->tagOf(id);
            if(!tag.category.empty())
            {
                if(tag.attr("auto")=="guess")
                    p.fillRect(cell,QColor(255,205,0,105));    // yellow = to review
                else
                    p.fillRect(cell,QColor(40,200,90,90));     // green = verified
            }
            else if(model_->tileHasPixels(id))
            {
                p.fillRect(cell,QColor(255,40,40,105));        // red = untagged
            }
            id++;
        }
    }

    // grid (only when tiles are big enough that lines help, not on a shrunk sheet)
    if(tw>=6.0)
    {
        p.setPen(QColor(0,0,0,70));
        int gx=0;
        while(gx<=img.width()) { p.drawLine(qRound(gx*zoom_),0,qRound(gx*zoom_),ih); gx+=model_->tileWidth(); }
        int gy=0;
        while(gy<=img.height()) { p.drawLine(0,qRound(gy*zoom_),iw,qRound(gy*zoom_)); gy+=model_->tileHeight(); }
    }

    // committed selection: fill every selected cell (works for non-rectangular sets)
    {
        std::set<int>::const_iterator it=selected_.cbegin();
        while(it!=selected_.cend())
        {
            const int c=(*it)%cols;
            const int r=(*it)/cols;
            const QRect cell(qRound(c*tw),qRound(r*th),qRound(tw),qRound(th));
            p.fillRect(cell,QColor(90,170,255,80));
            ++it;
        }
    }
    // live rubber-band while dragging (outline of the rectangle being added/replaced)
    if(selecting_)
    {
        const int c0 = dragC0_<dragC1_ ? dragC0_ : dragC1_;
        const int c1 = dragC0_<dragC1_ ? dragC1_ : dragC0_;
        const int r0 = dragR0_<dragR1_ ? dragR0_ : dragR1_;
        const int r1 = dragR0_<dragR1_ ? dragR1_ : dragR0_;
        const QRect sel(qRound(c0*tw),qRound(r0*th),qRound((c1-c0+1)*tw),qRound((r1-r0+1)*th));
        p.setPen(QPen(QColor(90,170,255),2));
        p.setBrush(QColor(90,170,255,45));
        p.drawRect(sel);
    }
}

void TilesetView::wheelEvent(QWheelEvent *event)
{
    // Ctrl+wheel zooms (multiplicative so it feels even at any scale); plain wheel
    // scrolls (let the scroll area handle it)
    if(event->modifiers() & Qt::ControlModifier)
    {
        setZoom(zoom_ * (event->angleDelta().y()>0 ? 1.15 : 1.0/1.15));
        event->accept();
    }
    else
        QWidget::wheelEvent(event);
}

void TilesetView::mousePressEvent(QMouseEvent *event)
{
    if(model_==nullptr || model_->image().isNull())
        return;
    if(event->button()==Qt::LeftButton)
    {
        // Ctrl or Shift = ADD to the selection (build a non-rectangular pick);
        // plain = start a fresh rectangle.
        additive_ = (event->modifiers() & (Qt::ControlModifier|Qt::ShiftModifier)) != 0;
        if(!additive_)
            selected_.clear();   // clear NOW so the old selection vanishes as the new drag starts
        const QPoint pos=event->position().toPoint();
        cellAt(pos,dragC0_,dragR0_);
        dragC1_=dragC0_;
        dragR1_=dragR0_;
        selecting_=true;
        update();
    }
}

void TilesetView::mouseMoveEvent(QMouseEvent *event)
{
    if(selecting_)
    {
        cellAt(event->position().toPoint(),dragC1_,dragR1_);
        update();
    }
}

void TilesetView::mouseReleaseEvent(QMouseEvent *event)
{
    if(selecting_ && event->button()==Qt::LeftButton)
    {
        cellAt(event->position().toPoint(),dragC1_,dragR1_);
        selecting_=false;
        commitDrag();
        update();
        emitSelection(true);
    }
}
