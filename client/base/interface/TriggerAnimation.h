#ifndef TriggerAnimation_H
#define TriggerAnimation_H

#include <QObject>
#include <QTimer>

#include "../../tiled/tiled_mapobject.h"
#include "../../tiled/tiled_tile.h"
#include "../../tiled/tiled_tileset.h"

class TriggerAnimation : public QObject
{
    Q_OBJECT
public:
    explicit TriggerAnimation(Tiled::MapObject* object,
                              const quint8 &framesCountEnter, const quint16 &msEnter,
                              const quint8 &framesCountLeave, const quint16 &msLeave,
                              const quint8 &framesCountAgain, const quint16 &msAgain,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              const quint8 &framesCountEnter, const quint16 &msEnter,
                              const quint8 &framesCountLeave, const quint16 &msLeave,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              Tiled::MapObject* objectOver,
                              const quint8 &framesCountEnter, const quint16 &msEnter,
                              const quint8 &framesCountLeave, const quint16 &msLeave,
                              const quint8 &framesCountAgain, const quint16 &msAgain,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              Tiled::MapObject* objectOver,
                              const quint8 &framesCountEnter, const quint16 &msEnter,
                              const quint8 &framesCountLeave, const quint16 &msLeave,
                              bool over=false
                              );
    ~TriggerAnimation();
    void startEnter();
    void startLeave();
private:
    Tiled::MapObject* object;
    Tiled::MapObject* objectOver;
    Tiled::Cell cell;
    Tiled::Cell cellOver;
    const Tiled::Tile* baseTile;
    const Tiled::Tile* baseTileOver;
    const quint8 framesCountEnter;
    const quint16 msEnter;
    const quint8 framesCountLeave;
    const quint16 msLeave;
    const quint8 framesCountAgain;
    const quint16 msAgain;
    bool over;
    quint16 playerOnThisTile;
    bool firstTime;

    struct EnterEventCall
    {
        QTimer *timer;
        quint8 frame;//0 to framesCount*2 -> open + close
    };
    QList<EnterEventCall> enterEvents;
    struct LeaveEventCall
    {
        QTimer *timer;
        quint8 frame;//0 to framesCount*2 -> open + close
    };
    QList<LeaveEventCall> leaveEvents;
private:
    void setTileOffset(const quint8 &offset);
private slots:
    void timerFinishEnter();
    void timerFinishLeave();
};

#endif // TriggerAnimation_H
