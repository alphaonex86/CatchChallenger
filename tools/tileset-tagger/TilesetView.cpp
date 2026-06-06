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
    hasSelection_(false),
    selCol0_(0),
    selRow0_(0),
    selCol1_(0),
    selRow1_(0)
{
}

void TilesetView::setModel(TagModel *model)
{
    model_=model;
    hasSelection_=false;
    resize(sizeHint());   // a non-resizable scroll child must be resized explicitly
    updateGeometry();
    update();
}

void TilesetView::refresh() { update(); }

void TilesetView::selectCell(int col,int row)
{
    selCol0_=col;
    selRow0_=row;
    selCol1_=col;
    selRow1_=row;
    hasSelection_=true;
    selecting_=false;
    update();
    emitSelection(true);
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
    int c0 = selCol0_<selCol1_ ? selCol0_ : selCol1_;
    int c1 = selCol0_<selCol1_ ? selCol1_ : selCol0_;
    int r0 = selRow0_<selRow1_ ? selRow0_ : selRow1_;
    int r1 = selRow0_<selRow1_ ? selRow1_ : selRow0_;
    const int n=(int)model_->tilesInRect(c0,r0,c1,r1).size();
    emit selectionChanged(c0,r0,c1,r1,n);
    if(finished)
        emit selectionFinished(c0,r0,c1,r1,n);
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

    // selection rectangle
    if(hasSelection_)
    {
        const int c0 = selCol0_<selCol1_ ? selCol0_ : selCol1_;
        const int c1 = selCol0_<selCol1_ ? selCol1_ : selCol0_;
        const int r0 = selRow0_<selRow1_ ? selRow0_ : selRow1_;
        const int r1 = selRow0_<selRow1_ ? selRow1_ : selRow0_;
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
        const QPoint pos=event->position().toPoint();
        cellAt(pos,selCol0_,selRow0_);
        selCol1_=selCol0_;
        selRow1_=selRow0_;
        selecting_=true;
        hasSelection_=true;
        update();
        emitSelection(false);
    }
}

void TilesetView::mouseMoveEvent(QMouseEvent *event)
{
    if(selecting_)
    {
        cellAt(event->position().toPoint(),selCol1_,selRow1_);
        update();
        emitSelection(false);
    }
}

void TilesetView::mouseReleaseEvent(QMouseEvent *event)
{
    if(selecting_ && event->button()==Qt::LeftButton)
    {
        cellAt(event->position().toPoint(),selCol1_,selRow1_);
        selecting_=false;
        update();
        emitSelection(true);
    }
}
