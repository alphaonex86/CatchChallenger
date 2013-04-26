/****************************************************************************
** Meta object code from reading C++ file 'MapVisualiserPlayer.h'
**
** Created: Fri Apr 26 14:18:58 2013
**      by: The Qt Meta Object Compiler version 63 (Qt 4.8.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../base/render/MapVisualiserPlayer.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MapVisualiserPlayer.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 63
#error "This file was generated using the moc from 4.8.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_MapVisualiserPlayer[] = {

 // content:
       6,       // revision
       0,       // classname
       0,    0, // classinfo
      21,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       6,       // signalCount

 // signals: signature, parameters, type, tag, flags
      35,   21,   20,   20, 0x05,
      93,   85,   20,   20, 0x05,
     157,   85,   20,   20, 0x05,
     221,  210,   20,   20, 0x05,
     263,   85,   20,   20, 0x05,
     328,  322,   20,   20, 0x05,

 // slots: signature, parameters, type, tag, flags
     343,   20,   20,   20, 0x09,
     359,   20,   20,   20, 0x09,
     379,   20,  374,   20, 0x09,
     400,   20,   20,   20, 0x09,
     432,  422,   20,   20, 0x09,
     480,   20,   20,   20, 0x09,
     501,   20,   20,   20, 0x09,
     517,   20,   20,   20, 0x09,
     544,   20,   20,   20, 0x09,
     573,   20,   20,   20, 0x09,
     585,   20,   20,   20, 0x09,
     615,  599,   20,   20, 0x09,
     643,   20,   20,   20, 0x09,
     660,  654,   20,   20, 0x09,
     714,  681,  374,   20, 0x09,

       0        // eod
};

static const char qt_meta_stringdata_MapVisualiserPlayer[] = {
    "MapVisualiserPlayer\0\0the_direction\0"
    "send_player_direction(CatchChallenger::Direction)\0"
    "map,x,y\0"
    "stopped_in_front_of(CatchChallenger::Map_client*,quint8,quint8)\0"
    "actionOn(CatchChallenger::Map_client*,quint8,quint8)\0"
    "blockOnVar\0blockedOn(MapVisualiserPlayer::BlockedOn)\0"
    "fightCollision(CatchChallenger::Map_client*,quint8,quint8)\0"
    "error\0error(QString)\0keyPressParse()\0"
    "moveStepSlot()\0bool\0haveStopTileAction()\0"
    "transformLookToMove()\0direction\0"
    "startGrassAnimation(CatchChallenger::Direction)\0"
    "stopGrassAnimation()\0loadGrassTile()\0"
    "loadPlayerFromCurrentMap()\0"
    "unloadPlayerFromCurrentMap()\0parseStop()\0"
    "parseAction()\0animationTilset\0"
    "setAnimationTilset(QString)\0resetAll()\0"
    "speed\0setSpeed(SPEED_TYPE)\0"
    "direction,map,x,y,checkCollision\0"
    "canGoTo(CatchChallenger::Direction,CatchChallenger::Map,COORD_TYPE,COO"
    "RD_TYPE,bool)\0"
};

void MapVisualiserPlayer::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        Q_ASSERT(staticMetaObject.cast(_o));
        MapVisualiserPlayer *_t = static_cast<MapVisualiserPlayer *>(_o);
        switch (_id) {
        case 0: _t->send_player_direction((*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[1]))); break;
        case 1: _t->stopped_in_front_of((*reinterpret_cast< CatchChallenger::Map_client*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 2: _t->actionOn((*reinterpret_cast< CatchChallenger::Map_client*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 3: _t->blockedOn((*reinterpret_cast< const MapVisualiserPlayer::BlockedOn(*)>(_a[1]))); break;
        case 4: _t->fightCollision((*reinterpret_cast< CatchChallenger::Map_client*(*)>(_a[1])),(*reinterpret_cast< const quint8(*)>(_a[2])),(*reinterpret_cast< const quint8(*)>(_a[3]))); break;
        case 5: _t->error((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->keyPressParse(); break;
        case 7: _t->moveStepSlot(); break;
        case 8: { bool _r = _t->haveStopTileAction();
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        case 9: _t->transformLookToMove(); break;
        case 10: _t->startGrassAnimation((*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[1]))); break;
        case 11: _t->stopGrassAnimation(); break;
        case 12: _t->loadGrassTile(); break;
        case 13: _t->loadPlayerFromCurrentMap(); break;
        case 14: _t->unloadPlayerFromCurrentMap(); break;
        case 15: _t->parseStop(); break;
        case 16: _t->parseAction(); break;
        case 17: _t->setAnimationTilset((*reinterpret_cast< QString(*)>(_a[1]))); break;
        case 18: _t->resetAll(); break;
        case 19: _t->setSpeed((*reinterpret_cast< const SPEED_TYPE(*)>(_a[1]))); break;
        case 20: { bool _r = _t->canGoTo((*reinterpret_cast< const CatchChallenger::Direction(*)>(_a[1])),(*reinterpret_cast< CatchChallenger::Map(*)>(_a[2])),(*reinterpret_cast< COORD_TYPE(*)>(_a[3])),(*reinterpret_cast< COORD_TYPE(*)>(_a[4])),(*reinterpret_cast< const bool(*)>(_a[5])));
            if (_a[0]) *reinterpret_cast< bool*>(_a[0]) = _r; }  break;
        default: ;
        }
    }
}

const QMetaObjectExtraData MapVisualiserPlayer::staticMetaObjectExtraData = {
    0,  qt_static_metacall 
};

const QMetaObject MapVisualiserPlayer::staticMetaObject = {
    { &MapVisualiser::staticMetaObject, qt_meta_stringdata_MapVisualiserPlayer,
      qt_meta_data_MapVisualiserPlayer, &staticMetaObjectExtraData }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &MapVisualiserPlayer::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *MapVisualiserPlayer::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *MapVisualiserPlayer::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_MapVisualiserPlayer))
        return static_cast<void*>(const_cast< MapVisualiserPlayer*>(this));
    return MapVisualiser::qt_metacast(_clname);
}

int MapVisualiserPlayer::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = MapVisualiser::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 21)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 21;
    }
    return _id;
}

// SIGNAL 0
void MapVisualiserPlayer::send_player_direction(const CatchChallenger::Direction & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MapVisualiserPlayer::stopped_in_front_of(CatchChallenger::Map_client * _t1, const quint8 & _t2, const quint8 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MapVisualiserPlayer::actionOn(CatchChallenger::Map_client * _t1, const quint8 & _t2, const quint8 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MapVisualiserPlayer::blockedOn(const MapVisualiserPlayer::BlockedOn & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MapVisualiserPlayer::fightCollision(CatchChallenger::Map_client * _t1, const quint8 & _t2, const quint8 & _t3)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)), const_cast<void*>(reinterpret_cast<const void*>(&_t3)) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MapVisualiserPlayer::error(const QString & _t1)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}
QT_END_MOC_NAMESPACE
