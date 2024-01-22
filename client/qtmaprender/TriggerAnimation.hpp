#ifndef TriggerAnimation_H
#define TriggerAnimation_H

#include <QObject>
#include <QTimer>

#include <libtiled/mapobject.h>
#include <libtiled/tile.h>

class TriggerAnimation : public QObject
{
    Q_OBJECT
public:
    explicit TriggerAnimation(Tiled::MapObject* object,
                              const uint8_t &framesCountEnter, const uint16_t &msEnter,
                              const uint8_t &framesCountLeave, const uint16_t &msLeave,
                              const uint8_t &framesCountAgain, const uint16_t &msAgain,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              const uint8_t &framesCountEnter, const uint16_t &msEnter,
                              const uint8_t &framesCountLeave, const uint16_t &msLeave,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              Tiled::MapObject* objectOver,
                              const uint8_t &framesCountEnter, const uint16_t &msEnter,
                              const uint8_t &framesCountLeave, const uint16_t &msLeave,
                              const uint8_t &framesCountAgain, const uint16_t &msAgain,
                              bool over=false
                              );
    explicit TriggerAnimation(Tiled::MapObject* object,
                              Tiled::MapObject* objectOver,
                              const uint8_t &framesCountEnter, const uint16_t &msEnter,
                              const uint8_t &framesCountLeave, const uint16_t &msLeave,
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
    const uint8_t framesCountEnter;
    const uint16_t msEnter;
    const uint8_t framesCountLeave;
    const uint16_t msLeave;
    const uint8_t framesCountAgain;
    const uint16_t msAgain;
    bool over;
    uint16_t playerOnThisTile;
    bool firstTime;

    struct EnterEventCall
    {
        QTimer *timer;
        uint8_t frame;//0 to framesCount*2 -> open + close
    };
    QList<EnterEventCall> enterEvents;
    struct LeaveEventCall
    {
        QTimer *timer;
        uint8_t frame;//0 to framesCount*2 -> open + close
    };
    QList<LeaveEventCall> leaveEvents;
private:
    void setTileOffset(const uint8_t &offset);
private slots:
    void timerFinishEnter();
    void timerFinishLeave();
};

#endif // TriggerAnimation_H
