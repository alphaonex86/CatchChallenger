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

    QPixmap backgroundbar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbb.png");
    painter.drawPixmap(bx+3,by+46,backgroundbar.width(),backgroundbar.height(),backgroundbar);

    if(monster.hp==0)
    {
        QPixmap monsterko=QPixmap(":/CC/images/interface/monsterko.png");
        painter.drawPixmap(cache.width()/2-monsterko.width()/2,cache.height()/2-monsterko.height()/2,monsterko.width(),monsterko.height(),monsterko);
    }

    QPixmap bar;
    if(monster.hp>(stat.hp/2))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png");
    else if(monster.hp>(stat.hp/4))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mborange.png");
    else
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbred.png");
    painter.drawPixmap(bx+3,by+46,bar.width()*monster.hp/stat.hp,bar.height(),bar);

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
