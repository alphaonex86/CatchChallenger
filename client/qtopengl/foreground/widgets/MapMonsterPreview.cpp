#include "MapMonsterPreview.hpp"
#include "../../cc/QtDatapackClientLoader.hpp"
#include "../../../qt/GameLoader.hpp"
#include "../../../../general/base/CommonDatapack.hpp"
#include "../../../../general/fight/CommonFightEngine.hpp"
#include <QPainter>

MapMonsterPreview::MapMonsterPreview(const CatchChallenger::PlayerMonster &monster, QGraphicsItem *parent) :
    QGraphicsPixmapItem(parent)
{
    QImage cache(QSize(80,80),QImage::Format_ARGB32_Premultiplied);
    cache.fill(Qt::transparent);
    QPainter painter(&cache);

    QPixmap background=*GameLoader::gameLoader->getImage(":/CC/images/interface/mgb.png");
    unsigned int bx=cache.width()/2-background.width()/2;
    unsigned int by=cache.height()/2-background.height()/2;
    painter.drawPixmap(bx,by,background.width(),background.height(),background);

    const QtDatapackClientLoader::QtMonsterExtra &monsterExtra=QtDatapackClientLoader::datapackLoader->QtmonsterExtra.at(monster.monster);
    QPixmap front=monsterExtra.front;
    painter.drawPixmap(0,0,front.width(),front.height(),front);

    const CatchChallenger::Monster &monsterGeneralInfo=CatchChallenger::CommonDatapack::commonDatapack.monsters.at(monster.monster);
    const CatchChallenger::Monster::Stat &stat=CatchChallenger::CommonFightEngine::getStat(monsterGeneralInfo,monster.level);

    QPixmap backgroundbar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbb.png");
    painter.drawPixmap(bx+3,by+46,backgroundbar.width(),backgroundbar.height(),backgroundbar);

    QPixmap bar;
    if(monster.hp>(stat.hp/2))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbgreen.png");
    else if(monster.hp>(stat.hp/4))
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mborange.png");
    else
        bar=*GameLoader::gameLoader->getImage(":/CC/images/interface/mbred.png");
    painter.drawPixmap(bx+3,by+46,bar.width()*stat.hp/monster.hp,bar.height(),bar);

    setPixmap(QPixmap::fromImage(cache));
}

MapMonsterPreview::~MapMonsterPreview()
{
}
