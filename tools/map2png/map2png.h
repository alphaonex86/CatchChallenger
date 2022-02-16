/*
 * tmxviewer.h
 * Copyright 2010, Thorbjørn Lindeijer <thorbjorn@lindeijer.nl>
 *
 * This file is part of the TMX Viewer example.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "../../client/tiled/tiled_mapobject.hpp"
#include "../../client/tiled/tiled_objectgroup.hpp"

#include "../../general/base/GeneralStructures.hpp"
#include "../../general/base/CommonMap.hpp"
#include "../../general/base/Map_loader.hpp"

#include "../../client/tiled/tiled_isometricrenderer.hpp"
#include "../../client/tiled/tiled_map.hpp"
#include "../../client/tiled/tiled_mapobject.hpp"
#include "../../client/tiled/tiled_mapreader.hpp"
#include "../../client/tiled/tiled_objectgroup.hpp"
#include "../../client/tiled/tiled_orthogonalrenderer.hpp"
#include "../../client/tiled/tiled_tilelayer.hpp"
#include "../../client/tiled/tiled_tileset.hpp"

#include "../../client/qtmaprender/Map_client.hpp"

#ifndef MAP_VISUALISER_H
#define MAP_VISUALISER_H

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>
#include <QGraphicsItem>
#include <QRectF>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QWidget>
#include <QSet>
#include <QMultiMap>

namespace Tiled {
class Map;
class MapRenderer;
}

#define STEPPERSO 1

class Map_full;
class TriggerAnimation;
class MapDoor;

class MapVisualiserOrder
{
public:
    struct Map_animation_object
    {
        Tiled::MapObject * animatedObject;
    };
    struct Map_animation
    {
        int minId;
        int maxId;
        std::vector<Map_animation_object> animatedObjectList;
    };
    struct TriggerAnimationContent
    {
        Tiled::Tile* objectTile;
        Tiled::Tile* objectTileOver;
        uint8_t framesCountEnter;
        uint16_t msEnter;
        uint8_t framesCountLeave;
        uint16_t msLeave;
        uint8_t framesCountAgain;
        uint16_t msAgain;
        bool over;
    };
    std::unordered_map<Tiled::Tile *,TriggerAnimationContent> tileToTriggerAnimationContent;

    explicit MapVisualiserOrder();
    ~MapVisualiserOrder();
    static void layerChangeLevelAndTagsChange(Map_full *tempMapObject, bool hideTheDoors=false);

    static std::string text_blockedtext;
    static std::string text_en;
    static std::string text_lang;
    static std::string text_Dyna_management;
    static std::string text_Moving;
    static std::string text_door;
    static std::string text_Object;
    static std::string text_bot;
    static std::string text_bots;
    static std::string text_WalkBehind;
    static std::string text_Collisions;
    static std::string text_Grass;
    static std::string text_animation;
    static std::string text_dotcomma;
    static std::string text_ms;
    static std::string text_frames;
    static std::string text_map;
    static std::string text_objectgroup;
    static std::string text_name;
    static std::string text_object;
    static std::string text_type;
    static std::string text_x;
    static std::string text_y;
    static std::string text_botfight;
    static std::string text_property;
    static std::string text_value;
    static std::string text_file;
    static std::string text_id;
    static std::string text_slash;
    static std::string text_dotxml;
    static std::string text_dottmx;
    static std::string text_properties;
    static std::string text_shop;
    static std::string text_learn;
    static std::string text_heal;
    static std::string text_fight;
    static std::string text_zonecapture;
    static std::string text_market;
    static std::string text_zone;
    static std::string text_fightid;
    static std::string text_randomoffset;
    static std::string text_visible;
    static std::string text_true;
    static std::string text_false;
    static std::string text_trigger;
protected:
    static QRegularExpression regexMs;
    static QRegularExpression regexFrames;
    static QRegularExpression regexTrigger;
    static QRegularExpression regexTriggerAgain;
};

class Map_full
{
public:
    Map_full();
public:
    CatchChallenger::Map_client logicalMap;
    Tiled::Map * tiledMap;
    Tiled::MapRenderer * tiledRender;
    Tiled::ObjectGroup * objectGroup;
    std::unordered_map<uint16_t/*ms*/,std::unordered_map<int/*minId*/,MapVisualiserOrder::Map_animation> > animatedObject;
    int objectGroupIndex;
    int relative_x,relative_y;//needed for the async load
    int relative_x_pixel,relative_y_pixel;
    bool displayed;
    std::unordered_map<std::pair<uint8_t,uint8_t>,MapDoor*,pairhash> doors;
    std::unordered_map<std::pair<uint8_t,uint8_t>,TriggerAnimation*,pairhash> triggerAnimations;
    std::string visualType;
    std::string name;
    std::string zone;
};

class MapItem : public QGraphicsItem
{
public:
    MapItem(QGraphicsItem *parent = 0);
    void addMap(Tiled::Map *map, Tiled::MapRenderer *renderer);
    void setMapPosition(Tiled::Map *map, qint16 x, qint16 y);
    QRectF boundingRect() const;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *);
private:
    QMultiMap<Tiled::Map *,QGraphicsItem *> displayed_layer;
};

class Map2Png : public QGraphicsView
{
    Q_OBJECT

public:
    explicit Map2Png(QWidget *parent = 0);
    ~Map2Png();
    void viewMap(const bool &renderAll,const QString &fileName, const QString &destination=QString());
    void displayMap();
    static QStringList listFolder(const QString& folder,const QString& suffix=QString());
    QString baseDatapack;
    QString mainDatapack;
    static QString text_slash;
    static QString text_dottmx;
    static QString text_dotpng;
    static QString text_Moving;
    static QString text_door;
    static QString text_Object;
    static QString text_bot;
    static QString text_skin;
    static QString text_fightertrainer;
    static QString text_lookAt;
    static QString text_empty;
    static QString text_top;
    static QString text_right;
    static QString text_left;
    static QString text_Collisions;
    static QString text_animation;
    static QString text_dotcomma;
    static QString text_ms;
    static QString text_frames;
    static QString text_visible;
    static QString text_false;
    static QString text_object;
    QRegularExpression regexMs;
    QRegularExpression regexFrames;
    QStringList folderListSkin;
private:
    struct Map_full_map2png
    {
        CatchChallenger::Map_client logicalMap;
        Tiled::Map * tiledMap;
        Tiled::MapRenderer * tiledRender;
        Tiled::ObjectGroup * objectGroup;
        qint32 x;
        qint32 y;
    };

    Tiled::MapReader reader;
    QGraphicsScene *mScene;
    MapItem* mapItem;
    QString mLastError;
    bool hideTheDoors;
    bool mRenderAll;

    QHash<QString,Map_full_map2png *> other_map;
private slots:
    static void layerChangeLevelAndTagsChange(Map_full *tempMapObject,bool hideTheDoors);
    Tiled::Tileset * getTileset(Tiled::Map * map,const QString &file);
    QString loadOtherMap(const QString &fileName);
    void loadCurrentMap(const QString &fileName,qint32 x, qint32 y);
};

#endif
