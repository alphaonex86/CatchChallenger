#include "MapMonsterPreview.hpp"
#include "../../cc/QtDatapackClientLoader.hpp"
#include "../../../qt/GameLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/fight/CommonFightEngine.hpp"
#include <QPainter>

MapMonsterPreview::MapMonsterPreview(const CatchChallenger::PlayerMonster &monster, QGraphicsItem *parent) :
    QGraphicsPixmapItem(parent)
{
    this->monster=monster;
    pressed=false;
    m_drag=false;
    regenCache();
}

MapMonsterPreview::~MapMonsterPreview()
{
}

void MapMonsterPreview::setInDrag(bool drag)
{
    if(m_drag==drag)
        return;
    m_drag=drag;
    regenCache();
}

void MapMonsterPreview::regenCache()
{
    QImage cache(QSize(80,80),QImage::Format_ARGB32_Premultiplied);
    cache.fill(Qt::transparent);
    QPainter painter(&cache);

    QPixmap background;
    if(m_drag)
        background=*GameLoader::gameLoader->getImage(":/CC/images/interface/mgbDrag.png");
    else
        background=*GameLoader::gameLoader->getImage(":/CC/images/interface/mgb.png");
    unsigned int bx=cache.width()/2-background.width()/2;
    unsigned int by=cache.height()/2-background.height()/2;
    painter.drawPixmap(bx,by,background.width(),background.height(),background);

    const QtDatapackClientLoader::QtMonsterExtra &monsterExtra=QtDatapackClientLoader::datapackLoader->getMonsterExtra(monster.monster);
    //QPixmap front=monsterExtra.front;
    QPixmap front=monsterExtra.thumb.scaledToWidth(monsterExtra.thumb.width()*2,Qt::FastTransformation);
    painter.drawPixmap(cache.width()/2-front.width()/2,cache.height()/2-front.height()/2,front.width(),front.height(),front);

    const CatchChallenger::Monster &monsterGeneralInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster);
    const CatchChallenger::Monster::Stat &stat=CatchChallenger::CommonFightEngine::getStat(monsterGeneralInfo,monster.level);

    if(monster.hp==0)
    {
        QPixmap monsterko=QPixmap(":/CC/images/interface/monsterko.png");
        painter.drawPixmap(cache.width()/2-monsterko.width()/2,cache.height()/2-monsterko.height()/2,monsterko.width(),monsterko.height(),monsterko);
    }

    int barx=bx+3;
    int bary=by+46;
    QPixmap backgroundLeft,backgroundMiddle,backgroundRight;
    QPixmap barLeft,barMiddle,barRight;
    if(backgroundLeft.isNull() || backgroundLeft.height()!=8)
    {
        QPixmap background(":/CC/images/interface/mbb.png");
        if(background.isNull())
            abort();
        QPixmap bar;
        if(monster.hp>(stat.hp/2))
            bar=QPixmap(":/CC/images/interface/mbgreen.png");
        else if(monster.hp>(stat.hp/4))
            bar=QPixmap(":/CC/images/interface/mborange.png");
        else
            bar=QPixmap(":/CC/images/interface/mbred.png");
        if(bar.isNull())
            abort();
        backgroundLeft=background.copy(0,0,3,8);
        backgroundMiddle=background.copy(3,0,44,8);
        backgroundRight=background.copy(47,0,3,8);
        barLeft=bar.copy(0,0,2,8);
        barMiddle=bar.copy(2,0,46,8);
        barRight=bar.copy(48,0,2,8);
    }
    painter.drawPixmap(barx+0,bary+0,backgroundLeft.width(),    backgroundLeft.height(),    backgroundLeft);
    painter.drawPixmap(barx+backgroundLeft.width(),        bary+0,
                     50-backgroundLeft.width()-backgroundRight.width(),    backgroundLeft.height(),backgroundMiddle);
    painter.drawPixmap(barx+50-backgroundRight.width(),bary+0,                         backgroundRight.width(),    backgroundRight.height(),backgroundRight);

    int startX=0;
    int size=50-startX-startX;
    int inpixel=monster.hp*size/stat.hp;
    if(inpixel<(barLeft.width()+barRight.width()))
    {
        if(inpixel>0)
        {
            const int tempW=inpixel/2;
            if(tempW>0)
            {
                QPixmap barLeftC=barLeft.copy(0,0,tempW,barLeft.height());
                painter.drawPixmap(barx+startX,bary+0,barLeftC.width(),           barLeftC.height(),           barLeftC);
                const unsigned int pixelremaining=inpixel-barLeftC.width();
                if(pixelremaining>0)
                {
                    QPixmap barRightC=barRight.copy(barRight.width()-pixelremaining,0,pixelremaining,barRight.height());
                    painter.drawPixmap(barx+startX+barLeftC.width(),               bary+0,
                                 barRightC.width(),                  barRightC.height(),barRightC);
                }
            }
        }
    }
    else {
        painter.drawPixmap(barx+startX,bary+0,barLeft.width(),           barLeft.height(),           barLeft);
        const unsigned int pixelremaining=inpixel-(barLeft.width()+barRight.width());
        if(pixelremaining>0)
            painter.drawPixmap(barx+startX+barLeft.width(),          bary+0,                          pixelremaining,           barMiddle.height(),barMiddle);
        painter.drawPixmap(barx+startX+barLeft.width()+pixelremaining,  bary+0,barRight.width(),                  barRight.height(),barRight);
    }

    setPixmap(QPixmap::fromImage(cache));
}

void MapMonsterPreview::mousePressEventXY(const QPointF &p,bool &pressValidated)
{
    if(this->pressed)
        return;
    if(pressValidated)
        return;
    if(!isVisible())
        return;
    if(!isEnabled())
        return;
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(t.contains(p))
    {
        pressValidated=true;
        setPressed(true);
    }
}

void MapMonsterPreview::mouseReleaseEventXY(const QPointF &p, bool &previousPressValidated)
{
    if(previousPressValidated)
    {
        setPressed(false);
        return;
    }
    const QRectF &b=boundingRect();
    const QRectF &t=mapRectToScene(b);
    if(!this->pressed)
        return;
    if(!isEnabled())
        return;
    if(!previousPressValidated && isVisible())
    {
        if(t.contains(p))
        {
            emit clicked();
            previousPressValidated=true;
        }
    }
    setPressed(false);
}

void MapMonsterPreview::setPressed(const bool &pressed)
{
    this->pressed=pressed;
    //regenCache();
}

bool MapMonsterPreview::isPressed()
{
    return this->pressed;
}

const CatchChallenger::PlayerMonster &MapMonsterPreview::getMonster() const
{
    return monster;
}
