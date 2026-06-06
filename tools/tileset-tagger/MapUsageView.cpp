#include "MapUsageView.hpp"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QTimer>
#include <set>
#include <utility>

MapUsageView::MapUsageView(QWidget *parent) :
    QWidget(parent),
    map_(),
    rects_(),
    tileW_(16),
    tileH_(16),
    phase_(0),
    timer_(new QTimer(this))
{
    timer_->setInterval(110);
    connect(timer_,&QTimer::timeout,this,&MapUsageView::onTick);
    setMinimumHeight(240);
}

QSize MapUsageView::sizeHint() const { return map_.isNull() ? QSize(520,340) : map_.size(); }

QPoint MapUsageView::firstHighlightCenter() const
{
    if(rects_.empty())
        return QPoint(map_.width()/2,map_.height()/2);
    const QRect &r=rects_.front();
    return QPoint(r.x()*tileW_+r.width()*tileW_/2, r.y()*tileH_+r.height()*tileH_/2);
}

void MapUsageView::clearUsage()
{
    map_=QImage();
    rects_.clear();
    timer_->stop();
    update();
}

void MapUsageView::setUsage(const QImage &mapImage,const std::vector<QPoint> &cells,int tileW,int tileH)
{
    map_=mapImage;
    tileW_ = tileW>0 ? tileW : 16;
    tileH_ = tileH>0 ? tileH : 16;
    rects_.clear();
    phase_=0;

    // Greedy rectangle cover of the usage cells: a contiguously-placed group
    // (a house, a sign) collapses to ONE rectangle; scattered cells stay small.
    // Ordered by (row,col) so *begin() is the top-left remaining cell.
    std::set<std::pair<int,int> > rem;
    size_t i=0;
    while(i<cells.size()) { rem.insert(std::make_pair(cells.at(i).y(),cells.at(i).x())); i++; }

    const int CAP=500;     // bound the work for ubiquitous tiles
    while(!rem.empty() && (int)rects_.size()<CAP)
    {
        const std::pair<int,int> tl=*rem.begin();
        const int y0=tl.first;
        const int x0=tl.second;
        int x1=x0;
        while(rem.find(std::make_pair(y0,x1+1))!=rem.end())
            x1++;
        int y1=y0;
        bool rowFull=true;
        while(rowFull)
        {
            int xx=x0;
            while(xx<=x1)
            {
                if(rem.find(std::make_pair(y1+1,xx))==rem.end()) { rowFull=false; break; }
                xx++;
            }
            if(rowFull)
                y1++;
        }
        rects_.push_back(QRect(x0,y0,x1-x0+1,y1-y0+1));
        int yy=y0;
        while(yy<=y1)
        {
            int xx=x0;
            while(xx<=x1) { rem.erase(std::make_pair(yy,xx)); xx++; }
            yy++;
        }
    }

    if(!rects_.empty())
        timer_->start();
    else
        timer_->stop();
    updateGeometry();   // native size changed -> let the scroll area resize
    update();
}

void MapUsageView::onTick()
{
    phase_=(phase_+1)%32;
    update();
}

void MapUsageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(),QColor(22,22,30));
    if(map_.isNull())
    {
        p.setPen(QColor(180,180,180));
        p.drawText(rect(),Qt::AlignCenter,tr("Select a tile group (drag a rectangle) to see where it is used"));
        return;
    }

    // draw the map at NATIVE 1:1 — exactly Tiled's pixel output, no scaling/aliasing
    // (the surrounding scroll area handles overflow + centring).
    p.drawImage(0,0,map_);

    size_t i=0;
    while(i<rects_.size())
    {
        const QRect r=rects_.at(i);
        const QRect wr(r.x()*tileW_,r.y()*tileH_,r.width()*tileW_,r.height()*tileH_);
        p.fillRect(wr,QColor(255,90,40,70));
        // marching ants: solid dark under-stroke + animated white dashes over it
        p.setPen(QPen(QColor(0,0,0,200),2));
        p.setBrush(Qt::NoBrush);
        p.drawRect(wr);
        QPen dash(QColor(255,255,255),2,Qt::DashLine);
        dash.setDashOffset(phase_);
        p.setPen(dash);
        p.drawRect(wr);
        i++;
    }
}
