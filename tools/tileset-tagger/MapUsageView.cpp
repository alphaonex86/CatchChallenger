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

QSize MapUsageView::sizeHint() const { return QSize(520,340); }

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

    // fit the map into the widget, centred (no upscale beyond 4x for tiny maps)
    if(map_.width()<=0 || map_.height()<=0)
        return;
    double s=(double)width()/map_.width();
    const double sy=(double)height()/map_.height();
    if(sy<s)
        s=sy;
    if(s>4.0)
        s=4.0;
    if(s<=0.0)
        s=1.0;
    const double ox=(width()-map_.width()*s)/2.0;
    const double oy=(height()-map_.height()*s)/2.0;
    p.setRenderHint(QPainter::SmoothPixmapTransform,false);
    p.drawImage(QRectF(ox,oy,map_.width()*s,map_.height()*s),map_);

    size_t i=0;
    while(i<rects_.size())
    {
        const QRect r=rects_.at(i);
        const QRectF wr(ox+r.x()*tileW_*s,
                        oy+r.y()*tileH_*s,
                        r.width()*tileW_*s,
                        r.height()*tileH_*s);
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
