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

#include "mapobject.h"
#include "objectgroup.h"

#include "general/base/GeneralStructures.h"
#include "general/base/Map.h"
#include "general/base/Map_loader.h"

#include "isometricrenderer.h"
#include "map.h"
#include "mapobject.h"
#include "mapreader.h"
#include "objectgroup.h"
#include "orthogonalrenderer.h"
#include "tilelayer.h"
#include "tileset.h"

#ifndef TMXVIEWER_H
#define TMXVIEWER_H

#include <QGraphicsView>
#include <QTimer>
#include <QKeyEvent>

namespace Tiled {
class Map;
class MapRenderer;
}

#define STEPPERSO 1

class MapVisualiserQt : public QGraphicsView
{
    Q_OBJECT

public:
    explicit MapVisualiserQt(QWidget *parent = 0);
    ~MapVisualiserQt();
    void keyPressEvent(QKeyEvent * event);
    void keyReleaseEvent(QKeyEvent *event);
    void viewMap(const QString &fileName);

private:
    QGraphicsScene *mScene;
    Tiled::Map *mMap;
    Tiled::MapRenderer *mRenderer;
    Tiled::MapObject * playerMapObject;
    Tiled::Tileset * playerTileset;
    QTimer timer;
    float xPerso,yPerso;
    bool inMove;
    QTimer moveTimer;
    QTimer lookToMove;
    int moveStep;
    Pokecraft::Direction direction;
    Pokecraft::Map logicalMap;
    QSet<int> keyPressed;
private slots:
    void moveTile();
    void moveStepSlot(bool justUpdateTheTile=false);
    void transformLookToMove();
};

#endif // TMXVIEWER_H
