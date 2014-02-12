#ifndef MAP_VISUALISER_PLAYER_WITH_FIGHT_H
#define MAP_VISUALISER_PLAYER_WITH_FIGHT_H

#include "../../base/render/MapVisualiserPlayer.h"

#include <QSet>

class MapVisualiserPlayerWithFight : public MapVisualiserPlayer
{
    Q_OBJECT
public:
    explicit MapVisualiserPlayerWithFight(const bool &centerOnPlayer=true, const bool &debugTags=false, const bool &useCache=true, const bool &OpenGL=false);
    void setBotsAlreadyBeaten(const QSet<quint32> &botAlreadyBeaten);
    void addBeatenBotFight(const quint32 &botFightId);
    bool haveBeatBot(const quint32 &botFightId) const;
    void addRepelStep(const quint32 &repel_step);
    void setInformations(QHash<quint32,quint32> *items,QHash<quint32, CatchChallenger::PlayerQuest> *quests);
protected slots:
    virtual void keyPressParse();
    virtual bool haveStopTileAction();
    virtual bool canGoTo(const CatchChallenger::Direction &direction,CatchChallenger::CommonMap map,COORD_TYPE x,COORD_TYPE y,const bool &checkCollision);
    virtual void resetAll();
protected:
    QSet<quint32> botAlreadyBeaten;
    quint32 repel_step;
    QHash<quint32,quint32> *items;
    QHash<quint32, CatchChallenger::PlayerQuest> *quests;
signals:
    void repelEffectIsOver() const;
    void teleportConditionNotRespected(const QString &text) const;
};

#endif
