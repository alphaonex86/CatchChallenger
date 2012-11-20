#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.h"

#include <QSet>
#include <QString>
#include <QTimer>

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT

public:
    explicit MapVisualiserPlayer(const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiserPlayer();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    QString lastLocation() const;
protected:
    //player
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    int moveStep;
    Pokecraft::Direction direction;
    quint8 x,y;
    bool inMove;
    bool stepAlternance;
    QString mLastLocation;

    //display
    bool centerOnPlayer;

    //timer
    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;

    //control
    QSet<int> keyPressed;

    //grass
    bool haveGrassCurrentObject;
    Tiled::MapObject * grassCurrentObject;
    bool haveNextCurrentObject;
    Tiled::MapObject * nextCurrentObject;
    Tiled::Tileset * animationTileset;
private slots:
    void keyPressParse();

    void moveStepSlot();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();

    //grass
    virtual void startGrassAnimation(const Pokecraft::Direction &direction);
    virtual void stopGrassAnimation();
    void loadGrassTile();
protected slots:
    //call after enter on new map
    virtual void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadPlayerFromCurrentMap();

    void setAnimationTilset(QString animationTilset);
    virtual void resetAll();
signals:
    void send_player_direction(const Pokecraft::Direction &the_direction);
};

#endif
