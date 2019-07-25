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

#include "../../client/qt/tiled/tiled_mapobject.h"
#include "../../client/qt/tiled/tiled_objectgroup.h"
#include "../../client/qt/render/MapVisualiserThread.h"

#include "../../general/base/GeneralStructures.h"
#include "../../general/base/CommonMap.h"
#include "../../client/qt/Map_client.h"
#include "../../general/base/Map_loader.h"

#include "../../client/qt/tiled/tiled_isometricrenderer.h"
#include "../../client/qt/tiled/tiled_map.h"
#include "../../client/qt/tiled/tiled_mapobject.h"
#include "../../client/qt/tiled/tiled_mapreader.h"
#include "../../client/qt/tiled/tiled_objectgroup.h"
#include "../../client/qt/tiled/tiled_orthogonalrenderer.h"
#include "../../client/qt/tiled/tiled_tilelayer.h"
#include "../../client/qt/tiled/tiled_tileset.h"

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
    Tiled::Tileset * getTileset(Tiled::Map * map,const QString &file);
    QString loadOtherMap(const QString &fileName);
    void loadCurrentMap(const QString &fileName,qint32 x, qint32 y);
};

#endif
