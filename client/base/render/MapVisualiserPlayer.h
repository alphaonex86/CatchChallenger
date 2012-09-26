#ifndef MAP_VISUALISER_PLAYER_H
#define MAP_VISUALISER_PLAYER_H

#include "MapVisualiser.h"
#include "../Api_protocol.h"

class MapVisualiserPlayer : public MapVisualiser
{
    Q_OBJECT

public:
    explicit MapVisualiserPlayer(Pokecraft::Api_protocol *client,const bool &centerOnPlayer=true,const bool &debugTags=false,const bool &useCache=true,const bool &OpenGL=false);
    ~MapVisualiserPlayer();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
protected:
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    int moveStep;
    Pokecraft::Direction direction;
    quint8 x,y;
    bool inMove;

    bool centerOnPlayer;

    QTimer timer;
    QTimer moveTimer;
    QTimer lookToMove;
    QSet<int> keyPressed;

    Pokecraft::Api_protocol *client;
private slots:
    void keyPressParse();

    void moveStepSlot();
    //have look into another direction, if the key remain pressed, apply like move
    void transformLookToMove();
protected:
    //call after enter on new map
    virtual void loadPlayerFromCurrentMap();
    //call before leave the old map (and before loadPlayerFromCurrentMap())
    virtual void unloadPlayerFromCurrentMap();
signals:
    void send_player_direction(const Pokecraft::Direction &the_direction);
};

#endif
